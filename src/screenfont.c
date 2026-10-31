/* src/screenfont.c -- the terminal and screen font on square-pixel modes. */
#include "screenfont.h"

static BOOL same_name(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
    {
        char x = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char y = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
        if (x != y) return FALSE;
    }
    return *a == *b;
}

void ScreenFont_ForMode(const char *fontname, UWORD fontsize,
                        UWORD resolutionX, UWORD resolutionY, struct ScreenFontChoice *out)
{
    BOOL topaz = same_name(fontname, "topaz.font");
    BOOL pro   = same_name(fontname, TOPAZ_PRO_NAME);
    BOOL known = resolutionX != 0 && resolutionY != 0;

    out->name = fontname;
    out->size = fontsize;
    out->topazPro = pro;
    if (!known || !(topaz || pro))
        return;
    if (resolutionX == resolutionY)
    {
        out->name = TOPAZ_PRO_NAME;
        out->size = TOPAZ_PRO_SIZE;
        out->topazPro = TRUE;
    }
    else if (pro)
    {
        out->name = "topaz.font";
        out->size = 8;
        out->topazPro = FALSE;
    }
}

void ScreenFont_Grid(UWORD width, UWORD height, UWORD gzzWidth, UWORD gzzHeight, BOOL gzz,
                     UWORD cellX, UWORD cellY, UWORD *cols, UWORD *rows)
{
    UWORD w = gzz ? gzzWidth : width;
    UWORD h = gzz ? gzzHeight : height;

    *cols = cellX ? (UWORD)(w / cellX) : 1;
    *rows = cellY ? (UWORD)(h / cellY) : 1;
    if (*cols == 0) *cols = 1;
    if (*rows == 0) *rows = 1;
    if (*cols > SCREENFONT_MAX_COLS) *cols = SCREENFONT_MAX_COLS;
}

UWORD ScreenFont_MaxWindowWidth(UWORD columns, UWORD cellX, UWORD borders)
{
    return (UWORD)(columns * cellX + borders);
}

BOOL ScreenFont_TallPixels(UWORD resolutionX, UWORD resolutionY)
{
    return resolutionX != 0 && resolutionY > resolutionX;
}
