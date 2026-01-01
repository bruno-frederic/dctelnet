RCS_ID_C="$Id: isatty.c,v 4.1 1994/09/29 23:09:02 jraja Exp $";
/*
 *      isatty.c - check is a file is a terminal (interactive) or not (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dos.h>
#include <dos/dos.h>
#include <proto/dos.h>

int
isatty(int fd)
{
  struct UFB *ufb;

  /*
   * find the ufb *
   */
  if ((ufb = __chkufb(fd)) != NULL &&
      !(ufb->ufbflg & UFB_SOCK)) { /* A socket is not a tty */
    /*
     * Convert DOSBOOL to BOOL
     */
    return (IsInteractive(ufb->ufbfh) & 0x1);
  }
  
  return 0;
}
