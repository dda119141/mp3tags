// Copyright(c) 2015-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3V1_BASE
#define ID3V1_BASE

#include "id3.hpp"
#include "result.hpp"
#include "logger.hpp"

namespace id3v1 {

constexpr uint32_t GetTagSize(void) {
    return 128;
}

struct tagReadWriter
{
    private:
        std::once_flag m_once;
        bool mHasId3v1Tag = false;
        const std::string& FileName;
        uint32_t tagBegin = 0;
        uint32_t tagPayload = 0;
        uint32_t tagPayloadLength = 0;
        uint32_t tagHeaderLength = 3;
        std::optional<std::vector<uint8_t>> buffer;

    public:
        explicit tagReadWriter(const std::string& fileName):
        FileName(fileName)
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
                        mHasId3v1Tag = true;
                    }

                    filRead.seekg(tagPayload);
                    std::vector<unsigned char> buffer1(tagPayloadLength, '0');
                    filRead.read(reinterpret_cast<char*>(buffer1.data()), tagPayloadLength);
                    buffer = buffer1;
                });
        }

        const uint32_t GetTagPayload() const { return tagPayload; }

        std::optional<std::vector<uint8_t>> GetBuffer() const {return buffer;}

        bool HasId3v1Tag() const { return mHasId3v1Tag; }
};

const expected::Result<std::vector<uint8_t>> GetBuffer(const std::string& FileName) {
    const tagReadWriter tagRW{FileName};

    if (tagRW.GetBuffer().has_value()) {
        return expected::makeValue<std::vector<uint8_t>>(tagRW.GetBuffer().value());
    } else {
        return expected::makeError<std::vector<uint8_t>>() << "__func__"
                                             << ": Error opening file";
    }
}

const expected::Result<std::string> GetTheFramePayload(const std::vector<uint8_t>& buffer, uint32_t start,
                                           uint32_t end) {
    assert(end > start);

    return id3::ExtractString<uint32_t>(buffer, start, (end - start)) |
           [](const std::string& readTag) {
               return expected::makeValue<std::string>(id3::stripLeft(readTag));
           };
}

const expected::Result<bool> SetFramePayload(const std::string& filename,
                                       std::string_view content,
                                       uint32_t relativeFramePayloadStart, 
                                       uint32_t relativeFramePayloadEnd) {
    assert(relativeFramePayloadEnd > relativeFramePayloadStart);

    const tagReadWriter tagRW{filename};

    if (!tagRW.HasId3v1Tag()) {
        return expected::makeError<bool>("id3v1 not valid");

    } else if (content.size() > (relativeFramePayloadEnd - relativeFramePayloadStart)) {
        ID3_LOG_ERROR("content length {} too big for frame area", content.size());

        return expected::makeError<bool>(
            "content length too big foe frame area");
    }

    const auto frameSettings = id3::FrameSettings()
        .with_frameID_offset(tagRW.GetTagPayload() + relativeFramePayloadStart)
        .with_framecontent_offset(tagRW.GetTagPayload() + relativeFramePayloadStart)
        .with_frame_length(tagRW.GetTagPayload() + relativeFramePayloadStart);

    ID3_LOG_INFO("ID3V1: Write content: {} at {}", std::string(content), tagRW.GetTagPayload());

    return WriteFile(filename, std::string(content), frameSettings);
}

const expected::Result<bool> SetTitle(const std::string& filename,
                                           std::string_view content) {
    return id3v1::SetFramePayload(filename, content, 0, 30);
}

const expected::Result<bool> SetLeadArtist(
    const std::string& filename, std::string_view content) {
    return id3v1::SetFramePayload(filename, content, 30, 60);
}

const expected::Result<bool> SetAlbum(const std::string& filename,
                                           std::string_view content) {
    return id3v1::SetFramePayload(filename, content, 60, 90);
}

const expected::Result<bool> SetYear(const std::string& filename,
                                      std::string_view content) {
    return id3v1::SetFramePayload(filename, content, 90, 94);
}

const expected::Result<bool> SetComment(const std::string& filename,
                                             std::string_view content) {
    return id3v1::SetFramePayload(filename, content, 94, 124);
}

const expected::Result<bool> SetGenre(const std::string& filename,
                                           std::string_view content) {
    return id3v1::SetFramePayload(filename, content, 124, 125);
}

const expected::Result<std::string> GetTitle(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const std::vector<uint8_t>& buffer) { return GetTheFramePayload(buffer, 0, 30); };
}

const expected::Result<std::string> GetLeadArtist(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const std::vector<uint8_t>& buffer) { return GetTheFramePayload(buffer, 30, 60); };
}

const expected::Result<std::string> GetAlbum(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const std::vector<uint8_t>& buffer) { return GetTheFramePayload(buffer, 60, 90); };
}

const expected::Result<std::string> GetYear(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const std::vector<uint8_t>& buffer) { return GetTheFramePayload(buffer, 90, 94); };
}

const expected::Result<std::string> GetComment(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const std::vector<uint8_t>& buffer) { return GetTheFramePayload(buffer, 94, 124); };
}

const expected::Result<std::string> GetGenre(const std::string& filename) {
    return id3v1::GetBuffer(filename) |
           [](const std::vector<uint8_t>& buffer) { return GetTheFramePayload(buffer, 124, 125); };
}

};  // end namespace id3v1

#endif //ID3V1_BASE
