/* tests/test_menu_checks.c -- which Settings menu items are checked for a given prefs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "prefs.h"
#include "charset.h"
#include "menu_checks.h"

static struct PrefsStruct make(ULONG state) {
    struct PrefsStruct p;
    memset(&p, 0, sizeof(p));
    p.State = state;
    return p;
}

static BOOL checked(const struct PrefsStruct *p, enum MenuItemID id) {
    BOOL c = 2;
    assert(MenuChecks_Checked(p, id, &c));
    assert(c == TRUE || c == FALSE);
    return c;
}

/* The reported bug: global CP437, the entry has UTF-8; the live menu must
 * follow the prefs the entry session applied. */
static void test_an_entry_with_utf8_shows_utf8_checked_not_cp437(void) {
    struct PrefsStruct p = make(APP_RENDERER_IBMCON_DEVICE);
    p.Charset = CHARSET_CP437;
    assert(checked(&p, MENU_CHARSET_CP437) && !checked(&p, MENU_CHARSET_LATIN1) && !checked(&p, MENU_CHARSET_UTF8));
    p.Charset = CHARSET_UTF8;
    assert(!checked(&p, MENU_CHARSET_CP437) && !checked(&p, MENU_CHARSET_LATIN1) && checked(&p, MENU_CHARSET_UTF8));
    p.Charset = CHARSET_LATIN1;
    assert(!checked(&p, MENU_CHARSET_CP437) && checked(&p, MENU_CHARSET_LATIN1) && !checked(&p, MENU_CHARSET_UTF8));
}

static void test_renderer_check_follows_prefs_with_builtin_first(void) {
    struct PrefsStruct p = make(APP_RENDERER_BUILTIN);
    assert(checked(&p, MENU_BUILTIN_RENDERER) && !checked(&p, MENU_CONSOLE_DEVICE)
           && !checked(&p, MENU_XEM_LIBRARY) && !checked(&p, MENU_IBMCON_DEVICE));
    p = make(APP_RENDERER_CONSOLE_DEVICE);
    assert(!checked(&p, MENU_BUILTIN_RENDERER) && checked(&p, MENU_CONSOLE_DEVICE)
           && !checked(&p, MENU_XEM_LIBRARY) && !checked(&p, MENU_IBMCON_DEVICE));
    p = make(APP_RENDERER_XEM_LIB);
    assert(!checked(&p, MENU_CONSOLE_DEVICE) && checked(&p, MENU_XEM_LIBRARY) && !checked(&p, MENU_IBMCON_DEVICE));
    p = make(APP_RENDERER_IBMCON_DEVICE);
    assert(!checked(&p, MENU_XEM_LIBRARY) && checked(&p, MENU_IBMCON_DEVICE));
    /* several bits: the same precedence as the menu build, builtin first */
    p = make(APP_RENDERER_BUILTIN | APP_RENDERER_XEM_LIB | APP_RENDERER_IBMCON_DEVICE);
    assert(checked(&p, MENU_BUILTIN_RENDERER) && !checked(&p, MENU_XEM_LIBRARY) && !checked(&p, MENU_IBMCON_DEVICE));
    p = make(APP_RENDERER_XEM_LIB | APP_RENDERER_IBMCON_DEVICE);
    assert(checked(&p, MENU_XEM_LIBRARY) && !checked(&p, MENU_IBMCON_DEVICE));
    /* none set: ibmcon is the else branch */
    p = make(0);
    assert(checked(&p, MENU_IBMCON_DEVICE) && !checked(&p, MENU_BUILTIN_RENDERER));
}

static void test_132_columns_is_checked_only_with_ibmcon(void) {
    struct PrefsStruct p = make(APP_RENDERER_IBMCON_DEVICE | APP_132_COLUMNS);
    assert(checked(&p, MENU_132_COLUMNS));
    p = make(APP_RENDERER_IBMCON_DEVICE);
    assert(!checked(&p, MENU_132_COLUMNS));
    p = make(APP_RENDERER_BUILTIN | APP_132_COLUMNS);
    assert(!checked(&p, MENU_132_COLUMNS));
}

