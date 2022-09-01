// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3_BASE
#define ID3_BASE

#include "id3_exceptions.hpp"
#include "logger.hpp"
#include "tagfilesystem.hpp"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace id3 {

#if defined(HAS_FS_EXPERIMENTAL)
namespace filesystem = std::experimental::filesystem;
#elif defined(_MSVC_LANG)
namespace filesystem = std::filesystem;
#elif defined(HAS_FS)
namespace filesystem = std::filesystem;
#endif

static const std::string modifiedEnding(".mod");
using buffer_t = std::shared_ptr<std::vector<uint8_t>>;
using shared_string_t = std::shared_ptr<std::string>;

enum class severity_t { high = 1, middle, low };
enum class frame_type_val_t { id3v1, id3v2, ape };

struct frame_type {
  unsigned int id3v1_present : 1;
  unsigned int id3v2_present : 1;
  unsigned int ape_present : 1;
};

enum class rstatus_t {
  no_error = 0,
  no_TagLength = 1, // tag container does not exists
  no_frameID,
  frameID_bad_position,
  no_framePayloadLength,
  frameLengthHigherThanTagLength
};

typedef struct execution_status {
  severity_t severity : 3;
  frame_type_val_t frame_type_val : 3;
  rstatus_t rstatus : 4;
  unsigned int frameIDPosition : 16;
} execution_status_t;

typedef struct mp3_execution_status {
  struct frame_type frame_type_v;
  execution_status_t status;
} mp3_execution_status_t;

constexpr execution_status_t get_status_no_tag_exists() {
  execution_status_t val = {};
  val.rstatus = rstatus_t::no_TagLength;
  return val;
}

constexpr execution_status_t get_status_no_frame_ID() {
  execution_status_t val = {};
  val.rstatus = rstatus_t::no_frameID;
  return val;
}

constexpr execution_status_t get_status_frame_ID_pos(unsigned int pos) {
  execution_status_t val = {};
  val.rstatus = rstatus_t::no_error;
  val.frameIDPosition = pos;
  return val;
}

/* Frame settings */
typedef struct frameScopeProperties_t {
  uint16_t frameIDStartPosition = {};
  uint8_t doSwap = {};
  uint8_t frameIDLength = {};
  uint32_t frameContentStartPosition = {};
  uint32_t frameLength = {};
  uint32_t framePayloadLength = {};
  uint32_t encodeFlag = {};

  uint32_t getFramePayloadLength() const {
    if (frameLength && frameIDLength)
      return (frameLength - frameIDLength);
    else
      return frameLength;
  }

  uint32_t getFrameLength() const {
    if (framePayloadLength && frameIDLength)
      return (framePayloadLength + frameIDLength);
    else
      return frameLength;
  }

  void with_additional_payload_size(uint32_t additionalPayload) {
    frameLength += additionalPayload;
    framePayloadLength += additionalPayload;
  }

} frameScopeProperties;

const std::string stripLeft(const std::string &valIn) {
  auto val = valIn;

  val.erase(
      remove_if(val.begin(), val.end(), [](char c) { return !isprint(c); }),
      val.end());

  return val;
}

template <typename T> constexpr T RetrieveSize(T n) { return n; }

template <typename T, typename F>
auto mbind(std::optional<T> &&_obj, F &&function)
    -> decltype(function(_obj.value())) {
  auto fuc = std::forward<F>(function);
  auto obj = std::forward<std::optional<T>>(_obj);

  if (obj.has_value()) {
    return fuc(obj.value());
  } else {
    return {};
  }
}

template <
    typename T,
    typename = std::enable_if_t<(
        std::is_same<std::decay_t<T>, std::optional<std::string>>::value ||
        std::is_same<std::decay_t<T>, std::optional<shared_string_t>>::value ||
        std::is_same<std::decay_t<T>, std::optional<uint32_t>>::value ||
        std::is_same<std::decay_t<T>, std::optional<std::string_view>>::value ||
        std::is_same<std::decay_t<T>, std::optional<buffer_t>>::value ||
        std::is_same<std::decay_t<T>, std::optional<bool>>::value ||
        std::is_same<std::decay_t<T>,
                     std::optional<id3::frameScopeProperties>>::value)>,
    typename F>
