// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef APE_BASE
#define APE_BASE

#include "id3v1.hpp"
#include "logger.hpp"
#include "result.hpp"

namespace ape {

static const std::string modifiedEnding(".ape.mod");

constexpr uint32_t GetTagFooterSize(void) { return static_cast<uint32_t>(32); }

constexpr uint32_t GetTagHeaderSize(void) { return static_cast<uint32_t>(32); }

constexpr uint32_t OffsetFromFrameStartToFrameID(void) {
  return static_cast<uint32_t>(8);
}

typedef struct _frameConfig {
  uint32_t frameStartPosition;
  uint32_t frameContentPosition;
  uint32_t frameLength;
  std::optional<std::string> frameContent;
} frameProperties_t;

auto extractTheTag(id3::buffer_t buffer, uint32_t start, uint32_t length) {
  auto ret = id3::ExtractString(buffer, start, length);
  id3::stripLeft(ret);
  return ret;
}

std::optional<id3::buffer_t> UpdateFrameSize(id3::buffer_t buffer,
                                             uint32_t extraSize,
                                             uint32_t frameIDPosition) {
  const uint32_t frameSizePositionInArea = frameIDPosition;
  constexpr uint32_t frameSizeLengthInArea = 4;
  constexpr uint32_t frameSizeMaxValuePerElement = 255;

  id3::log()->info(" Frame Index: {}", frameIDPosition);
  id3::log()->info(
      "Tag Frame Bytes before update : {}",
      spdlog::to_hex(std::begin(*buffer) + frameSizePositionInArea,
                     std::begin(*buffer) + frameSizePositionInArea + 4));
  const auto ret = id3::updateAreaSize<uint32_t>(
      buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
      frameSizeMaxValuePerElement, false);

  return ret;
}

class apeTagProperties {
private:
  const std::string &FileName;
  uint32_t mfileSize = 0;
  uint32_t mTagSize = 0;
  uint32_t mNumberOfFrames = 0;
  uint32_t mTagFooterBegin = 0;
  uint32_t mTagPayloadPosition = 0;
  uint32_t mTagStartPosition = 0;
  id3::buffer_t mTagBuffer = {};
  execution_status_t m_status{};

  uint32_t getTagSize(std::ifstream &fRead, uint32_t bufferLength) {
    const auto tagLengthBuffer =
        std::make_shared<std::vector<unsigned char>>(bufferLength);
    fRead.read(reinterpret_cast<char *>(tagLengthBuffer->data()), bufferLength);

    return id3::GetTagSizeDefault(tagLengthBuffer, bufferLength);
  }

  const auto readString(std::ifstream &fRead, uint32_t bufferLength) {

    id3::buffer_t Buffer =
        std::make_shared<std::vector<unsigned char>>(bufferLength);
    fRead.read(reinterpret_cast<char *>(Buffer->data()), bufferLength);

    return id3::ExtractString(Buffer, 0, bufferLength);
  }

  bool checkAPEtag(std::ifstream &fRead, uint32_t bufferLength,
                   const std::string &tagToCheck) {
    assert(tagToCheck.size() <= bufferLength);
    const auto stringToCheck = readString(fRead, bufferLength);

    if (bool ret = (stringToCheck == tagToCheck); !ret) {
      ID3_LOG_WARN("error: {} and {}", stringToCheck, tagToCheck);
      return ret;
    } else {
      return ret;
    }
  }

  const auto init(const std::string &fileName,
                  const uint32_t &id3v1TagSizeIfPresent) {
    uint32_t footerPreambleOffsetFromEndOfFile = 0;

    std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);

    if (id3v1TagSizeIfPresent)
      footerPreambleOffsetFromEndOfFile =
          id3v1::TagSize + ape::GetTagFooterSize();
    else
      footerPreambleOffsetFromEndOfFile = ape::GetTagFooterSize();

    mfileSize = filRead.tellg();
    mTagFooterBegin = mfileSize - footerPreambleOffsetFromEndOfFile;
    filRead.seekg(mTagFooterBegin);

