# Changelog
Changelog of project Nuspell.

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.2.0] - 2019-03-19
### Added
- Added build System CMake. Supports building as shared library.

### Changed
- Public API changed again, last for v2:
    - `Dictionary::suggest()` return data inside simple `vector<string>`.
      `List_Strings` is not used anymore.
    - Constructors of class `Dictionary` like `Dictionary::load_from_path()`
      throw `Dictionary_Loading_Error` on error. Previously they were throwing
      `ios_base::failure`.
- Boost::Locale is not dependency of library Nuspell anymore. It is still a
  dependency of the CLI tool. The library depends directly on ICU. Internally,
  all string now are in Unicode (UTF-8 or UTF-32, it depends of the need).

### Removed
- Removed old Autotools build system.
- Removed `NOSUGGEST_MODE` in CLI tool. It was very similar to
  `MISSPELLED_WORDS_MODE`.
- Class `Finder` does not search for Myspell dictionaries on the file-system
  anymore.

### Fixed
- Support compiling with GCC 5. Previously GCC 7 was needed.
- Faster dictionary loading and better internal error handing when parsing a
  dictionary file.
- Faster spellchecking as a consequence of faster case classification, which
  in turn, is a consequence of all string being Unicode and directly using ICU.

## [2.1.0] - 2019-01-03
### Changed
- Public API classes are inside inline namespace v2
- `List_Strings<char>` is renamed to just `List_Strings`. Affects client code.

### Fixed
- Improve public API docs

## [2.0.0] - 2018-11-22
### Added
- First public release
- Spelling error detection (checking) is closely matching Hunspell
- Support for spelling error correction (suggestions)

[2.2.0]: https://github.com/nuspell/nuspell/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/nuspell/nuspell/compare/v2.0.0...v2.1.0
[2.0.0]: https://github.com/nuspell/nuspell/compare/v1.6.2...v2.0.0
