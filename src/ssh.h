/* src/ssh.h -- an SSH-2 client for BBSes (RFC 4253/4252/4254), as a pure
 * state machine: bytes from the socket go in (Ssh_Receive), bytes for the
 * socket and text for the terminal come out (Ssh_TakeOutput, Ssh_TakeText),
 * typed keys go in (Ssh_Send). No socket, no screen: the whole protocol is
 * tested on the host against a real SSH server.
 *
 * Algorithms (what the BBS servers offer, probed 2026-09-28): key exchange
 * curve25519-sha256 (all) and diffie-hellman-group14-sha256 (Synchronet;
 * off when slowCpu -- a 68000 needs minutes for it), host key rsa-sha2-256,
 * cipher aes128-ctr, MAC hmac-sha2-256, no compression. Login: password,
 * then keyboard-interactive. One session channel with a pty and a shell. */
#ifndef SSH_H
#define SSH_H

#include <exec/types.h>
#include <string.h>
#include "crypto/aes.h"
#include "crypto/sha256.h"

#define SSH_BUF    36000            /* a whole packet (RFC 4253: 35000) */
#define SSH_MSG    17000            /* our largest message: one data chunk */
#define SSH_WINDOW 32768            /* channel bytes the server may send before we take them */
#define SSH_CHUNK  16384            /* largest channel data packet either way */

enum SshState
{
    SSH_VERSION, SSH_KEXINIT, SSH_KEXREPLY, SSH_NEWKEYS, SSH_SERVICE, SSH_AUTH,
    SSH_CHANNEL, SSH_OPEN, SSH_CLOSED
};

struct SshDirection
{
    struct Aes128Ctr aes;
    UBYTE mac[32];
    ULONG seq;
    BOOL  on;
};

struct Ssh;

/* The host key's SHA-256 fingerprint (of the key blob): TRUE accepts it. */
typedef BOOL (*SshHostKeyFn)(struct Ssh *s, const UBYTE *blob, size_t len, const UBYTE fp[32]);
/* A password or keyboard-interactive answer (prompt may be ""): TRUE with
 * the answer in out. */
typedef BOOL (*SshAskFn)(struct Ssh *s, const char *prompt, BOOL echo, char *out, size_t max);

/* Optional: told before each slow step of the first key exchange, which
 * takes seconds on a 68020 and a minute on a 68000. */
typedef void (*SshNoteFn)(struct Ssh *s, const char *text);

struct Ssh
{
    int   state;
    char  error[160];               /* why it closed ("" = the server closed normally) */
    BOOL  serverClosed;             /* the server ended it (else DCTelnet did, and said so) */
    char  user[64], pass[64], term[32];
    UWORD cols, rows;
    BOOL  slowCpu;                  /* no group14 */
    SshHostKeyFn hostKey;
    SshAskFn ask;
    SshNoteFn note;
    void *user_data;
    const char *kexName;            /* the key exchange agreed on */
    UWORD kexCount;                 /* key exchanges completed (more than 1: the server rekeyed) */

    char  vc[40], vs[256];
    UBYTE ic[512], is[4096];
    UWORD icLen, isLen;
    int   kex;                      /* 0 curve25519, 1 group14 */
    int   resume;                   /* the state a key exchange returns to */
    BOOL  failing;                  /* Fail() is sending its goodbye */
    BOOL  rekeying, serviceAsked, authPassTried, authKiTried, kiPassUsed, ignoreNext;
    UBYTE xPriv[32], xPub[32];
    UBYTE dhX[32], dhE[256];
    UBYTE H[32], sessionId[32];
    BOOL  haveSession;
    UBYTE hostFp[32];               /* the host key accepted at the first key exchange */
    UBYTE K[262];                   /* the shared secret as an mpint (length + value) */
    UWORD kLen;
    struct SshDirection in, out, nextIn, nextOut;

    ULONG remoteChan, remoteWindow, remoteMax, localWindow;
    BOOL  channelOpen;

    UBYTE rx[SSH_BUF];              /* socket bytes not yet used (a packet decrypts in place) */
    ULONG rxLen;
    ULONG rxPacket;                 /* length field of the packet at rx (0: not decrypted yet) */
    UBYTE msg[SSH_MSG];             /* the outgoing message being built */
    ULONG msgLen;
    UBYTE tx[SSH_BUF];              /* bytes for the socket */
    ULONG txLen;
    UBYTE text[SSH_BUF];            /* text for the terminal (never more than the local window) */
    ULONG textLen;
    UBYTE pend[4096];               /* keys typed while they cannot be sent yet */
    UWORD pendLen;
};

void   Ssh_Init(struct Ssh *s, const char *user, const char *pass, const char *term,
                UWORD cols, UWORD rows, BOOL slowCpu, SshHostKeyFn hostKey, SshAskFn ask);
/* Feeds socket bytes; returns how many it took (fewer when rx is full:
 * take the output and the text, then feed the rest). */
ULONG  Ssh_Receive(struct Ssh *s, const UBYTE *data, ULONG len);
void   Ssh_Send(struct Ssh *s, const UBYTE *data, ULONG len);
void   Ssh_WindowChange(struct Ssh *s, UWORD cols, UWORD rows);
void   Ssh_Disconnect(struct Ssh *s);
void   Ssh_KeepAlive(struct Ssh *s);    /* SSH_MSG_IGNORE: shows nothing, keeps the line up */
ULONG  Ssh_TakeOutput(struct Ssh *s, UBYTE *out, ULONG max);
/* Taking text opens the channel window again (queues a window adjust). */
ULONG  Ssh_TakeText(struct Ssh *s, UBYTE *out, ULONG max);

/* "SHA256:" + base64 of fp, as OpenSSH shows it (out holds 52 bytes). */
void   Ssh_Fingerprint(const UBYTE fp[32], char *out);

#endif /* SSH_H */
