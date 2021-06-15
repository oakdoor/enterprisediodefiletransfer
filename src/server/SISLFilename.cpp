// Copyright PA Knowledge 2021

#include "SISLFilename.hpp"
#include <SislTools/SislTools.hpp>
#include <iostream>
#include <rapidjson/document.h>
#include <regex>

SISLFilename::SISLFilename(std::uint32_t maxSislLength, std::uint32_t maxFilenameLength):
    maxSislLength(maxSislLength),
    maxFilenameLength(maxFilenameLength)
{}

std::optional<std::string> SISLFilename::extractFilename(const BytesBuffer& eofFrame) const
{
  const auto sislHeader = std::string(eofFrame.begin(), eofFrame.end());
  try
  {
    if (sislHeader.size() > maxSislLength)
    {
      std::cerr << "SISL too long" << "\n";
      return std::optional<std::string>();
    }
    const auto filename = convertFromSisl(sislHeader);
    if (filename.size() > maxFilenameLength)
    {
      std::cerr << "Filename too long" << "\n";
      return std::optional<std::string>();
    }
    std::regex filter("[a-zA-Z0-9\\.\\-_]+");

    return std::regex_match(filename, filter) ? filename : std::optional<std::string>();
  }
  catch (UnableToParseSislException&)
  {
    std::cerr << "Unable to parse SISL filename. possible regex problem" << "\n";
    return std::optional<std::string>();
  }
}

std::string SISLFilename::convertFromSisl(const std::string& sislFrame)
{
  const auto doc = parseSisl(sislFrame);
  return doc.HasMember("name") ? doc["name"].GetString() : "";
}

rapidjson::Document SISLFilename::parseSisl(const std::string& sislFrame)
{
  rapidjson::Document doc;
  doc.Parse(SislTools::toJson(sislFrame).c_str());
  return doc;
}