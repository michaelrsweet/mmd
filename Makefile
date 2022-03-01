#
# Unit test makefile for miniature markdown library.
#
#     https://github.com/michaelrsweet/mmd
#
# Copyright © 2017-2022 by Michael R Sweet.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

VERSION	=	1.9
prefix	=	$(DESTDIR)/usr/local
bindir	=	$(prefix)/bin
mandir	=	$(prefix)/share/man

CC	=	gcc
CFLAGS	=	$(OPTIM) -Wall '-DVERSION="$(VERSION)"'
LDFLAGS	=	$(OPTIM)
LIBS	=
OBJS	=	testmmd.o mmd.o mmdutil.o
OPTIM	=	-Os -g

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

# Scan the code using Cppcheck <http://cppcheck.sourceforge.net>
cppcheck:
	cppcheck --template=gcc --addon=cert.py --suppress=cert-MSC24-C --suppress=cert-EXP05-C --suppress=cert-API01-C $(OBJS:.o=.c) 2>cppcheck.log
	@test -s cppcheck.log && (echo ""; echo "Errors detected:"; echo ""; cat cppcheck.log; exit 1) || exit 0


sanitizer:
	$(MAKE) clean
	$(MAKE) OPTIM="-g -fsanitize=address" all

mmdutil:	mmd.o mmdutil.o
	$(CC) $(LDFLAGS) -o mmdutil mmd.o mmdutil.o $(LIBS)

testmmd:	mmd.o testmmd.o testmmd.md
	$(CC) $(LDFLAGS) -o testmmd mmd.o testmmd.o $(LIBS)

test:	testmmd
	./testmmd testmmd.md >testmmd.html 2>testmmd.log

$(OBJS):	mmd.h Makefile

DOCUMENTATION.html:	DOCUMENTATION.md testmmd
	./testmmd DOCUMENTATION.md >DOCUMENTATION.html 2>/dev/null

commonmark:	testmmd commonmark.md
	./testmmd --spec commonmark.md --ext none -o commonmark.log
