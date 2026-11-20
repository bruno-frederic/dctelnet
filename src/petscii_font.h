/* src/petscii_font.h -- the C64 fonts (Petscii, PetsciiLower) built from
 * the glyph data compiled into DCTelnet.
 *
 * A PETSCII session on ibmcon.device, console.device or the built-in
 * renderer draws with the real C64 glyphs. They came from Petscii.font on
 * disk, and when that file was not in FONTS: or PROGDIR:Fonts/ (a bare
 * binary, a hand-made install) the session showed CP437 lookalikes. The
 * glyphs now live in the program (src/petscii_glyphs.c, made from
 * assets/glyphs by tools/gen_petscii_glyphs.sh), and this module lays them out as an Amiga strike
 * font, the same layout tools/gen_petscii_font.py writes to the disk fonts:
 * 256 glyphs indexed by raw PETSCII byte, each 8x8 C64 glyph doubled
 * horizontally to a 16x8 cell (hires pixels are half as wide as a C64's). */
#ifndef PETSCII_FONT_H
#define PETSCII_FONT_H

#include <exec/types.h>

#define PETSCII_FONT_XSIZE     16
#define PETSCII_FONT_YSIZE     8
#define PETSCII_FONT_BASELINE  (PETSCII_FONT_YSIZE - 1)
#define PETSCII_FONT_GLYPHS    256
#define PETSCII_FONT_BLOB_SIZE (PETSCII_FONT_GLYPHS * 8)                        /* 8x8 source, glyph-major */
#define PETSCII_FONT_MODULO    (PETSCII_FONT_GLYPHS * PETSCII_FONT_XSIZE / 8)   /* bytes per strike row: 512 */
#define PETSCII_FONT_DATA_SIZE (PETSCII_FONT_YSIZE * PETSCII_FONT_MODULO)       /* tf_CharData: 4096 */
#define PETSCII_FONT_LOC_COUNT (PETSCII_FONT_GLYPHS + 1)                        /* tf_CharLoc, with the default glyph */

/* The C64 glyphs, PETSCII_FONT_BLOB_SIZE bytes each (src/petscii_glyphs.c). */
extern const unsigned char petscii_glyphs_upper[];
extern const unsigned char petscii_glyphs_lower[];

/* One 8-pixel row doubled to 16 pixels: bit N becomes bits 2N and 2N+1. */
UWORD petscii_font_double_row(UBYTE row);

/* Lay the PETSCII_FONT_BLOB_SIZE bytes of glyphs (glyph-major: glyph g's
 * row r at g * 8 + r) out as the strike bitmap tf_CharData points at:
 * PETSCII_FONT_YSIZE rows of PETSCII_FONT_MODULO bytes. */
void petscii_font_strike(const UBYTE *glyphs, UBYTE *chardata);

/* The tf_CharLoc table: per glyph its bit offset in a strike row (high
 * word) and width (low word); the last entry, drawn for characters outside
 * LoChar..HiChar, is the space. */
void petscii_font_charloc(ULONG *charloc);

#endif
