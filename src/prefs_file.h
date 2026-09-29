/*
 * @file prefs_file.h
 * @brief The DCTelnet.Prefs file as bytes: which format it is and what it holds.
 *
 * Pure (no AmigaOS calls), unit-tested on the host. prefs.c reads the file
 * and hands the bytes here.
 */
#ifndef PREFS_FILE_H
#define PREFS_FILE_H

#include <exec/types.h>
#include <string.h>
#include "prefs.h"

enum
{
    PREFS_FILE_BAD,     // too short, or a version this build does not know
    PREFS_FILE_V2,      // "DCTP" version 2, of any data size
    PREFS_FILE_LEGACY   // a DCTelnet 1.x file: the raw old struct, no header
};

/* The longest Prefs file there can be: the header and its 16-bit dataSize
 * (a 1.x file is 508 bytes at most). prefs.c reads the whole file, never a
 * fixed sizeof(PrefsStruct): an older file is converted, a newer one longer. */
#define PREFS_FILE_MAX  (sizeof(struct DCTFileHeader) + 65535UL)

/*
 * Decodes a whole Prefs file into out (cleared first).
 *
 * A v2 file whose data is shorter than this build's PrefsStruct (written
 * by an older 2.x) fills the fields it has and leaves the rest 0 -- for
 * ValidateAndInitPrefs() to default; a longer one (a newer 2.x) gives its
 * first sizeof(PrefsStruct) bytes. Fields are only ever appended, so
 * neither is an error.
 *
 * A legacy file keeps every setting it has a place for (Prefs_FromLegacy).
 */
int Prefs_Decode(const UBYTE *file, size_t len, struct PrefsStruct *out);

/*
 * The settings of a DCTelnet 1.x PrefsStruct (big-endian bytes, len long)
 * in the current struct: screen mode, font, palette, window geometry,
 * paths, libraries, terminal type, scrollback size, and the option flags
 * with their current meaning. FALSE if it is too short to hold the screen
 * mode, font and palette.
 */
BOOL Prefs_FromLegacy(const UBYTE *old, size_t len, struct PrefsStruct *out);

#endif /* PREFS_FILE_H */
