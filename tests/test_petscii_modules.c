/* test/test_petscii_modules.c -- screencode, CP437 fallback and keymap tables. */
#include <assert.h>
#include <stdio.h>
#include "petscii_screencode.h"
#include "petscii_fallback.h"
#include "petscii_keymap.h"

static void test_screencode_ranges(void) {
    assert(petscii_to_screencode(0x41) == 0x01); /* 'A' */
    assert(petscii_to_screencode(0x20) == 0x20); /* space */
    assert(petscii_to_screencode(0x61) == 0x41); /* graphics block */
    assert(petscii_to_screencode(0xA0) == 0x60); /* shifted space */
    assert(petscii_to_screencode(0xFF) == 0x5E); /* pi */
}

static void test_fallback_letters_swap_case(void) {
    assert(petscii_fallback_cp437(0x41) == 'a');
    assert(petscii_fallback_cp437(0x61) == 'A');
    assert(petscii_fallback_cp437('1') == '1');
}

static void test_keymap(void) {
    assert(petscii_translate_key('a', 0) == 'A');
    assert(petscii_translate_key('A', 0) == 'a');
    assert(petscii_translate_key('7', 0) == '7');
    assert(petscii_translate_key(PETSCII_KEY_UP, 1) == 145);
    assert(petscii_translate_key(PETSCII_KEY_DEL, 1) == 20);
    assert(petscii_translate_key(PETSCII_KEY_F1, 1) == 133);
    assert(petscii_translate_key(PETSCII_KEY_F8, 1) == 140);
}

/* The Amiga console reports F1-F10 as CSI <digit> ~ with digit 0-9, F1 = 0.
 * The mapping started at 1: F1 sent nothing and F2-F8 sent the key below. */
static void test_console_fkey_digit_0_is_c64_f1(void) {
    assert(petscii_fkey_from_console_digit('0') == 133);   /* F1 */
    assert(petscii_fkey_from_console_digit('1') == 137);   /* F2 */
    assert(petscii_fkey_from_console_digit('7') == 140);   /* F8 */
    assert(petscii_fkey_from_console_digit('8') == -1);    /* F9: no C64 key */
    assert(petscii_fkey_from_console_digit('x') == -1);
}


/* CTRL/C= with the number row, as on a C64 and in CGTerm: colours and
 * reverse video; anything else is not a C64 key. */
static void test_number_row_sends_colours_and_reverse(void) {
    assert(petscii_key_from_digit('1', 1, 0) == 0x90);   /* CTRL+1 black */
    assert(petscii_key_from_digit('2', 1, 0) == 0x05);   /* CTRL+2 white */
    assert(petscii_key_from_digit('8', 1, 0) == 0x9E);   /* CTRL+8 yellow */
    assert(petscii_key_from_digit('9', 1, 0) == 0x12);   /* CTRL+9 RVS ON */
    assert(petscii_key_from_digit('0', 1, 0) == 0x92);   /* CTRL+0 RVS OFF */
    assert(petscii_key_from_digit('1', 0, 1) == 0x81);   /* C=+1 orange */
    assert(petscii_key_from_digit('8', 0, 1) == 0x9B);   /* C=+8 light grey */
    assert(petscii_key_from_digit('9', 0, 1) == -1);
    assert(petscii_key_from_digit('0', 0, 1) == -1);
    assert(petscii_key_from_digit('5', 0, 0) == -1);     /* plain digit: typed text */
    assert(petscii_key_from_digit('a', 1, 1) == -1);
    assert(petscii_key_from_digit('3', 1, 1) == 0x1C);   /* CTRL wins */
}

int main(void) {
    test_number_row_sends_colours_and_reverse();
    test_console_fkey_digit_0_is_c64_f1();
    test_screencode_ranges();
    test_fallback_letters_swap_case();
    test_keymap();
    printf("petscii modules: all assertions passed\n");
    return 0;
}
