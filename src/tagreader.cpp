//============================================================================
// Name        : Threading.cpp
// Author      : Maxime Moge
// Version     :
// Copyright   : Your copyright notice
// Description : tag reader, tad writer in C++ > c++14
//============================================================================

#include <iostream>
#include <memory>
#include <experimental/filesystem>
#include <time.h>
#include "tagreader.hpp"
#include "tagwriter.hpp"


namespace fs = std::experimental::filesystem;

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

//  print_header_infos(filename);
    for (auto& filen : fs::directory_iterator("../files/"))
    {
        const std::string filename = filen.path().string();

        cout << "Album: " << GetAlbum(filename) << endl;
#if 1
        cout << "Composer: " << GetComposer(filename) << endl;
        cout << "Date: " << GetDate(filename) << endl;
        cout << "Year: " << GetYear(filename) << endl;
        cout << "Text writer: " << GetTextWriter(filename) << endl;
        cout << "Content type: " << GetContentType(filename) << endl;
        cout << "File type: " << GetFileType(filename) << endl;
        cout << "Title: " << GetTitle(filename) << endl;
        cout << "Artist: " << GetLeadArtist(filename) << endl;
        cout << "Group Description: " << GetContentGroupDescription(filename) << endl;
#endif
        //std::for_each(id3v2::tag_names.cbegin(), id3v2::tag_names.cend(), 
        //      id3v2::RetrieveTag<std::string>(std::string("../file_example_MP3_700KB.mp3")) );


        //printf("gcc version: %d.%d.%d\n",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);
        cout << endl;
    }

    return 0;
}

