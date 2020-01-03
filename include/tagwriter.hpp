#ifndef _TAG_WRITER
#define _TAG_WRITER

#include <id3v2_common.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v00.hpp>
#include <optional>
#include <string_conversion.hpp>


bool SetTag(const std::string& filename,
        const std::vector<std::pair<std::string, std::string_view>>& tags, std::string_view content)
{
    const auto ret =
        id3v2::GetHeader(filename)
        | id3v2::check_for_ID3
        | [](const std::vector<unsigned char>& buffer)
        {
            return id3v2::GetID3Version(buffer);
        }
        | [&](std::string id3Version) {
            id3v2::iD3Variant tagVersion;

            for (auto& tag: tags)
            {
                if (id3Version == tag.first) //tag.first is the id3 Version
                {
                    if(id3Version == "0x0300"){
                        tagVersion = id3v2::v30();
                    }
                    else if(id3Version == "0x0400"){
                        tagVersion = id3v2::v40();
                    }
                    else if(id3Version == "0x0000"){
                        tagVersion = id3v2::v00();
                    }
                    else{
                        std::cerr << "version not supported" << std::endl;
                        return std::optional<bool>(false);
                    }

                    id3v2::TagReadWriter<std::string_view> obj(filename);

                    std::optional<id3v2::TagInfos> locs = obj.findTagInfos(tag.second, tagVersion);
                    if(locs.has_value())
                    {
                        const auto &tagLoc = locs.value();

                        const auto ret1 = obj.SetTag(content, tagLoc);
                        return(
                                ret1 |
                                [&obj](const std::vector<unsigned char>& buf)
                                {
                                return obj.ReWriteFile(buf);
                                });
                    }
                    else{
                        std::cerr << "Tag slot not found" << std::endl;
                        return std::optional<bool>(false);
                    }

                }
            }

            return std::optional<bool>(false);
    };

    if(ret.has_value())
        return ret.value();
    else
        return false;
}

bool SetAlbum(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TALB"},
            {"0x0300", "TALB"},
            {"0x0000", "TAL"},
    };

    return  SetTag(filename, tags, content);
}

bool SetLeadArtist(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TPE1"},
        {"0x0300", "TPE1"},
            {"0x0000", "TP1"},
    };

    return  SetTag(filename, tags, content);
}


bool SetComposer(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TCOM"},
        {"0x0300", "TCOM"},
            {"0x0000", "TCM"},
    };

    return  SetTag(filename, tags, content);
}

bool SetDate(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0300", "TDAT"},
        {"0x0400", "TDRC"},
        {"0x0000", "TDA"},
    };

    return  SetTag(filename, tags, content);
}

bool SetTextWriter(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TEXT"},
        {"0x0300", "TEXT"},
        {"0x0000", "TXT"},
    };

    return SetTag(filename, tags, content);
}


#if 0
const std::string_view GetContentType(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TCON"},
        {"0x0300", "TCON"},
        {"0x0000", "TCO"},
    };

    return SetTag(filename, tags);
}

const std::string_view GetTextWriter(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TEXT"},
        {"0x0300", "TEXT"},
        {"0x0000", "TXT"},
    };

    return SetTag(filename, tags);
}

const std::string_view GetYear(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TDRC"},
        {"0x0300", "TYER"},
        {"0x0000", "TYE"},
    };

    return SetTag(filename, tags);
}

const std::string_view GetFileType(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0300", "TFLT"},
        {"0x0000", "TFT"},
    };

    return SetTag(filename, tags);
}

const std::string_view GetTitle(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TIT2"},
        {"0x0300", "TIT2"},
        {"0x0000", "TT2"},
    };

    return SetTag(filename, tags);
}

const std::string_view GetContentGroupDescription(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TIT1"},
        {"0x0300", "TIT1"},
        {"0x0000", "TT1"},
    };

    return SetTag(filename, tags);
}

const std::string_view GetTrackPosition(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TRCK"},
        {"0x0300", "TRCK"},
        {"0x0000", "TRK"},
    };

    return SetTag(filename, tags);
}

const std::string_view GetLeadArtist(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TPE1"},
        {"0x0300", "TPE1"},
        {"0x0000", "TP1"},
    };

    return SetTag(filename, tags);
}
#endif


#endif //_TAG_WRITER
