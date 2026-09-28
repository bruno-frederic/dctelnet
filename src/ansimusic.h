/* src/ansimusic.h -- ANSI music (as SyncTERM, BananaCom). Pure, unit-tested
 * on the host.
 *
 * A BBS plays a tune with ESC[M, ESC[N or ESC[| followed by a music string
 * in the BASIC PLAY language, ended by 0x0E:
 *   T n tempo (quarter notes a minute, 32-255)   L n default length (1-64)
 *   O n octave (0-6)   > <  octave up, down      A-G note (# or + sharp,
 *   - flat, a length, dots)   N n note 0-84 (0 rest)   P n pause
 *   MN / ML / MS normal, legato, staccato   MF / MB (ignored)
 * The filter takes these sequences out of the text -- the console must not
 * see them: ESC[M alone is Delete Line -- and turns them into notes. Only
 * ESC[M directly followed by music is music; ESC[2M stays Delete Lines. */
#ifndef ANSIMUSIC_H
#define ANSIMUSIC_H

#include <exec/types.h>
#include <string.h>

struct Note
{
    UWORD hz;           /* 0: a rest */
    UWORD ms;
};

struct AnsiMusic
{
    UWORD tempo;
    UWORD num;          /* the number being read */
    BOOL  hasNum;
    UBYTE state;        /* text, ESC, CSI, music */
    UBYTE octave, length, style, dots, flat, sharp;
    char  cmd;          /* the command whose number is being read */
    UBYTE spare;        /* (no padding: vbcc warns about it) */
};

void AnsiMusic_Init(struct AnsiMusic *m);

/* Copies in[0..len) to out without its music sequences (out holds len + 3
 * bytes: an ESC [ M held back from the previous read may come first); notes
 * found go to notes[] (at most maxNotes, the count in *nNotes). Returns the
 * length written to out. */
size_t AnsiMusic_Filter(struct AnsiMusic *m, const UBYTE *in, size_t len, UBYTE *out,
                        struct Note *notes, int maxNotes, int *nNotes);

/* The pitch of note number n (1-84, 46 = A 440 Hz) in Hz. */
UWORD AnsiMusic_Hz(int n);

#endif /* ANSIMUSIC_H */
