/* src/crypto/rsa.h -- RSA PKCS#1 v1.5 signature check with SHA-256: the
 * SSH host key algorithm rsa-sha2-256 (RFC 8332). Pure, unit-tested. */
#ifndef RSA_H
#define RSA_H

#include <exec/types.h>
#include <string.h>
#include "bignum.h"

/* The working space of Rsa_VerifySha256, owned by the caller with a
 * BnScratch (which it may share with a Diffie-Hellman exchange that is over). */
struct RsaWork
{
    struct BnMod mod;
    UBYTE s[BN_MAX_BITS / 8 + 2], em[BN_MAX_BITS / 8 + 2];
};

/* TRUE when sig is the signature of the SHA-256 digest hash by the key
 * (n, e), all big-endian bytes as in SSH. */
BOOL Rsa_VerifySha256(const UBYTE *n, size_t nLen, const UBYTE *e, size_t eLen,
                      const UBYTE *sig, size_t sigLen, const UBYTE hash[32], struct RsaWork *w, struct BnScratch *bn);

#endif /* RSA_H */
