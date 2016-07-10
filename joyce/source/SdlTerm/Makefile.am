
noinst_LTLIBRARIES = libsdlterm.la
noinst_PROGRAMS  = test psf2inc

BUILT_SOURCES=font8x16.inc

libsdlterm_la_SOURCES = font8x16.psf uksdl.hxx \
		       SdlContext.hxx SdlContext.cxx \
		       SdlTerm.hxx SdlTermInput.cxx SdlTermOutput.cxx \
		       SdlVt52.hxx SdlVt52.cxx font8x16.inc 
psf2inc_SOURCES = psf2inc.c cnvshell.h cnvshell.c psflib.h psfutils.h \
		  psflib.c psfutil1.c psfutil2.c psfutil3.c 
test_SOURCES  = test.cxx
test_LDADD    = libsdlterm.la -lSDL

font8x16.inc:	font8x16.psf
	$(MAKE) $(AM_MAKEFLAGS) psf2inc
	./psf2inc $< $@
