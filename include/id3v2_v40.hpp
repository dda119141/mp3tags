#ifndef _ID3V2_V40
#define _ID3V2_V40

#include <id3v2_base.hpp>

namespace id3v2
{

class v40
{
    public:
#if 0
        const std::vector<std::string> tag_names{
            "TALB" //     [#TALB Album/Movie/Show title]
                ,"TBPM" //     [#TBPM BPM (beats per minute)]
                ,"TCOM" //     [#TCOM Composer]
                ,"TCON" //     [#TCON Content type]
                ,"TCOP" //     [#TCOP Copyright message]
                ,"TDRC" //     [#TDRC release time]
                ,"TDLY" //     [#TDLY Playlist delay]
                ,"TENC" //     [#TENC Encoded by]
                ,"TEXT" //     [#TEXT Lyricist/Text writer]
                ,"TFLT" //     [#TFLT File type]
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
                ,"TDOR"//      [#TDOR Original release time]
                ,"TOWN"//      [#TOWN File owner/licensee]
                ,"TPE1"//      [#TPE1 Lead performer(s)/Soloist(s)]
                ,"TPE2"//      [#TPE2 Band/orchestra/accompaniment]
                ,"TPE3"//      [#TPE3 Conductor/performer refinement]
                ,"TPE4"//      [#TPE4 Interpreted, remixed, or otherwise modified by]
                ,"TPOS"//      [#TPOS Part of a set]
                ,"TPUB"//      [#TPUB Publisher]
                ,"TRCK"//      [#TRCK Track number/Position in set]
                ,"TRSN"//      [#TRSN Internet radio station name]
                ,"TRSO"//      [#TRSO Internet radio station owner]
                ,"TSRC"//      [#TSRC ISRC (international standard recording code)]
                ,"TSSE"//      [#TSEE Software/Hardware and settings used for encoding]
                ,"ASPI"//      [#Audio Seek Point]
                ,"EQU2"//        Equalisation (2) [F:4.12]
                ,"RVA2"//       Relative volume adjustment (2) [F:4.11]
                ,"SEEK"//        Seek frame [F:4.29]
                ,"WCOM"//      Commercial information
                ,"WCOP"// Copyright/Legal information
                ,"WOAF"// Official audio file webpage
                ,"WOAR"// Official artist/performer webpage
                ,"WOAS"// Official audio source webpage
                ,"WORS"// Official Internet radio station homepage
                ,"WPAY"// Payment
                ,"WPUB"// Publishers official webpage
                ,"WXXX"// User defined URL link frame

        };
#endif
        constexpr auto FrameIDSize(void)
        {
            return ::id3v2::RetrieveSize(4);
        }

        constexpr auto FrameHeaderSize(void)
        {
            return ::id3v2::RetrieveSize(10);
        }


        std::optional<uint32_t> GetFrameSize(const UCharVec& buffer, uint32_t index)
        {
            const auto start = FrameIDSize() + index;

            //    std::cout << __func__ << ": index " << index << " start: " << start << std::endl;
            if(buffer.size() >= start)
            {
                using paire = std::pair<uint32_t, uint32_t>;

                const std::vector<uint32_t> pow_val = { 24, 16, 8, 0 };

                std::vector<paire> result(pow_val.size());

                std::transform(std::begin(buffer) + start, std::begin(buffer) + start + pow_val.size(),
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

                return val;

            } else	{
                std::cout << __func__ << ": Error " << buffer.size() << " start: " << start << std::endl;
            }

            return {};
        }

}; //v40

};
#endif //ID3V2_V40
