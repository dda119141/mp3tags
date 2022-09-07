// Copyright(c) 2015-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3V1_BASE
#define ID3V1_BASE

#include "id3.hpp"
#include "logger.hpp"
#include "result.hpp"
#include <iostream>

using namespace id3;

namespace id3v1 {

constexpr uint32_t TagSize = 128;
constexpr uint32_t TagHeaderLength = 3;

struct tagReadWriter {
private:
  const std::string &mFileName;
  uint32_t mTagBegin = 0;
  uint32_t mTagPayloadPosition = 0;
  uint32_t mTagPayloadLength = 0;
  id3::buffer_t mBuffer{};
  execution_status_t mStatus{};

  execution_status_t init(const std::string &fileName) {
    std::ifstream filRead(mFileName, std::ios::binary | std::ios::ate);

    const unsigned int dataSize = filRead.tellg();
    mTagBegin = dataSize - ::id3v1::TagSize;
    filRead.seekg(mTagBegin);

    id3::buffer_t tagHeaderBuffer =
        std::make_shared<std::vector<uint8_t>>(::id3v1::TagHeaderLength, '0');
    filRead.read(reinterpret_cast<char *>(tagHeaderBuffer->data()),
                 ::id3v1::TagHeaderLength);

    const auto stringExtracted =
        id3::ExtractString(tagHeaderBuffer, 0, ::id3v1::TagHeaderLength);
    const auto ret = (stringExtracted == std::string("TAG"));
    if (!ret) {
      return get_status_error(tag_type_t::id3v1, rstatus_t::noTag);
    }

    mTagPayloadPosition = mTagBegin + ::id3v1::TagHeaderLength;
    if (::id3v1::TagSize < ::id3v1::TagHeaderLength)
      return get_status_error(tag_type_t::id3v1,
                              rstatus_t::tagSizeBiggerThanTagHeaderLength);

    mTagPayloadLength = ::id3v1::TagSize - ::id3v1::TagHeaderLength;

    filRead.seekg(mTagPayloadPosition);
    id3::buffer_t buffer1 =
        std::make_shared<std::vector<unsigned char>>(mTagPayloadLength, '0');
    filRead.read(reinterpret_cast<char *>(buffer1->data()), mTagPayloadLength);
    mBuffer = buffer1;

    return get_status_error(tag_type_t::id3v1, rstatus_t::noError);
  }

public:
  explicit tagReadWriter(const std::string &fileName)
      : mFileName(fileName), mBuffer({}) {
    mStatus = init(fileName);
  }

  uint32_t GetTagPayloadPosition() const { return mTagPayloadPosition; }

  frameBuffer_t GetBuffer() const { return frameBuffer_t{mStatus, mBuffer}; }
};

class frameObj_t {

  id3::buffer_t tagBuffer{};
  uint32_t mFrameStartPos = 0;
  uint32_t mFrameEndPos = 0;

public:
  explicit frameObj_t(id3::buffer_t buf, uint32_t frameStartPosIn,
                      uint32_t frameEndPosIn)
      : tagBuffer(buf), mFrameStartPos(frameStartPosIn),
        mFrameEndPos(frameEndPosIn) {

    if (mFrameStartPos > mFrameEndPos)
      throw id3::id3_error("id3v1: start pos > end pos");

    frameContent = id3::ExtractString(tagBuffer, mFrameStartPos,
                                      (mFrameEndPos - mFrameStartPos));

    id3::stripLeft(frameContent.value());
  }

  std::optional<std::string> frameContent{};
};

auto getFramePayload = [](const std::string &filename, uint32_t start,
                          uint32_t end) {
  const tagReadWriter tagRW{filename};
  const auto bufObj = tagRW.GetBuffer();

  if (noStatusErrorFrom(bufObj.parseStatus)) {
    const frameObj_t frameObj{bufObj.tagBuffer, start, end};

    return frameContent_t{bufObj.parseStatus, frameObj.frameContent.value()};
  } else {
    return frameContent_t{bufObj.parseStatus, {}};
  }
};

const auto SetFramePayload(const std::string &filename,
                           std::string_view content,
                           uint32_t relativeFramePayloadStart,
                           uint32_t relativeFramePayloadEnd) {
  if (relativeFramePayloadEnd > relativeFramePayloadStart)
    return get_status_error(tag_type_t::id3v1,
                            rstatus_t::PayloadStartAfterPayloadEnd);

  if (content.size() > (relativeFramePayloadEnd - relativeFramePayloadStart)) {
    return get_status_error(tag_type_t::id3v1,
                            rstatus_t::ContentLengthBiggerThanFrameArea);
  }
  const tagReadWriter tagRW{filename};

  frameScopeProperties frameScopeProperties = {};
  frameScopeProperties.tagType = tag_type_t::id3v1;
  frameScopeProperties.frameIDStartPosition =
      tagRW.GetTagPayloadPosition() + relativeFramePayloadStart;
  frameScopeProperties.frameContentStartPosition =
      tagRW.GetTagPayloadPosition() + relativeFramePayloadStart;
  frameScopeProperties.frameLength =
      relativeFramePayloadEnd - relativeFramePayloadStart;

  ID3_LOG_INFO("ID3V1: Write content: {} at {}", std::string(content),
               tagRW.GetTagPayloadPosition());

  const auto ret =
      WriteFile(filename, std::string(content), frameScopeProperties);

  return ret;
}

const auto SetTitle(const std::string &filename, std::string_view content) {
  return id3v1::SetFramePayload(filename, content, 0, 30);
}

const auto SetLeadArtist(const std::string &filename,
                         std::string_view content) {
  return id3v1::SetFramePayload(filename, content, 30, 60);
}

const auto SetAlbum(const std::string &filename, std::string_view content) {
  return id3v1::SetFramePayload(filename, content, 60, 90);
}

const auto SetYear(const std::string &filename, std::string_view content) {
  return id3v1::SetFramePayload(filename, content, 90, 94);
}

const auto SetComment(const std::string &filename, std::string_view content) {
  return id3v1::SetFramePayload(filename, content, 94, 124);
}

const auto SetGenre(const std::string &filename, std::string_view content) {
  return id3v1::SetFramePayload(filename, content, 124, 125);
}

const auto GetTitle(const std::string &filename) {
  const auto frameObj = getFramePayload;
  return frameObj(filename, 0, 30);
}

const auto GetLeadArtist(const std::string &filename) {
  const auto frameObj = getFramePayload;
  return frameObj(filename, 30, 60);
}

const auto GetAlbum(const std::string &filename) {
  const auto frameObj = getFramePayload;
  return frameObj(filename, 60, 90);
}

const auto GetYear(const std::string &filename) {
  const auto frameObj = getFramePayload;
  return frameObj(filename, 90, 94);
}

const auto GetComment(const std::string &filename) {
  const auto frameObj = getFramePayload;
  return frameObj(filename, 94, 124);
}

const auto GetGenre(const std::string &filename) {
  const auto frameObj = getFramePayload;
  return frameObj(filename, 124, 125);
}

}; // end namespace id3v1

#endif // ID3V1_BASE
