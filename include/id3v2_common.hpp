// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _ID3V2_COMMON
#define _ID3V2_COMMON

#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <cmath>
#include <variant>
#include <mutex>
#include <type_traits>
#include <id3v2_base.hpp>
#include <extendTagArea.hpp>
#include <string_conversion.hpp>

#define DEBUG


namespace id3v2
{
    std::optional<buffer_t> checkForID3(buffer_t buffer) 
    {
		const auto ID3_Identifier = GetID3FileIdentifier(buffer);

        if (ID3_Identifier != std::string("ID3")) {
            id3::log()->info(" ID3 tag not present");
            return {};
        } else {
            return buffer;
        }
    }

	FrameSettings getFrameSettingsFromEncodeByte(const FrameSettings& frSet , buffer_t tagBuffer)
    {
        //const auto& frameOffset = frSet->getFrameKeyOffset();
        const auto& frameContentOffset = frSet.getFramePayloadOffset();
        const auto& frameSize = frSet.getFrameLength();

        switch (tagBuffer->at(frameContentOffset)) {
            case 0x0: {
				return FrameSettings::create()->fromFrameProperties(frSet)
					.with_framecontent_offset(frameContentOffset + 1)
					.with_frame_length(frameSize - 1);

                break;
            }
            case 0x1: {
                uint32_t doSwap = 0;
                const uint16_t encodeOder = (tagBuffer->at(frameContentOffset + 1) << 8 |
					tagBuffer->at(frameContentOffset + 2));
                if (encodeOder == 0xfeff) {
                    doSwap = 1;
                }

                return FrameSettings::create()->fromFrameProperties(frSet)
					.with_framecontent_offset(frameContentOffset + 3)
					.with_encode_flag(0x01)
					.with_do_swap(doSwap)
					.with_frame_length(frameSize - 3);
                break;
            }
            case 0x2: { /* UTF-16BE [UTF-16] encoded Unicode [UNICODE] without
                           BOM */             
				return FrameSettings::create()->fromFrameProperties(frSet)
					.with_framecontent_offset(frameContentOffset + 1)
					.with_encode_flag(0x02)
					.with_frame_length(frameSize - 1);
                break;
            }
            case 0x3: { /* UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated
                           with $00 */
				return FrameSettings::create()->fromFrameProperties(frSet)
					.with_framecontent_offset(frameContentOffset + 3)
					.with_encode_flag(0x03)
					.with_frame_length(frameSize - 3);

                break;
            }
            default: {
				return FrameSettings::create()->fromFrameProperties(frSet)
					.with_framecontent_offset(frameContentOffset + 1)
					.with_frame_length(frameSize - 1);
                break;
            }
        }
    }

    std::string formatFramePayload(std::string_view content,
                                  const FrameSettings& frameProperties) {

        using localVariant =  std::variant<std::string_view, std::u16string, std::u32string>;

        const localVariant encodeValue = [&] {
            localVariant varEncode;

            if (frameProperties.getEncodingValue() == 0x01) {
                varEncode = std::u16string();
            } else if (frameProperties.getEncodingValue() == 0x02) {  // unicode without BOM
                varEncode = std::string_view("");
            } else if (frameProperties.getEncodingValue() == 0x03) {  // UTF8 unicode
                varEncode = std::u32string();
            }

            return varEncode;
        }();

        return std::visit(
            overloaded{

				[&](std::string_view arg) { (void)arg;  return std::string(content); },

                [&](std::u16string arg) {
					(void)arg;

                    if (frameProperties.getSwapValue() == 0x01) {
                        std::string_view val =
                            tagBase::getW16StringFromLatin<std::vector<uint8_t>>(content);
                        const auto ret = tagBase::swapW16String(val);
                        return ret;
                    } else {
                        const auto ret =
                            tagBase::getW16StringFromLatin<std::vector<uint8_t>>(content);
                        return ret;
                    }
                },

                [&](std::u32string arg) {
					(void)arg;
                    const auto ret = std::string(content) + '\0';
                    return ret;
                },

            },
            encodeValue);
    }

	uint32_t getFramePosition(std::string_view frameID, const std::string& tagAreaOpt)
	{
		const auto& tagArea = tagAreaOpt;

		auto searchFramePosition =
			id3::searchFrame<std::string_view>(tagArea);

		return searchFramePosition.execute(frameID);	
	}

