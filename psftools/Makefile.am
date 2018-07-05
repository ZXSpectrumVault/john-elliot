## Process this file with automake to produce Makefile.in

SUBDIRS= . include lib winfonts tools man doc Fonts

EXTRA_DIST= $(srcdir)/config/* $(srcdir)/unimaps/mappings.tar.gz \
	    $(srcdir)/dos32gcc/Makefile $(srcdir)/dos16ppd/Makefile \
	    $(srcdir)/dos32gcc/config.h $(srcdir)/dos16ppd/config.h \
	    $(srcdir)/dos16ppd/README \
	    $(srcdir)/README \
	    $(srcdir)/NEWS $(srcdir)/ChangeLog $(srcdir)/COPYING \
	    $(srcdir)/INSTALL 

ACLOCAL_AMFLAGS= -I m4

install-man:
	cd man && make install-man
