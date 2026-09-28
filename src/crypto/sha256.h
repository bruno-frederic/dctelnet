/* src/crypto/sha256.h -- SHA-256 (FIPS 180-4) and HMAC-SHA256 (RFC 2104).
 * Pure, unit-tested on the host (test/test_crypto.c). */
#ifndef SHA256_H
#define SHA256_H

#include <exec/types.h>
#include <string.h>

#define SHA256_LEN 32

struct Sha256
{
    ULONG state[8];
    ULONG lenLo, lenHi;         /* message length in bytes */
    UBYTE block[64];
    UWORD used;
};

void Sha256_Init(struct Sha256 *s);
void Sha256_Add(struct Sha256 *s, const void *data, size_t len);
void Sha256_Done(struct Sha256 *s, UBYTE out[SHA256_LEN]);
void Sha256(const void *data, size_t len, UBYTE out[SHA256_LEN]);

struct HmacSha256
{
    struct Sha256 inner, outer;
};

void HmacSha256_Init(struct HmacSha256 *h, const void *key, size_t keyLen);
void HmacSha256_Add(struct HmacSha256 *h, const void *data, size_t len);
void HmacSha256_Done(struct HmacSha256 *h, UBYTE out[SHA256_LEN]);

#endif /* SHA256_H */
