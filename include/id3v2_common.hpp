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
#include <id3v2_v30.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v00.hpp>
#include <string_conversion.hpp>

#define DEBUG


namespace id3v2
{
    
    using iD3Variant = std::variant <::id3v2::v00, ::id3v2::v30, ::id3v2::v40>;

    class basicParameters {
        public:
            const std::string& filename;
            std::optional<std::string_view> framePayload {};
            std::string_view frameID = {};
            iD3Variant tagVersion;

        public:
            explicit basicParameters(const std::string& Filename) :
                filename(Filename)
            {
            }

            explicit basicParameters(const std::string& Filename, 
                        const iD3Variant& TagVersion,
                        std::string_view FrameID) :

                    filename(Filename),
                    frameID(FrameID),
                    tagVersion(TagVersion)
            {
            }

            explicit basicParameters(const std::string& Filename, 
                        const iD3Variant& TagVersion,
                        std::string_view FrameID,
                        std::string_view FramePayload) :

                    filename(Filename),
                    framePayload(FramePayload),
                    frameID(FrameID),
                    tagVersion(TagVersion)
            {
            }

      };


    template <typename T>
    std::optional<T> GetFrameSize(buffer_t buff,
                                  const iD3Variant& TagVersion,
                                  uint32_t framePos) {
        return std::visit(
            overloaded{[&](id3v2::v30 arg) {
                           return arg.GetFrameSize(buff, framePos);
                       },
                       [&](id3v2::v40 arg) {
                           return arg.GetFrameSize(buff, framePos);
                       },
                       [&](id3v2::v00 arg) {
                           return arg.GetFrameSize(buff, framePos);
                       }},
            TagVersion);
    }

    uint32_t GetFrameHeaderSize(const iD3Variant& TagVersion) {
        return std::visit(
            overloaded{[&](id3v2::v30 arg) { return arg.FrameHeaderSize(); },
                       [&](id3v2::v40 arg) { return arg.FrameHeaderSize(); },
                       [&](id3v2::v00 arg) { return arg.FrameHeaderSize(); }},
            TagVersion);
    }

