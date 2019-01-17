# About Nuspell

Nuspell is a spell checker library and command-line program designed
for languages with rich morphology and complex word compounding.
Nuspell is a pure C++ reimplementation of Hunspell.

Main features of Nuspell spell checker and morphological analyzer:

  - Full unicode support
  - Max. 65535 affix classes and twofold affix stripping (for
    agglutinative languages, like Azeri, Basque, Estonian, Finnish,
    Hungarian, Turkish, etc.)
  - Support complex compounds (for example, Hungarian and German)
  - Support language specific features (for example, special casing of
    Azeri and Turkish dotted i, or German sharp s)
  - Handle conditional affixes, circumfixes, fogemorphemes, forbidden
    words, pseudoroots and homonyms.
  - Free software. Licenced under GNU LGPL.

# Building Nuspell

## Dependencies

Build-only dependencies:

    g++ make cmake git

Runtime dependencies:

    boost-locale icu4c

Recommended tools for developers:

```
qtcreator ninja clang-format gdb vim doxygen
```

## Building on GNU/Linux and Unixes

We first need to download the dependencies. Some may already be preinstalled.

For Ubuntu and Debian:

    sudo apt install g++ make cmake libboost-locale-dev libicu-dev

Then run the following commands:

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

<!--sudo ldconfig-->

For speeding up the build process, run `make -j`, or use Ninja instead of Make.

## Building on OSX and macOS

1. Install Apple's Command-line tools
2. Install Homebrew package manager
3. Install dependencies

```
brew install cmake
brew install boost --with-icu4c
```

Then run the standard cmake and make. See above.

If you want to build with GCC instead of Clang, you need to pull GCC with
Homebrew and rebuild all the dependencies with it. See Homewbrew manuals.

## Building on Windows

### 1\. Compiling with Mingw64 and MSYS2

Download MSYS2, update everything and install the following
packages:

    pacman -S base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-boost \
              mingw-w64-x86_64-cmake

Then from inside the nuspell folder run:

    mkdir build
    cd build
    cmake .. -G "Unix Makefiles"
    make
    make install

### 2\. Building in Cygwin environment

Download the above mentioned dependencies with Cygwin package manager.
Then compile the same way as on Linux. Cygwin builds depend on
Cygwin1.dll.

## Building on FreeBSD (experimental)

Install the following required packages

    pkg cmake icu boost-libs

Then run the standard cmake and make as on Linux. See above.

# Debugging Nuspell

First, always install the debugger:

    sudo apt install gdb

It is recomended to install a debug build of the standard library:

    sudo apt install libstdc++6-8-dbg

For debugging we need to create a debug build and then we need to start
`gdb`.

    mkdir debug
    cd debug
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make -j
    gdb src/nuspell/nuspell

If you like to develop and debug with an IDE, see documentation at
https://github.com/nuspell/nuspell/wiki/IDE-Setup

# Testing

Testing Nuspell (see tests in tests/ subdirectory):

    make test

or with Valgrind debugger:

    make test
    VALGRIND=[Valgrind_tool] make test

For example:

    make test
    VALGRIND=memcheck make test

# Documentation

## Using the command-line tool

The main executable is located in `src/nuspell`.

After compiling and installing you can run the Nuspell
spell checker with a Nuspell, Hunspell or Myspell dictionary:

    nuspell -d en_US text.txt

For more details see the [man-page](docs/nuspell.1.md).

<!-- old hunspell v1 stuff
The src/tools directory contains ten executables after compiling.

  - The main executable:
      - nuspell: main program for spell checking and others (see manual)
  - Example tools:
      - analyze: example of spell checking, stemming and morphological
        analysis
      - chmorph: example of automatic morphological generation and
        conversion
      - example: example of spell checking and suggestion
  - Tools for dictionary development:
      - affixcompress: dictionary generation from large (millions of
        words) vocabularies
      - makealias: alias compression (Nuspell only, not back compatible
        with MySpell)
      - wordforms: word generation (Nuspell version of unmunch)
      - ~~hunzip: decompressor of hzip format~~ (DEPRECATED)
      - ~~hzip: compressor of hzip format~~ (DEPRECATED)
      - munch (DEPRECATED, use affixcompress): dictionary generation
        from vocabularies (it needs an affix file, too).
      - unmunch (DEPRECATED, use wordforms): list all recognized words
        of a MySpell dictionary
-->

## Using the Library

Including in your program:

    #include <nuspell/dictionary.hxx>

Linking with Nuspell static library:

    g++ example.cxx -lnuspell -licuuc -licudata
    # or better, use pkg-config
    g++ example.cxx $(pkg-config --cflags --libs nuspell)

## See also

Full documentation in the
[wiki](https://github.com/nuspell/nuspell/wiki), which also holds the
[Code of Conduct](https://github.com/nuspell/nuspell/wiki/Code-of-Conduct).

API Documentation for developers can be generated from the source files by
running:

    doxygen

The result can be viewed by opening `doxygen/html/index.html` in a web
browser.

# Dictionaries

Myspell, Hunspell and Nuspell dictionaries:

<https://github.com/nuspell/nuspell/wiki/Dictionaries-and-Contacts>
