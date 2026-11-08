/* tests/test_palette.c -- ANSI colours and pen layout for deep screens. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "palette.h"

/* The device palette is RGB4 in ibmcon's unit-1 order (pen 1 white, pen 7
 * red, 9/15 swapped likewise); the ANSI pens of a deep screen want ANSI order
 * (1 red, 7 white) at 8 bits a gun. */
static void test_ansi_colours_come_from_the_device_palette_in_ansi_order(void) {
    UWORD dev[16];

    memset(dev, 0, sizeof(dev));
    dev[1] = 0xDDD;   /* ibmcon 1 = white */
    dev[7] = 0xD00;   /* ibmcon 7 = red */
    dev[9] = 0xFFF;   /* ibmcon 9 = bright white */
    dev[15] = 0xF00;  /* ibmcon 15 = bright red */
    dev[2] = 0x0A0;
    assert(Palette_AnsiColour(dev, 1) == 0xDD0000);    /* ANSI 1 red   */
    assert(Palette_AnsiColour(dev, 7) == 0xDDDDDD);    /* ANSI 7 white */
    assert(Palette_AnsiColour(dev, 9) == 0xFF0000);
    assert(Palette_AnsiColour(dev, 15) == 0xFFFFFF);
    assert(Palette_AnsiColour(dev, 2) == 0x00AA00);
}

/* Where the 16 ANSI colours live: shared pens 0-15 below 32 colours (no
 * table), otherwise pens of their own that skip the UI pens 0-7 and the AGA
 * pointer pens 16-19. */
static void test_ansi_pen_layout(void) {
    UBYTE pens[16];
    int i;

    assert(!Palette_AnsiPens(4, pens));
    assert(Palette_AnsiPens(5, pens));
    for (i = 0; i < 8; i++)  assert(pens[i] == 8 + i);
    for (i = 8; i < 16; i++) assert(pens[i] == 20 + (i - 8));
    assert(Palette_AnsiPens(8, pens) && pens[15] == 27);
    assert(Palette_AnsiPens(24, pens) && pens[0] == 8);   /* RTG true colour */
}

/* The editor works in ANSI order whatever the renderer keeps: the built-in
 * renderer's AnsiColors as they are, a console's DeviceColors swapped. */
static void test_the_editor_sees_the_renderers_palette_in_ansi_order(void) {
    struct PrefsStruct p;
    UWORD pal[16];

    memset(&p, 0, sizeof(p));
    p.State = APP_RENDERER_IBMCON_DEVICE;
    memcpy(p.DeviceColors, defaultDeviceColors, sizeof(p.DeviceColors));
    Palette_Get(&p, pal);
    assert(pal[1] == 0x0D00 && pal[7] == 0x0DDD);        /* ANSI red, ANSI white */
    pal[1] = 0x0E00;
    Palette_Put(&p, pal);
    assert(p.DeviceColors[7] == 0x0E00);                  /* ibmcon's red slot */
    Palette_DefaultFor(&p, pal);
    assert(pal[1] == 0x0D00);

    p.State = APP_RENDERER_BUILTIN;
    memcpy(p.AnsiColors, defaultAnsiColors, sizeof(p.AnsiColors));
    Palette_Get(&p, pal);
    assert(pal[1] == 0x0A00 && pal[7] == 0x0AAA);
    Palette_DefaultFor(&p, pal);
    assert(pal[15] == 0x0FFF);
}

static void test_channels_are_the_four_bits_the_prefs_keep(void) {
    assert(Palette_Channel(0x0A5F, 0) == 0xA && Palette_Channel(0x0A5F, 1) == 5 && Palette_Channel(0x0A5F, 2) == 0xF);
    assert(Palette_WithChannel(0x0A5F, 1, 0xC) == 0x0ACF);
    assert(Palette_RGB32(0x0F80) == 0xFF8800);
    assert(!strcmp(Palette_Name(9), "Bright Red"));
}

/* Recolouring a true-colour terminal's pixels merges two colours for good
 * when another ANSI colour has the same value: then it is not done. */
static void test_a_recolour_that_would_merge_colours_is_refused(void) {
    ULONG shown[16];
    int i;

    for (i = 0; i < 16; i++) shown[i] = (ULONG)i * 0x111111;
    assert(Palette_SafeRecolour(shown, 1, 0xABCDEF));
    assert(!Palette_SafeRecolour(shown, 1, shown[2]));     /* would become colour 2 */
    shown[3] = shown[1];
    assert(!Palette_SafeRecolour(shown, 1, 0xABCDEF));     /* colour 3 shares its pixels */
}

/* Issue #49: on AmigaOS 2.04 the menu items were drawn in pen 1, the pen
 * Intuition fills the menus with before V39 -- text invisible. Before V39 the
 * text keeps GadTools' default pen 0; from V39 on it is BARDETAILPEN. */
static void test_menu_text_is_legible_on_os2_and_os3(void) {
    UWORD pens[12];
    int i;

    for (i = 0; i < 12; i++) pens[i] = (UWORD)(100 + i);
    pens[1] = 1;   /* BLOCKPEN: the menu fill before V39 */
    assert(Palette_MenuTextPen(1, pens) == 0);      /* OS 2.04 (dri_Version 1) */
    assert(Palette_MenuTextPen(1, pens) != pens[1]);
    assert(Palette_MenuTextPen(0, NULL) == 0);      /* no DrawInfo */
    assert(Palette_MenuTextPen(2, pens) == 109);    /* OS 3.x: BARDETAILPEN (9) */
}

int main(void) {
    test_menu_text_is_legible_on_os2_and_os3();
    test_the_editor_sees_the_renderers_palette_in_ansi_order();
    test_channels_are_the_four_bits_the_prefs_keep();
    test_a_recolour_that_would_merge_colours_is_refused();
    test_ansi_colours_come_from_the_device_palette_in_ansi_order();
    test_ansi_pen_layout();
    printf("palette: all assertions passed\n");
    return 0;
}
