/* sshlib/test/ssh_lib_test.c -- runs ssh.library under vamos. Opens the real
 * library file from LIBS:, drives it through its vector table (so a wrong LVO
 * order fails), feeds the server half of a recorded session and checks the
 * result. Prints [OK] / [ERROR] lines, exit code 0 when everything passed.
 * Usage: ssh_lib_test <recorded session .bin> */
#include <string.h>
#include <stdio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/ssh.h>
#include <proto/ssh.h>

struct Library *SshBase;

static int failures;

static void check(int ok, const char *what)
{
    printf("%s %s\n", ok ? "[OK]" : "[ERROR]", what);
    if (!ok)
        failures++;
}

static BOOL HostKey(__REG__(a0, struct SshSession *s), __REG__(a1, const UBYTE *blob),
                           __REG__(d0, ULONG len), __REG__(a2, const UBYTE *fp))
{
    return TRUE;
}

static BOOL Ask(__REG__(a0, struct SshSession *s), __REG__(a1, const char *prompt),
                       __REG__(d0, BOOL echo), __REG__(a2, char *out), __REG__(d1, ULONG max))
{
    out[0] = 0;
    return TRUE;
}

/* Record {kind, 4 byte big-endian length, data} number n (0 based) after the
 * two NUL terminated strings at the start of the file. */
static const UBYTE *Record(const UBYTE *file, LONG size, int n, ULONG *len, char *kind)
{
    const UBYTE *p = file, *end = file + size;

    while (p < end && *p++) ;
    while (p < end && *p++) ;
    for (;; n--)
    {
        ULONG l;

        if (p + 5 > end)
            return NULL;
        l = ((ULONG)p[1] << 24) | ((ULONG)p[2] << 16) | ((ULONG)p[3] << 8) | p[4];
        if (n == 0)
        {
            *kind = (char)p[0];
            *len = l;
            return p + 5;
        }
        p += 5 + l;
    }
}

int main(int argc, char **argv)
{
    static const struct SshCallbacks cb = { HostKey, Ask, NULL };
    struct SshNewSession ns;
    struct SshSession *s;
    UBYTE *file = NULL;
    LONG size = 0;
    BPTR fh;
    UBYTE out[512], fp[32];
    char b64[64], kind;
    ULONG n, len, i;
    const UBYTE *rec;

    if (argc < 2)
    {
        printf("[ERROR] usage: ssh_lib_test <session.bin>\n");
        return 20;
    }
    fh = Open((STRPTR)argv[1], MODE_OLDFILE);
    if (fh)
    {
        Seek(fh, 0, OFFSET_END);
        size = Seek(fh, 0, OFFSET_BEGINNING);
        file = AllocVec(size, MEMF_ANY);
        if (file && Read(fh, file, size) != size)
        {
            FreeVec(file);
            file = NULL;
        }
        Close(fh);
    }
    check(file != NULL, "recorded session loaded");
    if (!file)
        return 20;

    SshBase = OpenLibrary("ssh.library", 1);
    check(SshBase != NULL, "OpenLibrary(ssh.library, 1)");
    if (!SshBase)
        return 20;
    check(SshBase->lib_Version == 1, "library version is 1");

    Ssh_AddEntropy((const UBYTE *)"entropy for the test", 20);

    memset(&ns, 0, sizeof(ns));
    ns.version = SSH_NEWSESSION_VERSION;
    ns.user = "user";
    ns.pass = "pass";
    ns.term = "ansi";
    ns.cols = 80;
    ns.rows = 24;
    ns.callbacks = &cb;
    ns.userData = &failures;
    s = Ssh_Create(&ns);
    check(s != NULL, "Ssh_Create");
    if (s)
    {
        check(s->version == SSH_SESSION_VERSION && s->headSize == sizeof(struct SshSession),
              "session head version and size");
        check(s->userData == &failures, "userData kept");
        check(s->state == SSH_STATE_VERSION, "state starts at VERSION");

        n = Ssh_TakeOutput(s, out, sizeof(out) - 1);
        out[n] = 0;
        check(n >= 8 && memcmp(out, "SSH-2.0-", 8) == 0, "Ssh_TakeOutput starts with SSH-2.0-");

        /* The server ident (record 1) and its KEXINIT (record 2). */
        rec = Record(file, size, 1, &len, &kind);
        check(rec && kind == 'S', "server ident record found");
        if (rec)
            Ssh_Receive(s, rec, len);
        rec = Record(file, size, 2, &len, &kind);
        check(rec && kind == 'S', "server KEXINIT record found");
        if (rec)
            Ssh_Receive(s, rec, len);
        check(s->state == SSH_STATE_KEXREPLY, "state advanced to KEXREPLY");
        check(s->error[0] == 0, "no error");
        n = Ssh_TakeOutput(s, out, sizeof(out));
        check(n > 0, "client KEXDH init queued");

        Ssh_Delete(s);
        check(1, "Ssh_Delete");
    }

    for (i = 0; i < 32; i++)
        fp[i] = (UBYTE)i;
    Ssh_Fingerprint(fp, b64);
    check(strcmp(b64, "SHA256:AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8") == 0,
          "Ssh_Fingerprint of 00..1f");

    CloseLibrary(SshBase);
    {
        struct Library *two = OpenLibrary("ssh.library", 2);

        check(two == NULL, "OpenLibrary(ssh.library, 2) fails");
        if (two)
            CloseLibrary(two);
    }
    FreeVec(file);
    printf("%s %d failure(s)\n", failures ? "[ERROR]" : "[OK]", failures);
    return failures ? 10 : 0;
}
