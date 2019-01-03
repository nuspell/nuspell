# Changelog
Changelog of project Nuspell.

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Added build System CMake

### Removed
- Removed old autotools build system

### Fixed
- Support compiling with GCC 5. Previously GCC 7 was needed.

## [2.1.0]
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

[Unreleased]: https://github.com/nuspell/nuspell/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/nuspell/nuspell/compare/v1.6.2...v2.0.0
[2.1.0]: https://github.com/nuspell/nuspell/compare/v2.0.0...v2.1.0
