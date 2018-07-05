## Process this file with automake to produce Makefile.in

SUBDIRS= . include lib tools man doc
ACLOCAL_AMFLAGS=-I m4

EXTRA_DIST= $(srcdir)/config/* \
	    $(srcdir)/m4/* \
	    $(srcdir)/dskwrite/* \
	    $(srcdir)/dos16ppd/mk.bat        \
	    $(srcdir)/dos16ppd/errno.h       \
	    $(srcdir)/dos16ppd/int25l.as     \
	    $(srcdir)/dos16ppd/config.h	     \
	    $(srcdir)/dos32gcc/Makefile	     \
	    $(srcdir)/dos32gcc/config.h      \
            $(srcdir)/rpcserv/*.c	     \
            $(srcdir)/rpcserv/*.h	     \
            $(srcdir)/rpcserv/ldserver.rc    \
            $(srcdir)/rpcserv/ldserver.def   \
            $(srcdir)/rpcserv/ldserver.ico   \
            $(srcdir)/rpcserv/shade.bmp	     \
	    $(srcdir)/win16vc1/*.MAK         \
	    $(srcdir)/win16vc1/config.h      \
	    $(srcdir)/win16vc1/linux/*.h     \
	    $(srcdir)/win16vc1/sys/*.h       \
	    $(srcdir)/win16vc1/libdsk.def    \
	    $(srcdir)/win16bc5/*.ide         \
	    $(srcdir)/win16bc5/libdsk.def    \
	    $(srcdir)/win16bc5/config.h	     \
	    $(srcdir)/win32bc5/*.ide         \
	    $(srcdir)/win32bc5/libdsk.def    \
	    $(srcdir)/win32bc5/config.h	     \
	    $(srcdir)/win32vc6/config.h      \
	    $(srcdir)/win32vc6/*.dsp         \
	    $(srcdir)/win32vc6/*.idl         \
	    $(srcdir)/win32vc6/*.rc          \
	    $(srcdir)/win32vc6/*.def         \
	    $(srcdir)/win32vc6/*.h           \
	    $(srcdir)/win32vc6/*.c           \
	    $(srcdir)/win32vc6/*.rgs         \
	    $(srcdir)/win32vc6/*.cpp         \
	    $(srcdir)/win32vc6/vbclient/*    \
	    $(srcdir)/win32vc6/*.dsw         \
	    $(srcdir)/LDBS/Makefile	     \
	    $(srcdir)/LDBS/ldbs.c	     \
	    $(srcdir)/LDBS/ldbs.h	     \
	    $(srcdir)/LDBS/ldbs2dsk.c	     \
	    $(srcdir)/LDBS/dsk2ldbs.c	     \
	    $(srcdir)/LDBS/ldbsdump.c	     \
	    $(srcdir)/LDBS/ldbscopy.c	     \
	    $(srcdir)/LDBS/ldbstest.c	     \
	    $(srcdir)/LDBS/ldbs.html

install-man:
	cd man && make install-man


