/* src/vtpens.h -- the screen pen for a cell colour. Pure, unit-tested on the
 * host. The engine hands over a palette index (0-255), a direct colour
 * (VT_COLOR_RGB | 0xRRGGBB) or a default; the screen has the 16 ANSI colours
 * in pens 0-15 (prefs.AnsiColors). */
#ifndef VTPENS_H
#define VTPENS_H

#include <exec/types.h>
#include "vtengine.h"

/* The pen: VT_COLOR_DEFAULT is 7, VT_COLOR_DEFAULT_BG 0, 0-15 themselves,
 * 16-255 the xterm 256-colour table's RGB and a direct colour its own RGB,
 * then the nearest of the 16 pens by squared RGB distance (the lowest pen
 * wins a tie). ansi: the 16 pens as RGB4, pen n holds ANSI colour n. */
UBYTE Vt_PenFor(vt_color c, const UWORD ansi[16]);

/* Does a screen get obtained pens for the 256 colours and direct colours?
 * Yes when its palette is larger than the 16 ANSI pens: more than 4 planes
 * (32, 64, 128 or 256 colour screens, RTG true/high colour), graphics.library
 * 39 or later (ObtainBestPen exists), and the display mode is neither HAM
 * (pens are not colours there) nor Extra Half-Brite (the upper 32 pens are
 * derived copies). Otherwise the 16 ANSI pens stand in. */
BOOL Vt_ObtainsPens(ULONG depth, ULONG modeId, UWORD gfxVersion);

/* 0xRRGGBB of xterm 256-colour index 16..255 (the 6x6x6 cube, the greys). */
ULONG Vt_Xterm256Rgb(int index);

#endif /* VTPENS_H */
