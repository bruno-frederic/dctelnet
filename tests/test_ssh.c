/* test/test_ssh.c -- the SSH engine against real servers, replayed.
 *
 * Each file in ssh_sessions/ is a recorded login to a real SSH server (a
 * paramiko test BBS, tools/ssh_test_server.py): every chunk the server sent,
 * and every byte DCTelnet's engine sent. The engine's random numbers come
 * from a pool fed only by what it receives, so a replay makes the same keys
 * and the same packets: the test checks the engine sends exactly the
 * recorded bytes, decrypts the server's, and reads the text through.
 *
 *   test_ssh                              replay every recording
 *   test_ssh record FILE PORT USER PASS   log in to 127.0.0.1:PORT, record FILE
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ssh.h"
#include "crypto/random.h"

static struct Ssh ssh;
static char transcript[80000];
static size_t transcriptLen;
static char fingerprint[64];
static int asked;
static char notes[256];

static void Note(struct Ssh *s, const char *text)
{
    (void)s;
    strcat(notes, text);
    strcat(notes, "|");
}

static BOOL HostKey(struct Ssh *s, const UBYTE *blob, size_t len, const UBYTE fp[32])
{
    (void)s; (void)blob; (void)len;
    Ssh_Fingerprint(fp, fingerprint);
    return TRUE;
}

static BOOL Ask(struct Ssh *s, const char *prompt, BOOL echo, char *out, size_t max)
{
    (void)s; (void)max;
    asked++;
    assert(echo && !strcmp(prompt, "Favourite colour? "));
    strcpy(out, "blue");
    return TRUE;
}

/* Where the server's bytes come from, and where the client's go. */
struct Source
{
    int sock;                       /* recording: the live server */
    FILE *rec;                      /* recording: output; replay: input */
    UBYTE *sent;                    /* replay: the recorded client bytes */
    size_t sentLen, sentAt;
};

static void Put32(FILE *f, unsigned long v)
{
    fputc((int)(v >> 24), f); fputc((int)(v >> 16 & 255), f); fputc((int)(v >> 8 & 255), f); fputc((int)(v & 255), f);
}

static long Get32(FILE *f)
{
    int a = fgetc(f), b = fgetc(f), c = fgetc(f), d = fgetc(f);
    if (d == EOF) return -1;
    return (long)a << 24 | b << 16 | c << 8 | d;
}

/* The client's output: to the socket and into the file, or compared to the file. */
static void Flush(struct Source *src)
{
    UBYTE out[4096];
    ULONG n;
    while ((n = Ssh_TakeOutput(&ssh, out, sizeof(out))) > 0)
    {
        if (src->sock >= 0)
        {
            assert(write(src->sock, out, n) == (ssize_t)n);
            fputc('C', src->rec); Put32(src->rec, n); fwrite(out, 1, n, src->rec);
        }
        else
        {
            assert(src->sentAt + n <= src->sentLen);
            assert(memcmp(src->sent + src->sentAt, out, n) == 0);
            src->sentAt += n;
        }
    }
}

/* The next chunk from the server: 0 at the end. */
static long NextChunk(struct Source *src, UBYTE *buf, size_t max)
{
    if (src->sock >= 0)
    {
        struct pollfd p;
        long n;
        p.fd = src->sock; p.events = POLLIN;
        if (poll(&p, 1, 15000) <= 0) return 0;
        n = read(src->sock, buf, max);
        if (n > 0) { fputc('S', src->rec); Put32(src->rec, (unsigned long)n); fwrite(buf, 1, (size_t)n, src->rec); }
        return n > 0 ? n : 0;
    }
    for (;;)
    {
        int kind = fgetc(src->rec);
        long n = Get32(src->rec);
        if (kind == EOF || n < 0) return 0;
        assert((size_t)n <= max);
        assert(fread(buf, 1, (size_t)n, src->rec) == (size_t)n);
        if (kind == 'S') return n;
    }
}

/* The user: says hello at the first prompt, then resizes to 132x50 and says bye. */
static void Drive(struct Source *src, const char *user, const char *pass)
{
    static UBYTE buf[40000];
    int step = 0;

    Random_Reset();                 /* a recording starts from the empty pool */
    transcriptLen = 0;
    asked = 0;
    fingerprint[0] = 0;
    notes[0] = 0;
    Ssh_Init(&ssh, user, pass, "ansi", 80, 25, FALSE, HostKey, Ask);
    ssh.note = Note;
    for (;;)
    {
        long n;
        ULONG at = 0;
        Flush(src);
        if (ssh.state == SSH_CLOSED) break;
        n = NextChunk(src, buf, sizeof(buf));
        if (n <= 0) break;
        while (at < (ULONG)n && ssh.state != SSH_CLOSED)
        {
            ULONG t;
            at += Ssh_Receive(&ssh, buf + at, (ULONG)n - at);
            t = Ssh_TakeText(&ssh, (UBYTE *)transcript + transcriptLen, (ULONG)(sizeof(transcript) - 1 - transcriptLen));
            transcriptLen += t;
            transcript[transcriptLen] = 0;
            Flush(src);
        }
        if (step == 0 && strstr(transcript, "> "))
        {
            Ssh_Send(&ssh, (const UBYTE *)"hello\r", 6);
            step = 1;
        }
        else if (step == 1 && strstr(transcript, "xxxx\r\n> "))
        {
            Ssh_WindowChange(&ssh, 132, 50);
            Ssh_Send(&ssh, (const UBYTE *)"bye\r", 4);
            step = 2;
        }
    }
    Flush(src);
}

