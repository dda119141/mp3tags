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

void getframeScopePropertiesFromEncodeByte(
    frameScopeProperties &mframeScopeProperties,
    const std::vector<uint8_t> &tagBuffer) {

  const auto &frameContentOffset =
      mframeScopeProperties.frameContentStartPosition;

  switch (tagBuffer.at(frameContentOffset)) {
  case 0x0: {

    mframeScopeProperties.frameContentStartPosition += 1;
    mframeScopeProperties.framePayloadLength -= 1;
    break;
  }
  case 0x1: {
    const uint16_t encodeOder = (tagBuffer.at(frameContentOffset + 1) << 8 |
                                 tagBuffer.at(frameContentOffset + 2));
    if (encodeOder == 0xfeff) {
      mframeScopeProperties.doSwap = 1;
    }

    mframeScopeProperties.frameContentStartPosition += 3;
    mframeScopeProperties.framePayloadLength -= 3;
    mframeScopeProperties.encodeFlag = 1;
    break;
  }
  case 0x2: { /* UTF-16BE [UTF-16] encoded Unicode [UNICODE] without
                 BOM */
    mframeScopeProperties.frameContentStartPosition += 1;
    mframeScopeProperties.framePayloadLength -= 1;
    mframeScopeProperties.encodeFlag = 2;
    break;
  }
  case 0x3: { /* UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated
                 with $00 */
    mframeScopeProperties.frameContentStartPosition += 3;
    mframeScopeProperties.framePayloadLength -= 3;
    mframeScopeProperties.encodeFlag = 3;
    break;
  }
  default: {
    mframeScopeProperties.frameContentStartPosition += 1;
    mframeScopeProperties.framePayloadLength -= 1;
    break;
  }
  }
}

const auto formatFramePayload(std::string_view content,
                              const frameScopeProperties &frameProperties) {

  using payloadType =
      std::variant<std::string_view, std::u16string, std::u32string>;

  const payloadType encodeValue = [&] {
    payloadType varEncode;

    if (frameProperties.encodeFlag == 0x01) {
      varEncode = std::u16string{};
    } else if (frameProperties.encodeFlag == 0x02) { // unicode without BOM
      varEncode = std::string_view{""};
    } else if (frameProperties.encodeFlag == 0x03) { // UTF8 unicode
      varEncode = std::u32string{};
    }

    return varEncode;
  }();

  return std::visit(
      overloaded{

          [&](std::string_view arg) {
            (void)arg;
            return std::string{content};
          },
          [&](std::u16string arg) {
            (void)arg;

            if (frameProperties.doSwap == 0x01) {
              auto val =
                  tagBase::getW16StringFromLatin<std::vector<uint8_t>>(content);
              const auto ret = tagBase::swapW16String(val);
              return ret;
            } else {
              return tagBase::getW16StringFromLatin<std::vector<uint8_t>>(
                  content);
            }
          },
          [&](std::u32string arg) {
            (void)arg;
            const auto ret = std::string{content} + '\0';
            return ret;
          },
      },
      encodeValue);
}

const auto GetFramePosition(std::string_view frameID,
                            std::string_view tagAreaIn) {

  const auto searchFramePosition =
      id3::searchFrame<std::string_view>(tagAreaIn);

  const auto ret = searchFramePosition.execute(frameID);

  return get_execution_status(tag_type_t::id3v2, ret);
}

class writer;

class TagReader {
public:
  explicit TagReader(id3::audioProperties_t &&Config)
      : mAudioProperties(std::move(Config)),
        mFileProperties(mAudioProperties.fileScopePropertiesObj),
        mTagHeaderBuffer(new std::vector<uint8_t>(id3v2::TagHeaderSize)) {

    if (mAudioProperties.fileScopePropertiesObj.get_filename() ==
        std::string{}) {
      m_status = get_status_error(tag_type_t::id3v2, rstatus_t::noTag);
    } else {
      GetTagHeader(mFileProperties.get_filename(), mTagHeaderBuffer);

      if (auto tag_payload_size = GetTagSizeWithoutHeader(*mTagHeaderBuffer);
          !tag_payload_size) {
        m_status =
            get_status_error(tag_type_t::id3v2, rstatus_t::noTagLengthError);
      } else {
        const auto tag_size = tag_payload_size + id3v2::TagHeaderSize;

        GetTagBufferFromFile(mFileProperties.get_filename(), tag_size,
                             mTagBuffer);
      }
    }
    if (m_status.rstatus == rstatus_t::idle) {
      if (mFileProperties.get_frame_id().length() == 0) {
        ID3V2_THROW("No frame ID parameter\n");
      } else {
        mAudioProperties.frameScopePropertiesObj.emplace();
      }

      m_status = retrieveFrameProperties(mFileProperties);
    }
  }

