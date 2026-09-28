/* src/ssh.c -- the SSH-2 client state machine (see ssh.h). */
#include "ssh.h"
#include "crypto/bignum.h"
#include "crypto/rsa.h"
#include "crypto/x25519.h"
#include "crypto/random.h"

enum
{
    MSG_DISCONNECT = 1, MSG_IGNORE, MSG_UNIMPLEMENTED, MSG_DEBUG, MSG_SERVICE_REQUEST, MSG_SERVICE_ACCEPT,
    MSG_KEXINIT = 20, MSG_NEWKEYS, MSG_KEX_INIT = 30, MSG_KEX_REPLY,
    MSG_USERAUTH_REQUEST = 50, MSG_USERAUTH_FAILURE, MSG_USERAUTH_SUCCESS, MSG_USERAUTH_BANNER,
    MSG_USERAUTH_INFO_REQUEST = 60, MSG_USERAUTH_INFO_RESPONSE,
    MSG_GLOBAL_REQUEST = 80, MSG_REQUEST_SUCCESS, MSG_REQUEST_FAILURE,
    MSG_CHANNEL_OPEN = 90, MSG_CHANNEL_OPEN_CONFIRMATION, MSG_CHANNEL_OPEN_FAILURE,
    MSG_CHANNEL_WINDOW_ADJUST, MSG_CHANNEL_DATA, MSG_CHANNEL_EXTENDED_DATA, MSG_CHANNEL_EOF,
    MSG_CHANNEL_CLOSE, MSG_CHANNEL_REQUEST, MSG_CHANNEL_SUCCESS, MSG_CHANNEL_FAILURE
};

#define VERSION "SSH-2.0-DCTelnet_2.0"
#define KEX_CURVE   "curve25519-sha256"
#define KEX_CURVE_L "curve25519-sha256@libssh.org"
#define KEX_GROUP14 "diffie-hellman-group14-sha256"
#define HOSTKEY     "rsa-sha2-256"
#define CIPHER      "aes128-ctr"
#define MAC         "hmac-sha2-256"

/* RFC 3526 group 14: the 2048-bit MODP prime, generator 2. */
static const UBYTE group14[256] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
    0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
    0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
    0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
    0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
    0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D, 0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
    0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
    0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D, 0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
    0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
    0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
    0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9, 0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
    0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
    0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAC, 0xAA, 0x68, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/* ---- reading a message ---- */

struct Reader
{
    const UBYTE *p;
    ULONG left;
    BOOL bad;                       /* ran past the end */
};

static ULONG GetU32(struct Reader *r)
{
    ULONG v;
    if (r->left < 4) { r->bad = TRUE; r->left = 0; return 0; }
    v = (ULONG)r->p[0] << 24 | (ULONG)r->p[1] << 16 | (ULONG)r->p[2] << 8 | r->p[3];
    r->p += 4; r->left -= 4;
    return v;
}

static UBYTE GetByte(struct Reader *r)
{
    if (!r->left) { r->bad = TRUE; return 0; }
    r->left--;
    return *r->p++;
}

/* A string: its bytes stay in the message; *len gets its length. */
static const UBYTE *GetString(struct Reader *r, ULONG *len)
{
    const UBYTE *p;
    ULONG n = GetU32(r);
    if (n > r->left) { r->bad = TRUE; r->left = 0; *len = 0; return (const UBYTE *)""; }
    p = r->p; r->p += n; r->left -= n; *len = n;
    return p;
}

/* A string copied as C text (cut to max - 1). */
static void GetText(struct Reader *r, char *out, ULONG max)
{
    ULONG n;
    const UBYTE *p = GetString(r, &n);
    if (n > max - 1) n = max - 1;
    memcpy(out, p, n);
    out[n] = 0;
}

/* ---- building a message (in s->msg) ---- */

static void Put(struct Ssh *s, const void *data, ULONG len)
{
    if (s->msgLen + len > SSH_MSG) len = SSH_MSG - s->msgLen;
    memcpy(s->msg + s->msgLen, data, len);
    s->msgLen += len;
}

static void PutByte(struct Ssh *s, UBYTE b) { Put(s, &b, 1); }

static void PutU32(struct Ssh *s, ULONG v)
{
    UBYTE b[4];
    b[0] = (UBYTE)(v >> 24); b[1] = (UBYTE)(v >> 16); b[2] = (UBYTE)(v >> 8); b[3] = (UBYTE)v;
    Put(s, b, 4);
}

static void PutString(struct Ssh *s, const void *data, ULONG len) { PutU32(s, len); Put(s, data, len); }
static void PutText(struct Ssh *s, const char *text) { PutString(s, text, strlen(text)); }
static void Start(struct Ssh *s, UBYTE type) { s->msgLen = 0; PutByte(s, type); }

/* An unsigned big-endian number as an mpint (RFC 4251 5): no leading zero
 * bytes, one zero byte in front when the top bit is set. Returns the bytes
 * written to out (4 + value). */
