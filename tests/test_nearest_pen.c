/* tests/test_nearest_pen.c -- ObtainNearestPen on a screen whose pens are
 * all held exclusively but the shared Intuition pens 0-3.
 *
 * Measured on an A1200 (16 colours, full screen): tool bar icons showed only
 * grey, blue and black; authored red came out black, cyan blue. ObtainBestPen()
 * without OBP_FailIfBad answers with the closest SHAREABLE pen however poor,
 * so the scan over every pen the screen holds never ran. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/graphics.h>
#include "nearest_pen.h"

#define SHAREABLE 4 /* pens 0-3: Intuition's, shared; 4-15 are exclusive */

static const ULONG screen[16] = {
    0xBCBCBC, 0x000000, 0xFFFFFF, 0x5577AA,                         /* Intuition */
    0x000000, 0xDD0000, 0x00DD00, 0xDDDD00, 0x0000DD, 0xDD00DD,     /* ANSI */
    0x00DDDD, 0xDDDDDD, 0x555555, 0xFF5555, 0x55FF55, 0xFFFF55,
};
static int bestPenCalls;

static ULONG spread(ULONG c) { return c * 0x01010101UL; }

static int channelOff(ULONG a, ULONG b, int shift)
{
    int d = (int)((a >> shift) & 0xFF) - (int)((b >> shift) & 0xFF);
    return d < 0 ? -d : d;
}

/* graphics.library's ObtainBestPen() on that screen: no free pen, so only
 * pens 0-3 can be shared. Within the precision (16 per channel and step) it
 * answers with the closest of them; outside it, -1 with OBP_FailIfBad and
 * the closest anyway without. */
LONG ObtainBestPen(struct ColorMap *cm, ULONG r, ULONG g, ULONG b, ...)
{
    va_list ap;
    ULONG tag, precision = 0;
    int failIfBad = 0, i, best = -1, bestOff = 0x7FFFFFFF;
    ULONG want = ((r >> 24) << 16) | ((g >> 24) << 8) | (b >> 24);

    (void)cm;
    bestPenCalls++;
    va_start(ap, b);
    while ((tag = va_arg(ap, ULONG)) != TAG_DONE)
    {
        ULONG v = va_arg(ap, ULONG);
        if (tag == OBP_Precision) precision = v;
        else if (tag == OBP_FailIfBad) failIfBad = (int)v;
    }
    va_end(ap);
    for (i = 0; i < SHAREABLE; i++)
    {
        int off = channelOff(want, screen[i], 16) + channelOff(want, screen[i], 8) + channelOff(want, screen[i], 0);
        if (off < bestOff) { bestOff = off; best = i; }
    }
    if (failIfBad &&
        (channelOff(want, screen[best], 16) > (int)precision * 16 ||
         channelOff(want, screen[best], 8) > (int)precision * 16 ||
         channelOff(want, screen[best], 0) > (int)precision * 16))
        return -1;
    return best;
}

void GetRGB32(struct ColorMap *cm, ULONG first, ULONG count, ULONG *table)
{
    (void)cm; (void)count;
    table[0] = spread((screen[first] >> 16) & 0xFF);
    table[1] = spread((screen[first] >> 8) & 0xFF);
    table[2] = spread(screen[first] & 0xFF);
}

static UBYTE pick(ULONG rgb, BOOL *owned)
{
    struct ColorMap cm = { 16 };
    return ObtainNearestPen(&cm, rgb, 1, owned);
}

static void test_red_icon_pen_finds_the_screens_red_not_black_when_only_ui_pens_are_shareable(void)
{
    BOOL owned = TRUE;
    UBYTE pen;

    bestPenCalls = 0;
    pen = pick(0xDD0000, &owned);
    assert(bestPenCalls == 1);
    assert(screen[pen] == 0xDD0000);
    assert(!owned);                 /* a pen the screen holds: never released */
}

static void test_cyan_icon_pen_finds_the_screens_cyan_not_blue(void)
{
    BOOL owned = TRUE;
    UBYTE pen = pick(0x00DDDD, &owned);

    assert(screen[pen] == 0x00DDDD);
    assert(!owned);
}

static void test_an_exact_shareable_match_is_shared_and_owned(void)
{
    BOOL owned = FALSE;
    UBYTE pen = pick(0x5577AA, &owned);

    assert(pen == 3 && owned);      /* round trip: released by the caller */
}

int main(void)
{
    test_red_icon_pen_finds_the_screens_red_not_black_when_only_ui_pens_are_shareable();
    test_cyan_icon_pen_finds_the_screens_cyan_not_blue();
    test_an_exact_shareable_match_is_shared_and_owned();
    puts("test_nearest_pen: OK");
    return 0;
}
