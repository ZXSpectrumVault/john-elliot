
CC = gcc 
CFLAGS = -g -Wall

ZIPFILES=dsk2ldbs.c  ldbs2dsk.c  ldbs.c  ldbsdump.c  ldbs.h  ldbstest.c \
	ldbscopy.c ldbs_v2.c \
	ldbs.html Makefile

EXAMPLES=examples.txt chaos.ldbs md3.ldbs textfiles.ldbs reformat.ldbs \
	32k.ldbs 32sectors.ldbs offsets.ldbs copies.ldbs gap3.ldbs

all:	dsk2ldbs ldbs2dsk ldbstest ldbsdump saveldbs.com ldbscopy ldbs_v2

zip:	$(ZIPFILES)
	zip ldbs.zip $(ZIPFILES)
	zip examples.zip $(EXAMPLES)

saveldbs.com:	saveldbs.asm
	zxcc mac saveldbs
	hexcom < saveldbs.hex > $@

#ldbsdump dsk2ldbs ldbs

dsk2ldbs:	dsk2ldbs.o ldbs.o
	$(CC) -o $@ dsk2ldbs.o ldbs.o

ldbs2dsk:	ldbs2dsk.o ldbs.o
	$(CC) -o $@ ldbs2dsk.o ldbs.o

ldbstest:	ldbstest.o ldbs.o
	$(CC) -o $@ ldbstest.o ldbs.o

ldbscopy:	ldbscopy.o ldbs.o
	$(CC) -o $@ ldbscopy.o ldbs.o

ldbs_v2:	ldbs_v2.o ldbs.o
	$(CC) -o $@ ldbs_v2.o ldbs.o

clean:
	rm *.o ldbs2dsk dsk2ldbs ldbstest ldbscopy ldbs_v2
