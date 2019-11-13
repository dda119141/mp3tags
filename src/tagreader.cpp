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

void print_header_infos(const std::string& filename)
{
	using std::cout;
	using std::endl;

	auto buffer = id3v2::GetHeader(filename).value();

	std::for_each(buffer.begin(), buffer.end() , [](char &n) { std::cout << std::hex << (int)n << ' '; } );
	cout << endl;

	cout << "file identifier: " << id3v2::GetID3FileIdentifier(buffer) << endl;
	cout << "version: " << id3v2::GetID3Version(buffer) << endl;
	cout << "Tag size: " << id3v2::GetTagSize(buffer) << endl;

}

int main() {

#ifdef __MP3
 	print_header_infos("../file_example_MP3_700KB.mp3");
#if 0
for (auto& tagi : id3v2::tag_names){
	auto tag = id3v2::retrieve_tag<std::string>(tagi, std::string("file_example_MP3_700KB.mp3") );
	if(tag.has_value())
		cout << "Retrieve tag : " << tagi << " at position: " << tag.value() << std::endl;
}
#endif

std::for_each(id3v2::tag_names.cbegin(), id3v2::tag_names.cend(), 
	id3v2::RetrieveTag<std::string>(std::string("../file_example_MP3_700KB.mp3")) );
	

printf("gcc version: %d.%d.%d\n",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);

#endif

	return 0;
}

