/* tests/test_site_prefs.c -- per-Address-Book-entry settings (issue #10). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "prefs.h"
#include "site_prefs.h"

static struct PrefsStruct make(ULONG flags, const char *font, UWORD size) {
    struct PrefsStruct p;
    memset(&p, 0, sizeof(p));
    p.State = flags;
    strcpy((char *)p.FontName, font);
    p.FontSize = size;
    p.DisplayID = 0x8000;
    p.DisplayWidth = 640; p.DisplayHeight = 256; p.DisplayDepth = 4;
    p.MainWinLeftEdge = 10; p.MainWinTopEdge = 20; p.MainWinWidth = 300; p.MainWinHeight = 200;
    strcpy((char *)p.TelnetTermType, "VT102");
    return p;
}

static struct SiteSettings entry_with(ULONG groups, struct PrefsStruct p) {
    struct SiteSettings e;
    memset(&e, 0, sizeof(e));
    e.groups = groups;
    e.prefs = p;
    return e;
}

/* An overridden group's members come from the entry; everything else --
 * other groups AND the app-wide settings (G3) -- from the global settings;
 * window geometry stays where the windows are. */
static void test_apply_takes_only_overridden_groups(void) {
    struct PrefsStruct global = make(APP_BACKSPACE_DEL_SWAPPED, "IBM.font", 8), live = global;
    struct PrefsStruct e = make(APP_PETSCII_MODE | APP_LEDS_ENABLED, "Petscii.font", 8);
    struct SiteSettings entry;

    strcpy((char *)e.TelnetTermType, "PETSCII");
    strcpy((char *)e.XferLibrary, "xprkermit.library");
    e.nScrollbackLines = 9999;                       /* app-wide: never from an entry */
    e.MainWinLeftEdge = 99;                         /* geometry: never from an entry */
    live.MainWinLeftEdge = 42;

    entry = entry_with(SITE_GROUP_SCREEN | SITE_GROUP_TERMINAL, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);

    assert(strcmp((char *)live.FontName, "Petscii.font") == 0);        /* Screen: entry */
    assert(live.State & APP_PETSCII_MODE);                     /* Terminal: entry */
    assert(strcmp((char *)live.TelnetTermType, "PETSCII") == 0);
    assert(live.State & APP_BACKSPACE_DEL_SWAPPED);                      /* Keyboard: global */
    assert(strcmp((char *)live.XferLibrary, "xprkermit.library") != 0); /* Transfer: global */
    assert(!(live.State & APP_LEDS_ENABLED));                     /* app-wide: global */
    assert(live.nScrollbackLines == global.nScrollbackLines);
    assert(live.MainWinLeftEdge == 42);
}