    std::optional<buffer_t> UpdateFrameSize(buffer_t buffer,
                                             uint32_t extraSize,
                                             uint32_t frameIDPosition) {

        const uint32_t frameSizePositionInArea = frameIDPosition + 4;
        constexpr uint32_t frameSizeLengthInArea = 4;
        constexpr uint32_t frameSizeMaxValuePerElement = 127;

        id3::log()->info(" frame ID Position: {}", frameIDPosition);
        id3::log()->info("Frame Bytes before update : {}",
                           spdlog::to_hex(std::begin(*buffer) + frameSizePositionInArea,
                                          std::begin(*buffer) + frameSizePositionInArea + 4));
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
                [&](id3v2::v00 arg) { return arg.UpdateFrameSize(args...); }},
            TagVersion);
    }

    std::optional<buffer_t> checkForID3(buffer_t buffer) 
    {
        return id3v2::GetID3FileIdentifier(buffer)
            |
            [&](shared_string_t val) -> std::optional<buffer_t> {
            if (*val != "ID3") {
                id3::log()->info(" ID3 tag not present");
                return {};
            } else {
                return buffer;
            }
        };
    }

    const auto getFrameSettingsFromEncodeByte(const FrameSettings& frSet)
    {
        const auto& frameOffset = frSet.getFrameKeyOffset();
        const auto& frameContentOffset = frSet.getFramePayloadOffset();
        const auto& frameSize = frSet.getFrameLength();
        const auto buff = frSet.getAudioBuffer();

        switch (buff->at(frameContentOffset)) {
            case 0x0: {
                const auto ret = FrameSettings()
                .with_frameID_offset(frameOffset) 
                .with_framecontent_offset(frameContentOffset + 1) 
                .with_frame_length(frameSize - 1);

                return ret;
                break;
            }
            case 0x1: {
                uint32_t doSwap = 0;
                const uint16_t encodeOder = (buff->at(frameContentOffset + 1) << 8 |
                                             buff->at(frameContentOffset + 2));
                if (encodeOder == 0xfeff) {
                    doSwap = 1;
                }

                const auto ret = FrameSettings()
                .with_frameID_offset(frameOffset) 
                .with_framecontent_offset(frameContentOffset + 3)
                .with_encode_flag(0x01) 
                .with_do_swap(doSwap) 
                .with_frame_length(frameSize - 3);

                return ret;
                break;
            }
            case 0x2: { /* UTF-16BE [UTF-16] encoded Unicode [UNICODE] without
                           BOM */
                const auto ret = FrameSettings()
                .with_frameID_offset(frameOffset) 
                .with_framecontent_offset(frameContentOffset + 1)
                .with_encode_flag(0x02) 
                .with_frame_length(frameSize - 1);
                                    
                return ret;
                break;
            }
            case 0x3: { /* UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated
                           with $00 */
                const auto ret = FrameSettings()
                .with_frameID_offset(frameOffset) 
                .with_framecontent_offset(frameContentOffset + 3)
                .with_encode_flag(0x03) 
                .with_frame_length(frameSize - 3);

                return ret;
                break;
            }
            default: {
                const auto ret = FrameSettings()
                .with_frameID_offset(frameOffset) 
                .with_framecontent_offset(frameContentOffset + 1)
                .with_frame_length(frameSize - 1);

                return ret;
                break;
            }
        }
    }

    template <typename T>
    std::optional<buffer_t> preparePayloadToWrite(T content, const FrameSettings& frameConfig,
                                            buffer_t buffer) {
        assert(frameConfig.getFramePayloadLength() >= content.size());

        const auto PositionTagStart = frameConfig.getFramePayloadOffset();

        id3::log()->info(" PositionTagStart: {}", PositionTagStart);

        assert(PositionTagStart > 0);

        const auto iter = std::begin(*buffer) + PositionTagStart;

        std::transform(iter, iter + content.size(), content.begin(), iter,
                       [](char a, char b) { return b; });

        std::transform(iter + content.size(), iter + frameConfig.getFramePayloadLength(),
                       content.begin(), iter + content.size(),
                       [](char a, char b) { return 0x00; });

        return buffer;
    }

    std::string formatFramePayload(std::string_view content,
                                  const FrameSettings& frameConfig) {

        using localVariant =  std::variant<std::string_view, std::u16string, std::u32string>;

        const localVariant encodeValue = [&] {
            localVariant varEncode;

            if (frameConfig.getEncodingValue() == 0x01) {
                varEncode = std::u16string();
            } else if (frameConfig.getEncodingValue() == 0x02) {  // unicode without BOM
                varEncode = std::string_view("");
            } else if (frameConfig.getEncodingValue() == 0x03) {  // UTF8 unicode
                varEncode = std::u32string();
            }

            return varEncode;
        }();

        return std::visit(
            overloaded{

                [&](std::string_view arg) { return std::string(content); },

                [&](std::u16string arg) {

                    if (frameConfig.getSwapValue() == 0x01) {
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
                    const auto ret = std::string(content) + '\0';
                    return ret;
                },

            },
            encodeValue);
    }

    struct newTagArea {

    public:
        explicit newTagArea(const std::string& filename, const FrameSettings& frameConfig,
                            const iD3Variant& tagVersion,
                            uint32_t additionalSize)
            : FileName(filename) 
        {
            std::call_once(m_once, [&filename, &frameConfig, &tagVersion,
                                    additionalSize, this]() {
                const auto ret =
                    GetTagHeader(filename) | GetTagSize | [&](uint32_t tags_size) {
                        return GetStringFromFile(
                            filename, tags_size + GetTagHeaderSize<uint32_t>());
                    };

                ID3_LOG_TRACE("Buffer could be read - prepare to extend.");
                mCBuffer = ret;

                const auto buffer_opt = extendBuffer(frameConfig, additionalSize, tagVersion) |
                    [&](buffer_t buffer) {
                        mCBuffer = buffer;
                        return contructNewFrameSettings(frameConfig, additionalSize);
                    };

                const auto result = buffer_opt |
                    [&](const FrameSettings& frameSettings) {
                        mFrameSettings = frameSettings;
                        mAdditionalSize = additionalSize;

                        return true;
                    };

                assert(result == true);
        });
        }

        std::optional<bool> extendFile(const std::string& content) 
        {
            return (mFrameSettings | [&](const FrameSettings& frameConfig) {

                assert(frameConfig.getFramePayloadLength() >= content.size());

                const auto ret = preparePayloadToWrite<std::string_view>(content, 
                        frameConfig, mCBuffer.value());

                return this->ReWriteFile(mAdditionalSize);
            });
        }

    private:
        std::optional<buffer_t> mCBuffer = {};
        std::optional<FrameSettings> mFrameSettings = {};
        uint32_t mAdditionalSize;
        std::once_flag m_once;
        const std::string& FileName;

        const std::optional<buffer_t> extendBuffer(
            const FrameSettings& frameConfig, uint32_t additionalSize,
            const iD3Variant& tagVersion) 
        {
            ID3_LOG_TRACE("entering extendBuffer func...");
            constexpr uint32_t tagsSizePositionInHeader = 6;
            constexpr uint32_t frameSizePositionInFrameHeader = 4;

            const auto ret = 
                mCBuffer 
                | [&](buffer_t cBuffer) {

                ID3_LOG_TRACE("FrameSize... length: {}, frameID start: {}",
                              frameConfig.getFramePayloadLength(),
                              frameConfig.getFrameKeyOffset());
                ID3_LOG_TRACE("Updating segments...");

                const auto tagSizeBuffer =
                    updateTagSize(cBuffer, additionalSize);

                const auto frameSizeBuff =
                    updateFrameSizeIndex<buffer_t, uint32_t, uint32_t>(
                        tagVersion, cBuffer, additionalSize,
                        frameConfig.getFrameKeyOffset());

                assert(frameConfig.getFrameKeyOffset() + frameConfig.getFramePayloadLength() <
                       cBuffer->size());

                auto finalBuffer = std::move(cBuffer);
                id3::replaceElementsInBuff(tagSizeBuffer.value(), finalBuffer,
                                           tagsSizePositionInHeader);

                id3::replaceElementsInBuff(frameSizeBuff.value(), finalBuffer,
                                           frameConfig.getFrameKeyOffset() +
                                               frameSizePositionInFrameHeader);

                ID3_LOG_TRACE(
                    "Tag Frame Bytes after update : {}",
                    spdlog::to_hex(
                        std::begin(*finalBuffer) + frameConfig.getFrameKeyOffset() +
                            frameSizePositionInFrameHeader,
                        std::begin(*finalBuffer) + frameConfig.getFrameKeyOffset() +
                            frameSizePositionInFrameHeader + 4));

                const auto it = finalBuffer->begin() + frameConfig.getFramePayloadOffset() +
                          frameConfig.getFramePayloadLength();
                finalBuffer->insert(it, additionalSize, 0);

                return finalBuffer;
            };

            return ret;
        }

        std::optional<bool> ReWriteFile(uint32_t extraSize) {
            std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);

            if (!filRead.good()) {
                ID3_LOG_ERROR("Error opening file");
                return {};
            }

            return mCBuffer 
                | [&](buffer_t buff) {
                const auto _headerAndTagsSize = GetTotalTagSize(buff);

                return [&](uint32_t headerAndTagsSize) {

                    ID3_LOG_TRACE("headerAndtagsize: {}", headerAndTagsSize);

                    const uint32_t tagsSizeOld = headerAndTagsSize - extraSize;

                    uint32_t dataSize = filRead.tellg();
                    assert(dataSize > tagsSizeOld);
                    dataSize -= tagsSizeOld;

                    filRead.seekg(tagsSizeOld);
                    auto bufRead = std::make_shared<std::vector<uint8_t>>();
                    bufRead->reserve(dataSize);
                    filRead.read(reinterpret_cast<char*>(&bufRead->at(0)),
                                 dataSize);

                    assert(buff->size() < dataSize);

                    ID3_LOG_TRACE("datasize: {} - buffer size: {}", dataSize,
                                  buff->size());

                    std::ofstream filWrite(FileName + modifiedEnding,
                                           std::ios::binary | std::ios::app);

                    std::for_each(
                        std::begin(*buff), std::end(*buff),
                        [&filWrite](const char& n) { filWrite << n; });

                    std::for_each(
                        bufRead->begin(), bufRead->begin() + dataSize,
                        [&filWrite](const char& n) { filWrite << n; });

                    return true;
                }(_headerAndTagsSize);
            };
        };
    };

    template <typename type>
    class TagReadWriter {
    public:
        explicit TagReadWriter(const ::id3v2::basicParameters& params) 
            : mParams(params) {
            std::call_once(m_once, [this]() {

                buffer =
                    GetTagHeader(mParams.filename) | GetTagSize |
                    [&](uint32_t tags_size) {
                        return GetStringFromFile(
                            mParams.filename, tags_size + GetTagHeaderSize<uint32_t>());
                    };

                if(mParams.frameID.length() == 0)
                     throw std::runtime_error("No frame ID parameter");

                const auto frameSet = findFrameSettings();

                mFrameSettings = frameSet.value();

            });
        }

        const FrameSettings& GetFrameSettings() {
            return mFrameSettings.value();
        }

        template <typename V>
        const std::optional<V> getFramePayload() {

            const auto res = buffer | [=](buffer_t buff) {
            const auto& FrameConfig = mFrameSettings.value();

            const auto val = ExtractString<uint32_t>(
               buff, FrameConfig.getFramePayloadOffset(),
               FrameConfig.getFramePayloadLength());

            return val | [&FrameConfig](
                            shared_string_t framePayload) {

               if (FrameConfig.getSwapValue() == 0x01) {
                   const auto tempPayload = tagBase::swapW16String(
                       std::string_view(*framePayload));
                   return tempPayload;
               } else {
                   return *framePayload;
               }
            };
            };

            return res;
        }

    private:
        mutable std::once_flag m_once;
        const ::id3v2::basicParameters& mParams;
        std::optional<FrameSettings> mFrameSettings = {};
        std::optional<buffer_t> buffer = {};

    uint32_t getFramePosition(std::string_view frameID, std::optional<shared_string_t> tagAreaOpt)
    {
        if (tagAreaOpt.has_value()){
                const auto& tagArea = *(tagAreaOpt.value());

                auto searchTagPosition =
                        id3::search_tag<std::string_view>(tagArea);

                try {
                  const auto _framePos = searchTagPosition(frameID);
                              return _framePos.value();
                
                } catch (const std::bad_optional_access& e) {
                
                    std::cout << e.what();
                    std::cout <<  " Could not find " << frameID << "\n";

                    return 0;
                }
          
        }else{
            throw std::invalid_argument("tagArea Empty while searching for frame position");
        }

    }

        const std::optional<FrameSettings> findFrameSettings() {

            id3::FrameSettings framesettings;

            std::optional<shared_string_t> tagAreaOpt =  buffer 
                | [](buffer_t buffer_arg) -> std::optional<shared_string_t>
                {
                    return GetTagArea(buffer_arg);
                };
#if 0
            const auto framePos = tagAreaOpt
              | [this](shared_string_t tagArea) -> uint32_t {

                    auto searchTagPosition =
                        id3::search_tag<std::string_view>(*tagArea);

                    const auto _framePos = searchTagPosition(mParams.frameID).value();
                    return _framePos;
            };
#endif
            const auto framePos = getFramePosition(mParams.frameID, tagAreaOpt);

            const auto FrameSize = buffer | [&](buffer_t audioBuffer) {
                return GetFrameSize<uint32_t>(audioBuffer
                     , mParams.tagVersion, framePos).value();
            };

            const auto ret =
                tagAreaOpt  | [&framesettings, &framePos, &FrameSize, this](shared_string_t
                           tagArea) -> std::optional<FrameSettings> {

                    framesettings.with_frameID_offset(framePos)
                        .with_frameID_length(mParams.frameID.length())
                        .with_framecontent_offset(framePos + GetFrameHeaderSize(mParams.tagVersion))
                        .with_frame_length(FrameSize)
                        .with_audio_buffer(buffer.value());

                    if (mParams.frameID.find_first_of("T") ==
                        0)  // if frameID starts with T
                    {
                        return getFrameSettingsFromEncodeByte(framesettings);
                    }

                    return framesettings;
                };

            if (!ret.has_value()) throw std::runtime_error("Could not retrieve frame settings");

            return ret;
        }
    };
  
    template <typename id3Type>
    const auto is_tag(std::string name) {
        return (std::any_of(id3v2::GetTagNames<id3Type>().cbegin(),
                            id3v2::GetTagNames<id3Type>().cend(),
                            [&](std::string obj) { return name == obj; }));
    }

    std::optional<bool> writeFramePayload(const ::id3v2::basicParameters& params) 
    {
        id3v2::TagReadWriter<std::string_view> obj(params);

        const id3::FrameSettings& frameConfig = obj.GetFrameSettings();

        const std::string framePayloadToWrite =
            formatFramePayload(params.framePayload.value(), frameConfig);

        if (frameConfig.getFramePayloadLength() == 0) {
            ID3_LOG_ERROR("findFrameSettings: Error, framesize = 0\n");

            return {};
        } else if (frameConfig.getFramePayloadLength() <
                   framePayloadToWrite.size())  // resize whole header
        {
            std::cerr << "Error, framesize does not fit\n";
            return false;

            const auto tagExtended = [&](){
                const uint32_t extraLength =
                    (framePayloadToWrite.size() - frameConfig.getFramePayloadLength());

                struct id3v2::newTagArea NewTagArea {
                    params.filename, frameConfig, params.tagVersion, extraLength
                };

                return NewTagArea.extendFile(framePayloadToWrite);
            };

            return (tagExtended() | [&](bool val) {
                return renameFile(params.filename + id3::modifiedEnding, params.filename);
            });

        } else {
            return id3::WriteFile(params.filename, framePayloadToWrite, frameConfig);
        }
    }

}; //end namespace id3v2


#endif //_ID3V2_COMMON
