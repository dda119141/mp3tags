// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef APE_BASE
#define APE_BASE

#include "id3.hpp"
#include "logger.hpp"
#include "result.hpp"

namespace ape {

static const std::string modifiedEnding(".ape.mod");

constexpr uint32_t GetTagFooterSize(void) { return 32; }

constexpr uint32_t GetTagHeaderSize(void) { return 32; }

constexpr uint32_t OffsetFromFrameStartToFrameID(void) { return 8; }

typedef struct _frameConfig {
    uint32_t frameStartPosition;
    uint32_t frameContentPosition;
    uint32_t frameLength;
    std::string frameContent;
} frameBlock_t;

const expected::Result<std::string> extractTheTag(buffer_t buffer,
                                                  uint32_t start,
                                                  uint32_t length) {
    return id3::ExtractString<uint32_t>(buffer, start, length) |
           [](const std::string& readTag) {
               return expected::makeValue<std::string>(id3::stripLeft(readTag));
           };
}

expected::Result<std::vector<uint8_t>> UpdateFrameSize(buffer_t buffer,
                                         uint32_t extraSize,
                                         uint32_t frameIDPosition) {
    const uint32_t frameSizePositionInArea = frameIDPosition;
    constexpr uint32_t frameSizeLengthInArea = 4;
    constexpr uint32_t frameSizeMaxValuePerElement = 255;

    id3::log()->info(" Frame Index: {}", frameIDPosition);
    id3::log()->info(
        "Tag Frame Bytes before update : {}",
        spdlog::to_hex(std::begin(buffer) + frameSizePositionInArea,
                       std::begin(buffer) + frameSizePositionInArea + 4));
    const auto ret =  id3::updateAreaSize<uint32_t>(
        buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
        frameSizeMaxValuePerElement, false);

    return ret;
}

struct tagReadWriter {
private:
    std::once_flag m_once;
    bool mValid;
    const std::string& FileName;
    uint32_t mfileSize;
    uint32_t mVersion;
    uint32_t mTagSize;
    uint32_t mNumberOfFrames;
    uint32_t mTagFooterBegin;
    uint32_t mTagPayloadPosition;
    uint32_t mTagStartPosition;
    std::optional<std::vector<uint8_t>> buffer;

    const uint32_t getTagSize(std::ifstream& fRead, uint32_t bufferLength) {
        std::vector<unsigned char> tagLengthBuffer(bufferLength);
        fRead.read(reinterpret_cast<char*>(tagLengthBuffer.data()),
                   bufferLength);

        return id3::GetTagSizeDefault(tagLengthBuffer, bufferLength);
    }

    const auto readString(std::ifstream& fRead, uint32_t bufferLength) {
        assert(bufferLength > 0);
        buffer_t Buffer = std::make_shared<std::vector<unsigned char>(bufferLength);
        fRead.read(reinterpret_cast<char*>(Buffer->data()), bufferLength);

        const auto ret = id3::ExtractString<uint32_t>(Buffer, 0, bufferLength) |
                         [](const std::string& readTag) {
                             return expected::makeValue<std::string>(readTag);
                         };
        if (ret.has_value())
            return ret.value();
        else
            return ret.error();
    }