static void test_each_group_moves_only_its_members(void) {
    struct PrefsStruct global = make(0, "IBM.font", 8), live;
    struct PrefsStruct e = make(APP_RETURN_SENDING_CRLF | APP_FULLSCREEN | APP_RAW_CONNECTION,
                                "Topaz.font", 11);
    struct SiteSettings entry;

    strcpy((char *)e.XferOptions, "TN");
    e.DisplayDepth = 8;
    e.DisplayID = 0x29000;

    live = global; entry = entry_with(SITE_GROUP_KEYBOARD, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(live.State == APP_RETURN_SENDING_CRLF && live.DisplayDepth == 4);

    live = global; entry = entry_with(SITE_GROUP_TRANSFER, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(strcmp((char *)live.XferOptions, "TN") == 0 && live.State == 0);

    live = global; entry = entry_with(SITE_GROUP_SCREEN, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    /* Full-screen is app-wide: an entry saved in Workbench mode opened
     * the Workbench window on connect while the user ran DCTelnet on its
     * own 800x600 screen. */
    assert(live.State == 0 && live.FontSize == 11);
    /* The screen mode is app-wide too: the entry's 8-bit PAL mode switched
     * the user's 800x600 screen on connect. */
    assert(live.DisplayDepth == 4 && live.DisplayID == global.DisplayID);
    live = make(APP_FULLSCREEN, "IBM.font", 8);
    global = live;
    e.State = 0;
    entry = entry_with(SITE_GROUP_SCREEN, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(live.State == APP_FULLSCREEN);
    global = make(0, "IBM.font", 8);
    e.State = APP_RETURN_SENDING_CRLF | APP_FULLSCREEN | APP_RAW_CONNECTION;

    live = global; entry = entry_with(SITE_GROUP_TERMINAL, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(live.State == APP_RAW_CONNECTION && live.FontSize == 8);

    live = global; entry = entry_with(0, e);                    /* nothing overridden */
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(memcmp(&live, &global, sizeof(live)) == 0);
}

/* Disconnect: the global settings come back, the windows stay where the user
 * moved them during the session. */
static void test_restore_brings_back_global_but_keeps_moved_windows(void) {
    struct PrefsStruct global = make(0, "IBM.font", 8);
    struct PrefsStruct live = make(APP_PETSCII_MODE | APP_LOCAL_ECHO, "Petscii.font", 8);
    live.MainWinLeftEdge = 55;

    SitePrefs_Restore(&live, &global);
    assert(live.State == 0);
    assert(strcmp((char *)live.FontName, "IBM.font") == 0);
    assert(live.MainWinLeftEdge == 55);
}

/* The invariant: DCTelnet.Prefs only ever holds the global settings. During an
 * entry session a menu change or telnet ECHO negotiation lands in the live
 * settings and must not reach the file; outside a session the live settings
 * ARE the global ones (today's behaviour). */
static void test_save_writes_global_during_a_session(void) {
    struct PrefsStruct global = make(0, "IBM.font", 8);
    struct PrefsStruct live = make(APP_PETSCII_MODE | APP_LOCAL_ECHO, "Petscii.font", 8);
    struct PrefsStruct out;
    live.MainWinTopEdge = 77;

    SitePrefs_ForSave(&out, &live, &global, TRUE);
    assert(out.State == 0 && strcmp((char *)out.FontName, "IBM.font") == 0);
    assert(out.MainWinTopEdge == 77); /* geometry is global: the moved window is saved */

    SitePrefs_ForSave(&out, &live, &global, FALSE);
    assert(memcmp(&out, &live, sizeof(out)) == 0);
}

static void test_display_differs_names_what_needs_a_reopen(void) {
    struct PrefsStruct a = make(0, "IBM.font", 8), b;
    BOOL screen;

    b = a;
    assert(!SitePrefs_DisplayDiffers(&a, &b, &screen));

    b = a; b.State |= APP_BACKSPACE_DEL_SWAPPED;                 /* read live: no reopen */
    assert(!SitePrefs_DisplayDiffers(&a, &b, &screen));

    b = a; b.FontSize = 11;                              /* font: screen reopen */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && screen);

    b = a; b.State |= APP_PETSCII_MODE;   /* C64 display follows the connection, not the flag */
    assert(!SitePrefs_DisplayDiffers(&a, &b, &screen));

    b = a; b.DisplayDepth = 3;
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && screen);

    b = a; b.State |= APP_TOOL_BAR_ENABLED;                     /* windows only */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);

    b = a; b.DeviceColors[3] = 0x0F00;                          /* palette: windows */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);

    b = a; strcpy((char *)b.XemLibrary, "xemvt340.library");
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);
}

/* An entry's settings file: 'DCS2', the group mask, the settings, the
 * function keys and the login macro. A truncated, foreign or unknown file
 * is "no settings", never a half-read struct. */
