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

#include "result.hpp"
#include "logger.hpp"

namespace id3v2
{
    using cUchar = std::vector<unsigned char>;

    static const std::string modifiedEnding(".mod");

    class TagInfos {
    public:
        TagInfos(uint32_t TagOffset, uint32_t TagContentOffset,
                 uint32_t Length, uint32_t _encodeFlag = 0,
                 uint32_t _doSwap = 0)
            : tagContentStartPosition(TagContentOffset),
              tagStartPosition(TagOffset),
              length(Length),
              encodeFlag(_encodeFlag),
              doSwap(_doSwap) {}

        TagInfos()
            : tagContentStartPosition(0), tagStartPosition(0), length(0), encodeFlag(0), doSwap(0) {}

        const uint32_t getTagOffset() const { return tagStartPosition; }
        const uint32_t getTagContentOffset() const { return tagContentStartPosition; }
        const uint32_t getLength() const { return length; }
        const uint32_t getEncodingValue() const { return encodeFlag; }
        const uint32_t getSwapValue() const { return doSwap; }

    private:
        // TagInfos() = delete;
        uint32_t tagContentStartPosition;
        uint32_t tagStartPosition;
        uint32_t length;
        uint32_t encodeFlag;
        uint32_t doSwap;
    };
    };

    template <typename T, typename F>
    auto mbind(std::optional<T>&& _obj, F&& function)
        -> decltype(function(_obj.value())) {
        auto fuc = std::forward<F>(function);
        auto obj = std::forward<std::optional<T>>(_obj);

        if (obj.has_value()) {
            return fuc(obj.value());
        } else {
            return {};
        }
}

template <typename T>
using is_optional_string =
    std::is_same<std::decay_t<T>, std::optional<std::string>>;

template <typename T>
using is_optional_vector_char =
    std::is_same<std::decay_t<T>, std::optional<id3v2::cUchar>>;

#if 1
template <
    typename T,
    typename = std::enable_if_t<(
        std::is_same<std::decay_t<T>, std::optional<id3v2::cUchar>>::value ||
        std::is_same<std::decay_t<T>, std::optional<std::string>>::value ||
        std::is_same<std::decay_t<T>, std::optional<std::string_view>>::value ||
        std::is_same<std::decay_t<T>, std::optional<bool>>::value ||
        std::is_same<std::decay_t<T>, std::optional<id3v2::TagInfos>>::value ||
        std::is_same<std::decay_t<T>, std::optional<uint32_t>>::value)>,
    typename F>
auto operator|(T&& _obj, F&& Function)
    -> decltype(std::forward<F>(Function)(std::forward<T>(_obj).value())) {
    auto fuc = std::forward<F>(Function);
    auto obj = std::forward<T>(_obj);

    if (obj.has_value()) {
        return fuc(obj.value());
    } else {
        return {};
    }
}
#endif

expected::Result<bool> renameFile(const std::string& fileToRename,
                                  const std::string& renamedFile) {
    namespace fs = std::experimental::filesystem;

    const fs::path FileToRenamePath =
        fs::path(fs::system_complete(fileToRename));
    const fs::path RenamedFilePath = fs::path(fs::system_complete(renamedFile));

    try {
        fs::rename(FileToRenamePath, RenamedFilePath);
        return expected::makeValue<bool>(true);
    } catch (fs::filesystem_error& e) {
        ID3_LOG_ERROR("could not rename modified audio file: {}", e.what());
    } catch (const std::bad_alloc& e) {
        ID3_LOG_ERROR("Renaming: Allocation failed: ", e.what());
    }

    return expected::makeError<bool>("could rename the modified audio file");
}

template <typename T>
struct integral_unsigned_asserts {
    void operator()() {
        static_assert(
            (std::is_integral<T>::value || std::is_unsigned<T>::value),
            "parameter should be integers");
    }
};

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

