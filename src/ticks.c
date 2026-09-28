/* src/ticks.c -- what DCTelnet has to do later, and when. */
#include "ticks.h"
#include <errno.h>      /* the Amiga's (vbcc, BSD values) or the host's */

static int slot(ULONG kind)
{
    int i;

    for (i = 0; i < TICK_KINDS; i++)
        if (kind == (1UL << i))
            return i;
    return -1;
}

void Ticks_Init(struct Ticks *t)
{
    int i;

    for (i = 0; i < TICK_KINDS; i++)
        t->at[i] = 0;
}

void Ticks_Set(struct Ticks *t, ULONG kind, ULONG when)
{
    int i = slot(kind);

    if (i >= 0)
        t->at[i] = when;
}

BOOL Ticks_IsSet(const struct Ticks *t, ULONG kind)
{
    int i = slot(kind);

    return i >= 0 && t->at[i] != 0;
}

ULONG Ticks_Due(struct Ticks *t, ULONG now)
{
    ULONG due = 0;
    int i;

    for (i = 0; i < TICK_KINDS; i++)
        if (t->at[i] && t->at[i] <= now)
        {
            due |= 1UL << i;
            t->at[i] = 0;
        }
    return due;
}

ULONG Ticks_Wait(const struct Ticks *t, ULONG now)
{
    ULONG wait = 0;
    int i;

    for (i = 0; i < TICK_KINDS; i++)
        if (t->at[i])
        {
            ULONG w = t->at[i] > now ? t->at[i] - now : 1;

            if (wait == 0 || w < wait)
                wait = w;
        }
    return wait;
}

BOOL Ticks_ShouldRedial(LONG errnoValue, BOOL timedOut, UBYTE triesLeft)
{
    if (triesLeft == 0)
        return FALSE;
    return timedOut || errnoValue == ECONNREFUSED || errnoValue == ETIMEDOUT
        || errnoValue == EHOSTUNREACH;
}

ULONG Ticks_FromEClock(ULONG hi, ULONG lo, ULONG perTick)
{
    /* hi:lo / perTick, 16 bits at a time: every step fits 32 bits. */
    ULONG r = (hi >> 16) % perTick, q1, q0;

    r = ((r << 16) | (hi & 0xFFFF)) % perTick;
    q1 = ((r << 16) | (lo >> 16)) / perTick;
    r = ((r << 16) | (lo >> 16)) % perTick;
    q0 = ((r << 16) | (lo & 0xFFFF)) / perTick;
    return (q1 << 16) + q0;
}
