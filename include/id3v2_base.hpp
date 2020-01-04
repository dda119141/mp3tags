#ifndef ID3V2_BASE
#define ID3V2_BASE

#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <cmath>
#include <variant>
#include <type_traits>
//#include <range/v3/all.hpp>
#include <range/v3/view.hpp>
#include <range/v3/action.hpp>
#include <range/v3/action/transform.hpp>
#include <range/v3/view/filter.hpp>

#include <result.hpp>

namespace id3v2
{
    using UCharVec = std::vector<unsigned char>;

    class TagInfos
    {
        public:
            TagInfos(uint32_t StartPos, uint32_t Length, 
                    uint32_t _encodeFlag = 0, uint32_t _doSwap = 0):
                startPos(StartPos)
                ,length(Length)
                ,encodeFlag(_encodeFlag)
                ,doSwap(_doSwap)
            {
            }
#if 1
            TagInfos():
                startPos(0)
                ,length(0)
                ,encodeFlag(0)
                ,doSwap(0)
             { }
#endif

            const uint32_t getStartPos() const
            {
                return startPos;
            }
            const uint32_t getLength() const
            {
                return length;
            }
            const uint32_t getEncodingValue() const { return encodeFlag; }
            const uint32_t getSwapValue() const { return doSwap; }
        private:
            // TagInfos() = delete;
            uint32_t startPos;
            uint32_t length;
            uint32_t encodeFlag;
            uint32_t doSwap;
    };
};

template <typename T, typename F>
auto mbind(std::optional<T>&& _obj, F&& function)
    -> decltype(function(_obj.value()))
{
    auto fuc = std::forward<F>(function);
    auto obj = std::forward<std::optional<T>>(_obj);

    if(obj.has_value()){
        return fuc(obj.value());
    }
    else{
        return {};
        //return std::nullopt;
    }
}


template <typename T>
using is_optional_string = std::is_same<std::decay_t<T>, std::optional<std::string>>;

template <typename T>
using is_optional_vector_char = std::is_same<std::decay_t<T>, std::optional<id3v2::UCharVec>>;

template <typename T,
    typename = std::enable_if_t<(std::is_same<std::decay_t<T>, std::optional<id3v2::UCharVec>>::value
        || std::is_same<std::decay_t<T>, std::optional<std::string>>::value
        || std::is_same<std::decay_t<T>, std::optional<std::string_view>>::value
        || std::is_same<std::decay_t<T>, std::optional<bool>>::value
        || std::is_same<std::decay_t<T>, std::optional<id3v2::TagInfos>>::value
        || std::is_same<std::decay_t<T>, std::optional<uint32_t>>::value) >
    , typename F >
 auto operator | (T&& _obj, F&& Function)
    -> decltype(std::forward<F>(Function)(std::forward<T>(_obj).value()))
{
    auto fuc = std::forward<F>(Function);
    auto obj = std::forward<T>(_obj);

    if(obj.has_value()){
        return fuc(obj.value());
    }
    else{
        return {};
    }
}

template <typename T>
struct integral_unsigned_asserts
{
    void operator()()
    {
        static_assert(std::is_integral<T>::value, "parameter should be integers");
        static_assert(std::is_unsigned<T>::value, "parameter should be unsigned");
    }
};

std::optional<id3v2::UCharVec> GetStringFromFile(const std::string& FileName, uint32_t num )
{
    std::ifstream fil(FileName);
    std::vector<unsigned char> buffer(num, '0');

    if(!fil.good()){
        std::cerr << "mp3 file could not be read\n";
        return {};
    }

    fil.read(reinterpret_cast<char*>(buffer.data()), num);

    return buffer;
}

namespace id3v2
{
    template <typename T>
        const auto GetValFromBuffer(const UCharVec& buffer, T index, T num_of_bytes_in_hex)
        {
            integral_unsigned_asserts<T> eval;
            eval();

            assert(buffer.size() > num_of_bytes_in_hex);

            auto version = 0;
            auto remaining = 0;
            auto bytes_to_add = num_of_bytes_in_hex;
            auto byte_to_pad = index;

            while(bytes_to_add > 0)
            {
                remaining = (num_of_bytes_in_hex - bytes_to_add);
                auto val = std::pow(2, 8 * remaining) * buffer[byte_to_pad];
                version += val;

                bytes_to_add--;
                byte_to_pad--;
            }

            return version;
        }

    template <typename T>
        const std::optional<std::string> GetHexFromBuffer(const UCharVec& buffer, T index, T num_of_bytes_in_hex)
        {
            integral_unsigned_asserts<T> eval;
            eval();

            assert(buffer.size() > num_of_bytes_in_hex);

            std::stringstream stream_obj;

            stream_obj << "0x"
                << std::setfill('0')
                << std::setw(num_of_bytes_in_hex * 2); 

            const auto version = GetValFromBuffer<T>(buffer, index, num_of_bytes_in_hex);

            if(version != 0){

                stream_obj << std::hex << version;

                return stream_obj.str();

            }else{
                return {};
            }
        }

