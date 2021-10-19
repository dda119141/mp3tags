// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include <lyra/lyra.hpp>
#include <future>
#include <iostream>
#include "tagreader.hpp"

struct DirectoryOptions {
    bool empty;
    bool remove;

    bool isEmpty() { return !(empty | remove ); }
} DirectoryOption;


namespace fs = id3::filesystem;


static bool deleteFiles(const fs::path& filePath)
{
    bool result = true;

    for (auto DirElt = fs::directory_iterator(filePath.string()); DirElt != fs::directory_iterator(); ++DirElt)
    {
        auto &dirElt = *DirElt;

        if (fs::is_regular_file(dirElt))
        {
            result |= fs::remove_all(dirElt);
        }
    }

    result |= fs::remove_all(filePath);

    return result;
}

bool findDirectoryToDelete(const fs::path& filePath)
{
    bool ElementToDelete = true;

    if (!fs::is_directory(filePath))
    {
        return false;
    }

    for (auto DirElt = fs::directory_iterator(filePath.string()); DirElt != fs::directory_iterator(); ++DirElt)
    {
        auto &dirElt = *DirElt;

        if (fs::is_directory(dirElt))
        {
            ElementToDelete = false;
        }
        else if (fs::is_regular_file(dirElt))
        {
            const auto scopedSize = fs::file_size(dirElt);

            if (scopedSize > (1024 * 1024))
            {
                ElementToDelete = false;
            }
        }
    }

    return ElementToDelete;
}

void removeObsoleteDirectory(const fs::path& filePath, const struct DirectoryOptions dirOption)
{
    auto funcObject = [&]() {
        std::vector<fs::path> lists = {};

        for (auto &elt : fs::recursive_directory_iterator(filePath))
        {
            if (findDirectoryToDelete(elt.path()))
            {
                std::cout << "Path to Delete: " << elt.path().string().c_str() << std::endl;
                lists.push_back(elt.path());
            };
        }

        if (lists.empty())
            return false;

        if (dirOption.remove)
            std::for_each(lists.begin(), lists.end(), [](const fs::path &elt) {
                deleteFiles(elt);
            });

        return true;
    };

    if (dirOption.remove)
        while (funcObject())
            ;
    else
        funcObject();

    return;
}


bool removePaths(const std::string& directory,
                           const struct DirectoryOptions& directoryOption) {
    using std::cout;
    using std::endl;

#ifdef HAS_FS_EXPERIMENTAL
	const std::string currentFilePath = fs::system_complete(directory);
#else
	const std::string currentFilePath = fs::absolute(directory).string();
#endif

    const fs::path mp3Path = fs::path(currentFilePath);

    if (!fs::exists(mp3Path)) {
        std::cerr << "Path: " << currentFilePath << " does not exist" << endl;
        return false;
    } else if (fs::is_regular_file(mp3Path)) {
        std::cerr << "Path: " << currentFilePath << " is not a directory " << endl;
        return false;
    } else {
        try
        {
            removeObsoleteDirectory(mp3Path, directoryOption);
        } catch (fs::filesystem_error& e) {
            cout << "ERROR filesystem:" << e.what() << endl;
        }
    }
    return true;
}

int main(int argc, const char** argv) {

    // Where we read in the argument value:
    bool show_help = false;
    std::string directory;

    const auto parser =
        lyra::help(show_help).description(
            "This is an utility for changing tag across all mp3 files \nwithin "
            "a directory.") |
        lyra::opt(directory, "dir")["--directory"]["-d"](
            "specify directory or file to use for reading the tag (REQUIRED).")
            .required() |
        lyra::opt(DirectoryOption.empty)["--empty"]["-t"]("Get the empty frame content.") |
        lyra::opt(DirectoryOption.remove)["--remove"]["-a"]("Get the remove frame content.");

    // Parse the program arguments:
    auto result = parser.parse({argc, argv});

    // Check that the arguments where valid:
    if (!result || DirectoryOption.isEmpty()) {
        std::cerr << "Error in command line: " << result.errorMessage()
                  << std::endl;
        std::cerr << parser << std::endl;
        std::exit(1);
    } else if(show_help) {
        std::cout << parser << std::endl;
    }else{
        removePaths(directory, DirectoryOption);
    }

    return 0;
}
