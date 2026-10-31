/* src/crypto/x25519.h -- X25519 (RFC 7748): SSH's curve25519-sha256 key
 * exchange. The field arithmetic is TweetNaCl's (public domain, Bernstein
 * et al.). Pure, unit-tested. */
#ifndef X25519_H
#define X25519_H

#include <exec/types.h>

/* out = scalar * point (32-byte little-endian u coordinates). */
void X25519(UBYTE out[32], const UBYTE scalar[32], const UBYTE point[32]);
/* out = scalar * 9, the public key of a private scalar. */
void X25519_Base(UBYTE out[32], const UBYTE scalar[32]);

#endif /* X25519_H */
