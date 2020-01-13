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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "tagreader.hpp"
#include "tagwriter.hpp"




TEST_CASE("Read/Write from/to id2v3 tag") {

   namespace fs = std::experimental::filesystem;

    using std::cout;
    using std::endl;

    std::string filename;

    const std::string currentFilePath = fs::system_complete(__FILE__);

   cout << "current file path:" << currentFilePath << endl;
   const std::string mp3Path = (fs::path(currentFilePath).parent_path() /= "../files").string();

    try{
        for (auto& filen : fs::directory_iterator(mp3Path))
        {
            std::string _filename = filen.path().string();

            if(_filename.find("test1.mp3") != std::string::npos){
                filename = _filename;
            }
        }
    }catch (fs::filesystem_error& e) {
        cout << "wrong path:" << e.what() << endl;
    }

    SECTION("test path"){
        REQUIRE(filename == (fs::path(mp3Path) /= "test1.mp3").string());
    }

    SECTION("Test reading album"){
        REQUIRE(GetAlbum(filename) == "Ying Yang Forever");
    }
    SECTION("Test reading title"){
        REQUIRE(GetTitle(filename) == "Top Model");
    }
    SECTION("Test reading artist"){
        REQUIRE(GetLeadArtist(filename) == "Ying Yang Twins");
    }
    SECTION("Test reading Year"){
        REQUIRE(GetYear(filename) == "2009");
    }
    SECTION("Test reading Content type"){
        REQUIRE(GetContentType(filename) == "Rap");
    }
    SECTION("Test reading track position"){
        REQUIRE(GetTrackPosition(filename) == "05/18");
    }
    //    cout << "File type: " << GetFileType(filename) << endl;
    //    cout << "Group Description: " << GetContentGroupDescription(filename) << endl;
    //    cout << "GetTrackPosition: " << GetTrackPosition(filename) << endl;
    /*
    SECTION("Test writing album name: test2"){
        REQUIRE(SetAlbum(filename, "test1") == 1);
    }
    */
    SECTION("Test writing album"){
        REQUIRE(SetAlbum(filename, "testYing") == 1);
    }
    SECTION("Test writing title"){
        REQUIRE(SetTitle(filename, "testYingTitle") == 1);
    }
    SECTION("Test writing artist"){
        REQUIRE(SetLeadArtist(filename, "TitTestYingTitle") == 1);
    }

     SECTION("Test writing back album"){
        REQUIRE(SetAlbum(filename, "readYing") == 1);
    }
    SECTION("Test writing back title"){
        REQUIRE(SetTitle(filename, "readYingTitle") == 1);
    }
    SECTION("Test writing back artist"){
        REQUIRE(SetLeadArtist(filename, "ReadTestYingTitle") == 1);
    }

}

