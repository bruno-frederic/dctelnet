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
#include <string.h>

/* A CP437 byte as ISO-8859-1. Control codes (below 32) become spaces. */
UBYTE Charset_Cp437ToLatin1(UBYTE c);

/* A PETSCII byte as the C64 shows it, as ISO-8859-1; lowerCase is the C64's
 * upper/lower case set (the font the terminal shows). Graphics become '#'. */
UBYTE Charset_PetsciiToLatin1(UBYTE c, BOOL lowerCase);

/* The Character Set setting: what the BBS sends (prefs.Charset). */
#define CHARSET_CP437   0       /* IBM PC: shown as it comes (the default) */
#define CHARSET_LATIN1  1       /* Amiga BBSes: ISO-8859-1, copied as it is */
#define CHARSET_UTF8    2       /* modern BBSes, Unix: shown in the IBM set */

/* UTF-8 from the BBS as CP437 for the IBM font: box drawing exact, what
 * CP437 lacks '?'. The decoder keeps a character split between two
 * network reads. out holds len + 1 bytes: a character the previous read
 * began and this one cuts short gives '?' and the byte that cut it. */
struct Utf8Decoder { ULONG code; UBYTE need; };
void   Charset_Utf8Init(struct Utf8Decoder *d);
size_t Charset_Utf8ToCp437(struct Utf8Decoder *d, const UBYTE *in, size_t len, UBYTE *out);

/* Typed or pasted ISO-8859-1 as UTF-8 (out holds 2 * len bytes). */
size_t Charset_Latin1ToUtf8(const UBYTE *in, size_t len, UBYTE *out);

#endif /* CHARSET_H */