static ULONG Mpint(const UBYTE *v, ULONG len, UBYTE *out)
{
    ULONG n;
    while (len && !*v) { v++; len--; }
    n = len + (len && (*v & 0x80) ? 1 : 0);
    out[0] = (UBYTE)(n >> 24); out[1] = (UBYTE)(n >> 16); out[2] = (UBYTE)(n >> 8); out[3] = (UBYTE)n;
    if (n > len) out[4] = 0;
    memcpy(out + 4 + (n - len), v, len);
    return 4 + n;
}

static void HashString(struct Sha256 *h, const void *data, ULONG len)
{
    UBYTE b[4];
    b[0] = (UBYTE)(len >> 24); b[1] = (UBYTE)(len >> 16); b[2] = (UBYTE)(len >> 8); b[3] = (UBYTE)len;
    Sha256_Add(h, b, 4);
    Sha256_Add(h, data, len);
}

/* v in decimal at out. */
static void Decimal(char *out, ULONG v)
{
    char d[12];
    int n = 0;
    do { d[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *out++ = d[--n];
    *out = 0;
}

/* ---- errors ---- */

static void SendMsg(struct Ssh *s);

/* DCTelnet ends the connection: the server is told why (RFC 4253 11.1),
 * once the version lines are exchanged. */
static void Fail(struct Ssh *s, const char *why)
{
    if (s->state == SSH_CLOSED || s->failing) return;
    strncpy(s->error, why, sizeof(s->error) - 1);
    s->error[sizeof(s->error) - 1] = 0;
    if (s->state != SSH_VERSION)
    {
        s->failing = TRUE;
        s->msgLen = 0;
        PutByte(s, MSG_DISCONNECT);
        PutU32(s, 2);                       /* SSH_DISCONNECT_PROTOCOL_ERROR */
        PutText(s, why);
        PutText(s, "");
        SendMsg(s);
    }
    s->state = SSH_CLOSED;
}

/* ---- packets ---- */

static void Emit(struct Ssh *s, const void *data, ULONG len)
{
    if (s->txLen + len > SSH_BUF) { Fail(s, "SSH: output buffer full"); return; }
    memcpy(s->tx + s->txLen, data, len);
    s->txLen += len;
}

/* Sends s->msg as one binary packet (RFC 4253 6): length, padding length,
 * payload, random padding to a multiple of 16, then the MAC of the plain
 * packet, and the packet encrypted. */
static void SendMsg(struct Ssh *s)
{
    ULONG pad = 16 - (5 + s->msgLen) % 16, total;
    UBYTE *p;

    if (s->state == SSH_CLOSED) return;
    if (pad < 4) pad += 16;
    total = 5 + s->msgLen + pad;
    if (s->txLen + total + 32 > SSH_BUF) { Fail(s, "SSH: output buffer full"); return; }
    p = s->tx + s->txLen;
    p[0] = (UBYTE)((total - 4) >> 24); p[1] = (UBYTE)((total - 4) >> 16);
    p[2] = (UBYTE)((total - 4) >> 8); p[3] = (UBYTE)(total - 4);
    p[4] = (UBYTE)pad;
    memcpy(p + 5, s->msg, s->msgLen);
    Random_Bytes(p + 5 + s->msgLen, pad);
    if (s->out.on)
    {
        struct HmacSha256 h;
        UBYTE seq[4];
        seq[0] = (UBYTE)(s->out.seq >> 24); seq[1] = (UBYTE)(s->out.seq >> 16);
        seq[2] = (UBYTE)(s->out.seq >> 8); seq[3] = (UBYTE)s->out.seq;
        HmacSha256_Init(&h, s->out.mac, 32);
        HmacSha256_Add(&h, seq, 4);
        HmacSha256_Add(&h, p, total);
        HmacSha256_Done(&h, p + total);
        Aes128Ctr_Crypt(&s->out.aes, p, total);
        total += 32;
    }
    s->txLen += total;
    s->out.seq++;
}

/* ---- key exchange ---- */

static void SendKexInit(struct Ssh *s)
{
    UBYTE cookie[16];
    Start(s, MSG_KEXINIT);
    Random_Bytes(cookie, 16);
    Put(s, cookie, 16);
    PutText(s, s->slowCpu ? KEX_CURVE "," KEX_CURVE_L : KEX_CURVE "," KEX_CURVE_L "," KEX_GROUP14);
    PutText(s, HOSTKEY);
    PutText(s, CIPHER); PutText(s, CIPHER);
    PutText(s, MAC); PutText(s, MAC);
    PutText(s, "none"); PutText(s, "none");
    PutText(s, ""); PutText(s, "");
    PutByte(s, 0);
    PutU32(s, 0);
    s->icLen = (UWORD)s->msgLen;
    memcpy(s->ic, s->msg, s->msgLen);
    SendMsg(s);
}

/* The first name of the client's list that the server's list holds. */
static const char *Choose(const char *client, const UBYTE *server, ULONG serverLen)
{
    static char pick[64];
    const char *c = client;
    while (*c)
    {
        const char *end = c;
        ULONG n, i;
        while (*end && *end != ',') end++;
        n = (ULONG)(end - c);
        for (i = 0; i < serverLen; )
        {
            ULONG j = i;
            while (j < serverLen && server[j] != ',') j++;
            if (j - i == n && !memcmp(server + i, c, n) && n < sizeof(pick))
            {
                memcpy(pick, c, n); pick[n] = 0;
                return pick;
            }
            i = j + 1;
        }
        c = *end ? end + 1 : end;
    }
    return NULL;
}

static BOOL FirstIs(const UBYTE *list, ULONG len, const char *name)
{
    ULONG n = strlen(name);
    return len >= n && !memcmp(list, name, n) && (len == n || list[n] == ',');
}

static void OnKexInit(struct Ssh *s, const UBYTE *payload, ULONG len)
{
    struct Reader r;
    const UBYTE *kex, *hostKey, *list;
    ULONG kexLen, hostKeyLen, n;
    const char *pick;
    int i;

    if (len > sizeof(s->is)) { Fail(s, "SSH: server KEXINIT too long"); return; }
    memcpy(s->is, payload, len);
    s->isLen = (UWORD)len;
    r.p = payload + 17; r.left = len > 17 ? len - 17 : 0; r.bad = len <= 17;
    kex = GetString(&r, &kexLen);
    hostKey = GetString(&r, &hostKeyLen);
    pick = Choose(s->slowCpu ? KEX_CURVE "," KEX_CURVE_L : KEX_CURVE "," KEX_CURVE_L "," KEX_GROUP14, kex, kexLen);
    if (!pick)
    {
        Fail(s, s->slowCpu ? "SSH: the server offers no curve25519 key exchange (the only one fast enough on a 68000)"
                           : "SSH: the server offers no key exchange DCTelnet knows (curve25519, group14)");
        return;
    }
    s->kexName = !strcmp(pick, KEX_GROUP14) ? KEX_GROUP14 : !strcmp(pick, KEX_CURVE) ? KEX_CURVE : KEX_CURVE_L;
    s->kex = !strcmp(pick, KEX_GROUP14);
    if (!Choose(HOSTKEY, hostKey, hostKeyLen)) { Fail(s, "SSH: the server has no RSA host key (rsa-sha2-256)"); return; }
    for (i = 0; i < 4; i++)
    {
        list = GetString(&r, &n);
        if (!Choose(i < 2 ? CIPHER : MAC, list, n))
        {
            Fail(s, i < 2 ? "SSH: the server does not offer aes128-ctr" : "SSH: the server does not offer hmac-sha2-256");
            return;
        }
    }
    for (i = 0; i < 2; i++)
    {
        list = GetString(&r, &n);
        if (!Choose("none", list, n)) { Fail(s, "SSH: the server insists on compression"); return; }
    }
    GetString(&r, &n); GetString(&r, &n);
    /* A guessed kex packet follows; it is ours to drop when the guess is wrong. */
    if (GetByte(&r) && !(FirstIs(kex, kexLen, s->kexName) && FirstIs(hostKey, hostKeyLen, HOSTKEY)))
        s->ignoreNext = TRUE;
    if (r.bad) { Fail(s, "SSH: bad KEXINIT from the server"); return; }

    if (!s->rekeying) SendKexInit(s);       /* the server started this key exchange */
    s->rekeying = TRUE;
    s->resume = s->haveSession ? s->state : SSH_SERVICE;

    if (s->note && !s->haveSession)
    {
        static char text[80];
        strcpy(text, "Key exchange: ");
        strcat(text, s->kexName);
        s->note(s, text);
    }
    Start(s, MSG_KEX_INIT);
    if (!s->kex)
    {
        Random_Bytes(s->xPriv, 32);
        X25519_Base(s->xPub, s->xPriv);
        PutString(s, s->xPub, 32);
    }
    else
    {
        static struct BnMod mod;
        static UBYTE two[256];
        UBYTE mp[4 + 257];
        Bn_SetMod(&mod, group14, 256);
        memset(two, 0, 256); two[255] = 2;
        Random_Bytes(s->dhX, 32);
        s->dhX[0] &= 0x7F;
        Bn_ModExp(&mod, two, s->dhX, 32, s->dhE);
        Put(s, mp, Mpint(s->dhE, 256, mp));
    }
    SendMsg(s);
    s->state = SSH_KEXREPLY;
}

static void Derive(struct Ssh *s, char letter, UBYTE out[32])
{
    struct Sha256 h;
    Sha256_Init(&h);
    Sha256_Add(&h, s->K, s->kLen);
    Sha256_Add(&h, s->H, 32);
    Sha256_Add(&h, &letter, 1);
    Sha256_Add(&h, s->sessionId, 32);
    Sha256_Done(&h, out);
}

static void OnKexReply(struct Ssh *s, const UBYTE *payload, ULONG len)
{
    struct Reader r, k;
    const UBYTE *ks, *f, *sigBlob, *name, *e, *n, *sig;
    ULONG ksLen, fLen, sigBlobLen, nameLen, eLen, nLen, sigLen;
    UBYTE fp[32], iv[32], key[32];
    struct Sha256 h;

    r.p = payload + 1; r.left = len - 1; r.bad = FALSE;
    ks = GetString(&r, &ksLen);
    f = GetString(&r, &fLen);
    sigBlob = GetString(&r, &sigBlobLen);
    if (r.bad) { Fail(s, "SSH: bad key exchange reply"); return; }

    /* The shared secret K. */
    if (!s->kex)
    {
        UBYTE secret[32], any = 0;
        int i;
        if (fLen != 32) { Fail(s, "SSH: bad curve25519 key from the server"); return; }
        X25519(secret, s->xPriv, f);
        for (i = 0; i < 32; i++) any |= secret[i];
        if (!any) { Fail(s, "SSH: the server's curve25519 key is weak"); return; }
        s->kLen = (UWORD)Mpint(secret, 32, s->K);
    }
    else
    {
        static struct BnMod mod;
        static UBYTE fb[256], secret[256];
        UBYTE pm1[256];
        int i;
        Bn_SetMod(&mod, group14, 256);
        while (fLen && !*f) { f++; fLen--; }
        memcpy(pm1, group14, 256); pm1[255]--;        /* p - 1 (p ends in FF) */
        Bn_Fit(f, fLen, fb, 256);
        if (fLen > 256 || memcmp(fb, pm1, 256) >= 0) { Fail(s, "SSH: bad group14 key from the server"); return; }
        for (i = 0; i < 255 && !fb[i]; i++) ;
        if (i == 255 && fb[255] <= 1) { Fail(s, "SSH: bad group14 key from the server"); return; }
        Bn_ModExp(&mod, fb, s->dhX, 32, secret);
        s->kLen = (UWORD)Mpint(secret, 256, s->K);
    }

    /* The exchange hash H. */
    Sha256_Init(&h);
    HashString(&h, s->vc, strlen(s->vc));
    HashString(&h, s->vs, strlen(s->vs));
    HashString(&h, s->ic, s->icLen);
    HashString(&h, s->is, s->isLen);
    HashString(&h, ks, ksLen);
    if (!s->kex)
    {
        HashString(&h, s->xPub, 32);
        HashString(&h, f, fLen);
    }
    else
    {
        UBYTE mp[4 + 257];
        Sha256_Add(&h, mp, Mpint(s->dhE, 256, mp));
        Sha256_Add(&h, mp, Mpint(f, fLen, mp));
    }
    Sha256_Add(&h, s->K, s->kLen);
    Sha256_Done(&h, s->H);
    if (!s->haveSession) memcpy(s->sessionId, s->H, 32);

    /* The host key: ssh-rsa e n, and its signature of H. */
    k.p = ks; k.left = ksLen; k.bad = FALSE;
    name = GetString(&k, &nameLen);
    e = GetString(&k, &eLen);
    n = GetString(&k, &nLen);
    if (k.bad || nameLen != 7 || memcmp(name, "ssh-rsa", 7)) { Fail(s, "SSH: the host key is not an RSA key"); return; }
    while (nLen && !*n) { n++; nLen--; }
    k.p = sigBlob; k.left = sigBlobLen; k.bad = FALSE;
    name = GetString(&k, &nameLen);
    sig = GetString(&k, &sigLen);
    if (k.bad || nameLen != strlen(HOSTKEY) || memcmp(name, HOSTKEY, nameLen))
        { Fail(s, "SSH: the host key signature is not rsa-sha2-256"); return; }
    if (s->note && !s->haveSession)
    {
        static char text[64];
        ULONG bits = nLen * 8, top = n[0];
        while (bits && !(top & 0x80)) { bits--; top <<= 1; }
        strcpy(text, "Checking the host key: RSA ");
        Decimal(text + strlen(text), bits);
        strcat(text, " bits");
        s->note(s, text);
    }
    Sha256(s->H, 32, fp);
    if (!Rsa_VerifySha256(n, nLen, e, eLen, sig, sigLen, fp))
        { Fail(s, "SSH: the host key signature is wrong -- this is not the server it claims to be"); return; }

    Sha256(ks, ksLen, fp);
    if (!s->haveSession)
    {
        if (s->hostKey && !s->hostKey(s, ks, ksLen, fp)) { Fail(s, "SSH: host key not accepted"); return; }
        memcpy(s->hostFp, fp, 32);
    }
    else if (memcmp(s->hostFp, fp, 32))
        { Fail(s, "SSH: the host key changed during the session"); return; }
    s->haveSession = TRUE;

    /* The keys: client to server A (IV), C (key), E (MAC); server to client B, D, F. */
    memset(&s->nextOut, 0, sizeof(s->nextOut));
    memset(&s->nextIn, 0, sizeof(s->nextIn));
    Derive(s, 'A', iv); Derive(s, 'C', key); Aes128Ctr_Init(&s->nextOut.aes, key, iv);
    Derive(s, 'E', s->nextOut.mac);
    Derive(s, 'B', iv); Derive(s, 'D', key); Aes128Ctr_Init(&s->nextIn.aes, key, iv);
    Derive(s, 'F', s->nextIn.mac);
    s->nextOut.on = s->nextIn.on = TRUE;
    memset(key, 0, sizeof(key));
    memset(s->xPriv, 0, 32);
    memset(s->dhX, 0, 32);

    Start(s, MSG_NEWKEYS);
    SendMsg(s);
    s->nextOut.seq = s->out.seq;
    s->out = s->nextOut;
    s->state = SSH_NEWKEYS;
}

/* ---- after the key exchange ---- */

static void FlushPending(struct Ssh *s);

static void SendUserAuth(struct Ssh *s, const char *method)
{
    Start(s, MSG_USERAUTH_REQUEST);
    PutText(s, s->user);
    PutText(s, "ssh-connection");
    PutText(s, method);
    if (!strcmp(method, "password"))
    {
        PutByte(s, 0);
        PutText(s, s->pass);
    }
    else if (!strcmp(method, "keyboard-interactive"))
    {
        PutText(s, "");
        PutText(s, "");
    }
    SendMsg(s);
}

static void OnNewKeys(struct Ssh *s)
{
    s->nextIn.seq = s->in.seq;
    s->in = s->nextIn;
    s->rekeying = FALSE;
    s->kexCount++;
    s->state = s->resume;
    if (s->state == SSH_SERVICE && !s->serviceAsked)
    {
        Start(s, MSG_SERVICE_REQUEST);
        PutText(s, "ssh-userauth");
        SendMsg(s);
        s->serviceAsked = TRUE;
    }
    FlushPending(s);
}

/* Terminal text: LF alone becomes CR LF (banners and prompts are Unix text). */
static void AddText(struct Ssh *s, const UBYTE *p, ULONG len, BOOL fixNewlines)
{
    ULONG i;
    for (i = 0; i < len && s->textLen < SSH_BUF - 1; i++)
    {
        if (fixNewlines && p[i] == '\n' && (i == 0 || p[i - 1] != '\r'))
            s->text[s->textLen++] = '\r';
        s->text[s->textLen++] = p[i];
    }
}

static void TryNextAuth(struct Ssh *s, const UBYTE *methods, ULONG len)
{
    if (!s->authPassTried && Choose("password", methods, len))
    {
        s->authPassTried = TRUE;
        if (!s->pass[0] && !(s->ask && s->ask(s, "Password:", FALSE, s->pass, sizeof(s->pass))))
            { Fail(s, "SSH: login cancelled"); return; }
        SendUserAuth(s, "password");
        return;
    }
    if (!s->authKiTried && Choose("keyboard-interactive", methods, len))
    {
        s->authKiTried = TRUE;
        SendUserAuth(s, "keyboard-interactive");
        return;
    }
    Fail(s, s->authPassTried || s->authKiTried ? "SSH: login failed (wrong user name or password?)"
                                               : "SSH: the server wants a login method DCTelnet lacks (public key)");
}

static void OnInfoRequest(struct Ssh *s, const UBYTE *payload, ULONG len)
{
    struct Reader r;
    char text[256], answer[64];
    ULONG prompts, i, n;
    const UBYTE *p;

    r.p = payload + 1; r.left = len - 1; r.bad = FALSE;
    p = GetString(&r, &n); AddText(s, p, n, TRUE); if (n) AddText(s, (const UBYTE *)"\n", 1, TRUE);
    p = GetString(&r, &n); AddText(s, p, n, TRUE); if (n) AddText(s, (const UBYTE *)"\n", 1, TRUE);
    GetString(&r, &n);
    prompts = GetU32(&r);
    if (r.bad || prompts > 16) { Fail(s, "SSH: bad keyboard-interactive request"); return; }
    Start(s, MSG_USERAUTH_INFO_RESPONSE);
    PutU32(s, prompts);
    for (i = 0; i < prompts; i++)
    {
        UBYTE echo;
        GetText(&r, text, sizeof(text));
        echo = GetByte(&r);
        /* A password prompt gets the stored password once; the rest are asked. */
        if (!echo && s->pass[0] && !s->kiPassUsed)
        {
            strcpy(answer, s->pass);
            s->kiPassUsed = TRUE;
        }
        else if (!s->ask || !s->ask(s, text, echo, answer, sizeof(answer)))
            { Fail(s, "SSH: login cancelled"); return; }
        PutText(s, answer);
    }
    memset(answer, 0, sizeof(answer));
    if (r.bad) { Fail(s, "SSH: bad keyboard-interactive request"); return; }
    SendMsg(s);
}

static void OpenChannel(struct Ssh *s)
{
    Start(s, MSG_CHANNEL_OPEN);
    PutText(s, "session");
    PutU32(s, 0);
    PutU32(s, SSH_WINDOW);
    PutU32(s, SSH_CHUNK);
    SendMsg(s);
    s->localWindow = SSH_WINDOW;
    s->state = SSH_CHANNEL;
}

static void StartShell(struct Ssh *s)
{
    Start(s, MSG_CHANNEL_REQUEST);
    PutU32(s, s->remoteChan);
    PutText(s, "pty-req");
    PutByte(s, 0);
    PutText(s, s->term);
    PutU32(s, s->cols); PutU32(s, s->rows);
    PutU32(s, 0); PutU32(s, 0);
    PutString(s, "", 1);                /* no terminal modes: TTY_OP_END */
    SendMsg(s);
    Start(s, MSG_CHANNEL_REQUEST);
    PutU32(s, s->remoteChan);
    PutText(s, "shell");
    PutByte(s, 0);
    SendMsg(s);
}

/* Channel data from pend, as far as the server's window allows. */
static void FlushPending(struct Ssh *s)
{
    while (s->pendLen && s->state == SSH_OPEN && !s->rekeying && s->remoteWindow)
    {
        ULONG n = s->pendLen;
        if (n > s->remoteWindow) n = s->remoteWindow;
        if (n > s->remoteMax) n = s->remoteMax;
        if (n > SSH_CHUNK) n = SSH_CHUNK;
        Start(s, MSG_CHANNEL_DATA);
        PutU32(s, s->remoteChan);
        PutString(s, s->pend, n);
        SendMsg(s);
        s->remoteWindow -= n;
        memmove(s->pend, s->pend + n, s->pendLen - n);
        s->pendLen -= (UWORD)n;
    }
}

/* Whether a message of the user authentication or connection protocol
 * (type 50 and up) fits where the session is: authentication replies while
 * logging in, the channel's answer once it was asked for, its traffic once
 * it is open. A key exchange keeps the state it returns to. */
static BOOL InPhase(const struct Ssh *s, UBYTE type)
{
    int at = s->rekeying ? s->resume : s->state;

    if (type < MSG_GLOBAL_REQUEST)
        return at == SSH_AUTH;
    if (type < MSG_CHANNEL_OPEN)
        return TRUE;                        /* global requests: any time after the keys */
    if (type == MSG_CHANNEL_OPEN_CONFIRMATION || type == MSG_CHANNEL_OPEN_FAILURE)
        return at == SSH_CHANNEL;
    return at == SSH_OPEN;
}

static void OnMessage(struct Ssh *s, const UBYTE *payload, ULONG len)
{
    struct Reader r;
    const UBYTE *p;
    ULONG n;
    UBYTE type;

    if (!len) { Fail(s, "SSH: empty message"); return; }
    type = payload[0];
    r.p = payload + 1; r.left = len - 1; r.bad = FALSE;
    if (s->ignoreNext && type >= 30 && type <= 49) { s->ignoreNext = FALSE; return; }
    /* Until the first NEWKEYS nothing is encrypted or authenticated: only the
     * key exchange's own messages count (RFC 4253 7.1). A "logged in" or a
     * channel sooner is a man in the middle -- the password would follow in
     * the clear. */
    if ((type >= MSG_USERAUTH_REQUEST || type == MSG_SERVICE_ACCEPT) && s->kexCount == 0)
        { Fail(s, "SSH: a message before the key exchange ended"); return; }
    if (type >= MSG_USERAUTH_REQUEST && !InPhase(s, type))
        { Fail(s, "SSH: a message out of order"); return; }
    switch (type)
    {
    case MSG_DISCONNECT:
        GetU32(&r);
        p = GetString(&r, &n);
        {
            static const char said[] = "SSH: the server closed: ";
            if (n > sizeof(s->error) - sizeof(said)) n = sizeof(s->error) - sizeof(said);
            strcpy(s->error, said);
            memcpy(s->error + sizeof(said) - 1, p, n);
            s->error[sizeof(said) - 1 + n] = 0;
        }
        s->serverClosed = TRUE;
        s->state = SSH_CLOSED;
        return;
    case MSG_IGNORE: case MSG_DEBUG: case MSG_UNIMPLEMENTED:
        return;
    case MSG_KEXINIT:
        OnKexInit(s, payload, len);
        return;
    case MSG_KEX_REPLY:
        if (s->state != SSH_KEXREPLY) break;
        OnKexReply(s, payload, len);
        return;
    case MSG_NEWKEYS:
        if (s->state != SSH_NEWKEYS) break;
        OnNewKeys(s);
        return;
    case MSG_SERVICE_ACCEPT:
        if (!s->serviceAsked || (s->rekeying ? s->resume : s->state) != SSH_SERVICE)
            { Fail(s, "SSH: a message out of order"); return; }
        /* No user name yet: asked now, not before connecting -- servers
         * close a connection that sends no version line for 15 s or so. */
        if (!s->user[0] && !(s->ask && s->ask(s, "User name:", TRUE, s->user, sizeof(s->user)) && s->user[0]))
            { Fail(s, "SSH: no user name, no login"); return; }
        SendUserAuth(s, "none");
        s->state = SSH_AUTH;
        return;
    case MSG_USERAUTH_BANNER:
        p = GetString(&r, &n);
        AddText(s, p, n, TRUE);
        return;
    case MSG_USERAUTH_FAILURE:
        p = GetString(&r, &n);
        TryNextAuth(s, p, n);
        return;
    case MSG_USERAUTH_SUCCESS:
        memset(s->pass, 0, sizeof(s->pass));
        OpenChannel(s);
        return;
    case MSG_USERAUTH_INFO_REQUEST:
        OnInfoRequest(s, payload, len);
        return;
    case MSG_GLOBAL_REQUEST:
        GetString(&r, &n);
        if (GetByte(&r)) { Start(s, MSG_REQUEST_FAILURE); SendMsg(s); }
        return;
    case MSG_CHANNEL_OPEN_CONFIRMATION:
        GetU32(&r);
        s->remoteChan = GetU32(&r);
        s->remoteWindow = GetU32(&r);
        s->remoteMax = GetU32(&r);
        if (!s->remoteMax) s->remoteMax = 1;
        s->channelOpen = TRUE;
        s->state = SSH_OPEN;
        StartShell(s);
        FlushPending(s);
        return;
    case MSG_CHANNEL_OPEN_FAILURE:
        Fail(s, "SSH: the server refused a terminal session");
        return;
    case MSG_CHANNEL_WINDOW_ADJUST:
        GetU32(&r);
        n = GetU32(&r);
        s->remoteWindow = s->remoteWindow + n < s->remoteWindow ? 0xFFFFFFFFUL : s->remoteWindow + n;
        FlushPending(s);
        return;
    case MSG_CHANNEL_DATA:
    case MSG_CHANNEL_EXTENDED_DATA:
        GetU32(&r);
        if (type == MSG_CHANNEL_EXTENDED_DATA) GetU32(&r);
        p = GetString(&r, &n);
        if (r.bad || n > s->localWindow) { Fail(s, "SSH: the server overran the channel window"); return; }
        s->localWindow -= n;
        AddText(s, p, n, FALSE);
        return;
    case MSG_CHANNEL_EOF:
        return;
    case MSG_CHANNEL_CLOSE:
        if (s->channelOpen)
        {
            Start(s, MSG_CHANNEL_CLOSE);
            PutU32(s, s->remoteChan);
            SendMsg(s);
            s->channelOpen = FALSE;
        }
        s->error[0] = 0;
        s->serverClosed = TRUE;
        s->state = SSH_CLOSED;
        return;
    case MSG_CHANNEL_REQUEST:
        GetU32(&r);
        GetString(&r, &n);
        if (GetByte(&r))
        {
            Start(s, MSG_CHANNEL_FAILURE);
            PutU32(s, s->remoteChan);
            SendMsg(s);
        }
        return;
    case MSG_REQUEST_SUCCESS: case MSG_REQUEST_FAILURE:
    case MSG_CHANNEL_SUCCESS: case MSG_CHANNEL_FAILURE:
        return;
    }
    Start(s, MSG_UNIMPLEMENTED);
    PutU32(s, s->in.seq - 1);
    SendMsg(s);
}

/* ---- the socket side ---- */

/* The server's version line (RFC 4253 4.2); lines before it are ignored. */
static BOOL ReadVersion(struct Ssh *s)
{
    for (;;)
    {
        UBYTE *nl = memchr(s->rx, '\n', s->rxLen);
        ULONG n;
        if (!nl)
        {
            if (s->rxLen > 255) Fail(s, "SSH: this is not an SSH server");
            return FALSE;
        }
        n = (ULONG)(nl - s->rx);
        if (n >= 4 && !memcmp(s->rx, "SSH-", 4))
        {
            ULONG v = n && s->rx[n - 1] == '\r' ? n - 1 : n;
            if (v >= sizeof(s->vs)) v = sizeof(s->vs) - 1;
            memcpy(s->vs, s->rx, v);
            s->vs[v] = 0;
            memmove(s->rx, nl + 1, s->rxLen - n - 1);
            s->rxLen -= n + 1;
            if (memcmp(s->vs, "SSH-2.0-", 8) && memcmp(s->vs, "SSH-1.99-", 9))
                { Fail(s, "SSH: the server speaks only SSH 1"); return FALSE; }
            s->state = SSH_KEXINIT;
            return TRUE;
        }
        memmove(s->rx, nl + 1, s->rxLen - n - 1);
        s->rxLen -= n + 1;
    }
}

/* One whole packet from rx, when it is there: decrypted, MAC checked, handled. */
static BOOL ReadPacket(struct Ssh *s)
{
    ULONG macLen = s->in.on ? 32 : 0, total;
    UBYTE *p = s->rx;

    if (!s->rxPacket)
    {
        if (s->rxLen < 16) return FALSE;
        if (s->in.on) Aes128Ctr_Crypt(&s->in.aes, p, 16);
        s->rxPacket = (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | (ULONG)p[2] << 8 | p[3];
        if (s->rxPacket < 12 || s->rxPacket > SSH_BUF - 4 - 32 || (s->rxPacket + 4) % (s->in.on ? 16 : 8))
            { Fail(s, "SSH: corrupt packet (bad length)"); return FALSE; }
    }
    total = 4 + s->rxPacket;
    if (s->rxLen < total + macLen) return FALSE;
    if (s->in.on)
    {
        struct HmacSha256 h;
        UBYTE seq[4], mac[32];
        Aes128Ctr_Crypt(&s->in.aes, p + 16, total - 16);
        seq[0] = (UBYTE)(s->in.seq >> 24); seq[1] = (UBYTE)(s->in.seq >> 16);
        seq[2] = (UBYTE)(s->in.seq >> 8); seq[3] = (UBYTE)s->in.seq;
        HmacSha256_Init(&h, s->in.mac, 32);
        HmacSha256_Add(&h, seq, 4);
        HmacSha256_Add(&h, p, total);
        HmacSha256_Done(&h, mac);
        if (memcmp(mac, p + total, 32)) { Fail(s, "SSH: corrupt packet (bad MAC)"); return FALSE; }
    }
    s->in.seq++;
    if (p[4] + 5 > total) { Fail(s, "SSH: corrupt packet (bad padding)"); return FALSE; }
    OnMessage(s, p + 5, total - 5 - p[4]);
    memmove(s->rx, s->rx + total + macLen, s->rxLen - total - macLen);
    s->rxLen -= total + macLen;
    s->rxPacket = 0;
    return s->state != SSH_CLOSED;
}

ULONG Ssh_Receive(struct Ssh *s, const UBYTE *data, ULONG len)
{
    ULONG took = 0;
    while (s->state != SSH_CLOSED)
    {
        ULONG n = len - took;
        if (n > SSH_BUF - s->rxLen) n = SSH_BUF - s->rxLen;
        memcpy(s->rx + s->rxLen, data + took, n);
        s->rxLen += n;
        took += n;
        Random_Add(data, n < 16 ? n : 16);
        if (s->state == SSH_VERSION && !ReadVersion(s)) { if (took == len) break; else continue; }
        while (s->state != SSH_CLOSED && s->state != SSH_VERSION && ReadPacket(s)) ;
        if (took == len || s->rxLen == SSH_BUF) break;
    }
    return took;
}

void Ssh_Send(struct Ssh *s, const UBYTE *data, ULONG len)
{
    if (s->state == SSH_CLOSED) return;
    if (len > sizeof(s->pend) - s->pendLen) len = sizeof(s->pend) - s->pendLen;
    memcpy(s->pend + s->pendLen, data, len);
    s->pendLen += (UWORD)len;
    FlushPending(s);
}

void Ssh_WindowChange(struct Ssh *s, UWORD cols, UWORD rows)
{
    s->cols = cols;
    s->rows = rows;
    if (s->state != SSH_OPEN || s->rekeying) return;
    Start(s, MSG_CHANNEL_REQUEST);
    PutU32(s, s->remoteChan);
    PutText(s, "window-change");
    PutByte(s, 0);
    PutU32(s, cols); PutU32(s, rows);
    PutU32(s, 0); PutU32(s, 0);
    SendMsg(s);
}

void Ssh_Disconnect(struct Ssh *s)
{
    if (s->state == SSH_CLOSED || s->state == SSH_VERSION) { s->state = SSH_CLOSED; return; }
    Start(s, MSG_DISCONNECT);
    PutU32(s, 11);                      /* SSH_DISCONNECT_BY_APPLICATION */
    PutText(s, "bye");
    PutText(s, "");
    SendMsg(s);
    s->error[0] = 0;
    s->state = SSH_CLOSED;
}

void Ssh_KeepAlive(struct Ssh *s)
{
    if (s->state != SSH_OPEN || s->rekeying) return;
    Start(s, MSG_IGNORE);
    PutText(s, "");
    SendMsg(s);
}

ULONG Ssh_TakeOutput(struct Ssh *s, UBYTE *out, ULONG max)
{
    ULONG n = s->txLen < max ? s->txLen : max;
    memcpy(out, s->tx, n);
    memmove(s->tx, s->tx + n, s->txLen - n);
    s->txLen -= n;
    return n;
}

ULONG Ssh_TakeText(struct Ssh *s, UBYTE *out, ULONG max)
{
    ULONG n = s->textLen < max ? s->textLen : max;
    memcpy(out, s->text, n);
    memmove(s->text, s->text + n, s->textLen - n);
    s->textLen -= n;
    /* Open the window again once half of it is used (not per byte). */
    if (s->state == SSH_OPEN && !s->rekeying && s->localWindow < SSH_WINDOW / 2 && s->textLen == 0)
    {
        Start(s, MSG_CHANNEL_WINDOW_ADJUST);
        PutU32(s, s->remoteChan);
        PutU32(s, SSH_WINDOW - s->localWindow);
        SendMsg(s);
        s->localWindow = SSH_WINDOW;
    }
    return n;
}

void Ssh_Init(struct Ssh *s, const char *user, const char *pass, const char *term,
              UWORD cols, UWORD rows, BOOL slowCpu, SshHostKeyFn hostKey, SshAskFn ask)
{
    memset(s, 0, sizeof(*s));
    strncpy(s->user, user, sizeof(s->user) - 1);
    strncpy(s->pass, pass ? pass : "", sizeof(s->pass) - 1);
    strncpy(s->term, term, sizeof(s->term) - 1);
    s->cols = cols;
    s->rows = rows;
    s->slowCpu = slowCpu;
    s->hostKey = hostKey;
    s->ask = ask;
    s->state = SSH_VERSION;
    strcpy(s->vc, VERSION);
    Emit(s, VERSION "\r\n", sizeof(VERSION) + 1);
    s->rekeying = TRUE;
    SendKexInit(s);
}

void Ssh_Fingerprint(const UBYTE fp[32], char *out)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i, o = 7;
    strcpy(out, "SHA256:");
    for (i = 0; i < 32; i += 3)
    {
        ULONG v = (ULONG)fp[i] << 16 | (i + 1 < 32 ? (ULONG)fp[i + 1] << 8 : 0) | (i + 2 < 32 ? fp[i + 2] : 0);
        out[o++] = b64[v >> 18 & 63];
        out[o++] = b64[v >> 12 & 63];
        if (i + 1 < 32) out[o++] = b64[v >> 6 & 63];
        if (i + 2 < 32) out[o++] = b64[v & 63];
    }
    out[o] = 0;                         /* OpenSSH leaves the "=" off */
}