    template <typename T1, typename T2>
        const std::optional<std::string> ExtractString(const UCharVec& buffer, T1 start, T1 end)
        {
            //if(end < start){
                std::cerr << __func__ << " error start: " << start << " end: " << end << std::endl;
            //}
           // assert(end > start);

            if(buffer.size() >= end) {
                static_assert(std::is_integral<T1>::value, "second and third parameters should be integers");
                static_assert(std::is_unsigned<T2>::value, "num of elements must be unsigned");

                std::string obj(reinterpret_cast<const char*>(buffer.data()), end);

                return obj.substr(start, (end - start));
            } else {
                std::cerr << __func__ << " error start: " << start << " end: " << end << std::endl;
                return {};
            }
        }

    template <typename T>
        std::string u8_to_u16_string(T val)
        {
            return std::accumulate(val.begin() + 1, val.end(), std::string(val.substr(0, 1)),
                    [](std::string arg1, char arg2)
                    { return arg1 + '\0' + arg2;}
                    );
        }


    template <typename T>
        constexpr T GetHeaderSize(void)
        {
            return 10;
        }

    template <typename T>
        constexpr T RetrieveSize(T n)
        {
            return n;
        }

    std::optional<uint32_t> GetTagSize(const UCharVec& buffer)
    {
        using paire = std::pair<uint32_t, uint32_t>;

        const std::vector<uint32_t> pow_val = { 21, 14, 7, 0 };

        std::vector<paire> result(pow_val.size());
        constexpr uint32_t TagIndex = 6;
        const auto it = std::begin(buffer) + TagIndex;

        std::transform(it, it + pow_val.size(),
                pow_val.begin(), result.begin(),
                [](uint32_t a, uint32_t b){
                return std::make_pair(a, b);
                }
                );

        const uint32_t val = std::accumulate(result.begin(), result.end(), 0,
                [](int a, paire b)
                {
                return ( a + (b.first * std::pow(2, b.second)) );
                }
                ); 

        assert(val > GetHeaderSize<uint32_t>());

        return val;

#if 0
        if(buffer.size() >= GetHeaderSize<uint32_t>())
        {
            auto val = buffer[6] * std::pow(2, 21);

            val += buffer[7] * std::pow(2, 14);
            val += buffer[8] * std::pow(2, 7);
            val += buffer[9] * std::pow(2, 0);

            if(val > GetHeaderSize<uint32_t>()){
                return val;
            }
        } else
        {
        }

        return {};
#endif
    }

    std::optional<uint32_t> GetHeaderAndTagSize(const UCharVec& buffer)
    {
        return mbind(GetTagSize(buffer), [=](const uint32_t Tagsize){
                return (Tagsize + GetHeaderSize<uint32_t>());
                });
    }

    const auto GetTagSizeExclusiveHeader(const UCharVec& buffer)
    {
        return mbind(GetTagSize(buffer), [](const int tag_size){
                return tag_size - GetHeaderSize<uint32_t>();}
                );
    }

    std::optional<UCharVec> GetHeader(const std::string& FileName )
    {
        const auto val = GetStringFromFile(FileName, GetHeaderSize<uint32_t>());

        if(!val.has_value()){
            std::cerr << "error " << __func__ << std::endl;
        }

        return val;
    }

    const std::optional<std::string> GetTagArea(const UCharVec& buffer)
    {
        return  GetHeaderAndTagSize(buffer)
            | [&](uint32_t tagSize)
            {
#ifdef DEBUG
                std::cout << "tag__size: " << tagSize << std::endl;
#endif
                return ExtractString<uint32_t, uint32_t>(buffer, 0, tagSize);
            };
    }

    template <typename id3Type>
        void GetTagNames(void)
        {
            return id3Type::tag_names;
        };

    const std::optional<std::string> GetID3FileIdentifier(const UCharVec& buffer)
    {
        constexpr auto FileIdentifierStart = 0;
        constexpr auto FileIdentifierEnd = 3;

        return ExtractString<uint32_t, uint32_t>(buffer, FileIdentifierStart, FileIdentifierEnd);

#ifdef __TEST_CODE
        auto y = [&buffer] () -> std::string {
            return Get_ID3_header_element(buffer, 0, 3);
        };
        return y();
#endif
    }

    const std::optional<std::string> GetID3Version(const UCharVec& buffer)
    {
        constexpr auto kID3IndexStart = 4;
        constexpr auto kID3VersionBytesLength = 2;

        return GetHexFromBuffer<uint8_t>(buffer, kID3IndexStart, kID3VersionBytesLength);
    }

    template <typename Type>
        class search_tag
        {
            const Type& mTagArea;

            public:

            search_tag(const Type& tagArea):
                mTagArea(tagArea)
            {
            }

            std::optional<uint32_t> operator()(Type tag)
            {
                const auto it = std::search(mTagArea.cbegin(), mTagArea.cend(), 
                        std::boyer_moore_searcher(tag.cbegin(), tag.cend()) );

                if(it != mTagArea.cend()){
                    return (it - mTagArea.cbegin());
                }else{
                    std::cerr << "tag: "  << tag << " not found" << std::endl;
                    //return {};
                    return std::nullopt;
                }
            }
        };

}; //end namespace id3v2




#endif //ID3V2_BASE
