/* src/vtpens.c -- see vtpens.h. */
#include "vtpens.h"

ULONG Vt_Xterm256Rgb(int index)
{
    static const UBYTE level[6] = { 0, 95, 135, 175, 215, 255 };

    if (index >= 232)
    {
        ULONG g = (ULONG)(8 + 10 * (index - 232));
        return (g << 16) | (g << 8) | g;
    }
    index -= 16;
    return ((ULONG)level[(index / 36) % 6] << 16) | ((ULONG)level[(index / 6) % 6] << 8)
         | (ULONG)level[index % 6];
}

UBYTE Vt_PenFor(vt_color c, const UWORD ansi[16])
{
    long r, g, b, best = 0x7FFFFFFFL;
    ULONG rgb;
    int pen, bestPen = 0;

    if (c == VT_COLOR_DEFAULT)    return 7;
    if (c == VT_COLOR_DEFAULT_BG) return 0;
    if (c < 16)                   return (UBYTE)c;
    rgb = c < 256 ? Vt_Xterm256Rgb((int)c) : (ULONG)VT_RGB_OF(c);
    r = (long)((rgb >> 16) & 0xFF);
    g = (long)((rgb >> 8) & 0xFF);
    b = (long)(rgb & 0xFF);
    for (pen = 0; pen < 16; pen++)
    {
        long dr = r - (long)(((ansi[pen] >> 8) & 15) * 17);
        long dg = g - (long)(((ansi[pen] >> 4) & 15) * 17);
        long db = b - (long)((ansi[pen] & 15) * 17);
        long d = dr * dr + dg * dg + db * db;

        if (d < best)
        {
            best = d;
            bestPen = pen;
        }
    }
    return (UBYTE)bestPen;
}
