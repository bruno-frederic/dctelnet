/* src/iconpens.c -- tool bar icons that carry their own colours. */
#include "iconpens.h"

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int IconPens_Parse(const char *value, ULONG rgb[ICONPENS_MAX])
{
    int n = 0;

    while (*value)
    {
        ULONG c = 0;
        int i;

        if (n == ICONPENS_MAX)
            return 0;
        for (i = 0; i < 6; i++)
        {
            int d = hexval(value[i]);

            if (d < 0)
                return 0;
            c = (c << 4) | (ULONG)d;
        }
        rgb[n++] = c;
        value += 6;
        if (*value == ',')
            value++;
        else if (*value)
            return 0;
    }
    return n;
}

/* Distance as the eye weighs it: brightness first, then hue. In plain RGB
 * distance the dark grey of an antialiased edge (717171) is nearer the
 * Workbench's blue (6688BB) than its grey, and edges came out blue. */
static void luma_chroma(ULONG c, LONG *y, LONG *cr, LONG *cb)
{
    LONG r = (LONG)((c >> 16) & 0xFF), g = (LONG)((c >> 8) & 0xFF), b = (LONG)(c & 0xFF);

    *y = (299 * r + 587 * g + 114 * b) / 1000;
    *cr = r - *y;
    *cb = b - *y;
}

int IconPens_Nearest(ULONG rgb, const ULONG *palette, int n)
{
    int i, best = 0;
    ULONG bestDist = 0xFFFFFFFFUL;
    LONG y, cr, cb;

    luma_chroma(rgb, &y, &cr, &cb);
    for (i = 0; i < n; i++)
    {
        LONG py, pcr, pcb;
        ULONG d;

        luma_chroma(palette[i], &py, &pcr, &pcb);
        d = (ULONG)((y - py) * (y - py) + 4 * ((cr - pcr) * (cr - pcr) + (cb - pcb) * (cb - pcb)));
        if (d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

UWORD IconPens_Depth(UBYTE maxPen)
{
    UWORD depth = 1;

    while (depth < 8 && (maxPen >> depth))
        depth++;
    return depth;
}

ULONG IconPens_PlaneSize(UWORD width, UWORD height)
{
    return (ULONG)((width + 15) >> 4) * 2 * height;
}

void IconPens_Remap(const UWORD *src, UWORD width, UWORD height, UWORD srcDepth,
                    UBYTE pick, UBYTE onoff, const UBYTE *map, int n,
                    UWORD *dst, UWORD dstDepth)
{
    ULONG words = (ULONG)((width + 15) >> 4) * height;   /* one plane, in words */
    ULONG w;
    UWORD plane;

    for (w = 0; w < words * dstDepth; w++)
        dst[w] = 0;
    for (w = 0; w < words; w++)
    {
        UWORD bit;

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
}
