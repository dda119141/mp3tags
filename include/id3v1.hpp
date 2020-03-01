// Copyright(c) 2015-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3V1_BASE
#define ID3V1_BASE

#include "id3.hpp"
#include "result.hpp"
#include "logger.hpp"

namespace id3v1 {
using cUchar = id3::cUchar;

constexpr uint32_t GetTagSize(void) {
    return 128;
}

struct tagReadWriter
{
    private:
        std::once_flag m_once;
        bool mValid;
        const std::string& FileName;
        uint32_t tagBegin;
        uint32_t tagPayload;
        uint32_t tagPayloadLength;
        uint32_t tagHeaderLength;
        std::optional<cUchar> buffer;

    public:
        explicit tagReadWriter(const std::string& fileName):
        mValid(false)
        ,FileName(fileName)
        ,tagBegin(0)
        ,tagPayload(0)
        ,tagPayloadLength(0)
        ,tagHeaderLength(3)
        ,buffer({})
        {
            std::call_once(m_once,
                [this]() {
                    std::ifstream filRead(FileName,
                                          std::ios::binary | std::ios::ate);

                    if (!filRead.good()) {
                        ID3_LOG_ERROR("tagReadWriter: Error opening file {}", FileName);
                    }

                    const unsigned int dataSize = filRead.tellg();
                    tagBegin = dataSize - ::id3v1::GetTagSize();
                    filRead.seekg(tagBegin);

                    std::vector<unsigned char> tagHeaderBuffer(tagHeaderLength,
                                                               '0');
                    filRead.read(
                        reinterpret_cast<char*>(tagHeaderBuffer.data()),
                        tagHeaderLength);

                    const auto ret = id3::ExtractString<uint32_t>(
                                         tagHeaderBuffer, 0, tagHeaderLength) |
                                     [](const std::string& readTag) {
                                         return readTag == std::string("TAG");
                                     };

                    tagPayload = tagBegin + tagHeaderLength;
                    assert(::id3v1::GetTagSize() > tagHeaderLength);
                    tagPayloadLength = ::id3v1::GetTagSize() - tagHeaderLength;

                    if (!ret) {
                        ID3_LOG_WARN("error: start {} and end {}");
                    } else {
                        mValid = true;
                    }

                    filRead.seekg(tagPayload);
                    std::vector<unsigned char> buffer1(tagPayloadLength, '0');
                    filRead.read(reinterpret_cast<char*>(buffer1.data()), tagPayloadLength);
                    buffer = buffer1;
                });
        }

        const uint32_t GetTagPayload() const { return tagPayload; }
        std::optional<cUchar> GetBuffer() const {return buffer;}

        bool IsValid() const {return mValid;}
};

const expected::Result<cUchar> GetBuffer(const std::string& FileName) {
    const tagReadWriter tagRW{FileName};

    if (tagRW.GetBuffer().has_value()) {
        return expected::makeValue<cUchar>(tagRW.GetBuffer().value());
    } else {
        return expected::makeError<cUchar>() << "__func__"
                                             << ": Error opening file";
    }
}

const expected::Result<std::string> GetTheTag(const cUchar& buffer, uint32_t start,
                                           uint32_t end) {
    assert(end > start);

    return id3::ExtractString<uint32_t>(buffer, start, (end - start)) |
           [](const std::string& readTag) {
               return expected::makeValue<std::string>(id3::stripLeft(readTag));
           };
}

const expected::Result<bool> SetTheTag(const std::string& filename,
                                       std::string_view content,
                                       uint32_t start, uint32_t end) {
    assert(end > start);

    const tagReadWriter tagRW{filename};
    if (!tagRW.IsValid()) {
        return expected::makeError<bool>("id3v1 not valid");
    } else if (content.size() > (end - start)) {
        ID3_LOG_ERROR("content length too big foe frame area");
        return expected::makeError<bool>(
            "content length too big foe frame area");
    }

    const id3::TagInfos frameInformations(tagRW.GetTagPayload() + start, tagRW.GetTagPayload() + start, (end - start));

    ID3_LOG_INFO("ID3V1: Write content: {} at {}", std::string(content), tagRW.GetTagPayload());
    return WriteFile(filename, std::string(content), frameInformations);
}

const expected::Result<bool> SetTitle(const std::string& filename,
                                           std::string_view content) {
    return id3v1::SetTheTag(filename, content, 0, 30);
}

const expected::Result<bool> SetLeadArtist(
    const std::string& filename, std::string_view content) {
    return id3v1::SetTheTag(filename, content, 30, 60);
}

const expected::Result<bool> SetAlbum(const std::string& filename,
                                           std::string_view content) {
    return id3v1::SetTheTag(filename, content, 60, 90);
}

const expected::Result<bool> SetYear(const std::string& filename,
                                      std::string_view content) {
    return id3v1::SetTheTag(filename, content, 90, 94);
}

const expected::Result<bool> SetComment(const std::string& filename,
                                             std::string_view content) {
    return id3v1::SetTheTag(filename, content, 94, 124);
}

const expected::Result<bool> SetGenre(const std::string& filename,
                                           std::string_view content) {
    return id3v1::SetTheTag(filename, content, 124, 125);
}

const expected::Result<std::string> GetTitle(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const cUchar& buffer) { return GetTheTag(buffer, 0, 30); };
}

const expected::Result<std::string> GetLeadArtist(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const cUchar& buffer) { return GetTheTag(buffer, 30, 60); };
}

const expected::Result<std::string> GetAlbum(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const cUchar& buffer) { return GetTheTag(buffer, 60, 90); };
}

const expected::Result<std::string> GetYear(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const cUchar& buffer) { return GetTheTag(buffer, 90, 94); };
}

const expected::Result<std::string> GetComment(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const cUchar& buffer) { return GetTheTag(buffer, 94, 124); };
}

const expected::Result<std::string> GetGenre(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const cUchar& buffer) { return GetTheTag(buffer, 124, 125); };
}

};  // end namespace id3v1

#endif //ID3V1_BASE
