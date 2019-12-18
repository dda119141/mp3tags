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

    explicit RetrieveTag(std::string filename)
        : FileName(filename)
    {
//  std::call_once(m_once, [&filename, this]
            {
                using namespace id3v2;

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

template <typename T>
const std::optional<T> GetTag(const std::variant <id3v2::v230, id3v2::v00> &tagversion,
     const std::string filename,
    const std::string tagname)
{
        RetrieveTag<T> obj(filename);

        return obj.find_tag (tagname, tagversion);
}
#if 0
const std::optional<std::string> GetTagType(const std::string& filename, const std::vector<std::pair<std::string, std::string>>& tags)
{
    using iD3Variant = std::variant <id3v2::v230, id3v2::v00>;
    iD3Variant _tagversion;

    return (
            id3v2::ProcessID3Version(filename)
            | [&](const std::string& id3Version) {
            std::for_each (tags.begin(), tags.end(), [&] (std::pair<std::string, std::string> tag)
                    {
                    if (id3Version == tag.first) //tag.first is the id3 Version
                    {
                         iD3Variant tagversion = std::get<id3v2::v230> (_tagversion);
                        return GetTag<std::string>(tagversion, filename, tag.second);
                    }
                    });
            }
           );
}

#else
const std::optional<std::string> GetTagType(const std::string& filename, const std::vector<std::pair<std::string, std::string>>& tags)
{
    using iD3Variant = std::variant <id3v2::v230, id3v2::v00>;
    iD3Variant _tagversion;

    return 
            id3v2::GetHeader(filename)
            | id3v2::check_for_ID3
            | [](const std::vector<char>& buffer)
            {
                return id3v2::GetID3Version(buffer);
            }
            | [&](const std::string& id3Version) {
                for (auto tag: tags)
                {
                    if (id3Version == tag.first) //tag.first is the id3 Version
                    {
                        iD3Variant tagversion = std::get<id3v2::v230> (_tagversion);
                        return GetTag<std::string>(tagversion, filename, tag.second);
                    }
                }
            };
}


#endif
#if 0
const std::optional<std::string> GetAlbum(const std::string& filename)
{
    using iD3Variant = std::variant <id3v2::v230, id3v2::v00>;
    iD3Variant _tagversion;

    return 
        id3v2::GetHeader(filename)
        | id3v2::check_for_ID3
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
#else
const std::optional<std::string> GetAlbum(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TALB"},
        {"0x0000", "TAL"},
    };

    return GetTagType(filename, tags);
}
#endif

const std::optional<std::string> GetComposer(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TCOM"},
        {"0x0000", "TCM"},
    };

    return GetTagType(filename, tags);
}

const std::optional<std::string> GetDate(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TDAT"},
        {"0x0000", "TDA"},
    };

    return GetTagType(filename, tags);
}


#endif //_TAG_READER
