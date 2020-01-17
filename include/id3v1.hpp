#ifndef ID3V1_BASE
#define ID3V1_BASE

#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <cmath>
#include <variant>
#include <type_traits>
#include <experimental/filesystem>

#include "id3.hpp"
#include "result.hpp"
#include "logger.hpp"

namespace id3v1 {
using cUchar = id3::cUchar;

constexpr uint32_t GetTagSize(void) {
    return 128;
}

const expected::Result<cUchar> GetBuffer(const std::string& FileName) {
    std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);

    if (!filRead.good()) {
        return expected::makeError<cUchar>() << "__func__"
                                             << ": Error opening file";
    }

    const unsigned int dataSize = filRead.tellg();
    const uint32_t tagBegin = dataSize - ::id3v1::GetTagSize();
    filRead.seekg(tagBegin);

    constexpr uint32_t tagHeaderLength = 3;
    std::vector<unsigned char> tagHeaderBuffer(tagHeaderLength, '0');
    filRead.read(reinterpret_cast<char*>(tagHeaderBuffer.data()), tagHeaderLength);

    const auto ret = 
    id3::ExtractString<uint32_t, uint32_t>(tagHeaderBuffer, 0, tagHeaderLength) | [](const std::string& readTag)
    {
        return readTag == std::string("TAG");
    };

    if(!ret){
        ID3_LOG_WARN("error: start {} and end {}");
        return expected::makeError<cUchar>() << "__func__"
                                             << ": No id3v1 tag";
    }

    const uint32_t tagPayload = tagBegin + tagHeaderLength;
    const uint32_t tagPayloadLength = ::id3v1::GetTagSize() - tagHeaderLength;
    filRead.seekg(tagPayload);
    std::vector<unsigned char> buffer(tagPayloadLength, '0');
    filRead.read(reinterpret_cast<char*>(buffer.data()), tagPayloadLength);

    return expected::makeValue<cUchar>(buffer);
}

const expected::Result<std::string> GetTheTag(const cUchar& buffer, uint32_t start,
                                           uint32_t end) {
    assert(end > start);

    return id3::ExtractString<uint32_t, uint32_t>(buffer, start, end) |
           [](const std::string& readTag) {
               return expected::makeValue<std::string>(id3::stripLeft(readTag));
           };
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
           [](const cUchar& buffer) { return GetTheTag(buffer, 60, 90);};
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