namespace id3v2 {

template <typename T>
const uint32_t GetValFromBuffer(const cUchar& buffer, T index,
                                T num_of_bytes_in_hex) {
    integral_unsigned_asserts<T> eval;
    eval();

    assert(buffer.size() > num_of_bytes_in_hex);

    auto version = 0;
    auto remaining = 0;
    auto bytes_to_add = num_of_bytes_in_hex;
    auto byte_to_pad = index;

    while (bytes_to_add > 0) {
        remaining = (num_of_bytes_in_hex - bytes_to_add);
        auto val = std::pow(2, 8 * remaining) * buffer[byte_to_pad];
        version += val;

        bytes_to_add--;
        byte_to_pad--;
    }

    return version;
}

template <typename T>
expected::Result<std::string> GetHexFromBuffer(const cUchar& buffer, T index,
                                               T num_of_bytes_in_hex) {
    integral_unsigned_asserts<T> eval;
    eval();

    assert(buffer.size() > num_of_bytes_in_hex);

    std::stringstream stream_obj;

    stream_obj << "0x" << std::setfill('0')
               << std::setw(num_of_bytes_in_hex * 2);

    const uint32_t version =
        GetValFromBuffer<T>(buffer, index, num_of_bytes_in_hex);

    if (version != 0) {
        stream_obj << std::hex << version;

        return expected::makeValue<std::string>(stream_obj.str());

    } else {
        return expected::makeError<std::string>() << __func__ << (":failed\n");
    }
}

template <typename T1, typename T2>
expected::Result<std::string> ExtractString(const cUchar& buffer, T1 start,
                                            T1 end) {
    if (end < start) {
        ID3_LOG_WARN("error: start {} and end {}", start, end);
    }
    assert(end > start);

    if (buffer.size() >= end) {
        static_assert(std::is_integral<T1>::value,
                      "second and third parameters should be integers");
        static_assert(std::is_unsigned<T2>::value,
                      "num of elements must be unsigned");

        std::string obj(reinterpret_cast<const char*>(buffer.data()), end);

        return expected::makeValue<std::string>(
            obj.substr(start, (end - start)));
    } else {
        return expected::makeError<std::string>() << __func__ << ":failed"
                                                  << " start: " << start
                                                  << " end: " << end << "\n";
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

template <typename T>
expected::Result<cUchar> updateAreaSize(const cUchar& buffer, uint32_t extraSize
        ,T tagIndex, T numberOfElements, T _maxValue) {

    const uint32_t TagIndex = tagIndex;
    const uint32_t NumberOfElements = numberOfElements;
    const uint32_t maxValue = _maxValue;
    auto itIn = std::begin(buffer) + TagIndex;
    auto ExtraSize = extraSize;
    auto extr = ExtraSize % maxValue;

    cUchar temp_vec(NumberOfElements);
    std::copy(itIn, itIn + NumberOfElements, temp_vec.begin());
    auto it = temp_vec.begin();

    /* reverse order of elements */
    std::reverse(it, it + NumberOfElements);

    std::transform(
        it, it + NumberOfElements, it, it, [&](uint32_t a, uint32_t b) {
            extr = ExtraSize % maxValue;
            a = (a >= maxValue) ? maxValue : a;

            if (ExtraSize >= maxValue) {
                const auto rest = maxValue - a;
                a = a + rest;
                ExtraSize -= rest;
            } else {
                auto rest2 = maxValue - a;
                a = (a + ExtraSize > maxValue) ? maxValue : (a + ExtraSize);
                ExtraSize =
                    ((int)(ExtraSize - rest2) < 0) ? 0 : (ExtraSize - rest2);
            }
            return a;
        });

    /* reverse back order of elements */
    std::reverse(it, it + NumberOfElements);

    ID3_LOG_INFO("success...");
    return expected::makeValue<cUchar>(temp_vec);

}

expected::Result<cUchar> updateTagSize(const cUchar& buffer,
                                       uint32_t extraSize) {

    constexpr uint32_t tagSizePositionInHeader = 6;
    constexpr uint32_t tagSizeLengthInHeader = 4;
    constexpr uint32_t tagSizeMaxValuePerElement = 127;

    return updateAreaSize<uint32_t>(buffer, extraSize, tagSizePositionInHeader,
                          tagSizeLengthInHeader, tagSizeMaxValuePerElement);
}

expected::Result<uint32_t> GetTagSize(const cUchar& buffer) {

    using paire = std::pair<uint32_t, uint32_t>;

    const std::vector<uint32_t> pow_val = {21, 14, 7, 0};

    std::vector<paire> result(pow_val.size());
    constexpr uint32_t TagIndex = 6;
    const auto it = std::begin(buffer) + TagIndex;

    std::transform(it, it + pow_val.size(), pow_val.begin(), result.begin(),
                   [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

    const uint32_t val = std::accumulate(
        result.begin(), result.end(), 0,
        [](int a, paire b) { return (a + (b.first * std::pow(2, b.second))); });

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

    auto val = GetStringFromFile(FileName, GetTagHeaderSize<uint32_t>()) |
               [&](const cUchar& _val) {
                   return expected::makeValue<cUchar>(_val);
               };

    if (!val.has_value())
        return expected::makeError<cUchar>() << "Error: " << __func__ << "\n";
    else
        return val;
}

expected::Result<std::string> GetTagArea(const cUchar& buffer) {

    return GetTotalTagSize(buffer) | [&](uint32_t tagSize) {
        ID3_LOG_INFO("{}: tagsize: {}", __func__, tagSize);

        return ExtractString<uint32_t, uint32_t>(buffer, 0, tagSize);
    };
}

template <typename T>
expected::Result<cUchar> processFrameHeaderFlags(
    const cUchar& buffer, uint32_t frameHeaderFlagsPosition, std::string_view tag) {
    constexpr uint32_t frameHeaderFlagsLength = 2;

    const auto it = buffer.begin() + frameHeaderFlagsPosition;

    cUchar temp_vec(frameHeaderFlagsLength);
    std::copy(it, it + 2, temp_vec.begin());

    std::bitset<8> frameStatusFlag(temp_vec[0]);
    std::bitset<8> frameDescriptionFlag(temp_vec[1]);

    if(frameStatusFlag[1]){
        ID3_LOG_WARN("frame {} should be discarded", std::string(tag));
    }
    if(frameStatusFlag[2]){
        ID3_LOG_WARN("frame {} should be discarded", std::string(tag));
    }
    if(frameStatusFlag[3]){
        ID3_LOG_WARN("frame {} is read-only and is not meant to be changed", std::string(tag));
    }

    return expected::makeValue<cUchar>(temp_vec);
}

template <typename id3Type>
void GetTagNames(void) {
    return id3Type::tag_names;
};

expected::Result<std::string> GetID3FileIdentifier(const cUchar& buffer) {
    constexpr auto FileIdentifierStart = 0;
    constexpr auto FileIdentifierEnd = 3;

    return ExtractString<uint32_t, uint32_t>(buffer, FileIdentifierStart,
                                             FileIdentifierEnd);

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

template <typename Type>
class search_tag {

private:
    const Type& mTagArea;
    std::once_flag m_once;
    uint32_t loc;

public:
    search_tag(const Type& tagArea) : mTagArea(tagArea), loc(0) {}

    expected::Result<uint32_t> operator()(Type tag) {

        std::call_once(m_once, [this, &tag]() {

            const auto it = std::search(
                mTagArea.cbegin(), mTagArea.cend(),
                std::boyer_moore_searcher(tag.cbegin(), tag.cend()));

            if (it != mTagArea.cend()) {
                loc = (it - mTagArea.cbegin());
            } else {
                ID3_LOG_WARN("{}: tag not found: {}", __func__, tag);
            }
        });

        if(loc != 0){
                return expected::makeValue<uint32_t>(loc);
        }else{
                return expected::makeError<uint32_t>() << "tag: " << tag
                                                       << " not found";
        }
    }
};

}; //end namespace id3v2




#endif //ID3V2_BASE
