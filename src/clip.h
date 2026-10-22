/* src/clip.h -- terminal text to and from the clipboard.
 * Pure, unit-tested on the host.
 *
 * The clipboard holds IFF FTXT: FORM <size> FTXT, then CHRS chunks of
 * ISO-8859-1 text. DCTelnet copies what the mouse selected on the screen
 * (read back from ibmcon's screen buffer: 4 bytes a cell, the character
 * first) and pastes the clipboard's text to the BBS. */
#ifndef CLIP_H
#define CLIP_H

#include <exec/types.h>
#include <string.h>

#define CLIP_CELL 4     /* bytes a cell in ibmcon's IBMCMD_READTEXT rows */

/* A selection from the cell where the mouse went down to the one where it
 * is, in reading order: start <= end. Rows and columns are 1-based. */
struct ClipRange { UWORD startRow, startCol, endRow, endCol; };
void Clip_Order(UWORD downRow, UWORD downCol, UWORD row, UWORD col, struct ClipRange *out);

/* The columns of row `row` a range covers (from..to), FALSE if none. */
BOOL Clip_RowSpan(const struct ClipRange *r, UWORD row, UWORD cols, UWORD *from, UWORD *to);

/* What the screen's characters are: CP437 (IBM PC BBSes, and UTF-8 text,
 * which is shown in CP437), ISO-8859-1 (Amiga BBSes), or PETSCII in the
 * C64's upper-case or lower-case set. */
enum { CLIP_CP437, CLIP_LATIN1, CLIP_PETSCII_UPPER, CLIP_PETSCII_LOWER };

/* Columns from..to (1-based, inclusive) of one screen row of `cols` cells,
 * as ISO-8859-1 text without its trailing blanks. Returns the length
 * written to out (at most to-from+1 bytes). */
size_t Clip_RowText(const UBYTE *cells, UWORD cols, UWORD from, UWORD to, int chars, char *out);

/* The length of text without the empty lines at its end (a copied screen's
 * blank rows below the text). */
size_t Clip_TrimEmptyLines(const char *text, size_t len);

/* text as an IFF FTXT file with one CHRS chunk. Returns its size, 0 when
 * max is too small. */
size_t Clip_BuildFtxt(const char *text, size_t len, UBYTE *out, size_t max);

/* The text of every CHRS chunk of an IFF FTXT file, joined. Returns its
 * length (at most max), 0 for anything that is not FTXT. A file cut short
 * (a clip larger than the buffer it was read into) gives the text there is. */
size_t Clip_ParseFtxt(const UBYTE *iff, size_t len, char *out, size_t max);

/* Clipboard text as the bytes a BBS gets when it is typed: line ends (LF,
 * CR LF, CR) become Return -- CR, or CR LF with crlf -- and, on a telnet
 * connection (doubleIac), a 255 byte is doubled. Returns the length written
 * (out must hold 2 * len bytes). */
size_t Clip_PasteBytes(const char *text, size_t len, BOOL crlf, BOOL doubleIac, char *out);

#endif /* CLIP_H */
