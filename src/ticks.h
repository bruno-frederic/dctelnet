/* src/ticks.h -- what DCTelnet has to do later, and when. Pure, unit-tested
 * on the host.
 *
 * Deadlines in any monotonic clock unit (DCTelnet: DOS ticks, 1/50 s,
 * since it started): the login macro's next
 * segment after a \d wait, the next redial after a failed connect, the
 * anti-idle NOP after minutes without a key sent. The main loop sleeps on
 * one timer request for exactly the time to the earliest (Ticks_Wait) and
 * runs what is due (Ticks_Due). Nothing is pending: no timer at all. */
#ifndef TICKS_H
#define TICKS_H

#include <exec/types.h>

#define TICK_MACRO   0x01
#define TICK_REDIAL  0x02
#define TICK_NOP     0x04
#define TICK_KINDS   3

struct Ticks
{
    ULONG at[TICK_KINDS];       /* 0 = not scheduled */
};

void  Ticks_Init(struct Ticks *t);
void  Ticks_Set(struct Ticks *t, ULONG kind, ULONG when);   /* when = 0: cancel */
BOOL  Ticks_IsSet(const struct Ticks *t, ULONG kind);

/* The kinds due at now (a mask), each cleared: a repeating one is set again
 * by its handler. */
ULONG Ticks_Due(struct Ticks *t, ULONG now);

/* Units from now to the earliest deadline, at least 1; 0 when none. */
ULONG Ticks_Wait(const struct Ticks *t, ULONG now);

/* Redial: the connect errors worth trying again (refused, timed out, no
 * route, interrupted by the connect timeout) -- not an unknown host. */
BOOL  Ticks_ShouldRedial(LONG errnoValue, BOOL timedOut, UBYTE triesLeft);

/* The E-clock count hi:lo (timer.device ReadEClock, monotonic) in units of
 * perTick E-clock counts; perTick must be below 65536 (a DOS tick is about
 * 14200). The low 32 bits: they wrap after 2.7 years of 1/50 s. */
ULONG Ticks_FromEClock(ULONG hi, ULONG lo, ULONG perTick);

#endif /* TICKS_H */
