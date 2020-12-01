// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include <iostream>
#include <memory>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "tagreader.hpp"
#include "tagwriter.hpp"

TEST_CASE("Read/Write from/to id3v2 tag") {
    namespace fs = id3::filesystem;

    using std::cout;
    using std::endl;

    std::string filename;

    const std::string currentFilePath = fs::system_complete(__FILE__);

    cout << "current file path:" << currentFilePath << endl;
    const std::string mp3Path =
        (fs::path(currentFilePath).parent_path() /= "../files").string();

    const auto findFileName = [&](const std::string& filName){
        const auto pos = filName.find_last_of('/');
        return filName.substr(pos+1);
    };

    try {
        for (auto& filen : fs::directory_iterator(mp3Path)) {
            std::string _filename = filen.path().string();

            if (findFileName(_filename) == std::string("test1.mp3")) {
                filename = _filename;
            } /* else if (_filename.find("test1.mp3") != std::string::npos) {
                filename = _filename;
            } */
        }
    } catch (fs::filesystem_error& e) {
        cout << "wrong path:" << e.what() << endl;
    }

    SECTION("test path") {
        REQUIRE(filename == (fs::path(mp3Path) /= "test1.mp3").string());
    }

    //    cout << "File type: " << GetFileType(filename) << endl;
    //    cout << "Group Description: " << GetContentGroupDescription(filename)
    //    << endl;
    //    cout << "GetTrackPosition: " << GetTrackPosition(filename) << endl;
    /*
    SECTION("Test writing album name: test2"){
        REQUIRE(SetAlbum(filename, "test1") == 1);
    }
    */

    SECTION("Test writing album") {
        REQUIRE(SetAlbum(filename, "AlbYingAlbum") == 1);
    }
    SECTION("Test writing title") {
        REQUIRE(SetTitle(filename, "testYingYangTitle") == 1);
    }
    SECTION("Test writing artist") {
        REQUIRE(SetLeadArtist(filename, "TitTestYingArtist") == 1);
    }

    SECTION("Test reading back album") {
        REQUIRE(GetAlbum(filename) == "AlbYingAlbum");
    }
    SECTION("Test reading back title") {
        REQUIRE(GetTitle(filename) == "testYingYangTitle");
    }
    SECTION("Test reading back artist") {
        REQUIRE(GetLeadArtist(filename) == "TitTestYingArtist");
    }
    SECTION("Test reading Year") { REQUIRE(GetYear(filename) == "2009"); }
    SECTION("Test reading Content type") {
        REQUIRE(GetContentType(filename) == "Rap");
    }
    SECTION("Test reading track position") {
        REQUIRE(GetTrackPosition(filename) == "05/18");
    }
    SECTION("Test reading band orchestra") {
        REQUIRE(GetBandOrchestra(filename) == "Ying Yang Twins");
    }

}

