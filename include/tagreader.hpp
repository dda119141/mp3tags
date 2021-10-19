// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_READER
#define _TAG_READER

#include <id3v2_common.hpp>
#include <id3v2_v40.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v00.hpp>
#include <id3v1.hpp>
#include <ape.hpp>


const std::string GetId3v2Tag(
    const std::string& fileName,
    const std::vector<std::pair<std::string, std::string_view>>& tags) {

    const auto ret =
        id3v2::GetTagHeader(fileName)

        | id3v2::checkForID3

        | [](id3::buffer_t buffer) {
            return id3v2::GetID3Version(buffer);
        }

        | [&](const std::string& id3Version) {

            for (auto& tag : tags) {
                if (id3Version == tag.first)  // tag.first is the id3 Version
                {
                        const auto params = [&](){
                            if (id3Version == "0x0300") {
                                const id3v2::basicParameters paramLoc {
                                    fileName,
                                    id3v2::v30(),
                                    tag.second
                                };
                                return paramLoc;

                            } else if (id3Version == "0x0400") {
                                const id3v2::basicParameters paramLoc {
                                    fileName,
                                    id3v2::v40(),
                                    tag.second
                                };
                                return paramLoc;

                            } else if (id3Version == "0x0000") {
                                const id3v2::basicParameters paramLoc {
                                    fileName,
                                    id3v2::v00(),
                                    tag.second
                                };
                                return paramLoc;

                            } else {
                                const id3v2::basicParameters paramLoc { std::string("") } ;
                                return paramLoc;
                            }
                        };
                    try {
                        id3v2::TagReadWriter<std::string> obj(params());

                        const auto found = obj.getFramePayload<std::string>().value();
                        return id3::stripLeft(found);

                    } catch (const std::runtime_error& e) {
                        std::cout << e.what();
                    }
                }
            }

            return std::string(" :No value");
        };

    return ret;
}

template <typename Function1, typename Function2>
const std::string GetTag(const std::string& filename,
        const std::vector<std::pair<std::string, std::string_view>>& id3v2Tags,
        Function1 fuc1, Function2 fuc2)
{
    const auto retApe = fuc1(filename);
    if (retApe.has_value()) {
        return retApe.value();
    }

    const auto retId3v1 = fuc2(filename);
    if(retId3v1.has_value()){
        return retId3v1.value();
    }

    return GetId3v2Tag(filename, id3v2Tags);
}

const std::string GetAlbum(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TALB"}, {"0x0300", "TALB"}, {"0x0000", "TAL"},
    };

    return GetTag(filename, id3v2Tags, ape::GetAlbum, id3v1::GetAlbum);
}

const std::string GetLeadArtist(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TPE1"}, {"0x0300", "TPE1"}, {"0x0000", "TP1"},
    };

    return GetTag(filename, id3v2Tags, ape::GetLeadArtist, id3v1::GetLeadArtist);
}

const std::string GetComposer(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TCOM"}, {"0x0300", "TCOM"}, {"0x0000", "TCM"},
    };

    return GetId3v2Tag(filename, id3v2Tags);
}

const std::string GetDate(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0300", "TDAT"}, {"0x0400", "TDRC"}, {"0x0000", "TDA"},
    };

    return GetId3v2Tag(filename, id3v2Tags);
}

//Also known as Genre
const std::string GetContentType(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TCON"}, {"0x0300", "TCON"}, {"0x0000", "TCO"},
    };

    return GetTag(filename, id3v2Tags, ape::GetGenre, id3v1::GetGenre);
}

const std::string GetComment(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "COMM"}, {"0x0300", "COMM"}, {"0x0000", "COM"},
    };

    return GetTag(filename, id3v2Tags, ape::GetComment, id3v1::GetComment);
}

const std::string GetTextWriter(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TEXT"}, {"0x0300", "TEXT"}, {"0x0000", "TXT"},
    };

    return GetId3v2Tag(filename, id3v2Tags);
}

const std::string GetYear(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TDRC"}, {"0x0300", "TYER"}, {"0x0000", "TYE"},
    };

    return GetTag(filename, id3v2Tags, ape::GetYear, id3v1::GetYear);
}

const std::string GetFileType(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0300", "TFLT"}, {"0x0000", "TFT"},
    };

    return GetId3v2Tag(filename, id3v2Tags);
}

const std::string GetTitle(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TIT2"}, {"0x0300", "TIT2"}, {"0x0000", "TT2"},
    };

    return GetTag(filename, id3v2Tags, ape::GetTitle, id3v1::GetTitle);
}

const std::string GetContentGroupDescription(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TIT1"}, {"0x0300", "TIT1"}, {"0x0000", "TT1"},
    };

    return GetId3v2Tag(filename, id3v2Tags);
}

const std::string GetTrackPosition(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TRCK"}, {"0x0300", "TRCK"}, {"0x0000", "TRK"},
    };

    return GetId3v2Tag(filename, id3v2Tags);
}

const std::string GetBandOrchestra(const std::string& filename) {
    const std::vector<std::pair<std::string, std::string_view>> id3v2Tags{
        {"0x0400", "TPE2"}, {"0x0300", "TPE2"}, {"0x0000", "TP2"},
    };

    return GetId3v2Tag(filename, id3v2Tags);
}

#endif //_TAG_READER
