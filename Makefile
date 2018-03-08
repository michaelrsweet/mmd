#
# Unit test makefile for miniature markdown library.
#
#     https://github.com/michaelrsweet/mmd
#
# Copyright © 2017-2018 by Michael R Sweet.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

VERSION	=	1.3
prefix	=	/usr/local
bindir	=	$(prefix)/bin
mandir	=	$(prefix)/share/man

CC	=	gcc
CFLAGS	=	-Os -g -Wall '-DVERSION="$(VERSION)"'
LIBS	=
OBJS	=	testmmd.o mmd.o mmdutil.o

.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<


all:	testmmd mmdutil DOCUMENTATION.html

clean:
	rm -f testmmd $(OBJS)

install:	mmdutil
	mkdir -p $(bindir)
	cp mmdutil $(bindir)
	mkdir -p $(mandir)/man1
	./mmdutil --man 1 mmdutil.md >$(mandir)/man1/mmdutil.1

mmdutil:	mmd.o mmdutil.o
	$(CC) $(CFLAGS) -o mmdutil mmd.o mmdutil.o $(LIBS)

testmmd:	mmd.o testmmd.o testmmd.md
	$(CC) $(CFLAGS) -o testmmd mmd.o testmmd.o $(LIBS)
	./testmmd testmmd.md >testmmd.html

$(OBJS):	mmd.h

DOCUMENTATION.html:	DOCUMENTATION.md testmmd
	./testmmd DOCUMENTATION.md >DOCUMENTATION.html
