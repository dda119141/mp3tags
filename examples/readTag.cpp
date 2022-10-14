// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "tagreader.hpp"
#include <iostream>
#include <lyra/lyra.hpp>

struct tagOptions {
  bool album;
  bool genre;
  bool title;
  bool artist;
  bool trackPosition;
  bool allEntries;
  bool year;
  bool isEmpty()
  {
    return !(album | genre | title | artist | trackPosition | year |
             allEntries);
  }
} tagOption = {};

bool readTagsInFile(const std::string &mediafile, const struct tagOptions &tags)
{
  namespace fs = id3::filesystem;
  using std::cout;
  using std::endl;

#ifdef HAS_FS_EXPERIMENTAL
  const std::string currentFilePath = fs::system_complete(mediafile);
#else
  const std::string currentFilePath = fs::absolute(mediafile).string();
#endif

  const fs::path mp3Path = fs::path(currentFilePath);
  if (!fs::exists(mp3Path)) {
    std::cerr << "Path: " << currentFilePath << " does not exist" << endl;
    return false;
  }
  const std::string filename = mp3Path.string();
  const auto EndFilename = mp3Path.filename();

  if (!fs::is_regular_file(mp3Path))
    return false;

  if (tags.album) {
    cout << "Get album of file: " << EndFilename << " : ";
    cout << GetAlbum(filename) << "\n";
  }
  if (tags.artist) {
    cout << "Get artist of file: " << EndFilename << " : ";
    cout << GetLeadArtist(filename) << "\n";
  }
  if (tags.genre) {
    cout << "Get genre of file: " << EndFilename << " : ";
    cout << GetContentType(filename) << "\n";
  }
  if (tags.title) {
    cout << "Get title of file: " << EndFilename << " : ";
    cout << GetTitle(filename) << "\n";
  }
  if (tags.trackPosition) {
    cout << "Get track position of file: " << EndFilename << " : ";
    cout << GetTrackPosition(filename) << "\n";
  }
  if (tags.year) {
    cout << "Get file release year: " << EndFilename << " : ";
    cout << GetYear(filename) << "\n";
  }
  if (tags.allEntries) {
    GetAllMetaEntries(filename);
  }

  return true;
}

bool readTags(const std::string &directory, const struct tagOptions &tags)
{
  namespace fs = id3::filesystem;

  using std::cout;
  using std::endl;

  const std::string currentFilePath = fs::absolute(directory).string();

  const fs::path mp3Path = fs::path(currentFilePath);
  if (!fs::exists(mp3Path)) {
    std::cerr << "Path: " << currentFilePath << " does not exist" << endl;
    return false;
  } else if (fs::is_regular_file(mp3Path)) {
    const std::string mediafile = mp3Path.string();
    readTagsInFile(mediafile, tags);
  } else {
    try {
      for (auto &filen : fs::recursive_directory_iterator(mp3Path.string())) {
        const std::string mediafile = filen.path().string();
        readTagsInFile(mediafile, tags);
      }
    } catch (fs::filesystem_error &e) {
      cout << "wrong path:" << e.what() << endl;
    }
  }
  return true;
}

int main(int argc, const char **argv)
{
  // Where we read in the argument value:
  bool show_help = false;
  std::string directory;

  auto parser =
      lyra::help(show_help).description(
          "This is an utility for changing tag across all mp3 files \nwithin "
          "a directory.") |
      lyra::opt(directory, "dir")["--directory"]["-d"](
          "specify directory or file to use for reading the tag (REQUIRED).")
          .required() |
      lyra::opt(tagOption.title)["--title"]["-t"](
          "Get the title frame content.") |
      lyra::opt(tagOption.genre)["--genre"]["-g"](
          "Get the genre frame content.") |
      lyra::opt(tagOption.artist)["--artist"]["-a"](
          "Get the artist frame content.") |
      lyra::opt(tagOption.album)["--album"]["-b"](
          "Get the album frame content.") |
      lyra::opt(tagOption.year)["--year"]["-y"]("Get the album release year.") |
      lyra::opt(tagOption.trackPosition)["--trackposition"]["-p"](
          "Get the track position.") |
      lyra::opt(tagOption.allEntries)["--all-entries"]["-e"](
          "All meta entries");

  // Parse the program arguments:
  auto result = parser.parse({argc, argv});

  // Check that the arguments where valid:
  if (!result || tagOption.isEmpty()) {
    std::cerr << "Error in command line: " << result.errorMessage()
              << std::endl;
    std::cerr << parser << std::endl;
    std::exit(1);
  } else if (show_help) {
    std::cout << parser << std::endl;
  } else {
    readTags(directory, tagOption);
  }

  return 0;
}
