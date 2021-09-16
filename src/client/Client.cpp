// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file

#include "Client.hpp"
#include <chrono>
#include <istream>
#include <random>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <regex>
#include "spdlog/spdlog.h"

Client::Client(
  std::shared_ptr<UdpClientInterface> udpClient,
  std::shared_ptr<TimerInterface> timer,
  std::uint16_t maxPayloadSize,
  std::string filename):
    udpClient(udpClient),
    edTimer(timer),
    maxPayloadSize(maxPayloadSize),
    headerBuffer({}),
    payloadBuffer(maxPayloadSize),
    numberOfPackets(1),
    filename(std::move(filename))
{
}

Client::Client(
  std::shared_ptr<UdpClientInterface> udpClient,
  std::shared_ptr<TimerInterface> timer,
  std::uint16_t maxPayloadSize,
  std::int32_t numberOfPackets,
  std::string filename):
    udpClient(udpClient),
    edTimer(timer),
    maxPayloadSize(maxPayloadSize),
    headerBuffer({}),
    payloadBuffer(maxPayloadSize),
    numberOfPackets(numberOfPackets),
    filename(std::move(filename))
{
}

void Client::send(std::istream& inputStream)
{
  inputStream.exceptions(std::istream::badbit);
  if (!inputStream)
  {
    throw std::runtime_error("file stream not found");
  }
  parseFilename();
  setSessionID();
  edTimer->runTimer([&]() {
    try
    {
      return sendFrame(inputStream);
    }
    catch (const std::exception& exception)
    {
      spdlog::error(std::string("exception in send frame ") + exception.what());
      return false;
    }
  });
}

void Client::parseFilename()
{
    const auto filenameFromPath = getFilenameFromPath();
    std::regex filter("[a-zA-Z0-9\\.\\-_]+");
    if (!std::regex_match(filenameFromPath, filter))
    {
      throw std::runtime_error("Invalid filename provided. Please rename. The filename can only contain alphanumeric characters, dashes(-) and dots(.)");
    }
    if (filenameFromPath.length() > 65)
    {
      throw std::runtime_error("Invalid filename provided. The maximum length of the filename is 65 characters.\n" + filenameFromPath + " is " + std::to_string(filenameFromPath.length()) + " characters.");
    }
}
std::string Client::getFilenameFromPath() const
{
  return std::filesystem::path(filename).filename();
}

bool Client::sendFrame(std::istream& inputStream)
{
  udpClient->send(generateEDPacket(inputStream, maxPayloadSize));
  return headerBuffer.at(8) != 1;
}

ConstSocketBuffers Client::generateEDPacket(std::istream& inputStream, std::uint32_t payloadSize)
{
//  spdlog::info("generateEDPacket called");
  static bool sislFileRead = false;
  static int32_t counter;
  static std::streamsize length = 0;
  incrementFrameCount();
  const auto payloadLength = inputStream.read((char*)&*(payloadBuffer.begin()), payloadSize).gcount();
  if (numberOfPackets == -1)
  {
    if (!sislFileRead)
    {
      counter = 0;
      spdlog::info("reading stream");
      length = inputStream.read((char*)&*(payloadBuffer.begin()), payloadSize).gcount();
      sislFileRead = true;
    }
    const auto _payloadLength = length;

    if (counter < numberOfPackets)
    {
      ++counter;
      return {
        boost::asio::buffer(headerBuffer, EnterpriseDiode::HeaderSizeInBytes),
        boost::asio::buffer(payloadBuffer, (size_t)_payloadLength)};
    }
    else
    {
      return addEOFframe();
    }
  }
  else if (payloadLength > 0)
  {
    return {
      boost::asio::buffer(headerBuffer, EnterpriseDiode::HeaderSizeInBytes),
      boost::asio::buffer(payloadBuffer, (size_t)payloadLength)};
  }
  else
  {
    return addEOFframe();
  }
}

void Client::incrementFrameCount()
{
  ++(*reinterpret_cast<std::uint32_t*>(&headerBuffer.at(4)));
}

void Client::setEOF()
{
  headerBuffer.at(8) = 1;
}

ConstSocketBuffers Client::addEOFframe()
{
  setEOF();
  filenameAsSisl = "{name: !str \"" + getFilenameFromPath() + "\"}";
  return {
    boost::asio::buffer(headerBuffer, EnterpriseDiode::HeaderSizeInBytes),
    boost::asio::buffer(filenameAsSisl, filenameAsSisl.length())};
}

void Client::setSessionID()
{
  const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
  *reinterpret_cast<std::uint32_t*>(&headerBuffer.at(0)) = (std::uint32_t)std::default_random_engine(seed)();
}

boost::posix_time::microseconds calculateTimerPeriod(double dataRateMbps, std::uint32_t mtuSize)
{
  const auto period = std::round((static_cast<double>((mtuSize * 8)) * 1000000) / (dataRateMbps * 1024 * 1024));
  return boost::posix_time::microseconds {static_cast<std::int64_t>(period)};
}
