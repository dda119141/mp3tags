#include <lyra/lyra.hpp>
#include <iostream>
#include <experimental/filesystem>
#include "tagwriter.hpp"

bool changeTags(const std::string& directory, std::string& album) {
    namespace fs = std::experimental::filesystem;
    using std::cout;
    using std::endl;

    const std::string currentFilePath = fs::system_complete(directory);

    cout << "current file path:" << currentFilePath << endl;
    const fs::path mp3Path = fs::path(currentFilePath);
    if (!fs::exists(mp3Path)) {
        std::cerr << "Path: " << currentFilePath << " does not exist" << endl;
        return false;
    }

    try {
        for (auto& filen : fs::directory_iterator(mp3Path.string())) {
            const std::string filename = filen.path().string();
            cout << "Change album: of file: " << filename << "\n";
            if (SetAlbum(filename, album) == 1)
                cout << __func__ << " Success\n";
        }
    } catch (fs::filesystem_error& e) {
        cout << "wrong path:" << e.what() << endl;
    }

    return true;
}

int main(int argc, const char** argv) {
    // Where we read in the argument value:
    bool show_help = false;
    std::string directory, album;

    auto parser =
        lyra::help(show_help).description(
            "This is an utility for changing tag across all mp3 files \nwithin "
            "a directory.") |
        lyra::opt(directory, "dir")["--directory"]["-d"](
            "specify directory to use for changing the tag.")
            .required() |
        lyra::opt([&](std::string album) { changeTags(directory, album); },
                  "album")["--album"]["-a"]("Change the album frame content.");

    // Parse the program arguments:
    auto result = parser.parse({argc, argv});

    // Check that the arguments where valid:
    if (!result) {
        std::cerr << "Error in command line: " << result.errorMessage()
                  << std::endl;
        std::cout << parser << std::endl;
        std::exit(1);
    } else {
        std::cout << "Need to add tag to change as well as contents\n";
        //        std::cout << parser << std::endl;
    }

    return 0;
}


