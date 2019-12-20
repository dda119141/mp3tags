#ifndef _TAG_WRITER
#define _TAG_WRITER

#include <id3v2_common.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v00.hpp>
#include <optional>

using iD3Variant = std::variant <id3v2::v30, id3v2::v00, id3v2::v40>;

bool SetTag(const std::string& filename,
        const std::vector<std::pair<std::string, std::string>>& tags, const std::string& content)
{
    const auto ret =
        id3v2::GetHeader(filename)
        | id3v2::check_for_ID3
        | [](const std::vector<char>& buffer)
        {
            return id3v2::GetID3Version(buffer);
        }
        | [&](const std::string& id3Version) {
            iD3Variant DefaultTagVersion, tagVersion;

            for (auto& tag: tags)
            {
                if (id3Version == tag.first) //tag.first is the id3 Version
                {
                    if(id3Version == "0x0300"){
                        tagVersion = std::get<id3v2::v30>(DefaultTagVersion);
                    }
                    else if(id3Version == "0x0400"){
                        tagVersion = std::get<id3v2::v40>(DefaultTagVersion);
                    }
                    else if(id3Version == "0x0000"){
                        tagVersion = std::get<id3v2::v00>(DefaultTagVersion);
                    }
                    else{
                        std::cerr << "version not supported" << std::endl;
                        return std::optional<bool>(false);
                    }

                    id3v2::TagReadWriter<std::string> obj(filename);

                    std::optional<id3v2::TagInfos> locs = obj.findTagInfos(tag.second, tagVersion);
                    if(locs.has_value())
                    {
                        const auto ret1 = obj.SetTag(content, locs.value());
                        return(
                            ret1 |
                            [&obj, &filename](uint32_t val)
                            {
                                return obj.ReWriteFile(filename);
                            });
                    }
                    else{
                        std::cerr << "Tag slot not found" << std::endl;
                        return std::optional<bool>(false);
                    }

                }
            }
    };

    if(ret.has_value())
        return ret.value();
    else
        return false;
}

bool SetAlbum(const std::string& filename, const std::string& content)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TALB"},
            {"0x0300", "TALB"},
            {"0x0000", "TAL"},
    };

    return  SetTag(filename, tags, content);
}

bool SetComposer(const std::string& filename, const std::string& content)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TCOM"},
        {"0x0300", "TCOM"},
            {"0x0000", "TCM"},
    };

    return  SetTag(filename, tags, content);
}

bool SetDate(const std::string& filename, const std::string& content)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TDAT"},
        {"0x0400", "TDRC"},
        {"0x0000", "TDA"},
    };

    return  SetTag(filename, tags, content);
}

#if 0
const std::string GetContentType(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TCON"},
        {"0x0300", "TCON"},
        {"0x0000", "TCO"},
    };

    return SetTag(filename, tags);
}

const std::string GetTextWriter(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TEXT"},
        {"0x0300", "TEXT"},
        {"0x0000", "TXT"},
    };

    return SetTag(filename, tags);
}

const std::string GetYear(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TDRC"},
        {"0x0300", "TYER"},
        {"0x0000", "TYE"},
    };

    return SetTag(filename, tags);
}

const std::string GetFileType(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0300", "TFLT"},
        {"0x0000", "TFT"},
    };

    return SetTag(filename, tags);
}

const std::string GetTitle(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TIT2"},
        {"0x0300", "TIT2"},
        {"0x0000", "TT2"},
    };

    return SetTag(filename, tags);
}

const std::string GetContentGroupDescription(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TIT1"},
        {"0x0300", "TIT1"},
        {"0x0000", "TT1"},
    };

    return SetTag(filename, tags);
}

const std::string GetTrackPosition(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TRCK"},
        {"0x0300", "TRCK"},
        {"0x0000", "TRK"},
    };

    return SetTag(filename, tags);
}

const std::string GetLeadArtist(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TPE1"},
        {"0x0300", "TPE1"},
        {"0x0000", "TP1"},
    };

    return SetTag(filename, tags);
}
#endif


#endif //_TAG_WRITER
