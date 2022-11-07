// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _ID3V2_COMMON
#define _ID3V2_COMMON

#include <algorithm>
#include <cmath>
#include <extendTagArea.hpp>
#include <functional>
#include <id3_precheck.hpp>
#include <id3v2_base.hpp>
#include <mutex>
#include <optional>
#include <string_conversion.hpp>
#include <type_traits>
#include <variant>
#include <vector>

#define DEBUG

namespace id3v2
{
void getframePropertiesFromEncodeByte(
    frameScopeProperties &mframeScopeProperties,
    const std::vector<char> &tagBuffer)
{
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
                              const frameScopeProperties &frameProperties)
{
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
                  tagBase::getW16StringFromLatin<std::vector<char>>(content);
              const auto ret = tagBase::swapW16String(val);
              return ret;
            } else {
              return tagBase::getW16StringFromLatin<std::vector<char>>(content);
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
                            std::string_view tagAreaIn)
{
  const auto searchFramePosition =
      id3::searchFrame<std::string_view>(tagAreaIn);

  const auto ret = searchFramePosition.execute(frameID);

  return get_execution_status(tag_type_t::id3v2, ret);
}

class writer;

class TagReader
{
public:
  explicit TagReader(id3::audioProperties_t &&Config)
      : mAudioProperties(std::move(Config)),
        mFileProperties(mAudioProperties.fileScopePropertiesObj),
        mTagHeaderBuffer(new std::vector<char>(id3v2::TagHeaderSize))
  {
    if (mFileProperties.get_filename() == std::string{}) {
      m_status = get_status_error(tag_type_t::id3v2, rstatus_t::noTag);
    } else {
      GetTagHeader(mFileProperties.get_filename(), mTagHeaderBuffer);

      if (const auto tag_payload_size =
              GetTagSizeWithoutHeader(*mTagHeaderBuffer);
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
      mAudioProperties.frameScopePropertiesObj.emplace();
      m_status.rstatus = rstatus_t::noError;
    }
  }

  auto &getHeaderBuffer() const { return mTagHeaderBuffer; }

  auto &getTagBuffer() const { return mTagBuffer; }

  const auto getVariant() const { return mFileProperties.get_tag_type(); }

  const auto getStatus() const { return m_status; }

  const auto getFramePayload(std::string_view FrameID = std::string_view{})
  {
    if (FrameID.empty())
      FrameID = mFileProperties.get_frame_id();

    auto &framePropertiesOpt = mAudioProperties.frameScopePropertiesObj;
    if (framePropertiesOpt.has_value()) {
      m_status = retrieveFrameProperties(FrameID, framePropertiesOpt.value());
      if (m_status.rstatus != rstatus_t::noError) {
        return frameContent_t{m_status, {}};
      }
    } else {
      return frameContent_t{m_status, {}};
    }

    const auto &frameProperties = framePropertiesOpt.value();
    auto framePayload =
        extractString(*mTagBuffer, frameProperties.frameContentStartPosition,
                      frameProperties.getFramePayloadLength());

    id3::stripLeft(framePayload);

    if (frameProperties.doSwap == 0x01) {
      const auto tempPayload = tagBase::swapW16String(framePayload);
      return frameContent_t{this->m_status, tempPayload};
    } else {
      return frameContent_t{this->m_status, framePayload};
    }
  }

  const auto getFrameID(const meta_entry &entry) const
  {
    const auto id3v2_variant = this->getVariant();
    return v2FrameID(id3v2_variant, entry);
  }

private:
  audioProperties_t mAudioProperties;
  fileScopeProperties &mFileProperties;
  buffer_t mTagHeaderBuffer{};
  buffer_t mTagBuffer{};
  execution_status_t m_status{};
  friend class id3v2::writer;

  execution_status_t
  retrieveFrameProperties(std::string_view FrameID,
                          frameScopeProperties &frameProperties) const
  {
    if (!mTagBuffer) {
      return get_status_no_tag_exists(tag_type_t::id3v2);
    }
    std::string_view tagArea{mTagBuffer->data(), mTagBuffer->size()};

    const auto id3Status = GetFramePosition(FrameID, tagArea);
    if (id3Status.rstatus != rstatus_t::noError) {
      return id3Status;
    }

    frameProperties.frameIDStartPosition = id3Status.frameIDPosition;

    /* get total frame length (including frame header length) */
    frameProperties.framePayloadLength =
        GetFrameSize<uint32_t>(*mTagBuffer, mFileProperties.get_tag_version(),
                               frameProperties.frameIDStartPosition)
            .value();
    frameProperties.frameLength =
        frameProperties.framePayloadLength + id3v2::FrameHeaderSize;

    frameProperties.frameContentStartPosition =
        frameProperties.frameIDStartPosition + id3v2::FrameHeaderSize;

    if (FrameID.find_first_of("T") == 0) // if frameID starts with T
    {
      getframePropertiesFromEncodeByte(frameProperties, *mTagBuffer);
    }

    return id3Status;
  }
}; // namespace id3v2

class writer
{
public:
  writer() = default;

