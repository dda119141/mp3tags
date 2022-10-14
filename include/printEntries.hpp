// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef PRINT_ID3_ENTRY_HPP
#define PRINT_ID3_ENTRY_HPP

#include <ape.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>

namespace id3
{
template <class T>
constexpr bool always_false = std::false_type::value;

template <typename T>
void print_entry(const T &handle, std::string_view entry_prefix,
                 meta_entry entry)
{
  std::cout << entry_prefix << ":\t\t\t";

  if constexpr ((std::is_same_v<T, id3v1::handle_t>) ||
                (std::is_same_v<T, ape::handle_t>)) {
    std::cout << handle.getFramePayload(entry).payload.value_or("");
  } else if constexpr (std::is_same_v<T, id3v2::ConstructTag>) {
    std::cout << handle.getFrame(entry).payload.value_or("");
  } else
    static_assert(always_false<T>);

  std::cout << std::endl;
}

template <typename T>
void print_meta_entries(const T &handle)
{
  std::cout << "\n";
  print_entry(handle, "Original filename", meta_entry::originalfilename);
  print_entry(handle, "Album", meta_entry::album);
  print_entry(handle, "Artist", meta_entry::artist);
  print_entry(handle, "Genre", meta_entry::genre);
  print_entry(handle, "Title", meta_entry::title);
  print_entry(handle, "BandOrchestra", meta_entry::bandOrchestra);
  print_entry(handle, "Date", meta_entry::date);
  print_entry(handle, "Audio encryption", meta_entry::audioencryption);
  print_entry(handle, "Year", meta_entry::year);
  print_entry(handle, "Filetype", meta_entry::filetype);
  print_entry(handle, "Composer", meta_entry::composer);
  print_entry(handle, "Textwriter", meta_entry::textwriter);
  print_entry(handle, "Language", meta_entry::language);
}

} // namespace id3
#endif // PRINT_ID3_ENTRY_HPP