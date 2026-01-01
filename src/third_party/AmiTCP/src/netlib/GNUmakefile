#
#       $Id: GNUmakefile,v 4.4 1996/01/21 01:17:42 jraja Exp $
# 
#       GNUmakefile for AmiTCP/IP network support library 
#
#       Copyright © 1994 AmiTCP/IP Group, 
#                        Network Solutions Development Inc.
#                        All rights reserved.
#

MAKE = gmake

RM= delete
RM_RECURSIVE= delete all
MKDIR= makedir
CC= sc

DIST_LIB = $(DIST)/netlib

CFLAGS= 

LD= 

NETLIBSRC= strerror.c autoinit.c dummy.c lineread.c \
        getopt.c printfault.c stubs.c perror.c herror.c \
        timerinit.c gettimeofday.c rcmd.c syslog.c \
        getpid.c chmod.c chown.c utime.c popen.c sleep.c usleep.c \
	printuserfault.c init_usergroup.c setegid.c seteuid.c \
	_dup.c _dup2.c stat.c _fstat.c fib.c dostat.c access.c \
        _allocufb.c _chkufb.c _close.c _lseek.c _open.c _read.c \
        _write.c fhopen.c ioctl.c iomode.c isatty.c \
	init_inet_daemon.c set_socket_stdio.c serveraccept.c \
	netlib.h fibex.h \
	linkaddr.c linkntoa.c \
	dosio_init.c dosio_sprintf.c pkts.c

SANA2LIBSRC= sana2perror.c sana2errlist.c sana2printfault.c

LIBSRC= $(NETLIBSRC) $(SANA2LIBSRC)

SRCS=   $(LIBSRC) GNUmakefile Smakefile 

all: 

DIST: $(DIST_LIB)

$(DIST_LIB): $(SRCS)
	test -d $(DIST) || mkdir $(DIST) 
	test -d $(DIST_LIB) || mkdir $(DIST_LIB)
	tar cf - $(SRCS) | (cd $(DIST_LIB); tar xf -)

RELEASE: $(SRCS)
	for F in $(SRCS) ; do \
	  rlog -R $$F && { \
	    if ident -q $$F | fgrep '$$' >/dev/null ; \
	      then rcs -q '-N$(RELEASE):$$' $$F ; \
	      else rcs -q '-N$(RELEASE):' $$F ;\
	    fi ; \
          } ;\
	done

locks:
	@rlog -R -L $(SRCS)
mylocks:
	@rlog -R -l`whoami` -L $(SRCS)
verlocks:
	@rlog -h -L $(CSRCS)

