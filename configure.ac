#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nuspell],[2.1.0],[https://github.com/nuspell/nuspell/issues],,
        [https://nuspell.github.io/])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE

AX_GENERATE_CHANGELOG
AX_CODE_COVERAGE
# The following few lines default-initialize the shell variable CXXFLAGS
# e.g. abc=text normal assingment, overwrites previous
#      ${abc=text} sets abc to text only if abc was not defined previously
# We do this before calling AC_PROG_CXX because that does ${CXXFLAGS="-g -O2"}
AS_IF([test x$enable_code_coverage = xyes],[
	: ${CXXFLAGS=""}; : ${CFLAGS=""}])
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],[Enable debug build])],
	[: ${CXXFLAGS="-g -O0 -Wall -Wextra -Werror"}])
AC_ARG_ENABLE([profile],
	[AS_HELP_STRING([--enable-profile],[Enable build for profiling])],
	[: ${CXXFLAGS="-O2 -fno-omit-frame-pointer"}])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
LT_INIT
AC_PROG_AWK
AC_CHECK_PROG([ronn], [ronn], [yes])
AM_CONDITIONAL([HAVE_RONN], [test x$ronn = xyes])

# Checks for libraries.
AM_ICONV
BOOST_REQUIRE([1.62.0])
BOOST_LOCALE
PKG_CHECK_MODULES([ICU], [icu-uc])
AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Checks for header files.
AC_CHECK_HEADERS([locale.h langinfo.h])

# Checks for typedefs, structures, and compiler characteristics.
gl_VISIBILITY

# Checks for library functions.
# maybe check here if Boost Locale was build with ICU as backend

AC_CONFIG_FILES([
	Makefile
	nuspell.pc
	docs/Makefile
	src/Makefile
	src/hunspell/Makefile
	src/hunspell/hunvisapi.h
	src/parsers/Makefile
	src/tools/Makefile
	src/nuspell/Makefile
	tests/Makefile
	tests/suggestiontest/Makefile
	tests/v1cmdline/Makefile
])
AC_OUTPUT
