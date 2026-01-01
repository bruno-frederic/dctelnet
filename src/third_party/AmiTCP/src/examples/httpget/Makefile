#
# GNUmakefile
#
# Created: Tue Sep 14 14:11:54 1993 too
# Last modified: Sat Mar  2 17:39:25 1996 too
#
# $Id: $
#


PRG=	httpget.bin.gcc

TMP=	ram:t
RM=	delete quiet
CP=	copy

CC=	gcc
CFLAGS= -O2 -Wall -Inetinclude:

#### definitions for unix cross-compiler environment ####
#TMP=	/tmp
#RM=	rm -f
#CP=	cp

#CC=	amigados-gcc
#CFLAGS= -O2 -Wall -I../AmiTCP-4.3SDK/netinclude 

#### ####

#LFLAGS= -nostdlib
LFLAGS= -nostdlib -s


AR=	ar
AFLAGS= urv

OBJS=	httpget.o

SRC=	$(OBJ:.o=.c)

all:	$(PRG)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.s.o:
	$(CP) $*.s $(TMP)/x.c
	$(CC) -traditional -E -o $(TMP)/x.s $(TMP)/x.c -I./rtlib
	$(CC) -o $@ -c $(TMP)/x.s


$(PRG): $(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS)

clean:
	$(RM) $(OBJS)

FRC:

# 
