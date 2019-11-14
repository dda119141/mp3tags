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

template <typename T, typename F>
auto mbind(std::optional<T> obj, F&& f)
	-> decltype(f(obj.value()))
{
    auto fuc = std::forward<F>(f);
	if(obj){
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

template <typename T>
std::optional<std::vector<char>> GetStringFromFile(const T& FileName, auto num )
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
        std::cerr << "error start: " << start << " end: " << end << std::endl;
		return {};
	}
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


constexpr auto GetHeaderSize(void)
{
	return 10;
}

constexpr auto FrameIDSize(void)
{
	return 4;
}

std::optional<std::vector<char>> GetHeader(const std::string& FileName )
{
	auto val = GetStringFromFile(FileName, GetHeaderSize());

	return val;
}

std::optional<unsigned int> GetTagSize(const std::vector<char>& buffer)
{
	if(buffer.size() >= GetHeaderSize())
	{
		auto val = buffer[6] * std::pow(2, 21);

		val += buffer[7] * std::pow(2, 14);
		val += buffer[8] * std::pow(2, 7);
		val += buffer[9] * std::pow(2, 0);

		if(val > GetHeaderSize()){
			return val;
		}
	} else
	{
	}

	return {};
}

std::optional<unsigned int> GetHeaderAndTagSize(const std::vector<char>& buffer)
{
	return mbind(GetTagSize(buffer), [=](const uint32_t Tagsize){
            return (Tagsize + GetHeaderSize());
            });
}


std::optional<unsigned int> GetFrameSize(const std::vector<char>& buffer, uint32_t index)
{
	const auto start = FrameIDSize() + index;

	if(buffer.size() >= start)
	{
		auto val = buffer[start + 0] * std::pow(2, 24);

		val += buffer[start + 1] * std::pow(2, 16);
		val += buffer[start + 2] * std::pow(2, 8);
		val += buffer[start + 3] * std::pow(2, 0);

		return val;

	} else	{
            std::cout << __func__ << ": Error " << buffer.size() << " start: " << start << std::endl;
	}

	return {};
}

const auto GetTagSizeExclusiveHeader(const std::vector<char>& buffer)
{
	return mbind(GetTagSize(buffer), [](const int tag_size){
		return tag_size - GetHeaderSize();}
		);
}

template <typename T>
T cond_return(T&& obj)
{
	T _obj = std::forward<T>(obj);

	if(_obj.has_value()){
		return _obj;
	}else{
		return {};
	}
}

template <typename T>
const std::optional<std::string> GetHeaderElement(const std::vector<char>& buffer, T start, T end)
{
			return ExtractString<T, unsigned int>(buffer, start, end);
}

#if 0
template <typename F, typename... Args>
const std::optional<std::string> GetArea(const std::vector<char>& buffer, uint32_t start, F fuc, Args&&... args)
{
	return mbind(fuc(buffer, std::forward<Args>(args)...), [&](const int& tag_size)
		{ 
			return ExtractString<unsigned int, unsigned int>(buffer, 
			    start, start + tag_size);
    });
}
#else

const std::optional<std::string> GetArea(const std::vector<char>& buffer, uint32_t start, uint32_t tag_size)
{
			return ExtractString<unsigned int, unsigned int>(buffer, 
			    start, start + tag_size);
}
#endif

const std::optional<std::string> GetTagArea(const std::vector<char>& buffer)
{
//	return mbind(GetTagSize(buffer), [&](uint32_t _size)
	return mbind(GetHeaderAndTagSize(buffer), [&](uint32_t _size)
            { 
                return GetArea(buffer, 0, _size); 
            });
}

auto GetFrameArea(uint32_t start)
{
    return ([=](const std::vector<char>& buffer)
    {
        const auto offset = start + GetHeaderSize();
      //  std::cout<< " GetFrameArea**: " << start << " size: " << GetFrameSize(buffer, start).value() << std::endl;
	    return mbind(GetFrameSize(buffer, start), [&](uint32_t _size)
                    {
                        return GetArea(buffer, offset, _size);
                    });
    });
}

const auto GetID3FileIdentifier(const std::vector<char>& buffer)
{
	return GetHeaderElement(buffer, 0, 3).value();

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

template <typename type>
class RetrieveTag
{
	std::string FileName;

	public:

	RetrieveTag(std::string filename)
	{
		FileName = filename;
	}

	const std::optional<std::string> operator() (const type& tag)
	{
		using namespace id3v2;
		using namespace ranges;

		const auto tags_size = mbind(GetHeader(FileName), GetTagSize).value();
		
		auto buffer = GetStringFromFile(FileName, tags_size + GetHeaderSize() );
		auto TagIndex = mbind( 
				mbind(buffer, [](const std::vector<char>& buffer)
					{ 
						return GetTagArea(buffer);
					}), 
				[&tag](const std::string& tagArea)
				{ 
					auto searchTag = search_tag<std::string>(tagArea);
					return searchTag(tag);
				});


		if(TagIndex.has_value()){
           // std::cout << "Retrieve tag : " << tag << " at position: " << std::dec << TagIndex.value() << std::endl;

            auto frameArea = GetFrameArea(TagIndex.value());
            auto res = mbind(buffer, frameArea);

            if(res.has_value()){
               // std::cout << "Tag content: " << res.value() << std::endl;
                return res.value();
            }
		}

		return {} ;
	
	}
};

const std::optional<std::string> GetAlbum(const std::string filename)
{
    const std::vector<std::string> tags{
	    "TALB" //     [#TALB Album/Movie/Show title]
    };
	
    RetrieveTag<std::string> obj(filename);

    return obj("TALB");

//	std::for_each(tags.cbegin(), id3v2::tags.cend(), 
//			id3v2::RetrieveTag<std::string>(std::string(filename)) );
}

const auto is_tag(std::string name)
{
    return (
        std::any_of(tag_names.cbegin(), tag_names.cend(), 
        [&](std::string obj) { return name == obj; } ) 
        );
}



}; //id3v2
#endif //_TAG_READER