    bool checkAPEtag(std::ifstream& fRead, uint32_t bufferLength,
                     const std::string& tagToCheck) {
        assert(tagToCheck.size() <= bufferLength);

        const auto stringToCheck = readString(fRead, bufferLength);
        const bool ret = (stringToCheck == tagToCheck);
        if (!ret) {
            ID3_LOG_WARN("error: {} and {}", stringToCheck, tagToCheck);
        }

        return ret;
    }

public:
    explicit tagReadWriter(const std::string& fileName,
                           uint32_t id3v1TagSizeIfPresent)
        : mValid(false),
          FileName(fileName),
          mfileSize(0),
          mVersion(0),
          mTagSize(0),
          mNumberOfFrames(0),
          mTagFooterBegin(0),
          mTagPayloadPosition(0),
          mTagStartPosition(0),
          buffer({}) {
        std::call_once(m_once, [this, &fileName, &id3v1TagSizeIfPresent]() {
            bool Ok = false;
            uint32_t footerPreambleOffsetFromEndOfFile = 0;

            std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);
            if (!filRead.good()) {
                ID3_LOG_ERROR("tagReadWriter: Error opening file {}", FileName);
            } else {
                Ok = true;
            }
            if (Ok) {
                if (id3v1TagSizeIfPresent)
                    footerPreambleOffsetFromEndOfFile =
                        id3v1::GetTagSize() + ape::GetTagFooterSize();
                else
                    footerPreambleOffsetFromEndOfFile = ape::GetTagFooterSize();

                mfileSize = filRead.tellg();
                mTagFooterBegin = mfileSize - footerPreambleOffsetFromEndOfFile;
                filRead.seekg(mTagFooterBegin);

                ID3_LOG_INFO("APE tag filename: {} start: {}", fileName,
                             mTagFooterBegin);
                constexpr uint32_t APEpreambleLength = 8;
                if (!checkAPEtag(filRead, APEpreambleLength,
                                 std::string("APETAGEX"))) {
                    ID3_LOG_TRACE("APE tagReadWriter: APETAGEX is not there");
                    Ok = false;
                }
            }
            if (Ok) {
                mVersion = getTagSize(filRead, 4);
                if ((mVersion != 1000) && (mVersion != 2000)) {
                    ID3_LOG_ERROR("APE tagReadWriter Version: Version = {}",
                                  mVersion);
                    ID3_LOG_ERROR("fRead Pos: {}", (uint32_t)filRead.tellg());
                    Ok = false;
                }
            }
            if (Ok) {
                mTagSize = getTagSize(filRead, 4);
                assert(mTagSize > 0);
                if (mTagSize == 0) {
                    ID3_LOG_ERROR("APE tagReadWriter: tagsize = 0");
                    ID3_LOG_ERROR("fRead Pos: {}", (uint32_t)filRead.tellg());
                    Ok = false;
                }
            }
            if (Ok) {
                mNumberOfFrames = getTagSize(filRead, 4);
                assert(mNumberOfFrames > 0);
                if (mNumberOfFrames == 0) {
                    ID3_LOG_WARN("APE tagReadWriter: No frame!");
                    Ok = false;
                }
            }
                if (Ok) {
                    mTagStartPosition = mTagFooterBegin - mTagSize;

                    mTagPayloadPosition =
                        mTagFooterBegin + GetTagFooterSize() - mTagSize;

                    filRead.seekg(mTagStartPosition);
                    std::vector<unsigned char> buffer1(
                        mTagSize + GetTagFooterSize(), '0');
                    filRead.read(reinterpret_cast<char*>(buffer1.data()),
                                 mTagSize + GetTagFooterSize());

                    mValid = true;
                    buffer = buffer1;
                }
        });
    }

    const uint32_t getTagPayloadPosition() const { return mTagPayloadPosition; }
    const uint32_t getTagStartPosition() const { return mTagStartPosition; }
    const uint32_t getTagFooterBegin() const { return mTagFooterBegin; }
    const uint32_t getFileLength() const { return mfileSize; }
    std::optional<buffer_t> GetBuffer() const { return buffer; }

    bool IsValid() const { return mValid; }
};

struct tagReader {
private:
    std::once_flag m_once;
    bool mValid;
    const std::string& filename;
    std::string_view tagKey;
    std::optional<buffer_t> mBuffer;
    std::unique_ptr<tagReadWriter> tagRW;

public:
    explicit tagReader(const std::string& FileName, std::string_view TagKey)
        : mValid(false), filename(FileName), tagKey(TagKey), mBuffer({}) {
        std::call_once(m_once, [this]() {
            tagRW = std::make_unique<tagReadWriter>(filename, true);
            if (!tagRW->IsValid()) {
                tagRW.reset(new tagReadWriter(filename, false));
                if (!tagRW->IsValid()) {
                    ID3_LOG_TRACE("ape not valid!");
                } else {
                    ID3_LOG_TRACE("tagReadWriter with no id3v1 exists!");
                }
            } else {
                ID3_LOG_TRACE("tagReadWriter with id3v1 exists!");
            }

            if (!tagRW->GetBuffer().has_value()) {
                ID3_LOG_TRACE("ape not valid!");
            } else {
                mValid = true;
            }
     });
    }

