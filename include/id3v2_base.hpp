// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3V2_BASE
#define ID3V2_BASE

#include <fstream>
#include <sstream>
#include <variant>

#include "id3.hpp"
#include <id3v2_v00.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v40.hpp>

#include "logger.hpp"

namespace id3v2
{

enum class tagVersion_t { idle = 0, v00, v30, v40, NrOfTypes_t };
using frame_t = std::vector<std::pair<tagVersion_t, std::string_view>>;

constexpr auto getIndex(tagVersion_t val)
{
  switch (val) {
  case tagVersion_t::idle:
    return -1;
    break;

  case tagVersion_t::v00:
    return 0;
    break;

  case tagVersion_t::v30:
    return 1;
    break;

  case tagVersion_t::v40:
    return 2;
    break;
  case tagVersion_t::NrOfTypes_t:
    return 3;
    break;
  }
  return -1;
}

using frameArray_t =
    std::array<std::string, getIndex(tagVersion_t::NrOfTypes_t)>;

template <typename T>
class frameID_s
{
public:
  explicit frameID_s(T frames)
  {
    for (auto frame : frames) {
      mFrames.at(getIndex(frame.first)) = frame.second;
    }
  }
  frameArray_t mFrames;
};

} // namespace id3v2

namespace id3
{
using iD3Variant = std::variant<::id3v2::v00, ::id3v2::v30, ::id3v2::v40>;

constexpr auto v2FrameIndex(iD3Variant valIn)
{
  return std::visit(overloaded{
                        [](id3v2::v00 arg) {
                          return id3v2::getIndex(id3v2::tagVersion_t::v00);
                        },
                        [](id3v2::v30 arg) {
                          return id3v2::getIndex(id3v2::tagVersion_t::v30);
                        },
                        [](id3v2::v40 arg) {
                          return id3v2::getIndex(id3v2::tagVersion_t::v40);
                        },
                    },
                    valIn);
}

constexpr auto v2FrameID(const iD3Variant &valIn, const meta_entry &entry)
{
  return std::visit(
      overloaded{
          [&entry](id3v2::v00 arg) { return arg.get_frame(entry); },
          [&entry](id3v2::v30 arg) { return arg.get_frame(entry); },
          [&entry](id3v2::v40 arg) { return arg.get_frame(entry); },
      },
      valIn);
}

class fileScopeProperties
{
private:
  const std::string filename;
  const iD3Variant tagVersion;
  std::string_view frameID = {};
  std::string_view framePayload = {};

public:
  explicit fileScopeProperties(const std::string &Filename,
                               const iD3Variant &TagVersion,
                               std::string_view FrameID)
      : filename(Filename), tagVersion(TagVersion), frameID(FrameID)
  {
  }

  explicit fileScopeProperties(const std::string &Filename,
                               const iD3Variant &TagVersion,
                               std::string_view FrameID,
                               std::string_view content)
      : filename{Filename}, tagVersion{TagVersion}, frameID{FrameID},
        framePayload{content}
  {
  }

  explicit fileScopeProperties(const std::string &Filename,
                               const iD3Variant &TagVersion)
      : filename{Filename}, tagVersion{TagVersion}
  {
  }

  fileScopeProperties() = default;
  fileScopeProperties(const fileScopeProperties &) = default;
  fileScopeProperties &operator=(const fileScopeProperties &) = default;
  fileScopeProperties(fileScopeProperties &&) = default;
  ~fileScopeProperties() = default;

  const iD3Variant &get_tag_version() const { return tagVersion; }

  const auto get_tag_type() const { return tagVersion; }

  std::string_view get_frame_id() const { return frameID; }

  const std::string &get_filename() const { return filename; }

  fileScopeProperties &with_frame_id(std::string_view FrameID)
  {
    frameID = FrameID;

    return *this;
  }

  std::string_view get_frame_content_to_write() const
  {
    return this->framePayload;
  }
};

class AudioSettings_t
{
public:
  AudioSettings_t() = default;
  fileScopeProperties fileScopePropertiesObj;
  std::optional<buffer_t> TagBuffer;
  std::optional<std::string> TagArea;
  std::optional<frameScopeProperties> frameScopePropertiesObj;
};

typedef struct AudioSettings_t audioProperties_t;

} // namespace id3

