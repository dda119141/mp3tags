//============================================================================
// Name        : Threading.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <memory>
#include <fstream>
#include <vector>
#include <algorithm>
#include <optional>
#include <time.h>

#define __MP3

template <unsigned int T>
std::optional<std::vector<char>> Get_string_from_file(const std::string& _filename )
{
	std::ifstream fil(_filename);
	std::vector<char> buffer(T, '0');

	if(!fil.good()){
		std::cerr << "mp3 file not good\n";
		return {};
	}

	fil.read(buffer.data(), T);

	return buffer;
}

auto Get_ID3v2_Header(const std::string& _filename )
{
	return Get_string_from_file<10>(_filename );
}

auto Get_ID3v2_Tag_Size(const std::string& _filename )
{
	//TODO compute size
	return Get_string_from_file<10>(_filename );
}

int main() {

	using std::cout;
	using std::endl;

#ifdef __MP3

	std::cout << "Start: \n";

	auto buffer = Get_ID3v2_Header("file_example_MP3_700KB.mp3").value();

	std::for_each(buffer.begin(), buffer.end() , [](char &n) { std::cout << std::dec << n << ' '; } );
	std::cout << std::endl;

	std::for_each(buffer.begin(), buffer.end() , [](char &n) { std::cout << std::hex << (int)n << ' '; } );
	std::cout << std::endl;

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

clock_t t = clock();

while( (clock() - t) * 1000/(CLOCKS_PER_SEC) < 0);

printf("hjjjjjj\n");
#endif
	return 0;
}

