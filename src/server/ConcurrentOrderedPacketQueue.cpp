// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "spdlog/spdlog.h"

ConcurrentOrderedPacketQueue::ConcurrentOrderedPacketQueue()
{
  //std::priority_queue<Packet, std::vector<Packet>, std::greater<>> queue;
  //std::mutex queueIsBusy;
}

ConcurrentOrderedPacketQueue::ConcurrentOrderedPacketQueue(ConcurrentOrderedPacketQueue&& fromQueue)
{
  std::unique_lock<std::mutex> lock_a(fromQueue.queueIsBusy, std::defer_lock);
  std::unique_lock<std::mutex> lock_b(queueIsBusy, std::defer_lock);
  std::lock(lock_a, lock_b);
  std::swap(fromQueue.queue, queue);
}  

void ConcurrentOrderedPacketQueue::emplace(Packet&& packet)
{
  std::unique_lock<std::mutex> lock_b(queueIsBusy);
  queue.emplace(std::move(packet));
}

const Packet& ConcurrentOrderedPacketQueue::top()
{
  std::unique_lock<std::mutex> lock_b(queueIsBusy);
  return queue.top();
} 

void ConcurrentOrderedPacketQueue::pop()
{
  std::unique_lock<std::mutex> lock_b(queueIsBusy);
  queue.pop();
}

size_t ConcurrentOrderedPacketQueue::size()
{
  std::unique_lock<std::mutex> lock_b(queueIsBusy);
  return queue.size();
}

bool ConcurrentOrderedPacketQueue::empty()
{
  std::unique_lock<std::mutex> lock_b(queueIsBusy);
  return queue.empty();
}

std::pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>> ConcurrentOrderedPacketQueue::nextInSequencedPacket(std::uint32_t nextFrameCount, std::uint32_t lastFrameWritten)
{
  std::unique_lock<std::mutex> lock_b(queueIsBusy);
  if (queue.empty()) return std::make_pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>>(sequencedPacketStatus::q_empty, {});
  if (queue.top().headerParams.frameCount == nextFrameCount)
  {
    // Priority queue does not support moving out of top of queue, so we need const cast to remove the const reference and allow move
    Packet topFrame(std::move(const_cast<Packet&>(queue.top())));
    queue.pop();
    return std::make_pair(sequencedPacketStatus::found, std::move(topFrame));
  }
  if (queue.top().headerParams.frameCount <= lastFrameWritten)
  {
    spdlog::info("#discarding frame: " + std::to_string(queue.top().headerParams.frameCount));
    queue.pop();
    spdlog::info("#queue size:" + std::to_string(queue.size()));
    return std::make_pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>>(sequencedPacketStatus::discarded, {});
  }
  return std::make_pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>>(sequencedPacketStatus::waiting, {});
}