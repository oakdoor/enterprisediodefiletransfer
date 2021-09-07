// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "Packet.hpp"
#include "SISLFilename.hpp"
#include <BytesBuffer.hpp>
#include <algorithm>
#include <optional>
#include <queue>
#include <rewrapper/StreamingRewrapper.hpp>
#include <thread>
#include "TestQueue.hpp"

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
    std::uint32_t maxFilenameLength = 65);
  bool write(Packet&& packet, StreamInterface* streamWrapper);

  enum unloadQueueThreadStatus { idle, running, done, interrupted, error, empty };
  unloadQueueThreadStatus unloadQueueThreadState = idle;

private:
  bool checkQueueAndWrite(StreamInterface* streamWrapper);
  void addFrameToQueue(Packet&& packet);
  void writeFrame(StreamInterface *streamWrapper);
  void logOutOfOrderPackets(uint32_t frameCount);
  void unloadQueueThread(StreamInterface* streamWrapper);

  SISLFilename sislFilename;
  bool queueAlreadyExceeded = false;
  std::uint32_t nextFrameCount = 1;
  std::uint32_t lastFrameReceived = 0;
  const std::uint32_t maxBufferSize;
  const std::uint32_t maxQueueLength;
  //std::priority_queue<Packet, std::vector<Packet>, std::greater<>> queue;

  TestQueue queue;

  //TestQueue&& queue = std::move(TestQueue());

  //TestQueue queueInstance;
  //TestQueue&& queue = std::move(queueInstance);
  
  ///std::mutex queueIsBusy;

  const DiodeType diodeType;
  StreamingRewrapper streamingRewrapper;

  std::thread* queueProcessorThread;
  std::uint32_t lastFrameWritten = 0;

  long unsigned int queueSize;
};
