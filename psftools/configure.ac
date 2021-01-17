dnl Process this file with autoconf to produce a configure script.
UPDATED='January 10, 2021'
AC_INIT(psftools, 1.1.0)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
dnl Obsoleted by LIBTOOL AC_PROG_RANLIB
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL

dnl GCC extra options
if test "$ac_cv_prog_gcc" = "yes"; then
  CFLAGS="-Wall $CFLAGS"
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(errno.h sys/ioctl.h sys/stat.h unistd.h fcntl.h limits.h)

dnl Checks for functions
AC_CHECK_FUNCS(strcmpi stricmp strcasecmp, break)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_EXEEXT
AC_OBJEXT

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_HEADER_MAJOR
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(strerror)

AC_OUTPUT(Makefile include/Makefile lib/Makefile winfonts/Makefile tools/Makefile man/Makefile doc/Makefile Fonts/Makefile)
