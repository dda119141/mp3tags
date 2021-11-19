// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3V2_BASE
#define ID3V2_BASE

#include <sstream>
#include <fstream>
#include <variant>

#include "id3.hpp"
#include <id3v2_v30.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v00.hpp>

#include "logger.hpp"


namespace id3v2 {

using iD3Variant = std::variant <::id3v2::v00, ::id3v2::v30, ::id3v2::v40>;

using namespace id3;

class basicParameters {
private:
	const std::string& filename;
	std::string_view frameID = {};
	const iD3Variant tagVersion;

	buffer_t tagBuffer = {};
	FrameSettings frameSettings = {};
	std::string tagArea = {};
	std::string_view framePayload = {};

	basicParameters() = default;

public:
	explicit basicParameters(const std::string& Filename,
		const iD3Variant& TagVersion
		, std::string_view FrameID) :
		filename(Filename)
		, tagVersion(TagVersion)
		, frameID(FrameID)
	{
	}

	explicit basicParameters(const std::string& Filename,
		const iD3Variant& TagVersion
		, std::string_view FrameID
		, std::string_view content) :
		filename{ Filename }
		, tagVersion{ TagVersion }
		, frameID{ FrameID }
		, framePayload{ content }
	{
	}

	explicit basicParameters(const std::string& Filename,
		const iD3Variant& TagVersion) :
		tagVersion{ TagVersion }
		, filename{ Filename }
	{
	}

	explicit basicParameters(const std::string& Filename)
	
		: filename{ Filename }
	{
	}

	const iD3Variant& get_tag_version() const
	{
		return tagVersion;
	}

	const std::string_view& get_frame_id() const
	{
		return frameID;
	}

	const std::string& get_filename() const
	{
		return filename;
	}

	basicParameters& with_frame_id(std::string_view FrameID)
	{
		frameID = FrameID;

		return *this;
	}

	basicParameters& with_frame_payload(std::string_view FramePayload)
	{
		framePayload = FramePayload;

		return *this;
	}

	const FrameSettings& get_frame_settings() const
	{
		return frameSettings;
	}

	std::string get_tag_area() const
	{
		return this->tagArea;
	}

	buffer_t get_tag_Buffer() const
	{
		return this->tagBuffer;
	}

	std::string_view get_frame_content_to_write() const
	{
		return this->framePayload;
	}

	basicParameters with_frame_settings(const FrameSettings& FrameSettings) const &
	{
		basicParameters obj(*this);

		obj.frameSettings = FrameSettings;

		return obj;
	}
	
	basicParameters with_tag_area(const std::string& TagArea) &&
	{
		basicParameters obj(std::move(*this));

		obj.tagArea = TagArea;

		return obj;
	}

