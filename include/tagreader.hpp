#ifndef _TAG_READER
#define _TAG_READER

#include <id3v2_common.hpp>
#include <id3v2_230.hpp>
#include <id3v2_000.hpp>


template <typename type>
class RetrieveTag
{
    using iD3Variant = std::variant <id3v2::v230, id3v2::v00>;
    std::string FileName;

    public:

    RetrieveTag(std::string filename)
    {
//  std::call_once(m_once, [&filename, this]
            {
            using namespace id3v2;

            FileName = filename;
            const auto tags_size = GetHeader(FileName) | GetTagSize;

            buffer = GetHeader(FileName)
            | GetTagSize
            | [&](uint32_t tags_size)
            {
            return GetStringFromFile(FileName, tags_size + GetHeaderSize<uint32_t>());
            };
            }
//          });
    }

    const std::optional<std::string> find_tag(const type& tag, const iD3Variant& TagVersion)
    {
        using namespace id3v2;
        using namespace ranges;

        auto res = buffer
            | [](const std::vector<char>& buffer)
            { 
                return GetTagArea(buffer);
            }
            | [&tag](const std::string& tagArea)
            { 

                auto searchTagPosition = search_tag<std::string>(tagArea);
                return searchTagPosition(tag);
            }
            | [&](uint32_t TagIndex)
            {
                return buffer 
                | [&TagIndex, &TagVersion](const std::vector<char>& buff)
                {
                   return GetFrameSize(buff, TagVersion, TagIndex);
                }
                | [&](uint32_t FrameSize)
                {
                    uint32_t tag_offset = TagIndex + GetFrameHeaderSize(TagVersion);
                    //std::cout << "tag_offset " << tag_offset << std::endl;
                    //
                    return buffer
                        | [&](const std::vector<char>& buff)
                        {
                            return GetArea(buff, tag_offset, FrameSize);
                        };
                };
            };

        return res;
    }

    private:
    //  std::once_flag m_once;
        std::optional<std::vector<char>> buffer;
};

template <typename E>
const std::optional<E> GetTag(const std::variant <id3v2::v230, id3v2::v00> &tagversion,
     const std::string filename,
    const std::string tagname)
{
        RetrieveTag<E> obj(filename);

        return obj.find_tag (tagname, tagversion);
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

const std::optional<std::string> GetAlbum(const std::string filename)
{
    using iD3Variant = std::variant <id3v2::v230, id3v2::v00>;
    iD3Variant _tagversion;

    return id3v2::GetHeader(filename)
    | check_for_ID3
    | [](const std::vector<char>& buffer)
    {
        return id3v2::GetID3Version(buffer);
    }
    | [&](const std::string& id3Version) {
        if (id3Version == "0x0300")
        {
            iD3Variant tagversion = std::get<id3v2::v230> (_tagversion);
            return GetTag<std::string>(tagversion, filename, "TALB");
        }
        else if (id3Version == "0x0000")
        {
            iD3Variant tagversion = std::get<id3v2::v00> (_tagversion);
            return GetTag<std::string>(tagversion, filename, "TAL");
        }
    };
}

template <typename id3Type>
const auto is_tag(std::string name)
{
    return (
        std::any_of(id3v2::GetTagNames<id3Type>().cbegin(), id3v2::GetTagNames<id3Type>().cend(), 
        [&](std::string obj) { return name == obj; } ) 
        );
}


#endif //_TAG_READER