auto operator|(T &&_obj, F &&Function)
    -> decltype(std::forward<F>(Function)(std::forward<T>(_obj).value())) {
  auto fuc = std::forward<F>(Function);
  auto obj = std::forward<T>(_obj);

  if (obj.has_value()) {
    return fuc(obj.value());
  } else {
    return {};
  }
}

bool WriteFile(const std::string &FileName, const std::string &content,
               const frameScopeProperties &frameScopeProperties) {
  std::fstream filWrite(FileName,
                        std::ios::binary | std::ios::in | std::ios::out);

  if (!filWrite.is_open()) {
    return false;
  }

  filWrite.seekp(frameScopeProperties.frameContentStartPosition);

  std::for_each(content.begin(), content.end(),
                [&filWrite](const char &n) { filWrite << n; });

  ID3_PRECONDITION(frameScopeProperties.getFramePayloadLength() >=
                   content.size());

  for (uint32_t i = 0;
       i < (frameScopeProperties.getFramePayloadLength() - content.size());
       ++i) {
    filWrite << '\0';
  }

  return true;
}

std::optional<bool> renameFile(const std::string &fileToRename,
                               const std::string &renamedFile) {

  namespace fs = ::id3::filesystem;

  const fs::path FileToRenamePath = fs::path(fs::absolute(fileToRename));
  const fs::path RenamedFilePath = fs::path(fs::absolute(renamedFile));

  try {
    fs::rename(FileToRenamePath, RenamedFilePath);
    return true;
  } catch (fs::filesystem_error &e) {
    ID3_LOG_ERROR("could not rename modified audio file: {}", e.what());
  } catch (const std::bad_alloc &e) {
    ID3_LOG_ERROR("Renaming: Allocation failed: ", e.what());
  }

  return {};
}

template <typename T>
uint32_t GetValFromBuffer(id3::buffer_t buffer, T index,
                          T num_of_bytes_in_hex) {

  constexpr bool is_integrale_asset =
      std::is_integral<T>::value || std::is_unsigned<T>::value;
  static_assert(is_integrale_asset, "Parameter should be integer");

  ID3_PRECONDITION(buffer->size() > num_of_bytes_in_hex);

  auto version = 0;
  auto remaining = 0;
  auto bytes_to_add = num_of_bytes_in_hex;
  auto byte_to_pad = index;

  ID3_PRECONDITION(bytes_to_add >= 0);

  while (bytes_to_add > 0) {
    remaining = (num_of_bytes_in_hex - bytes_to_add);
    auto val = std::pow(2, 8 * remaining) * buffer->at(byte_to_pad);
    version += val;

    bytes_to_add--;
    byte_to_pad--;
  }

  return version;
}

inline std::string_view ExtractString(buffer_t buffer, uint32_t start,
                                      uint32_t length) {
  ID3_PRECONDITION((start + length) <= static_cast<uint32_t>(buffer->size()));

  if (static_cast<uint32_t>(buffer->size()) >= (start + length)) {

    return std::string_view(
        reinterpret_cast<const char *>(buffer->data() + start), length);
  } else {

    ID3_LOG_ERROR("Buffer size {} < buffer length: {} ", buffer->size(),
                  length);
    return {};
  }
}

