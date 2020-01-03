#ifndef _ID3V2_COMMON
#define _ID3V2_COMMON

#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <cmath>
#include <variant>
#include <type_traits>
#include <id3v2_v30.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v00.hpp>
#include <string_conversion.hpp>



template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace id3v2
{
    using iD3Variant = std::variant <id3v2::v00, id3v2::v30, id3v2::v40>;


    template <typename T>
        std::optional<T> GetFrameSize(const std::vector<char>& buff, 
                const iD3Variant& TagVersion, uint32_t TagIndex)
        {
            return std::visit(overloaded {
                    [&](id3v2::v30 arg) {
                    return arg.GetFrameSize(buff, TagIndex);
                    },
                    [&](id3v2::v40 arg) {
                    return arg.GetFrameSize(buff, TagIndex);
                    },
                    [&](id3v2::v00 arg) {
                    return arg.GetFrameSize(buff, TagIndex);
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

    std::optional<std::vector<char>> check_for_ID3(const std::vector<char>& buffer)
    {
        auto tag = id3v2::GetID3FileIdentifier(buffer);
        if (tag != "ID3") {
            std::cerr << "error " << __func__ << std::endl;
            return {};
        }
        else {
            return buffer;
        }
    }
    template <typename T>
        T getTagPosFromEncodeByte(const std::vector<char>& buff, uint32_t tagOffset, uint32_t FrameSize)
        {
            switch(buff[tagOffset]){
                case 0x1:{
                             uint32_t doSwap = 0;
                             const uint16_t encodeOder = (buff[tagOffset + 1] << 8 | buff[tagOffset + 2]);
                             if(encodeOder == 0xfeff){
                                     doSwap = 1;
                             }

                             const T tagLoc(tagOffset + 3
                                     , FrameSize - 3
                                     , 0x01 /* doEncode */
                                     , doSwap /* doSwap */
                                     );
                             return tagLoc;
                             break;
                         }
                case 0x2:{
                             T tagLoc(tagOffset, FrameSize);
                             tagLoc.doEncode(0x02);
                             return tagLoc;
                             break;
                         }
                case 0x3:{
                             T tagLoc(tagOffset + 3, FrameSize - 3);
                             tagLoc.doEncode(0x03);
                             return tagLoc;
                             break;
                         }
                default:{
                            T tagLoc(tagOffset, FrameSize);
                            tagLoc.doEncode(0x0);
                            return tagLoc;
                            break;
                        }
            }

        }


    template <typename type>
        class TagReadWriter
        {
            public:

                explicit TagReadWriter(const std::string& filename)
                    : FileName(filename)
                {
                    buffer = GetHeader(FileName)
                        | GetTagSize
                        | [&](uint32_t tags_size)
                        {
                            return GetStringFromFile(FileName, tags_size + GetHeaderSize<uint32_t>());
                        };
                }

                std::optional<bool> ReWriteFile(const std::vector<char>& cBuffer)
                {
                    std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);
                    std::ofstream filWrite(FileName + ".mod", std::ios::binary | std::ios::app);

                    if(!filRead.good())
                    {
                        std::cerr << "Error reading file: " << FileName << std::endl;
                        return {};
                    }

                    std::vector<char> bufRead;

                    const unsigned int dataSize = filRead.tellg();

                    filRead.seekg(0);
                    bufRead.reserve(dataSize);
                    filRead.read(&bufRead[0], dataSize);

                    assert(cBuffer.size() <  dataSize);

                    std::for_each(std::begin(cBuffer), std::end(cBuffer), [&filWrite](const char& n)
                            {
                            filWrite << n;
                            });

                    std::for_each(bufRead.begin() + cBuffer.size(), bufRead.begin() + dataSize, [&filWrite](const char& n)
                            {
                            filWrite << n;
                            });

                    return true;
                }

                std::optional<std::vector<char>> SetTag(std::string_view content, const TagInfos& tagLoc)
                {
                    assert (tagLoc.getLength() >= content.size());

                    std::variant<std::string_view, std::u16string, std::string> varEncode;

                    if(tagLoc.getEncodingValue() == 0x01){
                        varEncode = std::u16string();
                    }else if (tagLoc.getEncodingValue() == 0x02){ //unicode without BOM
                        varEncode = std::string_view("");
                    }else if (tagLoc.getEncodingValue() == 0x03){ //UTF8 unicode
                        varEncode = std::string("");
                    }

                    return std::visit(overloaded {

                            [&](std::string_view arg) {
                            return prepareTagToWrite<std::string_view>(content, tagLoc);
                            },

                            [&](std::u16string arg) {
                            std::string_view wcont = tagBase::getW16StringFromLatin<std::vector<char>>(content);

                            return prepareTagToWrite<std::string_view>(wcont, tagLoc);
                            },

                            [&](std::string arg) {
                            std::string_view wcont = std::string(content) + '\0';

                            return prepareTagToWrite<std::string_view>(wcont, tagLoc);
                            },

                            }, varEncode);
                }

                template <typename T>
                    std::optional<std::vector<char>> prepareTagToWrite(T content, const TagInfos& tagLoc)
                    {
                        assert(tagLoc.getLength() >= content.size());

                        const auto res = buffer
                            | [&](std::vector<char>& buffer)
                            {
                                const auto PositionTagStart = tagLoc.getStartPos();

#ifdef DEBUG
                                std::cout << "PositionTagStart: " << PositionTagStart << std::endl;
                                std::cout << "Content size: " << content.size() << std::endl;
                                std::cout << "prepare tag content: ";

                                const auto uffer = reinterpret_cast<const char*>(content.data());
                                std::for_each(content.begin(), content.end(), [](char uffer)
                                        { std::cout << std::hex << (int)uffer<< ' ';}
                                        );
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
                            };

                        return res;
                    }

                const std::optional<TagInfos> findTagInfos(std::string_view tag, const iD3Variant& TagVersion)
                {
                    uint32_t FrameSize = 0;

                    const auto ret = buffer
                        | [](const std::vector<char>& buffer)
                        {
                            return GetTagArea(buffer);
                        }
                    | [&tag](const std::string& tagArea)
                    {
                        auto searchTagPosition = search_tag<std::string_view>(tagArea);
#ifdef DEBUG
                        std::cout << "tag name " << tag << std::endl;
#endif
                        return searchTagPosition(tag);
                    }
                    | [&](uint32_t TagIndex)
                    {
                        assert(TagIndex >= GetHeaderSize<uint32_t>());
#ifdef DEBUG
                        std::cout << "tag index " << TagIndex << std::endl;
#endif
                        return buffer
                            | [&TagIndex, &TagVersion](const std::vector<char>& buff)
                            {
                                return GetFrameSize<uint32_t>(buff, TagVersion, TagIndex);
                            }
                        | [&](uint32_t frameSize)
                        {
                            const uint32_t tagOffset = TagIndex + GetFrameHeaderSize(TagVersion);
                            FrameSize = frameSize;

                            assert(tagOffset > 0);

                            return buffer
                                | [&](const std::vector<char>& buff)
                                {
                                    if(tag.find_first_of("T") == 0) // if tag starts with T
                                    {
                                        return getTagPosFromEncodeByte<TagInfos>(buff, tagOffset, FrameSize);
                                    }

                                    return TagInfos(tagOffset, FrameSize);
                                };
                        };
                    };

                    return ret;
                }

                const std::optional<type> extractTag(std::string_view tag, const iD3Variant& TagVersion)
                {
                    const std::optional<TagInfos> tagLoc = findTagInfos(tag, TagVersion);
#if 0
                    if (tagLoc.has_value())
                        std::cout << "eract tag index " << tagLoc.value().getStartPos() << std::endl;
#endif
                    const auto res = buffer
                        | [=](const std::vector<char>& buff)
                        {
                            return tagLoc
                                | [&](const TagInfos& TagLoc)
                                {
                                    return ExtractString<uint32_t, uint32_t>(buff, TagLoc.getStartPos(),
                                            TagLoc.getStartPos() + TagLoc.getLength());
                                };
                        };
                    return res;
                }


            private:
                //  std::once_flag m_once;
                const std::string& FileName;
                std::optional<std::vector<char>> buffer;
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
