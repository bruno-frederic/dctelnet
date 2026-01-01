/*
 *      $Id: auth_none.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      Creates a client authentication handle for passing "null" 
 *      credentials and verifiers to remote systems. 
 *      
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)auth_none.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)auth_none.c 1.19 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc. 
 */

#include <sys/param.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#define MAX_MARSHEL_SIZE 20

/*
 * Authenticator operations routines
 */
static void	authnone_verf(AUTH *);
static bool_t	authnone_marshal(AUTH * client, XDR * xdrs);
static bool_t	authnone_validate(AUTH *, struct opaque_auth *);
static bool_t	authnone_refresh(AUTH *);
static void	authnone_destroy(AUTH *);

static struct auth_ops ops = {
	authnone_verf,
	authnone_marshal,
	authnone_validate,
	authnone_refresh,
	authnone_destroy
};

static struct authnone_private {
	AUTH	no_client;
	char	marshalled_client[MAX_MARSHEL_SIZE];
	u_int	mcnt;
} *authnone_private;

AUTH *
authnone_create()
{
	register struct authnone_private *ap = authnone_private;
	XDR xdr_stream;
	register XDR *xdrs;

	if (ap == 0) {
		ap = (struct authnone_private *)mem_calloc(1, sizeof (*ap));
		if (ap == 0)
			return (0);
		authnone_private = ap;
	}
	if (!ap->mcnt) {
		ap->no_client.ah_cred = ap->no_client.ah_verf = _null_auth;
		ap->no_client.ah_ops = &ops;
		xdrs = &xdr_stream;
		xdrmem_create(xdrs, ap->marshalled_client, (u_int)MAX_MARSHEL_SIZE,
		    XDR_ENCODE);
		(void)xdr_opaque_auth(xdrs, &ap->no_client.ah_cred);
		(void)xdr_opaque_auth(xdrs, &ap->no_client.ah_verf);
		ap->mcnt = XDR_GETPOS(xdrs);
		XDR_DESTROY(xdrs);
	}
	return (&ap->no_client);
}

/*ARGSUSED*/
static bool_t
authnone_marshal(client, xdrs)
	AUTH *client;
	XDR *xdrs;
{
	register struct authnone_private *ap = authnone_private;

	if (ap == 0)
		return (0);
	return ((*xdrs->x_ops->x_putbytes)(xdrs,
	    ap->marshalled_client, ap->mcnt));
}

static void 
authnone_verf(AUTH *client)
{
}

static bool_t
authnone_validate(AUTH *client, struct opaque_auth *verfp)
{
	return (TRUE);
}

static bool_t
authnone_refresh(AUTH * client)
{
	return (FALSE);
}

static void
authnone_destroy(AUTH * client)
{
}
