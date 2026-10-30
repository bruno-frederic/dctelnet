/* src/glyphraster.h -- box, block and other drawn glyphs as bitmap rows, for
 * the built-in renderer's cells (a font has no such characters in Latin-1).
 * Pure, host-tested. The geometry is vtcon's own Amiga renderer's
 * (third_party/vtcon/render/amiga_render.c: draw_box, draw_block,
 * draw_special), so both renderers draw alike. */
#ifndef GLYPHRASTER_H
#define GLYPHRASTER_H

#include <exec/types.h>
#include "glyphmap.h"

/* Fills the 8 rows of a w-pixel wide cell (w 8 or 16), left-aligned: bit 15
 * of a row is the leftmost pixel, a set bit is the foreground. The glyph
 * kind is any but VT_GLYPH_FONT (those are drawn from the font). */
void GlyphRaster_Draw(vt_glyph g, int w, int h, UWORD rows[]);

#endif /* GLYPHRASTER_H */
