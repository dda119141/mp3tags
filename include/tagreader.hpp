// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_READER
#define _TAG_READER

#include <ape.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>
#include <printEntries.hpp>
#include <sstream>

template <typename T>
bool isValidAudioTag(T &&tag)
{
  return (std::forward<T>(tag).has_value() &&
          std::forward<T>(tag).value().isOk());
}

class AudioTag
{
  const auto readSingleFrame(const meta_entry &audio_entry)
  {
    std::vector<frameContent_t> results;

    if (isValidAudioTag(v2Tag)) {
      const auto result = v2Tag.value().getFrame(audio_entry);
      if (noStatusErrorFrom(result.parseStatus)) {
        return result;
      } else {
        results.push_back(result);
      }
    }
    if (isValidAudioTag(v1Tag)) {
      const auto result = v1Tag.value().getFramePayload(audio_entry);
      if (noStatusErrorFrom(result.parseStatus)) {
        return result;
      } else {
        results.push_back(result);
      }
    }
    if (isValidAudioTag(apeTag)) {
      const auto result = apeTag.value().getFramePayload(audio_entry);
      if (noStatusErrorFrom(result.parseStatus)) {
        return result;
      } else {
        results.push_back(result);
      }
    }

    std::stringstream coll;
    coll << "\n"
         << FileName
         << " ape: " << get_message_from_status(results[0].parseStatus.rstatus);
    coll << FileName << " id3v1: "
         << get_message_from_status(results[1].parseStatus.rstatus);
    coll << FileName << " id3v2: "
         << get_message_from_status(results[2].parseStatus.rstatus);
    error_status_collection.push_back(coll.str());

    return frameContent_t{};
  }

  enum class AudioChecks { v2Check = 0, v1Check, apeCheck, exitChecks };

public:
  AudioTag(const AudioTag &obj) = default;
  AudioTag &operator=(const AudioTag &obj) = default;

  explicit AudioTag(const std::string &filename) : FileName(filename)
  {
    auto doCheck = AudioChecks::v2Check;
    do {
      switch (doCheck) {
      case AudioChecks::v2Check: {
        v2Tag.emplace(filename);
        if (statusErrorFrom(v2Tag.value().getStatus())) {
          get_message_from_status(v2Tag.value().getStatus().rstatus);
          doCheck = AudioChecks::v1Check;
        } else {
          doCheck = AudioChecks::exitChecks;
        }
      }
      case AudioChecks::v1Check: {
        v1Tag.emplace(filename);
        if (statusErrorFrom(v1Tag.value().getStatus())) {
          get_message_from_status(v1Tag.value().getStatus().rstatus);
          doCheck = AudioChecks::apeCheck;
        } else {
          doCheck = AudioChecks::exitChecks;
        }
        break;
      }
      case AudioChecks::apeCheck: {
        apeTag.emplace(filename);
        if (statusErrorFrom(apeTag.value().getStatus())) {
          get_message_from_status(apeTag.value().getStatus().rstatus);
          doCheck = AudioChecks::exitChecks;
        } else {
          doCheck = AudioChecks::exitChecks;
        }
        break;
      }
      default:
        break;
      }
    } while (doCheck != AudioChecks::exitChecks);
  }

  AudioTag &readEntry(const meta_entry &audio_entry) &&
  {
    read_results.push_back({audio_entry, readSingleFrame(audio_entry)});

    return *this;
  }

  AudioTag &readEntry(const meta_entry &audio_entry) &
  {
    read_results.push_back({audio_entry, readSingleFrame(audio_entry)});

    return *this;
  }

  const auto getLastResult() const
  {
    if (read_results.size() > 0) {
      return read_results.back().frameStatus;
    } else {
      return frameContent_t{};
    }
  }

  const auto printResults() const
  {
    std::stringstream ss;

    for (auto &elt : read_results) {
      ss << get_frame_name(elt.metaEntry) << ": "
         << elt.frameStatus.payload.value_or("");
      ss << "\n";
    }

    if (error_status_collection.size() > 1) {
      ss << "Errors: ";
      for (const auto &error_status : error_status_collection) {
        ss << error_status;
      }
    }

    return ss.str();
  }

private:
  const std::string &FileName;
  std::optional<id3v2::ConstructTag> v2Tag{};
  std::optional<id3v1::handle_t> v1Tag{};
  std::optional<ape::handle_t> apeTag{};
  std::vector<audio_contentView_t> read_results;
  std::vector<std::string> error_status_collection{};
  friend class AudioWriteTag;
};

const auto GetAudioFrames(const std::string &filename)
{
  int doCheck = 0;
  std::cout << "*  File: " << filename << std::endl;
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
    AudioTag aTag{filename};
    return aTag.readEntry(entry).getLastResult();
  };
}

const auto GetAlbum(const std::string &filename)
{
#if 0
  auto rete = AudioTag{filename}
                  .readEntry(meta_entry::album)
                  .readEntry(meta_entry::artist)
                  .printResults();
  std::cout << rete << std::endl;
#endif
  const auto ret = GetAudioFrame(filename)(meta_entry::album);
  return ret.payload.value_or("");
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
