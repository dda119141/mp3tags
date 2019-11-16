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

void print_header_infos(const std::string& filename)
{
	using std::cout;
	using std::endl;

	auto buffer = id3v2::GetHeader(filename).value();

	std::for_each(buffer.begin(), buffer.end() , [](char &n) { std::cout << std::hex << (int)n << ' '; } );
	cout << endl;

	cout << "file identifier: " << id3v2::GetID3FileIdentifier(buffer) << endl;
	cout << "version: " << id3v2::GetID3Version(buffer).value() << endl;
	cout << "Tag size: " << std::dec << id3v2::GetTagSize(buffer).value() << endl;
	cout << "HeaderTag size: " << std::dec << id3v2::GetHeaderAndTagSize(buffer).value() << endl;

}

int main() {
	using std::cout;
	using std::endl;

    const std::string filename = "../file_example_MP3_700KB.mp3";
	print_header_infos(filename);

	cout << "Album: " << GetAlbum(filename).value() << endl;
	//std::for_each(id3v2::tag_names.cbegin(), id3v2::tag_names.cend(), 
	//		id3v2::RetrieveTag<std::string>(std::string("../file_example_MP3_700KB.mp3")) );


	printf("gcc version: %d.%d.%d\n",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);


	return 0;
}

