/* src/crypto/bignum.h -- modular exponentiation for SSH: RSA signatures and
 * Diffie-Hellman group 14, by Montgomery multiplication.
 *
 * Numbers are arrays of 32-bit limbs, least significant first. All the
 * multiplying is done by one primitive, Bn_MulAdd; on the Amiga it is
 * 68000 or 68020 assembly (bn_muladd.s), picked at run time, since vbcc
 * turns every 64-bit product in C into a library call. */
#ifndef BIGNUM_H
#define BIGNUM_H

#include <exec/types.h>
#include <string.h>

#define BN_MAX_BITS  4096
#define BN_MAX_LIMBS (BN_MAX_BITS / 32 + 1)

typedef ULONG bn_limb;

struct BnMod                  /* a modulus, ready for Montgomery arithmetic */
{
    bn_limb m[BN_MAX_LIMBS];
    bn_limb rr[BN_MAX_LIMBS]; /* R^2 mod m, R = 2^(32 n) */
    UWORD   n;                /* limbs */
    bn_limb minv;             /* -m^-1 mod 2^32 */
};

/* r[0..n) += a[0..n) * w; returns the limb carried out (n >= 1). On the
 * Amiga a call through a pointer straight into the assembly for this CPU
 * (set on the first call): a C wrapper cost more than a limb product. */
#ifdef BN_ASM
typedef bn_limb (*BnMulAddFn)(__reg("a0") bn_limb *r, __reg("a1") const bn_limb *a,
                              __reg("d0") UWORD n, __reg("d1") bn_limb w);
extern BnMulAddFn Bn_MulAddFn;
#define Bn_MulAdd(r, a, n, w) Bn_MulAddFn((r), (a), (n), (w))
#else
bn_limb Bn_MulAdd(bn_limb *r, const bn_limb *a, UWORD n, bn_limb w);
#endif

/* t[0..2n) = x[0..n)^2: each cross product once, doubled, then the
 * squares on the diagonal -- about half the products of x * x. */
void Bn_Square(bn_limb *t, const bn_limb *x, UWORD n);

/* The modulus from its big-endian bytes (odd, at most BN_MAX_BITS). FALSE
 * for an even or too long one. */
BOOL Bn_SetMod(struct BnMod *mod, const UBYTE *bytes, size_t len);

/* The byte width of numbers modulo mod: n * 4. */
#define Bn_Bytes(mod) ((size_t)(mod)->n * 4)

/* out = base^exp mod m. base and out are big-endian, Bn_Bytes(mod) long
 * (base must be smaller than m); exp is big-endian of any length. */
void Bn_ModExp(const struct BnMod *mod, const UBYTE *base, const UBYTE *exp, size_t expLen, UBYTE *out);

/* Big-endian bytes (any length) padded or cut to width bytes. */
void Bn_Fit(const UBYTE *in, size_t len, UBYTE *out, size_t width);

#endif /* BIGNUM_H */
