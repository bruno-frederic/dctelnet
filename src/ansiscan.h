/* src/ansiscan.h -- reading and writing ANSI text. Pure, unit-tested on the host. */
#ifndef ANSISCAN_H
#define ANSISCAN_H

#include <exec/types.h>
#include <string.h>

/* The index after a CSI sequence's parameter bytes (ECMA-48: '0'-'?',
 * digits, ';', and the private '<' '=' '>' '?') from s[i], at most size.
 * The scroll back's copy of the text stopped at ';', so the rest of
 * CSI ?25h landed in it as text. */
size_t Ansi_ParamsEnd(const UBYTE *s, size_t i, size_t size);

/* Where needle (NUL-terminated) is in hay[0..len), ignoring the case of
 * ISO-8859-1 letters; -1 if it is not. An empty needle is not found. */
long Ansi_FindText(const char *hay, size_t len, const char *needle);

/* One screen row (ibmcon's IBMCMD_READTEXT cells: character, fg, bg,
 * flags) as ANSI text for a .ans file: the characters as they are (CP437),
 * an SGR sequence where the colours or attributes change, the row's
 * trailing blanks on the default background dropped, CR LF at the end.
 * *attr carries the attributes between rows (start with ANSI_ATTR_RESET).
 * swap17: the pens are ibmcon's unit order (no pen table: 1 and 7 swapped).
 * Returns the length written; out holds cols * 24 + 2 bytes. */
#define ANSI_ATTR_RESET 0xFFFFFFFFUL
size_t Ansi_ScreenRow(const UBYTE *cells, UWORD cols, BOOL swap17, ULONG *attr, char *out);

/* Removes every byte b from data[0..*len) in place; returns how many went
 * (BEL, when the Bell is a sound or off: the console would flash). */
size_t Ansi_StripByte(UBYTE *data, size_t *len, UBYTE b);

#endif /* ANSISCAN_H */
