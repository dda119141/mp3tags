// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3_BASE
#define ID3_BASE

#include "id3_exceptions.hpp"
#include "logger.hpp"
#include "metaEntry.hpp"
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

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace id3
{

#if defined(HAS_FS_EXPERIMENTAL)
namespace filesystem = std::experimental::filesystem;
#elif defined(_MSVC_LANG)
namespace filesystem = std::filesystem;
#elif defined(HAS_FS)
namespace filesystem = std::filesystem;
#endif

namespace v30_v40
{
const auto get_frame(const meta_entry &entry)
{
  switch (entry) {
  case meta_entry::album:
    return std::string_view{"TALB"};
    break;
  case meta_entry::artist:
    return std::string_view{"TPE1"};
    break;
  case meta_entry::composer:
    return std::string_view{"TCOM"};
    break;
  case meta_entry::title:
    return std::string_view{"TIT2"};
    break;
  case meta_entry::date:
    return std::string_view{"TDAT"};
    break;
  case meta_entry::genre:
    return std::string_view{"TCON"};
    break;
  case meta_entry::textwriter:
    return std::string_view{"TEXT"};
    break;
  case meta_entry::audioencryption:
    return std::string_view{"AENC"};
    break;
  case meta_entry::language:
    return std::string_view{"TLAN"};
    break;
  case meta_entry::time:
    return std::string_view{"TIME"};
    break;
  case meta_entry::year:
    return std::string_view{"TYER"};
    break;
  case meta_entry::originalfilename:
    return std::string_view{"TOFN"};
    break;
  case meta_entry::filetype:
    return std::string_view{"TFLT"};
    break;
  case meta_entry::bandOrchestra:
    return std::string_view{"TPE2"};
    break;
  default:
    return std::string_view{};
    break;
  }
}
}; // namespace v30_v40

static const std::string modifiedEnding(".mod");
using buffer_t = std::unique_ptr<std::vector<char>>;
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
  PayloadStartAfterPayloadEnd,
  ErrorNoPrintableContent
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

typedef struct _Content_view {
  execution_status_t parseStatus;
  std::string_view payload;
} ContentView_t;

typedef struct _audio_content_view {
  meta_entry metaEntry;
  frameContent_t frameStatus;
} audio_contentView_t;

typedef struct frameBuffer_s {
  execution_status_t parseStatus;
  buffer_t tagBuffer;
  uint32_t start;
  uint32_t end;
} frameBuffer_t;

const auto get_message_from_tag_type(const tag_type_t &status)
{
  switch (status) {
  case tag_type_t::ape:
    return std::string_view{"Ape: "};
    break;
  case tag_type_t::id3v1:
    return std::string_view{"Id3v1: "};
    break;
  case tag_type_t::id3v2:
    return std::string_view{"Id3v2: "};
    break;
  default:
    return std::string_view{};
  }
}

const auto get_message_from_status(const rstatus_t &status)
{
  switch (status) {
  case rstatus_t::idle:
    return std::string{"Idle\n"};
    break;
  case rstatus_t::noError:
    return std::string{"No Error\n"};
    break;
  case rstatus_t::noFrameIDError:
    return std::string{"Error: Frame ID could not be retrieved\n"};
    break;
  case rstatus_t::frameIDBadPositionError:
    return std::string{"Error: Bad Frame ID Position\n"};
    break;
  case rstatus_t::frameLengthBiggerThanTagLength:
    return std::string{"Error: Frame length > Tag length\n"};
    break;
  case rstatus_t::PayloadStartAfterPayloadEnd:
    return std::string{
        "Error: Payload Start Position > Payload End Position\n"};
    break;
  case rstatus_t::ContentLengthBiggerThanFrameArea:
    return std::string{
        "Error: Content Length to write > Existing Frame Payload Length\n"};
    break;
  case rstatus_t::fileOpeningError:
    return std::string{"Error: Could not open file\n"};
    break;
  case rstatus_t::tagVersionError:
    return std::string{"Error: Wrong Tag Version\n"};
    break;
  case rstatus_t::noTagLengthError:
    return std::string{"Error: Tag length = 0\n"};
    break;
  case rstatus_t::noTag:
    return std::string{"Tag does not exist\n"};
    break;
  case rstatus_t::ErrorNoPrintableContent:
    return std::string{"Frame content not an a printable character\n"};
    break;
  case rstatus_t::no_framePayloadLength:
    return std::string{"No frame payload length\n"};
    break;
  case rstatus_t::tagSizeBiggerThanTagHeaderLength:
    return std::string{"tag Size bigger than tag header length\n"};
    break;
  default:
    return std::string{"General Error\n"};
    break;
  }
  return std::string("");
}

constexpr auto noStatusErrorFrom(execution_status_t statusIn)
{
  if (statusIn.rstatus == rstatus_t::noError) {
    return true;
  } else {
    return false;
  }
}
constexpr auto statusErrorFrom(execution_status_t statusIn)
{
  return !noStatusErrorFrom(statusIn);
}

constexpr auto statusIsIdleFrom(execution_status_t statusIn)
{
  if (statusIn.rstatus == rstatus_t::idle) {
    return true;
  } else {
    return false;
  }
}

constexpr auto get_status_error(tag_type_t frameTypeIn, rstatus_t statusIn)
{
  execution_status_t val = {};
  val.frame_type_val = frameTypeIn;
  val.rstatus = statusIn;
  return val;
}

constexpr auto get_execution_status(tag_type_t frameTypeIn,
                                    execution_status_t statusIn)
{
  execution_status_t val = statusIn;
  val.frame_type_val = frameTypeIn;
  return val;
}

constexpr execution_status_t get_status_no_tag_exists(tag_type_t tagTypeIn)
{
  execution_status_t val = {};
  val.frame_type_val = tagTypeIn;
  val.rstatus = rstatus_t::noTagLengthError;
  return val;
}

constexpr execution_status_t get_status_no_frame_ID(tag_type_t tagTypeIn)
{
  execution_status_t val = {};
  val.frame_type_val = tagTypeIn;
  val.rstatus = rstatus_t::noFrameIDError;
  return val;
}

constexpr execution_status_t get_status_frame_ID_pos(unsigned int pos)
{
  execution_status_t val = {};
  val.rstatus = rstatus_t::noError;
  val.frameIDPosition = pos;
  return val;
}

constexpr bool get_execution_status_b(execution_status_t valIn)
{
  return (valIn.rstatus == rstatus_t::noError);
}

enum class frameRights_t {
  FrameShouldBePreserved = 0,
  FrameShouldBeDiscarded,
  FrameIsReadOnly
};

enum class frameAttributes_t {
  FrameIsNotCompressed = 0,
  FrameIsCompressed = 1,
  FrameIsEncrypted = 2,
  FrameContainsGroupInformation = 4
};

/* Frame settings */
typedef struct frameScopeProperties_t {
  frameRights_t frameRights : 3;
  frameAttributes_t frameAttributes : 3;
  uint32_t frameIDStartPosition = {};
  tag_type_t tagType;
  uint8_t doSwap = {};
  uint8_t encodeFlag = {};
  uint32_t frameContentStartPosition = {};
  uint32_t frameLength = {};
  uint32_t framePayloadLength = {};

  uint32_t getFramePayloadLength() const
  {
    if (framePayloadLength)
      return framePayloadLength;
    else if (frameLength)
      return frameLength;
    else
      return framePayloadLength;
  }

  uint32_t getFrameLength() const
  {
    if (frameLength)
      return frameLength;
    else if (framePayloadLength)
      return framePayloadLength;
    else
      return frameLength;
  }

  void with_additional_payload_size(uint32_t additionalPayload)
  {
    frameLength += additionalPayload;
    framePayloadLength += additionalPayload;
  }

} frameScopeProperties;

inline std::string &stripLeft(std::string &valIn)
{
  valIn.erase(
      remove_if(valIn.begin(), valIn.end(), [](char c) { return !isprint(c); }),
      valIn.end());

  return valIn;
}

template <typename T>
constexpr T RetrieveSize(T n)
{
  return n;
}

template <typename T, typename F>
auto mbind(std::optional<T> &&_obj, F &&function)
    -> decltype(function(_obj.value()))
{
  auto fuc = std::forward<F>(function);
  auto obj = std::forward<std::optional<T>>(_obj);

  if (obj.has_value()) {
    return fuc(obj.value());
  } else {
    return {};
  }
}

template <typename T>
struct is_composible_t {
  static constexpr bool value{
      std::is_same<T, uint32_t>::value || std::is_same<T, bool>::value ||
      std::is_same<T, shared_string_t>::value ||
      std::is_same<T, std::string_view>::value ||
      std::is_same<T, buffer_t>::value ||
      std::is_same<std::decay_t<T>, std::string>::value ||
      std::is_same<std::decay_t<T>, id3::frameScopeProperties>::value};
};

template <typename T, typename = void>
constexpr bool is_optional_t{};

template <typename T>
constexpr bool is_optional_t<std::optional<T>,
                             std::void_t<decltype(is_composible_t<T>::value)>> =
    true;

template <typename T, typename = std::enable_if_t<is_optional_t<T>>, typename F>
auto operator|(T &&_obj, F &&Function)
    -> decltype(std::forward<F>(Function)(std::forward<T>(_obj).value()))
{
  auto fuc = std::forward<F>(Function);
  auto obj = std::forward<T>(_obj);

  if (obj.has_value()) {
    return fuc(obj.value());
  } else {
    return {};
  }
}

execution_status_t writeFile(const std::string &FileName,
                             std::string_view content,
                             const frameScopeProperties &frameScopeProperties)
{
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

  return get_status_error(frameScopeProperties.tagType, rstatus_t::noError);
}

std::optional<bool> renameFile(const std::string &fileToRename,
                               const std::string &renamedFile)
{
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
uint32_t getValFromBuffer(std::vector<char> buffer, T index,
                          std::size_t num_of_bytes_in_hex)
{
  static constexpr bool is_integrale_asset =
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

inline std::string extractString(const std::vector<char> &buffer,
                                 uint32_t start, uint32_t length)
{
  ID3_PRECONDITION((start + length) <= static_cast<uint32_t>(buffer.size()));

  if (static_cast<uint32_t>(buffer.size()) >= (start + length)) {

    return std::string{reinterpret_cast<const char *>(buffer.data() + start),
                       length};
  } else {

    ID3_LOG_ERROR("Buffer size {} < buffer length: {} ", buffer.size(), length);
    return {};
  }
}

uint32_t GetTagSizeDefault(const std::vector<char> &buffer, uint32_t length,
                           uint32_t startPosition = 0, bool BigEndian = false)
{
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

inline bool replaceElementsInBuff(const std::vector<char> &buffIn,
                                  std::vector<char> &buffOut, uint32_t position)
{
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
uint32_t GetTagSize(const std::vector<char> &buffer,
                    const std::vector<unsigned int> &power_values,
                    uint32_t index)
{
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
std::optional<std::vector<char>>
updateAreaSize(const std::vector<char> &buffer, uint32_t extraSize, T tagIndex,
               T numberOfElements, T _maxValue, bool littleEndian = true)
{
  const uint32_t TagIndex = tagIndex;
  const uint32_t NumberOfElements = numberOfElements;
  const uint32_t maxValue = _maxValue;
  auto itIn = std::begin(buffer) + TagIndex;
  auto ExtraSize = extraSize;
  auto extr = ExtraSize % maxValue;

  std::vector<char> temp_vec = {};
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

template <typename Type>
class searchFrame
{
private:
  const Type &mTagArea;

public:
  explicit searchFrame(const Type &tagArea) : mTagArea(tagArea) {}

  searchFrame(searchFrame const &) = delete;
  searchFrame &operator=(searchFrame const &) = delete;

  execution_status_t execute(const Type &tag) const
  {
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

}; // namespace id3

#endif // ID3_BASE
