/*
 *      $Id: rpc_dtablesize.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      RPC getdtablesize() cache.
 *      
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)rpc_dtablesize.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)rpc_dtablesize.c 1.2 87/08/11 Copyr 1987 Sun Micro";
#endif

#include <sys/param.h>
#include <sys/socket.h>

/*
 * Cache the result of getdtablesize(), so we don't have to do an
 * expensive system call every time.
 */
int 
_rpc_dtablesize(void)
{
	static int size;
	
	if (size == 0) {
		size = getdtablesize();
	}
	return (size);
}
