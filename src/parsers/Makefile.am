AM_CPPFLAGS=-I${top_builddir}/src/hunspell -DHUNSPELL_STATIC \
	    $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS=$(CODE_COVERAGE_CXXFLAGS)

noinst_LIBRARIES=libparsers.a
libparsers_a_SOURCES=firstparser.cxx firstparser.hxx xmlparser.cxx \
		      xmlparser.hxx latexparser.cxx latexparser.hxx \
		      manparser.cxx manparser.hxx \
		      textparser.cxx textparser.hxx htmlparser.cxx \
		      htmlparser.hxx odfparser.hxx odfparser.cxx \
		      noparser.cxx noparser.hxx

#noinst_PROGRAMS=testparser
#testparser_SOURCES=testparser.cxx
#LDADD = libparsers.a ../hunspell/libhunspell.a $(CODE_COVERAGE_LIBS)
