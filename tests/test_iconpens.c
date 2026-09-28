/* test/test_iconpens.c -- tool bar icons that carry their own colours. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "iconpens.h"

/* The palette the shipped icons carry (tools/toolbar_icons.py). */
static const char shipped[] = "AAAAAA,000000,FFFFFF,6688BB,717171,383838,99B0D2,CCD7E8";

static void test_parses_the_shipped_palette(void)
{
    ULONG rgb[ICONPENS_MAX];

    assert(IconPens_Parse(shipped, rgb) == 8);
    assert(rgb[0] == 0xAAAAAA && rgb[3] == 0x6688BB && rgb[7] == 0xCCD7E8);
    assert(IconPens_Parse("aabbcc", rgb) == 1 && rgb[0] == 0xAABBCC);
}

static void test_rejects_a_malformed_palette(void)
{
    ULONG rgb[ICONPENS_MAX];

    assert(IconPens_Parse("AAAAA", rgb) == 0);            /* short */
    assert(IconPens_Parse("AAAAAG", rgb) == 0);           /* not hex */
    assert(IconPens_Parse("AAAAAA;000000", rgb) == 0);    /* separator */
    assert(IconPens_Parse("", rgb) == 0);
    assert(IconPens_Parse("000000,000000,000000,000000,000000,000000,000000,000000,"
                          "000000,000000,000000,000000,000000,000000,000000,000000,"
                          "000000", rgb) == 0);           /* 17 > ICONPENS_MAX */
}

/* Plain RGB distance put the antialiased edge's dark grey on the Workbench's
 * blue: a 4-colour Workbench drew blue fringes. */
static void test_nearest_colour_keeps_greys_grey(void)
{
    static const ULONG wb[4] = { 0xAAAAAA, 0x000000, 0xFFFFFF, 0x6688BB };

    assert(IconPens_Nearest(0x717171, wb, 4) == 0);       /* dark blend: grey */
    assert(IconPens_Nearest(0x383838, wb, 4) == 1);       /* darker: black */
    assert(IconPens_Nearest(0xCCD7E8, wb, 4) == 2);
    assert(IconPens_Nearest(0x6080C0, wb, 4) == 3);
}

static void test_depth_for_pens(void)
{
    assert(IconPens_Depth(0) == 1 && IconPens_Depth(1) == 1);
    assert(IconPens_Depth(2) == 2 && IconPens_Depth(7) == 3);
    assert(IconPens_Depth(8) == 4 && IconPens_Depth(27) == 5);
    assert(IconPens_Depth(255) == 8);
}

/* A 16x1 image, depth 3, pixel x has pen x & 7. */
static void make_ramp(UWORD src[3])
{
    int x, p;

    memset(src, 0, 3 * sizeof(UWORD));
    for (x = 0; x < 16; x++)
        for (p = 0; p < 3; p++)
            if ((x & 7) & (1 << p))
                src[p] |= (UWORD)(0x8000 >> x);
}

static int pen_at(const UWORD *img, int words, int depth, int x)
{
    int p, pen = 0;

    for (p = 0; p < depth; p++)
        if (img[p * words] & (0x8000 >> x))
            pen |= 1 << p;
    return pen;
}

/* The bug: pen 5 of an icon drew as the screen's pen 5, whatever colour that
 * was (cyan on DCTelnet's own 256-colour screen). Every pen must draw as the
 * screen pen its colour was matched to. */
static void test_every_pen_draws_as_its_matched_screen_pen(void)
{
    UWORD src[3], dst[5];
    static const UBYTE map[8] = { 0, 1, 2, 3, 28, 29, 30, 31 };
    int x;

    make_ramp(src);
    IconPens_Remap(src, 16, 1, 3, 7, 0, map, 8, dst, 5);
    for (x = 0; x < 16; x++)
        assert(pen_at(dst, 1, 5, x) == map[x & 7]);
}

static void test_plane_pick_and_onoff(void)
{
    /* Depth 3, only planes 0 and 2 stored (PlanePick 5), plane 1 all on. */
    UWORD src[2] = { 0xFF00, 0x0FF0 };
    static const UBYTE map[8] = { 10, 11, 12, 13, 14, 15, 16, 17 };
    UWORD dst[5];

    IconPens_Remap(src, 16, 1, 3, 5, 2, map, 8, dst, 5);
    assert(pen_at(dst, 1, 5, 0) == map[1 | 2]);        /* plane 0 on */
    assert(pen_at(dst, 1, 5, 5) == map[1 | 2 | 4]);    /* planes 0 and 2 */
    assert(pen_at(dst, 1, 5, 10) == map[2 | 4]);
    assert(pen_at(dst, 1, 5, 15) == map[2]);
}

static void test_pens_beyond_the_palette_take_the_background(void)
{
    UWORD src[3], dst[2];
    static const UBYTE map[4] = { 3, 1, 2, 0 };

    make_ramp(src);
    IconPens_Remap(src, 16, 1, 3, 7, 0, map, 4, dst, 2);
    assert(pen_at(dst, 1, 2, 2) == 2);
    assert(pen_at(dst, 1, 2, 6) == 3);       /* pen 6 > 4 entries: map[0] */
}

static void test_plane_size_rounds_rows_to_words(void)
{
    assert(IconPens_PlaneSize(55, 30) == 8 * 30);
    assert(IconPens_PlaneSize(16, 1) == 2);
    assert(IconPens_PlaneSize(17, 2) == 8);
}

int main(void)
{
    test_parses_the_shipped_palette();
    test_rejects_a_malformed_palette();
    test_nearest_colour_keeps_greys_grey();
    test_depth_for_pens();
    test_every_pen_draws_as_its_matched_screen_pen();
    test_plane_pick_and_onoff();
    test_pens_beyond_the_palette_take_the_background();
    test_plane_size_rounds_rows_to_words();
    printf("iconpens: all assertions passed\n");
    return 0;
}
