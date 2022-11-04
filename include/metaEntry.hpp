#ifndef METAENTRY_HPP
#define METAENTRY_HPP

enum class meta_entry {
  idle = 0,
  album,
  artist,
  genre,
  title,
  year,
  composer,
  date,
  textwriter,
  trackposition,
  audioencryption,
  language,
  time,
  originalfilename,
  filetype, // Only available in id3v2 tag
  bandOrchestra,
  comments,
  numberOfEntries
};

constexpr auto max_meta_entries =
    static_cast<std::size_t>(meta_entry::numberOfEntries);

constexpr auto get_frame_name(const meta_entry &entry)
{
  switch (entry) {
  case meta_entry::album:
    return std::string_view{"ALBUM"};
    break;
  case meta_entry::artist:
    return std::string_view{"ARTIST"};
    break;
  case meta_entry::composer:
    return std::string_view{"COMPOSER"};
    break;
  case meta_entry::title:
    return std::string_view{"TITLE"};
    break;
  case meta_entry::date:
    return std::string_view{"DATE"};
    break;
  case meta_entry::genre:
    return std::string_view{"Genre"};
    break;
  case meta_entry::textwriter:
    return std::string_view{"Text writer"};
    break;
  case meta_entry::trackposition:
    return std::string_view{"Track position"};
    break;
  case meta_entry::audioencryption:
    return std::string_view{"Audio Encryption"};
    break;
  case meta_entry::language:
    return std::string_view{"Language"};
    break;
  case meta_entry::time:
    return std::string_view{"Time"};
    break;
  case meta_entry::year:
    return std::string_view{"Year"};
    break;
  case meta_entry::originalfilename:
    return std::string_view{"Original filename"};
    break;
  case meta_entry::filetype:
    return std::string_view{"Filetype"};
    break;
  case meta_entry::bandOrchestra:
    return std::string_view{"Band Orchestra"};
    break;
  case meta_entry::comments:
    return std::string_view{"Comments"};
    break;
  default:
    return std::string_view{"No string from this meta entry"};
    break;
  }
}

#endif // METAENTRY_HPP