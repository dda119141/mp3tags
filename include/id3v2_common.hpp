#ifndef _ID3V2_COMMON
#define _ID3V2_COMMON

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


template <typename T, typename F>
auto mbind(std::optional<T> obj, F&& function)
	-> decltype(function(obj.value()))
{
    auto fuc = std::forward<F>(function);
	if(obj){
		return fuc(obj.value());
	}
	else{
		return {};
	}
}


template <typename T>
using is_optional_string = std::is_same<std::decay_t<T>, std::optional<std::string>>;

template <typename T>
using is_optional_vector_char =	std::is_same<std::decay_t<T>, std::optional<std::vector<char>>>;

template <typename T,
	typename = std::enable_if_t<(std::is_same<std::decay_t<T>, std::optional<std::vector<char>>>::value
		|| std::is_same<std::decay_t<T>, std::optional<std::string>>::value
		|| std::is_same<std::decay_t<T>, std::optional<uint32_t>>::value) >
	, typename F >
 auto operator | (T&& obj, F&& Function)
-> decltype(Function(obj.value()))
{
	//static_assert(is_optional<std::vector<char>>::value, "first operand needs to be of type std::optional<std::vector>");
	//static_assert(std::is_same<std::decay_t<T>, std::optional<std::vector<char>>>::value, "std::optional<std::vector>");

	return mbind(std::forward<T>(obj), Function);
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

template <typename T>
std::optional<std::vector<char>> GetStringFromFile(const T& FileName, uint32_t num )
{
	std::ifstream fil(FileName);
	std::vector<char> buffer(num, '0');

	if(!fil.good()){
		std::cerr << "mp3 file not good\n";
		return {};
	}

	fil.read(buffer.data(), num);

	return buffer;
}

namespace id3v2
{

template <typename T>
const auto GetValFromBuffer(const std::vector<char>& buffer, T index, T num_of_bytes_in_hex)
{
    integral_unsigned_asserts<T> eval;

    if(buffer.size() < num_of_bytes_in_hex){
        return 0;
    }
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
const std::optional<std::string> GetHexFromBuffer(const std::vector<char>& buffer, T index, T num_of_bytes_in_hex)
{
    integral_unsigned_asserts<T> eval;

	std::stringstream stream_obj;

	stream_obj << "0x"
		<< std::setfill('0')
		<< std::setw(num_of_bytes_in_hex * 2); 

	auto version = GetValFromBuffer<T>(buffer, index, num_of_bytes_in_hex);

    if(version != 0){

        stream_obj << std::hex << version;

        return stream_obj.str();

    }else{
        return {};
    }
}

template <typename T1, typename T2>
const std::optional<std::string> ExtractString(const std::vector<char>& buffer, T1 start, T1 end)
{
    if(end < start){
        std::cerr << "boundary error - start: " << start << " end: " << end << std::endl;
        return {};

    } else if(buffer.size() >= end) {
		static_assert(std::is_integral<T1>::value, "second and third parameters should be integers");
		static_assert(std::is_unsigned<T2>::value, "num of elements must be unsigned");

		std::string obj(buffer.data(), end);

		return obj.substr(start, (end - start));
	} else {
        std::cerr << __func__ << " error start: " << start << " end: " << end << std::endl;
		return {};
	}
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

std::optional<uint32_t> GetTagSize(const std::vector<char>& buffer)
{
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
}

std::optional<uint32_t> GetHeaderAndTagSize(const std::vector<char>& buffer)
{
	return mbind(GetTagSize(buffer), [=](const uint32_t Tagsize){
            return (Tagsize + GetHeaderSize<uint32_t>());
            });
}

const auto GetTagSizeExclusiveHeader(const std::vector<char>& buffer)
{
	return mbind(GetTagSize(buffer), [](const int tag_size){
		return tag_size - GetHeaderSize<uint32_t>();}
		);
}

std::optional<std::vector<char>> GetHeader(const std::string& FileName )
{
	auto val = GetStringFromFile(FileName, GetHeaderSize<uint32_t>());

	return val;
}


template <typename T>
const std::optional<std::string> GetHeaderElement(const std::vector<char>& buffer, T start, T end)
{
			return ExtractString<T, uint32_t>(buffer, start, end);
}

const std::optional<std::string> GetArea(const std::vector<char>& buffer, uint32_t start, uint32_t tag_size)
{
			return ExtractString<uint32_t, uint32_t>(buffer, 
			    start, start + tag_size);
}

const std::optional<std::string> GetTagArea(const std::vector<char>& buffer)
{
    return  GetHeaderAndTagSize(buffer)
			| [&](uint32_t _size)
            { 
                return GetArea(buffer, 0, _size); 
            };
}

template <typename id3Type>
void GetTagNames(void)
{
    return id3Type::tag_names;
};

template <typename id3Type>
auto GetFrameArea(uint32_t start, id3Type& tagversion)
{
    return ([start, &tagversion](const std::vector<char>& buffer)
    {
        const auto offset = start + tagversion.FrameHeaderSize();
        //std::cout<< " GetFrameArea**: " << start << " size: " << tagversion.GetFrameSize(buffer, start).value() << std::endl;
	    return mbind(tagversion.GetFrameSize(buffer, start), [&](uint32_t _size)
                    {
                        return GetArea(buffer, offset, _size);
                    });
    });
}

const std::optional<std::string> GetID3FileIdentifier(const std::vector<char>& buffer)
{
    constexpr auto FileIdentifierStart = 0;
    constexpr auto FileIdentifierEnd = 3;
	return GetHeaderElement(buffer, FileIdentifierStart, FileIdentifierEnd);

#ifdef __TEST_CODE
	auto y = [&buffer] () -> std::string {
		return Get_ID3_header_element(buffer, 0, 3);
	};
	return y();
#endif
}

const std::optional<std::string> GetID3Version(const std::vector<char>& buffer)
{
	constexpr auto kID3IndexStart = 4;
	constexpr auto kID3VersionBytesLength = 2;

	return GetHexFromBuffer<uint8_t>(buffer, kID3IndexStart, kID3VersionBytesLength);
}

template <typename Type>
class search_tag
{
	const Type& _tagArea;

	public:

	search_tag(const Type& tagArea):
	_tagArea(tagArea)
	{
	}

	std::optional<uint32_t> operator()(const Type& tag)
	{
		auto it = std::search(_tagArea.cbegin(), _tagArea.cend(), 
			std::boyer_moore_searcher(tag.cbegin(), tag.cend()) );
		
		if(it != _tagArea.cend()){
			return (it - _tagArea.cbegin());
		}else{
			return {};
		}
	}
};

}; //end namespace id3v2




#endif //_ID3V2_COMMON