static void test_sidecar_round_trip_and_rejects(void) {
    static struct SiteSettings in, out;
    static TEXT globalKeys[SITE_FKEY_BYTES];
    static UBYTE buf[SITE_FILE_SIZE_MAX + 8];
    size_t n;

    in = entry_with(SITE_GROUP_TERMINAL, make(APP_PETSCII_MODE, "Petscii.font", 8));
    strcpy((char *)in.fKeys, "F1 macro");
    strcpy(in.loginMacro, "\\u\\r\\p\\r");
    n = SitePrefs_Encode(&in, buf, sizeof(buf));
    assert(n == SITE_FILE_SIZE_MAX);
    assert(buf[0] == 'D' && buf[1] == 'C' && buf[2] == 'T' && buf[3] == 'S');
    memset(&out, 0xAA, sizeof(out));
    assert(SitePrefs_Decode(&out, buf, n, globalKeys));
    assert(memcmp(&in, &out, sizeof(in)) == 0);

    assert(!SitePrefs_Decode(&out, buf, n - 1, globalKeys));   /* truncated */
    buf[5] = 9;                                                 /* version 9 */
    buf[4] = 0;
    assert(!SitePrefs_Decode(&out, buf, n, globalKeys));       /* unknown version */
    memcpy(buf, "XXXX", 4);
    assert(!SitePrefs_Decode(&out, buf, n, globalKeys));       /* foreign */
    assert(SitePrefs_Encode(&in, buf, SITE_FILE_SIZE_MAX - 1) == 0);
}

/* A DCTelnet 1.x PrefsStruct as the builds before v2.0 stored it in their
 * settings files: big-endian, the old field layout. */
static UBYTE oldPrefs[508];
static void put16(size_t at, unsigned v) { oldPrefs[at] = (UBYTE)(v >> 8); oldPrefs[at + 1] = (UBYTE)v; }
static void put32(size_t at, unsigned long v) { put16(at, (unsigned)(v >> 16)); put16(at + 2, (unsigned)v); }

static void make_old_prefs(unsigned long flags, const char *font) {
    memset(oldPrefs, 0, sizeof(oldPrefs));
    put32(0, 0x8000); put16(4, 640); put16(6, 256); put16(8, 4); put16(10, 8);
    strcpy((char *)oldPrefs + 12, font);
    put32(232, flags);
    strcpy((char *)oldPrefs + 340, "ANSI");                 /* displayidstr */
}

/* Files written by the first per-entry build ('DCS1' + the old struct, a
 * full snapshot) load as every group overridden, with the global function
 * keys and no login macro. */
static void test_a_dcs1_snapshot_loads_as_all_groups(void) {
    static struct SiteSettings out;
    static TEXT globalKeys[SITE_FKEY_BYTES];
    UBYTE buf[4 + 376];

    make_old_prefs(1UL << 15, "Petscii.font");               /* PETSCII Mode */
    memcpy(buf, "DCS1", 4);
    memcpy(buf + 4, oldPrefs, 376);
    strcpy((char *)globalKeys, "global F1");
    assert(SitePrefs_Decode(&out, buf, sizeof(buf), globalKeys));
    assert(out.groups == SITE_GROUPS_ALL);
    assert(!strcmp((char *)out.prefs.FontName, "Petscii.font") && (out.prefs.State & APP_PETSCII_MODE));
    assert(strcmp((char *)out.fKeys, "global F1") == 0);
    assert(out.loginMacro[0] == 0);
    assert(!SitePrefs_Decode(&out, buf, sizeof(buf) - 1, globalKeys));
}

/* The entries made before v2.0 ('DCS4': groups, the 508-byte old struct,
 * keys, macro) keep their settings, keys and login macro. */
