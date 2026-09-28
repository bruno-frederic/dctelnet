/* src/crypto/rsa.c -- RSA PKCS#1 v1.5 signature check with SHA-256. */
#include "rsa.h"
#include "bignum.h"

/* DER DigestInfo prefix for SHA-256 (RFC 8017 9.2 note 1). */
static const UBYTE sha256Info[19] = { 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
                                      0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20 };

BOOL Rsa_VerifySha256(const UBYTE *n, size_t nLen, const UBYTE *e, size_t eLen,
                      const UBYTE *sig, size_t sigLen, const UBYTE hash[32])
{
    static struct BnMod mod;
    static UBYTE s[BN_MAX_BITS / 8 + 2], em[BN_MAX_BITS / 8 + 2];
    size_t k, i;

    if (!Bn_SetMod(&mod, n, nLen))
        return FALSE;
    k = Bn_Bytes(&mod);                             /* bytes of the limb width */
    if (sigLen > k)
        return FALSE;
    Bn_Fit(sig, sigLen, s, k);
    Bn_ModExp(&mod, s, e, eLen, em);
    /* em (k bytes, maybe one leading zero byte more than n): 00 01 FF..FF 00 info hash */
    i = 0;
    while (i < k && em[i] == 0) i++;
    if (i >= k || em[i] != 1) return FALSE;
    for (i++; i < k && em[i] == 0xFF; i++) ;
    if (i >= k || em[i] != 0 || k - i - 1 != sizeof(sha256Info) + 32) return FALSE;
    i++;
    return memcmp(em + i, sha256Info, sizeof(sha256Info)) == 0
        && memcmp(em + i + sizeof(sha256Info), hash, 32) == 0;
}
