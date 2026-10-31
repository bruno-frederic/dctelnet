/* test/test_iconpens.c -- tool bar icons that carry their own colours. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iconpens.h"
#include "palette.h"

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

static int near(ULONG a, ULONG b)
{
    int shift;

    for (shift = 0; shift <= 16; shift += 8)
        if (abs((int)((a >> shift) & 0xFF) - (int)((b >> shift) & 0xFF)) > 1)
            return 0;
    return 1;
}

/* #27: on a dark custom Workbench (pen 0 #222222, pen 1 light grey) the
 * antialiased edges matched pens by colour and the outlines looked soft. The
 * edge greys are blends of background and symbol: they become the same
 * blends of the screen's background and symbol colours. */
static void test_edges_on_a_dark_workbench_blend_its_own_colours(void)
{
    static const ULONG wb[4] = { 0xAAAAAA, 0x000000, 0xFFFFFF, 0x6688BB };
    static const ULONG dark[4] = { 0x222222, 0xC8C8C8, 0x9A9A9A, 0x3E7A96 };
    int j;

    for (j = 0; j < 4; j++)                               /* roles map exactly */
        assert(IconPens_Translate(wb[j], wb, dark, 4) == dark[j]);
    assert(near(IconPens_Translate(0x717171, wb, dark, 4), 0x595959));  /* 1/3 */
    assert(near(IconPens_Translate(0x383838, wb, dark, 4), 0x919191));  /* 2/3 */
    assert(near(IconPens_Translate(0x99B0D2, wb, dark, 4), 0x5D8597));  /* fill to shine */
    /* On the standard palette nothing moves. */
    assert(IconPens_Translate(0x717171, wb, wb, 4) == 0x717171);
    assert(near(IconPens_Translate(0xCCD7E8, wb, wb, 4), 0xCCD7E8));
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

/* The per-pixel remap the word-parallel one replaced, kept as the reference. */
static void remap_reference(const UWORD *src, UWORD width, UWORD height, UWORD srcDepth,
                            UBYTE pick, UBYTE onoff, const UBYTE *map, int n,
                            UWORD *dst, UWORD dstDepth)
{
    ULONG words = (ULONG)((width + 15) >> 4) * height;
    ULONG w;
    UWORD plane, bit;

    for (w = 0; w < words * dstDepth; w++)
        dst[w] = 0;
    for (w = 0; w < words; w++)
        for (bit = 0; bit < 16; bit++)
        {
            UWORD mask = (UWORD)(0x8000 >> bit), pen = 0, stored = 0;
            UBYTE out;

            for (plane = 0; plane < srcDepth; plane++)
            {
                if (pick & (1 << plane))
                {
                    if (src[stored * words + w] & mask)
                        pen |= (UWORD)(1 << plane);
                    stored++;
                }
                else if (onoff & (1 << plane))
                    pen |= (UWORD)(1 << plane);
            }
            out = map[pen < n ? pen : 0];
            for (plane = 0; plane < dstDepth; plane++)
                if (out & (1 << plane))
                    dst[plane * words + w] |= mask;
        }
}

/* Random images, widths on and off a word boundary, every depth, random plane
 * picks, random maps (also shorter than the pens of the image): the word-parallel
 * remap must equal the per-pixel one word for word. */
static void test_word_parallel_remap_equals_per_pixel(void)
{
    static const UWORD widths[] = { 1, 7, 15, 16, 17, 31, 32, 33, 48, 55, 96 };
    int round, i;

    srand(27);
    for (round = 0; round < 600; round++)
    {
        UWORD width = widths[rand() % (int)(sizeof widths / sizeof widths[0])];
        UWORD height = (UWORD)(1 + rand() % 5), srcDepth = (UWORD)(1 + rand() % 8);
        UWORD dstDepth = (UWORD)(1 + rand() % 8);
        UBYTE pick = (UBYTE)(rand() & ((1 << srcDepth) - 1)), onoff = (UBYTE)(rand() & 0xFF);
        int n = 1 + rand() % 16, planes = 0;
        ULONG words = (ULONG)((width + 15) >> 4) * height;
        UBYTE map[16];
        UWORD src[8 * 6 * 6], got[8 * 6 * 6], want[8 * 6 * 6];

        for (i = 0; i < srcDepth; i++)
            if (pick & (1 << i))
                planes++;
        for (i = 0; i < planes * (int)words; i++)
            src[i] = (UWORD)rand();
        for (i = 0; i < 16; i++)
            map[i] = (UBYTE)(rand() & ((1 << dstDepth) - 1));
        memset(got, 0xA5, sizeof got);
        remap_reference(src, width, height, srcDepth, pick, onoff, map, n, want, dstDepth);
        IconPens_Remap(src, width, height, srcDepth, pick, onoff, map, n, got, dstDepth);
        assert(memcmp(got, want, words * dstDepth * sizeof(UWORD)) == 0);
    }
}

/* The default own screen: icons drawn for pens 0-7 and a map that sends each
 * pen to itself need no redraw at all. */
static void test_an_identity_map_needs_no_remap(void)
{
    static const UBYTE id8[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    static const UBYTE shifted[8] = { 0, 1, 2, 3, 4, 5, 6, 8 };
    static const UBYTE id4[4] = { 0, 1, 2, 3 };
    UWORD src[3], dst[4];
    ULONG before;

    assert(IconPens_IsIdentity(id8, 8, 3, IconPens_Depth(7)));
    assert(!IconPens_IsIdentity(shifted, 8, 3, IconPens_Depth(8)));  /* a pen moved */
    assert(!IconPens_IsIdentity(id8, 8, 3, 5));                      /* deeper destination */
    assert(!IconPens_IsIdentity(id4, 4, 3, 2));                      /* pens 4-7 would go to pen 0 */
    assert(IconPens_IsIdentity(id8, 8, 2, 2));                       /* fewer pens than the map */

    make_ramp(src);
    before = IconPens_RemapCount;
    if (!IconPens_IsIdentity(id8, 8, 3, 3))          /* what OpenToolBarWindow does */
        IconPens_Remap(src, 16, 1, 3, 7, 0, id8, 8, dst, 3);
    assert(IconPens_RemapCount == before);           /* nothing ran */
    IconPens_Remap(src, 16, 1, 3, 7, 0, shifted, 8, dst, 4);
    assert(IconPens_RemapCount == before + 1);       /* the counter does move */
}

/* The tool type the SCIcons carry (tools/toolbar_icons.py SC_PALETTE_TOOLTYPE). */
static const char scicons[] = "000000,DDDDDD,00DD00,DDDD00,000000,DD00DD,555555,DD0000";

/* The colour terminal pen i shows on the default palette, as 0xRRGGBB. */
static void default_terminal_colours(ULONG termRgb[16])
{
    int i;

    for (i = 0; i < 16; i++)
        termRgb[i] = Palette_AnsiColour(defaultDeviceColors, Palette_IbmconToAnsi(i));
}

/* SCIcons were drawn on the default terminal colours of pens 0-7
 * (defaultDeviceColors in src/palette.c, ibmcon pen order) and name them, but
 * pen 4: black, not the palette's blue, and pen 6: the dark grey 555555 of
 * the bevel's shadow and the artwork, not the palette's cyan. The table must stay in step, so a
 * palette change that leaves the icons behind fails here. */
static void test_scicons_palette_is_the_default_terminal_pens_with_black_for_blue(void)
{
    ULONG rgb[ICONPENS_MAX], term[16];
    int i;

    default_terminal_colours(term);
    assert(IconPens_Parse(scicons, rgb) == 8);
    for (i = 0; i < 8; i++)
        assert(rgb[i] == (i == 4 ? 0x000000 : i == 6 ? 0x555555 : term[i]));
    assert(term[4] == 0x0000DD);
    assert(term[6] == 0x00DDDD);
}

/* The SCIcons' pen 4 drew blue on DCTelnet's own screens whatever their tool
 * type said: pen k was terminal colour k. Their declared colours now decide, on
 * every depth: pen 4 takes the terminal's black, the other pens their own
 * terminal colour. Pen 0 is the bar's background pen, here terminal pen 7's. */
static void check_scicons_on(const UBYTE termPen[16])
{
    ULONG rgb[ICONPENS_MAX], term[16];
    UBYTE map[8];
    int k;

    default_terminal_colours(term);
    assert(IconPens_Parse(scicons, rgb) == 8);
    IconPens_TerminalMap(rgb, 8, term, termPen, termPen[15], map);
    assert(map[4] == termPen[0]);                    /* black, not blue */
    for (k = 1; k < 8; k++)
        if (k != 4 && k != 6)
            assert(map[k] == termPen[k]);
    assert(term[8] == 0x555555);                     /* pen 6: the terminal's dark grey */
    assert(map[6] == termPen[8]);
    assert(map[6] != termPen[0] && map[6] != termPen[15]);   /* not black, not the bar */
}

static void test_scicons_pen_4_draws_black_on_16_and_256_colour_screens(void)
{
    UBYTE termPen[16], ansiPens[16];
    int i;

    for (i = 0; i < 16; i++)                         /* 16 colours, console renderer */
        termPen[i] = Palette_TerminalPen(i, NULL, FALSE);
    check_scicons_on(termPen);
    for (i = 0; i < 16; i++)                         /* 16 colours, built-in renderer */
        termPen[i] = Palette_TerminalPen(i, NULL, TRUE);
    check_scicons_on(termPen);
    assert(Palette_AnsiPens(8, ansiPens));           /* 256 colours: the ANSI pens */
    for (i = 0; i < 16; i++)
        termPen[i] = Palette_TerminalPen(i, ansiPens, FALSE);
    check_scicons_on(termPen);
    assert(termPen[0] != 0);                         /* a black of its own, not pen 0 */
}

/* Pen 6 is the one pen of the bevel's shadow and of the artwork (monitor
 * outlines, cable, door frame) and names 555555: on the 16-colour screen all of
 * it draws the terminal's dark grey, never cyan. */
static void test_tool_bar_button_shadow_and_artwork_draw_dark_grey_not_cyan(void)
{
    ULONG rgb[ICONPENS_MAX], term[16];
    UBYTE termPen[16], map[8];
    int i;

    default_terminal_colours(term);
    for (i = 0; i < 16; i++)
        termPen[i] = Palette_TerminalPen(i, NULL, FALSE);
    assert(IconPens_Parse(scicons, rgb) == 8);
    IconPens_TerminalMap(rgb, 8, term, termPen, termPen[15], map);
    assert(term[8] == 0x555555 && term[6] == 0x00DDDD);
    assert(rgb[6] == 0x555555);
    assert(map[6] == termPen[8]);        /* dark grey */
    assert(map[6] != termPen[6]);        /* not cyan */
}

static void test_plane_size_rounds_rows_to_words(void)
{
    assert(IconPens_PlaneSize(55, 30) == 8 * 30);
    assert(IconPens_PlaneSize(16, 1) == 2);
    assert(IconPens_PlaneSize(17, 2) == 8);
}

/* The bar's background on the default palette: white (terminal pen 15), as a
 * pen no terminal colour of the icon should share. */
static void sixteen_pens(UBYTE termPen[16])
{
    int i;

    for (i = 0; i < 16; i++)
        termPen[i] = Palette_TerminalPen(i, NULL, FALSE);
}

/* Measured on an A1200: pen 0 drew terminal black (000000) while the bar is
 * BCBCBC, so every icon sat in a black box and pen 4 (black) merged into it. */
static void test_own_screen_icon_background_matches_toolbar_background(void)
{
    ULONG rgb[ICONPENS_MAX], term[16];
    UBYTE termPen[16], map[8];
    int bg;

    default_terminal_colours(term);
    sixteen_pens(termPen);
    assert(IconPens_Parse(scicons, rgb) == 8);
    for (bg = 0; bg < 16; bg++)
    {
        IconPens_TerminalMap(rgb, 8, term, termPen, termPen[bg], map);
        assert(map[0] == termPen[bg]);               /* pen 0 is the bar's pen, not black */
    }
    IconPens_TerminalMap(rgb, 8, term, termPen, 200, map);   /* a pen of its own */
    assert(map[0] == 200);
}

/* On 16 colours the bar's pen is a terminal pen; pen 1 (DDDDDD) took the
 * terminal grey showing the bar's colour and the light detail vanished. */
static void test_pen_1_never_maps_to_the_background_pen_on_16_colour_screens(void)
{
    ULONG rgb[ICONPENS_MAX], term[16];
    UBYTE termPen[16], map[8];
    int bg, k;

    default_terminal_colours(term);
    sixteen_pens(termPen);
    assert(IconPens_Parse(scicons, rgb) == 8);
    IconPens_TerminalMap(rgb, 8, term, termPen, termPen[7], map);   /* grey AAAAAA bar */
    assert(map[1] != termPen[7]);                    /* DDDDDD would be 7 unexcluded */
    for (bg = 0; bg < 16; bg++)
    {
        IconPens_TerminalMap(rgb, 8, term, termPen, termPen[bg], map);
        for (k = 1; k < 8; k++)
            assert(map[k] != termPen[bg]);
    }
}

/* 32+ colours: colours that got a pen of exactly their own draw on it
 * (DD00DD, not the terminal's AA00AA); the ones that got none keep the
 * terminal pen; a pen that is the bar's is refused. */
static void test_32_and_256_colour_icons_use_their_exact_pens(void)
{
    ULONG rgb[ICONPENS_MAX], term[16];
    UBYTE termPen[16], map[8], exact[8] = { 0, 40, 41, 42, 43, 44, 45, 46 };
    BOOL have[8] = { 0, 1, 1, 1, 1, 1, 1, 1 };
    UBYTE want[8];
    int k;

    default_terminal_colours(term);
    sixteen_pens(termPen);
    assert(IconPens_Parse(scicons, rgb) == 8);
    IconPens_TerminalMap(rgb, 8, term, termPen, 0, want);
    IconPens_TerminalMap(rgb, 8, term, termPen, 0, map);
    have[3] = 0;                                     /* no free pen for colour 3 */
    exact[6] = 0;                                    /* colour 6 got the bar's pen */
    IconPens_ExactMap(8, exact, have, 0, map);
    assert(map[0] == 0);
    for (k = 1; k < 8; k++)
    {
        if (k == 3 || k == 6)
            assert(map[k] == want[k]);               /* the terminal mapping stays */
        else
            assert(map[k] == 40 + k - 1);
    }
}

int main(void)
{
    test_parses_the_shipped_palette();
    test_rejects_a_malformed_palette();
    test_nearest_colour_keeps_greys_grey();
    test_edges_on_a_dark_workbench_blend_its_own_colours();
    test_depth_for_pens();
    test_every_pen_draws_as_its_matched_screen_pen();
    test_plane_pick_and_onoff();
    test_pens_beyond_the_palette_take_the_background();
    test_word_parallel_remap_equals_per_pixel();
    test_an_identity_map_needs_no_remap();
    test_scicons_palette_is_the_default_terminal_pens_with_black_for_blue();
    test_scicons_pen_4_draws_black_on_16_and_256_colour_screens();
    test_tool_bar_button_shadow_and_artwork_draw_dark_grey_not_cyan();
    test_plane_size_rounds_rows_to_words();
    test_own_screen_icon_background_matches_toolbar_background();
    test_pen_1_never_maps_to_the_background_pen_on_16_colour_screens();
    test_32_and_256_colour_icons_use_their_exact_pens();
    printf("iconpens: all assertions passed\n");
    return 0;
}
