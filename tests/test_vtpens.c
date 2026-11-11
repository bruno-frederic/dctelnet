/* tests/test_vtpens.c -- MA2-7: the screen pen for a 256-colour or 24-bit
 * cell colour on the 16-pen screen. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vtpens.h"

/* The default AnsiColors: pen n holds the standard ANSI colour n (RGB4). */
static const UWORD ansi[16] = {
    0x000, 0xA00, 0x0A0, 0xA50, 0x00A, 0xA0A, 0x0AA, 0xAAA,
    0x555, 0xF55, 0x5F5, 0xFF5, 0x55F, 0xF5F, 0x5FF, 0xFFF
};

static void test_xterm_table_matches_the_engine(void)
{
    int i;

    assert(Vt_Xterm256Rgb(16) == 0x000000UL);
    assert(Vt_Xterm256Rgb(231) == 0xFFFFFFUL);
    assert(Vt_Xterm256Rgb(196) == 0xFF0000UL);
    assert(Vt_Xterm256Rgb(67) == 0x5F87AFUL);
    assert(Vt_Xterm256Rgb(232) == 0x080808UL);
    assert(Vt_Xterm256Rgb(255) == 0xEEEEEEUL);
    /* the engine's own RGB -> index map is the inverse on the whole range */
    for (i = 16; i < 256; i++)
        assert(vt_rgb_to_256((vt_u32)Vt_Xterm256Rgb(i)) == i);
}

static void test_defaults_and_low_colours(void)
{
    int i;

    assert(Vt_PenFor(VT_COLOR_DEFAULT, ansi) == 7);
    assert(Vt_PenFor(VT_COLOR_DEFAULT_BG, ansi) == 0);
    for (i = 0; i < 16; i++)
        assert(Vt_PenFor((vt_color)i, ansi) == i);
}

static void test_256_colours_take_the_nearest_pen(void)
{
    assert(Vt_PenFor(16, ansi) == 0);           /* 000000 -> black */
    assert(Vt_PenFor(231, ansi) == 15);         /* FFFFFF -> bright white */
    assert(Vt_PenFor(196, ansi) == 1);          /* FF0000: red AA0000 is 85 away, FF5555 is 120 */
    assert(Vt_PenFor(21, ansi) == 4);           /* 0000FF -> blue 0000AA */
    assert(Vt_PenFor(244, ansi) == 7);          /* 808080: AAAAAA is 73 away, 555555 is 75 */
}

static void test_24_bit_matches_the_same_256_colour(void)
{
    assert(Vt_PenFor(VT_RGB(0xFF, 0x00, 0x00), ansi) == Vt_PenFor(196, ansi));
    assert(Vt_PenFor(VT_RGB(0xFF, 0x00, 0x00), ansi) == 1);
    assert(Vt_PenFor(VT_RGB(0x00, 0x00, 0x00), ansi) == 0);
    assert(Vt_PenFor(VT_RGB(0xFF, 0xFF, 0xFF), ansi) == 15);
    assert(Vt_PenFor(VT_RGB(0x00, 0xAA, 0x00), ansi) == 2);   /* exact pen 2 */
}

int main(void)
{
    test_xterm_table_matches_the_engine();
    test_defaults_and_low_colours();
    test_256_colours_take_the_nearest_pen();
    test_24_bit_matches_the_same_256_colour();
    printf("vtpens: all assertions passed\n");
    return 0;
}
