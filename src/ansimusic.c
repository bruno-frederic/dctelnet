/* src/ansimusic.c -- ANSI music (as SyncTERM, BananaCom). */
#include "ansimusic.h"

enum { ST_TEXT, ST_ESC, ST_CSI, ST_CSI_M, ST_MUSIC };
enum { STYLE_NORMAL = 7, STYLE_LEGATO = 8, STYLE_STACCATO = 6 };    /* eighths played */

/* Octave 3 in Hz x 100: C C# D D# E F F# G G# A A# B (C4 is middle C). */
static const ULONG octave3[12] = { 26163, 27718, 29366, 31113, 32963, 34923,
                                   36999, 39200, 41530, 44000, 46616, 49388 };

UWORD AnsiMusic_Hz(int n)
{
    int semitone, octave;
    ULONG centi;

    if (n < 1 || n > 84)
        return 0;
    semitone = (n - 1) % 12;
    octave = (n - 1) / 12;              /* 0-6; octave 3 holds A 440 (n 46) */
    centi = octave3[semitone];
    if (octave > 3) centi <<= (octave - 3);
    else            centi >>= (3 - octave);
    return (UWORD)((centi + 50) / 100);
}

void AnsiMusic_Init(struct AnsiMusic *m)
{
    memset(m, 0, sizeof(*m));
    m->state = ST_TEXT;
    m->tempo = 120;
    m->length = 4;
    m->octave = 4;
    m->style = STYLE_NORMAL;
}

static void add(struct Note *notes, int maxNotes, int *n, UWORD hz, ULONG ms)
{
    if (ms == 0 || *n >= maxNotes)
        return;
    notes[*n].hz = hz;
    notes[*n].ms = (UWORD)(ms > 60000 ? 60000 : ms);
    (*n)++;
}

/* Milliseconds of a note of length len (1 whole .. 64) with dots. */
static ULONG duration(const struct AnsiMusic *m, UWORD len, UBYTE dots)
{
    ULONG whole = 240000UL / m->tempo;              /* 4 quarters */
    ULONG ms = whole / (len ? len : 1), add = ms;

    while (dots--) { add /= 2; ms += add; }
    return ms;
}

/* A note or rest (hz 0) of the pending command, played in the style. */
static void play(struct AnsiMusic *m, struct Note *notes, int maxNotes, int *n, UWORD hz, UWORD len)
{
    ULONG ms = duration(m, len, m->dots);

    if (hz == 0) { add(notes, maxNotes, n, 0, ms); return; }
    add(notes, maxNotes, n, hz, ms * m->style / 8);
    add(notes, maxNotes, n, 0, ms - ms * m->style / 8);
}

/* The command collected so far (m->cmd with its number) is complete. */
static void finish(struct AnsiMusic *m, struct Note *notes, int maxNotes, int *n)
{
    char c = m->cmd;
    UWORD v = m->num;

    m->cmd = 0;
    if (c >= 'A' && c <= 'G')
    {
        static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 };     /* A B C D E F G */
        int semi = base[c - 'A'] + m->sharp - m->flat;
        int note = m->octave * 12 + semi + 1;

        play(m, notes, maxNotes, n, AnsiMusic_Hz(note), m->hasNum && v ? v : m->length);
    }
    else if (c == 'N') play(m, notes, maxNotes, n, AnsiMusic_Hz(v), m->length);
    else if (c == 'P') play(m, notes, maxNotes, n, 0, m->hasNum && v ? v : m->length);
    else if (c == 'T' && v >= 32) m->tempo = v > 255 ? 255 : v;
    else if (c == 'L' && v >= 1 && v <= 64) m->length = (UBYTE)v;
    else if (c == 'O' && m->hasNum) m->octave = (UBYTE)(v > 6 ? 6 : v);
    m->dots = m->flat = m->sharp = 0;
    m->num = 0;
    m->hasNum = FALSE;
}

/* What may follow ESC[M in a tune: a command of the music language, or the
 * tune's end. Anything else makes it a Delete Line. */
