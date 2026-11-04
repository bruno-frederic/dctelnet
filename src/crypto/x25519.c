/* src/crypto/x25519.c -- X25519 (RFC 7748).
 *
 * Field elements mod p = 2^255 - 19 are 8 limbs of 32 bits, kept below
 * 2^256 but not fully reduced until the end. Every product goes through
 * Bn_MulAdd (assembly on the Amiga): a multiplication is 8 rows of it,
 * and the folding of the top half (2^256 = 38 mod p) one more. */
#include "x25519.h"
#include "bignum.h"

typedef ULONG fe[8];

/* r += 38 * c at limb 0, carried through (c small). Returns the carry out. */
static ULONG AddSmall(fe r, ULONG c)
{
    int i;
    ULONG v = c * 38;
    for (i = 0; i < 8 && v; i++)
    {
        r[i] += v;
        v = r[i] < v;
    }
    return v;
}

static void Add(fe r, const fe a, const fe b)
{
    ULONG c = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        ULONG s = a[i] + c;
        c = s < c;
        r[i] = s + b[i];
        c += r[i] < s;
    }
    /* 2^256 = 38 (mod p): at most twice, the second time without a carry. */
    if (AddSmall(r, c)) AddSmall(r, 1);
}

static void Sub(fe r, const fe a, const fe b)
{
    ULONG borrow = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        ULONG d = a[i] - b[i] - borrow;
        borrow = a[i] < b[i] || (a[i] == b[i] && borrow);
        r[i] = d;
    }
    /* A borrow added 2^256 = 38 too much: take 38 away (at most twice). */
    while (borrow)
    {
        ULONG v = 38;
        borrow = 0;
        for (i = 0; i < 8; i++)
        {
            ULONG d = r[i] - v;
            v = r[i] < v;
            r[i] = d;
            if (!v) break;
        }
        borrow = v;
    }
}

/* t is 16 limbs of scratch: as a local it was inlined into every call site,
 * 17 frames of it on a stack of 4 KB. */
static void Mul(fe r, const fe a, const fe b, ULONG *t)
{
    ULONG c;
    int i;

    memset(t, 0, 16 * sizeof(ULONG));
    for (i = 0; i < 8; i++)
        t[i + 8] = Bn_MulAdd(t + i, a, 8, b[i]);
    c = Bn_MulAdd(t, t + 8, 8, 38);                     /* low + 38 * high */
    while (c) c = AddSmall(t, c);
    memcpy(r, t, sizeof(fe));
}

static void Sq(fe r, const fe a, ULONG *t)
{
    ULONG c;

    Bn_Square(t, a, 8);
    c = Bn_MulAdd(t, t + 8, 8, 38);                     /* low + 38 * high */
    while (c) c = AddSmall(t, c);
    memcpy(r, t, sizeof(fe));
}

static void MulSmall(fe r, const fe a, ULONG w, ULONG *t)
{
    ULONG c;
    memset(t, 0, sizeof(fe));
    c = Bn_MulAdd(t, a, 8, w);
    while (c) c = AddSmall(t, c);
    memcpy(r, t, sizeof(fe));
}

/* a^(p-2) = 1/a, by the usual chain: 254 squarings, 11 multiplications. */
static void Invert(fe out, const fe z, fe v[9], ULONG *u)
{
    ULONG *z2 = v[0], *z9 = v[1], *z11 = v[2], *z5 = v[3], *z10 = v[4], *z20 = v[5], *z50 = v[6], *z100 = v[7], *t = v[8];
    int i;

    Sq(z2, z, u);
    Sq(t, z2, u); Sq(t, t, u);
    Mul(z9, t, z, u);
    Mul(z11, z9, z2, u);
    Sq(t, z11, u);
    Mul(z5, t, z9, u);                                     /* 2^5 - 1 */
    Sq(t, z5, u); for (i = 1; i < 5; i++) Sq(t, t, u);
    Mul(z10, t, z5, u);                                    /* 2^10 - 1 */
    Sq(t, z10, u); for (i = 1; i < 10; i++) Sq(t, t, u);
    Mul(z20, t, z10, u);                                   /* 2^20 - 1 */
    Sq(t, z20, u); for (i = 1; i < 20; i++) Sq(t, t, u);
    Mul(t, t, z20, u);                                     /* 2^40 - 1 */
    for (i = 0; i < 10; i++) Sq(t, t, u);
    Mul(z50, t, z10, u);                                   /* 2^50 - 1 */
    Sq(t, z50, u); for (i = 1; i < 50; i++) Sq(t, t, u);
    Mul(z100, t, z50, u);                                  /* 2^100 - 1 */
    Sq(t, z100, u); for (i = 1; i < 100; i++) Sq(t, t, u);
    Mul(t, t, z100, u);                                    /* 2^200 - 1 */
    for (i = 0; i < 50; i++) Sq(t, t, u);
    Mul(t, t, z50, u);                                     /* 2^250 - 1 */
    for (i = 0; i < 5; i++) Sq(t, t, u);
    Mul(out, t, z11, u);                                   /* 2^255 - 21 = p - 2 */
}