static void Record(const char *file, int port, const char *user, const char *pass)
{
    struct Source src;
    struct sockaddr_in a;

    memset(&src, 0, sizeof(src));
    src.sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = htonl(0x7F000001);
    assert(connect(src.sock, (struct sockaddr *)&a, sizeof(a)) == 0);
    src.rec = fopen(file, "wb");
    assert(src.rec);
    fprintf(src.rec, "%s%c%s%c", user, 0, pass, 0);
    Drive(&src, user, pass);
    fclose(src.rec);
    close(src.sock);
    printf("state %d error \"%s\" kex %s host %s asked %d\n%s\n", ssh.state, ssh.error,
           ssh.kexName ? ssh.kexName : "-", fingerprint, asked, transcript);
}

static void ReadString(FILE *f, char *out)
{
    int c;
    while ((c = fgetc(f)) != EOF && c) *out++ = (char)c;
    *out = 0;
}

/* Replays a recording; returns the kex it used. */
static const char *Replay(const char *file)
{
    struct Source src;
    char user[64], pass[64];
    FILE *f;
    long pos;

    memset(&src, 0, sizeof(src));
    src.sock = -1;
    f = fopen(file, "rb");
    assert(f);
    ReadString(f, user);
    ReadString(f, pass);
    pos = ftell(f);
    /* The client bytes, in order. */
    src.sent = malloc(200000);
    for (;;)
    {
        int kind = fgetc(f);
        long n = Get32(f);
        if (kind == EOF || n < 0) break;
        if (kind == 'C') { assert(fread(src.sent + src.sentLen, 1, (size_t)n, f) == (size_t)n); src.sentLen += (size_t)n; }
        else fseek(f, n, SEEK_CUR);
    }
    fseek(f, pos, SEEK_SET);
    src.rec = f;
    Drive(&src, user, pass);
    fclose(f);
    assert(src.sentAt == src.sentLen);           /* sent every recorded byte, no more */
    free(src.sent);
    return ssh.kexName;
}

/* The shell and the channel: the pty size and name reached the server, the
 * 40000-byte reply came through a 32768-byte window, the resize arrived,
 * and the server's close ends the session cleanly. */
static void CheckSession(void)
{
    assert(strstr(transcript, "Welcome to the test BBS\r\nterm=ansi cols=80 rows=25\r\n> "));
    assert(strstr(transcript, "you said: hello\r\n"));
    assert(strstr(transcript, "window 132x50\r\nbye\r\n"));
    {
        const char *x = strstr(transcript, "you said: hello\r\n") + 17;
        size_t i;
        for (i = 0; i < 40000; i++) assert(x[i] == 'x');
    }
    assert(ssh.state == SSH_CLOSED && ssh.error[0] == 0);
    assert(!strncmp(fingerprint, "SHA256:", 7) && strlen(fingerprint) == 50);
}

static void test_curve25519_password_and_a_rekey(void)
{
    assert(!strcmp(Replay("ssh_sessions/curve25519_password_rekey.bin"), "curve25519-sha256@libssh.org"));
    CheckSession();
    assert(asked == 0);                          /* the stored password was enough */
    assert(ssh.kexCount == 2);                   /* the server rekeyed mid-session */
    /* The slow steps are announced once, not again at the rekey. */
    assert(!strcmp(notes, "Key exchange: curve25519-sha256@libssh.org|Checking the host key: RSA 2048 bits|"));
}

static void test_group14_keyboard_interactive(void)
{
    assert(!strcmp(Replay("ssh_sessions/group14_keyboard_interactive.bin"), "diffie-hellman-group14-sha256"));
    CheckSession();
    assert(asked == 1);                          /* the password prompt was answered, the colour asked */
    assert(ssh.kexCount == 1);
    assert(strstr(transcript, "BBS login\r\nAnswer two questions\r\n"));
}

