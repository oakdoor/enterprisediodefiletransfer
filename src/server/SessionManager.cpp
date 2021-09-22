// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "SessionManager.hpp"
#include "FileStream.hpp"
#include "diodeheader/EnterpriseDiodeHeader.hpp"
#include <filesystem>
#include <future>
#include <iostream>

SessionManager::SessionManager(
  std::uint32_t maxBufferSize,
  std::uint32_t maxQueueLength,
  std::function<std::unique_ptr<StreamInterface>(std::uint32_t)> streamCreator,
  std::function<time_t()> getTime,
  std::uint32_t timeoutPeriod,
  DiodeType diodeType) :
    maxBufferSize(maxBufferSize),
    maxQueueLength(maxQueueLength),
    streamCreator(std::move(streamCreator)),
    getTime(std::move(getTime)),
    timeoutPeriod(timeoutPeriod),
    diodeType(diodeType)
{
}

void SessionManager::writeToStream(Packet&& packet)
{
  createSessionIfNewId(packet.headerParams.sessionId);

  if (isStreamExpired(packet.headerParams.sessionId))
  {
    std::cerr << "Stream has timed-out. Closing stream" << "\n";
    streams.at(packet.headerParams.sessionId).deleteFile();
    closeSession(packet.headerParams.sessionId);
    return;
  }

  writeFileAndSaveIfComplete(std::move(packet));
}

void SessionManager::createSessionIfNewId(const std::uint32_t sessionId)
{
  if (streams.find(sessionId) == streams.end())
  {
    createNewSession(sessionId);
  }
}

void SessionManager::createNewSession(std::uint32_t sessionId)
{
  streams.emplace(std::make_pair(
    sessionId, OrderingStreamWriter(maxBufferSize, maxQueueLength, streamCreator(sessionId), getTime, diodeType)));
}

bool SessionManager::isStreamExpired(std::uint32_t sessionId)
{
  return streams.at(sessionId).timeLastUpdated + timeoutPeriod < getTime();
}

void SessionManager::writeFileAndSaveIfComplete(Packet&& packet)
{
  const auto sessionId = packet.headerParams.sessionId;
  streams.at(sessionId).write(std::move(packet));
  // const bool fileComplete = streams.at(sessionId).write(std::move(packet));
  // if (fileComplete)
  // {
  //   streams.at(sessionId).renameFile();
  //   closeSession(sessionId);
  // }
}

void SessionManager::closeSession(std::uint32_t sessionId)
{
  streams.erase(sessionId);
}