static void test_a_dcs4_file_from_before_v2_loads(void) {
    static struct SiteSettings out;
    static TEXT globalKeys[SITE_FKEY_BYTES];
    static UBYTE buf[4 + 4 + 508 + SITE_FKEY_BYTES + SITE_LOGIN_MACRO_SIZE];
    ULONG groups = SITE_GROUP_TERMINAL | SITE_GROUP_KEYBOARD;
    UBYTE *p = buf;

    make_old_prefs((1UL << 13) | (1UL << 4), "IBM.font");     /* Raw Connection, BS/DEL swap */
    memcpy(p, "DCS4", 4);             p += 4;
    memcpy(p, &groups, 4);            p += 4;
    memcpy(p, oldPrefs, 508);         p += 508;
    strcpy((char *)p, "entry F1");    p += SITE_FKEY_BYTES;
    strcpy((char *)p, "\\u\\r");
    assert(SitePrefs_Decode(&out, buf, sizeof(buf), globalKeys));
    assert(out.groups == groups);
    assert(out.prefs.State & APP_RAW_CONNECTION);
    assert(out.prefs.State & APP_BACKSPACE_DEL_SWAPPED);
    assert(!strcmp((char *)out.prefs.TelnetTermType, "ANSI"));
    assert(!strcmp((char *)out.fKeys, "entry F1") && !strcmp(out.loginMacro, "\\u\\r"));
    buf[3] = '3';                          /* a DCS3 of DCS4's length: refused, not half-read */
    assert(!SitePrefs_Decode(&out, buf, sizeof(buf), globalKeys));
}

/* Collect the macro's segments; a wait between segments shows as '|'. */
static void expand(const char *macro, const char *user, const char *pass, char *out) {
    const char *cursor = macro;
    char seg[64];
    BOOL wait;
    size_t n;

    out[0] = 0;
    while (*cursor) {
        n = SitePrefs_NextMacroSegment(&cursor, user, pass, seg, sizeof(seg), &wait);
        strncat(out, seg, n);
        if (wait) strcat(out, "|");
    }
}

/* A login macro types the username and password for the user: \u, \p,
 * \r (Return), \d (wait one second), \\ (a backslash). A password with a
 * backslash in it must go out unchanged. */
static void test_login_macro_expands_codes(void) {
    char out[128];

    expand("\\u\\r\\d\\p\\r", "spot", "se\\cret", out);
    assert(strcmp(out, "spot\r|se\\cret\r") == 0);

    expand("x\\\\y\\q", "u", "p", out);          /* \\ -> \, unknown \q kept */
    assert(strcmp(out, "x\\y\\q") == 0);

    expand("", "u", "p", out);
    assert(out[0] == 0);
}

/* An entry that overrides the Keyboard group brings its own F1-F10 for the
 * connection; the global keys are kept aside and come back afterwards. */
static void test_entry_function_keys_swap_in_and_back(void) {
    static TEXT live[SITE_FKEY_BYTES], aside[SITE_FKEY_BYTES];
    static struct SiteSettings entry;

    strcpy((char *)live, "global F1");
    strcpy((char *)entry.fKeys, "entry F1");

    entry.groups = SITE_GROUP_SCREEN;               /* Keyboard not overridden */
    assert(!SitePrefs_SwapInKeys(live, aside, &entry));
    assert(strcmp((char *)live, "global F1") == 0);

    entry.groups = SITE_GROUP_KEYBOARD;
    assert(SitePrefs_SwapInKeys(live, aside, &entry));
    assert(strcmp((char *)live, "entry F1") == 0);
    SitePrefs_SwapOutKeys(live, aside);
    assert(strcmp((char *)live, "global F1") == 0);
}

/* Save Settings to Address Book Entry stores the groups the user changed
 * since connecting -- one field of a group is enough to mark it. */
static void test_differing_groups_names_what_changed(void) {
    static TEXT keysA[SITE_FKEY_BYTES], keysB[SITE_FKEY_BYTES];
    struct PrefsStruct a = make(0, "IBM.font", 8), b = a;

    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == 0);
    b.State |= APP_LEDS_ENABLED; b.MainWinLeftEdge = 7; b.nScrollbackLines = 1;     /* app-wide + geometry */
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == 0);

    b = a; b.FontSize = 11;
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_SCREEN);
    b = a; strcpy((char *)b.TelnetTermType, "ANSI");
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_TERMINAL);
    b = a; strcpy((char *)b.XferOptions, "TN");
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_TRANSFER);
    b = a; strcpy((char *)keysB, "F1");
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_KEYBOARD);
    b = a; b.State |= APP_BACKSPACE_DEL_SWAPPED | APP_PETSCII_MODE;
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == (SITE_GROUP_KEYBOARD | SITE_GROUP_TERMINAL));
}

