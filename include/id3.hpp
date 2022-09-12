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
using buffer_t = std::unique_ptr<std::vector<uint8_t>>;
using shared_string_t = std::shared_ptr<std::string>;

enum class severity_t { idle = 0, high, middle, low };
enum class tag_type_t { idle = 0, id3v1, id3v2, ape };

struct tag_presence_t {
  unsigned int id3v1_present : 1;
  unsigned int id3v2_present : 1;
  unsigned int ape_present : 1;
};

enum class rstatus_t {
  idle = 0,
  noError,
  fileOpeningError,
  fileRenamingError,
  noTag,
  noExtendedTagError,
  tagVersionError,
  noTagLengthError,                 // tag container does not exists
  tagSizeBiggerThanTagHeaderLength, // tag size > tag header length
  noFrameIDError,
  frameIDBadPositionError,
  no_framePayloadLength,
  frameLengthBiggerThanTagLength,
  ContentLengthBiggerThanFrameArea, // Content to write does not fit in
  PayloadStartAfterPayloadEnd
};

typedef struct execution_status {
  severity_t severity : 3;
  tag_type_t frame_type_val : 3;
  rstatus_t rstatus : 5;
  unsigned int frameIDPosition : 32;
} execution_status_t;

typedef struct mp3_execution_status {
  struct tag_presence_t frame_type_v;
  execution_status_t status;
} mp3_execution_status_t;

typedef struct frameContent_s {
  execution_status_t parseStatus;
  std::optional<std::string> payload;
} frameContent_t;

typedef struct _frameContent_v {
  execution_status_t parseStatus;
  std::string_view payload;
} frameContent_v;

typedef struct frameBuffer_s {
  execution_status_t parseStatus;
  buffer_t tagBuffer;
  uint32_t start;
  uint32_t end;
} frameBuffer_t;

const auto get_message_from_status(const rstatus_t &status) {
  switch (status) {
  case rstatus_t::noError:
    return std::string{"No Error"};
    break;
  case rstatus_t::noFrameIDError:
    return std::string{"Frame ID could not be retrieved"};
    break;
  case rstatus_t::frameIDBadPositionError:
    return std::string{"Bad Frame ID Position"};
    break;
  case rstatus_t::frameLengthBiggerThanTagLength:
    return std::string{"Frame length > Tag length"};
    break;
  case rstatus_t::PayloadStartAfterPayloadEnd:
    return std::string{"Payload Start Position > Payload End Position"};
    break;
  case rstatus_t::ContentLengthBiggerThanFrameArea:
    return std::string{
        "Content Length to write > Existing Frame Payload Length"};
    break;
  case rstatus_t::fileOpeningError:
    return std::string{"Error: Could not open file"};
    break;
  case rstatus_t::tagVersionError:
    return std::string{"Error: Wrong Tag Version"};
    break;
  default:
    break;
  }
  return std::string("");
}

constexpr auto noStatusErrorFrom(execution_status_t statusIn) {
  if (statusIn.rstatus == rstatus_t::noError) {
    return true;
  } else {
    return false;
  }
}

constexpr auto get_status_error(tag_type_t frameTypeIn, rstatus_t statusIn) {
  execution_status_t val = {};
  val.frame_type_val = frameTypeIn;
  val.rstatus = statusIn;
  return val;
}

constexpr auto get_execution_status(tag_type_t frameTypeIn,
                                    execution_status_t statusIn) {
  execution_status_t val = statusIn;
  val.frame_type_val = frameTypeIn;
  return val;
}

constexpr execution_status_t get_status_no_tag_exists(tag_type_t tagTypeIn) {
  execution_status_t val = {};
  val.frame_type_val = tagTypeIn;
  val.rstatus = rstatus_t::noTagLengthError;
  return val;
}

constexpr execution_status_t get_status_no_frame_ID(tag_type_t tagTypeIn) {
  execution_status_t val = {};
  val.frame_type_val = tagTypeIn;
  val.rstatus = rstatus_t::noFrameIDError;
  return val;
}

constexpr execution_status_t get_status_frame_ID_pos(unsigned int pos) {
  execution_status_t val = {};
  val.rstatus = rstatus_t::noError;
  val.frameIDPosition = pos;
  return val;
}

constexpr bool get_execution_status_b(execution_status_t valIn) {
  return (valIn.rstatus == rstatus_t::noError);
}

/* Frame settings */
typedef struct frameScopeProperties_t {
  uint32_t frameIDStartPosition = {};
  tag_type_t tagType;
  uint8_t doSwap = {};
  uint32_t frameContentStartPosition = {};
  uint32_t frameLength = {};
  uint32_t framePayloadLength = {};
  uint16_t encodeFlag = {};

  uint32_t getFramePayloadLength() const { return framePayloadLength; }

  uint32_t getFrameLength() const { return frameLength; }

  void with_additional_payload_size(uint32_t additionalPayload) {
    frameLength += additionalPayload;
    framePayloadLength += additionalPayload;
  }

} frameScopeProperties;

