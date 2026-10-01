/* src/palette.h -- the 16 ANSI colours on screens of every depth.
 *
 * The console devices keep their palette in prefs.DeviceColors (RGB4, in
 * ibmcon's unit-1 pen order: pen 1 white, pen 7 red). On 32+ colours the
 * ANSI colours get pens of their own, loaded in ANSI order from it.
 *
 * Pure functions, unit-tested on the host.
 */
#ifndef PALETTE_H
#define PALETTE_H

#include "prefs.h"

/* TRUE when a screen of this depth gives the ANSI colours pens of their own:
 * 32+ colours, pens 8-15 and 20-27 (UI pens 0-7 and the AGA pointer pens
 * 16-19 left alone), in ANSI order. FALSE: the ANSI colours share pens 0-15
 * with the UI, as before. */
BOOL Palette_AnsiPens(UWORD depth, UBYTE pens[16]);

/* ibmcon's unit-1 order <-> ANSI order (the swap is its own inverse). */
int Palette_IbmconToAnsi(int index);

/* ANSI colour `ansi` of a device palette, as 0x00RRGGBB. */
ULONG Palette_AnsiColour(const UWORD deviceColors[16], int ansi);

/* Sets ANSI colour `ansi` (0x00RRGGBB, rounded to RGB4) in a device palette. */
void Palette_SetAnsiColour(UWORD deviceColors[16], int ansi, ULONG rgb);

#endif /* PALETTE_H */
