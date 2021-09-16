// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <istream>
#include <boost/asio/time_traits.hpp>
#include <boost/asio/buffer.hpp>
#include "TimerInterface.hpp"
#include "UdpClientInterface.hpp"
#include "diodeheader/EnterpriseDiodeHeader.hpp"

class Client
{
public:
  Client(std::shared_ptr<UdpClientInterface> udpClient,
    std::shared_ptr<TimerInterface> timer,
    std::uint16_t maxPayloadSize,
    std::string filename="received");

  Client(std::shared_ptr<UdpClientInterface> udpClient,
    std::shared_ptr<TimerInterface> timer,
    std::uint16_t maxPayloadSize,
    std::int32_t numberOfPackets,
    std::string filename="received");

  void send(std::istream& inputStream);

private:
  bool sendFrame(std::istream& inputStream);
  ConstSocketBuffers generateEDPacket(std::istream& inputStream, std::uint32_t payloadSize);
  void incrementFrameCount();
  void setEOF();
  void setSessionID();
  ConstSocketBuffers addEOFframe();
  void parseFilename();
  std::string getFilenameFromPath() const;

  std::shared_ptr<UdpClientInterface> udpClient;
  std::shared_ptr<TimerInterface> edTimer;
  std::uint32_t maxPayloadSize;
  std::array<char, EnterpriseDiode::HeaderSizeInBytes> headerBuffer;
  std::vector<char> payloadBuffer;
  std::int32_t numberOfPackets = -1;
  const std::string filename;
  std::string filenameAsSisl;
};

boost::posix_time::microseconds calculateTimerPeriod(double dataRateMbps, std::uint32_t packetSizeBytes);

#endif //CLIENT_HPP
