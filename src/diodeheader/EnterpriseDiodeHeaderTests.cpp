// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include <array>
#include "test/catch.hpp"
#include "EnterpriseDiodeHeader.hpp"


TEST_CASE("ED Header. Header is created from a input stream. readHeaderParams constructs HeaderParams in order.")
{
  std::array<char, EnterpriseDiode::HeaderSizeInBytes> headerBuffer{'\x03', '\x00', '\x00', '\x00',
                                                                    '\x02', '\x00', '\x00', '\x00',
                                                                    '\x01', '\x00', '\x00', '\x00',
                                                                    '\x00', '\x00', '\x00', '\x00'};
  CloakedDaggerHeader testCloakDaggerHeader{4};
  std::copy(testCloakDaggerHeader.begin(), testCloakDaggerHeader.end(),
            headerBuffer.begin() + EnterpriseDiode::HeaderSizeInBytes - testCloakDaggerHeader.size());
  auto edHeader = EDHeader({headerBuffer.begin(), headerBuffer.end()});

  REQUIRE(edHeader.headerParams.sessionId == 3);
  REQUIRE(edHeader.headerParams.frameCount == 2);
  REQUIRE(edHeader.headerParams.eOFFlag == true);
  REQUIRE(edHeader.headerParams.cloakedDaggerHeader == testCloakDaggerHeader);
}

TEST_CASE("ED Header. Header fields at maximum")
{
  std::array<char, EnterpriseDiode::HeaderSizeInBytes> headerBuffer{'\xFF', '\xFF', '\xFF', '\xFF',
                                                                    '\xFF', '\xFF', '\xFF', '\xFF',
                                                                    '\x01', '\x00', '\x00', '\x00',
                                                                    '\x00', '\x00', '\x00', '\x00'};
  auto edHeader = EDHeader({headerBuffer.begin(), headerBuffer.end()});

  REQUIRE(edHeader.headerParams.sessionId == 0xFFFFFFFF);
  REQUIRE(edHeader.headerParams.frameCount == 0xFFFFFFFF);
  REQUIRE(edHeader.headerParams.eOFFlag == true);
}

TEST_CASE("ED Header. Header fields near maximum")
{
  std::array<char, EnterpriseDiode::HeaderSizeInBytes> headerBuffer{'\x00', '\xFF', '\xFF', '\xFF',
                                                                    '\x00', '\xFF', '\xFF', '\xFF',
                                                                    '\x01', '\x00', '\x00', '\x00',
                                                                    '\x00', '\x00', '\x00', '\x00'};
  auto edHeader = EDHeader({headerBuffer.begin(), headerBuffer.end()});

  REQUIRE(edHeader.headerParams.sessionId == 0xFFFFFF00);
  REQUIRE(edHeader.headerParams.frameCount == 0xFFFFFF00);
  REQUIRE(edHeader.headerParams.eOFFlag == true);
}

TEST_CASE("ED Header. calculateMaxBufferSize returns max buffer size given specific MTU values.")
{
  REQUIRE(EnterpriseDiode::calculateMaxBufferSize(1500) == 1472);
  REQUIRE(EnterpriseDiode::calculateMaxBufferSize(9000) == 8972);
}

TEST_CASE("ED Header. calculateMaxBufferSize throws error if given MTU size is less than 222.")
{
  REQUIRE_THROWS_AS(EnterpriseDiode::calculateMaxBufferSize(0), std::runtime_error);
  REQUIRE_THROWS_AS(EnterpriseDiode::calculateMaxBufferSize(221), std::runtime_error);
}

TEST_CASE("ED Header. Constructing EDHeader from bytesbuffer with size less than EDHeaderSize throws.")
{
  std::array<char, EnterpriseDiode::HeaderSizeInBytes - 1> headerBuffer{'\x00', '\xFF', '\xFF', '\xFF',
                                                                        '\x00', '\xFF', '\xFF', '\xFF',
                                                                        '\x01', '\x00', '\x00', '\x00',
                                                                        '\x00', '\x00', '\x00', '\x00'};
  REQUIRE_THROWS_AS(EDHeader({headerBuffer.begin(), headerBuffer.end()}), std::runtime_error);
}
