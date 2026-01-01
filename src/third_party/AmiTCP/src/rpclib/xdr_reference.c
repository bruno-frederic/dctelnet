/*
 *      $Id: xdr_reference.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      Generic XDR routines impelmentation.
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 *      
 */
/* @(#)xdr_reference.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)xdr_reference.c 1.11 87/08/11 SMI";
#endif

/*
 * Copyright (C) 1987, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * "pointers".  See xdr.h for more info on the interface to xdr.
 */

#include <sys/param.h>
#include <stdio.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <limits.h>

/*
 * XDR an indirect pointer
 * xdr_reference is for recursively translating a structure that is
 * referenced by a pointer inside the structure that is currently being
 * translated.  pp references a pointer to storage. If *pp is null
 * the  necessary storage is allocated.
 * size is the sizeof the referneced structure.
 * proc is the routine to handle the referenced structure.
 */
bool_t XDRFUN
xdr_reference(xdrs, pp, size, proc)
	register XDR *xdrs;
	caddr_t *pp;		/* the pointer to work on */
	u_int size;		/* size of the object pointed to */
	xdrproc_t proc;		/* xdr routine to handle the object */
{
	register caddr_t loc = *pp;
	register bool_t stat;

	if (loc == NULL)
		switch (xdrs->x_op) {
		case XDR_FREE:
			return (TRUE);

		case XDR_DECODE:
			*pp = loc = (caddr_t) mem_alloc(size);
			if (loc == NULL) {
				(void) fprintf(stderr,
				    "xdr_reference: out of memory\n");
				return (FALSE);
			}
			bzero(loc, (int)size);
			break;
	}

	stat = (*(xdr_string_t)proc)(xdrs, loc, UINT_MAX);

	if (xdrs->x_op == XDR_FREE) {
		mem_free(loc, size);
		*pp = NULL;
	}
	return (stat);
}


/*
 * xdr_pointer():
 *
 * XDR a pointer to a possibly recursive data structure. This
 * differs with xdr_reference in that it can serialize/deserialiaze
 * trees correctly.
 *
 *  What's sent is actually a union:
 *
 *  union object_pointer switch (boolean b) {
 *  case TRUE: object_data data;
 *  case FALSE: void nothing;
 *  }
 *
 * > objpp: Pointer to the pointer to the object.
 * > obj_size: size of the object.
 * > xdr_obj: routine to XDR an object.
 *
 */
bool_t XDRFUN
xdr_pointer(xdrs,objpp,obj_size,xdr_obj)
	register XDR *xdrs;
	char **objpp;
	u_int obj_size;
	xdrproc_t xdr_obj;
{

	bool_t more_data;

	more_data = (*objpp != NULL);
	if (! xdr_bool(xdrs,&more_data)) {
		return (FALSE);
	}
	if (! more_data) {
		*objpp = NULL;
		return (TRUE);
	}
	return (xdr_reference(xdrs,objpp,obj_size,xdr_obj));
}
