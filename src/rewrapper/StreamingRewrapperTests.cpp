// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include <src/test/catch.hpp>
#include "BytesBuffer.hpp"
#include "UnwrapperTestHelpers.hpp"
#include "StreamingRewrapper.hpp"
#include "CloakedDagger.hpp"

TEST_CASE("StreamingRewrapper. Wrapped files should remain wrapped")
{
  StreamingRewrapper streamingRewrapper;
  auto header = std::array<char, CloakedDagger::headerSize()>({static_cast<char>(0xd1), static_cast<char>(0xdf), 0x5f, static_cast<char>(0xff), // magic1
                                                               0x00, 0x01, // major version
                                                               0x00, 0x00, // minor version
                                                               0x00, 0x00, 0x00, 0x30, // total length
                                                               0x00, 0x00, 0x00, 0x01, // encoding type
                                                               0x00, 0x03, // encoding config
                                                               0x00, 0x08, // encoding data length
                                                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mask will be here
                                                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // header 1
                                                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // header 2
                                                               static_cast<char>(0xff), 0x5f, static_cast<char>(0xdf), static_cast<char>(0xd1)});  // magic2


  SECTION("For file that looks wrapped throw if header not valid")
  {
    auto input = createTestWrappedString("AAA").message;
    input[1] = 0xA;
    auto invalidHeader = std::array<char, 48>({0});
    REQUIRE_THROWS_AS(streamingRewrapper.rewrap(input, invalidHeader, 1), std::runtime_error);
  }

  SECTION("For single chunk rYaml files, ensure we don't 'rewrap'")
  {
    const auto data = BytesBuffer{'{'};
    REQUIRE(streamingRewrapper.rewrap(data, header, 1) == data);
  }

  SECTION("For single chunk BMP files, ensure we don't 'rewrap'")
  {
    const auto data = BytesBuffer{'B'};
    REQUIRE(streamingRewrapper.rewrap(data, header, 1) == data);
  }

  SECTION("Rewrap is called with framecount 1, returns the input - including the header")
  {
    auto input = createTestWrappedString("AAA");
    auto output = streamingRewrapper.rewrap(input.message, input.header, 1);
    REQUIRE(output == input.message);
    REQUIRE(output.at(0) == CloakedDagger::cloakedDaggerIdentifierByte);
    REQUIRE(output.size() == CloakedDagger::headerSize() + 3);
  }

  SECTION("If the first frame that rewrapper is given has frameCount != 1, the mask will not be set.")
  {
    auto input = createTestWrappedString("AAA");
    REQUIRE_THROWS_AS(streamingRewrapper.rewrap(input.message, input.header, 2), std::runtime_error);
  }

}
