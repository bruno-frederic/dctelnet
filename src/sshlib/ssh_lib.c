/* sshlib/ssh_lib.c -- ssh.library: the SSH engine (../ssh.c, ../crypto/) behind
 * an AmigaOS library vector table. Engine only: exec is the one library it
 * uses. The library has no per-opener data: a session belongs to whoever made
 * it, and the one shared thing, the entropy pool of crypto/random.c, is
 * guarded by a semaphore in the library base. The ROM tag is in ssh_romtag.asm
 * (first in the file); the C library functions the engine calls are in
 * ssh_libc.c. Interface: include/libraries/ssh.h. */
#include <exec/types.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include "../ssh.h"
#include "../crypto/random.h"

#define LIB_VERSION_NUMBER 1
#define LIB_REVISION_NUMBER 0

struct SshBase
{
    struct Library lib;
    BPTR   segList;
    struct SignalSemaphore randomLock;      /* around the entropy pool */
};

/* What Ssh_Create allocates: the engine's session, then the application's
 * callbacks (copied). The session starts with the public head. */
struct SshLibSession
{
    struct Ssh ssh;
    struct SshCallbacks cb;
};

struct ExecBase *SysBase;                   /* the one writable global, set in Init */
static struct SshBase *sshBase;

extern const char SshLib_Name[], SshLib_IdString[];     /* ssh_romtag.asm */

/* --- the entropy pool's lock (crypto/random.c, SSH_LIBRARY) --- */

void Random_Lock(void)
{
    ObtainSemaphore(&sshBase->randomLock);
}

void Random_Unlock(void)
{
    ReleaseSemaphore(&sshBase->randomLock);
}

/* --- the engine's callbacks, handed on to the application's with registers --- */

static BOOL HostKeyThunk(struct Ssh *s, const UBYTE *blob, size_t len, const UBYTE fp[32])
{
    return ((struct SshLibSession *)s)->cb.hostKey((struct SshSession *)s, blob, (ULONG)len, fp);
}

static BOOL AskThunk(struct Ssh *s, const char *prompt, BOOL echo, char *out, size_t max)
{
    return ((struct SshLibSession *)s)->cb.ask((struct SshSession *)s, prompt, echo, out, (ULONG)max);
}

static void NoteThunk(struct Ssh *s, const char *text)
{
    ((struct SshLibSession *)s)->cb.note((struct SshSession *)s, text);
}

/* --- the library functions (LVO -30 and on, see include/fd/ssh_lib.fd) --- */

static struct SshSession * LibCreate(__reg("a0") struct SshNewSession *n,
                                                 __reg("a6") struct SshBase *base)
{
    struct SshLibSession *l;

    if (!n || n->version < 1 || !n->callbacks || !n->callbacks->hostKey || !n->callbacks->ask)
        return NULL;
    l = (struct SshLibSession *)AllocVec(sizeof(struct SshLibSession), MEMF_ANY | MEMF_CLEAR);
    if (!l)
        return NULL;
    l->cb = *n->callbacks;
    Ssh_Init(&l->ssh, n->user ? n->user : "", n->pass, n->term ? n->term : "ansi", n->cols, n->rows,
             n->slowCpu, HostKeyThunk, AskThunk);
    if (l->cb.note)
        l->ssh.note = NoteThunk;
    l->ssh.userData = n->userData;
    return (struct SshSession *)l;
}

static void LibDelete(__reg("a0") struct SshSession *s, __reg("a6") struct SshBase *base)
{
    if (!s)
        return;
    memset(s, 0, sizeof(struct SshLibSession));     /* keys and the password go with it */
    FreeVec(s);
}

static ULONG LibReceive(__reg("a0") struct SshSession *s, __reg("a1") const UBYTE *data,
                                    __reg("d0") ULONG len, __reg("a6") struct SshBase *base)
{
    return Ssh_Receive((struct Ssh *)s, data, len);
}

static void LibSend(__reg("a0") struct SshSession *s, __reg("a1") const UBYTE *data,
                                __reg("d0") ULONG len, __reg("a6") struct SshBase *base)
{
    Ssh_Send((struct Ssh *)s, data, len);
}

static ULONG LibTakeOutput(__reg("a0") struct SshSession *s, __reg("a1") UBYTE *out,
                                       __reg("d0") ULONG max, __reg("a6") struct SshBase *base)
{
    return Ssh_TakeOutput((struct Ssh *)s, out, max);
}

