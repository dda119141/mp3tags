// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _ID3V2_V40
#define _ID3V2_V40

#include <id3.hpp>

namespace id3v2
{

constexpr unsigned int v40FrameIDSize = 4;

class v40
{
public:
  const auto get_frame(const meta_entry &entry) const
  {
    switch (entry) {
    case meta_entry::album:
    case meta_entry::artist:
    case meta_entry::composer:
    case meta_entry::title:
    case meta_entry::date:
    case meta_entry::genre:
    case meta_entry::textwriter:
    case meta_entry::audioencryption:
    case meta_entry::language:
    case meta_entry::time:
    case meta_entry::year:
    case meta_entry::originalfilename:
    case meta_entry::filetype:
    case meta_entry::bandOrchestra:
      return id3::v30_v40::get_frame(entry);
      break;
    default:
      return std::string_view();
      break;
    }
    return std::string_view();
  }

  constexpr unsigned int FrameHeaderSize(void) { return 10; }

  std::optional<uint32_t> GetFrameSize(const std::vector<char> &buffer,
                                       uint32_t index)
  {
    const auto start = v40FrameIDSize + index;

    if (buffer.size() >= start) {
      using paire = std::pair<uint32_t, uint32_t>;

      const std::vector<uint32_t> pow_val = {24, 16, 8, 0};

      std::vector<paire> result(pow_val.size());

      std::transform(
          std::begin(buffer) + start,
          std::begin(buffer) + start + pow_val.size(), pow_val.begin(),
          result.begin(),
          [](uint32_t a, uint32_t b) { return std::make_pair(a, b); });

      const uint32_t val =
          std::accumulate(result.begin(), result.end(), 0, [](int a, paire b) {
            return (a + (b.first * std::pow(2, b.second)));
          });

      return val;
    } else {
      ID3_LOG_ERROR("failed..: size: {} and start: {}..", buffer.size(), start);
    }

    return std::optional<uint32_t>{};
  }

}; // v40

};     // namespace id3v2
#endif // ID3V2_V40
