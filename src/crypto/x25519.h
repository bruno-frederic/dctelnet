/* src/crypto/x25519.h -- X25519 (RFC 7748): SSH's curve25519-sha256 key
 * exchange. The field arithmetic is TweetNaCl's (public domain, Bernstein
 * et al.). Pure, unit-tested. */
#ifndef X25519_H
#define X25519_H

#include <exec/types.h>

/* The ladder's temporaries (field elements of 8 limbs), the caller's to
 * place: on the 4 KB default Amiga stack they do not fit as locals. */
struct X25519Work
{
    ULONG x1[8], x2[8], z2[8], x3[8], z3[8], a[8], aa[8], b[8], bb[8], e[8], c[8], d[8], da[8], cb[8];
    ULONG inv[9][8];                /* Invert's chain */
    ULONG t[16];                    /* a product before its reduction */
    UBYTE k[32];
};

/* out = scalar * point (32-byte little-endian u coordinates). */
void X25519(UBYTE out[32], const UBYTE scalar[32], const UBYTE point[32], struct X25519Work *w);
/* out = scalar * 9, the public key of a private scalar. */
void X25519_Base(UBYTE out[32], const UBYTE scalar[32], struct X25519Work *w);

#endif /* X25519_H */
