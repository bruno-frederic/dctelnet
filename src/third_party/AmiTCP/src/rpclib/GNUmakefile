#
#       $Id: GNUmakefile,v 1.2 1994/09/29 23:48:50 jraja Exp $
# 
#       GNUmakefile for AmiTCP/IP Sun RPC 4.0 library 
#
#       Copyright © 1994 AmiTCP/IP Group,
#                        Network Solutions Development Inc.
#		         All rights reserved. 
#

MAKE = gmake

RM= delete
RM_RECURSIVE= delete all
MKDIR= makedir

DIST_LIB = $(DIST)/rpclib

RPCLIBSRC= Smakefile auth_none.c auth_unix.c authunix_prot.c \
	bindresvport.c chkabort.c clnt_generic.c \
	clnt_perror.c clnt_raw.c clnt_simple.c clnt_tcp.c \
	clnt_udp.c get_myaddress.c getrpcent.c getrpcport.c \
	gst.c pmap_clnt.c pmap_getmaps.c pmap_getport.c \
	pmap_prot.c pmap_prot2.c pmap_rmt.c rpc_callmsg.c \
	rpc_commondata.c rpc_dtablesize.c rpc_prot.c svc.c \
	svc_auth.c svc_auth_unix.c svc_raw.c svc_run.c \
	svc_simple.c svc_tcp.c svc_udp.c xdr.c \
	xdr_array.c xdr_float.c xdr_mem.c xdr_rec.c \
	xdr_reference.c xdr_stdio.c

SRCS=   $(RPCLIBSRC) GNUmakefile DISCLAIMER

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

