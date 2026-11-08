/* src/dsr.h -- ANSI Device Status Report requests in a BBS's text stream.
 * Pure, unit-tested on the host.
 *
 * Some BBSes ask the terminal where its cursor is (CSI 6 n, answered with
 * CSI row ; column R) or whether it is there at all (CSI 5 n, answered with
 * CSI 0 n) and wait for the answer: without one they stall or draw in the
 * wrong place (upstream issue #11: Absinthe, 20 For Beers, Constructive
 * Chaos). The console only draws, so DCTelnet answers. A request may be split
 * between two network reads; the scanner keeps its state across calls. */
#ifndef DSR_H
#define DSR_H

#include <exec/types.h>
#include <string.h>

struct DsrScan
{
    UBYTE state;        /* 0 text, 1 after ESC, 2 in a CSI sequence */
    UBYTE param;        /* the numeric parameter so far (saturates) */
    UBYTE plain;        /* no '?' prefix, no second parameter */
};

#define DSR_NONE     0
#define DSR_STATUS   5  /* CSI 5 n */
#define DSR_POSITION 6  /* CSI 6 n */

void Dsr_Init(struct DsrScan *s);

/* Scan buf[0..len) for the next complete request. Returns the offset just
 * after its final 'n' and sets *kind to DSR_STATUS or DSR_POSITION; with
 * no request, returns len and sets *kind to DSR_NONE. */
size_t Dsr_Find(struct DsrScan *s, const UBYTE *buf, size_t len, int *kind);

/* The answer to a request: "ESC[0n", or "ESC[row;colR". Returns its length
 * (0 if out does not hold it). */
size_t Dsr_Answer(int kind, UWORD row, UWORD col, char *out, size_t max);

#endif /* DSR_H */
