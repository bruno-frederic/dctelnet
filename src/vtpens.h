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

/* 0xRRGGBB of xterm 256-colour index 16..255 (the 6x6x6 cube, the greys). */
ULONG Vt_Xterm256Rgb(int index);

#endif /* VTPENS_H */