namespace id3v2
{

using namespace id3;
constexpr int TagHeaderStartPosition = 0;
constexpr int TagHeaderSize = 10;
constexpr int FrameHeaderSize = 10;
constexpr uint32_t frameSizePositionInFrameHeader = TagHeaderStartPosition + 4;
constexpr uint32_t tagSizePositionInHeader = TagHeaderStartPosition + 6;
constexpr uint32_t tagSizeLengthPositionInHeader = TagHeaderStartPosition + 4;
constexpr uint32_t tagSizeMaxValuePerElement = 127;

void CheckAudioPropertiesObject(
    const audioProperties_t *const audioPropertiesObj)
{
  if (audioPropertiesObj == nullptr) {
    ID3V2_THROW("frame properties object does not exists\n");
  }

  if (!audioPropertiesObj->frameScopePropertiesObj.has_value()) {
    ID3V2_THROW("frame properties not yet parsed from audio file\n");
  }
}

template <typename T>
void fillTagBufferWithPayload(T content, std::vector<char> &tagBuffer,
                              const frameScopeProperties &frameConfig,
                              uint32_t additionalSize = 0)
{

  const auto framePayloadStartPosition = frameConfig.frameContentStartPosition;

  ID3_PRECONDITION_MSG(
      tagBuffer.size() >= content.size() + framePayloadStartPosition,
      "Existing Frame Payload index + content size > buffer size");

  id3::log()->info(" PositionTagStart: {}", framePayloadStartPosition);

  const auto payload_start_index =
      std::begin(tagBuffer) + framePayloadStartPosition;

  const auto payload_end_index =
      std::begin(tagBuffer) + framePayloadStartPosition + content.size();

  std::transform(content.begin(), content.end(), payload_start_index,
                 [](char a) { return a; });

  std::transform(payload_end_index, payload_end_index + additionalSize,
                 payload_end_index, // location to write
                 [](char a) { return 0x00; });
}

template <typename T>
std::optional<T> GetFrameSize(const std::vector<char> &buff,
                              const iD3Variant &TagVersion, uint32_t framePos)
{
  return std::visit(
      overloaded{
          [&](id3v2::v30 arg) { return arg.GetFrameSize(buff, framePos); },
          [&](id3v2::v40 arg) { return arg.GetFrameSize(buff, framePos); },
          [&](id3v2::v00 arg) { return arg.GetFrameSize(buff, framePos); }},
      TagVersion);
}

uint32_t GetFrameHeaderSize(const iD3Variant &TagVersion)
{
  return std::visit(
      overloaded{[&](id3v2::v30 arg) { return arg.FrameHeaderSize(); },
                 [&](id3v2::v40 arg) { return arg.FrameHeaderSize(); },
                 [&](id3v2::v00 arg) { return arg.FrameHeaderSize(); }},
      TagVersion);
}

auto UpdateFrameSize(const std::vector<char> &buffer, uint32_t extraSize,
                     uint32_t frameIDPosition)
{

  const uint32_t frameSizePositionInArea = frameIDPosition + 4;
  constexpr uint32_t frameSizeLengthInArea = 4;
  constexpr uint32_t frameSizeMaxValuePerElement = 127;

  return updateAreaSize<uint32_t>(buffer, extraSize, frameSizePositionInArea,
                                  frameSizeLengthInArea,
                                  frameSizeMaxValuePerElement);
}

template <typename... Args>
std::optional<std::vector<char>>
updateFrameSizeIndex(const iD3Variant &TagVersion, Args... args)
{
  return std::visit(
      overloaded{[&](id3v2::v30 arg) {
                   (void)arg;
                   return UpdateFrameSize(args...);
                 },
                 [&](id3v2::v40 arg) {
                   (void)arg;
                   return UpdateFrameSize(args...);
                 },
                 [&](id3v2::v00 arg) { return arg.UpdateFrameSize(args...); }},
      TagVersion);
}

void GetTagBufferFromFile(const std::string &FileName, size_t num,
                          buffer_t &buffer)
{
  std::ifstream fil(FileName);
  constexpr auto tagBeginPosition = 0;

  const bool buffer_to_reset = ((buffer && buffer->size() < num) || !buffer);

  if (buffer_to_reset) {
    buffer.reset(new std::vector<char>(num));
  }
  fil.read(reinterpret_cast<char *>(&buffer->at(tagBeginPosition)), num);
}

template <typename T>
std::optional<std::string> GetHexFromBuffer(const std::vector<char> &buffer,
                                            T index, T num_of_bytes_in_hex)
{
  constexpr bool is_integrale_asset =
      std::is_integral<T>::value || std::is_unsigned<T>::value;
  static_assert(is_integrale_asset, "Parameter should be integer");

  ID3_PRECONDITION(buffer.size() >
                   static_cast<std::size_t>(num_of_bytes_in_hex));

  std::stringstream stream_obj;
  stream_obj << "0x" << std::setfill('0') << std::setw(num_of_bytes_in_hex * 2);

  const uint32_t version =
      id3::getValFromBuffer<T>(buffer, index, num_of_bytes_in_hex);
  if (version != 0) {
    stream_obj << std::hex << version;
    return stream_obj.str();
  } else {
    return std::nullopt;
  }
}

template <typename T>
std::string u8_to_u16_string(T val)
{
  return std::accumulate(
      val.begin() + 1, val.end(), std::string(val.substr(0, 1)),
      [](std::string arg1, char arg2) { return arg1 + '\0' + arg2; });
}

auto updateTagSize(const std::vector<char> &buffer, uint32_t extraSize)
{
  return updateAreaSize<uint32_t>(buffer, extraSize, tagSizePositionInHeader,
                                  tagSizeLengthPositionInHeader,
                                  tagSizeMaxValuePerElement);
}

const uint32_t GetTagSizeWithoutHeader(const std::vector<char> &buffer)
{
  const std::vector<uint32_t> pow_val = {21, 14, 7, 0};
  constexpr uint32_t TagIndex = 6;

  const auto val = id3::GetTagSize(buffer, pow_val, TagIndex);

  return val;
}

const auto GetTotalTagSize(const std::vector<char> &buffer)
{
  const auto val = GetTagSizeWithoutHeader(buffer);
  return val + TagHeaderSize;
}

buffer_t &GetTagHeader(const std::string &FileName, buffer_t &buffer)
{
  std::ifstream fil(FileName);
  const bool buffer_to_reset =
      ((buffer && buffer->size() < TagHeaderSize) || !buffer);

  if (buffer_to_reset) {
    buffer.reset(new std::vector<char>(TagHeaderSize));
  }
  fil.read(reinterpret_cast<char *>(&buffer->at(0)), TagHeaderSize);
  return buffer;
}

std::string GetStringFromBuffer(const std::vector<char> &buffer)
{
  return extractString(buffer, 0, buffer.size());
}

const auto
RetrieveFrameModificationRights(const std::vector<char> &HeaderBuffer,
                                std::size_t frameStartPosition)
{
  if (HeaderBuffer.size() < FrameHeaderSize) {
    ID3V2_THROW("frame properties object does not exists\n");
  }
  constexpr uint32_t frameHeaderFlagsPosition = 8;
  const auto it =
      HeaderBuffer.begin() + frameStartPosition + frameHeaderFlagsPosition;
  const std::bitset<8> frameStatusFlag{static_cast<unsigned long long>(*it)};
  const std::bitset<8> frameDescriptionFlag{
      static_cast<unsigned long long>(*(it + 1))};

  if (frameStatusFlag[1]) {
    return frameRights_t::FrameShouldBeDiscarded;
  }
  if (frameStatusFlag[2]) {
    return frameRights_t::FrameShouldBeDiscarded;
  }
  if (frameStatusFlag[3]) {
    return frameRights_t::FrameIsReadOnly;
  }
  return frameRights_t{};
}

constexpr auto FrameIsReadOnly(frameRights_t ValIn)
{
  return (ValIn == frameRights_t::FrameIsReadOnly);
}

void IfFrameIsReadOnlyStop(const std::vector<char> &HeaderBuffer,
                           std::size_t frameStartPosition,
                           std::string_view FrameID)
{
  if (FrameIsReadOnly(
          RetrieveFrameModificationRights(HeaderBuffer, frameStartPosition))) {
    const auto val = std::string{FrameID} + std::string{": Read-Only Frame\n"};
    ID3V2_THROW(val.c_str())
  }
}

const auto RetrieveFrameAttributed(const std::vector<char> &HeaderBuffer)
{
  if (HeaderBuffer.size() < FrameHeaderSize) {
    ID3V2_THROW("frame properties object does not exists\n");
  }
  constexpr uint32_t frameHeaderAttributesPosition = 9;
  const auto it = HeaderBuffer.begin() + frameHeaderAttributesPosition;
  const std::bitset<8> frameAttributeFlag{static_cast<unsigned long long>(*it)};

  uint8_t frameAttributes = 0;

  if (frameAttributeFlag[1]) {
    frameAttributes =
        static_cast<uint8_t>(frameAttributes_t::FrameIsCompressed);
  }
  if (frameAttributeFlag[2]) {
    frameAttributes +=
        static_cast<uint8_t>(frameAttributes_t::FrameIsEncrypted);
  }
  if (frameAttributeFlag[3]) {
    frameAttributes +=
        static_cast<uint8_t>(frameAttributes_t::FrameContainsGroupInformation);
  }
  return frameAttributes;
}

const auto FrameIsCompressed(uint8_t attribute)
{
  const auto val = static_cast<std::bitset<8>>(attribute);
  return (val[1] == 1);
}

const auto FrameIsEncrypted(uint8_t attribute)
{
  const auto val = static_cast<std::bitset<8>>(attribute);
  return (val[2] == 1);
}

const auto FrameContainsGroupInformation(uint8_t attribute)
{
  const auto val = static_cast<std::bitset<8>>(attribute);
  return (val[3] == 1);
}

}; // end namespace id3v2

#endif // ID3V2_BASE
