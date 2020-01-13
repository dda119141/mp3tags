#ifndef _ID3V2_000
#define _ID3V2_000

#include <id3v2_base.hpp>

namespace id3v2
{

class v00
{
    public:
#if 0
        std::vector<std::string> tag_names{
            "BUF" // Recommended buffer size
                ,"CNT " //Play counter
                ,"COM " //Comments
                ,"CRA " //Audio encryption
                ,"CRM " //Encrypted meta frameWow! All encryption related frames begins with "CR"
                ,"ETC " //Event timing codes
                ,"EQU" //Equalization
                ,"GEO" //General encapsulated object
                ,"IPL" //Involved people list
                ,"LNK" //Linked information
                ,"MCI " //Music CD Identifier
                ,"MLL" //MPEG location lookup table
                ,"PIC " //Attached picture
                ,"POP" //Popularimeter
                ,"REV " //Reverb
                ,"RVA" //Relative volume adjustment
                ,"SLT " //Synchronized lyric/text
                ,"STC" //Synced tempo codes
                ,"TAL " //Album/Movie/Show title
                ,"TBP " //BPM (Beats,"Per " //Minute)
                ,"TCM " //Composer
                ,"TCO " //Content type
                ,"TCR " //Copyright message
                ,"TDA " //Date
                ,"TDY " //Playlist delay
                ,"TEN " //Encoded by
                ,"TFT " //File type
                ,"TIM " //Time
                ,"TKE " //Initial key
                ,"TLA " //Language(s)
                ,"TLE " //Length
                ,"TMT " //Media type
                ,"TOA " //Original artist(s)/performer(s)
                ,"TOF " //Original filename
                ,"TOL " //Original Lyricist(s)/text writer(s)
                ,"TOR " //Original release year
                ,"TOT " //Original album/Movie/Show title
                ,"TP1 " //Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group
                ,"TP2 " //Band/Orchestra/Accompaniment
                ,"TP3 " //Conductor/Performer refinement
                ,"TP4 " //Interpreted, remixed, or otherwise modified by
                ,"TPA " //Part of a set
                ,"TPB " //Publisher
                ,"TRC " //ISRC (International Standard Recording Code)
                ,"TRD " //Recording dates
                ,"TRK " //Track number/Position in set
                ,"TSI " //Size
                ,"TSS " //Software/hardware,"and " //settings used,"for " //encoding
                ,"TT1 " //Content group description
                ,"TT2 " //Title/Songname/Content description
                ,"TT3 " //Subtitle/Description refinement
                ,"TXT " //Lyricist/text writer
                ,"TXX " //User defined text information frame
                ,"TYE" //Year
                ,"UFI " //Unique file identifier
                ,"ULT" //Unsychronized lyric/text transcription
                ,"WAF " //Official audio file webpage
                ,"WAR " //Official artist/performer webpage
                ,"WAS " //Official audio source webpage
                ,"WCM " //Commercial information
                ,"WCP " //Copyright/Legal information
                ,"WPB " //Publishers official webpage
                ,"WXX " //User defined URL link frame
        };
#endif
        constexpr auto FrameIDSize(void)
        {
            return ::id3v2::RetrieveSize(3);
        }

        constexpr auto FrameHeaderSize(void)
        {
            return ::id3v2::RetrieveSize(6);
        }

        std::optional<uint32_t> GetFrameSize(const UCharVec& buffer, uint32_t index)
        {
            const auto start = FrameIDSize() + index;

            if(buffer.size() >= start)
            {
                auto val = buffer[start + 0] * std::pow(2, 16);

                val += buffer[start + 1] * std::pow(2, 8);
                val += buffer[start + 2] * std::pow(2, 0);

                return val;

            } else	{
                std::cout << __func__ << ": Error " << buffer.size() << " start: " << start << std::endl;
            }

            return {};
        }

        expected::Result<bool> UpdateFrameSize(UCharVec& buffer, uint32_t extraSize, uint32_t tagLocation)
        {
            const uint32_t TagIndex = 3 + tagLocation;
            constexpr uint32_t NumberOfElements = 3;
            constexpr uint32_t maxValue = 127;
            auto it = std::begin(buffer) + TagIndex;
            auto ExtraSize = extraSize;
            auto extr = ExtraSize % 127;

            /* reverse order of elements */
            std::reverse(it, it + NumberOfElements);

            std::transform(
                it, it + NumberOfElements, it, it, [&](uint32_t a, uint32_t b) {
                    extr = ExtraSize % maxValue;
                    a = (a >= maxValue) ? maxValue : a;

                    if (ExtraSize >= maxValue) {
                        const auto rest = maxValue - a;
                        a = a + rest;
                        ExtraSize -= rest;
                    } else {
                        auto rest2 = maxValue - a;
                        a = (a + ExtraSize > maxValue) ? maxValue : (a + ExtraSize);
                        ExtraSize = ((int)(ExtraSize - rest2) < 0)
                                        ? 0
                                        : (ExtraSize - rest2);
                    }
                    return a;
                });

            /* reverse back order of elements */
            std::reverse(it, it + NumberOfElements);

            return expected::makeValue<bool>(true);

        }



}; //v00

}; //id3v2
#endif //_ID3V2_000
