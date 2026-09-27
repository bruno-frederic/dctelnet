/* src/petscii_fallback.h */
#ifndef PETSCII_FALLBACK_H
#define PETSCII_FALLBACK_H

#include <stdint.h>

/*
 * PETSCII byte -> nearest CP437 byte, for rendering through DCTelnet's
 * existing CP437-based fonts before the real 4-bank C64 font exists.
 * Ported from SyncTerm's src/conio/cterm_petscii.h `display_petscii[]`
 * table.
 *
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html
 * Licensed LGPLv2+ (GNU Lesser General Public License, version 2 or
 * later) - see lgpl.txt or http://www.fsf.org/copyleft/lesser.html.
 * This file must keep this notice; do not relicense it.
 */
uint8_t petscii_fallback_cp437(uint8_t petscii);

#endif /* PETSCII_FALLBACK_H */
