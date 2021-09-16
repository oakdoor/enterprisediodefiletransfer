// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include "ClientWrapper.hpp"
#include "FreeRunningTimer.hpp"
#include "Timer.hpp"
#include "spdlog/spdlog.h"
#include <utility>

ClientWrapper::ClientWrapper(
  const std::string& targetAddress,
  std::uint16_t targetPort,
  std::uint16_t mtuSize,
  double dataRateMbps,
  std::string filename,
  const std::string& logLevel,
  std::uint16_t sendPeriod) :
    edClient(
      std::make_shared<UdpClient>(targetAddress, targetPort),
      selectTimer(mtuSize, dataRateMbps, sendPeriod),
      calculatePayloadSize(mtuSize),
      1,
      std::move(filename))
{
  spdlog::set_level(spdlog::level::from_str(logLevel));
}

ClientWrapper::ClientWrapper(
  const std::string& targetAddress,
  std::uint16_t targetPort,
  std::uint16_t mtuSize,
  double dataRateMbps,
  std::string filename,
  const std::string& logLevel,
  std::uint16_t sendPeriod,
  std::int32_t numberOfPackets) :
    edClient(
      std::make_shared<UdpClient>(targetAddress, targetPort),
      selectTimer(mtuSize, dataRateMbps, sendPeriod),
      calculatePayloadSize(mtuSize),
      numberOfPackets,
      std::move(filename))
{
  spdlog::set_level(spdlog::level::from_str(logLevel));
}

std::shared_ptr<TimerInterface> ClientWrapper::selectTimer(uint16_t mtuSize, double dataRateMbps, uint16_t sendPeriod)
{
  if (sendPeriod > 0)
  {
    return std::make_shared<Timer>(sendPeriod);
  }
  else if (isZero(dataRateMbps))
  {
    spdlog::debug("Selecting free running timer");
    return std::make_shared<FreeRunningTimer>();
  }
  else
  {
    return std::make_shared<Timer>(calculateTimerPeriod(dataRateMbps, mtuSize));
  }
}

bool ClientWrapper::isZero(double dataRateMbps)
{
  return std::abs(dataRateMbps) < 0.001;
}

void ClientWrapper::sendData(const std::string& filename)
{
  std::ifstream inputStream(filename, std::ios::binary);
  try
  {
    edClient.send(inputStream);
  }
  catch(const std::exception& exc)
  {
    spdlog::error(std::string("Exception Sending Data: ") + exc.what());
    throw std::runtime_error("sendData failed");
  }
  spdlog::info("Send complete");
}

std::uint16_t calculatePayloadSize(std::uint16_t mtuSize)
{
  return std::uint16_t(
    (short) EnterpriseDiode::calculateMaxBufferSize(mtuSize) - (short) EnterpriseDiode::HeaderSizeInBytes);
}
