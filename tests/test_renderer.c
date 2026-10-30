/* tests/test_renderer.c -- which renderer gets the terminal's bytes, and the
 * Renderer cycle's order (renderer.h). The modern ANSI work (MA1-MA3) changes
 * the built-in path only: the other renderers must keep their bytes. */
#include <assert.h>
#include <stdio.h>
#include "renderer.h"

static const ULONG bits[RENDERER_COUNT] = {
    APP_RENDERER_BUILTIN, APP_RENDERER_CONSOLE_DEVICE, APP_RENDERER_XEM_LIB, APP_RENDERER_IBMCON_DEVICE
};

static void test_only_the_builtin_renderer_reaches_vtcon(void)
{
    ULONG other = APP_PETSCII_MODE | APP_RAW_CONNECTION;

    assert(Renderer_Route(APP_RENDERER_BUILTIN | other) == ROUTE_BUILTIN);
    assert(Renderer_Route(APP_RENDERER_CONSOLE_DEVICE | other) == ROUTE_CONSOLE);
    assert(Renderer_Route(APP_RENDERER_IBMCON_DEVICE | other) == ROUTE_IBMCON);
    assert(Renderer_Route(APP_RENDERER_XEM_LIB | other) == ROUTE_XEM);
}

static void test_fresh_prefs_renderer_follows_the_cpu(void)
{
    /* 68000/68010: no AFF_68020 -> ibmcon.device; 68020+ -> built-in. */
    assert(Renderer_DefaultForCpu(FALSE) == APP_RENDERER_IBMCON_DEVICE);
    assert(Renderer_DefaultForCpu(TRUE) == APP_RENDERER_BUILTIN);
}

static void test_iconified_draws_nothing(void)
{
    UWORD i;

    for (i = 0; i < RENDERER_COUNT; i++)
        assert(Renderer_Route(bits[i] | APP_ICONIFIED) == ROUTE_NONE);
}

static void test_the_cycle_visits_all_four_in_bit_order(void)
{
    ULONG state = APP_PETSCII_MODE | APP_RENDERER_XEM_LIB;
    UWORD i;

    for (i = 0; i < RENDERER_COUNT; i++)
    {
        state = Renderer_Select(state, i);
        assert((state & APP_RENDERER_ALL) == bits[i]);
        assert(Renderer_Index(state) == i);
        assert(state & APP_PETSCII_MODE);
    }
    assert(Renderer_Index(APP_PETSCII_MODE) == 0);
}

int main(void)
{
    test_only_the_builtin_renderer_reaches_vtcon();
    test_fresh_prefs_renderer_follows_the_cpu();
    test_iconified_draws_nothing();
    test_the_cycle_visits_all_four_in_bit_order();
    printf("test_renderer: all passed\n");
    return 0;
}
