/* tests/test_vtinput.c -- MA3: bracketed paste, title text and the mouse
 * move flag, as pure functions (the wiring is in test_term_engine.c). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vtinput.h"

static vt_term *term_with(const char *init)
{
    vt_term *t = vt_new(80, 25, 0, NULL, NULL);
    vt_set_personality(t, VT_XTERM);
    vt_write(t, (const vt_u8 *)init, (long)strlen(init));
    return t;
}

static void test_ma3_7_bracketed_paste_wraps_exactly(void)
{
    char buf[64];
    size_t n;
    vt_term *on = term_with("\x1b[?2004h"), *off = term_with("");

    strcpy(buf, "abc");
    n = Vt_PasteBracket(on, buf, 3, 0);
    assert(n == 3 + 12 && memcmp(buf, "\x1b[200~abc\x1b[201~", 15) == 0);
    strcpy(buf, "abc");
    n = Vt_PasteBracket(off, buf, 3, 0);
    assert(n == 3 && memcmp(buf, "abc", 3) == 0);
    vt_free(on);
    vt_free(off);
}

static void test_ma3_8_paste_cannot_inject_the_end_marker(void)
{
    char buf[64];
    size_t n;
    vt_term *on = term_with("\x1b[?2004h");

    strcpy(buf, "a\x1b[201~rm -rf");
    n = Vt_PasteBracket(on, buf, 13, 0);
    assert(n == 24 && memcmp(buf, "\x1b[200~a[201~rm -rf\x1b[201~", n) == 0);
    assert(memchr(buf + 6, 0x1B, n - 12) == NULL);

    /* 8-bit CSI: Latin-1 0x9B, or C2 9B once the paste is converted to UTF-8 */
    strcpy(buf, "a\x9b" "201~rm");
    n = Vt_PasteBracket(on, buf, 8, 0);
    assert(n == 6 + 7 + 6 && memcmp(buf, "\x1b[200~a201~rm\x1b[201~", n) == 0);
    strcpy(buf, "a\xc2\x9b" "201~rm");
    n = Vt_PasteBracket(on, buf, 9, 1);
    assert(n == 6 + 7 + 6 && memcmp(buf, "\x1b[200~a201~rm\x1b[201~", n) == 0);
    strcpy(buf, "\xe2\x80\x9b" "x");                      /* U+201B keeps its 9B */
    n = Vt_PasteBracket(on, buf, 4, 1);
    assert(n == 6 + 4 + 6 && memcmp(buf + 6, "\xe2\x80\x9b" "x", 4) == 0);
    vt_free(on);
}

static void test_ma3_title_text_latin1_and_fallback(void)
{
    char out[32];

    Vt_TitleText("caf\xC3\xA9", "own", out, sizeof(out));
    assert(strcmp(out, "caf\xE9") == 0);
    Vt_TitleText("", "own", out, sizeof(out));
    assert(strcmp(out, "own") == 0);
    Vt_TitleText("a\x01" "b\xE2\x82\xAC", "own", out, sizeof(out));    /* control dropped, euro -> ? */
    assert(strcmp(out, "ab?") == 0);
    Vt_TitleText("0123456789", "own", out, 5);
    assert(strcmp(out, "0123") == 0);
}

/* Own screen: the window is a backdrop with no title bar, so an OSC title that
 * only reached the window title never showed (LIVE A1200, 2026-10-30). */
static void test_ma3_title_reaches_screen_bar_on_own_screen(void)
{
    const ULONG backdrop = 0x100;

    assert(Vt_ScreenTitleFor(backdrop | 0x1000, backdrop, "t") != NULL);
    assert(strcmp(Vt_ScreenTitleFor(backdrop, backdrop, "t"), "t") == 0);
    assert(Vt_ScreenTitleFor(0x1000, backdrop, "t") == NULL);       /* Workbench window keeps the screen title */
}

static void test_ma3_6_idcmp_move_only_for_motion_modes(void)
{
    vt_term *t = term_with("\x1b[?1000h");

    assert(Vt_IdcmpFor(0x10, 0x10, t, FALSE) == 0);
    assert(Vt_IdcmpFor(0x10, 0x10, t, TRUE) == 0x10);
    vt_write(t, (const vt_u8 *)"\x1b[?1003h", 8);
    assert(Vt_IdcmpFor(0x01, 0x10, t, FALSE) == 0x11);
    vt_free(t);
}

static void test_ma3_wheel_reports_press_only_in_mouse_modes(void)
{
    struct VtMouse m;
    UBYTE out[32];
    vt_term *t = term_with("\x1b[?1000h\x1b[?1006h");
    int n;

    assert(Vt_WheelButton(0x7A) == 64);
    assert(Vt_WheelButton(0x7B) == 65);
    assert(Vt_WheelButton(0x7C) == -1 && Vt_WheelButton(0x79) == -1 && Vt_WheelButton(0xFA) == -1);

    Vt_MouseReset(&m);
    n = Vt_MouseButton(&m, t, Vt_WheelButton(0x7A), TRUE, 4, 2, 0, out);
    assert(n == (int)strlen("\x1b[<64;5;3M") && memcmp(out, "\x1b[<64;5;3M", (size_t)n) == 0);
    n = Vt_MouseButton(&m, t, Vt_WheelButton(0x7B), TRUE, 4, 2, 0, out);
    assert(n == (int)strlen("\x1b[<65;5;3M") && memcmp(out, "\x1b[<65;5;3M", (size_t)n) == 0);
    assert(m.held == 0);                        /* the wheel is never a held button */
    vt_free(t);

    t = term_with("\x1b[?1000h");              /* legacy encoding: ESC [ M, button + 32 */
    n = Vt_MouseButton(&m, t, 64, TRUE, 0, 0, 0, out);
    assert(n == 6 && out[3] == 32 + 64);
    vt_free(t);

    t = term_with("");                          /* no mouse mode: nothing, and not owned */
    assert(Vt_MouseOwned(t, FALSE) == FALSE);
    assert(Vt_MouseButton(&m, t, 64, TRUE, 0, 0, 0, out) == 0);
    vt_free(t);
}

int main(void)
{
    test_ma3_7_bracketed_paste_wraps_exactly();
    test_ma3_8_paste_cannot_inject_the_end_marker();
    test_ma3_title_text_latin1_and_fallback();
    test_ma3_title_reaches_screen_bar_on_own_screen();
    test_ma3_6_idcmp_move_only_for_motion_modes();
    test_ma3_wheel_reports_press_only_in_mouse_modes();
    printf("vtinput: all assertions passed\n");
    return 0;
}
