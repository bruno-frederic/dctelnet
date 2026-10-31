/* src/sshconn.c -- DCTelnet's SSH connection (see sshconn.h). */
#ifdef __VBCC__
    #pragma dontwarn 306                // padding in the system headers
#endif
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>             // VBeamPos()
#include <proto/socket.h>               // send(), recv()
#include <exec/execbase.h>
#include <exec/memory.h>
#ifdef __VBCC__
    #pragma popwarn
#endif
#include <string.h>
#include "DCTelnet.h"
#include "prefs.h"
#include "requesters.h"
#include "utils.h"
#include "ssh.h"
#include "knownhosts.h"
#include "crypto/random.h"
#include "sshconn.h"

#define KNOWN_HOSTS_FILE "PROGDIR:KnownHosts"
#define KNOWN_HOSTS_MAX  (256 * 1024)

extern struct ExecBase *SysBase;

static struct Ssh *ssh;                 // NULL: no SSH session
static char sshHost[128];
static UWORD sshPort;
static UBYTE raw[4096];                 // socket bytes for the engine

static struct Window *Parent(void)
{
    return STATE_IS(APP_FULLSCREEN) ? win : NULL;
}

/* The moment, as randomness: the beam position and the exec counters move
 * with every event, the date with every run. */
static void StirTime(void)
{
    struct { struct DateStamp date; LONG beam; ULONG idle, disp; } now;

    DateStamp(&now.date);
    now.beam = VBeamPos();
    now.idle = SysBase->IdleCount;
    now.disp = SysBase->DispCount;
    Random_Add(&now, sizeof(now));
}

/* The engine's bytes to the socket. */
static BOOL SendOut(void)
{
    static UBYTE out[2048];
    ULONG n;

    while ((n = Ssh_TakeOutput(ssh, out, sizeof(out))) > 0)
        if (send(tcpSocket, (char *)out, (LONG)n, 0) < 0)
            return FALSE;
    return TRUE;
}

static void Note(struct Ssh *s, const char *text)
{
    LocalFmt("\x1b[0;36mSSH: %s...\x1b[0m\r\n", (char *)text);
}

static void RememberHostKey(const char *text, LONG len, BOOL whole, const char *fp)
{
    ULONG max = (ULONG)len + 320;
    char *out = whole ? AllocVec(max, MEMF_ANY) : NULL;     // (the file with the new line)
    size_t n = out ? KnownHosts_Set(text, (size_t)len, sshHost, sshPort, fp, out, max) : 0;
    BPTR fh;

    // Written only from the whole file: a part would lose the other hosts.
    if (!n || !(fh = Open((STRPTR)KNOWN_HOSTS_FILE, MODE_NEWFILE)))
        InfoReq(Parent(), "Could not write " KNOWN_HOSTS_FILE ":\nthe key is trusted for this connection only.");
    else
    {
        Write(fh, out, (LONG)n);
        Close(fh);
    }
    if (out) FreeVec(out);
}

/* The server's host key: known and the same, or trusted by the user. */
static BOOL CheckHostKey(struct Ssh *s, const UBYTE *blob, size_t len, const UBYTE fp[32])
{
    // PROGDIR:KnownHosts, whole (none: no text). One that is there but was
    // not read (too large, no memory) checks as new and is not rewritten.
    LONG n;
    char *text = (char *)ReadWholeFile(KNOWN_HOSTS_FILE, &n, KNOWN_HOSTS_MAX);
    BOOL whole = text || FileLength(KNOWN_HOSTS_FILE) == 0;
    char shown[64];
    LONG answer;

    Ssh_Fingerprint(fp, shown);
    switch (KnownHosts_Check(text ? text : "", (size_t)n, sshHost, sshPort, shown))
    {
    case KNOWN_SAME:
        if (text) FreeVec(text);
        return TRUE;
    case KNOWN_NEW:
        answer = ConfirmRequester(Parent(), "Trust|Just This Time|Cancel",
                    "First connection to %s port %ld.\n\n"
                    "The server identifies itself with this key:\n%s\n\n"
                    "Trust it from now on?", sshHost, (LONG)sshPort, shown);
        break;
    default:
        answer = ConfirmRequester(Parent(), "Trust the New Key|Cancel",
                    "WARNING: the SSH key of %s port %ld has CHANGED.\n\n"
                    "The server may have been reinstalled -- or another\n"
                    "machine is pretending to be it, to read what you type.\n\n"
                    "The new key:\n%s\n\n"
                    "Trust it only if the sysop announced a new key.", sshHost, (LONG)sshPort, shown);
        break;
    }
    if (answer == 1)
        RememberHostKey(text ? text : "", n, whole, shown);
    if (text) FreeVec(text);
    return answer != 0;
}

