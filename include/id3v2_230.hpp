#ifndef _ID3V2_230
#define _ID3V2_230

#include <id3v2_base.hpp>

namespace id3v2
{

struct v230
{
    public:
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

        constexpr auto FrameIDSize(void)
        {
            return ::id3v2::RetrieveSize(4);
        }

        constexpr auto FrameHeaderSize(void)
        {
            return ::id3v2::RetrieveSize(10);
        }


        std::optional<uint32_t> GetFrameSize(const std::vector<char>& buffer, uint32_t index)
        {
            const auto start = FrameIDSize() + index;

            //    std::cout << __func__ << ": index " << index << " start: " << start << std::endl;
            if(buffer.size() >= start)
            {
                uint32_t val = buffer[start + 0] * std::pow(2, 24);

                val += buffer[start + 1] * std::pow(2, 16);
                val += buffer[start + 2] * std::pow(2, 8);
                val += buffer[start + 3] * std::pow(2, 0);

                return val;

            } else	{
                std::cout << __func__ << ": Error " << buffer.size() << " start: " << start << std::endl;
            }

            return {};
        }

}; //v230

};
#endif //_ID3V2_230
