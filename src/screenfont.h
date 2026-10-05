/* src/screenfont.h -- the terminal and screen font on square-pixel modes. */
#ifndef SCREENFONT_H
#define SCREENFONT_H

#include <exec/types.h>

/* Topaz was drawn for the Amiga's tall non-square pixels; Topaz Pro is
 * topaz redrawn for square pixels (8x16 for topaz 8). The two are one face:
 * on a mode with square pixels (resolution ticks x == y: every RTG mode,
 * AGA hires interlaced) topaz or Topaz Pro opens as Topaz Pro 16, on any
 * other mode as topaz -- Topaz Pro as topaz 8. Every other font opens as
 * chosen. resolutionX/Y are DisplayInfo.Resolution (0 when unknown: the
 * font as chosen). */
struct ScreenFontChoice
{
    const char *name;
    UWORD       size;
    BOOL        topazPro;       /* the bundled Topaz Pro */
};
void ScreenFont_ForMode(const char *fontname, UWORD fontsize,
                        UWORD resolutionX, UWORD resolutionY, struct ScreenFontChoice *out);

/* The text grid a terminal window holds, as ibmcon.device measures it
 * (MeasureGrid): the drawable area -- the inner area of a GimmeZeroZero
 * window, which the Workbench window is -- divided by the font cell, at
 * least 1 x 1 and at most SCREENFONT_MAX_COLS columns. Pass the window's
 * Width/Height, GZZWidth/GZZHeight and whether WFLG_GIMMEZEROZERO is set. */
#define SCREENFONT_MAX_COLS 199
void ScreenFont_Grid(UWORD width, UWORD height, UWORD gzzWidth, UWORD gzzHeight, BOOL gzz,
                     UWORD cellX, UWORD cellY, UWORD *cols, UWORD *rows);

/* BBS art is drawn for a fixed width -- 80 columns of ANSI, 40 of PETSCII
 * -- and relies on the terminal wrapping at its edge; a wider terminal
 * breaks it. The widest a window with these borders may be. */
#define SCREENFONT_ANSI_COLUMNS    80
#define SCREENFONT_PETSCII_COLUMNS 40
#define SCREENFONT_BBS_ROWS        25   /* the Workbench window opens 80x25 (40x25) */
UWORD ScreenFont_MaxWindowWidth(UWORD columns, UWORD cellX, UWORD borders);

/* TRUE on a mode whose pixels are taller than wide (hires or super-hires
 * without interlace: resolution ticks y > x). Art drawn for square pixels
 * looks squeezed there; the tool bar takes its wide icons. */
BOOL ScreenFont_TallPixels(UWORD resolutionX, UWORD resolutionY);

#define TOPAZ_PRO_NAME "TopazPro.font"
#define TOPAZ_PRO_SIZE 16

#endif /* SCREENFONT_H */
