/* tests/test_glyphraster.c -- box, block and diagonal glyphs as bitmap rows
 * (8 wide, 8 high; bit 15 is the leftmost pixel). */
#include <assert.h>
#include <stdio.h>
#include "glyphraster.h"

/* The rows the code point is drawn as, through the glyph map the renderer uses. */
static void raster(vt_u32 cp, int w, UWORD rows[8])
{
    vt_glyph g = vt_map_glyph(cp, VT_ENC_LATIN1);

    assert(g.kind != VT_GLYPH_FONT);
    GlyphRaster_Draw(g, w, 8, rows);
}

static int rows_are(const UWORD got[8], const UWORD want[8])
{
    int y;

    for (y = 0; y < 8; y++)
        if (got[y] != want[y])
        {
            printf("row %d: got %04X want %04X\n", y, got[y], want[y]);
            return 0;
        }
    return 1;
}

static void test_box_lines(void)
{
    UWORD r[8];
    static const UWORD light_h[8] = { 0, 0, 0, 0, 0xFF00, 0, 0, 0 };
    static const UWORD heavy_h[8] = { 0, 0, 0, 0, 0xFF00, 0xFF00, 0, 0 };
    static const UWORD double_h[8] = { 0, 0, 0, 0xFF00, 0, 0xFF00, 0, 0 };
    static const UWORD light_v[8] = { 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800 };
    static const UWORD corner[8] = { 0, 0, 0, 0, 0x0F00, 0x0800, 0x0800, 0x0800 };

    raster(0x2500, 8, r); assert(rows_are(r, light_h));
    raster(0x2501, 8, r); assert(rows_are(r, heavy_h));
    raster(0x2550, 8, r); assert(rows_are(r, double_h));
    raster(0x2502, 8, r); assert(rows_are(r, light_v));
    raster(0x250C, 8, r); assert(rows_are(r, corner));
    /* a 16 pixel cell: the line reaches both edges */
    raster(0x2500, 16, r); assert(r[4] == 0xFFFF && r[3] == 0 && r[5] == 0);
}

static void test_blocks_and_shades(void)
{
    UWORD r[8];
    static const UWORD full[8] = { 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00 };
    static const UWORD upper[8] = { 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0, 0, 0, 0 };
    static const UWORD shade[8] = { 0x8800, 0x2200, 0x8800, 0x2200, 0x8800, 0x2200, 0x8800, 0x2200 };
    static const UWORD upper_left[8] = { 0xF000, 0xF000, 0xF000, 0xF000, 0, 0, 0, 0 };

    raster(0x2588, 8, r); assert(rows_are(r, full));
    raster(0x2580, 8, r); assert(rows_are(r, upper));
    raster(0x2591, 8, r); assert(rows_are(r, shade));
    raster(0x2598, 8, r); assert(rows_are(r, upper_left));
}

static void test_diagonal(void)
{
    UWORD r[8];
    int y;

    raster(0x2571, 8, r);               /* bottom-left to top-right */
    for (y = 0; y < 8; y++)
        assert(r[y] == (UWORD)(0x8000U >> (7 - y)));
    raster(0x2572, 8, r);
    for (y = 0; y < 8; y++)
        assert(r[y] == (UWORD)(0x8000U >> y));
}

int main(void)
{
    test_box_lines();
    test_blocks_and_shades();
    test_diagonal();
    printf("glyphraster: all assertions passed\n");
    return 0;
}
