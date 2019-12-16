#ifndef _ID3V2_COMMON
#define _ID3V2_COMMON

#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <cmath>
#include <variant>
#include <type_traits>
#include <id3v2_230.hpp>
#include <id3v2_000.hpp>



template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace id3v2
{
    using iD3Variant = std::variant <id3v2::v230, id3v2::v00>;

    std::optional<uint32_t> GetFrameSize(const std::vector<char>& buff, 
            const iD3Variant& TagVersion, uint32_t TagIndex)
    {
        return std::visit(overloaded {
                [&](id3v2::v230 arg) {
                return arg.GetFrameSize(buff, TagIndex);
                },
                [&](id3v2::v00 arg) {
                return arg.GetFrameSize(buff, TagIndex);
                }
                }, TagVersion);
    }

    uint32_t GetFrameHeaderSize(const iD3Variant& TagVersion)
    {
        return std::visit(overloaded {
                [&](id3v2::v230 arg) {
                return arg.FrameHeaderSize();
                },
                [&](id3v2::v00 arg) {
                return arg.FrameHeaderSize();
                }
                }, TagVersion);
    }


}; //end namespace id3v2




#endif //_ID3V2_COMMON
