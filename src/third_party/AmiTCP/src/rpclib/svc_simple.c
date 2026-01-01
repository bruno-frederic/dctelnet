/*
 *      $Id: svc_simple.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      Simplified front end to rpc.
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)svc_simple.c	2.2 88/08/01 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_simple.c 1.18 87/08/11 Copyr 1984 Sun Micro";
#endif

/* 
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <netdb.h>

static struct proglst {
	char *(*p_progname)(char *);
	int  p_prognum;
	int  p_procnum;
	xdrproc_t p_inproc, p_outproc;
	struct proglst *p_nxt;
} *proglst;
static void universal(struct svc_req *rqstp, SVCXPRT *transp);
static SVCXPRT *transp;
struct proglst *pl;

registerrpc(u_long prognum, u_long versnum, u_long procnum, 
	    char *(*progname)(char *), xdrproc_t inproc, xdrproc_t outproc)
{
	
	if (procnum == NULLPROC) {
		(void) fprintf(stderr,
		    "can't reassign procedure number %lu\n", NULLPROC);
		return (-1);
	}
	if (transp == 0) {
		transp = svcudp_create(RPC_ANYSOCK);
		if (transp == NULL) {
			(void) fprintf(stderr, "couldn't create an rpc server\n");
			return (-1);
		}
	}
	(void) pmap_unset((u_long)prognum, (u_long)versnum);
	if (!svc_register(transp, (u_long)prognum, (u_long)versnum, 
	    universal, IPPROTO_UDP)) {
	    	(void) fprintf(stderr, "couldn't register prog %lu vers %lu\n",
		    prognum, versnum);
		return (-1);
	}
	pl = (struct proglst *)mem_alloc(sizeof(struct proglst));
	if (pl == NULL) {
		(void) fprintf(stderr, "registerrpc: out of memory\n");
		return (-1);
	}
	pl->p_progname = progname;
	pl->p_prognum = prognum;
	pl->p_procnum = procnum;
	pl->p_inproc = inproc;
	pl->p_outproc = outproc;
	pl->p_nxt = proglst;
	proglst = pl;
	return (0);
}

static void
universal(struct svc_req *rqstp, SVCXPRT *transp)
{
	int prog, proc;
	char *outdata;
	char xdrbuf[UDPMSGSIZE];
	struct proglst *pl;

	/* 
	 * enforce "procnum 0 is echo" convention
	 */
	if (rqstp->rq_proc == NULLPROC) {
		if (svc_sendreply(transp, xdr_void, (char *)NULL) == FALSE) {
			(void) fprintf(stderr, "xxx\n");
			exit(1);
		}
		return;
	}
	prog = rqstp->rq_prog;
	proc = rqstp->rq_proc;
	for (pl = proglst; pl != NULL; pl = pl->p_nxt)
		if (pl->p_prognum == prog && pl->p_procnum == proc) {
			/* decode arguments into a CLEAN buffer */
			bzero(xdrbuf, sizeof(xdrbuf)); /* required ! */
			if (!svc_getargs(transp, pl->p_inproc, xdrbuf)) {
				svcerr_decode(transp);
				return;
			}
			outdata = (*(pl->p_progname))(xdrbuf);
			if (outdata == NULL && pl->p_outproc != xdr_void)
				/* there was an error */
				return;
			if (!svc_sendreply(transp, pl->p_outproc, outdata)) {
				(void) fprintf(stderr,
				    "trouble replying to prog %lu\n",
				    pl->p_prognum);
				exit(1);
			}
			/* free the decoded arguments */
			(void)svc_freeargs(transp, pl->p_inproc, xdrbuf);
			return;
		}
	(void) fprintf(stderr, "never registered prog %lu\n", prog);
	exit(1);
}

