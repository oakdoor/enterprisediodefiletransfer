// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#ifndef CLIENTWRAPPER_HPP
#define CLIENTWRAPPER_HPP

#include <iostream>
#include <fstream>
#include "Client.hpp"
#include "UdpClient.hpp"
#include "diodeheader/EnterpriseDiodeHeader.hpp"

class TimerInterface;

class ClientWrapper
{
public:
  ClientWrapper(
    const std::string& targetAddress,
    std::uint16_t targetPort,
    std::uint16_t mtuSize,
    double dataRateMbps,
    std::string filename,
    const std::string& logLevel,
    std::uint16_t sendperiod);

  ClientWrapper(
    const std::string& targetAddress,
    std::uint16_t targetPort,
    std::uint16_t mtuSize,
    double dataRateMbps,
    std::string filename,
    const std::string& logLevel,
    std::uint16_t sendperiod,
    std::int32_t numberOfPackets);

  void sendData(const std::string& filename);

private:
  Client edClient;

  static std::shared_ptr<TimerInterface> selectTimer(uint16_t mtuSize, double dataRateMbps, uint16_t sendPeriod);
  static bool isZero(double dataRateMbps);
};

std::uint16_t calculatePayloadSize(std::uint16_t mtuSize);

#endif //CLIENTWRAPPER_HPP
