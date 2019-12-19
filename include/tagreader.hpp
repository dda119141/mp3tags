#ifndef _TAG_READER
#define _TAG_READER

#include <id3v2_common.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v00.hpp>


using iD3Variant = std::variant <id3v2::v30, id3v2::v00, id3v2::v40>;

template <typename T>
const std::optional<T> GetTag(const iD3Variant& tagVersion,
     const std::string filename,
    const std::string tagname)
{
        id3v2::RetrieveTagLocation<std::string> obj(filename);
        return obj.find_tag(tagname, tagVersion);
}


const std::string GetTheTag(const std::string& filename, const std::vector<std::pair<std::string, std::string>>& tags)
{
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
                        if(id3Version == "0x0300"){
                            iD3Variant tagVersion = std::get<id3v2::v30>(_tagversion);
                            return GetTag<std::string>(tagVersion, filename, tag.second);
                        }
                        else if(id3Version == "0x0400"){
                            iD3Variant tagVersion = std::get<id3v2::v40>(_tagversion);
                            return GetTag<std::string>(tagVersion, filename, tag.second);
                        }
                        else if(id3Version == "0x0000"){
                            iD3Variant tagVersion = std::get<id3v2::v00>(_tagversion);
                            return GetTag<std::string>(tagVersion, filename, tag.second);
                        }
                        else
                            return std::optional<std::string>(std::string("version not supported"));

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


const std::string GetAlbum(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TALB"},
            {"0x0300", "TALB"},
            {"0x0000", "TAL"},
    };

    return  GetTheTag(filename, tags);
}

const std::string GetComposer(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TCOM"},
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
        {"0x0400", "TEXT"},
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

const std::string GetTrackPosition(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TRCK"},
        {"0x0300", "TRCK"},
        {"0x0000", "TRK"},
    };

    return GetTheTag(filename, tags);
}

const std::string GetLeadArtist(const std::string& filename)
{
    const std::vector<std::pair<std::string, std::string>> tags
    {
        {"0x0400", "TPE1"},
        {"0x0300", "TPE1"},
        {"0x0000", "TP1"},
    };

    return GetTheTag(filename, tags);
}





#endif //_TAG_READER
