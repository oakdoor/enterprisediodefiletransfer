// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "OrderingStreamWriter.hpp"
#include <future>
#include <iostream>

OrderingStreamWriter::OrderingStreamWriter(
  std::uint32_t maxBufferSize,
  std::uint32_t maxQueueLength,
  std::unique_ptr<StreamInterface> streamWrapper,
  std::function<std::time_t()> getTime,
  DiodeType diodeType):
    packetQueue(maxBufferSize, maxQueueLength, diodeType),
    streamWrapper(std::move(streamWrapper)),
    getTime(std::move(getTime)),
    timeLastUpdated(this->getTime())
{
}

void OrderingStreamWriter::write(Packet&& data)
{
  timeLastUpdated = getTime();
  packetQueue.write(std::move(data), streamWrapper.get());
}

void OrderingStreamWriter::deleteFile()
{
  streamWrapper->deleteFile();
}

void OrderingStreamWriter::renameFile()
{
  std::cerr << "calling rename from inside OrderingStreamWriter::renameFile()" << std::endl;
  streamWrapper->renameFile();
}
