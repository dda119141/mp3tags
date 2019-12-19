#ifndef _TAG_READER
#define _TAG_READER

#include <id3v2_common.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v00.hpp>


template <typename type>
class RetrieveTag
{
    using iD3Variant = std::variant <id3v2::v30, id3v2::v00>;
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

    const std::optional<type> find_tag(const type& tag, const iD3Variant& TagVersion)
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
                auto searchTagPosition = search_tag<type>(tagArea);
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
                | [&](uint32_t FrameSize)
                {
                    uint32_t tag_offset = TagIndex + GetFrameHeaderSize(TagVersion);
#ifdef DEBUG
                    std::cout << "tag_offset " << tag_offset << std::endl;
#endif
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
const std::optional<T> GetTag(const std::variant <id3v2::v30, id3v2::v00> &tagversion,
     const std::string filename,
    const std::string tagname)
{
        RetrieveTag<T> obj(filename);

        return obj.find_tag(tagname, tagversion);
}
#if 0
const std::optional<std::string> GetTheTag(const std::string& filename, const std::vector<std::pair<std::string, std::string>>& tags)
{
    using iD3Variant = std::variant <id3v2::v30, id3v2::v00>;
    iD3Variant _tagversion;

    return (
            id3v2::ProcessID3Version(filename)
            | [&](const std::string& id3Version) {
            std::for_each (tags.begin(), tags.end(), [&] (std::pair<std::string, std::string> tag)
                    {
                    if (id3Version == tag.first) //tag.first is the id3 Version
                    {
                         iD3Variant tagversion = std::get<id3v2::v30> (_tagversion);
                        return GetTag<std::string>(tagversion, filename, tag.second);
                    }
                    });
            }
           );
}

#else
const std::string GetTheTag(const std::string& filename, const std::vector<std::pair<std::string, std::string>>& tags)
{
    using iD3Variant = std::variant <id3v2::v30, id3v2::v00>;
    iD3Variant _tagversion;

    const auto ret = 
            id3v2::GetHeader(filename)
            | id3v2::check_for_ID3
            | [](const std::vector<char>& buffer)
            {
                return id3v2::GetID3Version(buffer);
            }
            | [&](const std::string& id3Version) {
               // std::cout << "id3version: " << id3Version << std::endl;
                for (auto& tag: tags)
                {
                    if (id3Version == tag.first) //tag.first is the id3 Version
                    {
                        iD3Variant tagversion = std::get<id3v2::v30> (_tagversion);
                        return GetTag<std::string>(tagversion, filename, tag.second);
                    }
                }
            };

    if(!ret.has_value()){
        return "Tag not found";
    }else{

        auto val = ret.value();

        auto it = val.begin();
        while(it++ != val.end()){
            val.erase(remove_if(val.begin(), it, [](char c) { return !isprint(c); } ), it);
        }

        return val;
    }
}
#endif

const std::string GetAlbum(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TALB"},
            {"0x0000", "TAL"},
    };

    return  GetTheTag(filename, tags);
}

const std::string GetComposer(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TCOM"},
            {"0x0000", "TCM"},
    };

    return  GetTheTag(filename, tags);
}

const std::string GetDate(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TDAT"},
        {"0x0400", "TDRC"},
        {"0x0000", "TDA"},
    };

    return GetTheTag(filename, tags);
}

const std::string GetContentType(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TCON"},
        {"0x0300", "TCON"},
        {"0x0000", "TCO"},
    };

    return GetTheTag(filename, tags);
}

const std::string GetTextWriter(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TEXT"},
        {"0x0000", "TXT"},
    };

    return GetTheTag(filename, tags);
}

const std::string GetYear(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TDRC"},
        {"0x0300", "TYER"},
        {"0x0000", "TYE"},
    };

    return GetTheTag(filename, tags);
}

const std::string GetFileType(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TFLT"},
        {"0x0000", "TFT"},
    };

    return GetTheTag(filename, tags);
}

const std::string GetTitle(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TIT2"},
        {"0x0300", "TIT2"},
        {"0x0000", "TT2"},
    };

    return GetTheTag(filename, tags);
}

const std::string GetContentGroupDescription(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TIT1"},
        {"0x0300", "TIT1"},
        {"0x0000", "TT1"},
    };

    return GetTheTag(filename, tags);
}




#endif //_TAG_READER
