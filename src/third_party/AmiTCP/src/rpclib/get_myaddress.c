/*
 *      $Id: get_myaddress.c,v 4.2 1994/09/29 23:48:50 jraja Exp $
 *
 *      Get client's IP address via gethostid().
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/* @(#)get_myaddress.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)get_myaddress.c 1.4 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/pmap_prot.h>
#include <sys/socket.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* 
 * don't use gethostbyname, which would invoke yellow pages
 */
void
get_myaddress(struct sockaddr_in *addr)
{
#ifdef AMITCP
  /*
   * AmiTCP gethostid() is documented to return our primary IP address
   * as the host id, use it.
   */
  static struct sockaddr_in saddr = { sizeof(struct sockaddr_in),
					AF_INET,
					htons(PMAPPORT),
					0 };
  *addr = saddr;
  addr->sin_addr.s_addr = gethostid();
#else
	int s;
	char buf[BUFSIZ];
	struct ifconf ifc;
	struct ifreq ifreq, *ifr;
	int len, slop;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    perror("get_myaddress: socket");
	    exit(1);
	}
	ifc.ifc_len = sizeof (buf);
	ifc.ifc_buf = buf;
#ifdef AMITCP
#define ioctl IoctlSocket
#endif
	if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
		perror("get_myaddress: ioctl (get interface configuration)");
		exit(1);
	}
	ifr = ifc.ifc_req;
	for (len = ifc.ifc_len; len; len -= sizeof ifreq) {
		ifreq = *ifr;

		if (ioctl(s, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
			perror("get_myaddress: ioctl");
			exit(1);
		}
		if ((ifreq.ifr_flags & IFF_UP) &&
		    ifr->ifr_addr.sa_family == AF_INET) {
			*addr = *((struct sockaddr_in *)&ifr->ifr_addr);
			addr->sin_port = htons(PMAPPORT);

			/*
			 * Break on first non-loopback address. 
			 * Loopback address will remain if no other suitable
			 * address is found.
			 */
		        if (!(ifreq.ifr_flags & IFF_LOOPBACK))
			        break;
		}
		/*
		 * Deal with variable length addresses
		 */
		slop = ifr->ifr_addr.sa_len - sizeof (struct sockaddr);
		if (slop) {
			ifr = (struct ifreq *) ((caddr_t)ifr + slop);
			len -= slop;
		}
		ifr++;
	}
#ifdef AMITCP
#undef ioctl
	(void) CloseSocket(s);
#else
	(void) close(s);
#endif
#endif /* !AMITCP */
}
