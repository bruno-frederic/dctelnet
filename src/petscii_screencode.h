/* src/petscii_screencode.h */
#ifndef PETSCII_SCREENCODE_H
#define PETSCII_SCREENCODE_H

#include <stdint.h>

/*
 * Wire PETSCII byte -> C64 screen code. These are DIFFERENT numbering
 * spaces: the wire/keyboard byte value is not what the video chip uses
 * internally to select a glyph. Table per sta.c64.org/cbm64pettoscr.html.
 */
uint8_t petscii_to_screencode(uint8_t petscii);

#endif /* PETSCII_SCREENCODE_H */
