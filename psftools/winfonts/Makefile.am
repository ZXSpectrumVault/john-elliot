## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=-I$(top_srcdir)/include
LDADD=../lib/libpsf.la 

bin_PROGRAMS=fon2fnts fnts2fon fnt2psf psf2fnt
EXTRA_DIST=win1fonts

fon2fnts_SOURCES = fon2fnts.c \
                        mz.c ne.c pe.c \
		        nerootd.c neroote.c nesubd.c nesube.c \
			peresd.c perese.c resdir.c \
                        mz.h  pe.h res16.h  rstypes.h \
                        ne.h  resdir.h  rsmacro.h
fnts2fon_SOURCES=fnts2fon.c cnvmulti.c cnvshell.inc cnvmulti.h \
                mswfnt.h mswfntr.c mswfntw.c
fnt2psf_SOURCES=fnt2psf.c cnvshell.c cnvshell.inc cnvshell.h \
                mswfnt.h mswfntr.c
psf2fnt_SOURCES=psf2fnt.c cnvshell.c cnvshell.inc cnvshell.h \
                mswfnt.h mswfntw.c

