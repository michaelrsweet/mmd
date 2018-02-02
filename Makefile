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

CC	=	gcc
CFLAGS	=	-g -Wall
LIBS	=
OBJS	=	testmmd.o mmd.o

.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<


all:	testmmd DOCUMENTATION.html

clean:
	rm -f testmmd $(OBJS)

testmmd:	$(OBJS) testmmd.md
	$(CC) $(CFLAGS) -o testmmd $(OBJS) $(LIBS)
	./testmmd testmmd.md >testmmd.html

$(OBJS):	mmd.h

DOCUMENTATION.html:	DOCUMENTATION.md testmmd
	./testmmd DOCUMENTATION.md >DOCUMENTATION.html
