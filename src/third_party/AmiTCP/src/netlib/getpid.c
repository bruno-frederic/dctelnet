RCS_ID_C="$Id: getpid.c,v 4.1 1994/09/29 23:09:02 jraja Exp $";
/*
 *      getpid() - stub for the FindTask(NULL)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/types.h>
#include <unistd.h>
#include <exec/execbase.h>
extern struct ExecBase *SysBase;

pid_t
getpid(void)
{
  return (pid_t)SysBase->ThisTask;
}
