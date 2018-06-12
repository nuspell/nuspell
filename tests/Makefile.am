SUBDIRS = . suggestiontest v1cmdline

AM_CPPFLAGS = -I../src/nuspell $(BOOST_CPPFLAGS) $(CATCH_CFLAGS) $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS = -std=c++14 $(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS  = $(BOOST_LOCALE_LDFLAGS)
LDADD = ../src/nuspell/libnuspell.a $(BOOST_LOCALE_LIBS) $(CODE_COVERAGE_LIBS) $(ICU_LIBS)


TEST_EXTENSIONS = .catch
CATCH_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
CATCH_LOG_COMPILER = $(SHELL) $(srcdir)/catch-runner.sh

check_PROGRAMS = \
ch.catch

# Next setting is needed to build executables from Qt Creator.
noinst_PROGRAMS = $(check_PROGRAMS)

TESTS =	$(check_PROGRAMS)

ch_catch_SOURCES = \
locale_utils_test.cxx \
string_utils_test.cxx \
condition_test.cxx \
structures_test.cxx \
dictionary_test.cxx \
aff_data_test.cxx \
catch_main.cxx

# to run coverage, cd to root
# ./configure --enable-code-coverage
# make check-code-coverage

EXTRA_DIST = test.sh catch-runner.sh
