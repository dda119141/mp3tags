// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _ID3V2_000
#define _ID3V2_000

#include "id3.hpp"
namespace id3v2 {

class v00 {
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
  constexpr unsigned int FrameIDSize(void) { return 3; }

  constexpr unsigned int FrameHeaderSize(void) { return 6; }

  std::optional<uint32_t> GetFrameSize(id3::buffer_t buffer, uint32_t index) {
    const auto start = FrameIDSize() + index;

    if (buffer->size() >= start) {
      auto val = buffer->at(start + 0) * std::pow(2, 16);

      val += buffer->at(start + 1) * std::pow(2, 8);
      val += buffer->at(start + 2) * std::pow(2, 0);

      return val;

    } else {
      ID3_LOG_ERROR("failed..: {} ..", start);
    }

    return {};
  }

  std::optional<id3::buffer_t> UpdateFrameSize(id3::buffer_t buffer,
                                               uint32_t extraSize,
                                               uint32_t tagLocation) {

    const uint32_t frameSizePositionInArea = 3 + tagLocation;
    constexpr uint32_t frameSizeLengthInArea = 3;
    constexpr uint32_t frameSizeMaxValuePerElement = 127;

    return id3::updateAreaSize<uint32_t>(
        buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
        frameSizeMaxValuePerElement);
  }

}; // v00

};     // namespace id3v2
#endif //_ID3V2_000