  auto &getHeaderBuffer() const { return mTagHeaderBuffer; }
  auto &getTagBuffer() const { return mTagBuffer; }

  const auto getFramePayload() const {

    if (m_status.rstatus != rstatus_t::noError) {
      return frameContent_t{m_status, {}};
    }

    const frameScopeProperties &frameProperties =
        mAudioProperties.frameScopePropertiesObj.value();
    auto framePayload =
        ExtractString(*mTagBuffer, frameProperties.frameContentStartPosition,
                      frameProperties.getFramePayloadLength());

    id3::stripLeft(framePayload);

    if (frameProperties.doSwap == 0x01) {
      const auto tempPayload = tagBase::swapW16String(framePayload);
      return frameContent_t{this->m_status, tempPayload};
    } else {
      return frameContent_t{this->m_status, framePayload};
    }
  }

private:
  audioProperties_t mAudioProperties;
  fileScopeProperties &mFileProperties;
  buffer_t mTagHeaderBuffer{};
  buffer_t mTagBuffer{};
  execution_status_t m_status{};
  friend class id3v2::writer;

  execution_status_t
  retrieveFrameProperties(const fileScopeProperties &params) {
    frameScopeProperties &frameProperties =
        mAudioProperties.frameScopePropertiesObj.value();

    const auto tagArea = GetStringFromBuffer(*mTagBuffer);
    if (tagArea.size() == 0) {
      return get_status_no_tag_exists(tag_type_t::id3v2);
    }

    const auto id3Status = GetFramePosition(params.get_frame_id(), tagArea);
    if (id3Status.rstatus != rstatus_t::noError) {
      return id3Status;
    }

    frameProperties.frameIDStartPosition = id3Status.frameIDPosition;

    /* get total frame length (including frame header length) */
    frameProperties.framePayloadLength =
        GetFrameSize<uint32_t>(*mTagBuffer, params.get_tag_version(),
                               frameProperties.frameIDStartPosition)
            .value();
    frameProperties.frameLength =
        frameProperties.framePayloadLength + id3v2::FrameHeaderSize;

    frameProperties.frameContentStartPosition =
        frameProperties.frameIDStartPosition + id3v2::FrameHeaderSize;

    if (params.get_frame_id().find_first_of("T") ==
        0) // if frameID starts with T
    {
      getframeScopePropertiesFromEncodeByte(frameProperties, *mTagBuffer);
    }

    return id3Status;
  }
}; // namespace id3v2

class writer {
public:
  explicit writer(const TagReader &tagReaderIn)
      : mTagReaderIn(tagReaderIn),
        audioPropertiesObj(&tagReaderIn.mAudioProperties) {

    if (!noStatusErrorFrom(mTagReaderIn.m_status)) {
      ID3V2_THROW(
          get_message_from_status(mTagReaderIn.m_status.rstatus).c_str());
    }
    if (FrameIsReadOnly(
            RetrieveFrameModificationRights(*mTagReaderIn.mTagBuffer))) {
      const auto val =
          std::string(
              audioPropertiesObj->fileScopePropertiesObj.get_frame_id()) +
          std::string(": Read-Only Frame\n");
      ID3V2_THROW(val.c_str())
    }
    CheckAudioPropertiesObject(audioPropertiesObj);

    const auto &params = audioPropertiesObj->fileScopePropertiesObj;
    if (params.get_frame_content_to_write().size() == 0) {
      ID3V2_THROW("frame payload to write size = 0\n");
    }

    const frameScopeProperties &frameProperties =
        audioPropertiesObj->frameScopePropertiesObj.value();

    const std::string framePayloadToWrite = formatFramePayload(
        params.get_frame_content_to_write(), frameProperties);

    if (frameProperties.getFramePayloadLength() <
        framePayloadToWrite.size()) // resize whole header
    {
      const uint32_t extraLength =
          framePayloadToWrite.size() - frameProperties.getFramePayloadLength();

      const id3v2::FileExtended fileExtended{*mTagReaderIn.getTagBuffer(),
                                             audioPropertiesObj, extraLength,
                                             framePayloadToWrite};
      mStatus = fileExtended.get_status();

    } else {
      mStatus = id3::WriteFile(params.get_filename(), framePayloadToWrite,
                               frameProperties);
    }
  }

  const auto getStatus() const { return mStatus; }

private:
  writer() = delete;
  id3::execution_status_t mStatus{};
  const TagReader &mTagReaderIn;
  const audioProperties_t *const audioPropertiesObj;
};
}; // end namespace id3v2

#endif //_ID3V2_COMMON