  writer(const TagReader &tagReaderIn)
      : mTagReaderIn(tagReaderIn),
        audioPropertiesObj(&tagReaderIn.mAudioProperties),
        fileProperties(audioPropertiesObj->fileScopePropertiesObj)
  {
  }

  const auto writePayload(meta_entry entry, std::string_view content) const
  {
    if (statusErrorFrom(mTagReaderIn.m_status))
      return mTagReaderIn.m_status;

    frameScopeProperties frameProperties;
    frameProperties.tagType = tag_type_t::id3v2;

    auto frameID = mTagReaderIn.getFrameID(entry);

    const auto status =
        mTagReaderIn.retrieveFrameProperties(frameID, frameProperties);

    if (statusErrorFrom(status))
      return status;
    if (content.size() == 0) {
      ID3V2_THROW("frame payload to write size = 0\n");
    }

    IfFrameIsReadOnlyStop(*mTagReaderIn.mTagBuffer,
                          frameProperties.frameIDStartPosition, frameID);

    const auto framePayloadToWrite =
        formatFramePayload(content, frameProperties);

    if (frameProperties.getFramePayloadLength() <
        framePayloadToWrite.size()) // resize whole header
    {
      const uint32_t extraLength =
          framePayloadToWrite.size() - frameProperties.getFramePayloadLength();

      const id3v2::FileExtended fileExtended{*mTagReaderIn.getTagBuffer(),
                                             fileProperties, frameProperties,
                                             extraLength, framePayloadToWrite};
      return fileExtended.get_status();
    } else {
      return id3::writeFile(fileProperties.get_filename(), framePayloadToWrite,
                            frameProperties);
    }
  }

  const auto getStatus() const { return mStatus; }

private:
  id3::execution_status_t mStatus{};
  const TagReader &mTagReaderIn;
  const audioProperties_t *const audioPropertiesObj;
  const fileScopeProperties &fileProperties;
};

class ConstructTag
{
private:
  std::unique_ptr<id3v2::TagReader> TagObj;
  std::optional<id3v2::writer> Writer_m{};

  const auto construct(const std::string &fileName)
  {
    const auto params = [&fileName]() {
      const auto id3Version = preCheckId3(fileName).id3Version;

      switch (id3Version) {
      case id3v2::tagVersion_t::v30: {
        return audioProperties_t{id3v2::fileScopeProperties{
            fileName,
            id3v2::v30() // Tag version
        }};
      } break;
      case id3v2::tagVersion_t::v40: {
        return audioProperties_t{id3v2::fileScopeProperties{
            fileName,
            id3v2::v40() // Tag version
        }};
      } break;
      case id3v2::tagVersion_t::v00: {
        return audioProperties_t{id3v2::fileScopeProperties{
            fileName,
            id3v2::v00() // Tag version
        }};
      } break;
      default: {
        return audioProperties_t{};
      } break;
      }
    };
    return std::make_unique<id3v2::TagReader>(std::move(params()));
  }

public:
  explicit ConstructTag(const std::string &filename)
      : TagObj(construct(filename))
  {
  }

  const auto getStatus() const { return TagObj->getStatus(); }

  const auto isOk() const { return noStatusErrorFrom(TagObj->getStatus()); }

  const auto writeFrame(const meta_entry &entry, std::string_view content)
  {
    if (!Writer_m.has_value())
      Writer_m.emplace(*TagObj);

    return Writer_m.value().writePayload(entry, content);
  }

  const auto getFrame(const meta_entry &entry) const
  {
    auto frameID = TagObj->getFrameID(entry);
    const auto found = TagObj->getFramePayload(frameID);
    return found;
  }
}; // namespace id3v2

}; // end namespace id3v2

#endif //_ID3V2_COMMON
