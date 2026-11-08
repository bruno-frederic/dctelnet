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

int main(void) {
    test_console_fkey_digit_0_is_c64_f1();
    test_screencode_ranges();
    test_fallback_letters_swap_case();
    test_keymap();
    printf("petscii modules: all assertions passed\n");
    return 0;
}
