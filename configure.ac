#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([bgpdump2], [0.1], [yasu@nttv6.jp])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/bgpdump.c])
AC_CONFIG_HEADERS([config.h])

CFLAGS="-g -O3 -Wall"
defcflags=$CFLAGS;
AC_ARG_WITH([cflags],
  [AS_HELP_STRING([--with-cflags],
   [set CFLAGS environment for compilation. @<:@[default=$defcflags]@:>@])],
  [CFLAGS=$withval],
  [CFLAGS=$defcflags])
#echo "CFLAGS=$CFLAGS";

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([bz2], [BZ2_bzReadOpen])
AC_CHECK_LIB([z], [gzopen])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h stdlib.h string.h strings.h syslog.h stdint.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset strtoul])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
