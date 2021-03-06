// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include <sstream>
#include "test/catch.hpp"
#include "OrderingStreamWriter.hpp"
#include <test/EnterpriseDiodeTestHelpers.hpp>
#include "StreamSpy.hpp"

TEST_CASE("OrderingStreamWriter. Packet streams are written to the packet queue")
{
  std::stringstream outputStream;
  auto streamWriter = OrderingStreamWriter(1, 1, std::make_unique<StreamSpy>(outputStream, 1), []() { return 10000; },
                                           DiodeType::basic);

  auto packet = parsePacket(createTestPacketStream(1, 1, false), {'A', 'B'});

  streamWriter.write(std::move(packet));
  REQUIRE(outputStream.str() == "AB");
}

TEST_CASE("OrderingStreamWriter. Import diode - packet stream and cloakedDaggerHeader are written to the stream")
{
  std::stringstream outputStream;
  auto streamWriter = OrderingStreamWriter(1, 1, std::make_unique<StreamSpy>(outputStream, 1), []() { return 10000; },
                                           DiodeType::import);

  auto packet = parsePacket(createTestPacketStream(1, 1, false, true), {'A', 'B'});

  streamWriter.write(std::move(packet));
  REQUIRE(outputStream.str() == CDWrappedHeaderString + "AB");
}

TEST_CASE("OrderingStreamWriter. Write returns true when the eof has been received")
{
  std::stringstream outputStream;
  auto streamWriter = OrderingStreamWriter(1, 5, std::make_unique<StreamSpy>(outputStream, 1), []() { return 10000; },
                                           DiodeType::basic);

  SECTION("When the EOF packet is not queued")
  {
    auto packet = parsePacket(createTestPacketStream(1, 1, false), {'A', 'B'});

    const std::string filename = "{name: !str \"testFilename\"}";
    auto packet2 = parsePacket(createTestPacketStream(1, 2, true), {filename.begin(), filename.end()});

    REQUIRE_FALSE(streamWriter.write(std::move(packet)));
    REQUIRE(streamWriter.write(std::move(packet2)));
    REQUIRE(outputStream.str() == "AB");
  }

  SECTION("When the EOF packet is queued")
  {
    std::string filename = "{name: !str \"testFilename\"}";
    std::vector<char> vcFilename(filename.begin(), filename.end());
    auto packetC = parsePacket(createTestPacketStream(1, 3, true), {filename.begin(), filename.end()});

    REQUIRE_FALSE(streamWriter.write(std::move(packetC)));
    REQUIRE(outputStream.str().empty());

    auto packetA = parsePacket(createTestPacketStream(1, 2, false), {'C', 'D'});

    REQUIRE_FALSE(streamWriter.write(std::move(packetA)));
    REQUIRE(outputStream.str().empty());

    auto packetB = parsePacket(createTestPacketStream(1, 1, false), {'A', 'B'});

    REQUIRE(streamWriter.write(std::move(packetB)));
    REQUIRE(outputStream.str() == "ABCD");
  }
}

TEST_CASE(
  "OrderingStreamWriter. OrderingStreamWriter constructor sets timeLastUpdated to the the time returned by getTime")
{
  std::stringstream outputStream;
  std::uint32_t initialTime = 500;
  OrderingStreamWriter orderingStreamWriter(1,
                                            1,
                                            std::make_unique<StreamSpy>(outputStream, 1),
                                            [&initialTime]() mutable { return initialTime; }, DiodeType::basic);

  REQUIRE(orderingStreamWriter.timeLastUpdated == 500);

  SECTION("When the stream is written to, timeLastUpdated is updated")
  {
    auto packet = parsePacket(createTestPacketStream(1, 1, false), {'A', 'B'});
    initialTime = 501;

    REQUIRE(orderingStreamWriter.timeLastUpdated == 500);
    orderingStreamWriter.write(std::move(packet));
    REQUIRE(orderingStreamWriter.timeLastUpdated == 501);
    REQUIRE(outputStream.str() == "AB");
  }
}