// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ReorderPackets.hpp"
#include "Packet.hpp"
#include "StreamInterface.hpp"
#include "TotalFrames.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include "spdlog/spdlog.h"

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

bool ReorderPackets::write(Packet&& packet, StreamInterface* streamWrapper)
{
  logOutOfOrderPackets(packet.headerParams.frameCount);
  addFrameToQueue(std::move(packet));
  return checkQueueAndWrite(streamWrapper);
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
  queueSize = queue.size();
  if (queueSize >= maxQueueLength)
  {
    if (!queueAlreadyExceeded)
    {
      spdlog::error("ReorderPackets: maxQueueLength exceeded." + std::to_string(queueSize));
      spdlog::error(std::string("Last frame: ") + std::to_string(lastFrameReceived) +
           std::string(" This frame: ") + std::to_string(packet.headerParams.frameCount));
      queueAlreadyExceeded = true;
    }
    return;
  }
  queue.emplace(std::move(packet));
  queueUsagePeak = queueSize > queueUsagePeak ? queueSize : queueUsagePeak;
  spdlog::info("size=" + std::to_string(queue.size()));
}

bool ReorderPackets::checkQueueAndWrite(StreamInterface* streamWrapper)
{
  if (unloadQueueThreadState == idle)
  {
    unloadQueueThreadState = running;
    queueProcessorThread = new std::thread(&ReorderPackets::unloadQueueThread, this, streamWrapper);
    //queueProcessorThread.detach();
    spdlog::info("started thread");
    return false;
  }
  else if (unloadQueueThreadState == done)
  {
    spdlog::info("exiting thread");
    queueProcessorThread->join();
    spdlog::info("exited thread");
    return true;
  }
  else
  {
    return false;
  }
}

void ReorderPackets::unloadQueueThread(StreamInterface* streamWrapper)
{
  std::this_thread::sleep_for(std::chrono::microseconds(30));
  try
  {
    while (unloadQueueThreadState == running)
    {
      queue.processQueue();
    }
    unloadQueueThreadState = empty;
    spdlog::info("#exiting thread while expecting frame: " + std::to_string(nextFrameCount));
    spdlog::info("#queue size:" + std::to_string(queue.size()));
    spdlog::info("#frame on top of queue: " + std::to_string(queue.top().headerParams.frameCount));
    throw std::string("#unloadThreadQueue:") + std::to_string(unloadQueueThreadState);
  }
  catch (std::string ex)
  {
    spdlog::info(ex);
    if (unloadQueueThreadState == done)
    {
      streamWrapper->renameFile();
    }
  }
}

void ReorderPackets::writeFrame(StreamInterface* streamWrapper)
{
  if (diodeType == DiodeType::import)
  {
    streamWrapper->write(
      streamingRewrapper.rewrap(
        queue.top().getFrame(), queue.top().headerParams.cloakedDaggerHeader,
        nextFrameCount));
  }
  else
  {
    lastFrameWritten = queue.top().headerParams.frameCount;
    //spdlog::info("write: " + std::to_string(queue.top().getFrame()[0]));
    streamWrapper->write(queue.top().getFrame());
    spdlog::info("#frame: " + std::to_string(lastFrameWritten));
  }
}
