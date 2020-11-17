// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include <lyra/lyra.hpp>
#include <iostream>
#include "tagwriter.hpp"

bool changeTagsInFile(const std::string& mediafile,
                           const std::pair<std::string, std::string>& tags) {
    namespace fs = id3::filesystem;
    using std::cout;
    using std::endl;

    const std::string currentFilePath = fs::system_complete(mediafile);

    cout << "current file path:" << currentFilePath << endl;
    const fs::path mp3Path = fs::path(currentFilePath);
    if (!fs::exists(mp3Path)) {
        std::cerr << "Path: " << currentFilePath << " does not exist" << endl;
        return false;
    }

    const std::string filename = mp3Path.string();

    if (tags.first == "album") {
        const std::string& frameContent = tags.second;
        cout << "Change album: of file: " << filename << "\n";
        if (SetAlbum(filename, frameContent) == 1)
            cout << __func__ << " Success\n";
    } else if (tags.first == "artist") {
        const std::string& frameContent = tags.second;
        cout << "Change artist: of file: " << filename << "\n";
        if (SetLeadArtist(filename, frameContent) == 1)
            cout << __func__ << " Success\n";
    } else if (tags.first == "genre") {
        const std::string& frameContent = tags.second;
        cout << "Change genre: of file: " << filename << "\n";
        if (SetGenre(filename, frameContent) == 1)
            cout << __func__ << " Success\n";
    } else if (tags.first == "title") {
        const std::string& frameContent = tags.second;
        cout << "Change title: of file: " << filename << "\n";
        if (SetTitle(filename, frameContent) == 1)
            cout << __func__ << " Success\n";
    }

    return true;
}

bool changeTagsInDirectory(const std::string& directory,
                           const std::pair<std::string, std::string>& tags) {
    namespace fs = std::experimental::filesystem;
    using std::cout;
    using std::endl;

    const std::string currentFilePath = fs::system_complete(directory);

    cout << "current file path:" << currentFilePath << endl;
    const fs::path mp3Path = fs::path(currentFilePath);
    if (!fs::exists(mp3Path)) {
        std::cerr << "Path: " << currentFilePath << " does not exist" << endl;
        return false;
    } else if (fs::is_regular_file(mp3Path)) {
        const std::string mediafile = mp3Path.string();
        changeTagsInFile(mediafile, tags);
    } else {
        try {
            for (auto& filen : fs::recursive_directory_iterator(mp3Path.string())) {
                const std::string mediafile = filen.path().string();
                changeTagsInFile(mediafile, tags);
            }
        } catch (fs::filesystem_error& e) {
            cout << "wrong path:" << e.what() << endl;
        }
    }

    return true;
}

int main(int argc, const char** argv) {

    // Where we read in the argument value:
    bool show_help = false;
    std::string directory;
    std::vector<std::pair<std::string, std::string>> gTags;

    auto parser =
        lyra::help(show_help).description(
            "This is an utility for changing tag across all mp3 files \nwithin "
            "a directory.") |
        lyra::opt(directory, "dir")["--directory"]["-d"](
            "specify directory or file to use for changing the tag (REQUIRED).")
            .required() |
        lyra::opt([&](std::string title) { gTags.push_back({std::string("title"), title} ); },
                  "title")["--title"]["-t"]("Change the title frame content.") |
        lyra::opt([&](std::string genre) { gTags.push_back({std::string("genre"), genre} ); },
                  "genre")["--genre"]["-g"]("Change the genre frame content.") |
        lyra::opt([&](std::string artist) { gTags.push_back({std::string("artist"), artist} ); },
                  "artist")["--artist"]["-a"]("Change the artist frame content.") |
        lyra::opt([&](std::string album) { gTags.push_back({std::string("album"), album} ); },
                  "album")["--album"]["-b"]("Change the album frame content.");

    // Parse the program arguments:
    auto result = parser.parse({argc, argv});

    // Check that the arguments where valid:
    if (!result && !gTags.size()) {
        std::cerr << "Error in command line: " << result.errorMessage()
                  << std::endl;
        std::cout << parser << std::endl;
        std::exit(1);
    } else if(show_help) {
        std::cout << parser << std::endl;
    }else{
        for(auto& elt : gTags)
            changeTagsInDirectory(directory, elt);
    }

    return 0;
}


