/* tests/shim/proto/graphics.h -- the few graphics.library calls ObtainNearestPen
 * makes, declared for host tests. The test supplies the definitions. */
#ifndef PROTO_GRAPHICS_H
#define PROTO_GRAPHICS_H
#include <exec/types.h>

struct ColorMap
{
    UWORD Count;
};

#define TAG_DONE        0UL
#define OBP_Precision   1UL
#define OBP_FailIfBad   2UL

LONG ObtainBestPen(struct ColorMap *cm, ULONG r, ULONG g, ULONG b, ...);
void GetRGB32(struct ColorMap *cm, ULONG first, ULONG count, ULONG *table);
#endif
