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
#include <boost/locale.hpp>


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

    int res = buffer | [](const std::vector<unsigned char>& buff)
    {
        std::for_each(buff.cbegin(), buff.cend(), [](const char& n) { std::cout << std::hex << (int)n << ' '; });
        cout << endl;
        return 0;
    };

    assert(res == 0);

    int resi = buffer | [](const std::vector<unsigned char>& buff)
    {
        cout << "file identifier: " << id3v2::GetID3FileIdentifier(buff).value() << endl;
        cout << "version: " << id3v2::GetID3Version(buff).value() << endl;
        cout << "Tag size: " << std::dec << id3v2::GetTagSize(buff).value() << endl;
        cout << "HeaderTag size: " << std::dec << id3v2::GetHeaderAndTagSize(buff).value() << endl;

        return 0;
    };

    assert(resi == 0);
}

int main() {
    using std::cout;
    using std::endl;

#if 0
    //std::string ccont = "Hello";
    std::string ccont = "test1";

    //const auto cContent = tagBase::utf8ToUtf16String(ccont);
    const auto cContent = tagBase::getW16FromLatin<std::u16string>(ccont);

    const auto buffer = reinterpret_cast<const char*>(cContent.data());
    for (auto i = 0; i<10; i++)
        cout << std::hex << (int)buffer[i] << ' ';
#endif

    cout <<  endl;
#if 0
    {
        using namespace boost::locale::conv;
        std::ofstream obj("out.bin", std::ios::binary);
        UCharVec obj1 = {0x74, 0x00, 0x65, 0x0, 0x73, 0x00, 0x74, 0x00, 0x31};
        //UCharVec obj2 = {0x72, 0x00, 0x72, 0x0, 0x7A, 0x00, 0x7A};
        std::string exc1 = "test1";
        //std::string exc1(obj1.data(), 9);

        //std::string exc2 = exc1.replace(exc1.begin(), exc1.begin() + 3, 2, 'AA');
        //exc1.replace(exc1.begin(), exc1.begin() + 6, "AA");
     //   std::transform(exc1.begin(), exc1.end(), exc1.begin(),
     //                              [](char c) -> std::string { return (std::string(&c) + "\0") ; });

        cout << "iooooo: " << exc1 << endl;

        //std::u16string exc2 = utf_to_utf<char16_t>(exc1);
        std::string exc2{to_utf<char>(exc1, "ISO-8859-5")};
        //std::string latin1_string = from_utf(wide_string,"UTF-16");
//        std::string exc2 = utf_to_utf<char>(wide_string);
        //std::locale loc (std::locale(), new std::codecvt_utf16<char>);
        //obj << exc1;
        obj << exc2.c_str();

    }
#endif

#if 1
//  print_header_infos(filename);
  try{
    for (auto& filen : fs::directory_iterator("../files/"))
    {
        const std::string filename = filen.path().string();

        cout << "file: " << filename << " Album: " << GetAlbum(filename) << endl;
        cout <<"file: " << filename <<  " Composer: " << GetComposer(filename) << endl;
#if 0
        cout << "Date: " << GetDate(filename) << endl;
        cout << "Year: " << GetYear(filename) << endl;
        cout << "Text writer: " << GetTextWriter(filename) << endl;
        cout << "Content type: " << GetContentType(filename) << endl;
        cout << "File type: " << GetFileType(filename) << endl;
        cout << "Title: " << GetTitle(filename) << endl;
        cout << "Artist: " << GetLeadArtist(filename) << endl;
        cout << "Group Description: " << GetContentGroupDescription(filename) << endl;
#endif
        if(filename.find("test1.mp3") != std::string::npos)
            cout << "\n\n change album: " << SetAlbum(filename, "test1");

        //std::for_each(id3v2::tag_names.cbegin(), id3v2::tag_names.cend(), 
        //      id3v2::RetrieveTag<std::string>(std::string("../file_example_MP3_700KB.mp3")) );

        cout << endl;
    }
  }catch (fs::filesystem_error) {
        cout << "wrong path:" << endl;
  }

#endif
    return 0;
}

