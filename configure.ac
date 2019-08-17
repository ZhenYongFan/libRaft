#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(Makefile.am)
AC_PREREQ(2.59)

AM_INIT_AUTOMAKE(Raft,1.0.0)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

AC_ARG_ENABLE(debug,[  --enable-debug          Turn on debugging and compile time warnings],
  [enable_debug=yes],[enable_debug=no])dnl

if test $enable_debug = yes; then
	CFLAGS="-g -Wall -O0 -std=c++11 "
	CXXFLAGS="-g -Wall -O0 -std=c++11 "
else
	CFLAGS="-O2  -std=c++11 "
	CXXFLAGS="-O2  -std=c++11 "
fi

# Checks for library functions.
AC_HEADER_STDC
AC_FUNC_STAT
AC_CHECK_FUNCS([getcwd memmove memset strcasecmp strchr])
AC_OUTPUT([
	Makefile
        libRaftCore/Makefile
        TestRaftCore/Makefile
])
