// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "spdlog/spdlog.h"
#include <iostream>

ConcurrentOrderedPacketQueue::ConcurrentOrderedPacketQueue()
{
}

ConcurrentOrderedPacketQueue::ConcurrentOrderedPacketQueue(ConcurrentOrderedPacketQueue&& fromQueue) noexcept
{
  std::unique_lock<std::mutex> lock_a(fromQueue.queueIsBusy, std::defer_lock);
  std::unique_lock<std::mutex> lock_b(queueIsBusy, std::defer_lock);
  std::lock(lock_a, lock_b);
  std::swap(fromQueue.queue, queue);
}

void ConcurrentOrderedPacketQueue::emplace(Packet&& packet)
{
  {
    std::unique_lock<std::mutex> lock_b(queueIsBusy);
    queue.emplace(std::move(packet));
  }
  cv.notify_all();
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

std::optional<Packet> ConcurrentOrderedPacketQueue::nextInSequencePacket(std::uint32_t nextFrameCount, std::uint32_t lastFrameWritten)
{
  std::unique_lock<std::mutex> lock_b(queueIsBusy);

  cv.wait_for(lock_b, std::chrono::microseconds(100));

  if (queue.empty())
  {
    return {};
  }

  if (queue.top().headerParams.frameCount == nextFrameCount)
  {
    // Priority queue does not support moving out of top of queue, so we need const cast to remove the const reference and allow move
    Packet topFrame(std::move(const_cast<Packet&>(queue.top())));
    queue.pop();
    return std::move(topFrame);
  }

  if (queue.top().headerParams.frameCount <= lastFrameWritten)
  {
    spdlog::info("#discarding frame: " + std::to_string(queue.top().headerParams.frameCount));
    queue.pop();
    spdlog::info("#queue size:" + std::to_string(queue.size()));
    return {};
  }

  return {};
}
