// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_WRITER
#define _TAG_WRITER

#include <id3v2_common.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v00.hpp>
#include <id3v1.hpp>
#include <ape.hpp>
#include <optional>
#include <string_conversion.hpp>

bool SetTag(const std::string& filename,
            const std::vector<std::pair<std::string, std::string_view>>& tags,
            std::string_view content) {
    const auto ret =
        id3v2::GetTagHeader(filename) 

        | id3v2::checkForID3

        | [](const std::vector<unsigned char>& buffer) {
            return id3v2::GetID3Version(buffer);
        }

        | [&](std::string id3Version) {
            const auto writePayloadResult = [&](){

                for (auto& tag : tags) {
                    if (id3Version == tag.first)  // tag.first is the id3 Version
                    {
                        const auto param = [&](){
                            if (id3Version == "0x0300") {
                                const id3v2::basicParameters paramLoc
                                {
                                    filename,
                                    id3v2::v30(),
                                    tag.second,
                                    content
                                };
                                return paramLoc;
                            } else if (id3Version == "0x0400") {
                                const id3v2::basicParameters paramLoc
                                {
                                    filename,
                                    id3v2::v40(),
                                    tag.second,
                                    content
                                };
                                return paramLoc;
                            } else if (id3Version == "0x0000") {
                                const id3v2::basicParameters paramLoc
                                {
                                    filename,
                                    id3v2::v00(),
                                    tag.second, /* Frame ID */
                                    content
                                };
                                return paramLoc;
                            } else {
                                const id3v2::basicParameters paramLoc { std::string("") } ;
                                return paramLoc;
                            };
                        }();

                        try {
                            const auto result = id3v2::writeFramePayload(param);
                            return result;
                        } catch (const std::runtime_error& e) {
                            std::cerr << "Runtime Error: " << e.what() << std::endl;
                        }
                    }
                }

                ID3_LOG_WARN("{} failed", __func__);
                return expected::makeError<bool>() << __func__ << " failed\n";
            };

            return writePayloadResult();
        };

    if (ret.has_value())
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

    const auto ret1 = ape::SetAlbum(filename, content);
    const auto ret = id3v1::SetAlbum(filename, content);

    if(ret.has_value() || ret1.has_value()){

        SetTag(filename, tags, content);

        return ret.value();
    }else{
        return SetTag(filename, tags, content);
    }
}

bool SetLeadArtist(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TPE1"},
        {"0x0300", "TPE1"},
        {"0x0000", "TP1"},
    };
    const auto ret = id3v1::SetLeadArtist(filename, content);

    if(ret.has_value()){

        SetTag(filename, tags, content);

        return ret.value();
    }else{
        return SetTag(filename, tags, content);
    }
}

bool SetGenre(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TCON"}, {"0x0300", "TCON"}, {"0x0000", "TCO"},
    };
    const auto ret1 = ape::SetGenre(filename, content);
    const auto ret = id3v1::SetGenre(filename, content);

    if(ret.has_value() || ret1.has_value()){

        SetTag(filename, tags, content);

        return ret1.value();
    }else{
        return SetTag(filename, tags, content);
    }
}


bool SetBandOrchestra(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TPE2"},
        {"0x0300", "TPE2"},
            {"0x0000", "TP2"},
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

bool SetTitle(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TIT2"},
        {"0x0300", "TIT2"},
        {"0x0000", "TT2"},
    };
    const auto ret1 = ape::SetTitle(filename, content);
    const auto ret = id3v1::SetTitle(filename, content);

    if(ret.has_value() || ret1.has_value()){

        SetTag(filename, tags, content);

        return ret.value();
    }else{
        return SetTag(filename, tags, content);
    }
}

bool SetYear(const std::string& filename, std::string_view content)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TDRC"},
        {"0x0300", "TYER"},
        {"0x0000", "TYE"},
    };
    const auto ret = id3v1::SetYear(filename, content);

    if(ret.has_value()){

        SetTag(filename, tags, content);

        return ret.value();
    }else{
        return SetTag(filename, tags, content);
    }
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

const std::string_view GetFileType(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0300", "TFLT"},
        {"0x0000", "TFT"},
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

#endif


#endif //_TAG_WRITER
