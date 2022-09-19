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
  execution_status_t mStatus{};

  execution_status_t init(const std::string &fileName) {
    std::ifstream filRead(mFileName, std::ios::binary | std::ios::ate);

    /* move the cursor to tag begin */
    const unsigned int dataSize = filRead.tellg();
    mTagBegin = dataSize - ::id3v1::TagSize;
    filRead.seekg(mTagBegin);

    /* extract the tag identifier */
    std::string identifier{'-'};
    identifier.resize(TagHeaderLength);
    filRead.read(identifier.data(), TagHeaderLength);

    /* check tag identifier */
    if (bool ret = (identifier == std::string("TAG")); !ret) {
      return get_status_error(tag_type_t::id3v1, rstatus_t::noTag);
    }

    mTagPayloadPosition = mTagBegin + ::id3v1::TagHeaderLength;
    if (::id3v1::TagSize < ::id3v1::TagHeaderLength)
      return get_status_error(tag_type_t::id3v1,
                              rstatus_t::tagSizeBiggerThanTagHeaderLength);

    mTagPayloadLength = ::id3v1::TagSize - ::id3v1::TagHeaderLength;

    filRead.seekg(mTagPayloadPosition);
    tagStringBuffer.resize(mTagPayloadLength);
    filRead.read(tagStringBuffer.data(), mTagPayloadLength);

    return get_status_error(tag_type_t::id3v1, rstatus_t::noError);
  }

public:
  std::string tagStringBuffer{};
  explicit tagReadWriter(const std::string &fileName)
      : mFileName(fileName), tagStringBuffer({}) {
    mStatus = init(fileName);
  }

  uint32_t GetTagPayloadPosition() const { return mTagPayloadPosition; }

  frameContent_v GetBuffer() const {
    return frameContent_v{mStatus, tagStringBuffer};
  }
};

class frameObj_t {

  std::string_view mTagView{};
  uint32_t mFrameStartPos = 0;
  uint32_t mFrameEndPos = 0;

public:
  explicit frameObj_t(std::string_view tagView, uint32_t frameStartPosIn,
                      uint32_t frameEndPosIn)
      : mTagView(tagView), mFrameStartPos(frameStartPosIn),
        mFrameEndPos(frameEndPosIn) {

    if (mFrameStartPos >= mFrameEndPos)
      throw id3::id3_error("id3v1: start pos > end pos");

    frameContent =
        mTagView.substr(mFrameStartPos, (mFrameEndPos - mFrameStartPos));

    id3::stripLeft(frameContent);
  }

  std::string frameContent{};
};

const auto getFramePayload = [](const std::string &filename, uint32_t start,
                                uint32_t end) {
  const tagReadWriter tagRW{filename};
  const auto bufObj = tagRW.GetBuffer();

  if (noStatusErrorFrom(bufObj.parseStatus)) {
    const frameObj_t frameObj{bufObj.payload, start, end};

    return frameContent_t{bufObj.parseStatus, frameObj.frameContent};
  } else {
    return frameContent_t{bufObj.parseStatus, {}};
  }
};

const auto SetFramePayload(const std::string &filename,
                           std::string_view content,
                           uint32_t relativeFramePayloadStart,
                           uint32_t relativeFramePayloadEnd) {
  if (relativeFramePayloadEnd < relativeFramePayloadStart)
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

  const auto ret = WriteFile(filename, content, frameScopeProperties);

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
