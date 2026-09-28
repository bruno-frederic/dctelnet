/* src/crypto/random.h -- random bytes for SSH's key exchange, from a
 * SHA-256 pool. The Amiga has no random device: DCTelnet feeds the pool
 * whatever varies unpredictably -- the E-clock and beam position at each
 * key, mouse move and network read, the time, free memory -- and SSH draws
 * from it. Pure, unit-tested (the gathering lives in DCTelnet). */
#ifndef RANDOM_H
#define RANDOM_H

#include <exec/types.h>
#include <string.h>

void Random_Add(const void *data, size_t len);
void Random_Bytes(UBYTE *out, size_t len);
ULONG Random_Added(void);
void Random_Reset(void);            /* an empty pool again: only for tests that replay a session */           /* bytes fed so far (enough before a key exchange?) */

#endif /* RANDOM_H */
