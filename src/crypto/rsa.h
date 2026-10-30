/* src/crypto/rsa.h -- RSA PKCS#1 v1.5 signature check with SHA-256: the
 * SSH host key algorithm rsa-sha2-256 (RFC 8332). Pure, unit-tested. */
#ifndef RSA_H
#define RSA_H

#include <exec/types.h>
#include <string.h>

/* TRUE when sig is the signature of the SHA-256 digest hash by the key
 * (n, e), all big-endian bytes as in SSH. */
BOOL Rsa_VerifySha256(const UBYTE *n, size_t nLen, const UBYTE *e, size_t eLen,
                      const UBYTE *sig, size_t sigLen, const UBYTE hash[32]);

#endif /* RSA_H */