    if (uint32_t APEpreambleLength = 8;
        !checkAPEtag(filRead, APEpreambleLength, std::string{"APETAGEX"})) {
      return get_status_error(tag_type_t::ape, rstatus_t::noTag);
    }
    if (uint32_t mVersion = getTagSize(filRead, 4);
        (mVersion != 1000) && (mVersion != 2000)) {
      return get_status_error(tag_type_t::ape, rstatus_t::tagVersionError);
    }
    if (uint32_t mTagSize = getTagSize(filRead, 4); mTagSize == 0) {
      return get_status_error(tag_type_t::ape, rstatus_t::noTagLengthError);
    }
    if (uint32_t mNumberOfFrames = getTagSize(filRead, 4);
        mNumberOfFrames == 0) {
      return get_status_error(tag_type_t::ape, rstatus_t::noFrameIDError);
    }

    mTagStartPosition = mTagFooterBegin - mTagSize;

    mTagPayloadPosition = mTagFooterBegin + GetTagFooterSize() - mTagSize;

    filRead.seekg(mTagStartPosition);
    mTagBuffer = std::make_shared<std::vector<unsigned char>>(
        mTagSize + GetTagFooterSize(), '0');
    filRead.read(reinterpret_cast<char *>(mTagBuffer->data()),
                 mTagSize + GetTagFooterSize());

    return get_status_error(tag_type_t::ape, rstatus_t::noError);
  }

public:
  explicit apeTagProperties(const std::string &fileName,
                            uint32_t id3v1TagSizeIfPresent)
      : FileName{fileName} {
    m_status = init(fileName, id3v1TagSizeIfPresent);
  }

  uint32_t getTagPayloadPosition() const { return mTagPayloadPosition; }
  uint32_t getTagStartPosition() const { return mTagStartPosition; }
  uint32_t getTagFooterBegin() const { return mTagFooterBegin; }
  uint32_t getFileLength() const { return mfileSize; }
  id3::buffer_t GetBuffer() const { return mTagBuffer; }
  const auto GetStatus() const { return m_status; }
};

class tagReader {

public:
  explicit tagReader(const std::string &FileName, std::string_view FrameID)
      : mFilename{FileName}, frameID{FrameID} {

    bool bContinue = true;

    mTagProperties.emplace(mFilename, true);
    if (mTagProperties.value().GetStatus().rstatus == rstatus_t::noTag) {
      bContinue = false;
    }
    if (!bContinue) { // id3v1 not present
      mTagProperties.emplace(apeTagProperties(mFilename, false));
      if (mTagProperties.value().GetStatus().rstatus == rstatus_t::noTag) {
        bContinue = false;
      }
    }
    if (bContinue) {
      m_status = getFrameProperties();
    }
  }

  const auto getFramePayload(void) const {
    if (noStatusErrorFrom(this->m_status)) {
      return frameContent_t{this->m_status,
                            mFrameProperties->frameContent.value()};
    } else {
      return frameContent_t{m_status, {}};
    }
  }

  auto writeFramePayload(std::string_view framePayload, uint32_t length) const {
    if (framePayload.size() > length) {
      return get_status_error(tag_type_t::ape,
                              rstatus_t::ContentLengthBiggerThanFrameArea);
    }
    frameScopeProperties frameScopeProperties = {};
    frameScopeProperties.frameIDStartPosition =
        mFrameProperties->frameStartPosition + OffsetFromFrameStartToFrameID();

    frameScopeProperties.frameContentStartPosition =
        mFrameProperties->frameContentPosition;
    frameScopeProperties.frameLength = mFrameProperties->frameLength;

    if (mFrameProperties->frameLength >= framePayload.size()) {

      return WriteFile(mFilename, std::string(framePayload),
                       frameScopeProperties);
    } else {
      const uint32_t additionalSize =
          framePayload.size() - mFrameProperties->frameLength;

      const auto writeBackAction =
          extendTagBuffer(frameScopeProperties, framePayload, additionalSize) |
          [&](id3::buffer_t buffer) { return this->ReWriteFile(buffer); };

      auto ret = writeBackAction | [&](bool fileWritten) {
        (void)fileWritten;
        return id3::renameFile(mFilename + ape::modifiedEnding, mFilename);
      };
      if (ret)
        return id3::get_status_error(id3::tag_type_t::ape,
                                     id3::rstatus_t::noError);
      else
        return id3::get_status_error(id3::tag_type_t::ape,
                                     id3::rstatus_t::fileRenamingError);
    }
  }

