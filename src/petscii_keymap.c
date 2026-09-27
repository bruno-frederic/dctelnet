/* src/petscii_keymap.c */
#include "petscii_keymap.h"

/* Same LGPLv2+ notice as petscii_keymap.h applies to this table. */
int petscii_translate_key(int ascii_or_special, int is_special) {
    if (is_special) {
        switch (ascii_or_special) {
            case PETSCII_KEY_DOWN:   return 17;
            case PETSCII_KEY_HOME:   return 19;
            case PETSCII_KEY_DEL:    return 20;
            case PETSCII_KEY_RIGHT:  return 29;
            case PETSCII_KEY_F1:     return 133;
            case PETSCII_KEY_F3:     return 134;
            case PETSCII_KEY_F5:     return 135;
            case PETSCII_KEY_F7:     return 136;
            case PETSCII_KEY_F2:     return 137;
            case PETSCII_KEY_F4:     return 138;
            case PETSCII_KEY_F6:     return 139;
            case PETSCII_KEY_F8:     return 140;
            case PETSCII_KEY_UP:     return 145;
            case PETSCII_KEY_INSERT: return 148;
            case PETSCII_KEY_LEFT:   return 157;
            case PETSCII_KEY_STOP:   return 3;
            default:                 return -1;
        }
    }
    if (ascii_or_special >= 'A' && ascii_or_special <= 'Z')
        return ascii_or_special - 'A' + 'a';
    if (ascii_or_special >= 'a' && ascii_or_special <= 'z')
        return ascii_or_special - 'a' + 'A';
    return ascii_or_special; /* digits, punctuation: unchanged */
}
