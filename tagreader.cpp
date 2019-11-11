//============================================================================
// Name        : Threading.cpp
// Author      : Maxime Moge
// Version     :
// Copyright   : Your copyright notice
// Description : tag reader, tad writer in C++ > c++14
//============================================================================

#include <iostream>
#include <memory>
#include <time.h>
#include "tagreader.hpp"

#define __MP3

const std::vector<std::string> ID3v2_frame_names{
	"TALB", "TBPM"
	,"TCOM", "TCON"
	,"TCOP", "TDAT"
	,"TCOP", "TDLY"
	,"TENC", "TEXT"
	,"TFLT", "TIME"
	,"TIT1", "TIT2"
	,"TIT3", "TKEY"
};

int main() {

	using std::cout;
	using std::endl;

#ifdef __MP3

#if 0
	auto buffer = id3v2::GetHeader("file_example_MP3_700KB.mp3").value();

	std::for_each(buffer.begin(), buffer.end() , [](char &n) { std::cout << std::hex << (int)n << ' '; } );
	cout << std::endl;

	cout << "file identifier: " << id3v2::GetID3FileIdentifier(buffer) << std::endl;
	cout << "version: " << id3v2::GetID3Version(buffer) << std::endl;
	cout << "Tag size: " << id3v2::GetTagSize(buffer) << std::endl;
#endif

#if 0
for (auto& tagi : id3v2::tag_names){
	auto tag = id3v2::retrieve_tag<std::string>(tagi, std::string("file_example_MP3_700KB.mp3") );
	if(tag.has_value())
		cout << "Retrieve tag : " << tagi << " at position: " << tag.value() << std::endl;
}
#endif

std::for_each(id3v2::tag_names.cbegin(), id3v2::tag_names.cend(), 
	id3v2::RetrieveTag<std::string>(std::string("file_example_MP3_700KB.mp3")) );
	

#else

printf("gcc version: %d.%d.%d\n",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);

#define IPC_CID_AUTOUPDRPC 1
#define IPC_LUN_AUTOUPD 1
static const unsigned int update_channel = (unsigned int) ((IPC_CID_AUTOUPDRPC & 0x0000007F) | (unsigned int)((IPC_LUN_AUTOUPD << 7) & 0x0000FFF0));

std::cout << "channel: " << update_channel << std::endl;

#ifdef SAMPLE2
	char buf1[4] = {0x5, 0x48, 0x66, 0x57};
	char buf2[50];

	int j=0;
	int i = 0;
	int ind = i;

	for(int i = 0; i < 4; i++)
	{
		j += sprintf(buf2 + j, "0x%x ", buf1[i]);
		//ind += j;
		std::cout << "hhh" << j << std::endl;
	}

	std::cout << buf2 << std::endl;
#endif

#endif

	return 0;
}

