// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include <sstream>
#include <test/EnterpriseDiodeTestHelpers.hpp>
#include "test/catch.hpp"
#include "SessionManager.hpp"
#include "StreamSpy.hpp"

#define WAIT_FOR_ASYNC_THREAD   usleep(10000);

TEST_CASE("SessionManager.")
{
  std::vector<std::stringstream> outputStreams;
  std::uint32_t capturedSessionId = 0;

  bool fileDeletedWasCalled = false;
  bool fileRenameWasCalled = false;

  auto streamSpyCreator = [&](std::uint32_t sessionId) {
    capturedSessionId = sessionId;
    outputStreams.emplace_back(std::stringstream());
    return std::make_unique<StreamSpy>(outputStreams.back(), capturedSessionId, fileDeletedWasCalled, fileRenameWasCalled);
  };

  SECTION("SessionManager writes to a new stream given a new sessionId.")
  {
    auto fakeGetTime = []() { return 10000; };
    auto sessionManager = SessionManager(10, 10, streamSpyCreator, fakeGetTime, 5, DiodeType::basic);

    REQUIRE(outputStreams.empty());
    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 1, false), {'B', 'C'}));
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStreams.at(0).str() == std::string("BC"));
    REQUIRE(capturedSessionId == 1);
    REQUIRE_FALSE(fileDeletedWasCalled);
    REQUIRE_FALSE(fileRenameWasCalled);

    SECTION("SessionManager renames file and closes stream given EOF packet")
    {
      const std::string filename = "{name: !str \"testFilename\"}";
      sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 2, true), {filename.begin(), filename.end()}));
      WAIT_FOR_ASYNC_THREAD;

      REQUIRE(outputStreams.at(0).str() == std::string("BC"));
      REQUIRE(fileRenameWasCalled);

      SECTION("After a session is closed, new packets with the same sessionID are written to a new stream")
      {
        sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 1, false), {'F', 'G'}));
        WAIT_FOR_ASYNC_THREAD;
        REQUIRE(outputStreams.at(1).str() == std::string("FG"));
      }
    }
  }

  SECTION("SessionManager with import diode includes CDHeader writing to a new stream given a new sessionId.")
  {
    auto fakeGetTime = []() { return 10000; };
    auto sessionManager = SessionManager(10, 10, streamSpyCreator, fakeGetTime, 5, DiodeType::import);

    REQUIRE(outputStreams.empty());
    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 1, false, true), {'B', 'C'}));
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStreams.at(0).str() == CDWrappedHeaderString + "BC");
    REQUIRE(capturedSessionId == 1);
    REQUIRE_FALSE(fileDeletedWasCalled);
    REQUIRE_FALSE(fileRenameWasCalled);

    SECTION("SessionManager renames file and closes stream given EOF packet")
    {
      const std::string filename = "{name: !str \"testFilename\"}";
      sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 2, true, true), {filename.begin(), filename.end()}));
      WAIT_FOR_ASYNC_THREAD;

      REQUIRE(outputStreams.at(0).str() == CDWrappedHeaderString + "BC");
      REQUIRE(fileRenameWasCalled);

      SECTION("After a session is closed, new packets with the same sessionID are written to a new stream")
      {
        sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 1, false, true), {'F', 'G'}));
        WAIT_FOR_ASYNC_THREAD;
        REQUIRE(outputStreams.at(1).str() == CDWrappedHeaderString + "FG");
      }
    }
  }

  SECTION("SessionManager writes to 2 streams given 2 different sessionIds.")
  {
    auto fakeGetTime = []() { return 10000; };
    auto sessionManager = SessionManager(10, 10, streamSpyCreator, fakeGetTime, 5, DiodeType::basic);

    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 1, false), {'B', 'C'}));
    WAIT_FOR_ASYNC_THREAD;

    REQUIRE(outputStreams.at(0).str() == std::string("BC"));
    REQUIRE(capturedSessionId == 1);

    sessionManager.writeToStream(parsePacket(createTestPacketStream(2, 1, false), {'D', 'E'}));
    WAIT_FOR_ASYNC_THREAD;

    REQUIRE(outputStreams.at(1).str() == std::string("DE"));
    REQUIRE(capturedSessionId == 2);
  }

  SECTION("SessionManager doesn't write to an expired stream, but deletes the file and closes the stream.")
  {
    std::uint32_t initialTime = 500;
    auto sessionManager = SessionManager(
      10, 10, streamSpyCreator, [&initialTime]() mutable { return initialTime; }, 15, DiodeType::basic);

    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 1, false), {'B', 'C'}));
    WAIT_FOR_ASYNC_THREAD;

    REQUIRE(outputStreams.at(0).str() == std::string("BC"));
    REQUIRE(capturedSessionId == 1);
    REQUIRE_FALSE(fileDeletedWasCalled);

    std::uint32_t secondsSinceFirstPacketSent = 20;
    initialTime += secondsSinceFirstPacketSent;

    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 2, false), {'D', 'F'}));
    WAIT_FOR_ASYNC_THREAD;
    REQUIRE(outputStreams.at(0).str() == std::string("BC"));
    REQUIRE(fileDeletedWasCalled);
    REQUIRE_FALSE(fileRenameWasCalled);
  }

  SECTION("SessionManager doesn't rename file when queue length is exceeded.")
  {
    auto fakeGetTime = []() { return 10000; };
    auto sessionManager = SessionManager(10, 2, streamSpyCreator, fakeGetTime, 15, DiodeType::basic);

    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 1, false), {'B', 'C'}));
    WAIT_FOR_ASYNC_THREAD;

    REQUIRE(outputStreams.at(0).str() == std::string("BC"));
    REQUIRE(capturedSessionId == 1);

    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 3, false), {'F', 'G'}));
    WAIT_FOR_ASYNC_THREAD;
    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 4, false), {'H', 'I'}));
    WAIT_FOR_ASYNC_THREAD;
    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 5, false), {'J', 'K'}));
    WAIT_FOR_ASYNC_THREAD;
    sessionManager.writeToStream(parsePacket(createTestPacketStream(1, 2, false), {'D', 'E'}));
    WAIT_FOR_ASYNC_THREAD;

    REQUIRE_FALSE(fileRenameWasCalled);
  }
}