    const expected::Result<frameBlock_t> getFrameBlock() const {
        if (!mValid)
            return expected::makeError<frameBlock_t>("getFrameBlock object not valid");

        std::vector<uint8_t> buffer = tagRW->GetBuffer().value();

        ID3_LOG_TRACE("getFrameBlock object valid - size: {}", buffer.size());

        return id3::ExtractString<uint32_t>(buffer, 0, buffer.size()) |
               [&](const std::string& tagArea) {

                   constexpr uint32_t frameKeyTerminatorLength = 1;
                   auto searchTagPosition =
                       id3::search_tag<std::string_view>(tagArea);
                   const auto frameKeyPosition = searchTagPosition(tagKey);
                   const uint32_t frameContentPosition =
                       frameKeyPosition.has_value()
                           ? frameKeyPosition.value() + tagKey.size() +
                                 frameKeyTerminatorLength
                           : 0;

                   if (!frameContentPosition) {
                       ID3_LOG_TRACE("position not found: {} for tag name: #{}",
                                     frameContentPosition, std::string(tagKey));
                       return expected::makeError<frameBlock_t>()
                              << "frame Key: " << std::string(tagKey)
                              << " could not be found\n";
                   }

                   ID3_LOG_TRACE("key position found: {} for tag name: #{}",
                                 frameKeyPosition.value(), std::string(tagKey));

                   frameBlock_t frameConfig = {0};
                   assert(frameKeyPosition.value() >= OffsetFromFrameStartToFrameID());

                   const uint32_t frameStartPosition =
                       frameKeyPosition.value() - OffsetFromFrameStartToFrameID();
                   const uint32_t frameLength =
                       id3::GetTagSizeDefault(buffer, 4, frameStartPosition);
                   ID3_LOG_TRACE("key #{} - Frame length: {}",
                                 std::string(tagKey), frameLength);

                   frameConfig.frameContentPosition =
                       frameContentPosition + tagRW->getTagStartPosition();
                   frameConfig.frameStartPosition =
                       frameStartPosition + tagRW->getTagStartPosition();
                   frameConfig.frameLength = frameLength;

                   if (!frameContentPosition) {
                       return expected::makeError<frameBlock_t>()
                              << "frame Key: " << std::string(tagKey)
                              << " - Wrong frameLength\n";
                       ID3_LOG_ERROR("framelength: {} for tag name: #{}",
                                     frameLength, std::string(tagKey));
                   }

                   const auto frameContent =
                       extractTheTag(buffer, frameContentPosition, frameLength);
                   if (frameContent.has_value()) {
                       frameConfig.frameContent = frameContent.value();
                   } else {
                       frameConfig.frameContent = "";
                   }

                   return expected::makeValue<frameBlock_t>(frameConfig);
               };
    }

    const expected::Result<bool> writeFramePayload(std::string_view framePayload,
                                           uint32_t start, uint32_t length) const {
        if (framePayload.size() > length) {
            ID3_LOG_ERROR("framePayload length too big foe frame area");
            return expected::makeError<bool>(
                "framePayload length too big foe frame area");
        }

        const auto frameConfig = this->getFrameBlock();

        if (!frameConfig.has_value()) {
            ID3_LOG_TRACE("Could not retrieve Key");
            return expected::makeError<bool>("Could not retrieve key");
        } else {
            const auto frameGlobalConfig = id3::FrameSettings()
                .with_frameID_offset(frameConfig.value().frameStartPosition + OffsetFromFrameStartToFrameID())
                .with_framecontent_offset(frameConfig.value().frameContentPosition)
                .with_frame_length(frameConfig.value().frameLength);

            ID3_LOG_TRACE("ID3V1: Key: {} Write framePayload: {} at {}",
                          std::string(tagKey), std::string(framePayload),
                          frameConfig.value().frameContentPosition);

            if (frameConfig.value().frameLength >= framePayload.size()) {
                return WriteFile(filename, std::string(framePayload),
                                 frameGlobalConfig);
            } else {
                ID3_LOG_TRACE(
                    "Key: {} framePayload length {} too long for frame length {} - "
                    "extend buffer",
                    std::string(tagKey), framePayload.size(),
                    frameConfig.value().frameLength);

                const uint32_t additionalSize = framePayload.size() - frameConfig.value().frameLength;
                const auto writeBackAction = extendTagBuffer(frameGlobalConfig, framePayload, additionalSize) |
                    [&](const std::vector<uint8_t>& buffer) {
                        return this->ReWriteFile(buffer);
                    };
                return writeBackAction | [&](bool fileWritten) {
                    return id3::renameFile(filename + ape::modifiedEnding, filename);
                };
            }
        }
    }

