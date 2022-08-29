// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _ID3V2_COMMON
#define _ID3V2_COMMON

#include <algorithm>
#include <cmath>
#include <extendTagArea.hpp>
#include <functional>
#include <id3v2_base.hpp>
#include <mutex>
#include <optional>
#include <string_conversion.hpp>
#include <type_traits>
#include <variant>
#include <vector>

#define DEBUG

namespace id3v2 {
std::optional<buffer_t> checkForID3(buffer_t buffer) {
  const auto ID3_Identifier = GetID3FileIdentifier(buffer);

  if (ID3_Identifier != std::string("ID3")) {
    id3::log()->info(" ID3 tag not present");
    return {};
  } else {
    return buffer;
  }
}

void getframeScopePropertiesFromEncodeByte(
    frameScopeProperties &mframeScopeProperties, buffer_t tagBuffer) {

  const auto &frameContentOffset =
      mframeScopeProperties.frameContentStartPosition;

  switch (tagBuffer->at(frameContentOffset)) {
  case 0x0: {

    mframeScopeProperties.frameContentStartPosition += 1;
    mframeScopeProperties.frameLength -= 1;
    break;
  }
  case 0x1: {
    const uint16_t encodeOder = (tagBuffer->at(frameContentOffset + 1) << 8 |
                                 tagBuffer->at(frameContentOffset + 2));
    if (encodeOder == 0xfeff) {
      mframeScopeProperties.doSwap = 1;
    }

    mframeScopeProperties.frameContentStartPosition += 3;
    mframeScopeProperties.frameLength -= 3;
    mframeScopeProperties.encodeFlag = 1;
    break;
  }
  case 0x2: { /* UTF-16BE [UTF-16] encoded Unicode [UNICODE] without
                 BOM */
    mframeScopeProperties.frameContentStartPosition += 1;
    mframeScopeProperties.frameLength -= 1;
    mframeScopeProperties.encodeFlag = 2;
    break;
  }
  case 0x3: { /* UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated
                 with $00 */
    mframeScopeProperties.frameContentStartPosition += 3;
    mframeScopeProperties.frameLength -= 3;
    mframeScopeProperties.encodeFlag = 3;
    break;
  }
  default: {
    mframeScopeProperties.frameContentStartPosition += 1;
    mframeScopeProperties.frameLength -= 1;
    break;
  }
  }
}

std::string formatFramePayload(std::string_view content,
                               const frameScopeProperties &frameProperties) {

  using payloadType =
      std::variant<std::string_view, std::u16string, std::u32string>;

  const payloadType encodeValue = [&] {
    payloadType varEncode;

    if (frameProperties.encodeFlag == 0x01) {
      varEncode = std::u16string();
    } else if (frameProperties.encodeFlag == 0x02) { // unicode without BOM
      varEncode = std::string_view("");
    } else if (frameProperties.encodeFlag == 0x03) { // UTF8 unicode
      varEncode = std::u32string();
    }

    return varEncode;
  }();

  return std::visit(
      overloaded{

          [&](std::string_view arg) {
            (void)arg;
            return std::string(content);
          },

          [&](std::u16string arg) {
            (void)arg;

            if (frameProperties.doSwap == 0x01) {
              std::string_view val =
                  tagBase::getW16StringFromLatin<std::vector<uint8_t>>(content);
              const auto ret = tagBase::swapW16String(val);
              return ret;
            } else {
              const auto ret =
                  tagBase::getW16StringFromLatin<std::vector<uint8_t>>(content);
              return ret;
            }
          },

          [&](std::u32string arg) {
            (void)arg;
            const auto ret = std::string(content) + '\0';
            return ret;
          },

      },
      encodeValue);
}

uint32_t getFramePosition(std::string_view frameID,
                          const std::string &tagAreaOpt) {
  const auto &tagArea = tagAreaOpt;

  auto searchFramePosition = id3::searchFrame<std::string_view>(tagArea);

  return searchFramePosition.execute(frameID);
}

class TagReader {
public:
  explicit TagReader(id3::audioProperties_t Config) : audioProperties(Config) {

    std::call_once(m_once, [this]() {
      fileScopeProperties &fileProperties =
          audioProperties.fileScopePropertiesObj;

      tagBuffer = GetTagHeader(fileProperties.get_filename()) | GetTagSize |
                  [&](uint32_t tags_size) {
                    return CreateTagBufferFromFile(
                        fileProperties.get_filename(),
                        tags_size + GetTagHeaderSize<uint32_t>());
                  };

      if (fileProperties.get_frame_id().length() == 0)
        ID3V2_THROW("No frame ID parameter");

      audioProperties.frameScopePropertiesObj.emplace();

      frameScopeProperties &frameProperties =
          audioProperties.frameScopePropertiesObj.value();

      retrieveFrameProperties(fileProperties);

      if (fileProperties.get_tag_area().size() == 0) {
        ID3V2_THROW("tag area length = 0");
      }
      if (fileProperties.get_tag_Buffer()->size() == 0) {
        ID3V2_THROW("tag buffer length = 0");
      }
      if (frameProperties.getFrameLength() >
          fileProperties.get_tag_area().size()) {
        ID3V2_THROW("frame length > tag area length");
      }
      if (frameProperties.getFramePayloadLength() == 0) {
        ID3V2_THROW("frame payload length == 0");
      }
    });
  }

