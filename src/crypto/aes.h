/* src/crypto/aes.h -- AES-128 encryption (FIPS 197) and CTR mode (SP 800-38A):
 * all SSH's aes128-ctr needs (CTR decrypts by encrypting). Pure, unit-tested. */
#ifndef AES_H
#define AES_H

#include <exec/types.h>
#include <string.h>

struct Aes128Ctr
{
    UBYTE roundKeys[176];
    UBYTE counter[16];
    UBYTE stream[16];
    UWORD used;             /* bytes of stream already used (16: none left) */
};

void Aes128_Encrypt(const UBYTE roundKeys[176], const UBYTE in[16], UBYTE out[16]);
void Aes128_ExpandKey(const UBYTE key[16], UBYTE roundKeys[176]);

void Aes128Ctr_Init(struct Aes128Ctr *c, const UBYTE key[16], const UBYTE iv[16]);
/* XORs the key stream into data (encrypts or decrypts, in place). */
void Aes128Ctr_Crypt(struct Aes128Ctr *c, UBYTE *data, size_t len);

#endif /* AES_H */
