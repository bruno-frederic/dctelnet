/* tests/test_palette.c -- ANSI colours and pen layout for deep screens. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "palette.h"

/* The device palette is RGB4 in ibmcon's unit-1 order (pen 1 white, pen 7
 * red, 9/15 swapped likewise); the ANSI pens of a deep screen want ANSI order
 * (1 red, 7 white) at 8 bits a gun. */
static void test_ansi_colours_come_from_the_device_palette_in_ansi_order(void) {
    UWORD dev[16];

    memset(dev, 0, sizeof(dev));
    dev[1] = 0xDDD;   /* ibmcon 1 = white */
    dev[7] = 0xD00;   /* ibmcon 7 = red */
    dev[9] = 0xFFF;   /* ibmcon 9 = bright white */
    dev[15] = 0xF00;  /* ibmcon 15 = bright red */
    dev[2] = 0x0A0;
    assert(Palette_AnsiColour(dev, 1) == 0xDD0000);    /* ANSI 1 red   */
    assert(Palette_AnsiColour(dev, 7) == 0xDDDDDD);    /* ANSI 7 white */
    assert(Palette_AnsiColour(dev, 9) == 0xFF0000);
    assert(Palette_AnsiColour(dev, 15) == 0xFFFFFF);
    assert(Palette_AnsiColour(dev, 2) == 0x00AA00);
}

/* A colour edited on an ANSI pen goes back to its ibmcon slot, rounded. */
static void test_an_edited_ansi_colour_lands_in_its_device_slot(void) {
    UWORD dev[16];

    memset(dev, 0, sizeof(dev));
    Palette_SetAnsiColour(dev, 1, 0xFF0000);
    Palette_SetAnsiColour(dev, 7, 0x807F7F);
    assert(dev[7] == 0xF00);          /* ibmcon 7 = ANSI red */
    assert(dev[1] == 0x877);          /* rounds to nearest: 0x80 -> 8, 0x7F -> 7 */
    assert(Palette_AnsiColour(dev, 1) == 0xFF0000);
}

/* Where the 16 ANSI colours live: shared pens 0-15 below 32 colours (no
 * table), otherwise pens of their own that skip the UI pens 0-7 and the AGA
 * pointer pens 16-19. */
static void test_ansi_pen_layout(void) {
    UBYTE pens[16];
    int i;

    assert(!Palette_AnsiPens(4, pens));
    assert(Palette_AnsiPens(5, pens));
    for (i = 0; i < 8; i++)  assert(pens[i] == 8 + i);
    for (i = 8; i < 16; i++) assert(pens[i] == 20 + (i - 8));
    assert(Palette_AnsiPens(8, pens) && pens[15] == 27);
    assert(Palette_AnsiPens(24, pens) && pens[0] == 8);   /* RTG true colour */
}

int main(void) {
    test_ansi_colours_come_from_the_device_palette_in_ansi_order();
    test_an_edited_ansi_colour_lands_in_its_device_slot();
    test_ansi_pen_layout();
    printf("palette: all assertions passed\n");
    return 0;
}
