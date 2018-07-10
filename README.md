# About Nuspell

NOTICE: Nuspell is currently under development. For contributing see
[version 2
specification](https://github.com/nuspell/nuspell/wiki/Version-2-Specification).

Nuspell is a spell checker library and
program designed for languages with rich morphology and complex word
compounding or character encoding. Nuspell interfaces: Ispell pipe
interface and C++ API. Nuspell is a pure C++
reimplementation of Hunspell.

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

# Dependencies

Build-only dependencies:

    g++ make autoconf automake libtool pkg-config wget libiconv

Runtime dependencies:

    boost-locale icu4c

Recommended tools for developers:

```
qtcreator clang-format gdb vim cppcheck doxygen plantuml
```

# Building on GNU/Linux and Unixes

We first need to download the dependencies. Some may already be preinstalled.

For Ubuntu and Debian:

    sudo apt install g++ autoconf automake make libtool pkg-config \
                     libboost-locale-dev libboost-system-dev libicu-dev

Then run the following commands:

    autoreconf -vfi
    ./configure
    make
    sudo make install
    sudo ldconfig

For speeding up the build process, see also the -j option of make.

<!-- hunspell v1 stuff
For dictionary development, use the `--with-warnings` option of
configure.

For interactive user interface of Nuspell executable, use the `--with-ui
option`.

Optional developer packages:

  - ncurses (need for --with-ui), eg. libncursesw5 for UTF-8
  - readline (for fancy input line editing, configure parameter:
    --with-readline)

In Ubuntu, the packages are:

    libncurses5-dev libreadline-dev
-->

# Building on OSX and macOS

1. Install Apple's Command-line tools
2. Install Homebrew package manager
3. Install dependencies

```
brew install autoconf automake libtool pkg-config wget
brew install boost --with-icu4c
```

Then run the standard trio of autoreconf, configure and make. See above.

If you want to build with GCC instead of Clang, you need to pull GCC with
Homebrew and rebuild all the dependencies with it. See Homewbrew manuals.

# Building on Windows

## 1\. Compiling with Mingw64 and MSYS2

Download Msys2, update everything and install the following
    packages:

    pacman -S base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-libtool \
              mingw-w64-x86_64-boost

Open Mingw-w64 Win64 prompt and compile the same way as on Linux, see
above.

## 2\. Building in Cygwin environment

Download the above mentioned dependencies with Cygwin package manager.
Then compile the same way as on Linux. Cygwin builds depend on
Cygwin1.dll.

# Building on FreeBSD (experimental)

Install the following required packages

    autoconf automake libtool pkgconf icu boost-libs

Then run the standard trio of autoreconf, configure and make. See above.

# Debugging

First, always install the debugger:

    sudo apt install gdb

It is recomended to install a debug build of the standard library:

    sudo apt install libstdc++6-7-dbg

For debugging we need to create a debug build and then we need to start
`gdb`.

    ./configure CXXFLAGS='-g -O0 -Wall -Wextra'
    make
    gdb src/nuspell/nuspell

You can also pass the `CXXFLAGS` directly to `make` without calling
`./configure`, but we don't recommend this way during long development
sessions.

If you like to develop and debug with an IDE, see documentation at
https://github.com/nuspell/nuspell/wiki/IDE-Setup

# Testing

Testing Nuspell (see tests in tests/ subdirectory):

    make check

or with Valgrind debugger:

    make check
    VALGRIND=[Valgrind_tool] make check

For example:

    make check
    VALGRIND=memcheck make check

# Documentation

Short documentation in man-pages:

    man nuspell
    nuspell -h

Full documentation in the
[wiki](https://github.com/nuspell/nuspell/wiki).

Documentation for developers can be generated from the source files by
running:

    doxygen

The result can be viewed by opening `doxygen/html/index.html` in a web
browser. Doxygen will use Graphviz for generating all sorts of graphs
and PlantUML for generating UML diagrams. Make sure that the packages
plantuml and graphviz are installed before running Doxygen. The latter
is usually installed automatically when installing Doxygen.

# Usage

The main executable is located in `src/nuspell`.

<!--
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

After compiling and installing (see INSTALL) you can run the Nuspell
spell checker (compiled with user interface) with a Nuspell, Hunspell or
Myspell dictionary:

    nuspell -d en_US text.txt

# Using Nuspell library with GCC

Including in your program:

    #include <nuspell/dictionary.hxx>

Linking with Nuspell static library:

    g++ example.cxx -lnuspell -lboost-locale -licuuc -licudata
    # or better, use pkg-config
    g++ example.cxx $(pkg-config --cflags --libs nuspell)

## Dictionaries

Myspell, Hunspell and Nuspell dictionaries:

https://github.com/nuspell/nuspell/wiki/Dictionaries-and-Contacts
