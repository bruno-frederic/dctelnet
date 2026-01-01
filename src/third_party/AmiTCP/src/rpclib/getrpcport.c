/*
 * $Id: getrpcport.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 * Copyright © 1994 AmiTCP/IP Group,
 *                  Network Solutions Development Inc.
 *                  All rights reserved. 
 */

/* @(#)getrpcport.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)getrpcport.c 1.3 87/08/11 SMI";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <netdb.h>
#include <sys/socket.h>

int
getrpcport(char *host, u_long prognum, u_long versnum, u_long proto)
{
	struct sockaddr_in addr;
	struct hostent *hp;

	if ((hp = gethostbyname(host)) == NULL)
		return (0);
	bcopy(hp->h_addr, (char *) &addr.sin_addr, hp->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port =  0;
	return (pmap_getport(&addr, prognum, versnum, proto));
}

