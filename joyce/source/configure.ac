dnl Process this file with autoconf to produce a configure script.
UPDATED='16 April 2016'
AC_INIT(joyce, [2.2.9])
AC_CONFIG_SRCDIR(bin/Joyce.cxx)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(m4)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CANONICAL_HOST

dnl Get an absolute version of $srcdir and .
DSTROOT=`pwd`
if test x"$srcdir" = x
then
        SRCROOT=`pwd`
else
        cd $srcdir
        SRCROOT=`pwd`
        cd $DSTROOT
fi

AC_CYGWIN
AC_MINGW32
AC_EXEEXT
dnl Add cpmredir to list of things to do
CPPFLAGS="$CPPFLAGS -I$SRCROOT -I$SRCROOT/cpmredir/include -I$SRCROOT/LibLink/include -DALIBROOT=${datadir}/Joyce"
LDFLAGS="$LDFLAGS -L$DSTROOT/SdlTerm -L$DSTROOT/cpmredir/lib -L$SRCROOT/LibLink/lib"

dnl
dnl [2.2.6] Allow use of system libdsk / lib765, or builtins.
dnl

AC_ARG_WITH(system-libdsk,
[  --with-system-libdsk    Use systemwide libdsk (default=no)], int_libdsk=false, int_libdsk=true)
AC_ARG_WITH(system-lib765,
[  --with-system-lib765    Use systemwide lib765 (default=no)], int_lib765=false, int_lib765=true)


dnl
dnl Use mini-STL rather than the host platform's STL
dnl

