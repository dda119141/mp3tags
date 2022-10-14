// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_READER
#define _TAG_READER

#include <ape.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>
#include <printEntries.hpp>

const auto GetAudioFrames(const std::string &filename)
{
  int doCheck = 0;

  do {
    switch (doCheck) {
    case 0: {
      const id3v2::ConstructTag v2Tag{filename};
      if (noStatusErrorFrom(v2Tag.getStatus())) {
        print_meta_entries(v2Tag);
        doCheck = 3;
      } else {
        doCheck = 1;
      }
      break;
    }
    case 1: {
      const auto v1Tag = id3v1::handle_t{filename};
      if (noStatusErrorFrom(v1Tag.getStatus())) {
        print_meta_entries(v1Tag);
        doCheck = 3;
      } else {
        doCheck = 2;
      }
      break;
    }
    case 2: {
      const auto apeTag = ape::handle_t{filename};
      if (noStatusErrorFrom(apeTag.getStatus())) {
        print_meta_entries(apeTag);
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

  return frameContent_t{};
}

const auto GetAudioFrame(const std::string &filename)
{
  return [&filename](const meta_entry &entry) {
    const auto retApe = ape::getApePayload(filename)(entry);
    if (retApe.parseStatus.rstatus == rstatus_t::noError) {
      return retApe;
    }

    const auto retId3v1 = id3v1::getId3v1Payload(filename)(entry);
    if (retId3v1.parseStatus.rstatus == rstatus_t::noError) {
      return retId3v1;
    }

    const id3v2::ConstructTag Id3v2Tag{filename};
    const auto retId3v2 = Id3v2Tag.getFrame(entry);
    if (retId3v2.parseStatus.rstatus == rstatus_t::noError) {
      return retId3v2;
    }

    std::cout << "\n"
              << filename << " ape: "
              << get_message_from_status(retApe.parseStatus.rstatus);
    std::cout << filename << " id3v1: "
              << get_message_from_status(retId3v1.parseStatus.rstatus);
    std::cout << filename << " id3v2: "
              << get_message_from_status(retId3v2.parseStatus.rstatus);

    return frameContent_t{};
  };
}

const auto GetAlbum(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::album);
  return ret.payload.value_or(std::string(""));
}
const auto GetLeadArtist(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::artist);
  return ret.payload.value_or(std::string(""));
}
const auto GetComposer(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::composer);
  return ret.payload.value_or(std::string(""));
}
const auto GetDate(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::date);
  return ret.payload.value_or(std::string(""));
}
// Also known as Genre
const auto GetContentType(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::genre);
  return ret.payload.value_or(std::string(""));
}
const auto GetComment(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::comments);
  return ret.payload.value_or(std::string(""));
}
const auto GetTextWriter(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::textwriter);
  return ret.payload.value_or(std::string(""));
}
const auto GetYear(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::year);
  return ret.payload.value_or(std::string(""));
}
const auto GetFileType(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::filetype);
  return ret.payload.value_or(std::string(""));
}
const auto GetTitle(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::title);
  return ret.payload.value_or(std::string(""));
}
const auto GetTrackPosition(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::trackposition);
  return ret.payload.value_or(std::string(""));
}
const auto GetBandOrchestra(const std::string &filename)
{
  const auto ret = GetAudioFrame(filename)(meta_entry::bandOrchestra);
  return ret.payload.value_or(std::string(""));
}
const auto GetAllMetaEntries(const std::string &filename)
{
  return GetAudioFrames(filename);
}

#endif //_TAG_READER
