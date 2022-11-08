// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_WRITER
#define _TAG_WRITER

#include <tagreader.hpp>

class AudioWriteTag
{
  execution_status_t writeSingleFrame(const meta_entry &audio_entry,
                                      std::string_view content)
  {
    execution_status_t result;

    auto &v2Tag = m_audioTag.v2Tag;
    if (v2Tag.has_value()) {
      const auto retVal = v2Tag.value().writeFrame(audio_entry, content);
      if (statusErrorFrom(retVal)) {
        std::cout << get_message_from_tag_type(retVal.frame_type_val)
                  << get_message_from_status(retVal.rstatus);
        result = retVal;
      }
    }
    const auto &v1Tag = m_audioTag.v1Tag;
    if (v1Tag.has_value()) {
      const auto retVal = v1Tag.value().writeFramePayload(audio_entry, content);
      if (statusErrorFrom(retVal)) {
        std::cout << get_message_from_tag_type(retVal.frame_type_val)
                  << get_message_from_status(retVal.rstatus);
        result = retVal;
      }
    }
    auto &apeTag = m_audioTag.apeTag;
    if (apeTag.has_value()) {
      const auto retVal =
          apeTag.value().writeFramePayload(audio_entry, content);
      if (statusErrorFrom(retVal)) {
        std::cout << get_message_from_tag_type(retVal.frame_type_val)
                  << get_message_from_status(retVal.rstatus);
        result = retVal;
      }
    }

    return result;
  }

public:
  AudioWriteTag(const AudioWriteTag &obj) = default;
  AudioWriteTag &operator=(const AudioWriteTag &obj) = default;

  explicit AudioWriteTag(const std::string &filename) : m_audioTag(filename) {}

  bool writeFrame(const meta_entry &audio_entry, std::string_view content)
  {
    return noStatusErrorFrom(writeSingleFrame(audio_entry, content));
  }

  AudioWriteTag &writeEntry(const meta_entry &audio_entry,
                            std::string_view content) &&
  {
    read_results.push_back(
        std::make_pair(audio_entry, writeSingleFrame(audio_entry, content)));

    return *this;
  }

  AudioWriteTag &writeEntry(const meta_entry &audio_entry,
                            std::string_view content) &
  {
    read_results.push_back(
        std::make_pair(audio_entry, writeSingleFrame(audio_entry, content)));

    return *this;
  }

  bool getLastResult() const
  {
    if (read_results.size() > 0) {
      const auto res = read_results.back().second;
      return noStatusErrorFrom(res);
    } else {
      return false;
    }
  }

  void printResults() const
  {
    for (auto &elt : read_results) {
      std::cout << get_frame_name(elt.first) << ": "
                << get_message_from_status(elt.second.rstatus);
    }

    return;
  }

private:
  AudioTag m_audioTag;
  std::vector<std::pair<meta_entry, execution_status_t>> read_results;
};

void SetMany(const std::string &filename)
{
  return AudioWriteTag{filename}
      .writeEntry(meta_entry::album, "Rap")
      .writeEntry(meta_entry::artist, "PetiPays")
      .printResults();
}

bool SetAlbum(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::album, content);
}

bool SetLeadArtist(const std::string &filename, std::string_view content)
{
  return AudioWriteTag{filename}.writeFrame(meta_entry::artist, content);
}

bool SetGenre(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::genre, content);
}

bool SetBandOrchestra(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::bandOrchestra, content);
}

bool SetComposer(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::composer, content);
}

bool SetDate(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::date, content);
}

bool SetTextWriter(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::textwriter, content);
}

bool SetTitle(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::title, content);
}

bool SetYear(const std::string &filename, std::string_view content)
{
  AudioWriteTag tag{filename};
  return tag.writeFrame(meta_entry::year, content);
}

#endif //_TAG_WRITER
