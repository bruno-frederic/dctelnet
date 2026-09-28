/* src/rlogin.h -- the rlogin protocol (RFC 1282). Pure, unit-tested on the host.
 *
 * Rlogin is a plain byte stream, like a raw connection, after one message
 * from the client: NUL, the client's user name, NUL, the server's user
 * name, NUL, terminal type "/" speed, NUL. BBSes such as Synchronet use it
 * to log a user in directly: the server user name is the BBS user name,
 * the client user name its password. */
#ifndef RLOGIN_H
#define RLOGIN_H

#include <exec/types.h>
#include <string.h>

/* The client's first message. user and password are the Address Book
 * entry's (either may be ""): the client name is the password (Synchronet),
 * or the user name when there is no password; an empty user name is
 * "guest". Returns its length, 0 when max is too small. */
size_t Rlogin_Handshake(const char *user, const char *password, const char *termType,
                        const char *speed, char *out, size_t max);

#endif /* RLOGIN_H */
