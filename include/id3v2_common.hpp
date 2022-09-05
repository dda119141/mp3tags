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

auto getFramePosition(std::string_view frameID, std::string_view tagAreaIn) {

  const auto searchFramePosition =
      id3::searchFrame<std::string_view>(tagAreaIn);

  return searchFramePosition.execute(frameID);
}

class writer;

class TagReader {
public:
  explicit TagReader(id3::audioProperties_t &&Config)
      : audioProperties(std::move(Config)) {

    fileScopeProperties &fileProperties =
        this->audioProperties.fileScopePropertiesObj;

    tagBuffer =
        GetTagHeader(fileProperties.get_filename()) | GetTagSize |
        [&](uint32_t tags_size) {
          return CreateTagBufferFromFile(fileProperties.get_filename(),
                                         tags_size + id3v2::TagHeaderSize);
        };

    if (fileProperties.get_frame_id().length() == 0)
      ID3V2_THROW("No frame ID parameter");

    this->audioProperties.frameScopePropertiesObj.emplace();

    m_status = retrieveFrameProperties(fileProperties);
    if (m_status.rstatus != rstatus_t::no_error) {
      ID3V2_THROW(get_message_from_status(m_status.rstatus));
    }
  }

  std::string getFramePayload() const {
    const frameScopeProperties &frameProperties =
        audioProperties.frameScopePropertiesObj.value();
    auto framePayload =
        ExtractString(tagBuffer, frameProperties.frameContentStartPosition,
                      frameProperties.getFramePayloadLength());

    if (frameProperties.doSwap == 0x01) {
      const auto tempPayload = tagBase::swapW16String(framePayload);
      return tempPayload;
    } else {
      return framePayload;
    }
  }

private:
  audioProperties_t audioProperties;
  buffer_t tagBuffer = {};
  execution_status_t m_status;
  friend class id3v2::writer;

  execution_status_t
  retrieveFrameProperties(const fileScopeProperties &params) {
    frameScopeProperties &frameProperties =
        audioProperties.frameScopePropertiesObj.value();

    const auto tagArea = GetTagArea(tagBuffer);
    if (tagArea.size() == 0) {
      return get_status_no_tag_exists();
    }

    auto id3Status = getFramePosition(params.get_frame_id(), tagArea);
    if (id3Status.rstatus != rstatus_t::no_error) {
      return id3Status;
    }

    frameProperties.frameIDStartPosition = id3Status.frameIDPosition;

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

    return id3Status;
  }
};

class writer {
public:
  explicit writer(const TagReader &tagReaderIn)
      : audioPropertiesObj(&tagReaderIn.audioProperties) {

    CheckAudioPropertiesObject(audioPropertiesObj);

    const auto &params = audioPropertiesObj->fileScopePropertiesObj;

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

      static const id3v2::FileExtended fileExtended{
          audioPropertiesObj, extraLength, framePayloadToWrite};

      return fileExtended.get_status();

    } else {
      return id3::WriteFile(params.get_filename(), framePayloadToWrite,
                            frameProperties);
    }
  }

private:
  writer() = default;
  const audioProperties_t *const audioPropertiesObj;
};

template <typename id3Type> const auto is_tag(std::string name) {
  return (std::any_of(id3v2::GetTagNames<id3Type>().cbegin(),
                      id3v2::GetTagNames<id3Type>().cend(),
                      [&](std::string obj) { return name == obj; }));
}
}; // end namespace id3v2

#endif //_ID3V2_COMMON
