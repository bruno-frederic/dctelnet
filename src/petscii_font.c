/* src/petscii_font.c -- the C64 strike fonts from the embedded glyph data
 * (petscii_font.h). Platform independent: tests/test_petscii_font.c runs it
 * on the host. */
#include "petscii_font.h"

UWORD petscii_font_double_row(UBYTE row)
{
    UWORD out = 0;
    int i;

    for (i = 7; i >= 0; i--)
        out = (UWORD)((out << 2) | (((row >> i) & 1) ? 3 : 0));
    return out;
}

void petscii_font_strike(const UBYTE *glyphs, UBYTE *chardata)
{
    int g, r;

    for (g = 0; g < PETSCII_FONT_GLYPHS; g++)
        for (r = 0; r < PETSCII_FONT_YSIZE; r++)
        {
            UWORD wide = petscii_font_double_row(glyphs[g * 8 + r]);
            UBYTE *dst = chardata + r * PETSCII_FONT_MODULO + g * (PETSCII_FONT_XSIZE / 8);

            dst[0] = (UBYTE)(wide >> 8);
            dst[1] = (UBYTE)wide;
        }
}

void petscii_font_charloc(ULONG *charloc)
{
    int g;

    for (g = 0; g < PETSCII_FONT_GLYPHS; g++)
        charloc[g] = ((ULONG)(g * PETSCII_FONT_XSIZE) << 16) | PETSCII_FONT_XSIZE;
    charloc[PETSCII_FONT_GLYPHS] = ((ULONG)(0x20 * PETSCII_FONT_XSIZE) << 16) | PETSCII_FONT_XSIZE;
}