/* Swaps a and b when swap is 1, in constant time. */
static void CSwap(fe a, fe b, ULONG swap)
{
    ULONG mask = 0 - swap;
    int i;
    for (i = 0; i < 8; i++)
    {
        ULONG t = mask & (a[i] ^ b[i]);
        a[i] ^= t;
        b[i] ^= t;
    }
}

static void Decode(fe r, const UBYTE *b)
{
    int i;
    for (i = 0; i < 8; i++)
        r[i] = (ULONG)b[4 * i] | (ULONG)b[4 * i + 1] << 8 | (ULONG)b[4 * i + 2] << 16 | (ULONG)b[4 * i + 3] << 24;
    r[7] &= 0x7FFFFFFFUL;                               /* RFC 7748: the top bit is ignored */
}

/* The unique value below p, as 32 little-endian bytes. */
static void Encode(UBYTE *out, const fe a)
{
    fe r, t;
    int i, k;

    memcpy(r, a, sizeof(fe));
    for (k = 0; k < 2; k++)                             /* fold bit 255: 2^255 = 19 */
    {
        ULONG top = r[7] >> 31, v;
        r[7] &= 0x7FFFFFFFUL;
        for (i = 0, v = 19 * top; i < 8 && v; i++) { r[i] += v; v = r[i] < v; }
    }
    /* r < 2^255 now; r >= p exactly when r + 19 reaches 2^255. */
    memcpy(t, r, sizeof(fe));
    {
        ULONG v = 19;
        for (i = 0; i < 8 && v; i++) { t[i] += v; v = t[i] < v; }
    }
    if (t[7] >> 31) { t[7] &= 0x7FFFFFFFUL; memcpy(r, t, sizeof(fe)); }
    for (i = 0; i < 8; i++)
    {
        out[4 * i] = (UBYTE)r[i]; out[4 * i + 1] = (UBYTE)(r[i] >> 8);
        out[4 * i + 2] = (UBYTE)(r[i] >> 16); out[4 * i + 3] = (UBYTE)(r[i] >> 24);
    }
}

void X25519(UBYTE out[32], const UBYTE scalar[32], const UBYTE point[32], struct X25519Work *w)
{
    UBYTE *k = w->k;
    ULONG *x1 = w->x1, *x2 = w->x2, *z2 = w->z2, *x3 = w->x3, *z3 = w->z3, *a = w->a, *aa = w->aa, *b = w->b;
    ULONG *bb = w->bb, *e = w->e, *c = w->c, *d = w->d, *da = w->da, *cb = w->cb;
    ULONG swap = 0;
    int t;
    memcpy(k, scalar, 32);
    k[0] &= 248;
    k[31] = (UBYTE)((k[31] & 127) | 64);
    Decode(x1, point);
    memset(x2, 0, sizeof(fe)); x2[0] = 1;
    memset(z2, 0, sizeof(fe));
    memcpy(x3, x1, sizeof(fe));
    memset(z3, 0, sizeof(fe)); z3[0] = 1;

    for (t = 254; t >= 0; t--)                          /* the Montgomery ladder, RFC 7748 5 */
    {
        ULONG bit = (k[t >> 3] >> (t & 7)) & 1;
        swap ^= bit;
        CSwap(x2, x3, swap);
        CSwap(z2, z3, swap);
        swap = bit;

        Add(a, x2, z2); Sq(aa, a, w->t);
        Sub(b, x2, z2); Sq(bb, b, w->t);
        Sub(e, aa, bb);
        Add(c, x3, z3);
        Sub(d, x3, z3);
        Mul(da, d, a, w->t);
        Mul(cb, c, b, w->t);
        Add(x3, da, cb); Sq(x3, x3, w->t);
        Sub(z3, da, cb); Sq(z3, z3, w->t); Mul(z3, z3, x1, w->t);
        Mul(x2, aa, bb, w->t);
        MulSmall(z2, e, 121665, w->t); Add(z2, z2, aa); Mul(z2, z2, e, w->t);
    }
    CSwap(x2, x3, swap);
    CSwap(z2, z3, swap);
    Invert(z2, z2, w->inv, w->t);
    Mul(x2, x2, z2, w->t);
    Encode(out, x2);
}

void X25519_Base(UBYTE out[32], const UBYTE scalar[32], struct X25519Work *w)
{
    static const UBYTE nine[32] = { 9 };

    X25519(out, scalar, nine, w);
}
