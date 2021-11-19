tag-read-writer
===============

audio tags handling header-only library for C++17 applications/libraries. This project provides a header-only library for reading and writing tags in audio files.

About
-----

Why does the open source world need another audio tag library? Yes there is already the taglib out there. But there is a need to a solution that is highly functional and relies on current and future modern cpp patterns. Hence less vulnerability within this solution will be achieved. It is a heady-only library that can be easily included and used in any c++17 supported project.

This library supports parsing, reading and writing [APE tags](http://wiki.hydrogenaud.io/index.php?title=APEv2_specification), [id3v1 tags](http://id3.org/ID3v1), and [id3v2 tags](http://id3.org/id3v2.3.0).

For logging the internal behaviour, this spdlog library is used in this solution. Further informations about the used logging library can be found [here](https://github.com/gabime/spdlog).

Examples
---------

In the example folder, there are utilities giving good hints about how to use this library:
- readTag
  - Utility for simply reading tags out of an audio file or any audio file in a directory. The following action gives informations on how to use it.

```console
./readTag --help

Usage:
  readTag [-?|-h|--help] --directory|-d [--title|-t] [--genre|-g] [--artist|-a] [--album|-b]

This is an utility for changing tag across all audio files 
within a directory.

Options, arguments:
  -?, -h, --help

  Display usage information.

  --directory, -d <dir>
  specify directory or file to use for reading the tag (REQUIRED).
  --title, -t
  Change the title frame content.
  --genre, -g
  Change the genre frame content.
  --artist, -a
  Change the artist frame content.
  --album, -b
  Change the album frame content.
```

- writeTag
  - Utility for modifying tags in an audio file or in all audio file within a directory. The following command gives informations on how to use it.

```console
 ./writeTag --help
```


Testing
-------

This library relies on the Catch2 testing framework for its development driven tests. Further information about Catch2 can be retrieved [here](https://github.com/catchorg/Catch2). The test folder ("tests") contains the currently implemented behavioral test units.


License:
--------

This code is distributed under the MIT License (http://opensource.org/licenses/MIT).


Supported Compilers
-------------------

The code is known to work on the following compilers:

- GCC 7.4.0 (or later) ( with explicit C++17)
- Visual C++ 2015


Platforms
---------

 * Linux Ubuntu starting from Xenial
 * Windows (cygwin)
 * Windows Visual Studio 2019


**Development Status:** This code is still under development. First release is yet to happen. This code *will* evolve without regard to backwards compatibility.

* **1.0.0** Mars 08, 2020
  - NEW: Initial release.

