/* test/test_keys.c -- keys typed in the terminal, as the BBS expects them. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keys.h"

#define CSI "\x9b"

/* Parses buf and returns the keys it holds, text as its byte. */
static int keys_of(const char *buf, int *ids, UBYTE *texts, int max)
{
    size_t len = strlen(buf), at = 0;
    int n = 0;

    while (at < len && n < max)
    {
        int id;
        UBYTE t = 0;

        at += Keys_Next((const UBYTE *)buf + at, len - at, &id, &t);
        ids[n] = id;
        texts[n++] = t;
    }
    return n;
}

/* Shift-F1 is CSI 1 0 ~. DCTelnet read the '1' as F2's macro and sent the
 * "0~" as text. */
static void test_shift_f1_is_one_key_not_f2_and_text(void)
{
    int ids[8];
    UBYTE t[8];

    assert(keys_of(CSI "10~", ids, t, 8) == 1 && ids[0] == KEY_SHIFT_F1);
    assert(keys_of(CSI "19~", ids, t, 8) == 1 && ids[0] == KEY_SHIFT_F1 + 9);
    assert(keys_of(CSI "0~" CSI "9~", ids, t, 8) == 2 && ids[0] == KEY_F1 && ids[1] == KEY_F1 + 9);
}

/* Shift-Left is CSI " A": the 'A' went out as text after the space. */
static void test_shift_cursor_keys_leak_no_text(void)
{
    int ids[8];
    UBYTE t[8];

    assert(keys_of(CSI " A" CSI " @" CSI "T" CSI "S", ids, t, 8) == 4);
    assert(ids[0] == KEY_HOME && ids[1] == KEY_END && ids[2] == KEY_PAGE_UP && ids[3] == KEY_PAGE_DOWN);
    assert(keys_of(CSI "?~", ids, t, 8) == 1 && ids[0] == KEY_HELP);
}

static void test_text_and_cursor_keys(void)
{
    int ids[8];
    UBYTE t[8];

    assert(keys_of("a" CSI "A" "b", ids, t, 8) == 3);
    assert(ids[0] == KEY_TEXT && t[0] == 'a' && ids[1] == KEY_UP && ids[2] == KEY_TEXT && t[2] == 'b');
    assert(keys_of(CSI "44~" CSI "45~" CSI "40~", ids, t, 8) == 3);
    assert(ids[0] == KEY_HOME && ids[1] == KEY_END && ids[2] == KEY_INSERT);
}

static void test_unknown_and_cut_sequences_send_nothing(void)
{
    int ids[8];
    UBYTE t[8];

    assert(keys_of(CSI "5;3v", ids, t, 8) == 1 && ids[0] == KEY_NONE);
    assert(keys_of(CSI "12", ids, t, 8) == 1 && ids[0] == KEY_NONE);    /* cut short */
}

static void test_raw_codes_of_the_extra_keys(void)
{
    assert(Keys_FromRawCode(0x70) == KEY_HOME && Keys_FromRawCode(0x71) == KEY_END);
    assert(Keys_FromRawCode(0x48) == KEY_PAGE_UP && Keys_FromRawCode(0x49) == KEY_PAGE_DOWN);
    assert(Keys_FromRawCode(0x47) == KEY_INSERT && Keys_FromRawCode(0x45) == KEY_NONE);
}

static int sends(int id, BOOL vt, BOOL ckm, const char *want)
{
    char out[KEYS_MAX_BYTES];
    size_t n = Keys_Bytes(id, vt, ckm, out);

    return n == strlen(want) && memcmp(out, want, n) == 0;
}

/* Home, End, Page Up/Down and Insert sent nothing: BBS editors and lightbar
 * menus use them. */
static void test_navigation_keys_send_bbs_or_vt_codes(void)
{
    assert(sends(KEY_HOME, FALSE, FALSE, "\033[H") && sends(KEY_END, FALSE, FALSE, "\033[K"));
    assert(sends(KEY_PAGE_UP, FALSE, FALSE, "\033[V") && sends(KEY_PAGE_DOWN, FALSE, FALSE, "\033[U"));
    assert(sends(KEY_INSERT, FALSE, FALSE, "\033[@"));
    assert(sends(KEY_HOME, TRUE, FALSE, "\033[1~") && sends(KEY_END, TRUE, FALSE, "\033[4~"));
    assert(sends(KEY_PAGE_UP, TRUE, FALSE, "\033[5~") && sends(KEY_PAGE_DOWN, TRUE, FALSE, "\033[6~"));
    assert(sends(KEY_INSERT, TRUE, FALSE, "\033[2~"));
    assert(sends(KEY_HELP, FALSE, FALSE, "") && sends(KEY_F1, FALSE, FALSE, ""));
}

/* A host that set cursor key mode (CSI ?1h: vi, less) reads ESC O A. */
static void test_cursor_keys_follow_cursor_key_mode(void)
{
    assert(sends(KEY_UP, FALSE, FALSE, "\033[A") && sends(KEY_LEFT, TRUE, FALSE, "\033[D"));
    assert(sends(KEY_UP, FALSE, TRUE, "\033OA") && sends(KEY_RIGHT, TRUE, TRUE, "\033OC"));
}

int main(void)
{
    test_shift_f1_is_one_key_not_f2_and_text();
    test_shift_cursor_keys_leak_no_text();
    test_text_and_cursor_keys();
    test_unknown_and_cut_sequences_send_nothing();
    test_raw_codes_of_the_extra_keys();
    test_navigation_keys_send_bbs_or_vt_codes();
    test_cursor_keys_follow_cursor_key_mode();
    printf("keys: all assertions passed\n");
    return 0;
}
