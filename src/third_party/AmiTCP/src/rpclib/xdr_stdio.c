/*
 *      $Id: xdr_stdio.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      XDR implementation on standard i/o file.
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)xdr_stdio.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)xdr_stdio.c 1.16 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * This set of routines implements a XDR on a stdio stream.
 * XDR_ENCODE serializes onto the stream, XDR_DECODE de-serializes
 * from the stream.
 */

#include <sys/param.h>
#include <rpc/types.h>
#include <stdio.h>
#include <rpc/xdr.h>

static bool_t	xdrstdio_getlong(XDR * xdrs, long * lp);
static bool_t	xdrstdio_putlong(XDR * xdrs, long * lp);
static bool_t	xdrstdio_getbytes(XDR * xdrs, caddr_t addr, u_int len);
static bool_t	xdrstdio_putbytes(XDR * xdrs, caddr_t addr, u_int len);
static u_int	xdrstdio_getpos(XDR * xdrs);
static bool_t	xdrstdio_setpos(XDR * xdrs, u_int pos);
static long *	xdrstdio_inline(XDR * xdrs, u_int len);
static void	xdrstdio_destroy(XDR * xdrs);

/*
 * Ops vector for stdio type XDR
 */
static struct xdr_ops	xdrstdio_ops = {
	xdrstdio_getlong,	/* deseraialize a long int */
	xdrstdio_putlong,	/* seraialize a long int */
	xdrstdio_getbytes,	/* deserialize counted bytes */
	xdrstdio_putbytes,	/* serialize counted bytes */
	xdrstdio_getpos,	/* get offset in the stream */
	xdrstdio_setpos,	/* set offset in the stream */
	xdrstdio_inline,	/* prime stream for inline macros */
	xdrstdio_destroy	/* destroy stream */
};

/*
 * AmiTCP/IP specific
 */
#ifdef USE_DOSIO
/* dos.library file */
#define FILETYPE BPTR
#else
/* C runtime file */
#define FILETYPE FILE *
#endif

/*
 * Initialize a stdio xdr stream.
 * Sets the xdr stream handle xdrs for use on the stream file.
 * Operation flag is set to op.
 */
void
xdrstdio_create(xdrs, file, op)
	register XDR *xdrs;
	FILETYPE file;
	enum xdr_op op;
{

	xdrs->x_op = op;
	xdrs->x_ops = &xdrstdio_ops;
	xdrs->x_private = (caddr_t)file;
	xdrs->x_handy = 0;
	xdrs->x_base = 0;
}

/*
 * Destroy a stdio xdr stream.
 * Cleans up the xdr stream handle xdrs previously set up by xdrstdio_create.
 */
static void
xdrstdio_destroy(xdrs)
	register XDR *xdrs;
{
	(void)fflush((FILETYPE)xdrs->x_private);
	/* xx should we close the file ?? */
}

static bool_t
xdrstdio_getlong(xdrs, lp)
	XDR *xdrs;
	register long *lp;
{

	if (fread((caddr_t)lp, sizeof(long), 1, (FILETYPE)xdrs->x_private) != 1)
		return (FALSE);
#if !(defined(mc68000) || defined(_M68000))
	*lp = ntohl(*lp);
#endif
	return (TRUE);
}

static bool_t
xdrstdio_putlong(xdrs, lp)
	XDR *xdrs;
	long *lp;
{

#if !(defined(mc68000) || defined(_M68000))
	long mycopy = htonl(*lp);
	lp = &mycopy;
#endif
	if (fwrite((caddr_t)lp, sizeof(long), 1, (FILETYPE)xdrs->x_private) != 1)
		return (FALSE);
	return (TRUE);
}

static bool_t
xdrstdio_getbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{

	if ((len != 0) && (fread(addr, (int)len, 1, (FILETYPE)xdrs->x_private) != 1))
		return (FALSE);
	return (TRUE);
}

static bool_t
xdrstdio_putbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{

	if ((len != 0) && (fwrite(addr, (int)len, 1, (FILETYPE)xdrs->x_private) != 1))
		return (FALSE);
	return (TRUE);
}

static u_int
xdrstdio_getpos(xdrs)
	XDR *xdrs;
{

	return ((u_int) ftell((FILETYPE)xdrs->x_private));
}

static bool_t
xdrstdio_setpos(xdrs, pos) 
	XDR *xdrs;
	u_int pos;
{ 

	return ((fseek((FILETYPE)xdrs->x_private, (long)pos, SEEK_SET) < 0) ?
		FALSE : TRUE);
}

static long *
xdrstdio_inline(xdrs, len)
	XDR *xdrs;
	u_int len;
{

	/*
	 * Must do some work to implement this: must insure
	 * enough data in the underlying stdio buffer,
	 * that the buffer is aligned so that we can indirect through a
	 * long *, and stuff this pointer in xdrs->x_buf.  Doing
	 * a fread or fwrite to a scratch buffer would defeat
	 * most of the gains to be had here and require storage
	 * management on this buffer, so we don't do this.
	 */
	return (NULL);
}

