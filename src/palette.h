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

/* TRUE when all 16 entries are RGB4 and not all black: a palette in use. */
BOOL Palette_Valid(const UWORD palette[16]);

/* Makes both of p's palettes usable. One that is not (a DCTelnet 1.x file
 * has only the console's; zeroes; damage) becomes the other in its colour
 * order, or the default when neither is: the built-in renderer drew black
 * text on black with an Address Book entry converted from 1.x. */
void Palette_Repair(struct PrefsStruct *p);

/* ANSI colour `ansi` of a device palette, as 0x00RRGGBB. */
ULONG Palette_AnsiColour(const UWORD deviceColors[16], int ansi);


/* The default palettes: AnsiColors (ANSI order) for the built-in and XEM
 * renderers, DeviceColors (ibmcon order) for the console devices. */
extern const UWORD defaultAnsiColors[16];
extern const UWORD defaultDeviceColors[16];

/* The palette p's renderer shows (Prefs_Palette), as 16 RGB4 colours in
 * ANSI order, and back; and that renderer's default palette. What the
 * ANSI colours editor edits. */
void Palette_Get(struct PrefsStruct *p, UWORD out[16]);
void Palette_Put(struct PrefsStruct *p, const UWORD in[16]);
void Palette_DefaultFor(struct PrefsStruct *p, UWORD out[16]);

/* The colour's name in ANSI order ("Black" ... "Bright White"). */
const char *Palette_Name(int ansiIndex);

/* One channel (0 red, 1 green, 2 blue) of an RGB4 colour (0-15), and the
 * colour with that channel set to value. */
UBYTE Palette_Channel(UWORD rgb4, int channel);
UWORD Palette_WithChannel(UWORD rgb4, int channel, UBYTE value);

/* An RGB4 colour as 0x00RRGGBB. */
ULONG Palette_RGB32(UWORD rgb4);

/* On a true-colour screen the terminal's pixels hold colours, so the
 * palette editor recolours them: every pixel of shown[sel] becomes newRGB.
 * Safe only when no other ANSI colour shares either value -- else the two
 * colours' pixels would merge for good. */
BOOL Palette_SafeRecolour(const ULONG shown[16], int sel, ULONG newRGB);

#endif /* PALETTE_H */
