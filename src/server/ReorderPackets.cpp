// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ReorderPackets.hpp"
#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "StreamInterface.hpp"
#include "TotalFrames.hpp"
#include "spdlog/spdlog.h"
#include <chrono>
#include <future>
#include <iostream>
#include <optional>

ReorderPackets::ReorderPackets(
  std::uint32_t maxBufferSize,
  std::uint32_t maxQueueLength,
  DiodeType diodeType,
  std::uint32_t maxFilenameLength):
    sislFilename(maxFilenameLength),
    maxBufferSize(maxBufferSize),
    maxQueueLength(maxQueueLength),
    diodeType(diodeType)
{
}

ReorderPackets::~ReorderPackets()
{
  exitSignal.set_value();
  while (!isDone()) {usleep(100); }
}

void ReorderPackets::write(Packet&& packet, StreamInterface* streamWrapper)
{
  logOutOfOrderPackets(packet.headerParams.frameCount);
  addFrameToQueue(std::move(packet));
  if (!isQueueBeingUnloaded)
  {
    startUnloadQueueThread(streamWrapper);
  }
}

void ReorderPackets::logOutOfOrderPackets(uint32_t frameCount)
{
  if (frameCount != lastFrameReceived + 1)
  {
    spdlog::info(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + std::string(" Out of order frame: ") + std::to_string(frameCount));
    //spdlog::info(std::string("Last frame received was: ") + std::to_string(lastFrameReceived));
    outOfOrderFrames++;
  }
  lastFrameReceived = frameCount;
}

void ReorderPackets::addFrameToQueue(Packet&& packet)
{
  std::cerr << "Queue address (size): " << &queue << "\n";
  queueSize = queue.size();
  if (queueSize >= maxQueueLength)
  {
    if (!queueAlreadyExceeded)
    {
      spdlog::error("#ReorderPackets: maxQueueLength exceeded." + std::to_string(queueSize));
      spdlog::error(std::string("Last frame: ") + std::to_string(lastFrameReceived) +
           std::string(" This frame: ") + std::to_string(packet.headerParams.frameCount));
      queueAlreadyExceeded = true;
    }
    return;
  }
  std::cerr << "Queue address (emplace): " << &queue << "\n";
  std::cerr << "Packet address: " << &packet << "\n";
  queue.emplace(std::move(packet));
  std::cerr << "Queue address (after emplace): " << &queue << "\n";
  std::cerr << "Queue size: " << queue.size() << "\n";
  queueUsagePeak = queueSize > queueUsagePeak ? queueSize : queueUsagePeak;
  //spdlog::info("size=" + std::to_string(queueSize));
}

void ReorderPackets::startUnloadQueueThread(StreamInterface* streamWrapper)
{
  try
  {
    queueProcessorThread = std::async(std::launch::async, [&, streamWrapper]() {ReorderPackets::unloadQueueThread(streamWrapper); return true; });
  }
  catch (const std::system_error& exception)
  {
    spdlog::error(std::string("#Caught exception when starting thread: ") + exception.what());
    exit(1);
  }
  isQueueBeingUnloaded = true;
  spdlog::info("#started thread");
}

void ReorderPackets::unloadQueueThread(StreamInterface* streamWrapper)
{
  while (exitSignal.get_future().wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
  {
    try
    {
      auto queueResponse = queue.nextInSequencePacket(nextFrameCount, lastFrameWritten);
      if (queueResponse)
      {
        if (queueResponse->headerParams.eOFFlag)
        {
          streamWrapper->setStoredFilename(
            sislFilename.extractFilename(queueResponse->getFrame()).value_or("rejected."));
          streamWrapper->renameFile();
          spdlog::info("#File completed.");
          return;
        }
        else
        {
          writeFrame(streamWrapper, std::move(*queueResponse));
          ++nextFrameCount;
        }
      }
    }
    catch (const std::exception& ex)
    {
      spdlog::info("#Caught exception: " + std::string(ex.what()));
    }
  }
}

void ReorderPackets::writeFrame(StreamInterface* streamWrapper, Packet&& packet)
{
  if (diodeType == DiodeType::import)
  {
    streamWrapper->write(
      streamingRewrapper.rewrap(
        packet.getFrame(), packet.headerParams.cloakedDaggerHeader,
        nextFrameCount));
    lastFrameWritten = packet.headerParams.frameCount;
  }
  else
  {
    lastFrameWritten = packet.headerParams.frameCount;
    streamWrapper->write(packet.getFrame());
  }
}
bool ReorderPackets::isDone() const
{
  return queueProcessorThread.wait_for(std::chrono::microseconds(0)) == std::future_status::ready;
}
