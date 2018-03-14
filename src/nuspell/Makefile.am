lib_LIBRARIES = libnuspell.a

AM_CPPFLAGS = $(BOOST_CPPFLAGS)   $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS = -std=c++14          $(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS  = $(BOOST_LOCALE_LDFLAGS)
LIBADD      = $(BOOST_LOCALE_LIBS) $(CODE_COVERAGE_LIBS)

libnuspell_a_SOURCES=\
aff_data.cxx     aff_data.hxx     \
dic_data.cxx     dic_data.hxx     \
dictionary.cxx   dictionary.hxx   \
finder.cxx       finder.hxx       \
locale_utils.cxx locale_utils.hxx \
string_utils.cxx string_utils.hxx \
structures.cxx   structures.hxx

pkginclude_HEADERS=\
aff_data.hxx     \
dic_data.hxx     \
dictionary.hxx   \
finder.hxx       \
locale_utils.hxx \
string_utils.hxx \
structures.hxx

bin_PROGRAMS = nuspell
nuspell_SOURCES = main.cxx
LDADD = libnuspell.a $(LIBADD)
