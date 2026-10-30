/* test/test_crypto.c -- the SSH client's crypto against published vectors. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "crypto/sha256.h"
#include "crypto/aes.h"
#include "crypto/bignum.h"
#include "crypto/rsa.h"
#include "crypto/x25519.h"
#include "crypto/random.h"
#include "crypto_vectors.h"
#include "bignum_vectors.h"

static void hex(const char *h, UBYTE *out, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
    {
        unsigned v;
        sscanf(h + 2 * i, "%2x", &v);
        out[i] = (UBYTE)v;
    }
}

static int same(const UBYTE *got, const char *want)
{
    static UBYTE w[1024];
    size_t n = strlen(want) / 2;
    hex(want, w, n);
    return memcmp(got, w, n) == 0;
}

/* FIPS 180-4 examples, and a message across the 56-byte padding edge. */
static void test_sha256(void)
{
    UBYTE d[32];
    static char million[1000000];
    struct Sha256 s;
    int i;

    Sha256("abc", 3, d);
    assert(same(d, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    Sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, d);
    assert(same(d, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
    Sha256("", 0, d);
    assert(same(d, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    memset(million, 'a', sizeof(million));
    Sha256_Init(&s);
    for (i = 0; i < 10; i++) Sha256_Add(&s, million + i * 100000, 100000);   /* in pieces */
    Sha256_Done(&s, d);
    assert(same(d, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
}

/* RFC 4231 test cases 1, 2 and 6 (a key longer than a block). */
static void test_hmac_sha256(void)
{
    struct HmacSha256 h;
    UBYTE key[131], d[32];

    memset(key, 0x0b, 20);
    HmacSha256_Init(&h, key, 20);
    HmacSha256_Add(&h, "Hi There", 8);
    HmacSha256_Done(&h, d);
    assert(same(d, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"));
    HmacSha256_Init(&h, "Jefe", 4);
    HmacSha256_Add(&h, "what do ya want for nothing?", 28);
    HmacSha256_Done(&h, d);
    assert(same(d, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
    memset(key, 0xaa, 131);
    HmacSha256_Init(&h, key, 131);
    HmacSha256_Add(&h, "Test Using Larger Than Block-Size Key - Hash Key First", 54);
    HmacSha256_Done(&h, d);
    assert(same(d, "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"));
}

/* FIPS 197 C.1, SP 800-38A F.5.1 (CTR-AES128), and CTR across calls. */
static void test_aes128_ctr(void)
{
    UBYTE key[16], in[16], out[16], rk[176], iv[16], data[64], again[64];
    struct Aes128Ctr c;

    hex("000102030405060708090a0b0c0d0e0f", key, 16);
    hex("00112233445566778899aabbccddeeff", in, 16);
    Aes128_ExpandKey(key, rk);
    Aes128_Encrypt(rk, in, out);
    assert(same(out, "69c4e0d86a7b0430d8cdb78070b4c55a"));

    hex("2b7e151628aed2a6abf7158809cf4f3c", key, 16);
    hex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", iv, 16);
    hex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710", data, 64);
    memcpy(again, data, 64);
    Aes128Ctr_Init(&c, key, iv);
    Aes128Ctr_Crypt(&c, data, 64);
    assert(same(data, "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff"
                      "5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee"));
    Aes128Ctr_Init(&c, key, iv);                    /* in odd pieces: the same */
    Aes128Ctr_Crypt(&c, again, 5);
    Aes128Ctr_Crypt(&c, again + 5, 30);
    Aes128Ctr_Crypt(&c, again + 35, 29);
    assert(memcmp(again, data, 64) == 0);
}

/* Diffie-Hellman group14 as SSH does it: e = 2^x, K = f^x (Python pow). */
static void test_group14_modexp(void)
{
    static struct BnMod mod;
    static UBYTE p[256], x[64], two[256], f[256], out[256];

    hex(group14P, p, 256);
    hex(dhX, x, 64);
    hex(dhF, f, 256);
    assert(Bn_SetMod(&mod, p, 256) && mod.n == 64);
    memset(two, 0, sizeof(two));
    two[255] = 2;
    Bn_ModExp(&mod, two, x, 64, out);
    assert(same(out, dhE));
    Bn_ModExp(&mod, f, x, 64, out);
    assert(same(out, dhK));
}

/* RFC 7748 5.2: k = u = 9, then k, u = X25519(k, u), k -- once and 1000
 * times. Catches a wrong reduction that one vector can miss. */
#ifndef X25519_ITERATIONS
#define X25519_ITERATIONS 1000      /* 1 when run on a real (slow) Amiga */
#endif
static void test_x25519_iterated(void)
{
    UBYTE k[32], u[32], r[32];
    int i;

    memset(k, 0, 32); k[0] = 9;
    memcpy(u, k, 32);
    for (i = 1; i <= X25519_ITERATIONS; i++)
    {
        X25519(r, k, u);
        memcpy(u, k, 32);
        memcpy(k, r, 32);
        if (i == 1) assert(same(k, "422c8e7a6227d7bca1350b3e2bb7279f7897b87bb6854b783c60e80311ae3079"));
    }
    if (X25519_ITERATIONS == 1000)
        assert(same(k, "684cf59ba83309552800ef566f2f4d3c1c3887c49360e3875f2eb94d99532c51"));
}

/* The one multiply primitive (asm on the Amiga): r += a * w with the carry
 * out, at the largest multiplier. */
static void test_muladd(void)
{
    ULONG r[5];
    memcpy(r, mulAddR, sizeof(r));
    assert(Bn_MulAdd(r, mulAddA, 5, 0xFFFFFFFFUL) == mulAddWant[5]);
    assert(memcmp(r, mulAddWant, sizeof(r)) == 0);
}

/* The squaring (half the products, doubled, plus the diagonal) equals
 * the plain product, also with every bit set, where each carry is largest. */
static void test_square_matches_multiply(void)
{
    ULONG x[9], sq[18], mul[18];
    UWORD n, i, pass;

    for (pass = 0; pass < 2; pass++)
        for (n = 1; n <= 9; n++)
        {
            for (i = 0; i < n; i++) x[i] = pass ? 0xFFFFFFFFUL : 0x9E3779B9UL * (i + 1) ^ 0x7F4A7C15UL;
            Bn_Square(sq, x, n);
            memset(mul, 0, sizeof(mul));
            for (i = 0; i < n; i++) mul[i + n] = Bn_MulAdd(mul + i, x, n, x[i]);
            assert(memcmp(sq, mul, 2 * n * sizeof(ULONG)) == 0);
        }
}

/* Moduli whose length is no multiple of 32 bits (R^2 mod m starts from a
 * partial top limb), up to 4096 bits. */
static void test_modexp_sizes(void)
{
    static struct BnMod mod;
    static UBYTE m[512], base[512], exp[40], out[512], want[512];
    size_t c;

    for (c = 0; c < sizeof(modexpCases) / sizeof(modexpCases[0]); c++)
    {
        size_t mLen = strlen(modexpCases[c].m) / 2, bLen = strlen(modexpCases[c].base) / 2;
        size_t wLen = strlen(modexpCases[c].want) / 2;
        static UBYTE b[512];
        hex(modexpCases[c].m, m, mLen);
        hex(modexpCases[c].base, b, bLen);
        hex(modexpCases[c].exp, exp, 40);
        hex(modexpCases[c].want, want, wLen);
        assert(Bn_SetMod(&mod, m, mLen));
        Bn_Fit(b, bLen, base, Bn_Bytes(&mod));
        Bn_ModExp(&mod, base, exp, 40, out);
        Bn_Fit(want, wLen, b, Bn_Bytes(&mod));
        assert(memcmp(out, b, Bn_Bytes(&mod)) == 0);
    }
}

/* An RSA-4096 signature (the host key size of bbs.uprough.net). */
static void test_rsa4096_signature(void)
{
    static UBYTE n[512], sig[512], hash[32];
    static const UBYTE e[3] = { 1, 0, 1 };

    hex(rsaN4, n, 512);
    hex(rsaSig4, sig, 512);
    Sha256(rsaMsg, sizeof(rsaMsg) - 1, hash);
    assert(Rsa_VerifySha256(n, 512, e, 3, sig, 512, hash));
    sig[511] ^= 1;
    assert(!Rsa_VerifySha256(n, 512, e, 3, sig, 512, hash));
}

/* An RSA-2048 signature made with openssl: it checks; one bit off does not. */
static void test_rsa_sha256_signature(void)
{
    static UBYTE n[256], sig[256], hash[32];
    static const UBYTE e[3] = { 1, 0, 1 };

    hex(rsaN, n, 256);
    hex(rsaSig, sig, 256);
    Sha256(rsaMsg, strlen(rsaMsg), hash);
    assert(Rsa_VerifySha256(n, 256, e, 3, sig, 256, hash));
    hash[0] ^= 1;
    assert(!Rsa_VerifySha256(n, 256, e, 3, sig, 256, hash));
    hash[0] ^= 1;
    sig[100] ^= 0x10;
    assert(!Rsa_VerifySha256(n, 256, e, 3, sig, 256, hash));
}

/* RFC 7748 5.2 (one scalar mult) and 6.1 (Alice and Bob agree). */
static void test_x25519(void)
{
    UBYTE k[32], u[32], out[32], alice[32], bob[32], alicePub[32], bobPub[32], s1[32], s2[32];

    hex("a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4", k, 32);
    hex("e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c", u, 32);
    X25519(out, k, u);
    assert(same(out, "c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552"));
    hex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a", alice, 32);
    hex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb", bob, 32);
    X25519_Base(alicePub, alice);
    X25519_Base(bobPub, bob);
    assert(same(alicePub, "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"));
    assert(same(bobPub, "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f"));
    X25519(s1, alice, bobPub);
    X25519(s2, bob, alicePub);
    assert(same(s1, "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742"));
    assert(memcmp(s1, s2, 32) == 0);
}

/* Two draws never repeat; what was fed changes what comes out. */
static void test_random_pool(void)
{
    UBYTE a[40], b[40];

    Random_Add("seed", 4);
    Random_Bytes(a, sizeof(a));
    Random_Bytes(b, sizeof(b));
    assert(memcmp(a, b, sizeof(a)) != 0 && memcmp(a, a + 20, 20) != 0);
    assert(Random_Added() == 4);
}

int main(void)
{
    test_random_pool();
    test_x25519();
    test_x25519_iterated();
    test_muladd();
    test_square_matches_multiply();
    test_modexp_sizes();
    test_group14_modexp();
    test_rsa4096_signature();
    test_rsa_sha256_signature();
    test_aes128_ctr();
    test_sha256();
    test_hmac_sha256();
    printf("crypto: all assertions passed\n");
    return 0;
}
