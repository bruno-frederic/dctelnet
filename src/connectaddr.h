/* src/connectaddr.h -- the Connect requester's "[user@]host[:port]" input.
 * Pure, unit-tested on the host. */
#ifndef CONNECTADDR_H
#define CONNECTADDR_H

#include <exec/types.h>
#include <string.h>

enum { CONNECTADDR_OK, CONNECTADDR_EMPTY_HOST, CONNECTADDR_BAD_PORT, CONNECTADDR_TOO_LONG };

/* Split "[user@]host[:port]" (a ',' also separates the port, as before).
 * The user is everything before the last '@'. A ':' starts the port only when
 * it is the only one in the host part, so a bare IPv6 address stays a host.
 * No port: defaultPort. Port must be 1..65535. Parts that do not fit
 * userMax / hostMax bytes (with the NUL) are CONNECTADDR_TOO_LONG.
 * user, host and port are only written on CONNECTADDR_OK. */
int ConnectAddr_Parse(const char *input, char *user, size_t userMax, char *host, size_t hostMax,
                      UWORD *port, UWORD defaultPort);

#endif /* CONNECTADDR_H */
