#ifndef APE_BASE
#define APE_BASE

#include "id3.hpp"
#include "logger.hpp"
#include "result.hpp"

namespace ape {

using cUchar = id3::cUchar;

constexpr uint32_t GetTagFooterSize(void) { return 32; }

constexpr uint32_t GetTagHeaderSize(void) { return 32; }

typedef struct _frameConfig {
    uint32_t frameStartPosition;
    uint32_t frameContentPosition;
    uint32_t frameLength;
    std::string frameContent;
} frameConfig;

struct tagReadWriter {
private:
    std::once_flag m_once;
    bool mValid;
    const std::string& FileName;
    uint32_t mVersion;
    uint32_t mTagSize;
    uint32_t mNumberOfFrames;
    uint32_t mTagFooterBegin;
    uint32_t mTagPayloadPosition;
    std::optional<cUchar> buffer;

    const uint32_t getTagSize(std::ifstream& fRead, uint32_t bufferLength) {
        std::vector<unsigned char> tagLengthBuffer(bufferLength);
        fRead.read(reinterpret_cast<char*>(tagLengthBuffer.data()),
                   bufferLength);

        return id3::GetTagSizeDefault(tagLengthBuffer, bufferLength);
    }

    const auto readString(std::ifstream& fRead, uint32_t bufferLength) {
        assert(bufferLength > 0);
        std::vector<unsigned char> Buffer(bufferLength);
        fRead.read(reinterpret_cast<char*>(Buffer.data()), bufferLength);

        const auto ret =
            id3::ExtractString<uint32_t>(Buffer, 0, bufferLength) |
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
        bool ret = (stringToCheck == tagToCheck);
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
          mVersion(0),
          mTagSize(0),
          mNumberOfFrames(0),
          mTagFooterBegin(0),
          mTagPayloadPosition(0),
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

                const unsigned int dataSize = filRead.tellg();
                mTagFooterBegin = dataSize - footerPreambleOffsetFromEndOfFile;
                filRead.seekg(mTagFooterBegin);

                ID3_LOG_INFO("APE tag filename: {} start: {}", fileName, mTagFooterBegin);
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
                    ID3_LOG_ERROR("APE tagReadWriter Version: Version = {}", mVersion);
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
                mTagPayloadPosition =
                    mTagFooterBegin + GetTagFooterSize() - mTagSize;

                filRead.seekg(mTagPayloadPosition);
                std::vector<unsigned char> buffer1(mTagSize, '0');
                filRead.read(reinterpret_cast<char*>(buffer1.data()), mTagSize);

                mValid = true;
                buffer = buffer1;
            }
        });
    }

    const uint32_t getTagPayloadPosition() const { return mTagPayloadPosition; }
    std::optional<cUchar> GetBuffer() const { return buffer; }

    bool IsValid() const { return mValid; }
};

const expected::Result<std::string> extractTheTag(const cUchar& buffer,
                                                  uint32_t start,
                                                  uint32_t length) {
    return id3::ExtractString<uint32_t>(buffer, start, length) |
           [](const std::string& readTag) {
               return expected::makeValue<std::string>(id3::stripLeft(readTag));
           };
}

const expected::Result<frameConfig> getTag(const std::string& filename,
                                           std::string_view tagKey){

    auto tagRW = std::make_unique<tagReadWriter>(filename, true);
    if (!tagRW->IsValid()) {
        tagRW.reset(new tagReadWriter(filename, false));
        if (!tagRW->IsValid()) {
           return expected::makeError<frameConfig>("ape not valid");
        }else{
            ID3_LOG_TRACE("tagReadWriter with no id3v1 exists!");
        }
    }else{
        ID3_LOG_TRACE("tagReadWriter with id3v1 exists!");
    }

    if(!tagRW->GetBuffer().has_value()){
        return expected::makeError<frameConfig>("ape not valid");
    }

    cUchar buffer = tagRW->GetBuffer().value();

    return id3::ExtractString<uint32_t>(
               buffer, 0, buffer.size()) |
           [&](const std::string& tagArea) {

               constexpr uint32_t OffsetFromTagKey = 8;
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
                   return expected::makeError<frameConfig>()
                          << "frame Key: " << std::string(tagKey)
                          << " could not be found\n";
                   ID3_LOG_TRACE("position not found: {} for tag name: #{}",
                                 frameContentPosition, std::string(tagKey));
               }

               ID3_LOG_TRACE("key position found: {} for tag name: #{}",
                             frameKeyPosition.value(), std::string(tagKey));

               frameConfig fConfig = {0};
               assert(frameKeyPosition.value() >= OffsetFromTagKey);

               const uint32_t frameStartPosition =
                   frameKeyPosition.value() - OffsetFromTagKey;
               const uint32_t frameLength =
                   id3::GetTagSizeDefault(buffer, 4, frameStartPosition);
               ID3_LOG_TRACE("key #{} - Frame length: {}", std::string(tagKey), frameLength);

               fConfig.frameContentPosition =
                   frameContentPosition + tagRW->getTagPayloadPosition();
               fConfig.frameStartPosition =
                   frameStartPosition + tagRW->getTagPayloadPosition();
               fConfig.frameLength = frameLength;

               if (!frameContentPosition) {
                   return expected::makeError<frameConfig>()
                          << "frame Key: " << std::string(tagKey)
                          << " - Wrong frameLength\n";
                   ID3_LOG_ERROR("framelength: {} for tag name: #{}",
                                 frameLength, std::string(tagKey));
               }

               const auto frameContent =
                   extractTheTag(buffer, frameContentPosition, frameLength);
               if (frameContent.has_value()) {
                   fConfig.frameContent = frameContent.value();
               } else {
                   fConfig.frameContent = "";
               }

               return expected::makeValue<frameConfig>(fConfig);
           };
}

