VERSION = 5.1.2
CC	= gcc
OBJS	= lar.o crcsubs.o
CFLAGS  = -Wall -O2
TAR     = tar zcvf
NROFF   = nroff -c -mandoc
PACK    = README Makefile lar.doc lar.1 lar.c crcsubs.c crcsubs.h lbr lbr.doc

all:	lar lar.doc lar.man

lar:	$(OBJS)
	$(CC) $(CFLAGS) -o lar $(OBJS)

lar.man: lar.1
	$(NROFF) lar.1 >lar.man
	
lar.doc: lar.man
	col -b <lar.man >lar.doc

archive: $(PACK)
	mkdir lar-$(VERSION)
	ln $(PACK) lar-$(VERSION)
	$(TAR) lar-$(VERSION).tar.gz lar-$(VERSION)
	cd lar-$(VERSION) && rm $(PACK)
	rmdir lar-$(VERSION)

clean:
	rm -f *.o lar
