// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include <cstdint>
#include <vector>
#include <string>

#include "test/catch.hpp"
#include "test/EnterpriseDiodeTestHelpers.hpp"
#include "diodeheader/EnterpriseDiodeHeader.hpp"
#include "Server.hpp"
#include "StreamSpy.hpp"

#define WAIT_FOR_ASYNC_THREAD usleep(10000)

namespace testers
{
inline bool waitForResult(std::function<bool()> func)
{
  const std::int32_t tightWait(10);
  std::int32_t timeout(100000);
  while (!func())
  {
    usleep(10);
    timeout = timeout - tightWait;
    if (timeout <= 0)
    {
      return false;
    }
  }

  return true;
}
}

TEST_CASE("ED server.")
{
  std::stringstream outputStream;
  std::uint32_t capturedSessionId = 0;
  boost::asio::io_service fake_service;

  Server edServer = createEdServer(std::make_unique<UdpServerFake>(0, fake_service, 0, 0), 16, 100,
                                   capturedSessionId, outputStream, DiodeType::basic);


  SECTION("A packet received by the server is written to the stream")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false), {'B'});

    REQUIRE(testers::waitForResult([&outputStream](){ return outputStream.str() == std::string("B");}));
    REQUIRE(outputStream.str() == std::string("B"));

    const std::string filename = "{name: !str \"testFilename\"}";
    edServer.receivePacket(createTestPacketStream(1, 2, true), {filename.begin(), filename.end()});

    testers::waitForResult([&](){ return outputStream.str() == std::string("B");});
  }

  SECTION("Packets received are inserted into the same stream until the EOF flag is raised")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false), {'B', 'C'});
    edServer.receivePacket(createTestPacketStream(1, 2, false), {'D'});
    testers::waitForResult([&](){ return outputStream.str() == std::string("BCD");});
    REQUIRE(outputStream.str() == std::string("BCD"));
    std::string filename = "{name: !str \"testFilename\"}";
    edServer.receivePacket(createTestPacketStream(1, 3, true), {filename.begin(), filename.end()} );
    REQUIRE(outputStream.str() == std::string("BCD"));
  }

  SECTION("Packets are reordered before being written to stream")
  {
    edServer.receivePacket(createTestPacketStream(1, 2, false), {'C', 'D'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == std::string(""));

    const std::string filename = "{name: !str \"testFilename\"}";
    edServer.receivePacket(createTestPacketStream(1, 3, true), {filename.begin(), filename.end()});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == std::string(""));

    SECTION("The first frame is received and both are written to the output")
    {
      edServer.receivePacket(createTestPacketStream(1, 1, false), {'A', 'B'});
      WAIT_FOR_ASYNC_THREAD;
      REQUIRE(outputStream.str() == std::string("ABCD"));
    }
  }

  SECTION("Packets with invalid headers are not written to the output")
  {
    edServer.receivePacket(BytesBuffer(EnterpriseDiode::HeaderSizeInBytes - 1), {});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == std::string(""));

    SECTION("Subsequent packet with valid header is written to the output")
    {
      edServer.receivePacket(createTestPacketStream(1, 1, false),{'X', ' '} );
      WAIT_FOR_ASYNC_THREAD;
      REQUIRE(outputStream.str() == std::string("X "));
    }
  }

  SECTION("Session ID is passed to the stream manager")
  {
    edServer.receivePacket(createTestPacketStream(2, 1, false), {'B', 'C'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(capturedSessionId == 2);
    REQUIRE(outputStream.str() == std::string("BC"));
  }

  SECTION("Basic Diode server does not write the CDHeader even if it implies wrapping")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false, true), {'A'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == "A");
  }
}

TEST_CASE("ED server. Import Diode")
{
  std::stringstream outputStream;
  std::uint32_t capturedSessionId = 0;
  boost::asio::io_service fake_service;
  Server edServer = createEdServer(std::make_unique<UdpServerFake>(0, fake_service, 0, 0), 16, 100,
                                   capturedSessionId, outputStream, DiodeType::import);


  SECTION("Blank CDHeader and starting bmp char is written to disk without CDHeader")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false, false), {'B'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == "B");
  }

  SECTION("Blank CDHeader and starting sisl char is written to disk without CDHeader")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false, false), {'{'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == "{");
  }

  SECTION("Blank CDHeader and starting char not sisl or bmp char is not written.")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false, false), {'A'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str().empty());
  }

  SECTION("Import Diode server writes the CDHeader if it is the first frame and is wrapped")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false, true), {'\x11'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == CDWrappedHeaderString + "\x11");

    SECTION("Check the second wrapped packet is rewrapped and the CDHeader is not rewritten")
    {
      edServer.receivePacket(createTestPacketStream(1, 2, false, true), {'\x12'});
      WAIT_FOR_ASYNC_THREAD;
      REQUIRE_FALSE(outputStream.str() == CDWrappedHeaderString + "\x11\x12");
      REQUIRE(outputStream.str().size() == CDWrappedHeaderString.size() + 2);
    }
  }
}

TEST_CASE("ED server. Queue length Tests")
{
  std::stringstream outputStream;
  std::uint32_t capturedSessionId = 0;
  boost::asio::io_service fake_service;

  Server edServer = createEdServer(std::make_unique<UdpServerFake>(0, fake_service, 0, 0), 16, 2,
                                   capturedSessionId, outputStream, DiodeType::basic);

  SECTION("Packets are not queued if the size of the queue is at least maxQueueLength.")
  {
    edServer.receivePacket(createTestPacketStream(1, 1, false), {'A', 'B'});
    edServer.receivePacket(createTestPacketStream(1, 3, false),{'E', 'F'});
    edServer.receivePacket(createTestPacketStream(1, 4, false), {'G', 'H'});
    edServer.receivePacket(createTestPacketStream(1, 5, false), {'I', 'J'});
    std::string filename = "{name: !str \"testFilename\"}";
    edServer.receivePacket(createTestPacketStream(1, 6, true), {filename.begin(), filename.end()});
    edServer.receivePacket(createTestPacketStream(1, 2, false), {'C', 'D'});
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStream.str() == std::string("AB"));
  }
}

TEST_CASE("ED server. Stream is closed if timeout is exceeded.", "[integration]")
{
  std::stringstream outputStream;
  std::uint32_t capturedSessionId = 0;
  boost::asio::io_service fake_service;

  Server edServer = Server(std::make_unique<UdpServerFake>(0, fake_service, 0, 0), 16, 2,
                           [&outputStream, &capturedSessionId](std::uint32_t sessionId) {
                             capturedSessionId = sessionId;
                             return std::make_unique<StreamSpy>(outputStream, sessionId);
                           }, []() { return std::time(nullptr); }, 3, DiodeType::basic);

  edServer.receivePacket(createTestPacketStream(1, 1, false), {'A', 'B'});
  WAIT_FOR_ASYNC_THREAD;
  REQUIRE(outputStream.str() == std::string("AB"));
  sleep(4);

  std::string filename = "{name: !str \"testFilename\"}";
  edServer.receivePacket(createTestPacketStream(1, 2, true), {filename.begin(), filename.end()});
  WAIT_FOR_ASYNC_THREAD;
  REQUIRE(outputStream.str() == std::string("AB"));
}
