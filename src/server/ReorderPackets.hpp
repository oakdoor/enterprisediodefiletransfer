// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ConcurrentOrderedPacketQueue.hpp"
#include "Packet.hpp"
#include "SISLFilename.hpp"
#include <BytesBuffer.hpp>
#include <algorithm>
#include <future>
#include <optional>
#include <queue>
#include <rewrapper/StreamingRewrapper.hpp>
#include <thread>

class StreamInterface;

enum class DiodeType
{
  import,
  basic
};

class ReorderPackets
{
public:
  explicit ReorderPackets(
    std::uint32_t maxBufferSize,
    std::uint32_t maxQueueLength,
    DiodeType diodeType,
    std::promise<int>&& isStreamClosedPromise,
    std::uint32_t maxFilenameLength = 65);

  void write(Packet&& packet, StreamInterface* streamWrapper);

  enum unloadQueueThreadStatus { error, empty, idle, running, done, interrupted };
  unloadQueueThreadStatus unloadQueueThreadState = unloadQueueThreadStatus::idle;

private:
  void startUnloadQueueThread(StreamInterface* streamWrapper);
  void addFrameToQueue(Packet&& packet);
  void writeFrame(StreamInterface *streamWrapper, Packet&&);
  void logOutOfOrderPackets(uint32_t frameCount);
  void unloadQueueThread(StreamInterface* streamWrapper);

  SISLFilename sislFilename;
  bool queueAlreadyExceeded = false;
  std::uint32_t nextFrameCount = 1;
  std::uint32_t lastFrameReceived = 0;
  const std::uint32_t maxBufferSize;
  const std::uint32_t maxQueueLength;

  ConcurrentOrderedPacketQueue queue;

  const DiodeType diodeType;
  StreamingRewrapper streamingRewrapper;

  std::unique_ptr<std::thread> queueProcessorThread;
  std::uint32_t lastFrameWritten = 0;

  long unsigned int queueSize;

  std::promise<int> streamClosedPromise;
};
