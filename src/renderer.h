/* src/renderer.h -- which renderer gets the terminal's bytes, and the
 * Renderer cycle's order. Pure, unit-tested on the host.
 *
 * prefs.State holds one APP_RENDERER_* bit (prefs.h). ConWrite (DCTelnet.c)
 * sends the bytes where Renderer_Route says: only the built-in renderer
 * parses them with vtcon (VtWrite); console.device and ibmcon.device get them
 * through CMD_WRITE, the XEM library through XemWrite. The Renderer cycle in
 * the site editor (guis.c) lists the four in bit order. */
#ifndef RENDERER_H
#define RENDERER_H

#include <exec/types.h>
#include "prefs.h"

#define RENDERER_FIRST_BIT 9    /* APP_RENDERER_BUILTIN */
#define RENDERER_COUNT     4

/* The cycle index is the bit's offset from the first renderer bit. */
typedef char RendererBitsInOrder[(APP_RENDERER_BUILTIN == 1UL << RENDERER_FIRST_BIT
                                  && APP_RENDERER_CONSOLE_DEVICE == 1UL << (RENDERER_FIRST_BIT + 1)
                                  && APP_RENDERER_XEM_LIB == 1UL << (RENDERER_FIRST_BIT + 2)
                                  && APP_RENDERER_IBMCON_DEVICE == 1UL << (RENDERER_FIRST_BIT + 3)) ? 1 : -1];

enum RendererRoute
{
    ROUTE_NONE,         /* iconified: the bytes are not drawn */
    ROUTE_BUILTIN,      /* VtWrite */
    ROUTE_CONSOLE,      /* CMD_WRITE to console.device, colours 1 and 7 swapped */
    ROUTE_IBMCON,       /* CMD_WRITE to ibmcon.device */
    ROUTE_XEM           /* XemWrite; also a State with no renderer bit */
};

/* Where ConWrite sends the bytes for this State. */
enum RendererRoute Renderer_Route(ULONG state);

/* The Renderer cycle position of a State (its renderer bit); 0 without one. */
UWORD Renderer_Index(ULONG state);

/* The State with its renderer replaced by the one at cycle position index. */
ULONG Renderer_Select(ULONG state, UWORD index);

/* The renderer a fresh prefs file starts on. A 68000/68010 (no 68020 in
 * ExecBase->AttnFlags) gets ibmcon.device, the lighter engine; a 68020+ gets
 * the built-in renderer. Saved prefs never go through this. */
ULONG Renderer_DefaultForCpu(BOOL has68020);

#endif
