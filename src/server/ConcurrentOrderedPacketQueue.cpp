// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "spdlog/spdlog.h"
#include <iostream>

ConcurrentOrderedPacketQueue::ConcurrentOrderedPacketQueue()
{
  std::cerr << "This is a primary constructor" << std::endl;
}

ConcurrentOrderedPacketQueue::ConcurrentOrderedPacketQueue(ConcurrentOrderedPacketQueue&& fromQueue)
{
  std::unique_lock<std::mutex> lock_a(fromQueue.queueIsBusy, std::defer_lock);
  std::unique_lock<std::mutex> lock_b(queueIsBusy, std::defer_lock);
  std::lock(lock_a, lock_b);
  std::swap(fromQueue.queue, queue);
  std::cerr << "This is a move constructor " << queue.size() << std::endl;
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

std::pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>> ConcurrentOrderedPacketQueue::nextInSequencePacket(std::uint32_t nextFrameCount, std::uint32_t lastFrameWritten)
{
  try
  {
    std::unique_lock<std::mutex> lock_b(queueIsBusy, std::defer_lock);
    if (!lock_b.try_lock())
    {
      return std::make_pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>>(
        sequencedPacketStatus::waiting, {});
    }
//    std::cerr << "#nxtfrm, lstfrm, qaddr, paddr, frmcnt, qsize"
//              << "\n";
//    std::cerr << nextFrameCount << " " << lastFrameWritten << " " << &queue << " " << &queue.top() << " "
//              << queue.top().headerParams.frameCount << " " << queue.size() << "\n";
    if (queue.empty())
      return std::make_pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>>(
        sequencedPacketStatus::q_empty, {});
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
      return std::make_pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>>(
        sequencedPacketStatus::discarded, {});
    }
    return std::make_pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>>(
      sequencedPacketStatus::waiting, {});
  }
  catch (const std::system_error& exception)
  {
    spdlog::error(std::string("#Caught exception in next in sequence func: ") + exception.what());
    std::cerr << exception.code() << "\n";
    exit(1);
  }
}
