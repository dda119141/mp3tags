#ifndef _ID3V2_230
#define _ID3V2_230

#include <id3.hpp>

namespace id3v2
{

constexpr auto v30FrameIDSize = 4;

class v30
{
public:
  const auto get_frame(const meta_entry &entry) const
  {
    switch (entry) {
    case meta_entry::album:
    case meta_entry::artist:
    case meta_entry::composer:
    case meta_entry::date:
    case meta_entry::genre:
    case meta_entry::textwriter:
    case meta_entry::audioencryption:
    case meta_entry::language:
    case meta_entry::time:
    case meta_entry::year:
    case meta_entry::title:
    case meta_entry::originalfilename:
    case meta_entry::bandOrchestra:
    case meta_entry::filetype:
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
                                       uint32_t index) const
  {
    const auto start = v30FrameIDSize + index;

    if (buffer.size() >= start) {
      uint32_t val = buffer.at(start + 0) * std::pow(2, 24);

      val += buffer.at(start + 1) * std::pow(2, 16);
      val += buffer.at(start + 2) * std::pow(2, 8);
      val += buffer.at(start + 3) * std::pow(2, 0);

      return val;
    } else {
      ID3_LOG_ERROR("failed..: size: {} and start: {}..", buffer.size(), start);
    }

    return std::nullopt;
  }

}; // v30

};     // namespace id3v2
#endif //_ID3V2_230
