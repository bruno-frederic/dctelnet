/* src/charset.h -- character set conversions. Pure, unit-tested on the host.
 *
 * The terminal draws BBS text in the IBM PC character set (CP437, through
 * an IBM font) or, in PETSCII Mode, in the C64's. Text leaving the terminal
 * -- copied to the clipboard, saved -- is ISO-8859-1, as Amiga text is:
 * a character Latin-1 also has keeps its meaning, a line-drawing or other
 * graphic becomes the closest ASCII ('+', '-', '|', '#'). */
#ifndef CHARSET_H
#define CHARSET_H

#include <exec/types.h>

/* A CP437 byte as ISO-8859-1. Control codes (below 32) become spaces. */
UBYTE Charset_Cp437ToLatin1(UBYTE c);

/* A PETSCII byte as the C64 shows it, as ISO-8859-1; lowerCase is the C64's
 * upper/lower case set (the font the terminal shows). Graphics become '#'. */
UBYTE Charset_PetsciiToLatin1(UBYTE c, BOOL lowerCase);

#endif /* CHARSET_H */
