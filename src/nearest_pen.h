#ifndef NEAREST_PEN_H
#define NEAREST_PEN_H

#include <exec/types.h>

struct ColorMap;
/* V39: a shared pen of colour rgb (0x00RRGGBB) from cm, matched to precision
 * (PRECISION_EXACT, PRECISION_IMAGE ...). Without a pen to share (-1), the
 * nearest colour already there, which is not ours: *owned says whether to
 * ReleasePen() it. */
UBYTE ObtainNearestPen(struct ColorMap *cm, ULONG rgb, LONG precision, BOOL *owned);

#endif /* NEAREST_PEN_H */