    expected::Result<bool> ReWriteFile(const std::vector<uint8_t>& buff) const {
        const uint32_t endOf = tagRW->getTagFooterBegin() + GetTagFooterSize();

        std::ifstream filRead(filename, std::ios::binary | std::ios::ate);
        if (!filRead.good()) {
            return expected::makeError<bool>() << "__func__"
                                               << ": Error opening file";
        }

        const uint32_t fileSize = filRead.tellg();
        assert(fileSize >= endOf);
        const uint32_t endLength = fileSize - endOf;

        ID3_LOG_INFO("APE start pos: {}", tagRW->getTagStartPosition());
        ID3_LOG_INFO("file length: {}", fileSize);

        std::vector<uint8_t> bufFooter;
        bufFooter.reserve((endLength));
        if (endLength > 0) {
            filRead.seekg(endOf);
            filRead.read(reinterpret_cast<char*>(&bufFooter[0]), endLength);
        }

        std::vector<uint8_t> bufHeader;
        bufHeader.reserve((tagRW->getTagStartPosition()));
        filRead.seekg(0);
        filRead.read(reinterpret_cast<char*>(&bufHeader[0]),
                     tagRW->getTagStartPosition());

        filRead.close();

        const std::string writeFileName =
            filename + ::ape::modifiedEnding;
        ID3_LOG_INFO("file to write: {}", writeFileName);

        std::ofstream filWrite(writeFileName, std::ios::binary | std::ios::app);

        if (!filWrite.good()) {
            ID3_LOG_ERROR("file could not be opened: {}", writeFileName);
            return expected::makeError<bool>() << "__func__"
                                               << ": Error opening file";
        }

        filWrite.seekp(0);
        std::for_each(std::begin(bufHeader),
                      std::begin(bufHeader) + tagRW->getTagStartPosition(),
                      [&filWrite](const char& n) { filWrite << n; });

        ID3_LOG_TRACE("buffer to write : {}",
                      spdlog::to_hex(std::begin(buff), std::begin(buff) + 8));

        ID3_LOG_INFO("endlength: {}", endLength);

        filWrite.seekp(tagRW->getTagStartPosition());
        std::for_each(std::begin(buff), std::end(buff),
                      [&filWrite](const char& n) { filWrite << n; });

        filWrite.seekp(endOf);
        std::for_each(bufFooter.begin(), bufFooter.begin() + endLength,
                      [&filWrite](const char& n) { filWrite << n; });

        ID3_LOG_INFO("success: {}", __func__);

        return expected::makeValue<bool>(true);
    }

