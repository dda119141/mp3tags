// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3_PRECHECK
#define ID3_PRECHECK

#include "id3v2_base.hpp"
#include <cstring>
#include <fstream>
#include <string>

constexpr auto ID3IdentifierStartPosition = 0;
constexpr auto ID3IdentifierLength = 3;

enum class ePreCheck {
  idle = 0,
  getID3String,
  checkForID3,
  getID3version,
  exitFlow,
};

typedef struct id3Precheck_ {
  std::string id3Identifier_s{"==="};
  std::string id3Version_s{"=="};
  id3v2::tagVersion_t id3Type;
} id3Precheck_t;

struct id3Result {
  bool checkResult;
  id3v2::tagVersion_t id3Version;
};

const auto preCheckId3(const std::string &fileName)
{
  id3Precheck_t headerInfo;
  ePreCheck step = ePreCheck::getID3String;
  std::ifstream fil(fileName, std::ios::binary);

  do {
    switch (step) {
    case ePreCheck::getID3String: {
      fil.seekg(ID3IdentifierStartPosition);
      fil.read(headerInfo.id3Identifier_s.data(), ID3IdentifierLength);

      step = ePreCheck::checkForID3;
    } break;

    case ePreCheck::checkForID3: {
      if (headerInfo.id3Identifier_s == std::string{"ID3"}) {
        step = ePreCheck::getID3version;
      } else {
        step = ePreCheck::exitFlow;
      }
    } break;

    case ePreCheck::getID3version: {
      constexpr auto kID3IndexStart = 3;
      constexpr auto kID3VersionBytesLength = 2;

      fil.seekg(0);
      fil.seekg(kID3IndexStart);
      fil.read(headerInfo.id3Version_s.data(), kID3VersionBytesLength);

      const char v04[] = {0x04, 0x00};
      const char v03[] = {0x03, 0x00};
      const char v00[] = {0x00, 0x00};
      const auto parsedVersion = headerInfo.id3Version_s.c_str();

      if (std::strncmp(parsedVersion, v04, sizeof(v04)) == 0) {
        headerInfo.id3Type = id3v2::tagVersion_t::v40;
      } else if (std::strncmp(parsedVersion, v03, sizeof(v03)) == 0) {
        headerInfo.id3Type = id3v2::tagVersion_t::v30;
      } else if (std::strncmp(parsedVersion, v00, sizeof(v00)) == 0) {
        headerInfo.id3Type = id3v2::tagVersion_t::v00;
      } else {
        headerInfo.id3Type = id3v2::tagVersion_t::idle;
      }
      return id3Result{true, headerInfo.id3Type};
    } break;

    case ePreCheck::exitFlow: {
      return id3Result{false, headerInfo.id3Type};
    } break;
    default:
      return id3Result{false, headerInfo.id3Type};
      break;
    }
  } while (step != ePreCheck::exitFlow);

  return id3Result{false, headerInfo.id3Type};
};

#endif // ID3_PRECHECK
