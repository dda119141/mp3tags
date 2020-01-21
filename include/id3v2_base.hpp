#ifndef ID3V2_BASE
#define ID3V2_BASE

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
//#include <range/v3/all.hpp>
#include <range/v3/view.hpp>
#include <range/v3/action.hpp>
#include <range/v3/action/transform.hpp>
#include <range/v3/view/filter.hpp>

#include "id3.hpp"
#include "result.hpp"
#include "logger.hpp"

namespace id3v2 {
using namespace id3;
using cUchar = id3::cUchar;

const auto GetStringFromFile(const std::string& FileName, uint32_t num) {
    std::ifstream fil(FileName);
    std::vector<unsigned char> buffer(num, '0');

    if (!fil.good()) {
        return expected::makeError<id3v2::cUchar>() << __func__
                                                    << (":failed\n");
    }

    fil.read(reinterpret_cast<char*>(buffer.data()), num);

    return expected::makeValue<id3v2::cUchar>(buffer);
}

template <typename T>
expected::Result<std::string> GetHexFromBuffer(const cUchar& buffer, T index,
                                               T num_of_bytes_in_hex) {
    id3::integral_unsigned_asserts<T> eval;
    eval();

    assert(buffer.size() > num_of_bytes_in_hex);

    std::stringstream stream_obj;

    stream_obj << "0x" << std::setfill('0')
               << std::setw(num_of_bytes_in_hex * 2);

    const uint32_t version =
        id3::GetValFromBuffer<T>(buffer, index, num_of_bytes_in_hex);

    if (version != 0) {
        stream_obj << std::hex << version;

        return expected::makeValue<std::string>(stream_obj.str());

    } else {
        return expected::makeError<std::string>() << __func__ << (":failed\n");
    }
}

template <typename T>
std::string u8_to_u16_string(T val) {
    return std::accumulate(
        val.begin() + 1, val.end(), std::string(val.substr(0, 1)),
        [](std::string arg1, char arg2) { return arg1 + '\0' + arg2; });
}

template <typename T>
constexpr T GetTagHeaderSize(void) {
    return 10;
}

template <typename T>
constexpr T RetrieveSize(T n) {
    return n;
}

expected::Result<cUchar> updateTagSize(const cUchar& buffer,
                                       uint32_t extraSize) {
    constexpr uint32_t tagSizePositionInHeader = 6;
    constexpr uint32_t tagSizeLengthInHeader = 4;
    constexpr uint32_t tagSizeMaxValuePerElement = 127;

    return updateAreaSize<uint32_t>(buffer, extraSize, tagSizePositionInHeader,
                                    tagSizeLengthInHeader,
                                    tagSizeMaxValuePerElement);
}

expected::Result<uint32_t> GetTagSize(const cUchar& buffer) {

    const std::vector<uint32_t> pow_val = {21, 14, 7, 0};
    constexpr uint32_t TagIndex = 6;

    const auto val = id3::GetTagSize(buffer, pow_val, TagIndex);

    assert(val > GetTagHeaderSize<uint32_t>());

    return expected::makeValue<uint32_t>(val);

#if 0
        if(buffer.size() >= GetTagHeaderSize<uint32_t>())
        {
            auto val = buffer[6] * std::pow(2, 21);

            val += buffer[7] * std::pow(2, 14);
            val += buffer[8] * std::pow(2, 7);
            val += buffer[9] * std::pow(2, 0);

            if(val > GetTagHeaderSize<uint32_t>()){
                return val;
            }
        } else
        {
        }

        return {};
#endif
}

const auto GetTotalTagSize(const cUchar& buffer) {
    return GetTagSize(buffer) | [=](const uint32_t Tagsize) {

        return expected::makeValue<uint32_t>(Tagsize +
                                             GetTagHeaderSize<uint32_t>());
    };
}

const auto GetTagSizeExclusiveHeader(const cUchar& buffer) {
    return GetTagSize(buffer) | [](const int tag_size) {

        return expected::makeValue<uint32_t>(tag_size -
                                             GetTagHeaderSize<uint32_t>());
    };
}

expected::Result<cUchar> GetTagHeader(const std::string& FileName) {
    auto val =
        GetStringFromFile(FileName, GetTagHeaderSize<uint32_t>()) |
        [&](const cUchar& _val) { return expected::makeValue<cUchar>(_val); };

    if (!val.has_value())
        return expected::makeError<cUchar>() << "Error: " << __func__ << "\n";
    else
        return val;
}

expected::Result<std::string> GetTagArea(const cUchar& buffer) {
    return GetTotalTagSize(buffer) | [&](uint32_t tagSize) {
        ID3_LOG_TRACE("{}: tagsize: {}", __func__, tagSize);

        return ExtractString<uint32_t>(buffer, 0, tagSize);
    };
}

template <typename T>
expected::Result<cUchar> processFrameHeaderFlags(
    const cUchar& buffer, uint32_t frameHeaderFlagsPosition,
    std::string_view tag) {
    constexpr uint32_t frameHeaderFlagsLength = 2;

    const auto it = buffer.begin() + frameHeaderFlagsPosition;

    cUchar temp_vec(frameHeaderFlagsLength);
    std::copy(it, it + 2, temp_vec.begin());

    std::bitset<8> frameStatusFlag(temp_vec[0]);
    std::bitset<8> frameDescriptionFlag(temp_vec[1]);

    if (frameStatusFlag[1]) {
        ID3_LOG_WARN("frame {} should be discarded", std::string(tag));
    }
    if (frameStatusFlag[2]) {
        ID3_LOG_WARN("frame {} should be discarded", std::string(tag));
    }
    if (frameStatusFlag[3]) {
        ID3_LOG_WARN("frame {} is read-only and is not meant to be changed",
                     std::string(tag));
    }

    return expected::makeValue<cUchar>(temp_vec);
}

template <typename id3Type>
void GetTagNames(void) {
    return id3Type::tag_names;
};

expected::Result<std::string> GetID3FileIdentifier(const cUchar& buffer) {
    constexpr auto FileIdentifierStart = 0;
    constexpr auto FileIdentifierLength = 3;

    return ExtractString<uint32_t>(buffer, FileIdentifierStart,
                                             FileIdentifierLength);

#ifdef __TEST_CODE
    auto y = [&buffer]() -> std::string {
        return Get_ID3_header_element(buffer, 0, 3);
    };
    return y();
#endif
}

expected::Result<std::string> GetID3Version(const cUchar& buffer) {
    constexpr auto kID3IndexStart = 4;
    constexpr auto kID3VersionBytesLength = 2;

    return GetHexFromBuffer<uint8_t>(buffer, kID3IndexStart,
                                     kID3VersionBytesLength);
}

};  // end namespace id3v2

#endif //ID3V2_BASE
