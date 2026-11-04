/* src/crypto/sha256.c -- SHA-256 (FIPS 180-4) and HMAC-SHA256 (RFC 2104). */
#include "sha256.h"

static const ULONG K[64] =
{
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void block(struct Sha256 *s, const UBYTE *p)
{
    ULONG w[64], a, b, c, d, e, f, g, h, t1, t2;
    int i;

    for (i = 0; i < 16; i++)
        w[i] = ((ULONG)p[i * 4] << 24) | ((ULONG)p[i * 4 + 1] << 16) | ((ULONG)p[i * 4 + 2] << 8) | p[i * 4 + 3];
    for (i = 16; i < 64; i++)
    {
        ULONG s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        ULONG s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);

        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = s->state[0]; b = s->state[1]; c = s->state[2]; d = s->state[3];
    e = s->state[4]; f = s->state[5]; g = s->state[6]; h = s->state[7];
    for (i = 0; i < 64; i++)
    {
        t1 = h + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
        t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    s->state[0] += a; s->state[1] += b; s->state[2] += c; s->state[3] += d;
    s->state[4] += e; s->state[5] += f; s->state[6] += g; s->state[7] += h;
}

void Sha256_Init(struct Sha256 *s)
{
    static const ULONG iv[8] = { 0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL, 0xa54ff53aUL,
                                 0x510e527fUL, 0x9b05688cUL, 0x1f83d9abUL, 0x5be0cd19UL };

    memcpy(s->state, iv, sizeof(iv));
    s->lenLo = s->lenHi = 0;
    s->used = 0;
}

void Sha256_Add(struct Sha256 *s, const void *data, size_t len)
{
    const UBYTE *p = (const UBYTE *)data;

    if (s->lenLo + len < s->lenLo) s->lenHi++;
    s->lenLo += (ULONG)len;
    while (len)
    {
        size_t take = 64 - s->used;

        if (take > len) take = len;
        memcpy(s->block + s->used, p, take);
        s->used = (UWORD)(s->used + take);
        p += take;
        len -= take;
        if (s->used == 64)
        {
            block(s, s->block);
            s->used = 0;
        }
    }
}

void Sha256_Done(struct Sha256 *s, UBYTE out[SHA256_LEN])
{
    ULONG hi = (s->lenHi << 3) | (s->lenLo >> 29), lo = s->lenLo << 3;
    UBYTE pad = 0x80, len[8];
    int i;

    Sha256_Add(s, &pad, 1);
    pad = 0;
    while (s->used != 56)
        Sha256_Add(s, &pad, 1);
    for (i = 0; i < 4; i++) { len[i] = (UBYTE)(hi >> (24 - 8 * i)); len[4 + i] = (UBYTE)(lo >> (24 - 8 * i)); }
    Sha256_Add(s, len, 8);
    for (i = 0; i < 8; i++)
    {
        out[i * 4]     = (UBYTE)(s->state[i] >> 24);
        out[i * 4 + 1] = (UBYTE)(s->state[i] >> 16);
        out[i * 4 + 2] = (UBYTE)(s->state[i] >> 8);
        out[i * 4 + 3] = (UBYTE)s->state[i];
    }
}

void Sha256(const void *data, size_t len, UBYTE out[SHA256_LEN])
{
    struct Sha256 s;

    Sha256_Init(&s);
    Sha256_Add(&s, data, len);
    Sha256_Done(&s, out);
}

void HmacSha256_Init(struct HmacSha256 *h, const void *key, size_t keyLen)
{
    UBYTE k[64], pad[64];
    int i;

    memset(k, 0, sizeof(k));
    if (keyLen > 64) Sha256(key, keyLen, k);
    else memcpy(k, key, keyLen);
    for (i = 0; i < 64; i++) pad[i] = (UBYTE)(k[i] ^ 0x36);
    Sha256_Init(&h->inner);
    Sha256_Add(&h->inner, pad, 64);
    for (i = 0; i < 64; i++) pad[i] = (UBYTE)(k[i] ^ 0x5c);
    Sha256_Init(&h->outer);
    Sha256_Add(&h->outer, pad, 64);
}

void HmacSha256_Add(struct HmacSha256 *h, const void *data, size_t len)
{
    Sha256_Add(&h->inner, data, len);
}

void HmacSha256_Done(struct HmacSha256 *h, UBYTE out[SHA256_LEN])
{
    UBYTE inner[SHA256_LEN];

    Sha256_Done(&h->inner, inner);
    Sha256_Add(&h->outer, inner, sizeof(inner));
    Sha256_Done(&h->outer, out);
}
