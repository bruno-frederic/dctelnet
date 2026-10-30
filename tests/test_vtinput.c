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
    n = Vt_PasteBracket(on, buf, 3);
    assert(n == 3 + 12 && memcmp(buf, "\x1b[200~abc\x1b[201~", 15) == 0);
    strcpy(buf, "abc");
    n = Vt_PasteBracket(off, buf, 3);
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
    n = Vt_PasteBracket(on, buf, 13);
    assert(n == 24 && memcmp(buf, "\x1b[200~a[201~rm -rf\x1b[201~", n) == 0);
    assert(memchr(buf + 6, 0x1B, n - 12) == NULL);
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

static void test_ma3_6_idcmp_move_only_for_motion_modes(void)
{
    vt_term *t = term_with("\x1b[?1000h");

    assert(Vt_IdcmpFor(0x10, 0x10, t, FALSE) == 0);
    assert(Vt_IdcmpFor(0x10, 0x10, t, TRUE) == 0x10);
    vt_write(t, (const vt_u8 *)"\x1b[?1003h", 8);
    assert(Vt_IdcmpFor(0x01, 0x10, t, FALSE) == 0x11);
    vt_free(t);
}

int main(void)
{
    test_ma3_7_bracketed_paste_wraps_exactly();
    test_ma3_8_paste_cannot_inject_the_end_marker();
    test_ma3_title_text_latin1_and_fallback();
    test_ma3_6_idcmp_move_only_for_motion_modes();
    printf("vtinput: all assertions passed\n");
    return 0;
}
