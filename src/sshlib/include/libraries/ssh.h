/* libraries/ssh.h -- ssh.library, the SSH-2 client engine of DCTelnet as an
 * AmigaOS shared library. Engine only: no sockets, no requesters, no files.
 * The caller moves bytes between its socket and Ssh_Receive / Ssh_TakeOutput,
 * and between its terminal and Ssh_TakeText / Ssh_Send. Version 1. */
#ifndef LIBRARIES_SSH_H
#define LIBRARIES_SSH_H

#include <exec/types.h>
#include <clib/compiler-specific.h>

#define SSH_NAME "ssh.library"

/* SshSession.state */
#define SSH_STATE_VERSION   0
#define SSH_STATE_KEXINIT   1
#define SSH_STATE_KEXREPLY  2
#define SSH_STATE_NEWKEYS   3
#define SSH_STATE_SERVICE   4
#define SSH_STATE_AUTH      5
#define SSH_STATE_CHANNEL   6
#define SSH_STATE_OPEN      7
#define SSH_STATE_CLOSED    8

#define SSH_SESSION_VERSION 1       /* of the public head; it grows only at the end */
#define SSH_NEWSESSION_VERSION 1

/* The public head of a session, read-only for the application (it needs no
 * library call to ask SshConn_Active or SshConn_Buffered). The library's
 * private fields follow it and are not part of the interface. The engine's
 * own struct Ssh starts with the same fields (SSH_SESSION_HEAD). */
#define SSH_SESSION_HEAD \
    UWORD  version;                 /* SSH_SESSION_VERSION */ \
    UWORD  headSize;                /* sizeof(struct SshSession) the library filled in */ \
    LONG   state;                   /* SSH_STATE_* */ \
    ULONG  textLen;                 /* terminal bytes waiting for Ssh_TakeText */ \
    BOOL   serverClosed;            /* the server ended it (else the application did) */ \
    char   error[160];              /* why it closed ("" = the server closed normally) */ \
    APTR   userData;                /* the application's, for its callbacks */

struct SshSession
{
    SSH_SESSION_HEAD
    /* private fields follow, owned by the library */
};

/* Callbacks of the application, called with register arguments (like
 * XEM_IO), so an application built with any compiler can take them. */
struct SshCallbacks
{
    /* The host key's SHA-256 fingerprint (of the key blob, 32 bytes): TRUE accepts. */
    BOOL (*__ASM__ hostKey)(__REG__(a0, struct SshSession *s), __REG__(a1, const UBYTE *blob),
                            __REG__(d0, ULONG len), __REG__(a2, const UBYTE *fp));
    /* A password or keyboard-interactive answer (prompt may be ""): TRUE with
     * the answer in out (max bytes with the NUL). */
    BOOL (*__ASM__ ask)(__REG__(a0, struct SshSession *s), __REG__(a1, const char *prompt),
                        __REG__(d0, BOOL echo), __REG__(a2, char *out), __REG__(d1, ULONG max));
    /* Optional (NULL): told before each slow step of the first key exchange. */
    void (*__ASM__ note)(__REG__(a0, struct SshSession *s), __REG__(a1, const char *text));
};

/* Ssh_Create's argument. Set version to SSH_NEWSESSION_VERSION; fields added
 * in later versions follow cols/rows... at the end. */
struct SshNewSession
{
    UWORD  version;                 /* SSH_NEWSESSION_VERSION */
    UWORD  pad;
    const char *user;
    const char *pass;               /* NULL: ask */
    const char *term;               /* the pty's TERM, e.g. "ansi" */
    UWORD  cols, rows;
    BOOL   slowCpu;                 /* no group14: a 68000 needs minutes for it */
    const struct SshCallbacks *callbacks;   /* copied; must have hostKey and ask */
    APTR   userData;
};

#endif /* LIBRARIES_SSH_H */
