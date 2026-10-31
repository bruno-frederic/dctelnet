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

/* How far apart two 0x00RRGGBB colours look (IconPens_Nearest's measure). */
ULONG IconPens_Distance(ULONG a, ULONG b);

/* The pens for an icon on DCTelnet's own screen: each colour rgb[0..n) of the
 * tool type to the closest of the 16 terminal colours, termRgb[i] being the colour
 * terminal pen termPen[i] shows (the screen's colour map, not the defaults: the
 * terminal palette is the user's), into map[0..n). Without it pen k was terminal
 * colour k whatever the tool type said, so the declared colours did not decide. */
void IconPens_TerminalMap(const ULONG *rgb, int n, const ULONG termRgb[16],
                          const UBYTE termPen[16], UBYTE *map);

/* rgb carried from the palette from[0..n) to to[0..n): a colour of from
 * becomes the colour of to at the same index, and a blend of two of them (an
 * antialiased edge between symbol and background) the same blend of their
 * counterparts. The blend is read from the nearest segment between two
 * colours, the shorter one on a tie. */
ULONG IconPens_Translate(ULONG rgb, const ULONG *from, const ULONG *to, int n);

/* The planes needed to draw pens up to maxPen (1-8). */
UWORD IconPens_Depth(UBYTE maxPen);

/* Bytes of one plane of an image: a row is a whole number of words. */
ULONG IconPens_PlaneSize(UWORD width, UWORD height);

/* Whether map leaves every pen of a srcDepth image as it is (map[p] == p, all
 * pens of the image in the map) and the destination has the same depth: the image
 * is then drawn as it is and IconPens_Remap is not called. */
int IconPens_IsIdentity(const UBYTE *map, int n, UWORD srcDepth, UWORD dstDepth);

/* How many images IconPens_Remap has redrawn (the sentinel of the tests). */
extern ULONG IconPens_RemapCount;

/* Redraws a planar image (struct Image data: srcDepth planes, PlanePick pick,
 * PlaneOnOff onoff) with pen map[p] for every pixel of pen p, into dst:
 * dstDepth full planes. Pens beyond the map's n entries keep pen 0's. */
void IconPens_Remap(const UWORD *src, UWORD width, UWORD height, UWORD srcDepth,
                    UBYTE pick, UBYTE onoff, const UBYTE *map, int n,
                    UWORD *dst, UWORD dstDepth);

#endif /* ICONPENS_H */
