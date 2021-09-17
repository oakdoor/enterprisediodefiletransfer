// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "test/catch.hpp"

TEST_CASE("ConcurrentOrderedPacketQueue.")
{
  ConcurrentOrderedPacketQueue queue;

  SECTION("ConcurrentOrderedPacketQueue returns the q_empty status and no packet if the queue is empty")
  {
    auto queueResponse = queue.nextInSequencePacket(1, 0);
    REQUIRE(queueResponse.first == ConcurrentOrderedPacketQueue::sequencedPacketStatus::q_empty);
    REQUIRE_FALSE(queueResponse.second.has_value());
  }
}