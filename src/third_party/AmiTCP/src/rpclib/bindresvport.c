/*
 *      $Id: bindresvport.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/*static  char sccsid[] = "@(#)bindresvport.c	2.2 88/07/29 4.0 RPCSRC 1.8 88/02/08 SMI";*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

/*
 * Bind a socket to a privileged IP port
 */
bindresvport(int sd, struct sockaddr_in *sin)
{
	int res;
	static short port;
	struct sockaddr_in myaddr;
	int i;

#define STARTPORT 600
#define ENDPORT (IPPORT_RESERVED - 1)
#define NPORTS	(ENDPORT - STARTPORT + 1)

	if (sin == (struct sockaddr_in *)0) {
		sin = &myaddr;
		bzero(sin, sizeof (*sin));
		sin->sin_family = AF_INET;
	} else if (sin->sin_family != AF_INET) {
		errno = EPFNOSUPPORT;
		return (-1);
	}
	if (port == 0) {
		port = ((u_short)getpid() % NPORTS) + STARTPORT;
	}
	res = -1;
	errno = EADDRINUSE;
	for (i = 0; i < NPORTS && res < 0 && errno == EADDRINUSE; i++) {
		sin->sin_port = htons(port++);
		if (port > ENDPORT) {
			port = STARTPORT;
		}
		res = bind(sd,
		    (struct sockaddr *)sin, sizeof(struct sockaddr_in));
	}
	return (res);
}
