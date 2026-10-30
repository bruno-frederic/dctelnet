/* src/crypto/random.c -- random bytes from a SHA-256 pool. */
#include "random.h"
#include "sha256.h"

static UBYTE pool[SHA256_LEN];
static ULONG counter, added;

void Random_Add(const void *data, size_t len)
{
    struct Sha256 s;

    Sha256_Init(&s);
    Sha256_Add(&s, pool, sizeof(pool));
    Sha256_Add(&s, data, len);
    Sha256_Done(&s, pool);
    added += (ULONG)len;
}

/* Each block: SHA-256(pool, counter); the pool then moves on, so an output
 * never reveals a later one. */
void Random_Bytes(UBYTE *out, size_t len)
{
    UBYTE block[SHA256_LEN];

    while (len)
    {
        struct Sha256 s;
        size_t take = len < sizeof(block) ? len : sizeof(block);

        counter++;
        Sha256_Init(&s);
        Sha256_Add(&s, pool, sizeof(pool));
        Sha256_Add(&s, &counter, sizeof(counter));
        Sha256_Done(&s, block);
        memcpy(out, block, take);
        out += take;
        len -= take;
        Random_Add(block, 1);               /* ratchet (counts as one byte) */
        added--;
    }
}

ULONG Random_Added(void)
{
    return added;
}

void Random_Reset(void)
{
    memset(pool, 0, sizeof(pool));
    counter = added = 0;
}
