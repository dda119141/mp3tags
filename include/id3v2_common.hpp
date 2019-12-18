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

    std::optional<std::vector<char>> check_for_ID3(const std::vector<char>& buffer)
    {
        auto tag = id3v2::GetID3FileIdentifier(buffer);
        if (tag != "ID3") {
            std::cerr << "error " << __func__ << std::endl;
            return {};
        }
        else {
            return buffer;
        }
    }

    const std::optional<std::string> ProcessID3Version(const std::string& filename)
    {
        return(
                id3v2::GetHeader(filename)
                | id3v2::check_for_ID3
                | [](const std::vector<char>& buffer)
                {
                return id3v2::GetID3Version(buffer);
                }
              );
    }


    template <typename id3Type>
        const auto is_tag(std::string name)
        {
            return (
                    std::any_of(id3v2::GetTagNames<id3Type>().cbegin(), id3v2::GetTagNames<id3Type>().cend(), 
                        [&](std::string obj) { return name == obj; } ) 
                   );
        }



}; //end namespace id3v2




#endif //_ID3V2_COMMON
