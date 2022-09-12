// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_WRITER
#define _TAG_WRITER

#include <ape.hpp>
#include <id3_precheck.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>
#include <id3v2_v00.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v40.hpp>
#include <optional>
#include <string_conversion.hpp>

auto id3v2_SetFrameContent(const std::string &filename,
                           const id3v2::frameArray_t &frameIDs,
                           std::string_view content) {

  const auto params = [&filename, &frameIDs, content]() {
    const auto id3Version = preCheckId3(filename).id3Version;

    switch (id3Version) {
    case id3v2::tagVersion_t::v30: {
      return audioProperties_t{id3v2::fileScopeProperties{
          filename,                                        // filename
          id3v2::v30(),                                    // Tag version
          frameIDs.at(getIndex(id3v2::tagVersion_t::v30)), // Frame ID
          content                                          // frame payload
      }};
    } break;
    case id3v2::tagVersion_t::v40: {
      return audioProperties_t{id3v2::fileScopeProperties{
          filename,                                        // filename
          id3v2::v40(),                                    // Tag version
          frameIDs.at(getIndex(id3v2::tagVersion_t::v40)), // Frame ID
          content                                          // frame payload
      }};
    }
    case id3v2::tagVersion_t::v00: {
      return audioProperties_t{id3v2::fileScopeProperties{
          filename,                                        // filename
          id3v2::v00(),                                    // Tag version
          frameIDs.at(getIndex(id3v2::tagVersion_t::v00)), // Frame ID
          content                                          // frame payload
      }};
    } break;
    default: {
      return audioProperties_t{};
    } break;
    }
  };

  try {
    const auto obj = std::make_unique<id3v2::TagReader>(std::move(params()));
    const id3v2::writer Writer{*obj};

    return Writer.getStatus();
  } catch (const id3::audio_tag_error &e) {
    std::cout << e.what();

  } catch (const std::runtime_error &e) {
    std::cerr << "Runtime Error: " << e.what() << std::endl;
  }

  ID3_LOG_WARN("{} failed", __func__);
  return id3::execution_status_t{};
}

template <typename Function1>
auto SetFramePayload(const std::string &filename,
                     const id3v2::frameArray_t &id3v2Tags,
                     std::string_view content, Function1 fuc1) {

  const auto retId3v1 = fuc1(filename, content);
  auto ret = id3::get_execution_status_b(retId3v1);

  const auto retId3v2 = id3v2_SetFrameContent(filename, id3v2Tags, content);
  ret |= id3::get_execution_status_b(retId3v2);

  return ret;
}

bool SetAlbum(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{{id3v2::tagVersion_t::v40, "TALB"},
                                     {id3v2::tagVersion_t::v30, "TALB"},
                                     {id3v2::tagVersion_t::v00, "TAL"}};
  const id3v2::frameID_s<id3v2::frame_t> Id3v2Tags{frames};
  const auto retApe = ape::SetAlbum(filename, content);

  auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 =
      SetFramePayload(filename, Id3v2Tags.mFrames, content, id3v1::SetAlbum);

  return (ret | retId3);
}

bool SetLeadArtist(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TPE1"},
      {id3v2::tagVersion_t::v30, "TPE1"},
      {id3v2::tagVersion_t::v00, "TP1"},
  };
  const id3v2::frameID_s<id3v2::frame_t> Id3v2Tags{frames};

  const auto retApe = ape::SetLeadArtist(filename, content);
  auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 = SetFramePayload(filename, Id3v2Tags.mFrames, content,
                                      id3v1::SetLeadArtist);

  return (ret | retId3);
}

bool SetGenre(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TCON"},
      {id3v2::tagVersion_t::v30, "TCON"},
      {id3v2::tagVersion_t::v00, "TCO"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};

  const auto retApe = ape::SetGenre(filename, content);
  auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 =
      SetFramePayload(filename, id3v2Tags.mFrames, content, id3v1::SetGenre);

  return (ret | retId3);
}

bool SetBandOrchestra(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TPE2"},
      {id3v2::tagVersion_t::v30, "TPE2"},
      {id3v2::tagVersion_t::v00, "TP2"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = id3v2_SetFrameContent(filename, id3v2Tags.mFrames, content);

  return id3::get_execution_status_b(ret);
}

bool SetComposer(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TCOM"},
      {id3v2::tagVersion_t::v30, "TCOM"},
      {id3v2::tagVersion_t::v00, "TCM"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = id3v2_SetFrameContent(filename, id3v2Tags.mFrames, content);

  return id3::get_execution_status_b(ret);
}

bool SetDate(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v30, "TDAT"},
      {id3v2::tagVersion_t::v40, "TDRC"},
      {id3v2::tagVersion_t::v00, "TDA"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = id3v2_SetFrameContent(filename, id3v2Tags.mFrames, content);

  return id3::get_execution_status_b(ret);
}

bool SetTextWriter(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TEXT"},
      {id3v2::tagVersion_t::v30, "TEXT"},
      {id3v2::tagVersion_t::v00, "TXT"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto ret = id3v2_SetFrameContent(filename, id3v2Tags.mFrames, content);

  return id3::get_execution_status_b(ret);
}

bool SetTitle(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TIT2"},
      {id3v2::tagVersion_t::v30, "TIT2"},
      {id3v2::tagVersion_t::v00, "TT2"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto retApe = ape::SetTitle(filename, content);
  const auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 =
      SetFramePayload(filename, id3v2Tags.mFrames, content, id3v1::SetTitle);

  return (ret | retId3);
}

bool SetYear(const std::string &filename, std::string_view content) {
  static const id3v2::frame_t frames{
      {id3v2::tagVersion_t::v40, "TDRC"},
      {id3v2::tagVersion_t::v30, "TYER"},
      {id3v2::tagVersion_t::v00, "TYE"},
  };
  const id3v2::frameID_s<id3v2::frame_t> id3v2Tags{frames};
  const auto retId3 =
      SetFramePayload(filename, id3v2Tags.mFrames, content, id3v1::SetYear);

  return retId3;
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

    return id3v2_SetFrameContent(filename, tags);
}

const std::string_view GetTextWriter(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TEXT"},
        {"0x0300", "TEXT"},
        {"0x0000", "TXT"},
    };

    return id3v2_SetFrameContent(filename, tags);
}

const std::string_view GetFileType(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0300", "TFLT"},
        {"0x0000", "TFT"},
    };

    return id3v2_SetFrameContent(filename, tags);
}

const std::string_view GetContentGroupDescription(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TIT1"},
        {"0x0300", "TIT1"},
        {"0x0000", "TT1"},
    };

    return id3v2_SetFrameContent(filename, tags);
}

const std::string_view GetTrackPosition(const std::string_view& filename)
{
    const std::vector<std::pair<std::string, std::string_view>> tags
    {
        {"0x0400", "TRCK"},
        {"0x0300", "TRCK"},
        {"0x0000", "TRK"},
    };

    return id3v2_SetFrameContent(filename, tags);
}

#endif

#endif //_TAG_WRITER
