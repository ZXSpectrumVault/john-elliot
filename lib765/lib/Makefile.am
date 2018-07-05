## Process this file with automake to produce Makefile.in

INCLUDES=-I$(top_srcdir)/include

lib_LTLIBRARIES=lib765.la
#
#
# Binary compatibility. See libtool.info for full details, but:
# If the source has changed at all, increment the middle number
# If interfaces have been added, removed or changed, increment the
# first number and set the middle one to 0.
# If interfaces have been added increment the right-hand number.
# If interfaces have been removed set the right-hand number to 0.
#
lib765_la_LDFLAGS = -version-info 4:2:1
lib765_la_SOURCES = 765drive.c 765dsk.c 765fdc.c 765ldsk.c 765i.h error.c

