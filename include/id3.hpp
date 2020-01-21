#ifndef ID3_BASE
#define ID3_BASE

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <optional>
#include <sstream>
#include <type_traits>
#include <variant>
#include <vector>
#include "logger.hpp"
#include "result.hpp"

namespace id3 {

using cUchar = std::vector<unsigned char>;

static const std::string modifiedEnding(".mod");

const std::string stripLeft(const std::string& valIn) {
    auto val = valIn;

    val.erase(
        remove_if(val.begin(), val.end(), [](char c) { return !isprint(c); }),
        val.end());

    return val;
}

class TagInfos {
public:
    TagInfos(uint32_t FrameKeyOffset, uint32_t FrameContentOffset, uint32_t Length,
             uint32_t _encodeFlag = 0, uint32_t _doSwap = 0)
        : tagContentStartPosition(FrameContentOffset),
          tagStartPosition(FrameKeyOffset),
          length(Length),
          encodeFlag(_encodeFlag),
          doSwap(_doSwap) {}

    TagInfos()
        : tagContentStartPosition(0),
          tagStartPosition(0),
          length(0),
          encodeFlag(0),
          doSwap(0) {}

    const uint32_t getTagOffset() const { return tagStartPosition; }
    const uint32_t getTagContentOffset() const {
        return tagContentStartPosition;
    }
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

template <typename T>
expected::Result<T> constructNewTagInfos(const T& frameConfig,
                                         uint32_t newtagSize) {
    const T FrameConfig{
        frameConfig.getTagOffset(), frameConfig.getTagContentOffset(),
        frameConfig.getLength() + newtagSize, frameConfig.getEncodingValue(),
        frameConfig.getSwapValue()};

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
    std::is_same<std::decay_t<T>, std::optional<id3::cUchar>>;

template <
    typename T,
    typename = std::enable_if_t<(
        std::is_same<std::decay_t<T>, std::optional<id3::cUchar>>::value ||
        std::is_same<std::decay_t<T>, std::optional<std::string>>::value ||
        std::is_same<std::decay_t<T>, std::optional<std::string_view>>::value ||
        std::is_same<std::decay_t<T>, std::optional<bool>>::value ||
        std::is_same<std::decay_t<T>, std::optional<id3::TagInfos>>::value ||
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

expected::Result<bool> WriteFile(const std::string& FileName, const std::string& content,
                                 const TagInfos& frameInformations) {
    std::fstream filWrite(FileName,
                          std::ios::binary | std::ios::in | std::ios::out);

    if (!filWrite.is_open()) {
        return expected::makeError<bool>() << __func__
                                           << ": Error opening file\n";
    }

    filWrite.seekp(frameInformations.getTagContentOffset());

    std::for_each(content.begin(), content.end(),
                  [&filWrite](const char& n) { filWrite << n; });

    assert(frameInformations.getLength() >= content.size());

    for (uint32_t i = 0; i < (frameInformations.getLength() - content.size());
         ++i) {
        filWrite << '\0';
    }

    return expected::makeValue<bool>(true);
}

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

template <typename T1>
expected::Result<std::string> ExtractString(const cUchar& buffer, T1 start,
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

uint32_t GetTagSizeDefault(const cUchar& buffer,
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

bool replaceElementsInBuff(const cUchar& buffIn, cUchar& buffOut,
                           uint32_t position) {
    const auto iter = std::begin(buffOut) + position;

    std::transform(iter, iter + buffIn.size(), buffIn.begin(), iter,
                   [](char a, char b) { return b; });

    return true;
}

uint32_t GetTagSize(const cUchar& buffer,
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
expected::Result<cUchar> updateAreaSize(const cUchar& buffer,
                                        uint32_t extraSize, T tagIndex,
                                        T numberOfElements, T _maxValue) {
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
