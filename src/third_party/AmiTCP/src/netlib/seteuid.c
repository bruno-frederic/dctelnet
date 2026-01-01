RCS_ID_C="$Id: seteuid.c,v 4.1 1994/09/29 23:09:02 jraja Exp $";
/*
 *      seteuid() - set effective user
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/types.h>
#include <unistd.h>

#include <proto/usergroup.h>

#ifdef seteuid
#undef seteuid
#endif

int
seteuid(uid_t u)
{
  return setreuid(-1, u);
}