static void test_capture_is_not_a_prefs_check(void) {
    struct PrefsStruct p = make(0xFFFFFFFFUL);
    BOOL c = 2;
    assert(!MenuChecks_Checked(&p, MENU_CAPTURE, &c));
    assert(c == 2);
    assert(!MenuChecks_Checked(&p, MENU_ABOUT, &c));
    assert(!MenuChecks_Checked(&p, MENU_CHARSET, &c));    /* a parent, not a choice */
    assert(!MenuChecks_Checked(&p, MENU_BELL, &c));
    assert(!MenuChecks_Checked(&p, MENU_QUIT, &c));
    assert(c == 2);
}

static void test_toggles_follow_their_state_bits(void) {
    static const struct { enum MenuItemID id; ULONG flag; } t[] = {
        { MENU_FULLSCREEN, APP_FULLSCREEN },
        { MENU_LEDS, APP_LEDS_ENABLED },
        { MENU_TITLE_BAR, APP_TITLE_BAR_ENABLED },
        { MENU_BACKSPACE_DEL_SWAP, APP_BACKSPACE_DEL_SWAPPED },
        { MENU_SCROLLBACK, APP_SCROLLBACK_ENABLED },
        { MENU_PACKET_WINDOW, APP_PACKET_WINDOW_ENABLED },
        { MENU_TOOL_BAR, APP_TOOL_BAR_ENABLED },
        { MENU_RETURN_SENDING_CRLF, APP_RETURN_SENDING_CRLF },
        { MENU_VT_KEYS, APP_VT_KEYS },
        { MENU_LOCAL_ECHO, APP_LOCAL_ECHO },
        { MENU_RAW_CONNECTION, APP_RAW_CONNECTION },
        { MENU_RLOGIN, APP_RLOGIN },
        { MENU_FAST_SCROLL, APP_FAST_SCROLL_ENABLED },
        { MENU_PETSCII_MODE, APP_PETSCII_MODE },
    };
    size_t i, j;
    for (i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
        struct PrefsStruct on = make(t[i].flag), off = make((ULONG)~t[i].flag);
        assert(checked(&on, t[i].id));
        assert(!checked(&off, t[i].id));
        /* no other toggle moves with this bit */
        for (j = 0; j < sizeof(t) / sizeof(t[0]); j++)
            if (j != i && (t[j].flag != t[i].flag)) assert(!checked(&on, t[j].id));
    }
}

static void test_bell_and_ansi_music_follow_prefs(void) {
    struct PrefsStruct p = make(0);
    p.Bell = BELL_FLASH;
    assert(checked(&p, MENU_BELL_FLASH) && !checked(&p, MENU_BELL_SOUND) && !checked(&p, MENU_BELL_OFF));
    p.Bell = BELL_SOUND;
    assert(!checked(&p, MENU_BELL_FLASH) && checked(&p, MENU_BELL_SOUND) && !checked(&p, MENU_BELL_OFF));
    p.Bell = BELL_OFF;
    assert(!checked(&p, MENU_BELL_FLASH) && !checked(&p, MENU_BELL_SOUND) && checked(&p, MENU_BELL_OFF));
    p.AnsiMusic = 0;
    assert(!checked(&p, MENU_ANSI_MUSIC));
    p.AnsiMusic = 1;
    assert(checked(&p, MENU_ANSI_MUSIC));
}

int main(void) {
    test_an_entry_with_utf8_shows_utf8_checked_not_cp437();
    test_renderer_check_follows_prefs_with_builtin_first();
    test_132_columns_is_checked_only_with_ibmcon();
    test_capture_is_not_a_prefs_check();
    test_toggles_follow_their_state_bits();
    test_bell_and_ansi_music_follow_prefs();
    puts("test_menu_checks: all passed");
    return 0;
}
