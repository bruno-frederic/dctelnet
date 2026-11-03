/* src/ansimusic.h -- ANSI music (as GW-BASIC PLAY, SyncTERM). Pure,
 * unit-tested on the host against tests/ansimusic-vectors.json (made by
 * tools/ansimusic_reference.py; the same file DEViLBOX and UP-Term test).
 *
 * A BBS plays a tune with ESC[M, ESC[N or ESC[| followed by a music string
 * in the BASIC PLAY language, ended by 0x0E:
 *   T n tempo (quarter notes a minute, 32-255)   L n default length (1-64)
 *   O n octave (0-6)   > <  octave up, down      A-G note (# or + sharp,
 *   - flat, a length, dots x1.5 each)   N n note 0-84 (0 rest)   P n pause
 *   MN / ML / MS normal 7/8, legato, staccato 3/4   MF / MB foreground / background
 * Pitch is GW-BASIC's: O2 A = 440 Hz, O3 C = 523 Hz. Out-of-range numbers
 * are clamped. Other bytes in a string (CR, LF, '"') are skipped. ESC, CAN
 * and SUB end a string early (the command being read still plays). A string
 * with ';' is a SMILE sound effect: it plays nothing.
 * MF (the default, as SyncTERM and GW-BASIC PLAY) makes a tune foreground: the
 * text after it must wait until it has played. MB plays it in the background.
 * A string that queued a note while MF was on is foreground (MF/MB may come
 * anywhere in it, and stay in force for later strings). AnsiMusic_Filter stops
 * at the end of such a string and reports in *used how much input it took;
 * the caller shows the text so far, plays the notes, waits for the sound to
 * end, then filters the rest.
 * The filter takes these sequences out of the text -- the console must not
 * see them: ESC[M alone is Delete Line -- and turns them into notes. ESC[M
 * is music only when the next byte that is not a space starts music;
 * ESC[2M stays Delete Lines. */
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
    UBYTE spaces;       /* spaces held after ESC[M */
    UBYTE smile;        /* TRUE: this string is a SMILE sound effect */
    UBYTE foreground;   /* MF in force (MB clears it); TRUE after Init */
    UBYTE heldFore;     /* this string queued a note while foreground */
    UBYTE stopped;      /* the last Filter call ended on a foreground tune, also when
                         * that was its last input byte: the text after it must wait */
};

void AnsiMusic_Init(struct AnsiMusic *m);

/* Bytes held back from one read to the next: ESC [ M and up to 8 spaces
 * (a 9th space after ESC[M makes it Delete Line). */
#define ANSIMUSIC_HELD 11

/* Copies in[0..len) to out without its music sequences (out holds
 * len + ANSIMUSIC_HELD bytes: what was held back from the previous read may
 * come first); notes
 * found go to notes[] (at most maxNotes, the count in *nNotes). Returns the
 * length written to out. *used is the input taken: len, or less when a
 * foreground tune ended (the rest is not filtered yet; call again with it). */
size_t AnsiMusic_Filter(struct AnsiMusic *m, const UBYTE *in, size_t len, UBYTE *out,
                        struct Note *notes, int maxNotes, int *nNotes, size_t *used);

/* The pitch of note number n (1-84, 34 = O2 A = 440 Hz) in whole Hz. */
UWORD AnsiMusic_Hz(int n);

#endif /* ANSIMUSIC_H */
