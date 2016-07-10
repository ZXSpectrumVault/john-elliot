
VERSION=1.0.0
DIST=zxlilo.c README Makefile

CC = gcc
CFLAGS = -Wall -ansi -pedantic -g

all:	zxlilo

dist:	$(DIST)
	mkdir zxlilo-$(VERSION)
	ln $(DIST) zxlilo-$(VERSION)
	tar zcvf zxlilo-$(VERSION).tar.gz zxlilo-$(VERSION)/*
	rm zxlilo-$(VERSION)/*
	rmdir zxlilo-$(VERSION)
