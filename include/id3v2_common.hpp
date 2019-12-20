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



template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace id3v2
{
    using iD3Variant = std::variant <id3v2::v30, id3v2::v00, id3v2::v40>;


    std::optional<uint32_t> GetFrameSize(const std::vector<char>& buff, 
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

    template <typename type>
        class TagReadWriter
        {
            public:

            explicit TagReadWriter(const std::string& filename)
                : FileName(filename)
            {
                const auto tags_size = GetHeader(FileName) | GetTagSize;

                buffer = GetHeader(FileName)
                    | GetTagSize
                    | [&](uint32_t tags_size)
                    {
                        return GetStringFromFile(FileName, tags_size + GetHeaderSize<uint32_t>());
                    };
            }

            std::optional<bool> ReWriteFile(const std::string& filename)
            {
                std::ifstream filRead(filename, std::ios::binary | std::ios::ate);
                std::ofstream filWrite(filename + ".mod");

                if(!filRead.good())
                {
                    return {};
                }

                assert(buffer.has_value());

                std::vector<char> bufRead;

                const auto dataSize = filRead.tellg();
                filRead.seekg(0);
                filRead.read(&bufRead[0], dataSize);

                std::for_each(std::begin(buffer.value()), std::end(buffer.value()), [&filWrite](const char& n)
                        {
                            filWrite << n;
                        });

                std::for_each(std::begin(bufRead) + buffer.value().size(), std::end(bufRead), [&filWrite](const char& n)
                        {
                            filWrite << n;
                        });

                return true;
            }

            std::optional<bool> SetTag(const std::string& content, const TagInfos& tagLoc)
            {
                assert (content.size() > tagLoc.getLength());

                const auto res = buffer
                    | [&](std::vector<char>& buffer)
                    {
                       uint32_t index = 0;
                        auto start = tagLoc.getStartPos();

                        assert(start > 0);

                        const auto iter = std::begin(buffer) + tagLoc.getStartPos();
                        std::transform(iter, iter + content.size(), content.begin(), iter,
                                [](char a, char b){ return b;}
                                );

                        std::transform(iter + content.size(), iter + tagLoc.getLength(), content.begin(), iter,
                                [](char a, char b){ return 0x00;}
                                );

                        return true;
                    };

                return res;
#if 0
                        const auto start = buffer.begin() + tagLoc.getStartPos();
                        return std::string(start, start + tagLoc.getLength()).replace(0, tagLoc.getLength(), content);
                        while(start++ < tagLoc.getLength()){
                            if(index < content.size())
                                buffer[start] = content[index++];
                            else
                                buffer[start] = 0x00;
                        }

#endif
            }

            const std::optional<TagInfos> findTagInfos(std::string_view tag, const iD3Variant& TagVersion)
            {
                uint32_t FrameSize = 0;

                const auto Offset = buffer
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
#ifdef DEBUG
                    std::cout << "tag index " << TagIndex << std::endl;
#endif
                    return buffer
                        | [&TagIndex, &TagVersion](const std::vector<char>& buff)
                        {
                            return GetFrameSize(buff, TagVersion, TagIndex);
                        }
                    | [&](uint32_t frameSize)
                    {
                        const uint32_t tagOffset = TagIndex + GetFrameHeaderSize(TagVersion);
                        FrameSize = frameSize;
#ifdef DEBUG
                        std::cout << "tagOffset " << tagOffset << std::endl;
#endif
                        return tagOffset;
                    };
                };

                if(Offset > 0)
                    return TagInfos(Offset, FrameSize);
                else
                    return {};
            }

            const std::optional<type> extractTag(std::string_view tag, const iD3Variant& TagVersion)
            {
                const std::optional<TagInfos> tagLoc = findTagInfos(tag, TagVersion);

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
