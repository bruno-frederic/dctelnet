RCS_ID_C="$Id: in.example.c,v 4.1 1994/09/30 00:12:35 jraja Exp $";
/*
 *      in.example.c - an example server to be started by inetd
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <clib/netlib_protos.h>

void
main(int argc, char **argv)
{
  char buffer[81];
  char *cp, *cr;

  /*
   * If init_inet_daemon() fails, we continue and work in normal stdio mode,
   * i.e., the stdio is not bound to a socket. In this case, none of the
   * bsdsocket.library functions can be used on the files 0, 1 or 2. This
   * doesn't matter, since we aren't going to use any of them anyway.
   */
  int sock = init_inet_daemon();
  if (sock >= 0)
    set_socket_stdio(sock);
  /* else use normal stdio */

  /*
   * Simple loop that gets one line, and then puts it out in reverse.
   */
  while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    /*
     * Print the text in reverse
     */
    for (cp = buffer + strlen(buffer) - 1, cr = NULL; cp >= buffer; cp--) {
      if (*cp == '\n' || *cp == '\r')
	cr = cp;
      else
	fputc(*cp, stdout);
    }
    /*
     * print end of line if it was found
     */
    if (cr != NULL)
      fputs(cr, stdout);
  }
}
