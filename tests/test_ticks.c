/* test/test_ticks.c -- what DCTelnet has to do later, and when. */
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include "ticks.h"

static void test_nothing_pending_means_no_timer(void)
{
    struct Ticks t;

    Ticks_Init(&t);
    assert(Ticks_Wait(&t, 100) == 0 && Ticks_Due(&t, 100) == 0);
}

static void test_waits_for_the_earliest_and_runs_what_is_due(void)
{
    struct Ticks t;

    Ticks_Init(&t);
    Ticks_Set(&t, TICK_NOP, 400);
    Ticks_Set(&t, TICK_MACRO, 101);
    assert(Ticks_Wait(&t, 100) == 1);
    assert(Ticks_Due(&t, 100) == 0);
    assert(Ticks_Due(&t, 101) == TICK_MACRO);
    assert(!Ticks_IsSet(&t, TICK_MACRO) && Ticks_IsSet(&t, TICK_NOP));
    assert(Ticks_Wait(&t, 101) == 299);
    Ticks_Set(&t, TICK_REDIAL, 90);                 /* overdue: wait at least 1 */
    assert(Ticks_Wait(&t, 101) == 1);
    assert(Ticks_Due(&t, 500) == (TICK_REDIAL | TICK_NOP));
    assert(Ticks_Wait(&t, 500) == 0);
}

static void test_cancel(void)
{
    struct Ticks t;

    Ticks_Init(&t);
    Ticks_Set(&t, TICK_REDIAL, 10);
    Ticks_Set(&t, TICK_REDIAL, 0);
    assert(Ticks_Wait(&t, 1) == 0);
}

/* Redial after refused, timed out, unreachable -- not after an unknown
 * host (it will not appear) and not when the tries are used up. */
static void test_which_failures_redial(void)
{
    assert(Ticks_ShouldRedial(ECONNREFUSED, FALSE, 3));
    assert(Ticks_ShouldRedial(ETIMEDOUT, FALSE, 1));
    assert(Ticks_ShouldRedial(EHOSTUNREACH, FALSE, 1));
    assert(Ticks_ShouldRedial(EINTR, TRUE, 1));        /* our connect timeout */
    assert(!Ticks_ShouldRedial(EINTR, FALSE, 1));      /* the user's Abort */
    assert(!Ticks_ShouldRedial(ECONNREFUSED, FALSE, 0));
}

/* The timer's clock is the E-clock (monotonic; setting the time or a
 * daylight-saving change moved the DateStamp clock the jobs were on). */
static void test_eclock_counts_become_ticks(void)
{
    assert(Ticks_FromEClock(0, 14187, 14187) == 1);
    assert(Ticks_FromEClock(0, 14186, 14187) == 0);
    assert(Ticks_FromEClock(1, 0, 16) == 0x10000000UL);                 /* 2^32 / 16 */
    assert(Ticks_FromEClock(2, 0x9ABCDEF0UL, 14187) == 788468UL);      /* 0x29ABCDEF0 / 14187 */
    assert(Ticks_FromEClock(14187, 0, 14187) == 0);                    /* 2^32 ticks: wraps */
}

int main(void)
{
    test_nothing_pending_means_no_timer();
    test_waits_for_the_earliest_and_runs_what_is_due();
    test_cancel();
    test_which_failures_redial();
    test_eclock_counts_become_ticks();
    printf("ticks: all assertions passed\n");
    return 0;
}
