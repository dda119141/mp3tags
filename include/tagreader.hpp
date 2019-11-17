#ifndef _TAG_READER
#define _TAG_READER

#include <id3v2_common.hpp>
#include <id3v2_230.hpp>
#include <id3v2_000.hpp>

template <typename type>
class RetrieveTag
{
    using iD3Variant = std::variant <id3v2::v230, id3v2::v00>;
	std::string FileName;

	public:

	RetrieveTag(std::string filename)
	{
		std::call_once(m_once, [&filename, this]
			{
				using namespace id3v2;

				FileName = filename;
				const auto tags_size = GetHeader(FileName) | GetTagSize;

				buffer = GetHeader(FileName)
					| GetTagSize
					| [&](uint32_t tags_size)
				{
					return GetStringFromFile(FileName, tags_size + GetHeaderSize<uint32_t>());
				};

			});
	}

    template <typename id3Variant>
	//const std::optional<std::string> operator() (const type& tag, const id3Variant& TagVersion)
	const std::optional<std::string> find_tag(const type& tag, id3Variant& TagVersion)
	{
        using namespace id3v2;
        using namespace ranges;

        auto res = buffer
			| [](const std::vector<char>& buffer)
            { 
				return GetTagArea(buffer);
            }
			| [&tag](const std::string& tagArea)
            { 
				auto searchTagPosition = search_tag<std::string>(tagArea);
				return searchTagPosition(tag);
			}
			| [&](const uint32_t& TagIndex)
			{
				//std::cout<< " GetFrameArea**: " << start << " size: " << tagversion.GetFrameSize(buffer, start).value() << std::endl;
				return buffer 
				| [&TagIndex, &TagVersion](const std::vector<char>& buff)
				{
					return TagVersion.GetFrameSize(buff, TagIndex);
				}
				| [&](uint32_t FrameSize)
				{
					const auto tag_offset = TagIndex + TagVersion.FrameHeaderSize();
					
					return buffer
							| [&tag_offset, &FrameSize](const std::vector<char>& buff)
							{
								return GetArea(buff, tag_offset, FrameSize);
							};
				};
			};
        

        return res;
	
	}


#if 0
	//const std::optional<std::string> operator() (const type& tag, const id3Variant& TagVersion)
	const std::optional<std::string> find_tag__(const type& tag, id3Variant& TagVersion)
	{
		using namespace id3v2;
		using namespace ranges;

		const auto tags_size = GetHeader(FileName) | GetTagSize;

		auto buffer = GetHeader(FileName)
			| GetTagSize
			| [&](uint32_t tags_size)
		{
			return GetStringFromFile(FileName, tags_size + GetHeaderSize<uint32_t>());
		};

		auto TagIndex = mbind(
			(buffer |
				[](const std::vector<char>& buffer)
				{
					return GetTagArea(buffer);
				}
				),
			[&tag](const std::string& tagArea)
				{
					auto searchTag = search_tag<std::string>(tagArea);
					return searchTag(tag);
				});


		if (TagIndex.has_value()) {

			// std::cout << "Retrieve tag : " << tag << " at position: " << std::dec << TagIndex.value() << std::endl;
			auto frameArea = GetFrameArea(TagIndex.value(), TagVersion);
			auto res = buffer | frameArea;

			if (res.has_value()) {
				// std::cout << "Tag content: " << res.value() << std::endl;
				return res.value();
			}
		}

		return {};

	}
#endif

	private:
		mutable std::once_flag m_once;
		std::optional<std::vector<char>> buffer;
};

template <typename T, typename E>
const std::optional<E> GetTag(std::variant <id3v2::v230, id3v2::v00> &tagversion, const std::string filename, 
	const std::string tagname)
{
	if (auto TagVersion = std::get_if<T>(&tagversion))
	{
		auto tagVersion = std::get<T>(tagversion);

		RetrieveTag<E> obj(filename);

		return obj.find_tag<T>(tagname, tagVersion);
	}
	else
	{
		return {};
	}

}

std::optional<std::string> check_for_ID3(const std::vector<char>& buffer)
{
	auto tag = id3v2::GetID3FileIdentifier(buffer);
	if (tag != "ID3") {
		return {};
	}
	else {
		return tag;
	}
}

const std::optional<std::string> GetAlbum(const std::string filename)
{
	std::variant <id3v2::v230, id3v2::v00> tagversion;

	auto header_buffer = id3v2::GetHeader(filename);
	auto checkID3 = [](const std::vector<char>& buf)
	{
		return check_for_ID3(buf);
	};

	auto ID3_name = header_buffer | checkID3;

	return header_buffer
	| [](const std::vector<char>& buffer)
	{
		return id3v2::GetID3Version(buffer);
	}
	|
	[&](const std::string& id3Version) {
		if (id3Version == "0x0300")
		{
			return GetTag<id3v2::v230, std::string>(tagversion, filename, "TALB");
		}
		else if (id3Version == "0x0000")
		{
			return GetTag<id3v2::v00, std::string>(tagversion, filename, "TAL");
		}
	};
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
