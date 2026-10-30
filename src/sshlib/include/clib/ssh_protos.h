#ifndef CLIB_SSH_PROTOS_H
#define CLIB_SSH_PROTOS_H

#include <exec/types.h>
#include <libraries/ssh.h>

/* NULL when out of memory or the version of s is unknown. */
struct SshSession *Ssh_Create(struct SshNewSession *s);
/* Wipes the session (keys, password) and frees it. */
void   Ssh_Delete(struct SshSession *s);
/* Feeds socket bytes; returns how many it took (fewer when its buffer is
 * full: take the output and the text, then feed the rest). */
ULONG  Ssh_Receive(struct SshSession *s, const UBYTE *data, ULONG len);
/* Typed keys. */
void   Ssh_Send(struct SshSession *s, const UBYTE *data, ULONG len);
/* Bytes for the socket. */
ULONG  Ssh_TakeOutput(struct SshSession *s, UBYTE *out, ULONG max);
/* Text for the terminal; takes it from textLen. */
ULONG  Ssh_TakeText(struct SshSession *s, UBYTE *out, ULONG max);
void   Ssh_WindowChange(struct SshSession *s, ULONG cols, ULONG rows);
/* SSH_MSG_IGNORE: shows nothing, keeps the line up. */
void   Ssh_KeepAlive(struct SshSession *s);
void   Ssh_Disconnect(struct SshSession *s);
/* "SHA256:" + base64 of fp (32 bytes), as OpenSSH shows it; out holds 52 bytes. */
void   Ssh_Fingerprint(const UBYTE *fp, char *out);
/* Mixes data into the library's one entropy pool, shared by all openers. */
void   Ssh_AddEntropy(const UBYTE *data, ULONG len);

#endif /* CLIB_SSH_PROTOS_H */
