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

int main(void) {
    test_screencode_ranges();
    test_fallback_letters_swap_case();
    test_keymap();
    printf("petscii modules: all assertions passed\n");
    return 0;
}
