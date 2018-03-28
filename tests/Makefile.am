SUBDIRS = . nuspell suggestiontest v1cmdline

AM_CPPFLAGS = -I../src/nuspell $(BOOST_CPPFLAGS) $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS = -std=c++14 $(CODE_COVERAGE_CXXFLAGS)
AM_LDFLAGS  = $(BOOST_LOCALE_LDFLAGS)
LDADD = ../src/nuspell/libnuspell.a $(BOOST_LOCALE_LIBS) $(CODE_COVERAGE_LIBS) $(ICU_LIBS)


BUILT_SOURCES = catch.hpp catch_reporter_tap.hpp
catch_version = 2.2.1
catch_zip = v$(catch_version).zip
catch_dir = Catch2-$(catch_version)
catch_url = https://github.com/catchorg/Catch2/archive/$(catch_zip)
catch.hpp:
	if [ ! -e $(catch_zip) ]; then wget $(catch_url); fi; unzip $(catch_zip) $(catch_dir)/single_include/catch.hpp; mv -f $(catch_dir)/single_include/catch.hpp .

catch_reporter_tap.hpp:
	if [ ! -e $(catch_zip) ]; then wget $(catch_url); fi; unzip $(catch_zip) $(catch_dir)/single_include/catch_reporter_tap.hpp; mv -f $(catch_dir)/single_include/catch_reporter_tap.hpp .

CLEANFILES = catch.hpp catch_reporter_tap.hpp

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
structures_test.cxx \
dictionary_test.cxx \
aff_data_test.cxx \
finder_test.cxx \
catch_main.cxx

nodist_ch_catch_SOURCES = catch.hpp catch_reporter_tap.hpp


# to run coverage, cd to root
# ./configure --enable-code-coverage
# make check-code-coverage

EXTRA_DIST = test.sh catch-runner.sh
