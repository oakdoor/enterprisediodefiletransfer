// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "Packet.hpp"
#include "ReorderPackets.hpp"
#include "StreamSpy.hpp"
#include "test/catch.hpp"
#include <rewrapper/UnwrapperTestHelpers.hpp>
#include <boost/thread.hpp>
#include <future>

inline void waitForQueue(ReorderPackets& queueManager)
{
  while (!queueManager.isDone()) { usleep(100); };
}

TEST_CASE("ReorderPackets. Packets received in order are written to the output")
{
  std::stringstream outputStream;
  bool notused1;
  bool notused2;
  StreamSpy stream(outputStream, 1, notused1, notused2);
  auto queueManager = ReorderPackets(4, 1024, DiodeType::basic);
  queueManager.write({HeaderParams{0, 1, false, {}}, {'B', 'C'}}, &stream);

  queueManager.write({HeaderParams{0, 2, true, {}}, {}}, &stream);
  waitForQueue(queueManager);
  REQUIRE(queueManager.isDone());
  REQUIRE(outputStream.str() == "BC");
}

TEST_CASE("ReorderPackets. Handling filename", "[tagged]")
{

  bool notused1;
  bool notused2;
  std::stringstream outputStream;
  StreamSpy stream(outputStream, 1, notused1, notused2);
  auto queueManager = ReorderPackets(4, 1024, DiodeType::basic);

  SECTION("good filename")
  {
    auto inputStream = std::string("{name: !str \"testFilename\"}");
    queueManager.write({HeaderParams{0, 1, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
    waitForQueue(queueManager);
    REQUIRE(stream.storedFilename == "testFilename");
  }

  SECTION("Handling filename with length > maxFilenameLength, 65")
  {
    auto inputStream = std::string("{name: !str \"testFilenametestFilenametestFilenametestFilenametestFilenametestFilename\"}");
    queueManager.write({HeaderParams{0, 1, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
    waitForQueue(queueManager);
    REQUIRE(stream.storedFilename == "rejected.12345");
  }
}

TEST_CASE("ReorderPackets. ReorderPackets can be shut down with queue still populated")
{

  std::stringstream outputStream;
  bool notused1;
  bool notused2;
  StreamSpy stream(outputStream, 1, notused1, notused2);
  {
  auto queueManager = ReorderPackets(4, 1024, DiodeType::basic);
  queueManager.write({HeaderParams{0, 2, false, {}}, {'B', 'C'}}, &stream);
  }
  // If thread was still running this would have blocked forever
}

TEST_CASE("ReorderPackets. Out-of-order packets")
{
  std::stringstream outputStream;
  bool notused1;
  bool notused2;
  StreamSpy stream(outputStream, 1, notused1, notused2);
  auto queueManager = ReorderPackets(4, 1024, DiodeType::basic);

  SECTION("Frame 2 and 3 are not written to the output if frame 1 missing")
  {
    auto inputStream = std::string("BC");
    queueManager.write({HeaderParams {0, 2, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
    REQUIRE(outputStream.str().empty());

    inputStream = std::string("DE");
    queueManager.write({HeaderParams {0, 3, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
    REQUIRE(outputStream.str().empty());

    SECTION("After frame 1 arrives, frame 2 and 3 are written to the output")
    {
      inputStream = std::string("ZA");
      queueManager.write({HeaderParams {0, 1, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
      waitForQueue(queueManager);
      REQUIRE(queueManager.isDone());
      REQUIRE(outputStream.str() == "ZABCDE");

      SECTION("Frame 4 is EOF and is written to to the output and write returns true")
      {
        inputStream = std::string("{name: !str \"testFilename\"}");
        queueManager.write({HeaderParams {0, 4, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
        waitForQueue(queueManager);
        REQUIRE(outputStream.str() == "ZABCDE");
      }

//      SECTION("Frame 5 is eof with filename, but waits for all missing packets before setting filename.")
//      {
//        inputStream = std::string("{name: !str \"testFilename\"}");
//        queueManager.write({HeaderParams {0, 5, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDE");
//        REQUIRE_FALSE(stream.storedFilename == "testFilename");
//        inputStream = std::string("FG");
//        queueManager.write({HeaderParams {0, 4, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDEFG");
//        REQUIRE(stream.storedFilename == "testFilename");
//      }
//
//      SECTION("Frame 4 is EOF and a frame with frameCount above the eofFrame is ignored.")
//      {
//        inputStream = std::string("YZ");
//        queueManager.write({HeaderParams {0, 99, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDE");
//
//        inputStream = std::string("{name: !str \"testFilename\"}");
//        queueManager.write({HeaderParams {0, 4, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDE");
//      }
//
//      SECTION("Frame 4 is EOF and a frame with frameCount above the eofFrame and eof true is ignored.")
//      {
//        inputStream = std::string("");
//        queueManager.write({HeaderParams {0, 99, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDE");
//
//        inputStream = std::string("{name: !str \"testFilename\"}");
//        queueManager.write({HeaderParams {0, 4, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDE");
//      }
//
//      SECTION("Frame 5 is EOF and must be queued, "
//              "a spurious eof packet with a higher frameCount does not reassign the eofFrameNumber.")
//      {
//        inputStream = std::string("{name: !str \"testFilename\"}");
//        queueManager.write({HeaderParams {0, 5, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDE");
//        inputStream = std::string("{name: !str \"wrongFilename\"}");
//        queueManager.write({HeaderParams {0, 99, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDE");
//        inputStream = std::string("FG");
//        queueManager.write({HeaderParams {0, 4, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//        WAIT_FOR_FUTURE;
//        REQUIRE(outputStream.str() == "ZABCDEFG");
//
//        REQUIRE(stream.storedFilename == "testFilename");
//      }
    }
  }
}
//
//  SECTION("Packets are held in the queue until all previous packets are received")
//  {
//    auto inputStream = std::string("BC");
//    queueManager.write({HeaderParams{0, 3, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    REQUIRE(outputStream.str().empty());
//
//    inputStream = std::string("{name: !str \"testFilename\"}");
//    queueManager.write({HeaderParams{0, 4, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    REQUIRE(outputStream.str().empty());
//
//    inputStream = std::string("ZA");
//    queueManager.write({HeaderParams{0, 1, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    REQUIRE(outputStream.str() == "ZA");
//
//    inputStream = std::string("12");
//    queueManager.write({HeaderParams{0, 2, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    REQUIRE(outputStream.str() == "ZA12BC");
//  }
//
//  SECTION("ReorderPackets indicates when all packets have been output to stream")
//  {
//    SECTION("In-order EOF closes stream immediately")
//    {
//      auto inputStream = std::string("{name: !str \"testFilename\"}");
//      queueManager.write({HeaderParams{0, 1, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//      WAIT_FOR_FUTURE;
//      REQUIRE(outputStream.str() == "");
//    }
//
//    SECTION("Out-of-order EOF closes stream after all other packets have been received")
//    {
//      auto inputStream = std::string("BC");
//      queueManager.write({HeaderParams{0, 2, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//      WAIT_FOR_FUTURE;
//      REQUIRE(outputStream.str().empty());
//
//      inputStream = std::string("{name: !str \"testFilename\"}");
//      queueManager.write({HeaderParams{0, 4, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//      WAIT_FOR_FUTURE;
//      REQUIRE(outputStream.str().empty());
//
//      inputStream = std::string("ZA");
//      queueManager.write({HeaderParams{0, 1, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//      WAIT_FOR_FUTURE;
//      REQUIRE(outputStream.str() == "ZABC");
//
//      inputStream = std::string("DE");
//      queueManager.write({HeaderParams{0, 3, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//      WAIT_FOR_FUTURE;
//      REQUIRE(outputStream.str() == "ZABCDE");
//    }
//  }
//}
//
//TEST_CASE("ReorderPackets. Import diode.")
//{
//  std::promise<int> isStreamClosedPromise;
//  std::future<int> isStreamClosedFuture = isStreamClosedPromise.get_future();
//
//  std::stringstream outputStream;
//  bool notused1;
//  bool notused2;
//  StreamSpy stream(outputStream, 1, notused1, notused2);
//  auto queueManager = ReorderPackets(4, 1024, DiodeType::import, std::move(isStreamClosedPromise), 65);
//
//  SECTION("Data which is not wrapped, sisl or bitmap throws an error")
//  {
//    auto inputStream1 = std::string("AC");    // bad SISL
//    auto inputStream2 = std::string("BC");    // good SISL
//    queueManager.write({HeaderParams{0, 1, false, {}}, {inputStream1.begin(), inputStream1.end()}}, &stream);
//    queueManager.write({HeaderParams{0, 1, false, {}}, {inputStream2.begin(), inputStream2.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    REQUIRE("BC" == outputStream.str());
//  }
//
//  SECTION("Data with blank CDHeader starting with sisl or bmp starting char remains unchanged.")
//  {
//    auto inputStream = std::string("{A");
//    queueManager.write({HeaderParams{0, 1, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    REQUIRE(outputStream.str() == "{A");
//
//    inputStream = std::string("BC");
//    queueManager.write({HeaderParams{0, 2, false, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//
//    REQUIRE(outputStream.str() == "{ABC");
//  }
//
//  SECTION("The first frame of wrapped data remains unchanged")
//  {
//    auto wrappedInputStream = createTestWrappedString("abc", {0x12, 0x34, 0x56, 0x78, static_cast<char>(0x9a), static_cast<char>(0xbc), static_cast<char>(0xde), static_cast<char>(0xf0)});
//    queueManager.write({HeaderParams{0, 1, false, wrappedInputStream.header}, {wrappedInputStream.message.begin(), wrappedInputStream.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//
//    std::string firstFrame{wrappedInputStream.header.begin(), wrappedInputStream.header.end()};
//    firstFrame.insert(firstFrame.end(), wrappedInputStream.message.begin(), wrappedInputStream.message.end());
//    REQUIRE(outputStream.str() == firstFrame);
//  }
//
//  SECTION("A two frame wrapped file is rewrapped with the key from the first frame.")
//  {
//    auto wrappedInputStream = createTestWrappedString("abc", {0x12, 0x34, 0x56, 0x78, static_cast<char>(0x9a),
//                                                              static_cast<char>(0xbc), static_cast<char>(0xde),
//                                                              static_cast<char>(0xf0)});
//    queueManager.write({HeaderParams{0, 1, false, wrappedInputStream.header},
//                        {wrappedInputStream.message.begin(), wrappedInputStream.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    auto wrappedInputStream2 = createTestWrappedString("def", {static_cast<char>(0xf0), 0x34, 0x56, 0x78,
//                                                               static_cast<char>(0x9a), static_cast<char>(0xbc),
//                                                               static_cast<char>(0xde), 0x12});
//    queueManager.write({HeaderParams{0, 2, false, wrappedInputStream2.header},
//                        {wrappedInputStream2.message.begin(), wrappedInputStream2.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    std::stringstream unwrappedStream;
//    unwrapFromStream(outputStream, unwrappedStream);
//    REQUIRE(unwrappedStream.str() == "abcdef");
//  }
//
//  SECTION("Two wrapped frames out of order are rewrapped with the key from frame with frameCount 1.")
//  {
//    auto wrappedInputStream = createTestWrappedString("def", {0x12, 0x34, 0x56, 0x78, static_cast<char>(0x9a), static_cast<char>(0xbc), static_cast<char>(0xde), static_cast<char>(0xf0)});
//    queueManager.write({HeaderParams{0, 2, false, wrappedInputStream.header}, {wrappedInputStream.message.begin(), wrappedInputStream.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    auto wrappedInputStream2 = createTestWrappedString("abc", {static_cast<char>(0xf0), 0x34, 0x56, 0x78, static_cast<char>(0x9a), static_cast<char>(0xbc), static_cast<char>(0xde), 0x12});
//    queueManager.write({HeaderParams{0, 1, false, wrappedInputStream2.header}, {wrappedInputStream2.message.begin(), wrappedInputStream2.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//
//    std::stringstream unwrappedStream;
//    unwrapFromStream(outputStream, unwrappedStream);
//    REQUIRE(unwrappedStream.str() == "abcdef");
//  }
//
//  SECTION("A three frame wrapped file is rewrapped with the key from the first frame.")
//  {
//    auto wrappedInputStream = createTestWrappedString("abc", {0x12, 0x34, 0x56, 0x78, static_cast<char>(0x9a), static_cast<char>(0xbc), static_cast<char>(0xde), static_cast<char>(0xf0)});
//    queueManager.write({HeaderParams{0, 1, false, wrappedInputStream.header}, {wrappedInputStream.message.begin(), wrappedInputStream.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    auto wrappedInputStream2 = createTestWrappedString("def", {static_cast<char>(0xf0), 0x34, 0x56, 0x78, static_cast<char>(0x9a), static_cast<char>(0xbc), static_cast<char>(0xde), 0x12});
//    queueManager.write({HeaderParams{0, 2, false, wrappedInputStream2.header}, {wrappedInputStream2.message.begin(), wrappedInputStream2.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    auto wrappedInputStream3 = createTestWrappedString("ghi", {static_cast<char>(0xf5), 0x34, 0x56, 0x78, static_cast<char>(0x9a), static_cast<char>(0xbc), static_cast<char>(0xde), 0x34});
//    queueManager.write({HeaderParams{0, 3, false, wrappedInputStream3.header}, {wrappedInputStream3.message.begin(), wrappedInputStream3.message.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//    std::string inputStream = std::string("{name: !str \"testFilename\"}");
//    queueManager.write({HeaderParams{0, 4, true, {}}, {inputStream.begin(), inputStream.end()}}, &stream);
//    WAIT_FOR_FUTURE;
//
//    std::stringstream unwrappedStream;
//    unwrapFromStream(outputStream, unwrappedStream);
//    REQUIRE(unwrappedStream.str() == "abcdefghi");
//    REQUIRE(stream.storedFilename == "testFilename");
//  }
//}
