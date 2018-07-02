#bin_PROGRAMS=analyze chmorph hunspell munch unmunch hzip hunzip
noinst_PROGRAMS=hunspell

AM_CPPFLAGS = -I${top_builddir}/src/hunspell -I${top_srcdir}/src/hunspell \
              -I${top_srcdir}/src/parsers -DHUNSPELL_STATIC \
	      $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS = -std=c++14 $(CODE_COVERAGE_CXXFLAGS)
LDADD      = $(CODE_COVERAGE_LIBS)

#hzip_SOURCES=hzip.cxx
#hunzip_SOURCES=hunzip.cxx
#hunzip_LDADD = ../hunspell/libhunspell.a
#
#munch_SOURCES=munch.cxx munch.h
#unmunch_SOURCES=unmunch.cxx unmunch.h
#
#example_SOURCES=example.cxx
#example_LDADD = ../hunspell/libhunspell.a
#
hunspell_SOURCES=hunspell.cxx
nodist_hunspell_SOURCES= ../../config.h
hunspell_LDADD = $(LIBICONV) ../parsers/libparsers.a \
	../hunspell/libhunspell.a $(CURSESLIB) $(READLINELIB) $(LDADD)

#analyze_SOURCES=analyze.cxx
#analyze_LDADD = ../hunspell/libhunspell.a
#
#bulkcheck_SOURCES=bulkcheck.cxx
#bulkcheck_LDADD = ../hunspell/libhunspell.a
#
#chmorph_SOURCES=chmorph.cxx
#chmorph_LDADD = ../parsers/libparsers.a ../hunspell/libhunspell.a
#
#noinst_PROGRAMS=example bulkcheck
#
#dist_bin_SCRIPTS=makealias affixcompress wordforms ispellaff2myspell wordlist2hunspell
