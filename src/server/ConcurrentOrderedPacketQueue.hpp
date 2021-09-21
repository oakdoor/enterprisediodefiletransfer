// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#ifndef ENTERPRISEDIODETESTER_CONCURRENTORDEREDPACKETQUEUE_HPP
#define ENTERPRISEDIODETESTER_CONCURRENTORDEREDPACKETQUEUE_HPP

#include "Packet.hpp"
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <condition_variable>

class ConcurrentOrderedPacketQueue
{
public:
  ConcurrentOrderedPacketQueue();
  ConcurrentOrderedPacketQueue(ConcurrentOrderedPacketQueue&& fromQueue) noexcept ;
  ~ConcurrentOrderedPacketQueue() { std::cerr << "ConcurrentQueue was destructed" << queue.size() << std::endl; };

  void emplace(Packet&& packet);
  void pop();
  size_t size();
  bool empty();
  std::optional<Packet> nextInSequencePacket(std::uint32_t nextFrameCount, std::uint32_t lastFrameWritten);

private:
  std::priority_queue<Packet, std::vector<Packet>, std::greater<Packet>> queue;
  std::mutex queueIsBusy;
  std::condition_variable cv;

};

#endif // ENTERPRISEDIODETESTER_CONCURRENTORDEREDPACKETQUEUE_HPP
