// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3_BASE
#define ID3_BASE

#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <optional>
#include <sstream>
#include <type_traits>
#include <variant>
#include <vector>
#include <bitset>
#include "logger.hpp"
#include "result.hpp"
#include "tagfilesystem.hpp"


namespace id3 {

#ifdef HAS_FS_EXPERIMENTAL
namespace filesystem = std::experimental::filesystem;
#elif HAS_FS
namespace filesystem = std::filesystem;
#endif


static const std::string modifiedEnding(".mod");

/* Frame settings */
class FrameSettings {
public:
    FrameSettings(uint32_t FrameIDOffset, uint32_t FramePayloadOffset, uint32_t Length,
             uint32_t _encodeFlag = 0, uint32_t _doSwap = 0):
          frameIDStartPosition(FrameIDOffset),
          frameContentStartPosition(FramePayloadOffset),
          length(Length),
          encodeFlag(_encodeFlag),
          doSwap(_doSwap) {}

    FrameSettings() {}

    const uint32_t getFrameKeyOffset() const { return frameIDStartPosition; }
    const uint32_t getFramePayloadOffset() const {
        return frameContentStartPosition;
    }
    const uint32_t getPayloadLength() const { return length; }
    const uint32_t getEncodingValue() const { return encodeFlag; }
    const uint32_t getSwapValue() const { return doSwap; }

private:
    // FrameSettings() = delete;
    uint32_t frameIDStartPosition = 0;
    uint32_t frameContentStartPosition = 0;
    uint32_t length = 0;
    uint32_t encodeFlag = 0;
    uint32_t doSwap = 0;
};

const std::string stripLeft(const std::string& valIn) {
    auto val = valIn;

    val.erase(
        remove_if(val.begin(), val.end(), [](char c) { return !isprint(c); }),
        val.end());

    return val;
}

template <typename T>
expected::Result<T> contructNewFrameSettings(const T& frameConfig,
                                         uint32_t additionalFramePayloadSize) {
    const T FrameConfig{
        frameConfig.getFrameKeyOffset(), frameConfig.getFramePayloadOffset(),
        frameConfig.getPayloadLength() + additionalFramePayloadSize, 
        frameConfig.getEncodingValue(),  frameConfig.getSwapValue()};

    return expected::makeValue<T>(FrameConfig);
}

template <typename T>
constexpr T RetrieveSize(T n) {
    return n;
}

template <typename T>
struct integral_unsigned_asserts {
    void operator()() {
        static_assert(
            (std::is_integral<T>::value || std::is_unsigned<T>::value),
            "parameter should be integers");
    }
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
    std::is_same<std::decay_t<T>, std::optional<std::vector<uint8_t>>>;

template <
    typename T,
    typename = std::enable_if_t<(
        std::is_same<std::decay_t<T>, std::optional<std::vector<uint8_t>>>::value ||
        std::is_same<std::decay_t<T>, std::optional<std::string>>::value ||
        std::is_same<std::decay_t<T>, std::optional<std::string_view>>::value ||
        std::is_same<std::decay_t<T>, std::optional<bool>>::value ||
        std::is_same<std::decay_t<T>, std::optional<id3::FrameSettings>>::value ||
        std::is_same<std::decay_t<T>, std::optional<uint32_t>>::value)>,
    typename F>
auto operator | (T&& _obj, F&& Function)
    -> decltype(std::forward<F>(Function)(std::forward<T>(_obj).value())) {
    auto fuc = std::forward<F>(Function);
    auto obj = std::forward<T>(_obj);

    if (obj.has_value()) {
        return fuc(obj.value());
    } else {
        return {};
    }
}

expected::Result<bool> WriteFile(const std::string& FileName, const std::string& content,
                                 const FrameSettings& frameSettings) {
    std::fstream filWrite(FileName,
                          std::ios::binary | std::ios::in | std::ios::out);

    if (!filWrite.is_open()) {
        return expected::makeError<bool>() << __func__
                                           << ": Error opening file\n";
    }

    filWrite.seekp(frameSettings.getFramePayloadOffset());

    std::for_each(content.begin(), content.end(),
                  [&filWrite](const char& n) { filWrite << n; });

    assert(frameSettings.getPayloadLength() >= content.size());

    for (uint32_t i = 0; i < (frameSettings.getPayloadLength() - content.size());
         ++i) {
        filWrite << '\0';
    }

    return expected::makeValue<bool>(true);
}

expected::Result<bool> renameFile(const std::string& fileToRename,
                                  const std::string& renamedFile) {

    namespace fs = ::id3::filesystem;

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
const uint32_t GetValFromBuffer(const std::vector<uint8_t>& buffer, T index,
                                T num_of_bytes_in_hex) {
    integral_unsigned_asserts<T> eval;
    eval();

    assert(buffer.size() > num_of_bytes_in_hex);

    auto version = 0;
    auto remaining = 0;
    auto bytes_to_add = num_of_bytes_in_hex;
    auto byte_to_pad = index;

    assert(bytes_to_add >=0);

    while (bytes_to_add > 0) {
        remaining = (num_of_bytes_in_hex - bytes_to_add);
        auto val = std::pow(2, 8 * remaining) * buffer[byte_to_pad];
        version += val;

        bytes_to_add--;
        byte_to_pad--;
    }

    return version;
}

template <typename T1>
expected::Result<std::string> ExtractString(const std::vector<uint8_t>& buffer, T1 start,
                                            T1 length) {
    assert((start + length) <= buffer.size());

    if (buffer.size() >= (start + length)) {
        static_assert(std::is_integral<T1>::value,
                      "second and third parameters should be integers");

        std::string obj(reinterpret_cast<const char*>(buffer.data()), (start + length));

        return expected::makeValue<std::string>(
            obj.substr(start, (length)));
    } else {
        ID3_LOG_ERROR("Buffer size {} < buffer length: {} ", buffer.size(), length);
        return expected::makeError<std::string>() << __func__ << ":failed"
                                                  << " start: " << start
                                                  << " end: " << (start + length) << "\n";
    }
}

uint32_t GetTagSizeDefault(const std::vector<uint8_t>& buffer,
                    uint32_t length, uint32_t startPosition = 0, bool BigEndian = false) {

    assert((startPosition + length) <= buffer.size());

    using paire = std::pair<uint32_t, uint32_t>;
    std::vector<uint32_t> power_values(length);
    uint32_t n = 0;

    std::generate(power_values.begin(), power_values.end(), [&n]{ n+=8; return n-8; });

    if(BigEndian){
        std::reverse(power_values.begin(), power_values.begin() + power_values.size());
    }

    std::vector<paire> result(power_values.size());
    const auto it = std::begin(buffer) + startPosition;

    std::transform(it, it + length, power_values.begin(),
                   result.begin(),
                   [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

    const uint32_t val = std::accumulate(
        result.begin(), result.end(), 0,
        [](int a, paire b) { return (a + (b.first * std::pow(2, b.second))); });

    return val;
}

bool replaceElementsInBuff(const std::vector<uint8_t>& buffIn, std::vector<uint8_t>& buffOut,
                           uint32_t position) {
    const auto iter = std::begin(buffOut) + position;

    std::transform(iter, iter + buffIn.size(), buffIn.begin(), iter,
                   [](char a, char b) { return b; });

    return true;
}

uint32_t GetTagSize(const std::vector<uint8_t>& buffer,
                    const std::vector<unsigned int>& power_values,
                    uint32_t index) {

    using paire = std::pair<uint32_t, uint32_t>;
    std::vector<paire> result(power_values.size());
    const auto it = std::begin(buffer) + index;

    std::transform(it, it + power_values.size(), power_values.begin(),
                   result.begin(),
                   [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

    const uint32_t val = std::accumulate(
        result.begin(), result.end(), 0,
        [](int a, paire b) { return (a + (b.first * std::pow(2, b.second))); });

    return val;
}

template <typename T>
expected::Result<std::vector<uint8_t>> updateAreaSize(const std::vector<uint8_t>& buffer,
                                        uint32_t extraSize, T tagIndex,
                                        T numberOfElements, T _maxValue
                                        , bool littleEndian = true) {
    const uint32_t TagIndex = tagIndex;
    const uint32_t NumberOfElements = numberOfElements;
    const uint32_t maxValue = _maxValue;
    auto itIn = std::begin(buffer) + TagIndex;
    auto ExtraSize = extraSize;
    auto extr = ExtraSize % maxValue;

    std::vector<uint8_t> temp_vec(NumberOfElements);
    std::copy(itIn, itIn + NumberOfElements, temp_vec.begin());
    auto it = temp_vec.begin();

    if(littleEndian){
        /* reverse order of elements */
        std::reverse(it, it + NumberOfElements);
    }

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

    if(littleEndian){
        /* reverse back order of elements */
         std::reverse(it, it + NumberOfElements);
    }

    id3::log()->trace(
        "Tag Frame Bytes after update : {}",
        spdlog::to_hex(std::begin(temp_vec) ,
                       std::begin(temp_vec) + NumberOfElements));
    ID3_LOG_TRACE("success...");

    return expected::makeValue<std::vector<uint8_t>>(temp_vec);
}

template <typename Type>
class search_tag {
private:
    const Type& mTagArea;
    std::once_flag m_once;
    uint32_t loc = 0;

public:
    search_tag(const Type& tagArea) : mTagArea(tagArea) {}

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

        if (loc != 0) {
            return expected::makeValue<uint32_t>(loc);
        } else {
            return expected::makeError<uint32_t>() << "tag: " << tag
                                                   << " not found";
        }
    }
};


};  // end namespace id3

#endif  // ID3_BASE
