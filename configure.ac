#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nuspell],[2.0.0],[https://github.com/hunspell/nuspell/issues],,
        [https://nuspell.org])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE

AX_CODE_COVERAGE
AS_IF([test x$enable_code_coverage = xyes],[
	: ${CXXFLAGS=""}; : ${CFLAGS=""}
])

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

AC_ARG_WITH(warnings,[  --with-warnings         compile with warning messages],[
	AC_DEFINE(HUNSPELL_WARNING_ON,1,"Define if you need warning messages")
])

CURSESLIB=""
AC_ARG_WITH(
	[ui],
	[AS_HELP_STRING([--with-ui],[support Curses user interface])],
	[],
	[with_ui=no]
)
AS_IF([test "x$with_ui" != xno], [
	AC_CHECK_LIB([ncursesw],[tparm],[
		CURSESLIB=-lncursesw
	],[AC_CHECK_LIB([curses],[tparm],[
		CURSESLIB=-lcurses
	],[AC_CHECK_LIB([ncurses],[tparm],[
		CURSESLIB=-lncurses
	])])])
   if test "$CURSESLIB" != "" ; then
	echo Compiling with curses user interface.
	AC_CHECK_HEADERS([curses.h])
	if test "$CURSESLIB" = "-lncursesw" ; then
		AC_CHECK_HEADERS([ncursesw/curses.h])
	else
		echo "No Unicode support on interactive console. (Install Ncursesw library.)"
	fi
	AC_DEFINE(HUNSPELL_WARNING_ON,1,"Define if you need warning messages")
   fi
])
AC_SUBST(CURSESLIB)

AC_ARG_WITH(
	[readline],
	[AS_HELP_STRING([--with-readline],[support fancy command input editing])],
	[],
	[with_readline=no]
)
READLINELIB=""
AS_IF([test "x$with_readline" != xno],
	[AC_CHECK_LIB([curses],[tparm],[
		TERMLIB=-lncurses
	],[AC_CHECK_LIB([termcap],[tgetent],[
		TERMLIB=-ltermcap
	])])
	LDSAVE=$LDFLAGS
	LDFLAGS="$LDFLAGS $TERMLIB"
	AC_CHECK_LIB([readline],[readline],[
		AC_CHECK_HEADER([readline/readline.h],[
			READLINELIB="-lreadline $TERMLIB"
			echo Using the readline library.
			AC_DEFINE([HAVE_READLINE],[1],
			["Define if you have fancy command input editing with Readline"])
		])
	])
])
AC_SUBST(READLINELIB)

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
