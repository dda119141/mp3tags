#ifndef _TAG_READER
#define _TAG_READER

#include <id3v2_common.hpp>
#include <id3v2_230.hpp>
#include <id3v2_000.hpp>

template <typename type>
class RetrieveTag
{
	std::string FileName;

	public:

    std::variant <id3v2::v230, id3v2::v00> tagversion;

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
#if 1
            auto TagVersion = std::get<id3v2::v230> (tagversion);
#else
            std::visit(overloaded { 
                    [](id3v2::v230 obj)
                    {
                        id3v2::v230 TagVersion = obj;
                    },
                    [](id3v2::v00 obj)
                    {
                        id3v2::v00 TagVersion = obj;
                    },
                    }, tagversion);
#endif

            auto frameArea = GetFrameArea(TagIndex.value(), TagVersion);
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

template <typename id3Type>
const auto is_tag(std::string name)
{
    return (
        std::any_of(id3v2::GetTagNames<id3Type>().cbegin(), id3v2::GetTagNames<id3Type>().cend(), 
        [&](std::string obj) { return name == obj; } ) 
        );
}


#endif //_TAG_READER