/* Connecting to an entry with its own font on the Workbench closed and
 * reopened the window; a font or palette change needs only the console
 * reopened. Anything else about the display still takes the full reopen. */
static void test_font_or_palette_change_is_a_look_change(void) {
    struct PrefsStruct a = make(0, "topaz.font", 8), b = a;       /* on the Workbench */

    strcpy((char *)b.FontName, "IBM.font"); b.FontSize = 16;
    b.DeviceColors[1] = 0x0A00;
    assert(SitePrefs_OnlyLookDiffers(&a, &b));
    b.State |= APP_TOOL_BAR_ENABLED;                /* a window change */
    assert(!SitePrefs_OnlyLookDiffers(&a, &b));
    b = a; strcpy((char *)b.XemLibrary, "xemvt340.library");
    assert(!SitePrefs_OnlyLookDiffers(&a, &b));
}

/* A font chosen from the menu while connected to an entry with its own
 * font went back to topaz at disconnect: the change lived only in the
 * session. What the user changes by hand is carried into the global
 * settings -- whole fields, flags bit by bit, the rest untouched. */
static void test_a_manual_change_during_a_session_is_kept_globally(void) {
    struct PrefsStruct global = make(APP_BACKSPACE_DEL_SWAPPED, "topaz.font", 8);
    struct PrefsStruct before = make(APP_RETURN_SENDING_CRLF, "TopazPro.font", 16), after = before;
    struct PrefsStruct pristine;

    strcpy((char *)global.XferLibrary, "xprzmodem.library");
    strcpy((char *)before.XferLibrary, "xprkermit.library");        /* the entry's */
    after = before;
    strcpy((char *)after.FontName, "Thin711.font"); after.FontSize = 11;
    after.State |= APP_LOCAL_ECHO;
    pristine = global;
    SitePrefs_CarryChange(&global, &before, &after);
    assert(strcmp((char *)global.FontName, "Thin711.font") == 0 && global.FontSize == 11);
    assert(global.State == (APP_BACKSPACE_DEL_SWAPPED | APP_LOCAL_ECHO));  /* only the changed bit */
    assert(strcmp((char *)global.XferLibrary, "xprzmodem.library") == 0);  /* not changed: kept */
    /* Every field is in the carry table: a struct with every byte changed
     * arrives whole. */
    memset(&before, 0x11, sizeof(before));
    memset(&after, 0x5A, sizeof(after));
    before.State = ~after.State;                 /* every flag bit changed too */
    memset(&global, 0, sizeof(global));
    SitePrefs_CarryChange(&global, &before, &after);
    assert(memcmp(&global, &after, sizeof(global)) == 0);
    before = make(APP_RETURN_SENDING_CRLF, "TopazPro.font", 16);
    after = before;                                                  /* nothing changed */
    global = pristine;
    SitePrefs_CarryChange(&global, &before, &after);
    assert(memcmp(&global, &pristine, sizeof(global)) == 0);
}

/* A font picked while connected is for this run only: it survives the
 * disconnect but DCTelnet.Prefs keeps the one it replaced. Picked again
 * while not connected, it is a real change and saved. */
