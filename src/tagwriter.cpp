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




TEST_CASE("Read/Write album title from/to id2v3 tag") {

   namespace fs = std::experimental::filesystem;

    using std::cout;
    using std::endl;

    std::string filename;

    try{

        for (auto& filen : fs::directory_iterator("../files/"))
        {
            std::string _filename = filen.path().string();

            if(_filename.find("test1.mp3") != std::string::npos){
                filename = _filename;
            }
        }
    }catch (fs::filesystem_error) {
        cout << "wrong path:" << endl;
    }

    SECTION("Test reading album"){
        const std::string albumName = "Ying Yang Forever";
        REQUIRE(GetAlbum(filename) == albumName);
    }

    SECTION("Test writing album name"){
        REQUIRE(SetAlbum(filename, "test1") == 1);
    }
}

