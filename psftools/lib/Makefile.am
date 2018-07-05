## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=-I$(top_srcdir)/include

lib_LTLIBRARIES=libpsf.la 
noinst_PROGRAMS=dumpcps
#
# Binary compatibility. See libtool.info for full details, but:
# If the source has changed at all, increment the middle number
# If interfaces have been added, removed or changed, increment the
# first number and set the middle one to 0.
# If interfaces have been added increment the right-hand number.
# If interfaces have been removed set the right-hand number to 0.
# 
#
libpsf_la_LDFLAGS = -version-info 1:1:1
libpsf_la_SOURCES = psflib.c psfucs.c psfio.c psfpix.c psfver.c mappings.c \
                    psfio.h psferror.c psfutil1.c psfutil2.c psfutil3.c \
		    psfucs2.c \
8859-1.c   8859-16.c  8859-6.c	CP1251.c  CP1256.c  CP775.c  CP860.c  CP865.c \
8859-10.c  8859-2.c   8859-7.c	CP1252.c  CP1257.c  CP850.c  CP861.c  CP866.c \
8859-13.c  8859-3.c   8859-8.c	CP1253.c  CP1258.c  CP852.c  CP862.c  CP869.c \
8859-14.c  8859-4.c   8859-9.c	CP1254.c  CP437.c   CP855.c  CP863.c  CP874.c \
8859-15.c  8859-5.c   CP1250.c	CP1255.c  CP737.c   CP857.c  CP864.c  PCGEM.c \
AMSTRAD.c  BBCMICRO.c CP858.c    CP367.c  PPC437.c  PPC860.c PPC865.c \
PPCGREEK.c

dumpcps_SOURCES=dumpcps.c
dumpcps_LDADD=libpsf.la	
