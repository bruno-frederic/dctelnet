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

int petscii_fkey_from_console_digit(char digit) {
    static const int fkeys[8] = {
        PETSCII_KEY_F1, PETSCII_KEY_F2, PETSCII_KEY_F3, PETSCII_KEY_F4,
        PETSCII_KEY_F5, PETSCII_KEY_F6, PETSCII_KEY_F7, PETSCII_KEY_F8
    };
    if (digit < '0' || digit > '7') return -1;
    return petscii_translate_key(fkeys[digit - '0'], 1);
}

int petscii_key_from_digit(char digit, int ctrl, int commodore) {
    static const unsigned char ctrl_bytes[10] = {
        0x92, 0x90, 0x05, 0x1C, 0x9F, 0x9C, 0x1E, 0x1F, 0x9E, 0x12   /* 0-9 */
    };
    static const unsigned char cbm_bytes[8] = {
        0x81, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B                /* 1-8 */
    };

    if (digit < '0' || digit > '9') return -1;
    if (ctrl) return ctrl_bytes[digit - '0'];
    if (commodore && digit >= '1' && digit <= '8') return cbm_bytes[digit - '1'];
    return -1;
}
