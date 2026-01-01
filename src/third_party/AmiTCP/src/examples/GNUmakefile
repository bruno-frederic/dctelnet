#
#       $Id: GNUmakefile,v 4.2 1996/03/26 22:39:58 too Exp $
# 
#       GNUmakefile for AmiTCP/IP examples
#
#       Copyright © 1994 AmiTCP/IP Group, 
#                        Network Solutions Development Inc.
#                        All rights reserved.
#

MAKE = gmake

RM= delete
RM_RECURSIVE= delete all
MKDIR= makedir

DIST_EXAMPLES = $(DIST)/examples

RPCSRC= rpc/dir/SCOPTIONS rpc/dir/Smakefile rpc/dir/dir.x \
	rpc/dir/dir_proc.c rpc/dir/rls.c \
	rpc/msg/SCOPTIONS rpc/msg/Smakefile rpc/msg/msg.x \
	rpc/msg/msg_proc.c rpc/msg/printmsg.c rpc/msg/rprintmsg.c \
	rpc/sort/SCOPTIONS rpc/sort/Smakefile rpc/sort/rsort.c \
	rpc/sort/sort.x rpc/sort/sort_proc.c

SERVSRC=serv/Smake.def serv/in.example.c

HTTPGETSRC=httpget/Makefile httpget/Smakefile \
	httpget/httpget.bin.gcc httpget/httpget.bin.sc \
	httpget/httpget.c httpget/httpget.map


SRCS=   $(RPCSRC) $(SERVSRC) $(HTTPGETSRC) Smakefile GNUmakefile

all: 

DIST: $(DIST_EXAMPLES)

$(DIST_EXAMPLES): $(SRCS)
	test -d $(DIST) || mkdir $(DIST) 
	test -d $(DIST_EXAMPLES) || mkdir $(DIST_EXAMPLES)
	tar cf - $(SRCS) | (cd $(DIST_EXAMPLES); tar xf -)

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
	@rlog -h -L $(SRCS)



