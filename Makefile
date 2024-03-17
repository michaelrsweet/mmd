#
# Unit test makefile for miniature markdown library.
#
#     https://github.com/michaelrsweet/mmd
#
# Copyright © 2017-2024 by Michael R Sweet.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

VERSION	=	2.0
prefix	=	$(DESTDIR)/usr/local
bindir	=	$(prefix)/bin
mandir	=	$(prefix)/share/man

CC	=	gcc
CFLAGS	=	$(OPTIM) $(CPPFLAGS) -Wall
CPPFLAGS =	'-DVERSION="$(VERSION)"'
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


sanitizer:
	$(MAKE) clean
	$(MAKE) OPTIM="-g -fsanitize=address" all


# Fuzz-test the library <>
.PHONY: afl
afl:
	$(MAKE) -$(MAKEFLAGS) CC="afl-clang-fast" OPTIM="-g" clean all
	test afl-output || rm -rf afl-output
	afl-fuzz -x afl-pdf.dict -i afl-input -o afl-output -V 600 -e pdf -t 5000 ./testmmd @@


# Analyze code with the Clang static analyzer <https://clang-analyzer.llvm.org>
clang:
	clang $(CPPFLAGS) --analyze $(OBJS:.o=.c) 2>clang.log
	rm -rf $(OBJS:.o=.plist)
	test -s clang.log && (echo "$(GHA_ERROR)Clang detected issues."; echo ""; cat clang.log; exit 1) || exit 0


# Analyze code using Cppcheck <http://cppcheck.sourceforge.net>
cppcheck:
	cppcheck $(CPPFLAGS) --template=gcc --addon=cert.py --suppressions-list=.cppcheck $(OBJS:.o=.c) 2>cppcheck.log
	test -s cppcheck.log && (echo "$(GHA_ERROR)Cppcheck detected issues."; echo ""; cat cppcheck.log; exit 1) || exit 0

# Make various bits...
mmdutil:	mmd.o mmdutil.o
	$(CC) $(LDFLAGS) -o mmdutil mmd.o mmdutil.o $(LIBS)

testmmd:	mmd.o testmmd.o testmmd.md
	$(CC) $(LDFLAGS) -o testmmd mmd.o testmmd.o $(LIBS)

test:	testmmd
	./testmmd testmmd.md >testmmd.html 2>testmmd.log
	./testmmd <testmmd.md >testmmd.html 2>>testmmd.log

$(OBJS):	mmd.h Makefile

DOCUMENTATION.html:	DOCUMENTATION.md testmmd
	./testmmd DOCUMENTATION.md >DOCUMENTATION.html 2>/dev/null

# Test against commonmark "spec"...
commonmark:	testmmd commonmark.md
	./testmmd --spec commonmark.md --ext none -o commonmark.log