	basicParameters with_tag_buffer(buffer_t TagBuffer)	&&
	{
		basicParameters obj(std::move(*this));

		obj.tagBuffer = TagBuffer;

		return obj;
	}
};

template <typename T>
buffer_t fillTagBufferWithPayload(T content, id3::buffer_t tagBuffer, const FrameSettings& frameConfig) {

	assert(frameConfig.getFramePayloadLength() >= content.size());

	const auto PositionFramePayloadStart = frameConfig.getFramePayloadOffset();

	id3::log()->info(" PositionTagStart: {}", PositionFramePayloadStart);

	const auto payload_start_index = std::begin(*tagBuffer) + PositionFramePayloadStart;

	std::transform(payload_start_index, payload_start_index + content.size(), content.begin(), payload_start_index,
		[](char a, char b) { return b; });

	std::transform(payload_start_index + content.size(), payload_start_index + frameConfig.getFramePayloadLength(),
		content.begin(), payload_start_index + content.size(),
		[](char a, char b) { return 0x00; });

	return tagBuffer;
}

template <typename T>
std::optional<T> GetFrameSize(buffer_t buff,
	const iD3Variant& TagVersion,
	uint32_t framePos) {
	return std::visit(
		overloaded{ [&](id3v2::v30 arg) {
					   return arg.GetFrameSize(buff, framePos);
				   },
				   [&](id3v2::v40 arg) {
					   return arg.GetFrameSize(buff, framePos);
				   },
				   [&](id3v2::v00 arg) {
					   return arg.GetFrameSize(buff, framePos);
				   } },
		TagVersion);
}

uint32_t GetFrameHeaderSize(const iD3Variant& TagVersion) {
	return std::visit(
		overloaded{ [&](id3v2::v30 arg) { return arg.FrameHeaderSize(); },
				   [&](id3v2::v40 arg) { return arg.FrameHeaderSize(); },
				   [&](id3v2::v00 arg) { return arg.FrameHeaderSize(); } },
		TagVersion);
}

std::optional<buffer_t> UpdateFrameSize(buffer_t buffer,
	uint32_t extraSize,
	uint32_t frameIDPosition) {

	const uint32_t frameSizePositionInArea = frameIDPosition + 4;
	constexpr uint32_t frameSizeLengthInArea = 4;
	constexpr uint32_t frameSizeMaxValuePerElement = 127;

	return updateAreaSize<uint32_t>(
		buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
		frameSizeMaxValuePerElement);
}

template <typename... Args>
std::optional<buffer_t> updateFrameSizeIndex(const iD3Variant& TagVersion,
	Args... args)
{
	return std::visit(
		overloaded{
			[&](id3v2::v30 arg) { return UpdateFrameSize(args...); },
			[&](id3v2::v40 arg) { return UpdateFrameSize(args...); },
			[&](id3v2::v00 arg) { return arg.UpdateFrameSize(args...); } },
			TagVersion);
}


const auto CreateTagBufferFromFile(const std::string& FileName, uint32_t num) {
    std::ifstream fil(FileName);
    auto buffer = std::make_shared<std::vector<unsigned char>>(num, '0');

//    if (!fil.good()) {
//        return expected::makeError<std::vector<uint8_t>>() << __func__
//                                                    << (":failed\n");
//    }

    fil.read(reinterpret_cast<char*>(buffer->data()), num);

    return buffer;
}

template <typename T>
std::optional<std::string> GetHexFromBuffer(id3::buffer_t buffer, T index,
                                               T num_of_bytes_in_hex) {
    id3::integral_unsigned_asserts<T> eval;
    eval();

    assert(buffer->size() > num_of_bytes_in_hex);

    std::stringstream stream_obj;

    stream_obj << "0x" << std::setfill('0')
               << std::setw(num_of_bytes_in_hex * 2);

    const uint32_t version =
        id3::GetValFromBuffer<T>(buffer, index, num_of_bytes_in_hex);

    if (version != 0) {
        stream_obj << std::hex << version;

        return stream_obj.str();

    } else {
        return {};
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

std::optional<buffer_t> updateTagSize(buffer_t buffer,
                                       uint32_t extraSize) {
    constexpr uint32_t tagSizePositionInHeader = 6;
    constexpr uint32_t tagSizeLengthInHeader = 4;
    constexpr uint32_t tagSizeMaxValuePerElement = 127;

    return updateAreaSize<uint32_t>(buffer, extraSize, tagSizePositionInHeader,
                                    tagSizeLengthInHeader,
                                    tagSizeMaxValuePerElement);
}

std::optional<uint32_t> GetTagSize(buffer_t buffer) {

    const std::vector<uint32_t> pow_val = {21, 14, 7, 0};
    constexpr uint32_t TagIndex = 6;

    const auto val = id3::GetTagSize(buffer, pow_val, TagIndex);

    assert(val > GetTagHeaderSize<uint32_t>());

    return val;

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

const auto GetTotalTagSize(buffer_t buffer) {
    return GetTagSize(buffer) | [=](const uint32_t Tagsize) {

        return Tagsize + GetTagHeaderSize<uint32_t>();
    };
}

const auto GetTagSizeExclusiveHeader(buffer_t buffer) {
    return GetTagSize(buffer) | [](const int tag_size) {

        return (tag_size - GetTagHeaderSize<uint32_t>());
    };
}

std::optional<buffer_t> GetTagHeader(const std::string& FileName) {
    auto val =
        CreateTagBufferFromFile(FileName, GetTagHeaderSize<uint32_t>());

    return val;
}

std::string GetTagArea(buffer_t buffer) {

    const auto totalSize = GetTotalTagSize(buffer);

    return ExtractString(buffer, 0, totalSize);
}

template <typename T>
std::optional<std::vector<uint8_t>> processFrameHeaderFlags(
    buffer_t buffer, uint32_t frameHeaderFlagsPosition,
    std::string_view tag) {
    constexpr uint32_t frameHeaderFlagsLength = 2;

    const auto it = buffer->begin() + frameHeaderFlagsPosition;

    std::vector<uint8_t> temp_vec(frameHeaderFlagsLength);
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

    return temp_vec;
}

template <typename id3Type>
void GetTagNames(void) {
    return id3Type::tag_names;
};

std::string GetID3FileIdentifier(buffer_t buffer) {
    constexpr auto FileIdentifierStart = 0;
    constexpr auto FileIdentifierLength = 3;

    return ExtractString(buffer, FileIdentifierStart,
                                             FileIdentifierLength);

#ifdef __TEST_CODE
    auto y = [&buffer]() -> std::string {
        return Get_ID3_header_element(buffer, 0, 3);
    };
    return y();
#endif
}

std::optional<std::string> GetID3Version(id3::buffer_t buffer) {
    constexpr auto kID3IndexStart = 4;
    constexpr auto kID3VersionBytesLength = 2;

    return GetHexFromBuffer<uint8_t>(buffer, kID3IndexStart,
                                     kID3VersionBytesLength);
}

};  // end namespace id3v2

#endif //ID3V2_BASE
