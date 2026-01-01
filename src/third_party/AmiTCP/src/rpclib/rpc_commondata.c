/*
 *      $Id: rpc_commondata.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *      
 *      RPC common global data that is exported by the public interface.
 *      
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)rpc_commondata.c	2.1 88/07/29 4.0 RPCSRC */
#include <sys/param.h>
#include <rpc/rpc.h>
/*
 * This file should only contain common data (global data) that is exported
 * by public interfaces 
 */
struct opaque_auth _null_auth;
#ifdef FD_SETSIZE
fd_set svc_fdset;
#else
int svc_fds;
#endif /* def FD_SETSIZE */
struct rpc_createerr rpc_createerr;