  const std::string getFramePayload() const {
    frameScopeProperties &frameProperties =
        audioProperties.frameScopePropertiesObj.value();
    const auto framePayload =
        ExtractString(tagBuffer, frameProperties.frameContentStartPosition,
                      frameProperties.getFramePayloadLength());

    if (frameProperties.doSwap == 0x01) {
      const auto tempPayload =
          tagBase::swapW16String(std::string_view(framePayload));
      return tempPayload;
    } else {
      return framePayload;
    }
  }

  audioProperties_t *const GetMediaSettings() const { return &audioProperties; }

private:
  mutable std::once_flag m_once;
  audioProperties_t &audioProperties;
  buffer_t tagBuffer = {};

  void retrieveFrameProperties(const fileScopeProperties &params) {
    frameScopeProperties &frameProperties =
        audioProperties.frameScopePropertiesObj.value();

    const auto tagArea = GetTagArea(tagBuffer);

    frameProperties.frameIDStartPosition =
        getFramePosition(params.get_frame_id(), tagArea);

    frameProperties.frameLength =
        GetFrameSize<uint32_t>(tagBuffer, params.get_tag_version(),
                               frameProperties.frameIDStartPosition)
            .value();

    frameProperties.frameContentStartPosition =
        frameProperties.frameIDStartPosition +
        GetFrameHeaderSize(params.get_tag_version());

    if (params.get_frame_id().find_first_of("T") ==
        0) // if frameID starts with T
    {
      getframeScopePropertiesFromEncodeByte(frameProperties, tagBuffer);
    }

    audioProperties.TagArea = tagArea;
    audioProperties.TagBuffer = tagBuffer;
  }
};

class writer {
public:
  explicit writer(audioProperties_t *const audioPropertiesObjIn)
      : audioPropertiesObj(audioPropertiesObjIn) {

    const auto &params = audioPropertiesObj->fileScopePropertiesObj;

    if (!audioPropertiesObj->frameScopePropertiesObj.has_value()) {
      ID3V2_THROW("frame properties not yet parsed from audio file");
    }

    if (params.get_frame_content_to_write().size() == 0) {
      ID3V2_THROW("frame payload to write size = 0");
    }
  }

  bool execute() const {
    const auto &params = audioPropertiesObj->fileScopePropertiesObj;

    const frameScopeProperties &frameProperties =
        audioPropertiesObj->frameScopePropertiesObj.value();

    const std::string framePayloadToWrite = formatFramePayload(
        params.get_frame_content_to_write(), frameProperties);

    if (frameProperties.getFramePayloadLength() <
        framePayloadToWrite.size()) // resize whole header
    {
      std::cerr << "frames size does not fit\n";

      const uint32_t extraLength = (framePayloadToWrite.size() -
                                    frameProperties.getFramePayloadLength());

      const id3v2::FileExtended fileExtended{audioPropertiesObj, extraLength,
                                             framePayloadToWrite};

      return fileExtended.get_status();

    } else {
      return id3::WriteFile(params.get_filename(), framePayloadToWrite,
                            frameProperties);
    }
  }

private:
  writer() = default;
  audioProperties_t *const audioPropertiesObj;
};

template <typename id3Type> const auto is_tag(std::string name) {
  return (std::any_of(id3v2::GetTagNames<id3Type>().cbegin(),
                      id3v2::GetTagNames<id3Type>().cend(),
                      [&](std::string obj) { return name == obj; }));
}
}; // end namespace id3v2

#endif //_ID3V2_COMMON