inline std::string &stripLeft(std::string &valIn) {
  valIn.erase(
      remove_if(valIn.begin(), valIn.end(), [](char c) { return !isprint(c); }),
      valIn.end());

  return valIn;
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

execution_status_t WriteFile(const std::string &FileName,
                             std::string_view content,
                             const frameScopeProperties &frameScopeProperties) {
  std::fstream filWrite(FileName,
                        std::ios::binary | std::ios::in | std::ios::out);

  if (!filWrite.is_open()) {
    return get_status_error(frameScopeProperties.tagType,
                            rstatus_t::fileOpeningError);
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

  return execution_status_t{};
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
uint32_t GetValFromBuffer(std::vector<uint8_t> buffer, T index,
                          T num_of_bytes_in_hex) {

  constexpr bool is_integrale_asset =
      std::is_integral<T>::value || std::is_unsigned<T>::value;
  static_assert(is_integrale_asset, "Parameter should be integer");

  ID3_PRECONDITION(buffer.size() > num_of_bytes_in_hex);

  auto version = 0;
  auto remaining = 0;
  auto bytes_to_add = num_of_bytes_in_hex;
  auto byte_to_pad = index;

  ID3_PRECONDITION(bytes_to_add >= 0);

  while (bytes_to_add > 0) {
    remaining = (num_of_bytes_in_hex - bytes_to_add);
    auto val = std::pow(2, 8 * remaining) * buffer.at(byte_to_pad);
    version += val;

    bytes_to_add--;
    byte_to_pad--;
  }

  return version;
}

inline std::string ExtractString(const std::vector<uint8_t> &buffer,
                                 uint32_t start, uint32_t length) {
  ID3_PRECONDITION((start + length) <= static_cast<uint32_t>(buffer.size()));

  if (static_cast<uint32_t>(buffer.size()) >= (start + length)) {

    return std::string{reinterpret_cast<const char *>(buffer.data() + start),
                       length};
  } else {

    ID3_LOG_ERROR("Buffer size {} < buffer length: {} ", buffer.size(), length);
    return {};
  }
}

uint32_t GetTagSizeDefaultO(const std::vector<unsigned char> &buffer,
                            uint32_t length, uint32_t startPosition = 0,
                            bool BigEndian = false) {

  ID3_PRECONDITION((startPosition + length) <=
                   static_cast<uint32_t>(buffer.size()));

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

  const auto it = std::begin(buffer) + startPosition;
  std::transform(it, it + length, power_values.begin(), result.begin(),
                 [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

  const uint32_t val = std::accumulate(
      result.begin(), result.end(), 0, [](int a, pair_integers b) {
        return (a + (b.first * std::pow(2, b.second)));
      });

  return val;
}

uint32_t GetTagSizeDefault(const std::vector<uint8_t> &buffer, uint32_t length,
                           uint32_t startPosition = 0, bool BigEndian = false) {

  ID3_PRECONDITION((startPosition + length) <=
                   static_cast<uint32_t>(buffer.size()));

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

  const auto it = std::begin(buffer) + startPosition;
  std::transform(it, it + length, power_values.begin(), result.begin(),
                 [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

  const uint32_t val = std::accumulate(
      result.begin(), result.end(), 0, [](int a, pair_integers b) {
        return (a + (b.first * std::pow(2, b.second)));
      });

  return val;
}

inline bool replaceElementsInBuff(const std::vector<uint8_t> &buffIn,
                                  std::vector<uint8_t> &buffOut,
                                  uint32_t position) {
  const auto iter = std::begin(buffIn);

  std::transform(iter, iter + buffIn.size(), buffOut.begin() + position,
                 [](char a) { return a; });

  return true;
}

/*
Taken from id3.org:
An ID3 tag is a data container within an MP3 audio file stored
 in a prescribed format. This data commonly contains the Artist name,
 Song title, Year and Genre of the current audio file
*/
uint32_t GetTagSize(const std::vector<uint8_t> &buffer,
                    const std::vector<unsigned int> &power_values,
                    uint32_t index) {

  using pair_integers = std::pair<uint32_t, uint32_t>;
  std::vector<pair_integers> result(power_values.size());

  const auto it = std::begin(buffer) + index;

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
std::optional<std::vector<uint8_t>>
updateAreaSize(const std::vector<uint8_t> &buffer, uint32_t extraSize,
               T tagIndex, T numberOfElements, T _maxValue,
               bool littleEndian = true) {
  const uint32_t TagIndex = tagIndex;
  const uint32_t NumberOfElements = numberOfElements;
  const uint32_t maxValue = _maxValue;
  auto itIn = std::begin(buffer) + TagIndex;
  auto ExtraSize = extraSize;
  auto extr = ExtraSize % maxValue;

  std::vector<uint8_t> temp_vec = {};
  temp_vec.resize(NumberOfElements);
  std::copy(itIn, itIn + NumberOfElements, temp_vec.begin());
  auto it = temp_vec.begin();

  if (littleEndian) {
    /* reverse order of elements */
    std::reverse(it, it + NumberOfElements);
  }

  std::transform(it, it + NumberOfElements, it, [&](uint32_t a) {
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

        return get_status_no_frame_ID(tag_type_t::idle);
      }
    };

    return _execute(mTagArea, tag);
  }
};

}; // end namespace id3

#endif // ID3_BASE