const expected::Result<bool> SetTheTag(const std::string& filename, std::string_view tagKey,
        std::string_view content, uint32_t start,
                                       uint32_t length,
                                       bool id3v1Present = false) {
    if (content.size() > length) {
        ID3_LOG_ERROR("content length too big foe frame area");
        return expected::makeError<bool>(
            "content length too big foe frame area");
    }

    const auto fConfig = getTag(filename, tagKey);
    if (!fConfig.has_value()) {
        ID3_LOG_ERROR("Could not retrieve Key");
        return expected::makeError<bool>("Could not retrieve key");
    } else {

        const id3::TagInfos frameGlobalConfig(
            fConfig.value().frameStartPosition + 8,
            fConfig.value().frameContentPosition, fConfig.value().frameLength);

        ID3_LOG_INFO("ID3V1: Key: {} Write content: {} at {}", std::string(tagKey), std::string(content),
                     fConfig.value().frameContentPosition);

        return WriteFile(filename, std::string(content), frameGlobalConfig);
    }
}

const expected::Result<std::string> GetTag(const std::string& filename, std::string_view tagKey) {
    const auto ret = getTag(filename, tagKey);
    if (ret.has_value()){
        return expected::makeValue<std::string>(ret.value().frameContent);
    }else{
        ID3_LOG_WARN("Tag {} Not found", std::string(tagKey));
        return expected::makeError<std::string>("Not found");
    }
}

const expected::Result<std::string> GetTitle(const std::string& filename) {
    std::string_view tagKey("TITLE");

    return GetTag(filename, tagKey);
}

const expected::Result<std::string> GetLeadArtist(const std::string& filename) {
    std::string_view tagKey("ARTIST");

    return GetTag(filename, tagKey);
}

const expected::Result<std::string> GetAlbum(const std::string& filename) {
    std::string_view tagKey("ALBUM");

    return GetTag(filename, tagKey);
}

const expected::Result<std::string> GetYear(const std::string& filename) {
    return GetTag(filename, std::string_view("YEAR"));
}

const expected::Result<std::string> GetComment(const std::string& filename) {
    return GetTag(filename, std::string_view("COMMENT"));
}

const expected::Result<std::string> GetGenre(const std::string& filename) {
    return GetTag(filename, std::string_view("GENRE"));
}

const expected::Result<std::string> GetComposer(const std::string& filename) {
    return GetTag(filename, std::string_view("COMPOSER"));
}

const expected::Result<bool> SetTitle(const std::string& filename,
                                      std::string_view content) {
    return ape::SetTheTag(filename, std::string_view("TITLE"), content, 0, content.size());
}

const expected::Result<bool> SetAlbum(const std::string& filename,
                                      std::string_view content) {
    return ape::SetTheTag(filename,std::string_view("ALBUM"), content, 0, content.size());
}


const expected::Result<bool> SetLeadArtist(
    const std::string& filename, std::string_view content) {
    return ape::SetTheTag(filename,std::string_view("ARTIST"), content, 0, content.size());
}

const expected::Result<bool> SetYear(const std::string& filename,
                                      std::string_view content) {
    return ape::SetTheTag(filename,std::string_view("YEAR"), content, 0, content.size());
}

const expected::Result<bool> SetComment(const std::string& filename,
                                             std::string_view content) {
    return ape::SetTheTag(filename,std::string_view("COMMENT"), content, 0, content.size());
}

const expected::Result<bool> SetGenre(const std::string& filename,
                                           std::string_view content) {
    return ape::SetTheTag(filename,std::string_view("GENRE"), content, 0, content.size());
}

const expected::Result<bool> SetComposer(const std::string& filename,
                                           std::string_view content) {
    return ape::SetTheTag(filename,std::string_view("COMPOSER"), content, 0, content.size());
}


};  // end namespace ape

#endif  // APE_BASE