uint32_t GetTagSizeDefault(buffer_t buffer, uint32_t length,
                           uint32_t startPosition = 0, bool BigEndian = false) {

  ID3_PRECONDITION((startPosition + length) <=
                   static_cast<uint32_t>(buffer->size()));

  using pair_integers = std::pair<uint32_t, uint32_t>;
  std::vector<uint32_t> power_values(length);

  uint32_t n = 0;

  std::generate(power_values.begin(), power_values.end(), [&n] {
    n += 8;
    return n - 8;
  });
  if (BigEndian) {
    std::reverse(power_values.begin(),
                 power_values.begin() + power_values.size());
  }

  std::vector<pair_integers> result(power_values.size());

  const auto it = std::begin(*buffer) + startPosition;
  std::transform(it, it + length, power_values.begin(), result.begin(),
                 [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

  const uint32_t val = std::accumulate(
      result.begin(), result.end(), 0, [](int a, pair_integers b) {
        return (a + (b.first * std::pow(2, b.second)));
      });

  return val;
}

inline bool replaceElementsInBuff(buffer_t buffIn, buffer_t buffOut,
                                  uint32_t position) {
  const auto iter = std::begin(*buffOut) + position;

  std::transform(iter, iter + buffIn->size(), buffIn->begin(), iter,
                 [](char a, char b) {
                   (void)a;
                   return b;
                 });

  return true;
}

/*
Taken from id3.org:
An ID3 tag is a data container within an MP3 audio file stored
 in a prescribed format. This data commonly contains the Artist name,
 Song title, Year and Genre of the current audio file
*/
uint32_t GetTagSize(buffer_t buffer,
                    const std::vector<unsigned int> &power_values,
                    uint32_t index) {

  using pair_integers = std::pair<uint32_t, uint32_t>;
  std::vector<pair_integers> result(power_values.size());

  const auto it = std::begin(*buffer) + index;

  std::transform(it, it + power_values.size(), power_values.begin(),
                 result.begin(),
                 [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

  const uint32_t val = std::accumulate(
      result.begin(), result.end(), 0, [](int a, pair_integers b) {
        return (a + (b.first * std::pow(2, b.second)));
      });

  return val;
}

template <typename T>
std::optional<buffer_t> updateAreaSize(buffer_t buffer, uint32_t extraSize,
                                       T tagIndex, T numberOfElements,
                                       T _maxValue, bool littleEndian = true) {
  const uint32_t TagIndex = tagIndex;
  const uint32_t NumberOfElements = numberOfElements;
  const uint32_t maxValue = _maxValue;
  auto itIn = std::begin(*buffer) + TagIndex;
  auto ExtraSize = extraSize;
  auto extr = ExtraSize % maxValue;

  buffer_t temp_vec = {};
  temp_vec->reserve(NumberOfElements);
  std::copy(itIn, itIn + NumberOfElements, temp_vec->begin());
  auto it = temp_vec->begin();

  if (littleEndian) {
    /* reverse order of elements */
    std::reverse(it, it + NumberOfElements);
  }

  std::transform(
      it, it + NumberOfElements, it, it, [&](uint32_t a, uint32_t b) {
        (void)b;

        extr = ExtraSize % maxValue;
        a = (a >= maxValue) ? maxValue : a;

        if (ExtraSize >= maxValue) {
          const auto rest = maxValue - a;
          a = a + rest;
          ExtraSize -= rest;
        } else {
          auto rest2 = maxValue - a;
          a = (a + ExtraSize > maxValue) ? maxValue : (a + ExtraSize);
          ExtraSize = ((int)(ExtraSize - rest2) < 0) ? 0 : (ExtraSize - rest2);
        }
        return a;
      });

  if (littleEndian) {
    /* reverse back order of elements */
    std::reverse(it, it + NumberOfElements);
  }

  id3::log()->trace("Tag Frame Bytes after update : {}",
                    spdlog::to_hex(std::begin(*temp_vec),
                                   std::begin(*temp_vec) + NumberOfElements));
  ID3_LOG_TRACE("success...");

  return temp_vec;
}

template <typename Type> class searchFrame {
private:
  const Type &mTagArea;

public:
  explicit searchFrame(const Type &tagArea) : mTagArea(tagArea) {}

  searchFrame(searchFrame const &) = delete;
  searchFrame &operator=(searchFrame const &) = delete;

  execution_status_t execute(const Type &tag) const {
    auto _execute = [this](const Type &TagArea, const Type &_tag) {
      uint32_t loc = 0;

      const auto it =
          std::search(TagArea.cbegin(), TagArea.cend(),
                      std::boyer_moore_searcher(_tag.cbegin(), _tag.cend()));

      if (it != TagArea.cend()) {
        loc = (it - TagArea.cbegin());

        return get_status_frame_ID_pos(loc);
      } else {

        const std::string ret = std::string("could not find frame ID: ") +
                                std::string(_tag) + std::string("\n");

        return get_status_no_frame_ID();
      }
    };

    return _execute(mTagArea, tag);
  }
};

}; // end namespace id3

#endif // ID3_BASE
