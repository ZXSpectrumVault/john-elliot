## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=-I$(top_srcdir) -I$(top_srcdir)/include
LDADD=../lib/libpsf.la

bin_PROGRAMS=psf2bbc psf2bsd psf2inc psf2pbms psf2raw psf2wof psf2xbm psf2zx \
	     psfs2cpi psfs2mda psf2txt psf2wyse \
	     psfxform psfmerge psfjoin \
	     bbc2psf mda2psf raw2psf zx2psf psf2bdf wof2psf txt2psf wyse2psf \
	     psfpages page2cp2 psf2ams ams2psf \
	     cpi2psf cpicomp cpidcomp
noinst_PROGRAMS=cpitest
EXTRA_DIST=lat1-08.psf loadpsf.c load_pc.c ldshell.c \
		psfcom8.zsm psfcom8.bin \
		psfcom16.zsm psfcom16.bin

BUILT_SOURCES=latin1.inc

cpitest_SOURCES=cpitest.c cpi.h cpialloc.c cpiload.c cpisave.c cpidump.c
cpicomp_SOURCES=cpicomp.c cnvshell.c cnvshell.inc cnvshell.h \
		cpi.h cpialloc.c cpiload.c cpisave.c cpicrush.c cpidump.c
cpidcomp_SOURCES=cpidcomp.c cnvshell.c cnvshell.inc cnvshell.h \
		cpi.h cpialloc.c cpiload.c cpisave.c cpibloat.c
cpi2psf_SOURCES=cpi2psf.c cnvshell.c cnvshell.h cnvshell.inc cpi.h \
		cpialloc.c cpiload.c cpicrush.c
psfpages_SOURCES=psfpages.c
page2cp2_SOURCES=page2cp2.c 
psfmerge_SOURCES=psfmerge.c cnvmulti.h cnvmulti.c cnvshell.inc
psfxform_SOURCES=psfxform.c cnvshell.c cnvshell.inc cnvshell.h
psfjoin_SOURCES=psfjoin.c cnvmulti.h cnvmulti.c cnvshell.inc
psf2ams_SOURCES=psf2ams.c psfcom8.inc psfcom16.inc cnvshell.c cnvshell.inc \
		cnvshell.h
psf2bbc_SOURCES=psf2bbc.c cnvshell.c cnvshell.inc cnvshell.h
psf2bsd_SOURCES=psf2bsd.c cnvshell.c cnvshell.h
psf2inc_SOURCES=psf2inc.c cnvshell.c cnvshell.h
psf2pbms_SOURCES=psf2pbms.c cnvshell.c cnvshell.inc cnvshell.h
psf2raw_SOURCES=psf2raw.c cnvshell.c cnvshell.inc cnvshell.h
psf2xbm_SOURCES=psf2xbm.c cnvshell.c cnvshell.inc cnvshell.h
psf2txt_SOURCES=psf2txt.c cnvshell.c cnvshell.inc cnvshell.h
psf2wyse_SOURCES=psf2wyse.c cnvshell.c cnvshell.inc cnvshell.h
ams2psf_SOURCES=ams2psf.c cnvshell.c cnvshell.inc cnvshell.h
bbc2psf_SOURCES=bbc2psf.c cnvshell.c cnvshell.inc cnvshell.h
txt2psf_SOURCES=txt2psf.c cnvshell.c cnvshell.inc cnvshell.h
wyse2psf_SOURCES=wyse2psf.c cnvshell.c cnvshell.inc cnvshell.h
psf2bdf_SOURCES=psf2bdf.c cnvshell.c cnvshell.inc cnvshell.h
psf2zx_SOURCES=psf2zx.c cnvshell.c cnvshell.inc cnvshell.h zxflib.c zxflib.h
psfs2cpi_SOURCES=psfs2cpi.c cnvmulti.c cnvshell.inc cnvmulti.h \
		 cpi.h cpialloc.c cpiload.c cpisave.c cpicrush.c cpibloat.c
psfs2mda_SOURCES=psfs2mda.c cnvmulti.c cnvshell.inc cnvmulti.h 
mda2psf_SOURCES=mda2psf.c cnvshell.c cnvshell.inc cnvshell.h
raw2psf_SOURCES=raw2psf.c cnvshell.c cnvshell.inc cnvshell.h
wof2psf_SOURCES=wof2psf.c cnvshell.c cnvshell.inc cnvshell.h
psf2wof_SOURCES=psf2wof.c cnvshell.c cnvshell.inc cnvshell.h
zx2psf_SOURCES=zx2psf.c cnvshell.c cnvshell.inc cnvshell.h zxflib.c zxflib.h \
		latin1.inc

latin1.inc:	lat1-08.psf
	$(MAKE) $(AM_MAKEFLAGS) psf2inc
	./psf2inc $< $@


