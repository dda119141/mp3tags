// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef APE_BASE
#define APE_BASE

#include "id3v1.hpp"
#include "logger.hpp"
#include "result.hpp"

namespace ape
{

static const std::string modifiedEnding(".ape.mod");

constexpr auto GetTagFooterSize = 32;
constexpr auto GetTagHeaderSize = 32;
constexpr auto OffsetFromFrameStartToFrameID = 8;

typedef struct _frameConfig {
  uint32_t frameStartPosition;
  uint32_t frameContentPosition;
  uint32_t frameLength;
  std::optional<std::string> frameContent;
} frameProperties_t;

const auto UpdateFrameSize(const std::vector<char> &buffer, uint32_t extraSize,
                           uint32_t frameIDPosition)
{
  const uint32_t frameSizePositionInArea = frameIDPosition;
  constexpr uint32_t frameSizeLengthInArea = 4;
  constexpr uint32_t frameSizeMaxValuePerElement = 255;

  const auto ret = id3::updateAreaSize<uint32_t>(
      buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
      frameSizeMaxValuePerElement, false);

  return ret.value();
}

class apeTagProperties
{
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

  uint32_t getTagSize(std::ifstream &fRead, uint32_t bufferLength)
  {
    std::vector<unsigned char> tagbufferLength(bufferLength);
    fRead.read(reinterpret_cast<char *>(tagbufferLength.data()), bufferLength);

    return id3::GetTagSizeDefaultO(tagbufferLength, bufferLength);
  }

  bool checkAPEtag(std::ifstream &fRead, uint32_t bufferLength,
                   const std::string &tagToCheck)
  {

    std::string apeIdentifier{'-'};
    apeIdentifier.resize(bufferLength);
    fRead.read(apeIdentifier.data(), bufferLength);

    return (apeIdentifier == tagToCheck);
  }

  const auto init(const std::string &fileName,
                  const uint32_t &id3v1TagSizeIfPresent)
  {
    uint32_t footerPreambleOffsetFromEndOfFile = 0;

    std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);

    if (id3v1TagSizeIfPresent)
      footerPreambleOffsetFromEndOfFile =
          id3v1::TagSize + ape::GetTagFooterSize;
    else
      footerPreambleOffsetFromEndOfFile = ape::GetTagFooterSize;

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

    mTagPayloadPosition = mTagFooterBegin + GetTagFooterSize - mTagSize;

    filRead.seekg(mTagStartPosition);
    mTagBuffer =
        std::make_unique<std::vector<char>>(mTagSize + GetTagFooterSize, '0');
    filRead.read(reinterpret_cast<char *>(mTagBuffer->data()),
                 mTagSize + GetTagFooterSize);

    return get_status_error(tag_type_t::ape, rstatus_t::noError);
  }

public:
  explicit apeTagProperties(const std::string &fileName,
                            uint32_t id3v1TagSizeIfPresent)
      : FileName{fileName}
  {
    m_status = init(fileName, id3v1TagSizeIfPresent);
  }

  uint32_t getTagPayloadPosition() const { return mTagPayloadPosition; }
  uint32_t getTagStartPosition() const { return mTagStartPosition; }
  uint32_t getTagFooterBegin() const { return mTagFooterBegin; }
  uint32_t getFileLength() const { return mfileSize; }
  auto &GetTagView() const { return mTagBuffer; }
  const auto GetStatus() const { return m_status; }
};

class tagReader
{

public:
  explicit tagReader(const std::string &FileName) : mFilename{FileName}
  {

    bool bContinue = true;

    mTagProperties.emplace(mFilename, true);
    if (mTagProperties.value().GetStatus().rstatus == rstatus_t::noTag) {
      m_status = mTagProperties.value().GetStatus();
      bContinue = false;
    }
    if (!bContinue) { // id3v1 not present
      mTagProperties.emplace(apeTagProperties(mFilename, false));
      if (mTagProperties.value().GetStatus().rstatus == rstatus_t::noTag) {
        m_status = mTagProperties.value().GetStatus();
        bContinue = false;
      }
    }
  }

  const auto getFramePayload(std::string_view FrameID) const
  {
    if (noStatusErrorFrom(this->m_status)) {
      frameProperties_t frameProperties;

      const auto status = this->getFrameProperties(FrameID, frameProperties);

      return frameContent_t{status, frameProperties.frameContent.value()};
    } else {
      return frameContent_t{m_status, {}};
    }
  }