static BOOL MusicStarts(UBYTE c)
{
    if (c >= 'a' && c <= 'z') c = (UBYTE)(c - 32);
    return (c >= 'A' && c <= 'G') || c == 'N' || c == 'P' || c == 'T' || c == 'L'
        || c == 'O' || c == 'M' || c == '<' || c == '>' || c == 0x0E;
}

size_t AnsiMusic_Filter(struct AnsiMusic *m, const UBYTE *in, size_t len, UBYTE *out,
                        struct Note *notes, int maxNotes, int *nNotes)
{
    size_t i, o = 0;

    *nNotes = 0;
    for (i = 0; i < len; i++)
    {
        UBYTE c = in[i];

        switch (m->state)
        {
        case ST_TEXT:
            if (c == 0x1B) m->state = ST_ESC;
            else out[o++] = c;
            break;
        case ST_ESC:
            if (c == '[') m->state = ST_CSI;
            else { out[o++] = 0x1B; out[o++] = c; m->state = ST_TEXT; }
            break;
        case ST_CSI:                                /* ESC [ then ... */
            if (c == 'N' || c == '|')
            {
                m->state = ST_MUSIC;                /* a music string follows */
                m->cmd = 0;
            }
            else if (c == 'M')
                m->state = ST_CSI_M;                /* music, or Delete Line */
            else { out[o++] = 0x1B; out[o++] = '['; out[o++] = c; m->state = ST_TEXT; }
            break;
        case ST_CSI_M:                              /* ESC [ M then ... */
            if (MusicStarts(c))
            {
                m->state = ST_MUSIC;
                m->cmd = 'M';                       /* MF/MB/MN... or plain music */
            }
            else
            {
                out[o++] = 0x1B; out[o++] = '['; out[o++] = 'M';
                m->state = ST_TEXT;                 /* Delete Line: c is text again */
            }
            i--;                                    /* c once more, in the new state */
            break;
        case ST_MUSIC:
            if (c == 0x0E)                          /* the end of the tune */
            {
                if (m->cmd && m->cmd != 'M') finish(m, notes, maxNotes, nNotes);
                m->cmd = 0;
                m->state = ST_TEXT;
                break;
            }
            if (c >= 'a' && c <= 'z') c = (UBYTE)(c - 32);
            if (m->cmd == 'M' && (c == 'F' || c == 'B' || c == 'N' || c == 'L' || c == 'S'))
            {
                if (c == 'N') m->style = STYLE_NORMAL;
                if (c == 'L') m->style = STYLE_LEGATO;
                if (c == 'S') m->style = STYLE_STACCATO;
                m->cmd = 0;
                break;
            }
            if (c >= '0' && c <= '9' && m->cmd && m->cmd != 'M')
            {
                m->num = (UWORD)(m->num * 10 + (c - '0'));
                m->hasNum = TRUE;
            }
            else if ((c == '#' || c == '+') && m->cmd >= 'A' && m->cmd <= 'G') m->sharp = 1;
            else if (c == '-' && m->cmd >= 'A' && m->cmd <= 'G') m->flat = 1;
            else if (c == '.' && m->cmd) m->dots++;
            else if (c == '>' || c == '<')
            {
                if (m->cmd && m->cmd != 'M') finish(m, notes, maxNotes, nNotes);
                if (c == '>' && m->octave < 6) m->octave++;
                if (c == '<' && m->octave > 0) m->octave--;
                m->cmd = 0;
            }
            else if ((c >= 'A' && c <= 'G') || c == 'N' || c == 'P' || c == 'T' || c == 'L' || c == 'O'
                     || c == 'M')                   /* M: MN ML MS MF MB anywhere */
            {
                if (m->cmd && m->cmd != 'M') finish(m, notes, maxNotes, nNotes);
                m->cmd = (char)c;
            }
            else if (m->cmd == 'M' && c != ' ')
                m->cmd = 0;                         /* ESC[M then something else */
            break;
        }
    }
    return o;
}
