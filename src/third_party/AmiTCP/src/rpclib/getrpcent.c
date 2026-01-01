/*
 *      $Id: getrpcent.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)getrpcent.c	2.2 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)getrpcent.c 1.9 87/08/11  Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/socket.h>

/*
 * Internet version.
 */
struct rpcdata {
#ifdef USE_DOSIO /* AMIGA specific */
        BPTR    rpcf;
#else
	FILE	*rpcf;
#endif
#ifndef AMIGA
	char	*current;
	int	currentlen;
#endif
	int	stayopen;
#define	MAXALIASES	35
	char	*rpc_aliases[MAXALIASES];
	struct	rpcent rpc;
	char	line[BUFSIZ+1];
	char	*domain;
} *rpcdata;

static struct rpcdata *_rpcdata(void);

/*
 * The ent-functions have been made static, since the database
 * implementation should be hidden from the applications (J.R.)
 */
static void setrpcent(int f);
static void endrpcent(void);
static struct rpcent * getrpcent(void);
static struct rpcent * interpret(char *val, size_t len);

#ifndef AMIGA
static	char *index();
#endif

#if defined(AMITCP)
static char RPCDB[] = "AmiTCP:db/rpc";
#else
static char RPCDB[] = "/etc/rpc";
#endif

static struct rpcdata *
_rpcdata(void)
{
	register struct rpcdata *d = rpcdata;

	if (d == 0) {
		d = (struct rpcdata *)mem_calloc(1, sizeof (struct rpcdata));
		rpcdata = d;
	}
	return (d);
}

struct rpcent *
getrpcbynumber(number)
	register int number;
{
	register struct rpcdata *d = _rpcdata();
	register struct rpcent *p;

	if (d == 0)
		return (0);
	setrpcent(0);
	while (p = getrpcent()) {
		if (p->r_number == number)
			break;
	}
	endrpcent();
	return (p);
}

struct rpcent *
getrpcbyname(name)
	char *name;
{
	struct rpcent *rpc;
	char **rp;

	setrpcent(0);
	while(rpc = getrpcent()) {
		if (strcmp(rpc->r_name, name) == 0)
			return (rpc);
		for (rp = rpc->r_aliases; *rp != NULL; rp++) {
			if (strcmp(*rp, name) == 0)
				return (rpc);
		}
	}
	endrpcent();
	return (NULL);
}

static void
setrpcent(int f)
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;
	if (d->rpcf == NULL)
#ifdef USE_DOSIO
		d->rpcf = Open((STRPTR)RPCDB, MODE_OLDFILE);
#else
		d->rpcf = fopen(RPCDB, "r");
#endif
	else
		rewind(d->rpcf);
#ifndef AMIGA
	if (d->current)
		mem_free(d->current, d->currentlen);
	d->current = NULL;
#endif
	d->stayopen |= f;
}

static void
endrpcent(void)
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;
#ifndef AMIGA
	if (d->current && !d->stayopen) {
		mem_free(d->current, d->currentlen);
		d->current = NULL;
	}
#endif
	if (d->rpcf && !d->stayopen) {
		fclose(d->rpcf);
		d->rpcf = NULL;
	}
}

static struct rpcent *
getrpcent(void)
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return(NULL);
	if (d->rpcf == NULL &&
#ifdef USE_DOSIO
	    (d->rpcf = Open((STRPTR)RPCDB, MODE_OLDFILE))
#else
	    (d->rpcf = fopen(RPCDB, "r"))
#endif
	    == NULL)
		return (NULL);
    if (fgets(d->line, BUFSIZ, d->rpcf) == NULL)
		return (NULL);
	return interpret(d->line, strlen(d->line));
}

static struct rpcent *
interpret(char *val, size_t len)
{
	register struct rpcdata *d = _rpcdata();
	char *p;
	register char *cp, **q;

	if (d == 0)
		return NULL;
	strncpy(d->line, val, len);
	p = d->line;
	d->line[len] = '\n';
	if (*p == '#')
		return (getrpcent());
	cp = index(p, '#');
	if (cp == NULL)
    {
		cp = index(p, '\n');
		if (cp == NULL)
			return (getrpcent());
	}
	*cp = '\0';
	cp = index(p, ' ');
	if (cp == NULL)
    {
		cp = index(p, '\t');
		if (cp == NULL)
			return (getrpcent());
	}
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	d->rpc.r_name = d->line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->rpc.r_number = atoi(cp);
	q = d->rpc.r_aliases = d->rpc_aliases;
	cp = index(p, ' ');
	if (cp != NULL)
		*cp++ = '\0';
	else
    {
		cp = index(p, '\t');
		if (cp != NULL)
			*cp++ = '\0';
	}
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->rpc_aliases[MAXALIASES - 1]))
			*q++ = cp;
		cp = index(p, ' ');
		if (cp != NULL)
			*cp++ = '\0';
		else
	    {
			cp = index(p, '\t');
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&d->rpc);
}