  /* function not thread safe */
  expected::Result<bool> ReWriteFile(id3::buffer_t buff) const {
    const uint32_t endOfFooter =
        mTagProperties->getTagFooterBegin() + GetTagFooterSize();

    std::ifstream filRead(mFilename, std::ios::binary | std::ios::ate);

    const uint32_t fileSize = filRead.tellg();
    assert(fileSize >= endOfFooter);
    const uint32_t endLength = fileSize - endOfFooter;

    /* read in bytes from footer end until audio file end */
    std::vector<uint8_t> bufFooter;
    bufFooter.reserve((endLength));
    filRead.seekg(endOfFooter);
    filRead.read(reinterpret_cast<char *>(&bufFooter[0]), endLength);

    /* read in bytes from audio file start end until ape area start */
    std::vector<uint8_t> bufHeader;
    bufHeader.reserve((mTagProperties->getTagStartPosition()));
    filRead.seekg(0);
    filRead.read(reinterpret_cast<char *>(&bufHeader[0]),
                 mTagProperties->getTagStartPosition());

    filRead.close();

    const std::string writeFileName = mFilename + ::ape::modifiedEnding;
    ID3_LOG_INFO("file to write: {}", writeFileName);

    std::ofstream filWrite(writeFileName, std::ios::binary | std::ios::app);

    filWrite.seekp(0);
    std::for_each(std::begin(bufHeader),
                  std::begin(bufHeader) + mTagProperties->getTagStartPosition(),
                  [&filWrite](const char &n) { filWrite << n; });

    filWrite.seekp(mTagProperties->getTagStartPosition());
    std::for_each(std::begin(*buff), std::end(*buff),
                  [&filWrite](const char &n) { filWrite << n; });

    filWrite.seekp(endOfFooter);
    std::for_each(bufFooter.begin(), bufFooter.begin() + endLength,
                  [&filWrite](const char &n) { filWrite << n; });

    return expected::makeValue<bool>(true);
  }

  const expected::Result<id3::buffer_t>
  extendTagBuffer(const id3::frameScopeProperties &frameConfig,
                  std::string_view framePayload,
                  uint32_t additionalSize) const {

    const uint32_t relativeBufferPosition =
        mTagProperties->getTagStartPosition();
    const uint32_t tagsSizePositionInHeader =
        mTagProperties->getTagStartPosition() - relativeBufferPosition + 12;
    const uint32_t tagsSizePositionInFooter =
        mTagProperties->getTagFooterBegin() - relativeBufferPosition + 12;
    const uint32_t frameSizePositionInFrameHeader =
        frameConfig.frameIDStartPosition - OffsetFromFrameStartToFrameID() -
        relativeBufferPosition;
    const uint32_t frameContentStart =
        frameConfig.frameContentStartPosition - relativeBufferPosition;

    auto cBuffer = mTagProperties->GetBuffer();

    const auto tagSizeBuff =
        UpdateFrameSize(cBuffer, additionalSize, tagsSizePositionInHeader);

    const auto frameSizeBuff = UpdateFrameSize(cBuffer, additionalSize,
                                               frameSizePositionInFrameHeader);

    assert(frameConfig.getFramePayloadLength() <= cBuffer->size());

    auto finalBuffer = std::move(cBuffer);

    id3::replaceElementsInBuff(frameSizeBuff.value(), finalBuffer,
                               frameSizePositionInFrameHeader);

    id3::replaceElementsInBuff(tagSizeBuff.value(), finalBuffer,
                               tagsSizePositionInHeader);

    id3::replaceElementsInBuff(tagSizeBuff.value(), finalBuffer,
                               tagsSizePositionInFooter);

    auto it = finalBuffer->begin() + frameContentStart +
              frameConfig.getFramePayloadLength();
    finalBuffer->insert(it, additionalSize, 0);

    const auto iter = std::begin(*finalBuffer) + frameContentStart;
    std::transform(iter, iter + framePayload.size(), framePayload.begin(), iter,
                   [](char a, char b) {
                     (void)a;
                     return b;
                   });

    return expected::makeValue<id3::buffer_t>(finalBuffer);
  }

private:
  const std::string &mFilename;
  std::string_view frameID;
  id3::execution_status_t m_status{};
  std::optional<apeTagProperties> mTagProperties{};
  std::unique_ptr<frameProperties_t> mFrameProperties{};

