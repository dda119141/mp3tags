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
            std::optional<std::string_view> frameID {};
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
    std::optional<T> GetFrameSize(const std::vector<uint8_t>& buff,
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

    expected::Result<std::vector<uint8_t>> UpdateFrameSize(const std::vector<uint8_t>& buffer,
                                             uint32_t extraSize,
                                             uint32_t frameIDPosition) {

        const uint32_t frameSizePositionInArea = frameIDPosition + 4;
        constexpr uint32_t frameSizeLengthInArea = 4;
        constexpr uint32_t frameSizeMaxValuePerElement = 127;

        id3::log()->info(" frame ID Position: {}", frameIDPosition);
        id3::log()->info("Frame Bytes before update : {}",
                           spdlog::to_hex(std::begin(buffer) + frameSizePositionInArea,
                                          std::begin(buffer) + frameSizePositionInArea + 4));
        return updateAreaSize<uint32_t>(
            buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
            frameSizeMaxValuePerElement);
    }

    template <typename... Args>
    expected::Result<std::vector<uint8_t>> updateFrameSizeIndex(const iD3Variant& TagVersion,
                                                Args... args) 
    {
        return std::visit(
            overloaded{
                [&](id3v2::v30 arg) { return UpdateFrameSize(args...); },
                [&](id3v2::v40 arg) { return UpdateFrameSize(args...); },
                [&](id3v2::v00 arg) { return arg.UpdateFrameSize(args...); }},
            TagVersion);
    }

    expected::Result<std::vector<uint8_t>> checkForID3(const std::vector<uint8_t>& buffer) 
    {
        return id3v2::GetID3FileIdentifier(buffer)
            |
            [&](const std::string& val) -> expected::Result<std::vector<uint8_t>> {
            if (val != "ID3") {
                id3::log()->info(" ID3 tag not present");

                std::string ret = std::string("ID3 tag not present ") +
                                  std::string("\n");
                return expected::makeError<std::vector<uint8_t>, std::string>(ret.c_str());
            } else {
                return expected::makeValue<std::vector<uint8_t>>(buffer);
            }
        };
    }

    template <typename T>
    const auto getFrameSettingsFromEncodeByte(const std::vector<uint8_t>& buff, uint32_t frameOffset,
                                       uint32_t frameContentOffset,
                                       uint32_t FrameSize) 
    {
        switch (buff[frameContentOffset]) {
            case 0x0: {
                const T frameConfig{
                    frameOffset, frameContentOffset + 1, FrameSize - 1,
                };
                return expected::makeValue<FrameSettings>(frameConfig);
                break;
            }
            case 0x1: {
                uint32_t doSwap = 0;
                const uint16_t encodeOder = (buff[frameContentOffset + 1] << 8 |
                                             buff[frameContentOffset + 2]);
                if (encodeOder == 0xfeff) {
                    doSwap = 1;
                }

                const T frameConfig{
                    frameOffset, frameContentOffset + 3, FrameSize - 3,
                    0x01 /* doEncode */
                    ,
                    doSwap /* doSwap */
                };
                return expected::makeValue<FrameSettings>(frameConfig);
                break;
            }
            case 0x2: { /* UTF-16BE [UTF-16] encoded Unicode [UNICODE] without
                           BOM */
                const T frameConfig{frameOffset, frameContentOffset + 1,
                                    FrameSize - 1, 0x02};
                return expected::makeValue<FrameSettings>(frameConfig);
                break;
            }
            case 0x3: { /* UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated
                           with $00 */
                const T frameConfig{frameOffset, frameContentOffset + 3,
                                    FrameSize - 3, 0x03};
                return expected::makeValue<FrameSettings>(frameConfig);
                break;
            }
            default: {
                T frameConfig{frameOffset, frameContentOffset + 1, FrameSize - 1};
                return expected::makeValue<FrameSettings>(frameConfig);
                break;
            }
        }
    }

    template <typename T>
    std::optional<std::vector<uint8_t>> preparePayloadToWrite(T content, const FrameSettings& frameConfig,
                                            std::vector<uint8_t>& buffer) {
        assert(frameConfig.getPayloadLength() >= content.size());

        const auto PositionTagStart = frameConfig.getFramePayloadOffset();

        id3::log()->info(" PositionTagStart: {}", PositionTagStart);

        assert(PositionTagStart > 0);

        const auto iter = std::begin(buffer) + PositionTagStart;

        std::transform(iter, iter + content.size(), content.begin(), iter,
                       [](char a, char b) { return b; });

        std::transform(iter + content.size(), iter + frameConfig.getPayloadLength(),
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
                if (ret.has_value()) {

                    ID3_LOG_TRACE("Buffer could be read - prepare to extend.");
                    mCBuffer = ret.value();

                    const auto ret1 =
                        extendBuffer(frameConfig, additionalSize, tagVersion) |
                        [&](const std::vector<uint8_t>& buffer) {
                            mCBuffer = buffer;
                            return id3::contructNewFrameSettings<FrameSettings>(
                                frameConfig, additionalSize);
                        } |
                        [&](const FrameSettings& frameSettings) {
                            mFrameSettings = frameSettings;
                            mAdditionalSize = additionalSize;

                            return expected::makeValue<bool>(true);
                        };
                    assert(ret1.has_value());
                };
            });
        }

        expected::Result<bool> extendFile(const std::string& content) 
        {
            return (mFrameSettings | [&](const FrameSettings& frameConfig) {

                assert(frameConfig.getPayloadLength() >= content.size());

                const auto ret = preparePayloadToWrite<std::string_view>(content, 
                        frameConfig, mCBuffer.value())
                        | [&](const std::vector<uint8_t>& buff) {
                            return this->ReWriteFile(mAdditionalSize);
                        };

                return ret;
            });
        }

    private:
        std::optional<std::vector<uint8_t>> mCBuffer {};
        std::optional<FrameSettings> mFrameSettings {};
        uint32_t mAdditionalSize;
        std::once_flag m_once;
        const std::string& FileName;

        const expected::Result<std::vector<uint8_t>> extendBuffer(
            const FrameSettings& frameConfig, uint32_t additionalSize,
            const iD3Variant& tagVersion) 
        {
            ID3_LOG_TRACE("entering extendBuffer func...");
            constexpr uint32_t tagsSizePositionInHeader = 6;
            constexpr uint32_t frameSizePositionInFrameHeader = 4;

            const auto ret = 
                mCBuffer 
                | [&](const std::vector<uint8_t>& cBuffer) {

                ID3_LOG_TRACE("FrameSize... length: {}, frameID start: {}",
                              frameConfig.getPayloadLength(),
                              frameConfig.getFrameKeyOffset());
                ID3_LOG_TRACE("Updating segments...");

                const auto tagSizeBuffer =
                    updateTagSize(cBuffer, additionalSize);

                const auto frameSizeBuff =
                    updateFrameSizeIndex<const std::vector<uint8_t>&, uint32_t, uint32_t>(
                        tagVersion, cBuffer, additionalSize,
                        frameConfig.getFrameKeyOffset());

                assert(frameConfig.getFrameKeyOffset() + frameConfig.getPayloadLength() <
                       cBuffer.size());

                auto finalBuffer = std::move(cBuffer);
                id3::replaceElementsInBuff(tagSizeBuffer.value(), finalBuffer,
                                           tagsSizePositionInHeader);

                id3::replaceElementsInBuff(frameSizeBuff.value(), finalBuffer,
                                           frameConfig.getFrameKeyOffset() +
                                               frameSizePositionInFrameHeader);

                ID3_LOG_TRACE(
                    "Tag Frame Bytes after update : {}",
                    spdlog::to_hex(
                        std::begin(finalBuffer) + frameConfig.getFrameKeyOffset() +
                            frameSizePositionInFrameHeader,
                        std::begin(finalBuffer) + frameConfig.getFrameKeyOffset() +
                            frameSizePositionInFrameHeader + 4));

                auto it = finalBuffer.begin() + frameConfig.getFramePayloadOffset() +
                          frameConfig.getPayloadLength();
                finalBuffer.insert(it, additionalSize, 0);

                return expected::makeValue<std::vector<uint8_t>>(finalBuffer);
            };

            return ret;
        }

        expected::Result<bool> ReWriteFile(uint32_t extraSize) {
            std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);

            if (!filRead.good()) {
                ID3_LOG_ERROR("Error opening file");
                return expected::makeError<bool>() << "__func__"
                                                   << ": Error opening file";
            }

            return mCBuffer 
                | [&](const std::vector<uint8_t>& buff) {
                const auto _headerAndTagsSize = GetTotalTagSize(buff);

                return _headerAndTagsSize | [&](uint32_t headerAndTagsSize) {

                    ID3_LOG_TRACE("headerAndtagsize: {}", headerAndTagsSize);

                    const uint32_t tagsSizeOld = headerAndTagsSize - extraSize;

                    uint32_t dataSize = filRead.tellg();
                    assert(dataSize > tagsSizeOld);
                    dataSize -= tagsSizeOld;

                    filRead.seekg(tagsSizeOld);
                    std::vector<uint8_t> bufRead;
                    bufRead.reserve(dataSize);
                    filRead.read(reinterpret_cast<char*>(&bufRead[0]),
                                 dataSize);

                    assert(buff.size() < dataSize);

                    ID3_LOG_TRACE("datasize: {} - buffer size: {}", dataSize,
                                  buff.size());

                    std::ofstream filWrite(FileName + modifiedEnding,
                                           std::ios::binary | std::ios::app);

                    std::for_each(
                        std::begin(buff), std::end(buff),
                        [&filWrite](const char& n) { filWrite << n; });

                    std::for_each(
                        bufRead.begin(), bufRead.begin() + dataSize,
                        [&filWrite](const char& n) { filWrite << n; });

                    ID3_LOG_INFO("success: {}", __func__);
                    return expected::makeValue<bool>(true);
                };
            };
        };
    };

    template <typename type>
    class TagReadWriter {
    public:
        explicit TagReadWriter(const ::id3v2::basicParameters& params) 
            : mParams(params) {
            std::call_once(m_once, [this]() {

                const auto ret =
                    GetTagHeader(mParams.filename) | GetTagSize |
                    [&](uint32_t tags_size) {
                        return GetStringFromFile(
                            mParams.filename, tags_size + GetTagHeaderSize<uint32_t>());
                    };
                if (ret.has_value()) {
                    buffer = ret.value();
                }else{
                     throw std::range_error("No found area for tag");
                }
                if(!mParams.frameID)
                     throw std::runtime_error("No frame ID parameter");

                const auto frameSet = findFrameSettings();

                if (frameSet.has_value())
                    mFrameSettings = frameSet.value();

            });
        }

        struct frameObj_t
        {
            uint32_t framePos;
            uint32_t frameSize;
            uint32_t frameContentOffset;
        };

        const FrameSettings& GetFrameSettings() {
            return mFrameSettings.value();
        }

        template <typename V>
        const expected::Result<V> getFramePayload() {

            const auto res = buffer | [=](const std::vector<uint8_t>& buff) {
            const auto& FrameConfig = mFrameSettings.value();

            ID3_LOG_INFO("frame payload offset: {}, framesize {}",
                        FrameConfig.getFramePayloadOffset(),
                        FrameConfig.getPayloadLength());

            if (FrameConfig.getPayloadLength() == 0) {
               ID3_LOG_ERROR(
                   "getFramePayload: Error: frame length = {}",
                   FrameConfig.getPayloadLength());
               return expected::makeError<V>(
                   "getFramePayload: Error: frame length = 0");
            }

            const auto val = ExtractString<uint32_t>(
               buff, FrameConfig.getFramePayloadOffset(),
               FrameConfig.getPayloadLength());

            return val | [&FrameConfig](
                            const std::string& framePayload) {

               if (FrameConfig.getSwapValue() == 0x01) {
                   const auto tempPayload = tagBase::swapW16String(
                       std::string_view(framePayload));
                   return expected::makeValue<V>(tempPayload);
               } else {
                   return expected::makeValue<V>(framePayload);
               }
            };
            };

            return res;
        }

    private:
        mutable std::once_flag m_once;
        const ::id3v2::basicParameters& mParams;
        std::optional<FrameSettings> mFrameSettings {};
        std::optional<std::vector<uint8_t>> buffer {};

        const expected::Result<FrameSettings> findFrameSettings() {

            struct frameObj_t frameObj = {};

            const auto ret =
                buffer 
                | [this](const std::vector<uint8_t>& buffer) -> expected::Result<std::string>
                {
                    ID3_LOG_TRACE("frameID name: #{}", std::string(mParams.frameID.value()));
                    return GetTagArea(buffer);
                } 
                | [this](const std::string&
                           tagArea) -> expected::Result<uint32_t> {

                    auto searchTagPosition =
                        id3::search_tag<std::string_view>(tagArea);
                    const auto ret = searchTagPosition(mParams.frameID.value());

                    ID3_LOG_TRACE("frameID name: #{}", std::string(mParams.frameID.value()));

                    return ret;
                } 
                | [this, &frameObj](uint32_t framePos) -> expected::Result<FrameSettings> {

                    if (framePos == 0) throw std::range_error("frame position = 0");

                    assert(framePos >= GetTagHeaderSize<uint32_t>());

                    frameObj.framePos = framePos;

                    ID3_LOG_TRACE("frame position: {}", framePos);

                    return buffer | [&framePos, this](const std::vector<uint8_t>& buff) {

                        return GetFrameSize<uint32_t>(buff, mParams.tagVersion, framePos);

                    } 
                 | [frameObj, this](uint32_t FrameSize) -> expected::Result<FrameSettings> {
                    const uint32_t frameContentOffset =
                        frameObj.framePos + GetFrameHeaderSize(mParams.tagVersion);

                    ID3_LOG_TRACE("frame content offset: {} and Frame Size: {}",
                                  frameContentOffset, FrameSize);

                    if (frameContentOffset == 0) throw std::range_error("Error frameContentOffset == 0");
                    if (FrameSize == 0) throw std::range_error("frame Size = 0");

                    return buffer | [&](const std::vector<uint8_t>& buff) {
                        ID3_LOG_TRACE("frameID starts with T: {}", mParams.frameID.value());

                        if (mParams.frameID.value().find_first_of("T") ==
                            0)  // if frameID starts with T
                        {
                            return getFrameSettingsFromEncodeByte<FrameSettings>(
                                buff, frameObj.framePos, frameContentOffset, FrameSize);
                        }

                        const auto result =
                            FrameSettings(frameObj.framePos, frameContentOffset, FrameSize);

                        return expected::makeValue<FrameSettings>(result);
                    };
                };
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

    expected::Result<bool> writeFramePayload(const ::id3v2::basicParameters& params) 
    {
        id3v2::TagReadWriter<std::string_view> obj(params);

        const id3::FrameSettings& frameConfig = obj.GetFrameSettings();

        if (!params.framePayload.has_value()) return expected::makeError<bool>("No frame payload parameter\n");

        const std::string framePayloadToWrite =
            formatFramePayload(params.framePayload.value(), frameConfig);

        ID3_LOG_TRACE("Content write: {} prepared", framePayloadToWrite);
        ID3_LOG_TRACE("frame content offset: {}",
                     frameConfig.getFramePayloadOffset());
        ID3_LOG_TRACE("frame payload length: {} ", frameConfig.getPayloadLength());

        if (frameConfig.getPayloadLength() == 0) {
            ID3_LOG_ERROR("findFrameSettings: Error, framesize = 0\n");

            return expected::makeError<bool>(
                "findFrameSettings: Error, framesize = 0\n");

        } else if (frameConfig.getPayloadLength() <
                   framePayloadToWrite.size())  // resize whole header
        {
            const auto tagExtended = [&](){
                const uint32_t extraLength =
                    (framePayloadToWrite.size() - frameConfig.getPayloadLength());

                struct id3v2::newTagArea NewTagArea {
                    params.filename, frameConfig, params.tagVersion, extraLength
                };

                return NewTagArea.extendFile(framePayloadToWrite);
            }();

            return (tagExtended | [&](const bool& val) {
                return renameFile(params.filename + id3::modifiedEnding, params.filename);
            });

        } else {
            return id3::WriteFile(params.filename, framePayloadToWrite, frameConfig);
        }
    }

}; //end namespace id3v2


#endif //_ID3V2_COMMON
