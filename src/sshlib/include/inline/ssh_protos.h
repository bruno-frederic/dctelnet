#ifndef _VBCCINLINE_SSH_H
#define _VBCCINLINE_SSH_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct SshSession * __Ssh_Create(__reg("a6") void *, __reg("a0") struct SshNewSession * s)="\tjsr\t-30(a6)";
#define Ssh_Create(s) __Ssh_Create(SshBase, (s))

VOID __Ssh_Delete(__reg("a6") void *, __reg("a0") struct SshSession * s)="\tjsr\t-36(a6)";
#define Ssh_Delete(s) __Ssh_Delete(SshBase, (s))

ULONG __Ssh_Receive(__reg("a6") void *, __reg("a0") struct SshSession * s, __reg("a1") const UBYTE * data, __reg("d0") ULONG len)="\tjsr\t-42(a6)";
#define Ssh_Receive(s, data, len) __Ssh_Receive(SshBase, (s), (data), (len))

VOID __Ssh_Send(__reg("a6") void *, __reg("a0") struct SshSession * s, __reg("a1") const UBYTE * data, __reg("d0") ULONG len)="\tjsr\t-48(a6)";
#define Ssh_Send(s, data, len) __Ssh_Send(SshBase, (s), (data), (len))

ULONG __Ssh_TakeOutput(__reg("a6") void *, __reg("a0") struct SshSession * s, __reg("a1") UBYTE * out, __reg("d0") ULONG max)="\tjsr\t-54(a6)";
#define Ssh_TakeOutput(s, out, max) __Ssh_TakeOutput(SshBase, (s), (out), (max))

ULONG __Ssh_TakeText(__reg("a6") void *, __reg("a0") struct SshSession * s, __reg("a1") UBYTE * out, __reg("d0") ULONG max)="\tjsr\t-60(a6)";
#define Ssh_TakeText(s, out, max) __Ssh_TakeText(SshBase, (s), (out), (max))

VOID __Ssh_WindowChange(__reg("a6") void *, __reg("a0") struct SshSession * s, __reg("d0") ULONG cols, __reg("d1") ULONG rows)="\tjsr\t-66(a6)";
#define Ssh_WindowChange(s, cols, rows) __Ssh_WindowChange(SshBase, (s), (cols), (rows))

VOID __Ssh_KeepAlive(__reg("a6") void *, __reg("a0") struct SshSession * s)="\tjsr\t-72(a6)";
#define Ssh_KeepAlive(s) __Ssh_KeepAlive(SshBase, (s))

VOID __Ssh_Disconnect(__reg("a6") void *, __reg("a0") struct SshSession * s)="\tjsr\t-78(a6)";
#define Ssh_Disconnect(s) __Ssh_Disconnect(SshBase, (s))

VOID __Ssh_Fingerprint(__reg("a6") void *, __reg("a0") const UBYTE * fp, __reg("a1") char * out)="\tjsr\t-84(a6)";
#define Ssh_Fingerprint(fp, out) __Ssh_Fingerprint(SshBase, (fp), (out))

VOID __Ssh_AddEntropy(__reg("a6") void *, __reg("a0") const UBYTE * data, __reg("d0") ULONG len)="\tjsr\t-90(a6)";
#define Ssh_AddEntropy(data, len) __Ssh_AddEntropy(SshBase, (data), (len))

#endif /* _VBCCINLINE_SSH_H */
