/* src/sshconn.h -- DCTelnet's SSH connection: the protocol engine (ssh.c)
 * on the TCP socket, with the Amiga side of it -- host key requester and
 * PROGDIR:KnownHosts, login requesters, random seeding.
 *
 * Every socket read and write of a session goes through here once
 * SshConn_Start() ran: SshConn_Read() returns terminal bytes as recv()
 * would, SshConn_Write() sends typed bytes. */
#ifndef SSHCONN_H
#define SSHCONN_H

#include <exec/types.h>

#define SSHCONN_AGAIN (-2)      /* SshConn_Read: nothing for the terminal yet (protocol only) */

BOOL SshConn_Active(void);

/* After SshConn_Read() returned 0: the server ended the session, or the
 * socket closed (TRUE) -- or DCTelnet did (a refused host key, a cancelled
 * login, a protocol error), and told the server so (FALSE). */
BOOL SshConn_ClosedByServer(void);

/* After the TCP connect: starts the SSH login. FALSE (with a message
 * printed) if it cannot. user "" is asked for when the login starts. */
BOOL SshConn_Start(const char *host, UWORD port, const char *user, const char *pass,
                   const char *term, UWORD cols, UWORD rows);

/* Terminal bytes, as recv() returns them: > 0 bytes, 0 closed, -1 error
 * (errno from the socket), SSHCONN_AGAIN nothing yet. */
LONG SshConn_Read(UBYTE *buf, LONG size);

/* Terminal bytes are waiting in the engine: WaitSelect() would not wake
 * for them, so read before waiting. */
BOOL SshConn_Buffered(void);

/* The number of terminal bytes a read would return now (0 if none). */
LONG SshConn_Peek(void);

LONG SshConn_Write(const UBYTE *buf, LONG len);
void SshConn_WindowChange(UWORD cols, UWORD rows);
void SshConn_KeepAlive(void);

/* The connection ends: says goodbye to the server (quiet = the socket is
 * already gone) and frees the session. */
void SshConn_End(BOOL quiet);

#endif /* SSHCONN_H */
