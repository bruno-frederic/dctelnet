/* src/ansimusic.c -- ANSI music (as GW-BASIC PLAY, SyncTERM). The rules and
 * the test vectors: tools/ansimusic_reference.py. */
#include "ansimusic.h"

enum { ST_TEXT, ST_ESC, ST_CSI, ST_CSI_M, ST_MUSIC };
enum { STYLE_NORMAL = 7, STYLE_LEGATO = 8, STYLE_STACCATO = 6 };    /* eighths played */
enum { MAX_SPACES = 8, MAX_DOTS = 6 };

/* GW-BASIC NOTTAB: octave 6 in whole Hz, C C# D D# E F F# G G# A A# B. */
static const UWORD octave6[12] = { 4186, 4435, 4699, 4978, 5274, 5588,
                                   5920, 6272, 6645, 7040, 7459, 7902 };

UWORD AnsiMusic_Hz(int n)
{
    int shift;
    UWORD v;

    if (n < 1 || n > 84)
        return 0;
    v = octave6[(n - 1) % 12];
    shift = 6 - (n - 1) / 12;
    if (shift == 0)
        return v;
    return (UWORD)((v >> shift) + ((v >> (shift - 1)) & 1));    /* SHR, then ADC: half up */
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

/* A note or rest (hz 0) of length len (1 whole .. 64) with the pending
 * dots, played in the style. Length in ms = 240000 / (T * L) x 1.5^dots,
 * kept as one fraction so the sound and the silence add up to it. */
static void play(struct AnsiMusic *m, struct Note *notes, int maxNotes, int *n, UWORD hz, UWORD len)
{
    ULONG num = 240000UL, den = (ULONG)m->tempo * len, ms, sound;
    int d;

    for (d = 0; d < m->dots && d < MAX_DOTS; d++) { num *= 3; den *= 2; }
    ms = num / den;
    if (hz == 0) { add(notes, maxNotes, n, 0, ms); return; }
    sound = num * m->style / (den * 8);
    add(notes, maxNotes, n, hz, sound);
    add(notes, maxNotes, n, 0, ms - sound);
}

static UWORD clamp(UWORD v, UWORD lo, UWORD hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* The command collected so far (m->cmd with its number) is complete. */
static void finish(struct AnsiMusic *m, struct Note *notes, int maxNotes, int *n)
{
    char c = m->cmd;
    UWORD v = m->num;
    BOOL has = m->hasNum;

    if (c == 'n')                               /* M then N, no digit: MN */
        m->style = STYLE_NORMAL;
    else if (c >= 'A' && c <= 'G')
    {
        static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 };     /* A B C D E F G */
        int note = m->octave * 12 + base[c - 'A'] + m->sharp - m->flat + 1;

        play(m, notes, maxNotes, n, AnsiMusic_Hz(note), has && v ? clamp(v, 1, 64) : m->length);
    }
    else if (c == 'N' && has && v <= 84) play(m, notes, maxNotes, n, AnsiMusic_Hz(v), m->length);
    else if (c == 'P') play(m, notes, maxNotes, n, 0, has && v ? clamp(v, 1, 64) : m->length);
    else if (c == 'T' && has) m->tempo = clamp(v, 32, 255);
    else if (c == 'L' && has) m->length = (UBYTE)clamp(v, 1, 64);
    else if (c == 'O' && has) m->octave = (UBYTE)clamp(v, 0, 6);
    m->cmd = 0;
    m->dots = m->flat = m->sharp = 0;
    m->num = 0;
    m->hasNum = FALSE;
}

/* What may follow ESC[M (and its spaces) in a tune: a command of the music
 * language, or the tune's end. Anything else makes it a Delete Line. */
static BOOL MusicStarts(UBYTE c)
{
    if (c >= 'a' && c <= 'z') c = (UBYTE)(c - 32);
    return (c >= 'A' && c <= 'G') || c == 'N' || c == 'P' || c == 'T' || c == 'L'
        || c == 'O' || c == 'S' || c == '<' || c == '>' || c == 0x0E;
}

static void StartString(struct AnsiMusic *m, char cmd)
{
    m->state = ST_MUSIC;
    m->cmd = cmd;
    m->num = 0;
    m->hasNum = FALSE;
    m->dots = m->flat = m->sharp = 0;
    m->smile = FALSE;
}

/* The string ends: its last command plays (not in a SMILE effect). */
static void EndString(struct AnsiMusic *m, struct Note *notes, int maxNotes, int *n)
{
    if (!m->smile && m->cmd && m->cmd != 'M') finish(m, notes, maxNotes, n);
    m->cmd = 0;
    m->smile = FALSE;
}

size_t AnsiMusic_Filter(struct AnsiMusic *m, const UBYTE *in, size_t len, UBYTE *out,
                        struct Note *notes, int maxNotes, int *nNotes)
{
    size_t i, o = 0;
    int first = 0;                              /* the first note of this string in notes[] */

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
                StartString(m, 0);                  /* a music string follows */
                first = *nNotes;
            }
            else if (c == 'M')
            {
                m->state = ST_CSI_M;                /* music, or Delete Line */
                m->spaces = 0;
            }
            else { out[o++] = 0x1B; out[o++] = '['; out[o++] = c; m->state = ST_TEXT; }
            break;
        case ST_CSI_M:                              /* ESC [ M, spaces, then ... */
            if (c == ' ' && m->spaces < MAX_SPACES)
            {
                m->spaces++;
                break;
            }
            if (c != ' ' && MusicStarts(c))
            {
                StartString(m, 'M');                /* MF/MB/MN... or plain music */
                first = *nNotes;
            }
            else
            {
                out[o++] = 0x1B; out[o++] = '['; out[o++] = 'M';
                while (m->spaces) { out[o++] = ' '; m->spaces--; }
                m->state = ST_TEXT;                 /* Delete Line: c is text again */
            }
            i--;                                    /* c once more, in the new state */
            break;
        case ST_MUSIC:
            if (c == 0x0E || c == 0x1B || c == 0x18 || c == 0x1A)
            {                                       /* the end; ESC, CAN, SUB: early */
                EndString(m, notes, maxNotes, nNotes);
                m->state = c == 0x1B ? ST_ESC : ST_TEXT;
                break;
            }
            if (c == ';' && !m->smile)              /* a SMILE sound effect: no tune */
            {
                m->smile = TRUE;
                *nNotes = first;
            }
            if (m->smile)
                break;
            if (c >= 'a' && c <= 'z') c = (UBYTE)(c - 32);
            if (m->cmd == 'M')                      /* the letter after M */
            {
                m->cmd = 0;
                if (c == 'F' || c == 'B') break;
                if (c == 'L') { m->style = STYLE_LEGATO; break; }
                if (c == 'S') { m->style = STYLE_STACCATO; break; }
                if (c == 'N') { m->cmd = 'n'; break; }      /* MN, or note N n */
            }
            if (m->cmd == 'n')
            {
                if (c >= '0' && c <= '9') m->cmd = 'N';
                else finish(m, notes, maxNotes, nNotes);    /* MN */
            }
            if (c >= '0' && c <= '9' && m->cmd)
            {
                m->num = (UWORD)(m->num > 6553 ? 65535 : m->num * 10 + (c - '0'));
                m->hasNum = TRUE;
            }
            else if ((c == '#' || c == '+') && m->cmd >= 'A' && m->cmd <= 'G') m->sharp = 1;
            else if (c == '-' && m->cmd >= 'A' && m->cmd <= 'G') m->flat = 1;
            else if (c == '.' && m->cmd) m->dots++;
            else if (c == '>' || c == '<')
            {
                finish(m, notes, maxNotes, nNotes);
                if (c == '>' && m->octave < 6) m->octave++;
                if (c == '<' && m->octave > 0) m->octave--;
            }
            else if ((c >= 'A' && c <= 'G') || c == 'N' || c == 'P' || c == 'T' || c == 'L' || c == 'O'
                     || c == 'M')                   /* M: MN ML MS MF MB anywhere */
            {
                finish(m, notes, maxNotes, nNotes);
                m->cmd = (char)c;
            }
            /* anything else (space, CR, LF, '"', unknown letters): skipped */
            break;
        }
    }
    return o;
}
