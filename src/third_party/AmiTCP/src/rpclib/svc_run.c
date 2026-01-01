/*
 *      $Id: svc_run.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      The rpc server side idle loop: Wait for input, call server program.
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)svc_run.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_run.c 1.1 87/10/13 Copyr 1984 Sun Micro";
#endif

#include <sys/param.h>
#include <rpc/rpc.h>
#include <errno.h>
#include <sys/socket.h>

void
svc_run()
{
#ifdef FD_SETSIZE
	fd_set readfds;
#else
      int readfds;
#endif /* def FD_SETSIZE */

	for (;;) {
#ifdef FD_SETSIZE
		readfds = svc_fdset;
#else
		readfds = svc_fds;
#endif /* def FD_SETSIZE */
	
	switch (select(_rpc_dtablesize(), &readfds, NULL, NULL,
			       NULL)) {
		case -1:
#ifndef AMITCP /* EINTR is returned in case of a CTRL-C by default */
			if (errno == EINTR) {
				continue;
			}
#endif
			perror("svc_run: - select failed");
			return;
		case 0:
			continue;
		default:
			svc_getreqset(&readfds);
		}
	}
}
