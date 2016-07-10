
if INTERNAL_LIBDSK
  MAYBE_LIBDSK = LibDsk
endif

if INTERNAL_LIB765
  MAYBE_LIB765 = 765
endif

SUBDIRS=share cpmredir $(MAYBE_LIB765) SdlTerm SdSdl $(MAYBE_LIBDSK) LibLink Xbm Docs bin .

EXTRA_DIST=$(srcdir)/config/*  $(srcdir)/ministl/* $(srcdir)/m4/* \
	COPYING.txt \
	setup/pcw.ico setup/joyce16.bmp setup/pcw16.ico setup/doc.ico \
	setup/joyce.nsi \
	md3trans.exe pcwtrans.exe pcw.png font8x16.x16 

ACLOCAL_AMFLAGS=-I m4
