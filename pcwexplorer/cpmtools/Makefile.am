
noinst_LTLIBRARIES=libcpmfs.la

libcpmfs_la_SOURCES=device_libdsk.c cpmfs.c cpmfs.h cpmdir.h device.h config.h getopt.h
EXTRA_DIST=COPYING diskdefs
SOURCES=diskdefs

install-data-local:
	$(mkinstalldirs) $(datadir)/PcwPlore
	$(INSTALL_DATA) $(srcdir)/diskdefs $(datadir)/PcwPlore
