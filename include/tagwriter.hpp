// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_WRITER
#define _TAG_WRITER

#include <ape.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>
#include <id3v2_v00.hpp>
#include <id3v2_v30.hpp>
#include <id3v2_v40.hpp>
#include <optional>
#include <string_conversion.hpp>

auto id3v2_SetFrameContent(
    const std::string &filename,
    const std::vector<std::pair<std::string, std::string_view>> &tags,
    std::string_view content) {
  const auto ret =
      id3v2::GetTagHeader(filename)

      | id3v2::checkForID3

      | [](id3::buffer_t buffer) { return id3v2::GetID3Version(buffer); }

      | [&](const std::string &id3Version) {
          const auto writePayloadResult = [&]() -> id3::execution_status_t {
            for (auto &tag : tags) {
              if (id3Version == tag.first) // tag.first is the id3 Version
              {
                const auto param = [&]() {
                  if (id3Version == "0x0300") {
                    return audioProperties_t{id3v2::fileScopeProperties{
                        filename,     // filename
                        id3v2::v30(), // Tag version
                        tag.second,   // Frame ID
                        content       // frame payload
                    }};
                  } else if (id3Version == "0x0400") {
                    return audioProperties_t{id3v2::fileScopeProperties{
                        filename,     // filename
                        id3v2::v40(), // Tag version
                        tag.second,   // Frame ID
                        content       // frame payload
                    }};
                  } else if (id3Version == "0x0000") {
                    return audioProperties_t{id3v2::fileScopeProperties{
                        filename,     // filename
                        id3v2::v00(), // Tag version
                        tag.second,   // Frame ID
                        content       // frame payload
                    }};
                  } else {
                    return audioProperties_t{};
                  };
                };

                try {
                  const auto obj =
                      std::make_unique<id3v2::TagReader>(std::move(param()));

                  const id3v2::writer Writer{*obj};

                  return Writer.execute();
                } catch (const id3::audio_tag_error &e) {
                  std::cout << e.what();

                } catch (const std::runtime_error &e) {
                  std::cerr << "Runtime Error: " << e.what() << std::endl;
                }
              }
            }

            ID3_LOG_WARN("{} failed", __func__);

            return id3::execution_status_t{};
          };

          return writePayloadResult();
        };

  return ret;
}

template <typename Function1>
auto SetFramePayload(
    const std::string &filename,
    const std::vector<std::pair<std::string, std::string_view>> &id3v2Tags,
    std::string_view content, Function1 fuc1) {

  const auto retId3v1 = fuc1(filename, content);
  auto ret = id3::get_execution_status_b(retId3v1);

  const auto retId3v2 = id3v2_SetFrameContent(filename, id3v2Tags, content);
  ret |= id3::get_execution_status_b(retId3v2);

  return ret;
}

class setHandle {
public:
  explicit setHandle(
      const std::string &filename, std::string_view content,
      const std::vector<std::pair<std::string, std::string_view>> tags) {}
};

bool SetAlbum(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TALB"},
      {"0x0300", "TALB"},
      {"0x0000", "TAL"},
  };

  const auto retApe = ape::SetAlbum(filename, content);
  auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 = SetFramePayload(filename, tags, content, id3v1::SetAlbum);

  return (ret | retId3);
}

bool SetLeadArtist(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TPE1"},
      {"0x0300", "TPE1"},
      {"0x0000", "TP1"},
  };

  const auto retApe = ape::SetLeadArtist(filename, content);
  auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 =
      SetFramePayload(filename, tags, content, id3v1::SetLeadArtist);

  return (ret | retId3);
}

bool SetGenre(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TCON"},
      {"0x0300", "TCON"},
      {"0x0000", "TCO"},
  };

  const auto retApe = ape::SetGenre(filename, content);
  auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 = SetFramePayload(filename, tags, content, id3v1::SetGenre);

  return (ret | retId3);
}

bool SetBandOrchestra(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TPE2"},
      {"0x0300", "TPE2"},
      {"0x0000", "TP2"},
  };

  const auto ret = id3v2_SetFrameContent(filename, tags, content);
  return id3::get_execution_status_b(ret);
}

bool SetComposer(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TCOM"},
      {"0x0300", "TCOM"},
      {"0x0000", "TCM"},
  };

  const auto ret = id3v2_SetFrameContent(filename, tags, content);
  return id3::get_execution_status_b(ret);
}

bool SetDate(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0300", "TDAT"},
      {"0x0400", "TDRC"},
      {"0x0000", "TDA"},
  };

  const auto ret = id3v2_SetFrameContent(filename, tags, content);
  return id3::get_execution_status_b(ret);
}

bool SetTextWriter(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TEXT"},
      {"0x0300", "TEXT"},
      {"0x0000", "TXT"},
  };

  const auto ret = id3v2_SetFrameContent(filename, tags, content);
  return id3::get_execution_status_b(ret);
}

bool SetTitle(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TIT2"},
      {"0x0300", "TIT2"},
      {"0x0000", "TT2"},
  };

  const auto retApe = ape::SetTitle(filename, content);
  const auto ret = id3::get_execution_status_b(retApe);
  const auto retId3 = SetFramePayload(filename, tags, content, id3v1::SetTitle);

  return (ret | retId3);
}

bool SetYear(const std::string &filename, std::string_view content) {
  const std::vector<std::pair<std::string, std::string_view>> tags{
      {"0x0400", "TDRC"},
      {"0x0300", "TYER"},
      {"0x0000", "TYE"},
  };

  const auto retId3 = SetFramePayload(filename, tags, content, id3v1::SetYear);

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
