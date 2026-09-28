/* src/palette.c -- the 16 ANSI colours on screens of every depth. */
#include "palette.h"
#include "prefs_file.h"

// prefs.AnsiColors: 16-color CGA/VGA text palette in ANSI order. Convenient for the built-in
// renderer: ANSI SGR color numbers map directly (Black, Red, Green, Yellow, Blue, Magenta, Cyan,
// White), requiring only a subtraction.
// Values: 4 unused bits followed by 4 bits for each colour channel: Red, Green, Blue.
const UWORD defaultAnsiColors[16] = {
    0x0000,  // 0 : #000 black
    0x0A00,  // 1 : #A00 red
    0x00A0,  // 2 : #0A0 green
    0x0A50,  // 3 : #A50 brown
    0x000A,  // 4 : #00A blue
    0x0A0A,  // 5 : #A0A magenta
    0x00AA,  // 6 : #0AA cyan
    0x0AAA,  // 7 : #AAA dark white = light gray (!= #FFF bright white)

    // Bright variants: the built-in renderer treats a color as bright when atr_bold or
    // atr_blink is set.
    // #555  #F55 #5F5  #FF5  #55F  #F5F  #5FF  #FFF
        0x0555, 0x0F55, 0x05F5, 0x0FF5, 0x055F, 0x0F5F, 0x05FF, 0x0FFF
};

// prefs.DeviceColors: original ibmcon/console.device palette, brighter than ANSI and with less
// contrast between regular and bright color variants
const UWORD defaultDeviceColors[16] = {
    0x0000,  // 0 : #000 black
    0x0DDD,  // 1 : #DDD dark white = light gray (order differs from ANSI)
    0x00D0,  // 2 : #0D0 green
    0x0DD0,  // 3 : #DD0 yellow
    0x000D,  // 4 : #00D blue
    0x0D0D,  // 5 : #D0D magenta
    0x00DD,  // 6 : #0DD cyan
    0x0D00,  // 7 : #D00 red (order differs from ANSI)

    // brighter :
    // #555  #FFF #5F0  #FF0  #00F  #F0F  #0FF  #F00
        0x0555, 0x0FFF, 0x00F0, 0x0FF0, 0x000F, 0x0F0F, 0x00FF, 0x0F00
};

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

static const char *const ansiNames[16] =
{
    "Black", "Red", "Green", "Brown", "Blue", "Magenta", "Cyan", "White",
    "Grey", "Bright Red", "Bright Green", "Yellow", "Bright Blue", "Bright Magenta",
    "Bright Cyan", "Bright White"
};

void Palette_Get(struct PrefsStruct *p, UWORD out[16])
{
    const UWORD *pal = Prefs_Palette(p);
    BOOL device = pal == p->DeviceColors;
    int i;

    for (i = 0; i < 16; i++)
        out[i] = pal[device ? Palette_IbmconToAnsi(i) : i];
}

void Palette_Put(struct PrefsStruct *p, const UWORD in[16])
{
    UWORD *pal = Prefs_Palette(p);
    BOOL device = pal == p->DeviceColors;
    int i;

    for (i = 0; i < 16; i++)
        pal[device ? Palette_IbmconToAnsi(i) : i] = in[i];
}

void Palette_DefaultFor(struct PrefsStruct *p, UWORD out[16])
{
    const UWORD *def = Prefs_Palette(p) == p->DeviceColors ? defaultDeviceColors : defaultAnsiColors;
    BOOL device = def == defaultDeviceColors;
    int i;

    for (i = 0; i < 16; i++)
        out[i] = def[device ? Palette_IbmconToAnsi(i) : i];
}

const char *Palette_Name(int ansiIndex)
{
    return (ansiIndex >= 0 && ansiIndex < 16) ? ansiNames[ansiIndex] : "";
}

UBYTE Palette_Channel(UWORD rgb4, int channel)
{
    return (UBYTE)((rgb4 >> (4 * (2 - channel))) & 0xF);
}

UWORD Palette_WithChannel(UWORD rgb4, int channel, UBYTE value)
{
    int shift = 4 * (2 - channel);

    return (UWORD)((rgb4 & ~(0xFU << shift)) | ((value & 0xFU) << shift));
}

ULONG Palette_RGB32(UWORD rgb4)
{
    ULONG r = (rgb4 >> 8) & 0xF, g = (rgb4 >> 4) & 0xF, b = rgb4 & 0xF;

    return (r * 17) << 16 | (g * 17) << 8 | (b * 17);
}

BOOL Palette_SafeRecolour(const ULONG shown[16], int sel, ULONG newRGB)
{
    int i;

    for (i = 0; i < 16; i++)
        if (i != sel && (shown[i] == shown[sel] || shown[i] == newRGB))
            return FALSE;
    return TRUE;
}

BOOL Palette_Valid(const UWORD palette[16])
{
    BOOL nonZero = FALSE;
    int i;

    for (i = 0; i < 16; i++)
    {
        if (palette[i] & 0xF000)
            return FALSE;
        if (palette[i])
            nonZero = TRUE;
    }
    return nonZero;
}

void Palette_Repair(struct PrefsStruct *p)
{
    BOOL ansiOk = Palette_Valid(p->AnsiColors), deviceOk = Palette_Valid(p->DeviceColors);
    int i;

    for (i = 0; i < 16; i++)       /* (the order swap is its own inverse) */
    {
        if (!ansiOk)
            p->AnsiColors[i] = deviceOk ? p->DeviceColors[Palette_IbmconToAnsi(i)] : defaultAnsiColors[i];
        if (!deviceOk)
            p->DeviceColors[i] = ansiOk ? p->AnsiColors[Palette_IbmconToAnsi(i)] : defaultDeviceColors[i];
    }
}
