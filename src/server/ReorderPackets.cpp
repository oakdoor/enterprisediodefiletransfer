// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ReorderPackets.hpp"
#include "TestQueue.hpp"
#include "Packet.hpp"
#include "StreamInterface.hpp"
#include "TotalFrames.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <future>
#include <optional>
#include "spdlog/spdlog.h"

ReorderPackets::ReorderPackets(
  std::uint32_t maxBufferSize,
  std::uint32_t maxQueueLength,
  DiodeType diodeType,
  std::promise<int>&& isStreamClosedPromise,
  std::uint32_t maxFilenameLength):
    sislFilename(maxFilenameLength),
    maxBufferSize(maxBufferSize),
    maxQueueLength(maxQueueLength),
    diodeType(diodeType)
{
  streamClosedPromise = std::move(isStreamClosedPromise);
}

void ReorderPackets::write(Packet&& packet, StreamInterface* streamWrapper)
{
  logOutOfOrderPackets(packet.headerParams.frameCount);
  addFrameToQueue(std::move(packet));
  if (unloadQueueThreadState == unloadQueueThreadStatus::idle)
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
  //spdlog::info("size=" + std::to_string(queueSize));
}

void ReorderPackets::startUnloadQueueThread(StreamInterface* streamWrapper)
{
  unloadQueueThreadState = unloadQueueThreadStatus::running;
  queueProcessorThread = new std::thread(&ReorderPackets::unloadQueueThread, this, streamWrapper);
  queueProcessorThread->detach();
  spdlog::info("started thread");
}

void ReorderPackets::unloadQueueThread(StreamInterface* streamWrapper)
{
  std::this_thread::sleep_for(std::chrono::microseconds(30));
  while (unloadQueueThreadState == unloadQueueThreadStatus::running)
  {
    try
    {
      auto queueResponse = queue.nextInSequencedPacket(nextFrameCount, lastFrameWritten);
      TestQueue::sequencedPacketStatus packetStatus = queueResponse.first;
      if (packetStatus == TestQueue::sequencedPacketStatus::found)
      {
        Packet packet(std::move(queueResponse.second.value()));
        if (packet.headerParams.eOFFlag)
        {
          streamWrapper->setStoredFilename(
            sislFilename.extractFilename(packet.getFrame()).value_or("rejected."));
          unloadQueueThreadState = unloadQueueThreadStatus::done;
          streamWrapper->renameFile();
          streamClosedPromise.set_value(1);
          spdlog::info("#File completed.");
        } 
        else 
        {
          writeFrame(streamWrapper, std::move(packet));
          ++nextFrameCount;
        }
      }
      else if (packetStatus == TestQueue::sequencedPacketStatus::discarded)
      {
        continue;
      }
      else if (packetStatus == TestQueue::sequencedPacketStatus::waiting)
      {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
      }
      else if (packetStatus == TestQueue::sequencedPacketStatus::q_empty)
      {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
      }
      else if (packetStatus == TestQueue::sequencedPacketStatus::error)
      {
        unloadQueueThreadState = ReorderPackets::unloadQueueThreadStatus::error;
        throw std::string("Queue Error");
      }
    }
    catch (std::string ex)
    {
      spdlog::info(ex);
      unloadQueueThreadState = unloadQueueThreadStatus::interrupted;
      spdlog::info("#exiting thread while expecting frame: " + std::to_string(nextFrameCount));
      spdlog::info("#queue size:" + std::to_string(queue.size()));
      spdlog::info("#frame on top of queue: " + std::to_string(queue.top().headerParams.frameCount));
      throw std::string("#unloadThreadQueue:") + std::to_string(unloadQueueThreadState);
    }
    catch (const std::exception& ex)
    {
      spdlog::info("Caught exception: " + std::string(ex.what()));
    }
  }
  spdlog::info("#Exiting thread. TODO handle thread cleanup.");
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
