// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#ifndef ENTERPRISEDIODETESTER_CONCURRENTORDEREDPACKETQUEUE_HPP
#define ENTERPRISEDIODETESTER_CONCURRENTORDEREDPACKETQUEUE_HPP

#include "Packet.hpp"
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>

class ConcurrentOrderedPacketQueue
{
public:
  ConcurrentOrderedPacketQueue();
  ConcurrentOrderedPacketQueue(ConcurrentOrderedPacketQueue&& fromQueue);
  ~ConcurrentOrderedPacketQueue() { std::cerr << "ConcurrentQueue was destructed" << queue.size() << std::endl; };

  enum sequencedPacketStatus { error, q_empty, found, waiting, discarded };

  void emplace(Packet&& packet);
  const Packet& top();
  void pop();
  size_t size();
  bool empty();
  std::pair<ConcurrentOrderedPacketQueue::sequencedPacketStatus, std::optional<Packet>> nextInSequencePacket(std::uint32_t nextFrameCount, std::uint32_t lastFrameWritten);

private:
  std::priority_queue<Packet, std::vector<Packet>, std::greater<Packet>> queue;
  std::mutex queueIsBusy;

};

#endif // ENTERPRISEDIODETESTER_CONCURRENTORDEREDPACKETQUEUE_HPP