/* A login question from the server: the password, or a keyboard-interactive prompt. */
static BOOL Ask(struct Ssh *s, const char *prompt, BOOL echo, char *out, size_t max)
{
    static char title[160];

    mysprintf(title, "SSH Login: %s", sshHost);
    out[0] = 0;
    return echo ? GetStringRequester(Parent(), title, (STRPTR)prompt, out, (UWORD)max)
                : GetSecretRequester(Parent(), title, (STRPTR)prompt, out, (UWORD)max);
}

BOOL SshConn_Active(void)
{
    return ssh != NULL;
}

BOOL SshConn_ClosedByServer(void)
{
    return !ssh || ssh->state != SSH_CLOSED || ssh->serverClosed;
}

BOOL SshConn_Start(const char *host, UWORD port, const char *user, const char *pass,
                   const char *term, UWORD cols, UWORD rows)
{
    ssh = (struct Ssh *)AllocVec(sizeof(struct Ssh), MEMF_ANY | MEMF_CLEAR);
    if (!ssh)
    {
        LocalFmt("SSH: not enough memory (%ld bytes).\r\n", (LONG)sizeof(struct Ssh));
        return FALSE;
    }
    strlcpy(sshHost, host, sizeof(sshHost));
    sshPort = port;
    StirTime();
    Random_Add(host, strlen(host));
    Ssh_Init(ssh, user ? user : "", pass, term, cols, rows, !(SysBase->AttnFlags & AFF_68020), CheckHostKey, Ask);
    ssh->note = Note;
    if (!SendOut())
    {
        SshConn_End(TRUE);
        return FALSE;
    }
    return TRUE;
}

/* Socket bytes into the engine (one recv). */
static LONG Fill(void)
{
    LONG n = recv(tcpSocket, raw, sizeof(raw), 0), at = 0;

    if (n <= 0)
        return n;
    StirTime();
    while (at < n && ssh->state != SSH_CLOSED)
    {
        at += (LONG)Ssh_Receive(ssh, raw + at, (ULONG)(n - at));
        if (!SendOut())
            return -1;
    }
    return n;
}

LONG SshConn_Read(UBYTE *buf, LONG size)
{
    LONG n;

    if (!ssh->textLen)
    {
        if (ssh->state == SSH_CLOSED)
            return 0;
        n = Fill();
        if (n <= 0)
            return n;
    }
    n = (LONG)Ssh_TakeText(ssh, buf, (ULONG)size);
    if (!SendOut())                                   // (a window adjust)
        return -1;
    if (n)
        return n;
    if (ssh->state == SSH_CLOSED)
    {
        if (ssh->error[0])
            LocalFmt("\r\n\x1b[0;31m%s\x1b[0m\r\n", ssh->error);
        return 0;
    }
    return SSHCONN_AGAIN;
}

BOOL SshConn_Buffered(void)
{
    return ssh && ssh->textLen;
}

LONG SshConn_Peek(void)
{
    if (!ssh->textLen && ssh->state != SSH_CLOSED && Fill() < 0)
        return -1;
    return (LONG)ssh->textLen;
}

LONG SshConn_Write(const UBYTE *buf, LONG len)
{
    StirTime();
    Ssh_Send(ssh, buf, (ULONG)len);
    return SendOut() ? len : -1;
}

void SshConn_WindowChange(UWORD cols, UWORD rows)
{
    if (!ssh)
        return;
    Ssh_WindowChange(ssh, cols, rows);
    SendOut();
}

void SshConn_KeepAlive(void)
{
    if (!ssh)
        return;
    Ssh_KeepAlive(ssh);
    SendOut();
}

void SshConn_End(BOOL quiet)
{
    if (!ssh)
        return;
    if (!quiet)
    {
        Ssh_Disconnect(ssh);
        SendOut();
    }
    memset(ssh, 0, sizeof(struct Ssh));             // keys and the password go with it
    FreeVec(ssh);
    ssh = NULL;
}