  auto writeFramePayload(std::string_view framePayload,
                         std::string_view frameID) const
  {
    if (!noStatusErrorFrom(this->m_status)) {
      return get_status_error(tag_type_t::ape, m_status.rstatus);
    }

    frameProperties_t frameProperties;
    if (const auto status = this->getFrameProperties(frameID, frameProperties);
        !noStatusErrorFrom(status)) {
      return get_status_error(tag_type_t::ape, status.rstatus);
    }

    frameScopeProperties frameScopeProperties;
    frameScopeProperties.frameIDStartPosition =
        frameProperties.frameStartPosition + OffsetFromFrameStartToFrameID;

    frameScopeProperties.frameContentStartPosition =
        frameProperties.frameContentPosition;
    frameScopeProperties.frameLength = frameProperties.frameLength;

    if (frameProperties.frameLength >= framePayload.size()) {

      return WriteFile(mFilename, framePayload, frameScopeProperties);
    } else {
      const uint32_t additionalSize =
          framePayload.size() - frameProperties.frameLength;

      /* extend tag buffer */
      const auto bufferExtended =
          extendTagBuffer(frameScopeProperties, framePayload, additionalSize);
      if (!bufferExtended.has_value())
        return id3::get_status_error(id3::tag_type_t::ape,
                                     id3::rstatus_t::noExtendedTagError);

      const auto writeBackAction = this->ReWriteFile(bufferExtended.value());
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
  expected::Result<bool> ReWriteFile(const id3::buffer_t &buff) const
  {
    const uint32_t endOfFooter =
        mTagProperties->getTagFooterBegin() + GetTagFooterSize;

    std::ifstream filRead(mFilename, std::ios::binary | std::ios::ate);

    const uint32_t fileSize = filRead.tellg();
    assert(fileSize >= endOfFooter);
    const uint32_t endLength = fileSize - endOfFooter;

    /* read in bytes from footer end until audio file end */
    std::vector<char> bufFooter;
    bufFooter.reserve((endLength));
    filRead.seekg(endOfFooter);
    filRead.read(reinterpret_cast<char *>(&bufFooter[0]), endLength);

    /* read in bytes from audio file start end until ape area start */
    std::vector<char> bufHeader;
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

  const auto getStatus() const { return m_status; }

private:
  const std::string &mFilename;
  id3::execution_status_t m_status{};
  std::optional<apeTagProperties> mTagProperties{};

  const id3::execution_status_t
  getFrameProperties(std::string_view FrameID,
                     frameProperties_t &frameProperties) const
  {
    auto &tagbuffer = mTagProperties->GetTagView();

    std::string_view tagArea{tagbuffer->data(), tagbuffer->size()};

    /* Frame ID lookup and validation check */
    const auto searchFramePosition =
        id3::searchFrame<std::string_view>{tagArea};
    const auto frameIDstatus = searchFramePosition.execute(FrameID);
    if (frameIDstatus.rstatus != rstatus_t::noError) {
      return get_execution_status(tag_type_t::ape, frameIDstatus);
    }
    if (frameIDstatus.frameIDPosition < OffsetFromFrameStartToFrameID) {
      return get_status_error(tag_type_t::ape,
                              rstatus_t::frameIDBadPositionError);
    }

    constexpr uint32_t frameKeyTerminatorLength = 1;
    const uint32_t frameContentPosition = frameIDstatus.frameIDPosition +
                                          FrameID.size() +
                                          frameKeyTerminatorLength;

    const uint32_t frameStartPosition =
        frameIDstatus.frameIDPosition - OffsetFromFrameStartToFrameID;
    const uint32_t frameLength =
        id3::GetTagSizeDefault(*tagbuffer, 4, frameStartPosition);

    frameProperties.frameContentPosition =
        frameContentPosition + mTagProperties->getTagStartPosition();
    frameProperties.frameStartPosition =
        frameStartPosition + mTagProperties->getTagStartPosition();
    frameProperties.frameLength = frameLength;

    auto ret =
        id3::ExtractString(*tagbuffer, frameContentPosition, frameLength);
    frameProperties.frameContent = id3::stripLeft(ret);

    return frameIDstatus;
  }

  std::optional<id3::buffer_t>
  extendTagBuffer(const id3::frameScopeProperties &frameConfig,
                  std::string_view framePayload, uint32_t additionalSize) const
  {

    const uint32_t relativeBufferPosition =
        mTagProperties->getTagStartPosition();
    const uint32_t tagsSizePositionInHeader =
        mTagProperties->getTagStartPosition() - relativeBufferPosition + 12;
    const uint32_t tagsSizePositionInFooter =
        mTagProperties->getTagFooterBegin() - relativeBufferPosition + 12;
    const uint32_t frameSizePositionInFrameHeader =
        frameConfig.frameIDStartPosition - OffsetFromFrameStartToFrameID -
        relativeBufferPosition;
    const uint32_t frameContentStart =
        frameConfig.frameContentStartPosition - relativeBufferPosition;

    auto &TagBuffer = mTagProperties->GetTagView();

    const auto tagSizeBuff =
        UpdateFrameSize(*TagBuffer, additionalSize, tagsSizePositionInHeader);

    const auto frameSizeBuff = UpdateFrameSize(*TagBuffer, additionalSize,
                                               frameSizePositionInFrameHeader);

    assert(frameConfig.getFramePayloadLength() <= TagBuffer->size());

    id3::replaceElementsInBuff(frameSizeBuff, *TagBuffer,
                               frameSizePositionInFrameHeader);

    id3::replaceElementsInBuff(tagSizeBuff, *TagBuffer,
                               tagsSizePositionInHeader);

    id3::replaceElementsInBuff(tagSizeBuff, *TagBuffer,
                               tagsSizePositionInFooter);

    auto it = TagBuffer->begin() + frameContentStart +
              frameConfig.getFramePayloadLength();
    TagBuffer->insert(it, additionalSize, 0);

    const auto iter = std::begin(*TagBuffer) + frameContentStart;
    std::transform(iter, iter + framePayload.size(), framePayload.begin(), iter,
                   [](char a, char b) {
                     (void)a;
                     return b;
                   });

    return std::make_unique<std::vector<char>>((*TagBuffer));
  }

}; // namespace ape

class handle_t
{
  const tagReader TagR;

public:
  handle_t(const std::string &filename) : TagR(filename) {}

  const auto getStatus() const { return TagR.getStatus(); }

  const auto getFramePayload(meta_entry entry) const
  {
    switch (entry) {
    case meta_entry::album:
      return TagR.getFramePayload("ALBUM");
      break;
    case meta_entry::artist:
      return TagR.getFramePayload("ARTIST");
      break;
    case meta_entry::comments:
      return TagR.getFramePayload("COMMENT");
      break;
    case meta_entry::genre:
      return TagR.getFramePayload("GENRE");
      break;
    case meta_entry::title:
      return TagR.getFramePayload("TITLE");
      break;
    case meta_entry::year:
      return TagR.getFramePayload("YEAR");
      break;
    case meta_entry::composer:
      return TagR.getFramePayload("COMPOSER");
      break;
    default:
      return TagR.getFramePayload("");
      break;
    };
    return TagR.getFramePayload("");
  };
};

const auto setFramePayload(const std::string &filename,
                           std::string_view frameID,
                           std::string_view framePayload)
{
  const tagReader TagR{filename};

  return TagR.writeFramePayload(framePayload, frameID);
} // namespace ape

const auto getApePayload = getPayload<ape::handle_t>;

const auto GetTitle(const std::string &filename)
{
  return getApePayload(filename)(meta_entry::title);
}

const auto GetLeadArtist(const std::string &filename)
{
  return getApePayload(filename)(meta_entry::artist);
}

const auto GetAlbum(const std::string &filename)
{
  return getApePayload(filename)(meta_entry::album);
}

const auto GetYear(const std::string &filename)
{
  return getApePayload(filename)(meta_entry::year);
}

const auto GetComment(const std::string &filename)
{
  return getApePayload(filename)(meta_entry::comments);
}

const auto GetGenre(const std::string &filename)
{
  return getApePayload(filename)(meta_entry::genre);
}

const auto GetComposer(const std::string &filename)
{
  return getApePayload(filename)(meta_entry::composer);
}

auto SetTitle(const std::string &filename, std::string_view content)
{
  return ape::setFramePayload(filename, std::string_view("TITLE"), content);
}

auto SetAlbum(const std::string &filename, std::string_view content)
{
  return ape::setFramePayload(filename, std::string_view("ALBUM"), content);
}

auto SetLeadArtist(const std::string &filename, std::string_view content)
{
  return ape::setFramePayload(filename, std::string_view("ARTIST"), content);
}

auto SetYear(const std::string &filename, std::string_view content)
{
  return ape::setFramePayload(filename, std::string_view("YEAR"), content);
}

auto SetComment(const std::string &filename, std::string_view content)
{
  return ape::setFramePayload(filename, std::string_view("COMMENT"), content);
}

auto SetGenre(const std::string &filename, std::string_view content)
{
  return ape::setFramePayload(filename, std::string_view("GENRE"), content);
}

auto SetComposer(const std::string &filename, std::string_view content)
{
  return ape::setFramePayload(filename, std::string_view("COMPOSER"), content);
}
}; // end namespace ape

#endif // APE_BASE
