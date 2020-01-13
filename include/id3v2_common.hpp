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
                const iD3Variant& TagVersion, uint32_t tagIndex)
        {
            return std::visit(overloaded {
                    [&](id3v2::v30 arg) {
                    return arg.GetFrameSize(buff, tagIndex);
                    },
                    [&](id3v2::v40 arg) {
                    return arg.GetFrameSize(buff, tagIndex);
                    },
                    [&](id3v2::v00 arg) {
                    return arg.GetFrameSize(buff, tagIndex);
                    }
                    }, TagVersion);
        }

    uint32_t GetFrameHeaderSize(const iD3Variant& TagVersion)
    {
        return std::visit(overloaded {
                [&](id3v2::v30 arg) {
                return arg.FrameHeaderSize();
                },
                [&](id3v2::v40 arg) {
                return arg.FrameHeaderSize();
                },
                [&](id3v2::v00 arg) {
                return arg.FrameHeaderSize();
                }
                }, TagVersion);
    }

    expected::Result<bool> UpdateFrameSize(UCharVec& buffer, uint32_t extraSize, uint32_t tagLocation)
        {
            const uint32_t tagIndex = 4 + tagLocation;
            constexpr uint32_t NumberOfElements = 4;
            constexpr uint32_t maxValue = 127;
            auto it = std::begin(buffer) + tagIndex;
            auto ExtraSize = extraSize;
            auto extr = ExtraSize % 127;

            std::cout << " Tag Index: " << tagIndex << "\n";
            for(int i = 0; i < 5; i++)
                std::cout << std::hex << (int)*( std::begin(buffer) + tagIndex + i) << " ";
             std::cout << "\n";
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
                        ExtraSize = ((int)(ExtraSize - rest2) < 0)
                                        ? 0
                                        : (ExtraSize - rest2);
                    }
                    return a;
                });

            /* reverse back order of elements */
            std::reverse(it, it + NumberOfElements);

            for(int i = 0; i < 5; i++)
                std::cout <<std::hex << (int)*( std::begin(buffer) + tagIndex + i) << " ";
             std::cout << "\n";
            return expected::makeValue<bool>(true);

        }

        template <typename... Args>
        expected::Result<bool> updateFrameSizeIndex(
            const iD3Variant& TagVersion, Args... args) {
            return std::visit(
                overloaded{
                    [&](id3v2::v30 arg) { return UpdateFrameSize(args...); },
                    [&](id3v2::v40 arg) { return UpdateFrameSize(args...); },
                    [&](id3v2::v00 arg) {
                        return arg.UpdateFrameSize(args...);
                    }},
                TagVersion);
        }

        expected::Result<cUchar> check_for_ID3(const cUchar& buffer) {
            return id3v2::GetID3FileIdentifier(buffer) |
                       [&](const std::string& val) -> expected::Result<cUchar> {
                if (val != "ID3") {
                    std::string ret = std::string("error ") +
                                      std::string(__func__) + std::string("\n");
                    return expected::makeError<cUchar, std::string>(
                        ret.c_str());
                } else {
                    return expected::makeValue<cUchar>(buffer);
                }
            };
        }

        template <typename T>
        const auto getTagPosFromEncodeByte(const cUchar& buff,
                                           uint32_t tagOffset,
                                           uint32_t tagContentOffset,
                                           uint32_t FrameSize) {
            switch(buff[tagContentOffset]){
                case 0x1:{
                             uint32_t doSwap = 0;
                             const uint16_t encodeOder = (buff[tagContentOffset + 1] << 8 | buff[tagContentOffset + 2]);
                             if(encodeOder == 0xfeff){
                                 doSwap = 1;
                             }

                             const T tagLoc{tagOffset, tagContentOffset + 3
                                 , FrameSize - 3
                                     , 0x01 /* doEncode */
                                     , doSwap /* doSwap */
                             };
                             return expected::makeValue<TagInfos>(tagLoc);
                             break;
                         }
                case 0x2:{ /* UTF-16BE [UTF-16] encoded Unicode [UNICODE] without BOM */
                             const T tagLoc{tagOffset, tagContentOffset, FrameSize, 0x02};
                             return expected::makeValue<TagInfos>(tagLoc);
                             break;
                         }
                case 0x3:{ /* UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated with $00 */
                             const T tagLoc{tagOffset, tagContentOffset + 3, FrameSize - 3, 0x03};
                             return expected::makeValue<TagInfos>(tagLoc);
                             break;
                         }
                default:{
                            T tagLoc{tagOffset, tagContentOffset, FrameSize};
                            return expected::makeValue<TagInfos>(tagLoc);
                            break;
                        }
            }

        }

         template <typename T>
                    std::optional<cUchar> prepareTagToWrite(T content, const TagInfos& tagLoc, cUchar& buffer)
                    {
                        assert(tagLoc.getLength() >= content.size());

                                const auto PositionTagStart = tagLoc.getTagContentOffset();

#if 0
                                std::cout << "PositionTagStart: " << std::dec << PositionTagStart << std::endl;
                                std::cout << "Content size: " << std::dec <<  content.size() << std::endl;
                                std::cout << "prepare tag content: ";

                            auto uffer = reinterpret_cast<const char*>(content.data());
                            for(uint32_t i = 0; i<content.size(); i++)
                            { std::cout << std::hex << *(uffer++)<< ' ';}
                             std::cout <<  std::endl;
#endif
                                assert(PositionTagStart > 0);

                                const auto iter = std::begin(buffer) + PositionTagStart;
                                std::transform(iter, iter + content.size(), content.begin(), iter,
                                        [](char a, char b){ return b;}
                                        );

                                std::transform(iter + content.size(), iter + tagLoc.getLength(),
                                        content.begin(), iter + content.size(),
                                        [](char a, char b){ return 0x00;}
                                        );
                                return buffer;

                    }

       std::string prepareTagContent(std::string_view content, const TagInfos& tagLoc)
                {
                    std::variant<std::string_view, std::u16string, std::u32string> varEncode;

                    if(tagLoc.getEncodingValue() == 0x01){
                        varEncode = std::u16string();
                    }else if (tagLoc.getEncodingValue() == 0x02){ //unicode without BOM
                        varEncode = std::string_view("");
                    }else if (tagLoc.getEncodingValue() == 0x03){ //UTF8 unicode
                        varEncode = std::u32string();
                    }

                    return std::visit(overloaded {

                            [&](std::string_view arg) {
                            return std::string(content);
                            },

                            [&](std::u16string arg) {

                            if(tagLoc.getSwapValue() == 0x01){
                            std::string_view val = tagBase::getW16StringFromLatin<cUchar>(content);
                            const auto ret= tagBase::swapW16String(val);
                            return ret;
                            }else{
                            const auto ret = tagBase::getW16StringFromLatin<cUchar>(content);
                            return ret;
                            }
                            },

                            [&](std::u32string arg) {
                            const auto ret = std::string(content) + '\0';
                            return ret;
                            },

                    }, varEncode);
                }

        std::optional<cUchar> prepareBufferWithNewTagContent(cUchar &buff, const std::string& content, const TagInfos& tagLoc)
                {
                    assert (tagLoc.getLength() >= content.size());
//                    std::string_view tag_str = prepareTagContent(content, tagLoc);
                    return prepareTagToWrite<std::string_view>(content, tagLoc, buff);
                }


    struct newTagArea
    {
        public:
            explicit newTagArea(const std::string& filename, const TagInfos& tagLoc, const iD3Variant& tagVersion, uint32_t additionalSize)
                :FileName(filename)
            {
                std::call_once(m_once, [&filename, &tagLoc, &tagVersion,
                                        additionalSize, this]() {
                    auto ret = GetHeader(filename) | GetTagSize |
                               [&](uint32_t tags_size) {
                                   return GetStringFromFile(
                                       filename,
                                       tags_size + GetHeaderSize<uint32_t>());
                               };
                    if (ret.has_value()) {
                        mCBuffer = ret.value();

                        auto ret1 = extendBuffer(tagLoc, additionalSize,
                                                       tagVersion)
                        | [&](const cUchar& buffer) {
                            mCBuffer = buffer;
                            return constructNewTagInfos<TagInfos>(
                                tagLoc, additionalSize);
                        }
                        | [&](const TagInfos& tagloc) {
                           // std::cout << "rewriting file...\n";
                            mTagLoc = tagloc;
                            mAdditionalSize = additionalSize;

                            return expected::makeValue<bool>(true);
                        };
                        assert (ret1.has_value());
                    };
                });
            }

            expected::Result<bool> writeFile(const std::string& content) {
                return (mTagLoc | [&](const TagInfos& NewTagLoc) {

                    const auto ret =
                        prepareBufferWithNewTagContent(mCBuffer.value(),
                                                       content, NewTagLoc) |
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

            template <typename T>
            expected::Result<T> constructNewTagInfos(const T& tagLoc,
                                                     uint32_t newtagSize) {
                const T TagLoc{
                    tagLoc.getTagOffset(), tagLoc.getTagContentOffset(), tagLoc.getLength() + newtagSize,
                    tagLoc.getEncodingValue(), tagLoc.getSwapValue()};

                return expected::makeValue<T>(TagLoc);
            }

            expected::Result<cUchar> extendBuffer(
                const TagInfos& tagLoc, uint32_t additionalSize,
                const iD3Variant& tagVersion) {

                const auto ret = mCBuffer | [&](cUchar& cBuffer) {

                    updateTagSize(cBuffer, additionalSize);

                    updateFrameSizeIndex<cUchar&, uint32_t, uint32_t>(tagVersion, cBuffer, additionalSize, tagLoc.getTagOffset());
                    assert(tagLoc.getTagOffset() + tagLoc.getLength() <
                           cBuffer.size());

                    auto it = cBuffer.begin() + tagLoc.getTagOffset() +
                              tagLoc.getLength();

                    std::cout << "FrameSize... length: " <<
                     tagLoc.getLength() <<
                     " start: " << tagLoc.getTagOffset() << " additionalsize: "
                     <<
                     additionalSize << " content " << (int)*(cBuffer.begin() + tagLoc.getTagOffset() + 7) << "\n";
                    cBuffer.insert(it, additionalSize, 0);

                    return expected::makeValue<cUchar>(cBuffer);
                };

                return ret;
            }

            expected::Result<bool> ReWriteFile(uint32_t extraSize) {
                std::ifstream filRead(FileName,
                                      std::ios::binary | std::ios::ate);

                if (!filRead.good()) {
                    return expected::makeError<bool>()
                           << "__func__"
                           << ": Error opening file";
                }

                return mCBuffer | [&](cUchar& buff) {
                    auto _headerAndTagsSize = GetHeaderAndTagSize(buff);

                    return _headerAndTagsSize |
                           [&](uint32_t headerAndTagsSize) {

                                std::cout << "headerAndtagsize: " << headerAndTagsSize << "...\n";

                               const uint32_t tagsSizeOld =
                                   headerAndTagsSize - extraSize;

                               uint32_t dataSize = filRead.tellg();
                               assert(dataSize > tagsSizeOld);
                               dataSize -= tagsSizeOld;

                               filRead.seekg(tagsSizeOld);
                               cUchar bufRead;
                               bufRead.reserve(dataSize);
                               filRead.read(
                                   reinterpret_cast<char*>(&bufRead[0]),
                                   dataSize);

                               assert(buff.size() < dataSize);

                                std::cout << "datasize: " << dataSize << " buffer size: " << buff.size() << "...\n";
                               std::ofstream filWrite(
                                   FileName + ".mod", std::ios::binary | std::ios::app);

                               std::for_each(std::begin(buff), std::end(buff),
                                             [&filWrite](const char& n) {
                                                 filWrite << n;
                                             });

                               std::for_each(bufRead.begin(), bufRead.begin() + dataSize,
                                             [&filWrite](const char& n) {
                                                 filWrite << n;
                                             });

                  //              std::cout << "inserdfft done...\n";
                               return expected::makeValue<bool>(true);
                           };
                };
            };
    };

    template <typename type>
        class TagReadWriter
        {
            public:

                explicit TagReadWriter(const std::string& filename)
                    : FileName(filename)
                {
                    std::call_once(m_once, [this]()
                            {
#if 1
                            auto ret = GetHeader(FileName)
                            | GetTagSize
                            | [&](uint32_t tags_size)
                            {
                            return GetStringFromFile(FileName, tags_size + GetHeaderSize<uint32_t>());
                            };
                            if(ret.has_value())
                            {
                            buffer = ret.value();
                            }
#endif
                            });
                }


                expected::Result<bool> WriteFile(const std::string& content, const TagInfos& tagLoc)
                {
                    std::fstream filWrite(FileName, std::ios::binary | std::ios::in | std::ios::out);

                    if(!filWrite.is_open())
                    {
                        return expected::makeError<bool>() << __func__ << ": Error opening file\n";
                    }

                    filWrite.seekp(tagLoc.getTagContentOffset());

                    std::for_each(content.begin(), content.end(), [&filWrite](const char& n)
                            {
                            filWrite << n;
                            });

                    for (uint32_t i = 0; i < (tagLoc.getLength() - content.size()); ++i){
                        filWrite << '\0';
                    }

                    return expected::makeValue<bool>(true);
                }

                expected::Result<bool> ReWriteFile(const cUchar& cBuffer)
                {
                    std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);
                    std::ofstream filWrite(FileName + ".mod", std::ios::binary | std::ios::app);

                    if(!filRead.good())
                    {
                        return expected::makeError<bool>()<< "__func__" << ": Error opening file";
                    }

                    cUchar bufRead;

                    const unsigned int dataSize = filRead.tellg();

                    filRead.seekg(0);
                    bufRead.reserve(dataSize);
                    filRead.read(reinterpret_cast<char*>(&bufRead[0]), dataSize);

                    assert(cBuffer.size() <  dataSize);

                    std::for_each(std::begin(cBuffer), std::end(cBuffer), [&filWrite](const char& n)
                            {
                            filWrite << n;
                            });

                    std::for_each(bufRead.begin() + cBuffer.size(), bufRead.begin() + dataSize, [&filWrite](const char& n)
                            {
                            filWrite << n;
                            });

                    return expected::makeValue<bool>(true);
                }

                const expected::Result<TagInfos> findTagInfos(std::string_view tag, const iD3Variant& TagVersion)
                {
                    uint32_t FrameSize = 0;
                    uint32_t TagLocation = 0;

                    const auto ret = buffer
                        | [](const cUchar& buffer) -> expected::Result<std::string>

                        {
                            return GetTagArea(buffer);
                        }
                    | [&tag](const std::string& tagArea) -> expected::Result<uint32_t>
                    {
                        auto searchTagPosition = search_tag<std::string_view>(tagArea);
#ifdef DEBUG
                        std::cout << "tag name " << tag << std::endl;
#endif
                        const auto ret =  searchTagPosition(tag);

                        return ret;
                    }
                    | [&](uint32_t tagIndex) -> expected::Result<TagInfos>
                    {
                        if(tagIndex == 0)
                            return expected::makeError<TagInfos>("tagIndex = 0");

                        assert(tagIndex >= GetHeaderSize<uint32_t>());

                        TagLocation = tagIndex;
#ifdef DEBUG
                        std::cout << "tag index " << tagIndex << std::endl;
#endif
                        return buffer
                            | [&tagIndex, &TagVersion](const cUchar& buff)
                            {
                                return GetFrameSize<uint32_t>(buff, TagVersion, tagIndex);
                            }
                        | [&](uint32_t frameSize) -> expected::Result<TagInfos>
                        {
                            const uint32_t tagContentOffset = tagIndex + GetFrameHeaderSize(TagVersion);
                            FrameSize = frameSize;

#ifdef DEBUG
                            std::cout << "tag offset " << tagContentOffset << " FrameSize: " << FrameSize << std::endl;
#endif
                            assert(tagContentOffset > 0);

                            return buffer
                                | [&](const cUchar& buff)
                                {
                                    if(tag.find_first_of("T") == 0) // if tag starts with T
                                    {
                                        return getTagPosFromEncodeByte<TagInfos>(buff, TagLocation, tagContentOffset, FrameSize);
                                    }

                                    const auto ret1 = TagInfos(TagLocation, tagContentOffset, FrameSize);
                                    return expected::makeValue<TagInfos>(ret1);
                                };
                        };
                    };

                    if(ret.has_value()){
#ifdef DEBUG
                        std::cerr << "Ret has value!!: " << ret.value().getTagContentOffset() << "\n";
#endif
                    }

                    return ret;
                }

                const expected::Result<std::string> extractTag(std::string_view tag, const iD3Variant& TagVersion)
                {
                    const auto res = buffer
                        | [=](const cUchar& buff)
                        {
                            return findTagInfos(tag, TagVersion)
                                | [&](const TagInfos& TagLoc)
                                {
#ifdef DEBUG
                                    std::cout << "** tag offset " << TagLoc.getTagContentOffset() << " FrameSize: " << TagLoc.getLength() << std::endl;
#endif
                                    const auto val =  ExtractString<uint32_t, uint32_t>(buff, TagLoc.getTagContentOffset(),
                                            TagLoc.getTagContentOffset() + TagLoc.getLength());

                                    return val
                                        | [&TagLoc](const std::string& val_str) -> expected::Result<std::string>
                                        {
                                            if(TagLoc.getSwapValue() == 0x01){
                                                const auto cont = tagBase::swapW16String(std::string_view(val_str));
                                                return expected::makeValue<std::string>(cont);
                                            }else{
                                                return expected::makeValue<std::string>(val_str);
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
        const auto is_tag(std::string name)
        {
            return (
                    std::any_of(id3v2::GetTagNames<id3Type>().cbegin(), id3v2::GetTagNames<id3Type>().cend(), 
                        [&](std::string obj) { return name == obj; } ) 
                   );
        }

}; //end namespace id3v2




#endif //_ID3V2_COMMON
