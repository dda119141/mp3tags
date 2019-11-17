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


void print_header_infos(const std::string& filename)
{
	using std::cout;
	using std::endl;

	auto bufferO = id3v2::GetHeader(filename);

	if (!bufferO.has_value()) {
		cout << "file identifier: \n";
		return;
	}

	auto buffer = id3v2::GetHeader(filename);

	int res = buffer | [](const std::vector<char>& buff)
	{
		std::for_each(buff.cbegin(), buff.cend(), [](const char& n) { std::cout << std::hex << (int)n << ' '; });
		cout << endl;
		return 0;
	};

	int resi = buffer | [](const std::vector<char>& buff)
	{
		cout << "file identifier: " << id3v2::GetID3FileIdentifier(buff).value() << endl;
		cout << "version: " << id3v2::GetID3Version(buff).value() << endl;
		cout << "Tag size: " << std::dec << id3v2::GetTagSize(buff).value() << endl;
		cout << "HeaderTag size: " << std::dec << id3v2::GetHeaderAndTagSize(buff).value() << endl;

		return 0;
	};

}

int main() {
	using std::cout;
	using std::endl;

    const std::string filename = "..\\file_example_MP3_700KB.mp3";
	print_header_infos(filename);

	cout << "Album: " << GetAlbum(filename).value() << endl;
	//std::for_each(id3v2::tag_names.cbegin(), id3v2::tag_names.cend(), 
	//		id3v2::RetrieveTag<std::string>(std::string("../file_example_MP3_700KB.mp3")) );


	//printf("gcc version: %d.%d.%d\n",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);


	return 0;
}

