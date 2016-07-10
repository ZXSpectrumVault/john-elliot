#
# IQRun is part of UnQuill. The "linker" was written before UnQuill's own
# Z-code output module was, to do the same thing that UnQuill does.
# Now that UnQuill exists the linker is obsolete.
#
all:	iqrun.z5 # linker subsunk.z5 subscpc.z5 bugsy1.z5

bugsy1.z5:	linker iqrun.z5
	./linker iqrun.z5 bugsy1.s64 bugsy1.z5 64

subscpc.z5:	linker iqrun.z5
	./linker iqrun.z5 subscpc.sna subscpc.z5 C

subsunk.z5:	linker iqrun.z5
	./linker iqrun.z5 subsunk.sna subsunk.z5

linker:	linker.c

iqrun.z5: iqrun.inf iqproc.h
	inform iqrun.inf -k \$$MAX_STATIC_DATA=60000
