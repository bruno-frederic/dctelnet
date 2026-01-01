RCS_ID_C="$Id: dummy.c,v 4.1 1994/09/29 23:09:02 jraja Exp $";
/*
 *      dummy.c - unimplemented netdb functions 
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <netdb.h>
#if !__SASC
#define NULL ((void *) 0)
#endif
struct hostent  *gethostent(void)
{ return NULL; }
struct netent  *getnetent(void)
{ return NULL; }
struct servent  *getservent(void)
{ return NULL; }
struct protoent *getprotoent(void)
{ return NULL; }
