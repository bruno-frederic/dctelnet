/* test/test_screenfont.c -- Topaz Pro on square-pixel screens. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "screenfont.h"

/* Topaz was drawn for the Amiga's tall non-square pixels (PAL hires is 22x44
 * ticks per pixel); on square-pixel modes -- every RTG mode -- it looks
 * squashed. Topaz Pro replaces it there, and only it: a font the user chose
 * is never swapped. */
static int picks(const char *font, UWORD size, UWORD rx, UWORD ry, const char *name, UWORD want) {
    struct ScreenFontChoice c;

    ScreenFont_ForMode(font, size, rx, ry, &c);
    return strcmp(c.name, name) == 0 && c.size == want
        && c.topazPro == (strcmp(name, TOPAZ_PRO_NAME) == 0);
}

static void test_topaz_on_square_pixels_becomes_topaz_pro(void) {
    assert(picks("topaz.font", 8, 44, 44, TOPAZ_PRO_NAME, 16));   /* RTG */
    assert(picks("Topaz.font", 8, 22, 22, TOPAZ_PRO_NAME, 16));   /* case-insensitive */
    assert(picks("topaz.font", 8, 22, 44, "topaz.font", 8));      /* PAL hires */
    assert(picks("IBM.font", 8, 44, 44, "IBM.font", 8));          /* the user's choice */
    assert(picks("topaz.font", 8, 0, 0, "topaz.font", 8));        /* no display info */
}

/* An Address Book entry saved with Topaz Pro kept it on a 640x256 PAL
 * screen, where Topaz Pro's square-pixel shapes look stretched: topaz and
 * Topaz Pro are one face, Topaz Pro only on square pixels. */
static void test_topaz_pro_on_tall_pixels_becomes_topaz(void) {
    assert(picks(TOPAZ_PRO_NAME, 16, 22, 44, "topaz.font", 8));   /* PAL hires */
    assert(picks(TOPAZ_PRO_NAME, 16, 44, 44, TOPAZ_PRO_NAME, 16)); /* RTG */
    assert(picks(TOPAZ_PRO_NAME, 16, 0, 0, TOPAZ_PRO_NAME, 16));   /* no display info */
}

/* DCTelnet told the BBS (telnet NAWS) the outer window size: a 640 x 200
 * Workbench window with Topaz Pro 8x16 went out as 80 x 12, while its text
 * area (618 x 184 inside the title bar and borders) holds 77 x 11 -- the grid
 * ibmcon draws. */
static void test_window_size_sent_to_the_bbs_is_the_text_area(void) {
    UWORD cols, rows;

    ScreenFont_Grid(640, 200, 618, 184, TRUE, 8, 16, &cols, &rows);
    assert(cols == 77 && rows == 11);
    ScreenFont_Grid(640, 256, 0, 0, FALSE, 8, 8, &cols, &rows);     /* own screen, no GZZ */
    assert(cols == 80 && rows == 32);
    ScreenFont_Grid(40, 10, 4, 4, TRUE, 8, 16, &cols, &rows);       /* never below 1 x 1 */
    assert(cols == 1 && rows == 1);
    ScreenFont_Grid(1920, 1080, 1900, 1060, TRUE, 8, 16, &cols, &rows);
    assert(cols == 199);                                              /* ibmcon's column cap */
}

/* A Workbench window could be sized wider than 80 columns, and a BBS title
 * drawn for 80 columns wrapped in the wrong places. The window stops at 80
 * columns of text (40 in PETSCII Mode) plus its borders; resized back
 * down, the grid measures 80. */
static void test_window_stops_at_80_columns(void) {
    UWORD max = ScreenFont_MaxWindowWidth(SCREENFONT_ANSI_COLUMNS, 8, 22);
    UWORD cols, rows;

    assert(max == 662);
    ScreenFont_Grid(max, 200, max - 22, 184, TRUE, 8, 16, &cols, &rows);
    assert(cols == 80);
    assert(ScreenFont_MaxWindowWidth(SCREENFONT_PETSCII_COLUMNS, 16, 22) == 662);
}

/* 24x24 tool bar symbols drawn for square pixels looked tall and thin on
 * DCTelnet's own 640x256 screen (hires: ticks 22 x 44). */
static void test_hires_without_interlace_has_tall_pixels(void) {
    assert(ScreenFont_TallPixels(22, 44));      /* hires */
    assert(ScreenFont_TallPixels(11, 44));      /* super-hires */
    assert(!ScreenFont_TallPixels(44, 44));     /* lores */
    assert(!ScreenFont_TallPixels(22, 22));     /* hires interlaced */
    assert(!ScreenFont_TallPixels(0, 0));       /* unknown: square */
}

int main(void) {
    test_hires_without_interlace_has_tall_pixels();
    test_window_stops_at_80_columns();
    test_topaz_on_square_pixels_becomes_topaz_pro();
    test_topaz_pro_on_tall_pixels_becomes_topaz();
    test_window_size_sent_to_the_bbs_is_the_text_area();
    printf("screenfont: all assertions passed\n");
    return 0;
}
