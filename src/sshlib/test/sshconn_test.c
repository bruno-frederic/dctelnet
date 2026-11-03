/* sshlib/test/sshconn_test.c -- drives ../sshconn.c, the real DCTelnet source,
 * under vamos: SshConn_Start / SshConn_End against a LIBS: with and without
 * ssh.library. Everything sshconn.c leans on in the application (requesters,
 * the console, the TCP stack, graphics.library) is replaced here; the library
 * is the built ssh.library file and nothing else. Prints [OK] / [ERROR]
 * lines, exit code 0 when everything passed.
 * Usage: sshconn_test present|missing */
#include <string.h>
#include <stdio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "DCTelnet.h"
#include "prefs.h"
#include "requesters.h"
#include "utils.h"
#include "sshconn.h"

struct PrefsStruct prefs;
struct Window *win;
long tcpSocket = 1, nBytesReceived;
struct Library *SocketBase, *GfxBase;

extern struct Library *SshBase;                 /* sshconn.c's */

static int failures;
static char infoText[256], sentStart[16];
static int infoCalls, sendCalls;

static void check(int ok, const char *what)
{
    printf("%s %s\n", ok ? "[OK]" : "[ERROR]", what);
    if (!ok)
        failures++;
}

/* --- the application's side, as sshconn.c uses it --- */

VOID InfoReq(struct Window *parent, CONST_STRPTR str, ...)
{
    infoCalls++;
    strlcpy(infoText, (const char *)str, sizeof(infoText));
}

LONG ConfirmRequester(struct Window *parent, CONST_STRPTR gadgetFormat, CONST_STRPTR str, ...)
{
    return 0;
}

BOOL GetStringRequester(struct Window *parent, STRPTR title, STRPTR prompt, STRPTR buffer, UWORD maxLen)
{
    return FALSE;
}

BOOL GetSecretRequester(struct Window *parent, STRPTR title, STRPTR prompt, STRPTR buffer, UWORD maxLen)
{
    return FALSE;
}

static char localText[256];
static int localCalls;

void LocalFmt(char *ctl, ...)
{
    /* varargs sit on the stack right after ctl; the one argument is the %s text */
    localCalls++;
    strlcpy(localText, ((const char **)&ctl)[1], sizeof(localText));
}

size_t strlcpy(char *dst, const char *src, size_t dstSize)
{
    size_t n = strlen(src);

    if (dstSize)
    {
        memcpy(dst, src, n < dstSize ? n : dstSize - 1);
        dst[n < dstSize ? n : dstSize - 1] = 0;
    }
    return n;
}

void mysprintf(char *Buffer, char *ctl, ...)
{
    Buffer[0] = 0;
}

UBYTE *ReadWholeFile(const char *path, LONG *size, LONG max)
{
    *size = 0;
    return NULL;
}

LONG FileLength(const char *path)
{
    return -1;
}

/* --- the TCP stack and graphics.library: a library base is a jump table
 * below it, so a fake one is a block of memory with JMP instructions. --- */

static LONG FakeSend(__reg("d0") LONG sock, __reg("a0") APTR buf, __reg("d1") LONG len, __reg("d2") LONG flags)
{
    if (!sendCalls++)
        memcpy(sentStart, buf, len < (LONG)sizeof(sentStart) ? len : (LONG)sizeof(sentStart));
    return len;
}

static LONG FakeRecv(__reg("d0") LONG sock, __reg("a0") APTR buf, __reg("d1") LONG len, __reg("d2") LONG flags)
{
    return 0;
}

static LONG FakeBeam(void)
{
    return 0;
}

static struct Library *FakeLibrary(void)
{
    UBYTE *mem = AllocVec(1024, MEMF_ANY | MEMF_CLEAR);

    return mem ? (struct Library *)(mem + 800) : NULL;
}

static void Vector(struct Library *base, LONG lvo, APTR func)
{
    UBYTE *at = (UBYTE *)base - lvo;

    at[0] = 0x4E; at[1] = 0xF9;                 /* JMP abs.l */
    *(ULONG *)(at + 2) = (ULONG)func;
}

int main(int argc, char **argv)
{
    BOOL present = argc > 1 && strcmp(argv[1], "present") == 0;
    BOOL started;

    SocketBase = FakeLibrary();
    GfxBase = FakeLibrary();
    if (!SocketBase || !GfxBase)
        return 20;
    Vector(SocketBase, 66, (APTR)FakeSend);
    Vector(SocketBase, 78, (APTR)FakeRecv);
    Vector(GfxBase, 384, (APTR)FakeBeam);
    check(SshBase == NULL, "no library open before SshConn_Start");
    started = SshConn_Start("bbs.test", 22, "user", "pass", "ansi", 80, 24);
    if (!present)
    {
        check(!started, "missing library: SshConn_Start fails");
        check(infoCalls == 1 && strcmp(infoText, "SSH needs LIBS:ssh.library") == 0,
              "missing library: the requester says \"SSH needs LIBS:ssh.library\"");
        check(sendCalls == 0 && !SshConn_Active() && SshBase == NULL,
              "missing library: no connection attempt (nothing sent, no session)");
    }
    else
    {
        check(started, "SshConn_Start succeeds");
        check(infoCalls == 0, "no requester");
        check(SshBase != NULL && SshBase->lib_Version == 1 && SshBase->lib_OpenCnt == 1,
              "ssh.library v1 is open");
        check(SshConn_Active() && !SshConn_Buffered(), "session exists (head fields read, no call)");
        check(sendCalls == 1 && memcmp(sentStart, "SSH-2.0-", 8) == 0,
              "the library's Ssh_Create / Ssh_TakeOutput produced the identification string");
        {
            UBYTE tbuf[16];
            LONG r = SshConn_Read(tbuf, sizeof(tbuf));       /* FakeRecv: EOF */

            check(r == 0 && localCalls == 1 && strstr(localText, "no SSH version line"),
                  "EOF before the version line shows the Telnet-port message");
        }
        SshConn_End(FALSE);
        check(!SshConn_Active() && SshBase == NULL, "SshConn_End deletes the session and closes the library");
    }
    return failures ? 10 : 0;
}
