/* src/renderer.c -- which renderer gets the terminal's bytes (renderer.h). */
#include "renderer.h"

enum RendererRoute Renderer_Route(ULONG state)
{
    if (state & APP_ICONIFIED) return ROUTE_NONE;
    if (state & APP_RENDERER_BUILTIN) return ROUTE_BUILTIN;
    if (state & APP_RENDERER_CONSOLE_DEVICE) return ROUTE_CONSOLE;
    if (state & APP_RENDERER_IBMCON_DEVICE) return ROUTE_IBMCON;
    return ROUTE_XEM;
}

UWORD Renderer_Index(ULONG state)
{
    UWORD i;

    for (i = 0; i < RENDERER_COUNT; i++)
        if (state & (1UL << (RENDERER_FIRST_BIT + i))) return i;
    return 0;
}

ULONG Renderer_Select(ULONG state, UWORD index)
{
    return (state & ~APP_RENDERER_ALL) | (1UL << (RENDERER_FIRST_BIT + (index % RENDERER_COUNT)));
}
