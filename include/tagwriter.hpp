// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_WRITER
#define _TAG_WRITER

#include <ape.hpp>
#include <id3_precheck.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>

class AudioTag
{
public:
  explicit AudioTag(const std::string &filename)
  {
    int doCheck = 0;
    do {
      switch (doCheck) {
      case 0: {
        v2Tag.emplace(filename);
        if (statusErrorFrom(v2Tag.value().getStatus())) {
          get_message_from_status(v2Tag.value().getStatus().rstatus);
          v2Tag.reset();
          doCheck = 1;
        } else {
          doCheck = 3;
        }
      }
      case 1: {
        v1Tag.emplace(filename);
        if (statusErrorFrom(v1Tag.value().getStatus())) {
          get_message_from_status(v1Tag.value().getStatus().rstatus);
          v1Tag.reset();
          doCheck = 2;
        } else {
          doCheck = 3;
        }
        break;
      }
      case 2: {
        apeTag.emplace(filename);
        if (statusErrorFrom(apeTag.value().getStatus())) {
          get_message_from_status(apeTag.value().getStatus().rstatus);
          apeTag.reset();
          doCheck = 3;
        } else {
          doCheck = 3;
        }
        break;
      }
      default:
        break;
      }
    } while (doCheck < 3);
  }

  bool writeFrame(const meta_entry &entry, std::string_view content)
  {
    std::atomic_bool ret;

    if (v2Tag.has_value()) {
      auto retVal = v2Tag.value().writeFrame(entry, content);
      if (statusErrorFrom(retVal)) {
        std::cout << get_message_from_status(retVal.rstatus);
        ret = false;
      }
    }
    if (v1Tag.has_value()) {
      const auto retVal = v1Tag.value().writeFramePayload(entry, content);
      if (statusErrorFrom(retVal)) {
        std::cout << get_message_from_status(retVal.rstatus);
        ret = false;
      }
    }
    if (apeTag.has_value()) {
      const auto retVal = apeTag.value().writeFramePayload(entry, content);
      if (statusErrorFrom(retVal)) {
        std::cout << get_message_from_status(retVal.rstatus);
        ret = false;
      }
    }

    return ret;
  }

private:
  std::optional<id3v2::ConstructTag> v2Tag{};
  std::optional<id3v1::handle_t> v1Tag{};
  std::optional<ape::handle_t> apeTag{};
};

bool SetAlbum(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::album, content);
}

bool SetLeadArtist(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::artist, content);
}

bool SetGenre(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::genre, content);
}

bool SetBandOrchestra(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::bandOrchestra, content);
}

bool SetComposer(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::composer, content);
}

bool SetDate(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::date, content);
}

bool SetTextWriter(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::textwriter, content);
}

bool SetTitle(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::title, content);
}

bool SetYear(const std::string &filename, std::string_view content)
{
  AudioTag tag{filename};
  return tag.writeFrame(meta_entry::year, content);
}

#endif //_TAG_WRITER
