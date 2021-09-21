// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "test/catch.hpp"

TEST_CASE("ConcurrentOrderedPacketQueue.")
{
  ConcurrentOrderedPacketQueue queue;

  SECTION("ConcurrentOrderedPacketQueue returns the 'q_empty' status and no packet if the queue is empty")
  {
    auto queueResponse = queue.nextInSequencePacket(1, 0);
//    REQUIRE(queueResponse.first == ConcurrentOrderedPacketQueue::sequencedPacketStatus::q_empty);
//    REQUIRE_FALSE(queueResponse.second.has_value());
  }

  SECTION("ConcurrentOrderedPacketQueue returns the 'found' status "
          "and the packet at the top of the queue if it's the next in the sequence")
  {
    queue.emplace({HeaderParams{0, 1, false, {}}, {'A', 'B'}});
    auto queueResponse = queue.nextInSequencePacket(1, 0);

    REQUIRE(queueResponse.first == ConcurrentOrderedPacketQueue::sequencedPacketStatus::found);
    Packet packet(std::move(queueResponse.second.value()));
    REQUIRE(packet.headerParams.frameCount == 1);
    REQUIRE(packet.headerParams.eOFFlag == false);
    REQUIRE(std::string(packet.payload.begin(), packet.payload.end()) == "AB");
  }

  SECTION("ConcurrentOrderedPacketQueue returns the 'waiting' status "
          "and no packet if the packet at the top of the queue is not the next in the sequence")
  {
    queue.emplace({HeaderParams{0, 2, false, {}}, {'A', 'B'}});
    queue.emplace({HeaderParams{0, 3, false, {}}, {'C', 'D'}});
    queue.emplace({HeaderParams{0, 4, false, {}}, {'E', 'F'}});
    auto queueResponse = queue.nextInSequencePacket(1, 0);

    REQUIRE(queueResponse.first == ConcurrentOrderedPacketQueue::sequencedPacketStatus::waiting);
    REQUIRE_FALSE(queueResponse.second.has_value());
  }

  SECTION("ConcurrentOrderedPacketQueue discards duplicated packets and returns the 'discared' status")
  {
    queue.emplace({HeaderParams{0, 5, false, {}}, {'A', 'B'}});
    auto queueResponse = queue.nextInSequencePacket(9, 8);

    REQUIRE(queueResponse.first == ConcurrentOrderedPacketQueue::sequencedPacketStatus::discarded);
    REQUIRE_FALSE(queueResponse.second.has_value());
    REQUIRE(queue.empty());
  }
}