AC_ARG_WITH(ministl,
[  --with-ministl          Use Mini-STL, not the host system's STL (default=no)],MINISTL_CFLAGS="-I$SRCROOT/ministl", MINISTL_CFLAGS="")

AC_ARG_ENABLE(floppy,
[  --enable-floppy         Enable host floppy driver (default=yes)],
              , enable_floppy=yes)
if test x$enable_floppy = xyes; then
    :
else
    CFLAGS="$CFLAGS -DDISABLE_FLOPPY"
fi
dnl Don't build a .so version of LibDsk; JOYCE will use the static one.
AC_DISABLE_SHARED

CPPFLAGS="$CPPFLAGS $MINISTL_CFLAGS"

dnl Dummy-out the Java bits for LibDsk & LibLink. We won't need them.
LIBDSKJAR=
LLINKJAR=
TOOLCLASSES=
EGCLASSES=
JAVAH=true 
JAVAC=true 
JAVA=true
JAVADOC=true
JAR=true


dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LIBTOOL
dnl Obsoleted by LIBTOOL AC_PROG_RANLIB
AC_PROG_MAKE_SET

if test "$GCC" = yes
then
  CFLAGS="${CFLAGS} -Wall -D_GNU_SOURCE" 
fi



dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(glob.h termios.h)
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h sys/time.h sys/types.h unistd.h)
AC_CHECK_HEADERS(sys/vfs.h utime.h time.h)
AC_CHECK_HEADERS(errno.h sys/errno.h sys/stat.h sys/mman.h fcntl.h)
AC_CHECK_HEADERS(linux/fd.h linux/fdreg.h windows.h winioctl.h)
AC_CHECK_HEADERS(sys/socket.h sys/un.h netinet/in.h netdb.h)
AC_CHECK_HEADERS(linux/parport.h linux/ppdev.h termios.h)
AC_CHECK_HEADERS(windows.h winnt.h winsock.h conio.h)
AC_CHECK_HEADERS(kernel/fs_info.h sys/param.h sys/mount.h)

dnl
dnl Don't replicate the nassty fudge in LibLink for checking -lwsock32;
dnl just always assume it in Cygwin or MinGW.
dnl
if test "$CYGWIN" = "yes"
then
  LIBS="$LIBS -lz -lwsock32"
  CFLAGS="$CFLAGS -fvtable-thunks"
  CXXFLAGS="$CXXFLAGS -fvtable-thunks"
else 
  if test "$MINGW32" = "yes"
  then
    LIBS="$LIBS -lz -lwsock32"
    CFLAGS="$CFLAGS -fvtable-thunks"
    CXXFLAGS="$CXXFLAGS -fvtable-thunks"
  fi
fi

AM_PATH_XML2(2.0.0,[
  LIBS="$LIBS $XML_LIBS"
  CFLAGS="$CFLAGS $XML_CFLAGS $XML_CPPFLAGS"
  CXXFLAGS="$CXXFLAGS $XML_CFLAGS $XML_CPPFLAGS"
],[
dnl Old libxml check:

dnl AC_CHECK_LIB(xml, xmlAddChild, , [echo "libxml not found - aborting"; exit 1])

dnl Replace the previous libxml check with David Helder's version 
dnl from <http://xmlsoft.org/messages/0430.html>
AC_PATH_PROG(XML_CONFIG, xml-config, no)
AC_MSG_CHECKING(for libxml)
if test "$XML_CONFIG" = "no" ; then
  AC_MSG_RESULT(no)
  echo "*** Could not find libxml. Check out the libxml homepage at "
  echo "*** http://rufus.w3.org/veillard/XML/xml.html or just get "
  echo "*** the appropriate package for your system. If you have Debian, "
  echo "*** get libxml-dev"
  AC_MSG_ERROR([libxml needed])
else
  AC_MSG_RESULT(yes)

  # Check for proper version
  AC_MSG_CHECKING(for version 1.8.9 (or better) of libxml)

  # Check if old or new style xml-config
  libxml_version=`$XML_CONFIG --version`
  libxml_version=`echo $libxml_version | sed 's/ //'`
  if test `expr $libxml_version : libxml` != 0; then

    xml_config_major_version=`$XML_CONFIG --version | \
      sed 's/libxml \([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    xml_config_minor_version=`$XML_CONFIG --version | \
      sed 's/libxml \([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    xml_config_micro_version=`$XML_CONFIG --version | \
      sed 's/libxml \([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

  else

    xml_config_major_version=`$XML_CONFIG --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    xml_config_minor_version=`$XML_CONFIG --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    xml_config_micro_version=`$XML_CONFIG --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

  fi

  # Check if version is good
  if test \( $xml_config_major_version -eq 1 \) -a \
          \( $xml_config_minor_version -eq 8 \) -a \
          \( $xml_config_micro_version -lt 9 \) ; then

     AC_MSG_RESULT(no)
     echo "*** You need libxml 1.8.9 or greater. Check out the libxml "
     echo "*** homepage at http://rufus.w3.org/veillard/XML/xml.html or just get "
     echo "*** the appropriate package for your system. If you have Debian, "
     echo "*** get the latest libxml-dev"
     AC_MSG_ERROR([libxml 1.8.9 or greater needed])

  else
     AC_MSG_RESULT(yes)
  fi

  XML_LIBS=`$XML_CONFIG --libs`
  XML_CFLAGS=`$XML_CONFIG --cflags`
  LIBS="$LIBS $XML_LIBS"
  CFLAGS="$CFLAGS $XML_CFLAGS"
  CXXFLAGS="$CXXFLAGS $XML_CFLAGS"
fi 
])

AC_CHECK_LIB(png, png_init_io, , [echo "libpng not found - aborting"; exit 1])
dnl echo int_libdsk=$int_libdsk
dnl echo int_lib765=$int_lib765

if test x$int_libdsk = xfalse; then 
	AC_CHECK_LIB(dsk, dsk_open, [], [$int_libdsk = true])
fi

if test x$int_lib765 = xfalse; then 
	AC_CHECK_LIB(765, fd_new, [], [$int_lib765 = true])
fi

if test x$int_libdsk = xtrue; then
	CPPFLAGS="$CPPFLAGS -I$SRCROOT/LibDsk/include -DAUTOSHARE=${datadir}/LibDsk -DHAVE_LIBDSK_H=1 -DNOTWINDLL"
	LDFLAGS ="$LDFLAGS -L$DSTROOT/LibDsk/lib"
fi

if test x$int_lib765 = xtrue; then
	CPPFLAGS="$CPPFLAGS -I$SRCROOT/765/include"
	LDFLAGS="$LDFLAGS -L$DSTROOT/765"
fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_ST_RDEV

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_HEADER_MAJOR
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_FUNC_UTIME_NULL
AC_FUNC_MMAP
AC_CHECK_FUNCS(mkdir rmdir)
AC_CHECK_FUNCS(strerror)
AC_CHECK_FUNCS(mkstemp)

dnl Checks for functions
AC_CHECK_FUNCS(strcmpi stricmp strcasecmp, break)
if test "$CYGWIN" = "yes"
then
  SDL_CFLAGS=`sdl-config --cflags`
  SDL_LIBS=`sdl-config --libs`
  LIBS="$LIBS -lSDLmain"
else 
  if test "$MINGW32" = "yes"
  then
    SDL_CFLAGS=`sdl-config --cflags`
    SDL_LIBS=`sdl-config --libs`
    LIBS="$LIBS -lSDLmain"
  else
    SDL_VERSION=1.0.8
    AM_PATH_SDL($SDL_VERSION,
            :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)

  fi
fi

dnl Check for SDL
CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"






dnl Add our own libraries to the link line. This has to be done last,
dnl because otherwise the earlier tests try to link these libraries (which 
dnl haven't been built yet) and fail.
LIBS="-lcpmredir $LIBS"
AC_SUBST(LIBDSKJAR)
AC_SUBST(LLINKJAR)
AC_SUBST(TOOLCLASSES)
AC_SUBST(EGCLASSES)
AC_SUBST(JAVAH)
AC_SUBST(JAVAC)
AC_SUBST(JAVA)
AC_SUBST(JAVADOC)
AC_SUBST(JAR)

AM_CONDITIONAL(INTERNAL_LIBDSK, test x$int_libdsk = xtrue)
AM_CONDITIONAL(INTERNAL_LIB765, test x$int_lib765 = xtrue)

AC_OUTPUT(Makefile Xbm/Makefile share/Makefile 765/Makefile SdlTerm/Makefile \
          SdSdl/Makefile bin/Makefile Docs/Makefile \
	  cpmredir/Makefile cpmredir/include/Makefile cpmredir/lib/Makefile \
          LibDsk/Makefile LibDsk/include/Makefile LibDsk/lib/Makefile \
          LibDsk/tools/Makefile 765/lib/Makefile 765/include/Makefile \
	  LibLink/Makefile LibLink/lib/Makefile LibLink/include/Makefile \
	  LibLink/examples/Makefile )

