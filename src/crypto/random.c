/* src/crypto/random.c -- random bytes from a SHA-256 pool. */
#include "random.h"
#include "sha256.h"

/* One pool for every user of the engine, by design: the entropy of all the
 * sessions goes into it. In ssh.library several tasks reach it at once, so
 * Random_Add / Random_Bytes / Random_Reset run under the library's semaphore
 * (SSH_LIBRARY: Random_Lock / Random_Unlock, in sshlib/ssh_lib.c; it is
 * recursive, Random_Bytes calls Random_Add). Host builds have one task and
 * no lock. The check-statics target lists these three as the only allowed
 * statics. */
#ifdef SSH_LIBRARY
void Random_Lock(void);
void Random_Unlock(void);
#else
#define Random_Lock()
#define Random_Unlock()
#endif
static UBYTE pool[SHA256_LEN];
static ULONG counter, added;

void Random_Add(const void *data, size_t len)
{
    struct Sha256 s;

    Random_Lock();
    Sha256_Init(&s);
    Sha256_Add(&s, pool, sizeof(pool));
    Sha256_Add(&s, data, len);
    Sha256_Done(&s, pool);
    added += (ULONG)len;
    Random_Unlock();
}

/* Each block: SHA-256(pool, counter); the pool then moves on, so an output
 * never reveals a later one. */
void Random_Bytes(UBYTE *out, size_t len)
{
    UBYTE block[SHA256_LEN];

    Random_Lock();
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
    Random_Unlock();
}

ULONG Random_Added(void)
{
    return added;
}

void Random_Reset(void)
{
    Random_Lock();
    memset(pool, 0, sizeof(pool));
    counter = added = 0;
    Random_Unlock();
}