static ULONG LibTakeText(__reg("a0") struct SshSession *s, __reg("a1") UBYTE *out,
                                     __reg("d0") ULONG max, __reg("a6") struct SshBase *base)
{
    return Ssh_TakeText((struct Ssh *)s, out, max);
}

static void LibWindowChange(__reg("a0") struct SshSession *s, __reg("d0") ULONG cols,
                                        __reg("d1") ULONG rows, __reg("a6") struct SshBase *base)
{
    Ssh_WindowChange((struct Ssh *)s, (UWORD)cols, (UWORD)rows);
}

static void LibKeepAlive(__reg("a0") struct SshSession *s, __reg("a6") struct SshBase *base)
{
    Ssh_KeepAlive((struct Ssh *)s);
}

static void LibDisconnect(__reg("a0") struct SshSession *s, __reg("a6") struct SshBase *base)
{
    Ssh_Disconnect((struct Ssh *)s);
}

static void LibFingerprint(__reg("a0") const UBYTE *fp, __reg("a1") char *out,
                                       __reg("a6") struct SshBase *base)
{
    Ssh_Fingerprint(fp, out);
}

static void LibAddEntropy(__reg("a0") const UBYTE *data, __reg("d0") ULONG len,
                                      __reg("a6") struct SshBase *base)
{
    Random_Add(data, len);
}

/* --- open, close, expunge --- */

static struct Library * LibOpen(__reg("a6") struct SshBase *base)
{
    base->lib.lib_OpenCnt++;
    base->lib.lib_Flags &= ~LIBF_DELEXP;
    return &base->lib;
}

static BPTR LibExpunge(__reg("a6") struct SshBase *base)
{
    BPTR seg;

    if (base->lib.lib_OpenCnt)
    {
        base->lib.lib_Flags |= LIBF_DELEXP;         /* still open: go when the last closes */
        return 0;
    }
    seg = base->segList;
    Remove(&base->lib.lib_Node);
    FreeMem((APTR)((char *)base - base->lib.lib_NegSize), base->lib.lib_NegSize + base->lib.lib_PosSize);
    return seg;
}

static BPTR LibClose(__reg("a6") struct SshBase *base)
{
    if (base->lib.lib_OpenCnt)
        base->lib.lib_OpenCnt--;
    if (!base->lib.lib_OpenCnt && (base->lib.lib_Flags & LIBF_DELEXP))
        return LibExpunge(base);
    return 0;
}

static ULONG LibReserved(void)
{
    return 0;
}

/* Called by exec with the new base in d0, the segment list in a0 and
 * ExecBase in a6, once, when the library is made (RTF_AUTOINIT). */
static struct SshBase * LibInit(__reg("d0") struct SshBase *base, __reg("a0") BPTR segList,
                                            __reg("a6") struct ExecBase *sys)
{
    SysBase = sys;
    sshBase = base;
    base->segList = segList;
    base->lib.lib_Node.ln_Type = NT_LIBRARY;
    base->lib.lib_Node.ln_Name = (char *)SshLib_Name;
    base->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    base->lib.lib_Version = LIB_VERSION_NUMBER;
    base->lib.lib_Revision = LIB_REVISION_NUMBER;
    base->lib.lib_IdString = (APTR)SshLib_IdString;
    InitSemaphore(&base->randomLock);
    return base;
}

/* The vector table, in LVO order: Open -6, Close -12, Expunge -18, Reserved
 * -24, then the functions from -30 (include/fd/ssh_lib.fd). */
static const LONG funcTable[] =
{
    (LONG)LibOpen, (LONG)LibClose, (LONG)LibExpunge, (LONG)LibReserved,
    (LONG)LibCreate,        /* -30 */
    (LONG)LibDelete,        /* -36 */
    (LONG)LibReceive,       /* -42 */
    (LONG)LibSend,          /* -48 */
    (LONG)LibTakeOutput,    /* -54 */
    (LONG)LibTakeText,      /* -60 */
    (LONG)LibWindowChange,  /* -66 */
    (LONG)LibKeepAlive,     /* -72 */
    (LONG)LibDisconnect,    /* -78 */
    (LONG)LibFingerprint,   /* -84 */
    (LONG)LibAddEntropy,    /* -90 */
    -1L
};

/* rt_Init of the ROM tag (RTC_AUTOINIT): base size, vectors, no InitStruct
 * table (LibInit fills the node), init routine. */
const LONG SshLib_InitTable[4] =
{
    (LONG)sizeof(struct SshBase), (LONG)funcTable, 0L, (LONG)LibInit
};
