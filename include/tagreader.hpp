#ifndef _TAG_READER
#define _TAG_READER

#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <cmath>
#include <thread>
//#include <range/v3/all.hpp>
#include <range/v3/view.hpp>
#include <range/v3/action.hpp>
#include <range/v3/action/transform.hpp>
#include <range/v3/view/filter.hpp>


template <typename T>
void integral_unsigned_asserts()
{
	static_assert(std::is_integral<T>::value, "parameter should be integers");
	static_assert(std::is_unsigned<T>::value, "parameter should be unsigned");
}

template <typename T>
std::optional<std::vector<char>> GetStringFromFile(const T& _filename, auto num )
{
	std::ifstream fil(_filename);
	std::vector<char> buffer(num, '0');

	if(!fil.good()){
		std::cerr << "mp3 file not good\n";
		return {};
	}

	fil.read(buffer.data(), num);

	return buffer;
}

template <typename T>
const auto GetValFromBuffer(const std::vector<char>& buffer, T index, T num_of_bytes_in_hex)
{
    integral_unsigned_asserts<T>();

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
const auto GetHexFromBuffer(const std::vector<char>& buffer, T index, T num_of_bytes_in_hex)
{
    integral_unsigned_asserts<T>();

	std::stringstream stream_obj;

	stream_obj << "0x"
		<< std::setfill('0')
		<< std::setw(num_of_bytes_in_hex * 2); 

	auto version = GetValFromBuffer<T>(buffer, index, num_of_bytes_in_hex);

	stream_obj << std::hex << version;

	return stream_obj.str();
}

template <typename T1, typename T2>
const auto CutStringElement(const std::vector<char>& buffer, T1 start, T1 end, T2 num_of_elements=0)
{
	static_assert(std::is_integral<T1>::value, "second and third parameters should be integers");
	static_assert(std::is_unsigned<T2>::value, "num of elements must be unsigned");

	std::string obj(buffer.data(), (num_of_elements==0 ? (end - start) : num_of_elements) );
	const std::string obj2(obj, start, end);

	return obj2;
}


namespace id3v2
{

const std::vector<std::string> tag_names{
	"TALB" //     [#TALB Album/Movie/Show title]
	,"TBPM" //     [#TBPM BPM (beats per minute)]
	,"TCOM" //     [#TCOM Composer]
	,"TCON" //     [#TCON Content type]
	,"TCOP" //     [#TCOP Copyright message]
	,"TDAT" //     [#TDAT Date]
	,"TDLY" //     [#TDLY Playlist delay]
	,"TENC" //     [#TENC Encoded by]
	,"TEXT" //     [#TEXT Lyricist/Text writer]
	,"TFLT" //     [#TFLT File type]
	,"TIME" //     [#TIME Time]
	,"TIT1" //     [#TIT1 Content group description]
	,"TIT2" //     [#TIT2 Title/songname/content description]
	,"TIT3" //     [#TIT3 Subtitle/Description refinement]	
	,"TLAN" //    [#TLAN Language(s)]
	,"TLEN"//      [#TLEN Length]
	,"TMED"//      [#TMED Media type]
	,"TOAL"//      [#TOAL Original album/movie/show title]
	,"TOFN"//      [#TOFN Original filename]
	,"TOLY"//      [#TOLY Original lyricist(s)/text writer(s)]
	,"TOPE"//      [#TOPE Original artist(s)/performer(s)]
	,"TORY"//      [#TORY Original release year]
	,"TOWN"//      [#TOWN File owner/licensee]
	,"TPE1"//      [#TPE1 Lead performer(s)/Soloist(s)]
	,"TPE2"//      [#TPE2 Band/orchestra/accompaniment]
	,"TPE3"//      [#TPE3 Conductor/performer refinement]
	,"TPE4"//      [#TPE4 Interpreted, remixed, or otherwise modified by]
	,"TPOS"//      [#TPOS Part of a set]
	,"TPUB"//      [#TPUB Publisher]
	,"TRCK"//      [#TRCK Track number/Position in set]
	,"TRDA"//      [#TRDA Recording dates]
	,"TRSN"//      [#TRSN Internet radio station name]
	,"TRSO"//      [#TRSO Internet radio station owner]
	,"TSIZ"//      [#TSIZ Size]
	,"TSRC"//      [#TSRC ISRC (international standard recording code)]
	,"TSSE"//      [#TSEE Software/Hardware and settings used for encoding]
	,"TYER"//      [#TYER Year]
};


template <typename T, typename F>
auto mbind(std::optional<T> obj, F f)
	-> decltype(f(obj.value()))
{
	if(obj){
		return f(obj.value());
	}
	else{
		return {};
	}
}

constexpr auto GetHeaderSize(void)
{
	return 10;
}

std::optional<std::vector<char>> GetHeader(const std::string& _filename )
{
	auto val = GetStringFromFile(_filename, GetHeaderSize());

	return val;
}

const auto GetTagSize(const std::vector<char>& buffer)
{
	auto val = buffer[6] * std::pow(2, 21);

	val += buffer[7] * std::pow(2, 14);
	val += buffer[8] * std::pow(2, 7);
	val += buffer[9] * std::pow(2, 0);

	return val;
}

const auto GetTagSizeExclusiveHeader(const std::vector<char>& buffer)
{
	return GetTagSize(buffer) - GetHeaderSize();
}

template <typename T>
const auto GetHeaderElement(const std::vector<char>& buffer, T start, T end)
{
	return CutStringElement<T, unsigned int>(buffer, start, end, GetHeaderSize());
}

template <typename T>
const std::optional<std::string> GetTagArea(const std::vector<char>& buffer)
{
	return CutStringElement<T, unsigned int>(buffer, 
	static_cast<unsigned int>(GetHeaderSize()), static_cast<unsigned int>(GetTagSize(buffer)));
}

const auto GetID3FileIdentifier(const std::vector<char>& buffer)
{
	return GetHeaderElement(buffer, 0, 3);

#ifdef __TEST_CODE
	auto y = [&buffer] () -> std::string {
		return Get_ID3_header_element(buffer, 0, 3);
	};
	return y();
#endif
}

const auto GetID3Version(const std::vector<char>& buffer)
{
	constexpr auto kID3IndexStart = 4;
	constexpr int kID3VersionBytesLength = 2;

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

	std::optional<int> operator()(const Type& tag)
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


template <typename type>
const std::optional<int> retrieve_tag(const type& tag, const type& filename)
{
	auto header = id3v2::GetHeader(filename);
	if(!header.has_value())
		return {};

	auto tags_size = GetTagSize(header.value());

	auto buffer = GetStringFromFile(filename, tags_size );
	if(!buffer.has_value())
		return {};
	
	auto tagArea = GetTagArea<unsigned int>(buffer.value());
	
	auto AreaToSearch = search_tag<std::string>(tagArea);
	auto it = AreaToSearch(tag);

	return it;
}

template <typename type>
class RetrieveTag
{
	std::string _filename;

	public:

	RetrieveTag(std::string filename)
	{
		_filename = filename;
	}

	const std::optional<int> operator() (const type& tag)
	{
		using namespace id3v2;
		using namespace ranges;

		auto tags_size = mbind(GetHeader(_filename), GetTagSize);
		
		auto buffer = GetStringFromFile(_filename, tags_size );
		auto it = mbind( 
				mbind(buffer, [](const std::vector<char>& buffer)
					{ 
						return GetTagArea<unsigned int>(buffer);
					}), 
				[&](const std::string& tagArea)
				{ 
					auto searchTag = search_tag<std::string>(tagArea);
					return searchTag(tag);
				});

		if(it.has_value()){
			std::cout << "Retrieve tag : " << tag << " at position: " << std::dec << it.value() << std::endl;
		}

		return it ;
	
	}
};

auto is_tag(std::string name)
{
    return (
        std::any_of(tag_names.cbegin(), tag_names.cend(), 
        [&](std::string obj) { return name == obj; } ) 
        );
}



}; //id3v2
#endif //_TAG_READER
