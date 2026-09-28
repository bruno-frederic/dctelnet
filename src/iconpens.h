/* src/iconpens.h -- tool bar icons that carry their own colours.
 * Pure, unit-tested on the host.
 *
 * DCTelnet's tool bar icons used to be drawn for fixed pens (8 colours of the
 * MagicWB palette): on any other palette -- a standard Workbench, DCTelnet's
 * own 256-colour screen -- they came out pink, cyan and blue. An icon now
 * names the colour of each of its pens in the tool type
 * DCTELNET_PALETTE=RRGGBB,RRGGBB,... and DCTelnet redraws its images with the
 * screen's closest pens. Pen 0 is the icon's background. */
#ifndef ICONPENS_H
#define ICONPENS_H

#include <exec/types.h>

#define ICONPENS_TOOLTYPE "DCTELNET_PALETTE"
#define ICONPENS_MAX      16

/* The colours of value ("AAAAAA,000000,...", the tool type's value) as
 * 0x00RRGGBB into rgb[]. Returns how many (0 for a malformed list). */
int IconPens_Parse(const char *value, ULONG rgb[ICONPENS_MAX]);

/* The index of the colour in palette[0..n) closest to rgb. */
int IconPens_Nearest(ULONG rgb, const ULONG *palette, int n);

/* The planes needed to draw pens up to maxPen (1-8). */
UWORD IconPens_Depth(UBYTE maxPen);

/* Bytes of one plane of an image: a row is a whole number of words. */
ULONG IconPens_PlaneSize(UWORD width, UWORD height);

/* Redraws a planar image (struct Image data: srcDepth planes, PlanePick pick,
 * PlaneOnOff onoff) with pen map[p] for every pixel of pen p, into dst:
 * dstDepth full planes. Pens beyond the map's n entries keep pen 0's. */
void IconPens_Remap(const UWORD *src, UWORD width, UWORD height, UWORD srcDepth,
                    UBYTE pick, UBYTE onoff, const UBYTE *map, int n,
                    UWORD *dst, UWORD dstDepth);

#endif /* ICONPENS_H */
