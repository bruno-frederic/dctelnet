/* src/nearest_pen.c -- the pen of a screen nearest to a colour. */
#include <exec/types.h>
#ifdef __VBCC__
    #pragma dontwarn 306
#endif
#include <proto/graphics.h>           // ObtainBestPen(), GetRGB32()
#ifdef __VBCC__
    #pragma popwarn
#endif
#include "nearest_pen.h"
#include "iconpens.h"                 // IconPens_Distance()

UBYTE ObtainNearestPen(struct ColorMap *cm, ULONG rgb, LONG precision, BOOL *owned)
{
    ULONG r = ((rgb >> 16) & 0xFF) * 0x01010101UL, g = ((rgb >> 8) & 0xFF) * 0x01010101UL,
          b = (rgb & 0xFF) * 0x01010101UL;
    LONG pen = ObtainBestPen(cm, r, g, b, OBP_Precision, precision, OBP_FailIfBad, TRUE, TAG_DONE);

    // OBP_FailIfBad: without it ObtainBestPen() answers with the closest
    // SHAREABLE pen however poor -- on a screen whose pens are all
    // exclusively held that is one of the Intuition pens 0-3, and the scan
    // below never runs. With it a match outside the precision is -1.
    // -1 cast to UBYTE was pen 255.
    *owned = pen >= 0;
    if (pen < 0)
    {
        // No free pen: the closest as the eye sees it. FindColor() measures
        // plain RGB, where a dark grey is nearer a blue than a grey.
        ULONG best = 0xFFFFFFFFUL, c[3];
        UWORD i;

        for (i = 0, pen = 0; i < cm->Count; i++)
        {
            ULONG d;

            GetRGB32(cm, i, 1, c);
            d = IconPens_Distance(rgb, ((c[0] >> 8) & 0xFF0000) | ((c[1] >> 16) & 0xFF00) | (c[2] >> 24));
            if (d < best)
            {
                best = d;
                pen = i;
            }
        }
    }
    return (UBYTE)pen;
}
