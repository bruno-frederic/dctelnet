/* src/glyphraster.c -- drawn glyphs as bitmap rows (see glyphraster.h). */
#include "glyphraster.h"

static void put(UWORD rows[], int w, int h, int x, int y)
{
    if (x >= 0 && x < w && y >= 0 && y < h)
        rows[y] |= (UWORD)(0x8000U >> x);
}

static void hline(UWORD rows[], int w, int h, int x0, int x1, int y)
{
    int x;

    for (x = x0; x <= x1; x++)
        put(rows, w, h, x, y);
}

static void vline(UWORD rows[], int w, int h, int x, int y0, int y1)
{
    int y;

    for (y = y0; y <= y1; y++)
        put(rows, w, h, x, y);
}

static void fill(UWORD rows[], int w, int h, int x0, int y0, int x1, int y1)
{
    int y;

    for (y = y0; y <= y1; y++)
        hline(rows, w, h, x0, x1, y);
}

/* amiga_render.c draw_box: arms from the cell centre to its edges, 1 px
 * light, 2 px heavy, two 1 px lines double. */
static void draw_box(UWORD rows[], int w, int h, UBYTE code)
{
    int cx = w / 2, cy = h / 2, x1 = w - 1, y1 = h - 1;
    int arm;

    for (arm = 0; arm < 4; arm++)
    {
        int wt = VT_BOX_ARM(code, arm);
        int ax0, ay0, ax1, ay1, dx = 0, i, n;

        if (!wt)
            continue;
        switch (arm)
        {
        case 0: ax0 = cx; ay0 = 0;  ax1 = cx; ay1 = cy; dx = 1; break;
        case 1: ax0 = cx; ay0 = cy; ax1 = x1; ay1 = cy; break;
        case 2: ax0 = cx; ay0 = cy; ax1 = cx; ay1 = y1; dx = 1; break;
        default: ax0 = 0; ay0 = cy; ax1 = cx; ay1 = cy; break;
        }
        /* offsets of the lines: single 0; heavy 0 and +1; double -1 and +1 */
        n = wt == 1 ? 1 : 2;
        for (i = 0; i < n; i++)
        {
            int off = wt == 3 ? (i ? 1 : -1) : i;

            if (dx)
                vline(rows, w, h, ax0 + off, ay0, ay1);
            else
                hline(rows, w, h, ax0, ax1, ay0 + off);
        }
    }
}

static const UWORD shade_pat[3][2] = {
    { 0x8888, 0x2222 }, /* light: 25 % */
    { 0xAAAA, 0x5555 }, /* medium: 50 % */
    { 0x7777, 0xDDDD }  /* dark: 75 % */
};

/* amiga_render.c draw_block */
static void draw_block(UWORD rows[], int w, int h, UBYTE code)
{
    if (code & 0x80)
    {
        UWORD mask = (UWORD)(w >= 16 ? 0xFFFFU : (0xFFFFU << (16 - w)) & 0xFFFFU);
        int y;

        for (y = 0; y < h; y++)
            rows[y] = (UWORD)(shade_pat[(code & 3) - 1][y & 1] & mask);
        return;
    }
    if (code & 0x40)
    {
        int hw = w / 2, hh = h / 2;

        if (code & 1)
            fill(rows, w, h, 0, 0, hw - 1, hh - 1);
        if (code & 2)
            fill(rows, w, h, hw, 0, w - 1, hh - 1);
        if (code & 4)
            fill(rows, w, h, 0, hh, hw - 1, h - 1);
        if (code & 8)
            fill(rows, w, h, hw, hh, w - 1, h - 1);
        return;
    }
    {
        int side = (code >> 4) & 3, k = code & 15;
        int bw = (w * k + 4) / 8, bh = (h * k + 4) / 8;

        switch (side)
        {
        case 0: fill(rows, w, h, 0, 0, w - 1, bh - 1); break;
        case 1: fill(rows, w, h, 0, h - bh, w - 1, h - 1); break;
        case 2: fill(rows, w, h, 0, 0, bw - 1, h - 1); break;
        default: fill(rows, w, h, w - bw, 0, w - 1, h - 1); break;
        }
    }
}

/* One pixel per row, x interpolated across the cell: from (x0, y = 0) to
 * (x1, y = h - 1). */
static void diagonal(UWORD rows[], int w, int h, int x0, int x1)
{
    int y, dy = h > 1 ? h - 1 : 1;

    for (y = 0; y < h; y++)
        put(rows, w, h, x0 + (x1 - x0) * y / dy, y);
}

void GlyphRaster_Draw(vt_glyph g, int w, int h, UWORD rows[])
{
    int y;

    for (y = 0; y < h; y++)
        rows[y] = 0;
    switch (g.kind)
    {
    case VT_GLYPH_BOX:
        draw_box(rows, w, h, g.code);
        break;
    case VT_GLYPH_BLOCK:
        draw_block(rows, w, h, g.code);
        break;
    case VT_GLYPH_DIAGONAL:
        if (g.code & 1)
            diagonal(rows, w, h, w - 1, 0);     /* / */
        if (g.code & 2)
            diagonal(rows, w, h, 0, w - 1);     /* \ */
        break;
    case VT_GLYPH_HLINE:
        hline(rows, w, h, 0, w - 1, (h - 1) * g.code / 7);
        break;
    case VT_GLYPH_DIAMOND:
    {
        int cx = w / 2, cy = h / 2, rx = w / 2 - 1, ry = h / 2 - 1, d;

        for (d = 0; d <= ry; d++)
        {
            int half = rx * (ry - d) / (ry ? ry : 1);

            hline(rows, w, h, cx - half, cx + half, cy - d);
            hline(rows, w, h, cx - half, cx + half, cy + d);
        }
        break;
    }
    default:
        break;
    }
}
