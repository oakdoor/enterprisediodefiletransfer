// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "test/catch.hpp"
#include <unistd.h>

inline std::optional<Packet> waitForPacket(ConcurrentOrderedPacketQueue& queue, std::uint32_t frame)
{
  std::int32_t timeout = 10000;
  while (timeout-- > 0)
  {
    if (auto packet = queue.nextInSequencePacket(frame, 0); packet)
    {
      return packet;
    }
    usleep(10);
  }

  return {};
}

TEST_CASE("ConcurrentOrderedPacketQueue.")
{
  ConcurrentOrderedPacketQueue queue;

  SECTION("ConcurrentOrderedPacketQueue returns the 'q_empty' status and no packet if the queue is empty")
  {
    REQUIRE_FALSE(queue.nextInSequencePacket(1, 0).has_value());
  }

  SECTION("maintains ordering of packets")
  {
    queue.emplace({HeaderParams{0, 1, false, {}}, {'A', 'B'}});
    queue.emplace({HeaderParams{0, 2, false, {}}, {'C', 'D'}});

    REQUIRE(waitForPacket(queue, 1)->headerParams.frameCount == 1);
    REQUIRE(waitForPacket(queue, 2)->headerParams.frameCount == 2);
  }

  SECTION("re-orders packets")
  {
    queue.emplace({HeaderParams{0, 2, false, {}}, {'A', 'B'}});
    queue.emplace({HeaderParams{0, 1, false, {}}, {'C', 'D'}});

    REQUIRE(waitForPacket(queue, 1)->headerParams.frameCount == 1);
    REQUIRE(waitForPacket(queue, 2)->headerParams.frameCount == 2);
  }


  SECTION("ConcurrentOrderedPacketQueue returns the 'found' status "
          "and the packet at the top of the queue if it's the next in the sequence")
  {
    queue.emplace({HeaderParams{0, 1, false, {}}, {'A', 'B'}});
    auto queueResponse = queue.nextInSequencePacket(1, 0);

    Packet packet(std::move(queueResponse.value()));
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

    REQUIRE_FALSE(queueResponse.has_value());
  }

  SECTION("ConcurrentOrderedPacketQueue discards duplicated packets and returns the 'discared' status")
  {
    queue.emplace({HeaderParams{0, 5, false, {}}, {'A', 'B'}});
    auto queueResponse = queue.nextInSequencePacket(9, 8);

    REQUIRE_FALSE(queueResponse.has_value());
    REQUIRE(queue.empty());
  }
}