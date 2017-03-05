#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([kit], [1], [rjose@ejorp.com])
AC_CONFIG_SRCDIR([kit.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

# Checks for libraries.
PKG_CHECK_MODULES([DEPS], [glib-2.0,sqlite3])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_SEARCH_LIBS([ceil], [m], [], [
  AC_MSG_ERROR([unable to find the ceil() function])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
