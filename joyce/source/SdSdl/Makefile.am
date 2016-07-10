## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES=libsdsdl.la
#noinst_PROGRAMS=vfntcnv
EXTRA_DIST=6x6ful.hdr 6x6.ful 8x16ful.hdr 8x16.ful \
		   sdsdl.hxx  driver.hxx \
		   drvglobals.hxx  vfntcnv.c \
		   drv_bits.h  drvdata.h  fontdef.h  types.h \
		   drvdata.sdl LICENSE.TXT README
#
# Binary compatibility. See libtool.info for full details, but:
# If the source has changed at all, increment the middle number
# If interfaces have been added, removed or changed, increment the
# first number and set the middle one to 0.
# If interfaces have been added increment the right-hand number.
# If interfaces have been removed set the right-hand number to 0.
# 
#
libsdsdl_la_SOURCES = drv_bits.cc   drv_open.cc  monoblit.cc   \
		   opttext.cc    sdlmouse.cc   \
		   fonts.cc	 monout.cc    opttxt1.cc    \
		   sdlpat.cc     drv_fill.cc   \
                   muldiv.cc     opttxt2.cc   sdsdl.cc      \
		   monobj.cc    opttdraw.cc   \
		   sdlline.cc    isin.cc    escape.cc

# vfntcnv_SOURCES=vfntcnv.c

#vfntcnv:	vfntcnv.c
#
# This is commented out because I can't cross-compile Joyce with it in.
#
#fonts.cc:	vfntcnv $(srcdir)/6x6ful.hdr $(srcdir)/6x6.ful $(srcdir)/8x16ful.hdr $(srcdir)/8x16.ful
#	./vfntcnv $(srcdir)/8x16.ful $(srcdir)/8x16ful.hdr $(srcdir)/6x6.ful $(srcdir)/6x6ful.hdr fonts.cc

