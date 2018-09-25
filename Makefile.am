ACLOCAL_AMFLAGS = -I m4

SUBDIRS= src docs tests

pkgconfdir = $(libdir)/pkgconfig
pkgconf_DATA = nuspell.pc
doc_DATA = README

EXTRA_DIST = COPYING.LESSER nuspell.pc.in LICENSES

@CODE_COVERAGE_RULES@

@GENERATE_CHANGELOG_RULES@
CHANGELOG_START = v1.6.2^
dist-hook: dist-ChangeLog
