/* src/petscii_keymap.h */
#ifndef PETSCII_KEYMAP_H
#define PETSCII_KEYMAP_H

/*
 * Special-key enum ported from the shape of SyncTerm's src/conio/
 * cterm_petscii.h `input_petscii[]` table (CIO_KEY_* entries).
 * Copyright Rob Swindell - LGPLv2+ - see petscii_fallback.h for the full
 * notice; this table carries the same license.
 *
 * This is an ABSTRACTION, not an AmigaOS key code: DCTelnet.c's keyboard
 * handling maps console key sequences onto these values, so this module has
 * no AmigaOS header dependency and can be unit-tested on the host.
 */
enum PetsciiSpecialKey {
    PETSCII_KEY_UP,
    PETSCII_KEY_DOWN,
    PETSCII_KEY_LEFT,
    PETSCII_KEY_RIGHT,
    PETSCII_KEY_HOME,
    PETSCII_KEY_DEL,
    PETSCII_KEY_INSERT,
    PETSCII_KEY_F1, PETSCII_KEY_F2, PETSCII_KEY_F3, PETSCII_KEY_F4,
    PETSCII_KEY_F5, PETSCII_KEY_F6, PETSCII_KEY_F7, PETSCII_KEY_F8,
    PETSCII_KEY_STOP /* RUN/STOP */
};

/*
 * is_special == 0: ascii_or_special is a plain ASCII char, returns the
 * case-swapped PETSCII byte to transmit (digits/punctuation pass through
 * unchanged).
 * is_special == 1: ascii_or_special is a PetsciiSpecialKey value, returns
 * the PETSCII control byte for that key.
 */
int petscii_translate_key(int ascii_or_special, int is_special);

#endif /* PETSCII_KEYMAP_H */
