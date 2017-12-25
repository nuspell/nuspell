lib_LIBRARIES = libnuspell.a
bin_PROGRAMS = nuspell

AM_CPPFLAGS = $(BOOST_CPPFLAGS)   $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS = -std=c++14          $(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS  = $(BOOST_LOCALE_LDFLAGS)
LDADD       = $(BOOST_LOCALE_LIBS) $(CODE_COVERAGE_LIBS)
LIBADD      = $(BOOST_LOCALE_LIBS) $(CODE_COVERAGE_LIBS)

libnuspell_a_SOURCES=\
aff_data.cxx     \
dic_data.cxx     \
finder.cxx       \
locale_utils.cxx

pkginclude_HEADERS=\
aff_data.hxx     \
dic_data.hxx     \
dictionary.hxx   \
finder.hxx       \
locale_utils.hxx \
stream_utils.hxx \
string_utils.hxx

nuspell_SOURCES = main.cxx
nuspell_LDADD = libnuspell.a $(LDADD)