    const expected::Result<std::vector<uint8_t>> extendTagBuffer(
        const id3::FrameSettings& frameConfig, std::string_view framePayload, uint32_t additionalSize) const {

        const uint32_t relativeBufferPosition = tagRW->getTagStartPosition();
        const uint32_t tagsSizePositionInHeader =
            tagRW->getTagStartPosition() - relativeBufferPosition + 12;
        const uint32_t tagsSizePositionInFooter =
            tagRW->getTagFooterBegin() - relativeBufferPosition + 12;
        const uint32_t frameSizePositionInFrameHeader =
            frameConfig.getFrameKeyOffset() - OffsetFromFrameStartToFrameID() - relativeBufferPosition;
        const uint32_t frameContentStart =
            frameConfig.getFramePayloadOffset() - relativeBufferPosition;

        if (!tagRW->GetBuffer().has_value()) {
            ID3_LOG_ERROR("No buffer!...");
            return expected::makeError<std::vector<uint8_t>>("ape:extendTagBuffer - No buffer");
        }
        auto cBuffer = tagRW->GetBuffer().value();
        ID3_LOG_TRACE("FrameSize... length: {}, frame start: {}",
                      frameConfig.getFramePayloadLength(), frameConfig.getFrameKeyOffset());
        ID3_LOG_TRACE("Updating segments...");

        const auto tagSizeBuff =
            UpdateFrameSize(cBuffer, additionalSize, tagsSizePositionInHeader);

        const auto frameSizeBuff = UpdateFrameSize(cBuffer, additionalSize,
                                                   frameSizePositionInFrameHeader);

        assert(frameConfig.getFramePayloadLength() <= cBuffer.size());

        auto finalBuffer = std::move(cBuffer);

        ID3_LOG_TRACE("final Buffer size {} and frameSizePositionInFrameHeader {}...", finalBuffer.size(), frameSizePositionInFrameHeader);

        id3::replaceElementsInBuff(frameSizeBuff.value(), finalBuffer,
                                   frameSizePositionInFrameHeader);

        ID3_LOG_TRACE("tagsSizePositionInHeader {}...", tagsSizePositionInHeader);
        id3::replaceElementsInBuff(tagSizeBuff.value(), finalBuffer,
                                   tagsSizePositionInHeader);

        ID3_LOG_TRACE("tagsSizePositionInFooter {}...", tagsSizePositionInFooter);
        id3::replaceElementsInBuff(tagSizeBuff.value(), finalBuffer,
                                   tagsSizePositionInFooter);


        ID3_LOG_TRACE("previous buffer Size {}...", finalBuffer.size());

        ID3_LOG_TRACE("frame Key Offset {}...", frameConfig.getFrameKeyOffset());
        auto it = finalBuffer.begin() + frameContentStart +
                  frameConfig.getFramePayloadLength();
        finalBuffer.insert(it, additionalSize, 0);

        ID3_LOG_TRACE("frame Content Start {}...", frameContentStart);
        const auto iter = std::begin(*finalBuffer) + frameContentStart;
        std::transform(iter, iter + framePayload.size(), framePayload.begin(), iter,
                       [](char a, char b) { return b; });

        ID3_LOG_TRACE(
            "Tag Frame Bytes after update : {}",
            spdlog::to_hex(
                std::begin(finalBuffer),
                std::begin(finalBuffer) + 8));
        ID3_LOG_TRACE("final buffer Size {}...", finalBuffer.size());

        return expected::makeValue<std::vector<uint8_t>>(finalBuffer);
    }

};  // class tagReader


const expected::Result<std::string> getFramePayload(const std::string& filename,
                                           std::string_view tagKey) {
    const tagReader TagR{filename, tagKey};
    const auto ret = TagR.getFrameBlock();
    if (ret.has_value()) {
        return expected::makeValue<std::string>(ret.value().frameContent);
    } else {
        ID3_LOG_WARN("Tag {} Not found", std::string(tagKey));
        return expected::makeError<std::string>("Not found");
    }
}

const expected::Result<bool> setFramePayload(const std::string& filename, std::string_view tagKey, std::string_view framePayload) {

    const tagReader TagR{filename, tagKey};

    return TagR.writeFramePayload(framePayload, 0, framePayload.size());
}


const expected::Result<std::string> GetTitle(const std::string& filename) {
    std::string_view tagKey("TITLE");

    return getFramePayload(filename, tagKey);
}

const expected::Result<std::string> GetLeadArtist(const std::string& filename) {
    std::string_view tagKey("ARTIST");

    return getFramePayload(filename, tagKey);
}

const expected::Result<std::string> GetAlbum(const std::string& filename) {
    std::string_view tagKey("ALBUM");

    return getFramePayload(filename, tagKey);
}

const expected::Result<std::string> GetYear(const std::string& filename) {
    return getFramePayload(filename, std::string_view("YEAR"));
}

const expected::Result<std::string> GetComment(const std::string& filename) {
    return getFramePayload(filename, std::string_view("COMMENT"));
}

const expected::Result<std::string> GetGenre(const std::string& filename) {
    return getFramePayload(filename, std::string_view("GENRE"));
}

const expected::Result<std::string> GetComposer(const std::string& filename) {
    return getFramePayload(filename, std::string_view("COMPOSER"));
}

const expected::Result<bool> SetTitle(const std::string& filename,
                                      std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("TITLE"), content);
}

const expected::Result<bool> SetAlbum(const std::string& filename,
                                      std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("ALBUM"), content);
}

const expected::Result<bool> SetLeadArtist(const std::string& filename,
                                           std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("ARTIST"), content);
}

const expected::Result<bool> SetYear(const std::string& filename,
                                     std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("YEAR"), content);
}

const expected::Result<bool> SetComment(const std::string& filename,
                                        std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("COMMENT"), content);
}

const expected::Result<bool> SetGenre(const std::string& filename,
                                      std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("GENRE"), content);
}

const expected::Result<bool> SetComposer(const std::string& filename,
                                         std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("COMPOSER"), content);
}

};  // end namespace ape

#endif  // APE_BASE
