#!/bin/sh
# tools/gen_petscii_glyphs.sh -- regenerate src/petscii_glyphs.c from
# assets/glyphs/c64_upper_8x8.bin and c64_lower_8x8.bin. Run from the
# repository root after a change to either asset:
#   sh tools/gen_petscii_glyphs.sh > src/petscii_glyphs.c
# tests/test_petscii_font fails while the tracked file and the assets differ.
set -e
cat <<'HEAD'
/* src/petscii_glyphs.c -- the C64 character ROM glyphs DCTelnet builds its
 * Petscii fonts from (src/petscii_font.c): 256 glyphs of 8 bytes each,
 * indexed by raw PETSCII byte.
 *
 * Made by tools/gen_petscii_glyphs.sh from assets/glyphs; do not edit.
 * Tracked because the Windows build has no POSIX shell to generate it;
 * tests/test_petscii_font fails while this file and the assets differ. */
#include "petscii_font.h"

HEAD
sh tools/bin2c.sh petscii_glyphs_upper assets/glyphs/c64_upper_8x8.bin
echo
sh tools/bin2c.sh petscii_glyphs_lower assets/glyphs/c64_lower_8x8.bin
