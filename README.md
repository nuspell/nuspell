# About Nuspell

Nuspell is a spell checker library and command-line program designed for
languages with rich morphology and complex word compounding. Nuspell is
a pure C++ re-implementation of Hunspell.

Main features of Nuspell spell checker:

  - Full unicode support backed by ICU
  - Backward compatibility with Hunspell dictionary file format
  - Twofold affix stripping (for agglutinative languages, like Azeri,
    Basque, Estonian, Finnish, Hungarian, Turkish, etc.)
  - Support complex compounds (for example, Hungarian and German)
  - Support language specific features (for example, special casing of
    Azeri and Turkish dotted i, or German sharp s)
  - Handle conditional affixes, circumfixes, fogemorphemes, forbidden
    words, pseudoroots and homonyms.
  - Free software. Licensed under GNU LGPL v3.

# Building Nuspell

## Dependencies

Build-only dependencies:

  - C++ 17 compiler, GCC >= v7, Clang >= v5, MSVC >= 2017
  - Cmake version 3.8 or newer
  - Git
  - Ronn
  - Boost headers version 1.62 or newer

Run-time (and build-time) dependencies:

  - icu4c
  - boost-locale (needed only for the CLI tool, not the library)

Recommended tools for developers: qtcreator, ninja, clang-format, gdb,
vim, doxygen.

## Building on GNU/Linux and Unixes

We first need to download the dependencies. Some may already be
preinstalled.

For Ubuntu and Debian:

```bash
sudo apt install git cmake libboost-locale-dev libicu-dev
```

Then run the following commands inside the Nuspell directory:

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

<!--sudo ldconfig-->

For faster build process run `make -j`, or use Ninja instead
of Make.

If you are making a Linux distribution package (dep, rpm) you need
some additional configurations on the CMake invocation. For example:

```bash
cmake .. -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr
```

## Building on OSX and macOS

1.  Install Apple's Command-line tools.
2.  Install Homebrew package manager.
3.  Install dependencies with the next commands.

<!-- end list -->

```bash
brew install cmake icu4c boost
export ICU_ROOT=$(brew --prefix icu4c)
```

Then run the standard cmake and make. See above. The ICU\_ROOT variable
is needed because icu4c is keg-only package in Homebrew and CMake can
not find it by default. Alternatively, you can use `-DICU_ROOT=...` on
the cmake command line.

If you want to build with GCC instead of Clang, you need to pull GCC
with Homebrew and rebuild all the dependencies with it. See Homewbrew
manuals.

## Building on Windows

### Compiling with Visual C++

1.  Install Visual Studio 2017 or newer. Alternatively, you can use
    Visual Studio Build Tools.
2.  Install Git for Windows and Cmake.
3.  Install vcpkg in some folder, e.g. in `c:\vcpkg`.
4.  With vcpkg install: icu, boost-locale\[icu\].
5.  Run the commands bellow.

<!-- end list -->

```bat
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=c:\vcpkg\scripts\buildsystems\vcpkg.cmake -A Win32
cmake --build .
```

### Compiling with Mingw64 and MSYS2

Download MSYS2, update everything and install the following packages:

```bash
pacman -S base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-boost \
          mingw-w64-x86_64-cmake
```

Then from inside the Nuspell folder run:

```bash
mkdir build
cd build
cmake .. -G "Unix Makefiles"
make
make install
```

### Building in Cygwin environment

Download the above mentioned dependencies with Cygwin package manager.
Then compile the same way as on Linux. Cygwin builds depend on
Cygwin1.dll.

## Building on FreeBSD (experimental)

Install the following required packages

```bash
pkg cmake icu boost-libs catch
```

Then run the standard cmake and make as on Linux. See above.

# Using the software

## Using the command-line tool

The main executable is located in `src/nuspell`.

After compiling and installing you can run the Nuspell spell checker
with a Nuspell, Hunspell or Myspell dictionary:

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

Sample program:

```cpp
#include <boost/locale/utf8_codecvt.hpp>
#include <nuspell/dictionary.hxx>
#include <nuspell/finder.hxx>

using namespace std;

int main()
{
	auto dict_finder = nuspell::Finder::search_all_dirs_for_dicts();
	auto path = dict_finder.get_dictionary_path("en_US");

	auto dict = nuspell::Dictionary::load_from_path(path);

	auto utf8_loc = locale(locale::classic(),
	                       new boost::locale::utf8_codecvt<wchar_t>());
	dict.imbue(utf8_loc);

	auto word = string();
	auto sugs = vector<string>();
	while (cin >> word) {
		if (dict.spell(word)) {
			cout << "Word \"" << word << "\" is ok.\n";
			continue;
		}

		cout << "Word \"" << word << "\" is incorrect.\n";
		dict.suggest(word, sugs);
		if (sugs.empty())
			continue;
		cout << "  Suggestions are: ";
		for (auto& sug : sugs)
			cout << sug << ' ';
		cout << '\n';
	}
}
```

On the command line you can link like this:

```bash
g++ example.cxx -lnuspell -licuuc -licudata
# or better, use pkg-config
g++ example.cxx $(pkg-config --cflags --libs nuspell)
```

Within Cmake you can use `find_package()` to link. For example:

```cmake
find_package(Nuspell)
add_executable(myprogram main.cpp)
target_link_libraries(myprogram Nuspell::nuspell)
```

# Dictionaries

Myspell, Hunspell and Nuspell dictionaries:

<https://github.com/nuspell/nuspell/wiki/Dictionaries-and-Contacts>


# Advanced topics
## Debugging Nuspell

First, always install the debugger:

```bash
sudo apt install gdb
```

For debugging we need to create a debug build and then we need to start
`gdb`.

```bash
mkdir debug
cd debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j
gdb src/nuspell/nuspell
```

We recommend debugging to be done
[with an IDE](https://github.com/nuspell/nuspell/wiki/IDE-Setup).

## Testing

To run the tests, run the following command after building:

    ctest

# See also

Full documentation in the [wiki](https://github.com/nuspell/nuspell/wiki),
which also holds the [Code of
Conduct](https://github.com/nuspell/nuspell/wiki/Code-of-Conduct).

API Documentation for developers can be generated from the source files
by running:

    doxygen

The result can be viewed by opening `doxygen/html/index.html` in a web
browser.


