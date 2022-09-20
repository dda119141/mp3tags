// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_READER
#define _TAG_READER

#include <ape.hpp>
#include <id3_precheck.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>
#include <id3v2_v00.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v40.hpp>

const auto GetId3v2Tag(const std::string &fileName,
                       const id3v2::frameArray_t &frameIDs) {

  const auto params = [&fileName, &frameIDs]() {
    const auto id3Version = preCheckId3(fileName).id3Version;

    switch (id3Version) {
    case id3v2::tagVersion_t::v30: {
      return audioProperties_t{id3v2::fileScopeProperties{
          fileName,
          id3v2::v30(),                                   // Tag version
          frameIDs.at(getIndex(id3v2::tagVersion_t::v30)) // Frame ID
      }};
    } break;
    case id3v2::tagVersion_t::v40: {
      return audioProperties_t{id3v2::fileScopeProperties{
          fileName,
          id3v2::v40(),                                   // Tag version
          frameIDs.at(getIndex(id3v2::tagVersion_t::v40)) // Frame ID
      }};
    } break;
    case id3v2::tagVersion_t::v00: {
      return audioProperties_t{id3v2::fileScopeProperties{
          fileName,
          id3v2::v00(),                                   // Tag version
          frameIDs.at(getIndex(id3v2::tagVersion_t::v00)) // Frame ID
      }};
    } break;
    default: {
      return audioProperties_t{};
    } break;
    }
  };
  try {
    const auto obj = std::make_unique<id3v2::TagReader>(std::move(params()));
    const auto found = obj->getFramePayload();

    return found;
  } catch (const id3::audio_tag_error &e) {
    std::cout << e.what();

  } catch (const std::runtime_error &e) {
    std::cout << e.what();
  }

  return frameContent_t{};
}

template <typename Function1, typename Function2>
const auto GetTag(const std::string &filename,
                  const id3v2::frameArray_t &id3v2Tags, Function1 fuc1,
                  Function2 fuc2) {
  const auto retApe = fuc1(filename);
  if (retApe.parseStatus.rstatus == rstatus_t::noError) {
    return retApe;
  }

  const auto retId3v1 = fuc2(filename);
  if (retId3v1.parseStatus.rstatus == rstatus_t::noError) {
    return retId3v1;
  }

  const auto retId3v2 = GetId3v2Tag(filename, id3v2Tags);
  if (retId3v2.parseStatus.rstatus == rstatus_t::noError) {
    return retId3v2;
  }

  std::cout << "\n"
            << filename
            << " ape: " << get_message_from_status(retApe.parseStatus.rstatus);
  std::cout << filename << " id3v1: "
            << get_message_from_status(retId3v1.parseStatus.rstatus);
  std::cout << filename << " id3v2: "
            << get_message_from_status(retId3v2.parseStatus.rstatus);

  return frameContent_t{};
}

const auto GetAlbum(const std::string &filename) {
  static const id3v2::frame_t frames{{id3v2::tagVersion_t::v40, "TALB"},
                                     {id3v2::tagVersion_t::v30, "TALB"},
                                     {id3v2::tagVersion_t::v00, "TAL"}};

  const id3v2::frameID_s<id3v2::frame_t> Id3v2Tags{frames};

  const auto ret =
      GetTag(filename, Id3v2Tags.mFrames, ape::GetAlbum, id3v1::GetAlbum);

  return ret.payload.value_or(std::string(""));
}

const auto GetLeadArtist(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TPE1"},
      {id3v2::tagVersion_t::v30, "TPE1"},
      {id3v2::tagVersion_t::v00, "TP1"},
  };
  const id3v2::frameID_s<id3v2::frame_t> Id3v2Tags{frames};

  const auto ret = GetTag(filename, Id3v2Tags.mFrames, ape::GetLeadArtist,
                          id3v1::GetLeadArtist);
  return ret.payload.value_or(std::string(""));
}

const auto GetComposer(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TCOM"},
      {id3v2::tagVersion_t::v30, "TCOM"},
      {id3v2::tagVersion_t::v00, "TCM"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = GetId3v2Tag(filename, id3v2Tags.mFrames);

  return ret.payload.value_or(std::string(""));
}

const auto GetDate(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v30, "TDAT"},
      {id3v2::tagVersion_t::v40, "TDRC"},
      {id3v2::tagVersion_t::v00, "TDA"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = GetId3v2Tag(filename, id3v2Tags.mFrames);

  return ret.payload.value_or(std::string(""));
}

// Also known as Genre
const auto GetContentType(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TCON"},
      {id3v2::tagVersion_t::v30, "TCON"},
      {id3v2::tagVersion_t::v00, "TCO"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret =
      GetTag(filename, id3v2Tags.mFrames, ape::GetGenre, id3v1::GetGenre);

  return ret.payload.value_or(std::string(""));
}

const auto GetComment(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "COMM"},
      {id3v2::tagVersion_t::v30, "COMM"},
      {id3v2::tagVersion_t::v00, "COM"},
  };

  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret =
      GetTag(filename, id3v2Tags.mFrames, ape::GetComment, id3v1::GetComment);
  return ret.payload.value_or(std::string(""));
}

const auto GetTextWriter(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TEXT"},
      {id3v2::tagVersion_t::v30, "TEXT"},
      {id3v2::tagVersion_t::v00, "TXT"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = GetId3v2Tag(filename, id3v2Tags.mFrames);

  return ret.payload.value_or(std::string(""));
}

const auto GetYear(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TDRC"},
      {id3v2::tagVersion_t::v30, "TYER"},
      {id3v2::tagVersion_t::v00, "TYE"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret =
      GetTag(filename, id3v2Tags.mFrames, ape::GetYear, id3v1::GetYear);

  return ret.payload.value_or(std::string(""));
}

const auto GetFileType(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v30, "TFLT"},
      {id3v2::tagVersion_t::v00, "TFT"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = GetId3v2Tag(filename, id3v2Tags.mFrames);

  return ret.payload.value_or(std::string(""));
}

const auto GetTitle(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TIT2"},
      {id3v2::tagVersion_t::v30, "TIT2"},
      {id3v2::tagVersion_t::v00, "TT2"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret =
      GetTag(filename, id3v2Tags.mFrames, ape::GetTitle, id3v1::GetTitle);

  return ret.payload.value_or(std::string(""));
}

const auto GetContentGroupDescription(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TIT1"},
      {id3v2::tagVersion_t::v30, "TIT1"},
      {id3v2::tagVersion_t::v00, "TT1"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = GetId3v2Tag(filename, id3v2Tags.mFrames);

  return ret.payload.value_or(std::string(""));
}

const auto GetTrackPosition(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TRCK"},
      {id3v2::tagVersion_t::v30, "TRCK"},
      {id3v2::tagVersion_t::v00, "TRK"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = GetId3v2Tag(filename, id3v2Tags.mFrames);

  return ret.payload.value_or(std::string(""));
}

const auto GetBandOrchestra(const std::string &filename) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TPE2"},
      {id3v2::tagVersion_t::v30, "TPE2"},
      {id3v2::tagVersion_t::v00, "TP2"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = GetId3v2Tag(filename, id3v2Tags.mFrames);

  return ret.payload.value_or(std::string(""));
}
#endif //_TAG_READER
