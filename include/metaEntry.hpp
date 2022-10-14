#ifndef METAENTRY_HPP
#define METAENTRY_HPP

enum class meta_entry {
  idle,
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
  comments
};

#endif // METAENTRY_HPP