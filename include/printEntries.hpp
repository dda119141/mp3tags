// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef PRINT_ID3_ENTRY_HPP
#define PRINT_ID3_ENTRY_HPP

#include <ape.hpp>
#include <id3v1.hpp>
#include <id3v2_common.hpp>
#include <iomanip>

namespace id3
{
template <class T>
constexpr bool always_false = std::false_type::value;

void print_sub_entry(std::string_view entry, std::string_view content)
{
  std::cout << std::left << std::setfill(' ') << std::setw(20) << entry << ":";
  std::cout << std::right << content;
  std::cout << std::endl;
}

template <typename T>
void print_entry(const T &handle, meta_entry entry)
{
  if constexpr ((std::is_same_v<T, id3v1::handle_t>) ||
                (std::is_same_v<T, ape::handle_t>)) {
    const auto ret = handle.getFramePayload(entry);
    if (ret.payload.has_value()) {
      print_sub_entry(get_frame_name(entry), ret.payload.value());
    }
  } else if constexpr (std::is_same_v<T, id3v2::ConstructTag>) {
    const auto ret = handle.getFrame(entry);
    if (ret.payload.has_value()) {
      print_sub_entry(get_frame_name(entry), ret.payload.value());
    }
  } else
    static_assert(always_false<T>);
}

template <typename T>
void print_meta_entries(const T &handle)
{
  for (unsigned int i = 1; i < max_meta_entries; i++) {
    print_entry(handle, static_cast<meta_entry>(i));
  }
  std::cout << "\n";
}

} // namespace id3
#endif // PRINT_ID3_ENTRY_HPP