  id3::execution_status_t getFrameProperties() {
    auto buffer = mTagProperties->GetBuffer();

    const auto tagArea = id3::ExtractString(buffer, 0, buffer->size());

    constexpr uint32_t frameKeyTerminatorLength = 1;
    const auto searchFramePosition =
        id3::searchFrame<std::string_view>{tagArea};
    auto frameIDstatus = searchFramePosition.execute(frameID);
    if (frameIDstatus.rstatus != rstatus_t::noError) {
      return get_execution_status(tag_type_t::ape, frameIDstatus);
    }
    const uint32_t frameContentPosition = frameIDstatus.frameIDPosition +
                                          frameID.size() +
                                          frameKeyTerminatorLength;

    if (frameIDstatus.frameIDPosition < OffsetFromFrameStartToFrameID()) {
      return get_status_error(tag_type_t::ape,
                              rstatus_t::frameIDBadPositionError);
    }

    const uint32_t frameStartPosition =
        frameIDstatus.frameIDPosition - OffsetFromFrameStartToFrameID();
    const uint32_t frameLength =
        id3::GetTagSizeDefault(buffer, 4, frameStartPosition);

    mFrameProperties = std::make_unique<frameProperties_t>();
    mFrameProperties->frameContentPosition =
        frameContentPosition + mTagProperties->getTagStartPosition();
    mFrameProperties->frameStartPosition =
        frameStartPosition + mTagProperties->getTagStartPosition();
    mFrameProperties->frameLength = frameLength;

    auto ret = id3::ExtractString(buffer, frameContentPosition, frameLength);
    mFrameProperties->frameContent = id3::stripLeft(ret);

    return frameIDstatus;
  }
}; // namespace ape

const auto getFramePayload(const std::string &filename,
                           std::string_view frameID) {
  const tagReader TagR{filename, frameID};

  return TagR.getFramePayload();
}

auto setFramePayload(const std::string &filename, std::string_view frameID,
                     std::string_view framePayload) {
  const tagReader TagR{filename, frameID};

  return TagR.writeFramePayload(framePayload, framePayload.size());
} // namespace ape

const auto GetTitle(const std::string &filename) {
  std::string_view frameID("TITLE");

  return getFramePayload(filename, frameID);
}

const auto GetLeadArtist(const std::string &filename) {
  std::string_view frameID("ARTIST");

  return getFramePayload(filename, frameID);
}

const auto GetAlbum(const std::string &filename) {
  std::string_view frameID("ALBUM");

  return getFramePayload(filename, frameID);
}

const auto GetYear(const std::string &filename) {
  return getFramePayload(filename, std::string_view("YEAR"));
}

const auto GetComment(const std::string &filename) {
  return getFramePayload(filename, std::string_view("COMMENT"));
}

const auto GetGenre(const std::string &filename) {
  return getFramePayload(filename, std::string_view("GENRE"));
}

const auto GetComposer(const std::string &filename) {
  return getFramePayload(filename, std::string_view("COMPOSER"));
}

auto SetTitle(const std::string &filename, std::string_view content) {
  return ape::setFramePayload(filename, std::string_view("TITLE"), content);
}

auto SetAlbum(const std::string &filename, std::string_view content) {
  return ape::setFramePayload(filename, std::string_view("ALBUM"), content);
}

auto SetLeadArtist(const std::string &filename, std::string_view content) {
  return ape::setFramePayload(filename, std::string_view("ARTIST"), content);
}

auto SetYear(const std::string &filename, std::string_view content) {
  return ape::setFramePayload(filename, std::string_view("YEAR"), content);
}

auto SetComment(const std::string &filename, std::string_view content) {
  return ape::setFramePayload(filename, std::string_view("COMMENT"), content);
}

auto SetGenre(const std::string &filename, std::string_view content) {
  return ape::setFramePayload(filename, std::string_view("GENRE"), content);
}

auto SetComposer(const std::string &filename, std::string_view content) {
  return ape::setFramePayload(filename, std::string_view("COMPOSER"), content);
}
}; // end namespace ape

#endif // APE_BASE
