// Copyright PA Knowledge Ltd 2021
// MIT License. For licence terms see LICENCE.md file.

#include <chrono>
#include <filesystem>

#include "clara/clara.hpp"
#include "spdlog/spdlog.h"

#include "ClientWrapper.hpp"

struct Params
{
  std::string clientAddress;
  std::uint16_t clientPort;
  std::string filename;
  double dataRateMbps;
  std::uint16_t mtuSize;
  std::string logLevel;
};

inline Params parseArgs(int argc, char **argv)
{
  bool showHelp = false;
  std::string filename;
  std::string clientAddress;
  std::uint16_t clientPort;
  std::uint16_t mtuSize = 1500;
  double dataRateMbps = 0;
  std::string logLevel = "info";
  const auto cli = clara::Help(showHelp) |
                   clara::Opt(filename, "filename")["-f"]["--filename"]("name of a file you want to send").required() |
                   clara::Opt(clientAddress, "client address")["-a"]["--address"]("address send packets to").required() |
                   clara::Opt(clientPort, "client port")["-c"]["--clientPort"]("port to send packets to").required() |
                   clara::Opt(mtuSize, "MTU size")["-m"]["--mtu"]("MTU size of the network interface. default 1500") |
                   clara::Opt(dataRateMbps, "date rate in Megabits per second")["-r"]["--datarate"]("data rate of transfer. default as fast as possible") |
                   clara::Opt(logLevel, "Log level")["-l"]["--logLevel"]("Logging level for program output - default info");

  const auto result = cli.parse(clara::Args(argc, argv));
  if (!result)
  {
    spdlog::error(std::string("Unable to parse command line args: ") + result.errorMessage());
    exit(1);
  }

  if (showHelp)
  {
    std::stringstream helpText;
    helpText << cli;
    spdlog::info(helpText.str());
    exit(1);
  }

  return {clientAddress, clientPort, filename, dataRateMbps, mtuSize, logLevel};
}

int main(int argc, char **argv)
{
  const auto params = parseArgs(argc, argv);
  spdlog::set_level(spdlog::level::from_str(params.logLevel));
  spdlog::info("Starting Enterprise Diode Client application.");

  try
  {
    ClientWrapper(
      params.clientAddress,
      params.clientPort,
      params.mtuSize,
      params.dataRateMbps,
      params.filename,
      params.logLevel
    ).sendData(params.filename);
  }
  catch (const std::exception& exception)
  {
    spdlog::error(std::string("Caught exception: ") + exception.what());
    return 2;
  }
}