static void test_a_font_picked_while_connected_is_not_saved(void) {
    static struct SiteHandChanges hand;
    struct PrefsStruct global = make(0, "topaz.font", 8), session, before, toSave;

    SitePrefs_HandInit(&hand, &global);
    session = make(0, "TopazPro.font", 16);                 /* the entry's font */
    before = session;
    strcpy((char *)session.FontName, "Thin711.font"); session.FontSize = 11;
    SitePrefs_HandChange(&hand, &global, &before, &session, TRUE);
    assert(strcmp((char *)global.FontName, "Thin711.font") == 0);   /* the run keeps it */
    toSave = global;
    SitePrefs_HandForSave(&hand, &toSave);
    assert(strcmp((char *)toSave.FontName, "topaz.font") == 0 && toSave.FontSize == 8);

    before = global;                                         /* not connected now */
    global.State |= APP_LOCAL_ECHO;                         /* an unrelated real change */
    SitePrefs_HandChange(&hand, &global, &before, &global, FALSE);
    toSave = global;
    SitePrefs_HandForSave(&hand, &toSave);
    assert(strcmp((char *)toSave.FontName, "topaz.font") == 0 && (toSave.State & APP_LOCAL_ECHO));

    before = global;                                         /* the font, for real */
    strcpy((char *)global.FontName, "IBM.font"); global.FontSize = 8;
    SitePrefs_HandChange(&hand, &global, &before, &global, FALSE);
    toSave = global;
    SitePrefs_HandForSave(&hand, &toSave);
    assert(strcmp((char *)toSave.FontName, "IBM.font") == 0);
}

/* The settings window's one-line summary per group. */
static void test_group_summaries(void) {
    struct PrefsStruct p = make(APP_FULLSCREEN | APP_PETSCII_MODE | APP_BACKSPACE_DEL_SWAPPED, "Petscii.font", 8);
    char out[80];

    strcpy((char *)p.TelnetTermType, "VT102");    /* PETSCII Mode sends "PETSCII" regardless */
    strcpy((char *)p.XferLibrary, "xprzmodem.library");
    SitePrefs_GroupSummary(SITE_GROUP_SCREEN, &p, out, sizeof(out));
    assert(strcmp(out, "Petscii 8") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_TERMINAL, &p, out, sizeof(out));
    assert(strcmp(out, "PETSCII Mode, type PETSCII") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_KEYBOARD, &p, out, sizeof(out));
    assert(strcmp(out, "BS/DEL Swap") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_TRANSFER, &p, out, sizeof(out));
    assert(strcmp(out, "xprzmodem.library") == 0);

    p.State = APP_RENDERER_XEM_LIB | APP_RETURN_SENDING_CRLF;      /* on the Workbench */
    strcpy((char *)p.XemLibrary, "xemvt340.library");
    SitePrefs_GroupSummary(SITE_GROUP_TERMINAL, &p, out, 20);        /* truncates safely */
    assert(strlen(out) == 19);
    SitePrefs_GroupSummary(SITE_GROUP_TERMINAL, &p, out, sizeof(out));
    assert(strcmp(out, "XEM xemvt340.library, type VT102") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_KEYBOARD, &p, out, sizeof(out));
    assert(strcmp(out, "Return = CR + LF") == 0);
    p.State = 0;
    SitePrefs_GroupSummary(SITE_GROUP_KEYBOARD, &p, out, sizeof(out));
    assert(strcmp(out, "Standard keys") == 0);
}

int main(void) {
    test_differing_groups_names_what_changed();
    test_font_or_palette_change_is_a_look_change();
    test_a_manual_change_during_a_session_is_kept_globally();
    test_a_font_picked_while_connected_is_not_saved();
    test_group_summaries();
    test_entry_function_keys_swap_in_and_back();
    test_login_macro_expands_codes();
    test_sidecar_round_trip_and_rejects();
    test_apply_takes_only_overridden_groups();
    test_each_group_moves_only_its_members();
    test_a_dcs1_snapshot_loads_as_all_groups();
    test_a_dcs4_file_from_before_v2_loads();
    test_restore_brings_back_global_but_keeps_moved_windows();
    test_save_writes_global_during_a_session();
    test_display_differs_names_what_needs_a_reopen();
    printf("site_prefs: all assertions passed\n");
    return 0;
}
