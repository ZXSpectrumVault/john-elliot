dnl Process this file with autoconf to produce a configure script.
UPDATED='21 August 2014'

AC_INIT([taptools], [1.1.0])
AC_CONFIG_SRCDIR(tapeio.c)
AM_INIT_AUTOMAKE


AM_CONFIG_HEADER(config.h)
AC_CANONICAL_HOST

AC_CYGWIN
AC_MINGW32
AC_EXEEXT

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(stat.h sys/stat.h limits.h sys.h)
AC_CHECK_HEADERS(libdsk.h, LIBS="$LIBS -ldsk",
		 [AC_MSG_WARN(libdsk.h not found - tap2dsk and mkp3fs will be very limited)])
AC_CHECK_FUNCS(mkstemp)
AC_CHECK_LIB(z, zlibVersion)
dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(strcmpi stricmp strcasecmp, break)


AC_OUTPUT(Makefile)
