#ifndef _TAG_READER
#define _TAG_READER

#include <id3v2_common.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v00.hpp>
#include <id3v1.hpp>

template <typename tagType>
const std::string GetTheTag(
    const std::string& filename,
    const std::vector<std::pair<std::string, std::string_view>>& tags) {
    const auto ret =
        id3v2::GetTagHeader(filename) | id3v2::check_for_ID3 |
        [](const std::vector<unsigned char>& buffer) {
            return id3v2::GetID3Version(buffer);
        } |
        [&](const std::string& id3Version) {
            // std::cout << "id3version: " << id3Version << std::endl;
            id3v2::iD3Variant tagVersion;

            for (auto& tag : tags) {
                if (id3Version == tag.first)  // tag.first is the id3 Version
                {
                    if (id3Version == "0x0300") {
                        tagVersion = id3v2::v30();
                    } else if (id3Version == "0x0400") {
                        tagVersion = id3v2::v40();
                    } else if (id3Version == "0x0000") {
                        tagVersion = id3v2::v00();
                    } else {
                        return expected::makeError<std::string>(
                            "version not supported");
                    }

                    id3v2::TagReadWriter<std::string> obj(filename);

                    return obj.extractTag<std::string>(tag.second, tagVersion);
                }
            }
            return expected::makeError<std::string>()
                   << "id3 version not supported"
                   << "\n";
        };

    if (!ret.has_value()) {
        // return "Tag not found";
        return ret.error();
    } else {
        return id3::stripLeft(ret.value());
    }
}

const std::string GetAlbum(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TALB"}, {"0x0300", "TALB"}, {"0x0000", "TAL"},
    };

    const auto ret = id3v1::GetAlbum(filename);

    if(ret.has_value()){
        return ret.value();
    }else{
        return GetTheTag<std::string>(filename, tags);
    }
}

const std::string GetLeadArtist(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TPE1"}, {"0x0300", "TPE1"}, {"0x0000", "TP1"},
    };

    const auto ret = id3v1::GetLeadArtist(filename);

    if(ret.has_value()){
        return ret.value();
    }else{
        return GetTheTag<std::string>(filename, tags);
    }
}

const std::string GetComposer(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TCOM"}, {"0x0300", "TCOM"}, {"0x0000", "TCM"},
    };

    return GetTheTag<std::string>(filename, tags);
}

const std::string GetDate(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0300", "TDAT"}, {"0x0400", "TDRC"}, {"0x0000", "TDA"},
    };

    return GetTheTag<std::string>(filename, tags);
}

//Also known as Genre
const std::string GetContentType(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TCON"}, {"0x0300", "TCON"}, {"0x0000", "TCO"},
    };

    return GetTheTag<std::string>(filename, tags);
}

const std::string GetComment(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "COMM"}, {"0x0300", "COMM"}, {"0x0000", "COM"},
    };

    const auto ret = id3v1::GetComment(filename);

    if(ret.has_value()){
        return ret.value();
    }else{
        return GetTheTag<std::string>(filename, tags);
    }
}

const std::string GetTextWriter(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TEXT"}, {"0x0300", "TEXT"}, {"0x0000", "TXT"},
    };

    return GetTheTag<std::string>(filename, tags);
}

const std::string GetYear(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TDRC"}, {"0x0300", "TYER"}, {"0x0000", "TYE"},
    };

    const auto ret = id3v1::GetYear(filename);

    if(ret.has_value()){
        return ret.value();
    }else{
        return GetTheTag<std::string>(filename, tags);
    }
}

const std::string GetFileType(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0300", "TFLT"}, {"0x0000", "TFT"},
    };

    return GetTheTag<std::string>(filename, tags);
}

const std::string GetTitle(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TIT2"}, {"0x0300", "TIT2"}, {"0x0000", "TT2"},
    };

    const auto ret = id3v1::GetTitle(filename);

    if(ret.has_value()){
        return ret.value();
    }else{
        return GetTheTag<std::string>(filename, tags);
    }
}

const std::string GetContentGroupDescription(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TIT1"}, {"0x0300", "TIT1"}, {"0x0000", "TT1"},
    };

    return GetTheTag<std::string>(filename, tags);
}

const std::string GetTrackPosition(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TRCK"}, {"0x0300", "TRCK"}, {"0x0000", "TRK"},
    };

    return GetTheTag<std::string>(filename, tags);
}

const std::string GetBandOrchestra(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> tags{
        {"0x0400", "TPE2"}, {"0x0300", "TPE2"}, {"0x0000", "TP2"},
    };

    return GetTheTag<std::string>(filename, tags);
}

#endif //_TAG_READER
