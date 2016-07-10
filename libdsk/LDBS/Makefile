
CC = gcc 
CFLAGS = -g -Wall

ZIPFILES=dsk2ldbs.c  ldbs2dsk.c  ldbs.c  ldbsdump.c  ldbs.h  ldbstest.c \
	ldbs.html Makefile

EXAMPLES=examples.txt chaos.ldbs md3.ldbs textfiles.ldbs reformat.ldbs

all:	dsk2ldbs ldbs2dsk ldbstest ldbsdump

zip:	$(ZIPFILES)
	zip ldbs.zip $(ZIPFILES)
	zip examples.zip $(EXAMPLES)

#ldbsdump dsk2ldbs ldbs

dsk2ldbs:	dsk2ldbs.o ldbs.o
	$(CC) -o $@ dsk2ldbs.o ldbs.o

ldbs2dsk:	ldbs2dsk.o ldbs.o
	$(CC) -o $@ ldbs2dsk.o ldbs.o

ldbstest:	ldbstest.o ldbs.o
	$(CC) -o $@ ldbstest.o ldbs.o


clean:
	rm *.o ldbs2dsk dsk2ldbs ldbstest
