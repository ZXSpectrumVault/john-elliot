
noinst_DATA = libdsk.lyx libdsk.pdf libdsk.txt protocol.txt \
	      COPYING  TODO INSTALL README cfi.html apridisk.html \
              libdskrc.sample qrst.html

EXTRA_DIST = $(noinst_DATA)

#
# At some point LyX silently switched over to outputting files ending .text
# rather than .txt. So rename back to .txt.
#
%.txt:	%.lyx
	lyx -e text $<
	mv `basename $@ .txt`.text $@	

%.tex:	%.lyx
	lyx -e latex $<

%.pdf:	%.lyx
	lyx -e pdf $<
