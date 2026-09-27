/* src/petscii_screencode.c */
#include "petscii_screencode.h"

uint8_t petscii_to_screencode(uint8_t petscii) {
    if (petscii <= 0x1F) return (uint8_t)(petscii + 0x80);
    if (petscii <= 0x3F) return petscii;
    if (petscii <= 0x5F) return (uint8_t)(petscii - 0x40);
    if (petscii <= 0x7F) return (uint8_t)(petscii - 0x20);
    if (petscii <= 0x9F) return (uint8_t)(petscii + 0x40);
    if (petscii <= 0xBF) return (uint8_t)(petscii - 0x40);
    if (petscii <= 0xDF) return (uint8_t)(petscii - 0x80);
    if (petscii <= 0xFE) return (uint8_t)(petscii - 0x80);
    return 0x5E; /* 0xFF: pi symbol */
}
