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
    using cUchar = std::vector<unsigned char>;
    using iD3Variant = std::variant <id3v2::v00, id3v2::v30, id3v2::v40>;

    template <typename T>
    std::optional<T> GetFrameSize(const cUchar& buff,
                                  const iD3Variant& TagVersion,
                                  uint32_t tagIndex) {
        return std::visit(
            overloaded{[&](id3v2::v30 arg) {
                           return arg.GetFrameSize(buff, tagIndex);
                       },
                       [&](id3v2::v40 arg) {
                           return arg.GetFrameSize(buff, tagIndex);
                       },
                       [&](id3v2::v00 arg) {
                           return arg.GetFrameSize(buff, tagIndex);
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

    expected::Result<cUchar> UpdateFrameSize(const cUchar& buffer,
                                             uint32_t extraSize,
                                             uint32_t tagLocation) {

        const uint32_t frameSizePositionInArea = tagLocation + 4;
        constexpr uint32_t frameSizeLengthInArea = 4;
        constexpr uint32_t frameSizeMaxValuePerElement = 127;

        id3::log()->info(" Tag Index: {}", tagLocation);
        id3::log()->info("Tag Frame Bytes before update : {}",
                           spdlog::to_hex(std::begin(buffer) + frameSizePositionInArea,
                                          std::begin(buffer) + frameSizePositionInArea + 4));
        return updateAreaSize<uint32_t>(
            buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
            frameSizeMaxValuePerElement);
    }

    template <typename... Args>
    expected::Result<cUchar> updateFrameSizeIndex(const iD3Variant& TagVersion,
                                                Args... args) {
        return std::visit(
            overloaded{
                [&](id3v2::v30 arg) { return UpdateFrameSize(args...); },
                [&](id3v2::v40 arg) { return UpdateFrameSize(args...); },
                [&](id3v2::v00 arg) { return arg.UpdateFrameSize(args...); }},
            TagVersion);
    }

    expected::Result<cUchar> check_for_ID3(const cUchar& buffer) {
        return id3v2::GetID3FileIdentifier(buffer) |
                   [&](const std::string& val) -> expected::Result<cUchar> {
            if (val != "ID3") {
                std::string ret = std::string("error ") +
                                  std::string(__func__) + std::string("\n");
                return expected::makeError<cUchar, std::string>(ret.c_str());
            } else {
                return expected::makeValue<cUchar>(buffer);
            }
        };
    }

    template <typename T>
    const auto getTagPosFromEncodeByte(const cUchar& buff, uint32_t tagOffset,
                                       uint32_t frameContentOffset,
                                       uint32_t FrameSize) {
        switch (buff[frameContentOffset]) {
        case 0x0: {
                const T frameConfig{
                    tagOffset, frameContentOffset + 1, FrameSize - 1,
                };
                return expected::makeValue<TagInfos>(frameConfig);
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
                    tagOffset, frameContentOffset + 3, FrameSize - 3,
                    0x01 /* doEncode */
                    ,
                    doSwap /* doSwap */
                };
                return expected::makeValue<TagInfos>(frameConfig);
                break;
            }
            case 0x2: { /* UTF-16BE [UTF-16] encoded Unicode [UNICODE] without
                           BOM */
                const T frameConfig{tagOffset, frameContentOffset + 1, FrameSize - 1, 0x02};
                return expected::makeValue<TagInfos>(frameConfig);
                break;
            }
            case 0x3: { /* UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated
                           with $00 */
                const T frameConfig{tagOffset, frameContentOffset + 3, FrameSize - 3,
                               0x03};
                return expected::makeValue<TagInfos>(frameConfig);
                break;
            }
            default: {
                T frameConfig{tagOffset, frameContentOffset+1, FrameSize-1};
                return expected::makeValue<TagInfos>(frameConfig);
                break;
            }
        }
    }

    template <typename T>
    std::optional<cUchar> prepareTagToWrite(T content, const TagInfos& frameConfig,
                                            cUchar& buffer) {
        assert(frameConfig.getLength() >= content.size());

        const auto PositionTagStart = frameConfig.getTagContentOffset();

        id3::log()->info(" PositionTagStart: {}", PositionTagStart);

        assert(PositionTagStart > 0);

        const auto iter = std::begin(buffer) + PositionTagStart;
        std::transform(iter, iter + content.size(), content.begin(), iter,
                       [](char a, char b) { return b; });

        std::transform(iter + content.size(), iter + frameConfig.getLength(),
                       content.begin(), iter + content.size(),
                       [](char a, char b) { return 0x00; });

        return buffer;
    }

    std::string prepareTagContent(std::string_view content,
                                  const TagInfos& frameConfig) {
        std::variant<std::string_view, std::u16string, std::u32string>
            varEncode;

        if (frameConfig.getEncodingValue() == 0x01) {
            varEncode = std::u16string();
        } else if (frameConfig.getEncodingValue() == 0x02) {  // unicode without BOM
            varEncode = std::string_view("");
        } else if (frameConfig.getEncodingValue() == 0x03) {  // UTF8 unicode
            varEncode = std::u32string();
        }

        return std::visit(
            overloaded{

                [&](std::string_view arg) { return std::string(content); },

                [&](std::u16string arg) {

                    if (frameConfig.getSwapValue() == 0x01) {
                        std::string_view val =
                            tagBase::getW16StringFromLatin<cUchar>(content);
                        const auto ret = tagBase::swapW16String(val);
                        return ret;
                    } else {
                        const auto ret =
                            tagBase::getW16StringFromLatin<cUchar>(content);
                        return ret;
                    }
                },

                [&](std::u32string arg) {
                    const auto ret = std::string(content) + '\0';
                    return ret;
                },

            },
            varEncode);
    }

    std::optional<cUchar> prepareBufferWithNewTagContent(
        cUchar& buff, const std::string& content, const TagInfos& frameConfig) {

        assert(frameConfig.getLength() >= content.size());
        return prepareTagToWrite<std::string_view>(content, frameConfig, buff);
    }

    struct newTagArea {
    public:
        explicit newTagArea(const std::string& filename, const TagInfos& frameConfig,
                            const iD3Variant& tagVersion,
                            uint32_t additionalSize)
            : FileName(filename) {
            std::call_once(m_once, [&filename, &frameConfig, &tagVersion,
                                    additionalSize, this]() {
                auto ret =
                    GetTagHeader(filename) | GetTagSize | [&](uint32_t tags_size) {
                        return GetStringFromFile(
                            filename, tags_size + GetTagHeaderSize<uint32_t>());
                    };
                if (ret.has_value()) {

                    ID3_LOG_TRACE("Buffer could be read - prepare to extend.");
                    mCBuffer = ret.value();

                    auto ret1 =
                        extendBuffer(frameConfig, additionalSize, tagVersion) |
                        [&](const cUchar& buffer) {
                            mCBuffer = buffer;
                            return id3::constructNewTagInfos<TagInfos>(
                                frameConfig, additionalSize);
                        } |
                        [&](const TagInfos& tagloc) {
                            mTagLoc = tagloc;
                            mAdditionalSize = additionalSize;

                            return expected::makeValue<bool>(true);
                        };
                    assert(ret1.has_value());
                };
            });
        }

        expected::Result<bool> writeFile(const std::string& content) {
            return (mTagLoc | [&](const TagInfos& NewTagLoc) {

                const auto ret = prepareBufferWithNewTagContent(
                                     mCBuffer.value(), content, NewTagLoc) |
                                 [&](const cUchar& buff) {
                                     return this->ReWriteFile(mAdditionalSize);
                                 };

                return ret;
            });
        }

    private:
        std::optional<cUchar> mCBuffer;
        std::optional<TagInfos> mTagLoc;
        uint32_t mAdditionalSize;
        std::once_flag m_once;
        const std::string& FileName;

        const expected::Result<cUchar> extendBuffer(
            const TagInfos& frameConfig, uint32_t additionalSize,
            const iD3Variant& tagVersion) {
            ID3_LOG_TRACE("entering extendBuffer func...");
            constexpr uint32_t tagsSizePositionInHeader = 6;
            constexpr uint32_t frameSizePositionInFrameHeader = 4;

            const auto ret = mCBuffer | [&](const cUchar& cBuffer) {

                ID3_LOG_TRACE("FrameSize... length: {}, tag start: {}",
                              frameConfig.getLength(),
                              frameConfig.getFrameKeyOffset());
                ID3_LOG_TRACE("Updating segments...");

                const auto tagSizeBuffer =
                    updateTagSize(cBuffer, additionalSize);

                const auto frameSizeBuff =
                    updateFrameSizeIndex<const cUchar&, uint32_t, uint32_t>(
                        tagVersion, cBuffer, additionalSize,
                        frameConfig.getFrameKeyOffset());

                assert(frameConfig.getFrameKeyOffset() + frameConfig.getLength() <
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

                auto it = finalBuffer.begin() + frameConfig.getFrameKeyOffset() +
                          frameConfig.getLength();
                finalBuffer.insert(it, additionalSize, 0);

                return expected::makeValue<cUchar>(finalBuffer);
            };

            return ret;
        }

        expected::Result<bool> ReWriteFile(uint32_t extraSize) {
            std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);

            if (!filRead.good()) {
                return expected::makeError<bool>() << "__func__"
                                                   << ": Error opening file";
            }

            return mCBuffer | [&](const cUchar& buff) {
                const auto _headerAndTagsSize = GetTotalTagSize(buff);

                return _headerAndTagsSize | [&](uint32_t headerAndTagsSize) {

                    ID3_LOG_TRACE("headerAndtagsize: {}", headerAndTagsSize);

                    const uint32_t tagsSizeOld = headerAndTagsSize - extraSize;

                    uint32_t dataSize = filRead.tellg();
                    assert(dataSize > tagsSizeOld);
                    dataSize -= tagsSizeOld;

                    filRead.seekg(tagsSizeOld);
                    cUchar bufRead;
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
        explicit TagReadWriter(const std::string& filename)
            : FileName(filename) {

            std::call_once(m_once, [this]() {

                const auto ret =
                    GetTagHeader(FileName) | GetTagSize | [&](uint32_t tags_size) {
                        return GetStringFromFile(
                            FileName, tags_size + GetTagHeaderSize<uint32_t>());
                    };
                if (ret.has_value()) {
                    buffer = ret.value();
                }
            });
        }

        expected::Result<bool> ReWriteFile(const cUchar& cBuffer) {
            std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);
            std::ofstream filWrite(FileName + id3::modifiedEnding,
                                   std::ios::binary | std::ios::app);

            if (!filRead.good()) {
                return expected::makeError<bool>() << "__func__"
                                                   << ": Error opening file";
            }
            if (!filWrite.good()) {
                return expected::makeError<bool>() << "__func__"
                                                   << ": Error opening file";
            }


            const unsigned int dataSize = filRead.tellg();
            filRead.seekg(0);

            cUchar bufRead;
            bufRead.reserve(dataSize);
            filRead.read(reinterpret_cast<char*>(&bufRead[0]), dataSize);

            assert(cBuffer.size() < dataSize);

            std::for_each(std::begin(cBuffer), std::end(cBuffer),
                          [&filWrite](const char& n) { filWrite << n; });

            std::for_each(bufRead.begin() + cBuffer.size(),
                          bufRead.begin() + dataSize,
                          [&filWrite](const char& n) { filWrite << n; });

            return expected::makeValue<bool>(true);
        }

        const expected::Result<TagInfos> findFrameInfos(
            std::string_view tag, const iD3Variant& TagVersion) {
            uint32_t FrameSize = 0;
            uint32_t FramePos = 0;

            const auto ret =
                buffer |
                [](const cUchar& buffer) -> expected::Result<std::string>

            {
                return GetTagArea(buffer);
            } | [&tag](const std::string&
                           tagArea) -> expected::Result<uint32_t> {

                auto searchTagPosition = id3::search_tag<std::string_view>(tagArea);
                const auto ret = searchTagPosition(tag);

                ID3_LOG_TRACE("tag name: #{}", std::string(tag));

                return ret;
            } | [&](uint32_t tagIndex) -> expected::Result<TagInfos> {

                if (tagIndex == 0){
                    return expected::makeError<TagInfos>("framePosition = 0");
                }

                assert(tagIndex >= GetTagHeaderSize<uint32_t>());

                FramePos = tagIndex;

                ID3_LOG_TRACE("frame position: {}", tagIndex);

                return buffer | [&tagIndex, &TagVersion](const cUchar& buff) {

                    return GetFrameSize<uint32_t>(buff, TagVersion, tagIndex);
                } | [&](uint32_t frameSize) -> expected::Result<TagInfos> {

                    const uint32_t frameContentOffset =
                        tagIndex + GetFrameHeaderSize(TagVersion);
                    FrameSize = frameSize;

                    ID3_LOG_TRACE("frame content offset: {}", frameContentOffset);
                    assert(frameContentOffset > 0);

                    return buffer | [&](const cUchar& buff) {
                        if (tag.find_first_of("T") ==
                            0)  // if tag starts with T
                        {
                            return getTagPosFromEncodeByte<TagInfos>(
                                buff, FramePos, frameContentOffset, FrameSize);
                        }

                        ID3_LOG_TRACE("tag starts with T: {}", tag);
                        const auto ret1 =
                            TagInfos(FramePos, frameContentOffset, FrameSize);

                        return expected::makeValue<TagInfos>(ret1);
                    };
                };
            };

            ID3_LOG_INFO("return val: {}", ret.has_value());

            return ret;
        }

        template <typename V>
        const expected::Result<V> extractTag(std::string_view tag,
                                             const iD3Variant& TagVersion) {
            const auto res = buffer | [=](const cUchar& buff) {
                return findFrameInfos(tag, TagVersion) |
                       [&](const TagInfos& FrameConfig) {
                           ID3_LOG_INFO("tag content offset: {}, framesize {}",
                                        FrameConfig.getTagContentOffset(),
                                        FrameConfig.getLength());

                           if (FrameConfig.getLength() == 0) {
                               ID3_LOG_ERROR(
                                   "extractTag: Error: frame length = {}",
                                   FrameConfig.getLength());
                               return expected::makeError<V>(
                                   "extractTag: Error: frame length = 0");
                           }
                           const auto val = ExtractString<uint32_t>(
                               buff, FrameConfig.getTagContentOffset(),
                                   FrameConfig.getLength());

                           return val | [&FrameConfig](
                                            const std::string& val_str) {

                               if (FrameConfig.getSwapValue() == 0x01) {
                                   const auto cont = tagBase::swapW16String(
                                       std::string_view(val_str));
                                   return expected::makeValue<V>(cont);
                               } else {
                                   return expected::makeValue<V>(val_str);
                               }
                           };
                       };
            };

            return res;
        }

    private:
        mutable std::once_flag m_once;
        const std::string& FileName;
        std::optional<cUchar> buffer;
    };

    template <typename id3Type>
    const auto is_tag(std::string name) {
        return (std::any_of(id3v2::GetTagNames<id3Type>().cbegin(),
                            id3v2::GetTagNames<id3Type>().cend(),
                            [&](std::string obj) { return name == obj; }));
    }

    template <typename T, typename V>
    expected::Result<bool> setTag(const std::string& filename,
                                  const iD3Variant& tagVersion, T content,
                                  V tagName) {
        id3v2::TagReadWriter<std::string_view> obj(filename);
        expected::Result<id3::TagInfos> locs =
            obj.findFrameInfos(tagName, tagVersion);

        if (locs.has_value()) {
            const id3::TagInfos& frameConfig = locs.value();
            const std::string tag_str =
                prepareTagContent(content, frameConfig);

            ID3_LOG_TRACE("Content write: {} prepared", tag_str);
            ID3_LOG_TRACE("tag content offset: {}",
                         frameConfig.getTagContentOffset());
            ID3_LOG_TRACE("tag area length: {} ", frameConfig.getLength());
            if (frameConfig.getLength() == 0) {
                ID3_LOG_ERROR("findFrameInfos: Error, framesize = 0\n");
                return expected::makeError<bool>(
                    "findFrameInfos: Error, framesize = 0\n");
            } else if (frameConfig.getLength() <
                       tag_str.size())  // resize whole header
            {
                const uint32_t extraLength =
                    (tag_str.size() - frameConfig.getLength());
                struct id3v2::newTagArea NewTagArea {
                    filename, frameConfig, tagVersion, extraLength
                };

                return (NewTagArea.writeFile(tag_str) | [&](const bool& val) {
                    return renameFile(filename + id3::modifiedEnding, filename);
                });
            } else {
                return id3::WriteFile(filename, tag_str, frameConfig);
            }
        } else {
            return expected::makeError<bool>(
                "findFrameInfos: tag could not be located\n");
        }
    }

}; //end namespace id3v2


#endif //_ID3V2_COMMON
