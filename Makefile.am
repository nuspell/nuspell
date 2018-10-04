ACLOCAL_AMFLAGS = -I m4

SUBDIRS= src docs tests

pkgconfdir = $(libdir)/pkgconfig
pkgconf_DATA = nuspell.pc
doc_DATA = README

EXTRA_DIST = COPYING.LESSER nuspell.pc.in LICENSES

@CODE_COVERAGE_RULES@
CODE_COVERAGE_OUTPUT_FILE = lcov.info
CODE_COVERAGE_OUTPUT_DIRECTORY = lcov
CODE_COVERAGE_LCOV_OPTIONS = --no-external
CODE_COVERAGE_IGNORE_PATTERN = "*.hpp" "*_test.cxx"
CODE_COVERAGE_GENHTML_OPTIONS = --prefix ~/*/nuspell/src

@GENERATE_CHANGELOG_RULES@
CHANGELOG_START = v1.6.2^
dist-hook: dist-ChangeLog
