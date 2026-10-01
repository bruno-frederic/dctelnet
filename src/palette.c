/* src/palette.c -- the 16 ANSI colours on screens of every depth. */
#include "palette.h"

/* ibmcon's unit-1 pen order swaps 1<->7 and 9<->15 against ANSI order. */
int Palette_IbmconToAnsi(int index)
{
    switch (index)
    {
        case 1:  return 7;
        case 7:  return 1;
        case 9:  return 15;
        case 15: return 9;
        default: return index;
    }
}

ULONG Palette_AnsiColour(const UWORD deviceColors[16], int ansi)
{
    UWORD c = deviceColors[Palette_IbmconToAnsi(ansi)];
    ULONG r = (c >> 8) & 0xF, g = (c >> 4) & 0xF, b = c & 0xF;

    return (r * 17) << 16 | (g * 17) << 8 | (b * 17);
}

void Palette_SetAnsiColour(UWORD deviceColors[16], int ansi, ULONG rgb)
{
    ULONG r = ((rgb >> 16 & 0xFF) * 15 + 127) / 255;
    ULONG g = ((rgb >> 8 & 0xFF) * 15 + 127) / 255;
    ULONG b = ((rgb & 0xFF) * 15 + 127) / 255;

    deviceColors[Palette_IbmconToAnsi(ansi)] = (UWORD)(r << 8 | g << 4 | b);
}

BOOL Palette_AnsiPens(UWORD depth, UBYTE pens[16])
{
    int i;

    if (depth < 5)
        return FALSE;
    for (i = 0; i < 8; i++)
    {
        pens[i]     = (UBYTE)(8 + i);
        pens[i + 8] = (UBYTE)(20 + i);
    }
    return TRUE;
}