/* A 68000 offers no group14: against a group14-only server it says why. */
static void test_slow_cpu_refuses_group14_only_servers(void)
{
    static const UBYTE kexinit[] = "SSH-2.0-x\r\n";
    UBYTE pkt[256], *p = pkt;
    const char *lists[10] = { "diffie-hellman-group14-sha256", "rsa-sha2-256", "aes128-ctr", "aes128-ctr",
                              "hmac-sha2-256", "hmac-sha2-256", "none", "none", "", "" };
    size_t len, i, pad;

    Ssh_Init(&ssh, "u", "p", "ansi", 80, 25, TRUE, HostKey, Ask);
    Ssh_Receive(&ssh, kexinit, sizeof(kexinit) - 1);
    p = pkt + 5;
    *p++ = 20;
    memset(p, 0, 16); p += 16;
    for (i = 0; i < 10; i++)
    {
        size_t n = strlen(lists[i]);
        *p++ = 0; *p++ = 0; *p++ = 0; *p++ = (UBYTE)n;
        memcpy(p, lists[i], n); p += n;
    }
    *p++ = 0; memset(p, 0, 4); p += 4;
    len = (size_t)(p - pkt);
    pad = 8 - len % 8; if (pad < 4) pad += 8;
    memset(p, 0, pad); len += pad;
    pkt[0] = 0; pkt[1] = 0; pkt[2] = (UBYTE)((len - 4) >> 8); pkt[3] = (UBYTE)(len - 4); pkt[4] = (UBYTE)pad;
    Ssh_Receive(&ssh, pkt, (ULONG)len);
    assert(ssh.state == SSH_CLOSED);
    assert(strstr(ssh.error, "curve25519"));
    /* The server is told why (SSH_MSG_DISCONNECT, not yet encrypted). */
    assert(!ssh.serverClosed);
    {
        UBYTE out[2048];
        ULONG n = Ssh_TakeOutput(&ssh, out, sizeof(out)), i;
        BOOL told = FALSE;
        for (i = 0; i + 10 < n; i++)
            if (!memcmp(out + i, ssh.error, 10)) told = TRUE;
        assert(told);
    }
}

/* A plain (not encrypted) packet of one message: type, then body. */
static size_t PlainPacket(UBYTE type, const UBYTE *body, size_t bodyLen, UBYTE *pkt)
{
    size_t len = 5 + 1 + bodyLen, pad = 8 - len % 8;

    if (pad < 4) pad += 8;
    pkt[5] = type;
    memcpy(pkt + 6, body, bodyLen);
    memset(pkt + len, 0, pad);
    len += pad;
    pkt[0] = 0; pkt[1] = 0; pkt[2] = (UBYTE)((len - 4) >> 8); pkt[3] = (UBYTE)(len - 4); pkt[4] = (UBYTE)pad;
    return len;
}

/* A server (or a man in the middle) that skips the key exchange and says
 * "logged in" in the clear gets no session: before the first NEWKEYS only
 * the key exchange's own messages count. Else the password would follow
 * unencrypted. */
static void test_nothing_before_the_keys(void)
{
    static const UBYTE version[] = "SSH-2.0-x\r\n";
    static const UBYTE confirm[16] = { 0,0,0,0, 0,0,0,7, 0,1,0,0, 0,0,0x80,0 };
    UBYTE pkt[64];

    Ssh_Init(&ssh, "u", "p", "ansi", 80, 25, FALSE, HostKey, Ask);
    Ssh_Receive(&ssh, version, sizeof(version) - 1);
    Ssh_Receive(&ssh, pkt, (ULONG)PlainPacket(52, confirm, 0, pkt));      /* USERAUTH_SUCCESS */
    Ssh_Receive(&ssh, pkt, (ULONG)PlainPacket(91, confirm, 16, pkt));  /* CHANNEL_OPEN_CONFIRMATION */
    assert(ssh.state == SSH_CLOSED && !ssh.channelOpen);
    assert(strstr(ssh.error, "key exchange"));
}

static void test_fingerprint_is_openssh_format(void)
{
    UBYTE fp[32];
    char out[64];
    size_t i;
    for (i = 0; i < 32; i++) fp[i] = (UBYTE)i;
    Ssh_Fingerprint(fp, out);
    /* base64 of 00 01 .. 1f without the '=': */
    assert(!strcmp(out, "SHA256:AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8"));
}

int main(int argc, char **argv)
{
    if (argc == 6 && !strcmp(argv[1], "record"))
    {
        Record(argv[2], atoi(argv[3]), argv[4], argv[5]);
        return 0;
    }
    test_fingerprint_is_openssh_format();
    test_slow_cpu_refuses_group14_only_servers();
    test_nothing_before_the_keys();
    test_curve25519_password_and_a_rekey();
    test_group14_keyboard_interactive();
    printf("ssh: all assertions passed\n");
    return 0;
}
