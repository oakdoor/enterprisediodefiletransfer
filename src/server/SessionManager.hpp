// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include <map>
#include <set>
#include "OrderingStreamWriter.hpp"
#include "StreamInterface.hpp"

class SessionManager
{
public:
  explicit SessionManager(
    std::uint32_t maxBufferSize,
    std::uint32_t maxQueueLength,
    std::function<std::unique_ptr<StreamInterface>(std::uint32_t)> streamCreator,
    std::function<time_t()> getTime,
    std::uint32_t timeoutPeriod,
    DiodeType diodeType);

  void writeToStream(Packet&& packet);

private:
  void closeSession(std::uint32_t sessionId);
  void createNewSession(uint32_t sessionId);

  std::uint32_t maxBufferSize;
  std::uint32_t maxQueueLength;
  std::map<std::uint32_t, OrderingStreamWriter> streams;
  std::function<std::unique_ptr<StreamInterface>(std::uint32_t)> streamCreator;
  std::function<time_t()> getTime;
  std::uint32_t timeoutPeriod;
  DiodeType diodeType;
  void createSessionIfNewId(std::uint32_t sessionId);
  bool isStreamExpired(std::uint32_t sessionId);
  void writeFileAndSaveIfComplete(Packet&& packet);
};

#endif //SESSIONMANAGER_HPP