    class TagReadWriter {
    public:
        explicit TagReadWriter(const ::id3v2::basicParameters& params)
        {
            std::call_once(m_once, [&params, this]() {

                tagBuffer =
                    GetTagHeader(params.get_filename()) | GetTagSize |
                    [&](uint32_t tags_size) {
                        return CreateTagBufferFromFile(
                            params.get_filename(), tags_size + GetTagHeaderSize<uint32_t>());
                    };

                if(params.get_frame_id().length() == 0)
					ID3V2_THROW("No frame ID parameter");

				mParams = findFrameSettings(params);

				const auto& frameProperties = mParams->get_frame_settings();

				if (mParams->get_tag_area().size() == 0) {
					ID3V2_THROW("tag area length = 0");
				}
				if (mParams->get_tag_Buffer()->size() == 0) {
					ID3V2_THROW("tag buffer length = 0");
				}
				if (frameProperties.getFrameLength() > mParams->get_tag_area().size()) {
					ID3V2_THROW("frame length > tag area length");
				}
				if (frameProperties.getFramePayloadLength() == 0) {
					ID3V2_THROW("frame payload length == 0");
				}
            });
        }

        const std::string getFramePayload() const
		{
			const auto& frameProperties = mParams->get_frame_settings();

			const auto framePayload = ExtractString(tagBuffer, frameProperties.getFramePayloadOffset(),
				frameProperties.getFramePayloadLength());

			if (frameProperties.getSwapValue() == 0x01) 
			{
				const auto tempPayload = tagBase::swapW16String(std::string_view(framePayload));
				return tempPayload;
			} else 
			{
				return framePayload;
			}  
        }

		const std::shared_ptr<::id3v2::basicParameters> getIDParameters() const
		{
			return mParams;
		}

    private:
        mutable std::once_flag m_once;
		std::shared_ptr<::id3v2::basicParameters> mParams;
        buffer_t tagBuffer = {};

		std::shared_ptr<::id3v2::basicParameters> findFrameSettings(const basicParameters& params) 
		{
			const auto tagArea = GetTagArea(tagBuffer);

			const auto framePos = getFramePosition(params.get_frame_id(), tagArea);

			const auto FrameSize = GetFrameSize<uint32_t>(tagBuffer
						, params.get_tag_version(), framePos).value();

			if (params.get_frame_id().find_first_of("T") ==
				0)  // if frameID starts with T
			{
				const auto framesettings = FrameSettings::create()
					->with_frameID_offset(framePos)
					.with_framecontent_offset(framePos + GetFrameHeaderSize(params.get_tag_version()))
					.with_frame_length(FrameSize);

				const auto framesettingsObj = getFrameSettingsFromEncodeByte(framesettings, tagBuffer);

				return std::make_shared<basicParameters>(params.with_frame_settings(framesettingsObj)
					.with_tag_area(tagArea)
					.with_tag_buffer(tagBuffer)
					);
			}
			else {
				const auto frameSettings = FrameSettings::create()
					->with_frameID_offset(framePos)
					.with_framecontent_offset(framePos + GetFrameHeaderSize(mParams->get_tag_version()))
					.with_frame_length(FrameSize);
			
				return std::make_shared<basicParameters>(params.with_frame_settings(frameSettings)
					.with_tag_area(tagArea)
					.with_tag_buffer(tagBuffer)
					);
			}
    }
    
};

	class writer
	{
	public:
		explicit writer(const TagReadWriter& ReadWriteFrameObj):
			FrameReadWriterObj(ReadWriteFrameObj)
		{
			const auto params = ReadWriteFrameObj.getIDParameters();

			if (params->get_frame_content_to_write().size() == 0) {
				ID3V2_THROW("frame payload to write size = 0");
			}
		}

		bool execute() const
		{
			const auto params = FrameReadWriterObj.getIDParameters();

			return this->writeFramePayload(*params);
		}

	private:
		writer() = default;
		const TagReadWriter& FrameReadWriterObj;

		bool writeFramePayload(const basicParameters& params) const
		{
			const auto& frameProperties = params.get_frame_settings();

			const std::string framePayloadToWrite =
				formatFramePayload(params.get_frame_content_to_write(), frameProperties);

			if (frameProperties.getFramePayloadLength() <
				framePayloadToWrite.size())  // resize whole header
			{
				std::cerr << "frames ize does not fit\n";
				//return false;

				const uint32_t extraLength =
					(framePayloadToWrite.size() - frameProperties.getFramePayloadLength());

				const id3v2::FileExtended fileExtended{
					params, extraLength, framePayloadToWrite
				};

				return fileExtended.get_status();

			}
			else {
				return id3::WriteFile(params.get_filename(), framePayloadToWrite, frameProperties);
			}
		}
	};

	template <typename id3Type>
	const auto is_tag(std::string name) {
		return (std::any_of(id3v2::GetTagNames<id3Type>().cbegin(),
			id3v2::GetTagNames<id3Type>().cend(),
			[&](std::string obj) { return name == obj; }));
	}
}; //end namespace id3v2


#endif //_ID3V2_COMMON
