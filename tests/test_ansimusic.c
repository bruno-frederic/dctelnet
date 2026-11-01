/* tests/test_ansimusic.c -- ANSI music. The full rules are checked against
 * the shared vectors (tools/check_ansimusic_vectors.py); these cases name the
 * behaviours one by one. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ansimusic.h"

/* GW-BASIC pitch: octave 6 table shifted down, rounded half up. O2 A is
 * concert A (N34); O3 C (N37) is 523 Hz. */
static void test_pitch(void)
{
    assert(AnsiMusic_Hz(34) == 440 && AnsiMusic_Hz(46) == 880);
    assert(AnsiMusic_Hz(37) == 523 && AnsiMusic_Hz(25) == 262 && AnsiMusic_Hz(49) == 1047);
    assert(AnsiMusic_Hz(1) == 65 && AnsiMusic_Hz(84) == 7902);
    assert(AnsiMusic_Hz(0) == 0 && AnsiMusic_Hz(85) == 0);
}

static struct AnsiMusic m;
static UBYTE out[256 + ANSIMUSIC_HELD];
static struct Note notes[64];
static int n;

static size_t used;

/* One call, as the display pipeline makes it. */
static size_t filter1(const char *s)
{
    return AnsiMusic_Filter(&m, (const UBYTE *)s, strlen(s), out, notes, 64, &n, &used);
}

/* The whole input, calling again after each foreground stop: the text out,
 * the notes of all calls in notes[], their count in n. */
static size_t filter(const char *s)
{
    static UBYTE part[256 + ANSIMUSIC_HELD];
    static struct Note all[64];
    size_t len = strlen(s), p = 0, o = 0;
    int total = 0, k;

    while (p < len)
    {
        size_t w = AnsiMusic_Filter(&m, (const UBYTE *)s + p, len - p, part, notes, 64, &n, &used);

        memcpy(out + o, part, w);
        o += w;
        for (k = 0; k < n; k++) all[total++] = notes[k];
        p += used;
    }
    memcpy(notes, all, sizeof(struct Note) * (size_t)total);
    n = total;
    return o;
}

/* The tune leaves the text; the text around it stays. */
static void test_music_is_taken_out_of_the_text(void)
{
    size_t len;

    AnsiMusic_Init(&m);
    len = filter("Hi\x1b[MFT120L4O2AO3C\x0e!");
    assert(len == 3 && memcmp(out, "Hi!", 3) == 0);
    assert(n == 4);                                         /* 2 notes, 2 gaps */
    assert(notes[0].hz == 440 && notes[0].ms == 437);       /* O2 A, quarter at 120 = 500 ms x 7/8 */
    assert(notes[1].hz == 0 && notes[1].ms == 63);
    assert(notes[2].hz == 523);                             /* O3 C */
}

/* ESC[2M deletes two lines, ESC[5;1H moves: not music. */
static void test_other_sequences_pass(void)
{
    static const char seq[] = "\x1b[2M\x1b[5;1Hx\x1b(B";
    size_t len;

    AnsiMusic_Init(&m);
    len = filter(seq);
    assert(len == sizeof(seq) - 1 && memcmp(out, seq, len) == 0 && n == 0);
}

/* A bare ESC[M (delete one line) followed by text is not music: the text
 * shows, also when the ESC[M ends one network read. */
static void test_bare_delete_line_keeps_the_text(void)
{
    static const char seq[] = "\x1b[MHello\x1b[M\x1b[1;1Hx";

    AnsiMusic_Init(&m);
    assert(filter(seq) == sizeof(seq) - 1 && memcmp(out, seq, sizeof(seq) - 1) == 0 && n == 0);
    assert(filter("\x1b[M") == 0);
    assert(filter("Hi") == 5 && memcmp(out, "\x1b[MHi", 5) == 0 && n == 0);
}

/* ESC[M then spaces then music is a tune (!ANSIMUS.ANS); then spaces and
 * text is a Delete Line that keeps the spaces, also split over reads. */
static void test_spaces_after_esc_m(void)
{
    AnsiMusic_Init(&m);
    assert(filter("\x1b[M  T200C\x0e") == 0 && n == 2 && notes[0].hz == 1047);
    assert(filter("\x1b[M ") == 0 && n == 0);
    assert(filter(" x") == 6 && memcmp(out, "\x1b[M  x", 6) == 0 && n == 0);
    assert(filter("\x1b[M         C") == 13 && n == 0);    /* 9 spaces: not music */
}

/* ESC[MN34 plays note 34; ESC[MNC sets normal style then plays C;
 * lower case ESC[n is not music. */
static void test_m_then_n(void)
{
    AnsiMusic_Init(&m);
    filter("\x1b[MN34\x0e");
    assert(n == 2 && notes[0].hz == 440 && notes[0].ms == 437);
    filter("\x1b[MLMNC\x0e");
    assert(n == 2 && notes[0].hz == 1047 && notes[0].ms == 437);
    assert(filter("\x1b[n") == 3 && n == 0);
}

/* A tune split between two network reads. */
static void test_split_tune(void)
{
    AnsiMusic_Init(&m);
    assert(filter("a\x1b[") == 1 && n == 0);
    assert(filter("NL8O4C") == 0 && n == 0);
    assert(filter("D\x0e" "b") == 1 && out[0] == 'b' && n == 4);
    assert(notes[0].hz == 1047 && notes[0].ms == 218);      /* O4 C eighth = 250 x 7/8 */
    assert(notes[2].hz == 1175);                            /* D */
}

static void test_sharps_dots_rests_octaves(void)
{
    AnsiMusic_Init(&m);
    filter("\x1b[|MLO2A#4.P8>C\x0e");
    assert(n == 3);
    assert(notes[0].hz == 466 && notes[0].ms == 750);       /* legato dotted quarter, no gap */
    assert(notes[1].hz == 0 && notes[1].ms == 250);         /* P8 */
    assert(notes[2].hz == 523);                             /* > then C: octave 3 */
}

/* Each dot multiplies by 1.5 again: a double-dotted quarter is 1125 ms. */
static void test_dots_compound(void)
{
    AnsiMusic_Init(&m);
    filter("\x1b[NMLC4..\x0e");
    assert(n == 1 && notes[0].ms == 1125);
}

/* Out-of-range values clamp: T999 is T255, L99 is L64, O9 is O6. */
static void test_clamps(void)
{
    AnsiMusic_Init(&m);
    filter("\x1b[NMLT999L99O9C\x0e");
    assert(n == 1 && notes[0].hz == 4186 && notes[0].ms == 14);     /* 240000 / (255 x 64) */
    filter("\x1b[NT10L1C\x0e");
    assert(n == 1 && notes[0].ms == 7500);                          /* still ML; T32 */
}

/* A string with ';' is a SMILE sound effect: no notes, no text. */
static void test_smile_effect(void)
{
    AnsiMusic_Init(&m);
    assert(filter("\x1b[MF C 7000;.12;200\x0ex") == 1 && out[0] == 'x' && n == 0);
}

/* ESC ends a string early: the note being read plays, the ESC sequence
 * stays. CAN ends it too and is swallowed. */
static void test_early_end(void)
{
    AnsiMusic_Init(&m);
    assert(filter("\x1b[NCD\x1b[1mx") == 5 && memcmp(out, "\x1b[1mx", 5) == 0);
    assert(n == 4 && notes[2].hz == 1175);
    assert(filter("\x1b[NE\x18y") == 1 && out[0] == 'y' && n == 2 && notes[0].hz == 1319);
}

/* Owner ear test: "AFTER MUSIC" showed while the scale played. A foreground
 * tune stops the filter at its end; the text after it comes on the next call. */
static void test_text_after_a_foreground_tune_waits(void)
{
    static const char in[] = "\x1b[MFT120O3L4CDE\x0e" "AFTER";
    size_t end = strlen("\x1b[MFT120O3L4CDE\x0e");

    AnsiMusic_Init(&m);
    assert(filter1(in) == 0 && used == end && n == 6);
    assert(filter1(in + used) == 5 && memcmp(out, "AFTER", 5) == 0 && used == 5 && n == 0);
}

/* No MF needed: foreground is the default (SyncTERM, GW-BASIC PLAY). */
static void test_default_is_foreground(void)
{
    static const char in[] = "\x1b[MT120CDE\x0e" "AFTER";

    AnsiMusic_Init(&m);
    assert(filter1(in) == 0 && used == strlen("\x1b[MT120CDE\x0e") && n == 6);
    AnsiMusic_Init(&m);
    assert(filter1("\x1b[NCDE\x0e" "AFTER") == 0 && used == 7 && n == 6);     /* ESC[N too */
}

/* MB plays in the background: the text does not wait. */
static void test_background_does_not_wait(void)
{
    AnsiMusic_Init(&m);
    assert(filter1("\x1b[MBCDE\x0e" "AFTER") == 5 && memcmp(out, "AFTER", 5) == 0);
    assert(used == strlen("\x1b[MBCDE\x0e" "AFTER") && n == 6);
}

/* MB stays in force for the strings after it; MF inside a string after MB
 * makes that string foreground (a note is queued while MF is on). */
static void test_background_persists_and_mid_string_mf(void)
{
    AnsiMusic_Init(&m);
    assert(filter1("\x1b[MBC\x0e") == 0 && used == 6);
    assert(filter1("\x1b[MCD\x0e X") == 2 && used == 8);          /* second string: still background */
    assert(filter1("\x1b[MFCD\x0e X") == 0 && used == 7);         /* MF again: foreground */
    AnsiMusic_Init(&m);
    filter1("\x1b[MBC\x0e");
    assert(filter1("\x1b[MBCMFD\x0e" "X") == 0 && used == 9 && n == 4);
}

/* SMILE plays nothing, so it never holds the text. */
static void test_smile_never_waits(void)
{
    AnsiMusic_Init(&m);
    assert(filter1("\x1b[MFC 7000;.12;200\x0ex") == 1 && out[0] == 'x' && n == 0);
}

/* A string ended early by ESC is foreground too; the ESC sequence after it
 * still parses when the rest is filtered afterwards. */
static void test_early_end_by_esc_waits(void)
{
    AnsiMusic_Init(&m);
    assert(filter1("\x1b[MC\x1b[1mX") == 0 && used == 5 && n == 2);
    assert(filter1("[1mX") == 5 && memcmp(out, "\x1b[1mX", 5) == 0 && used == 4);
}

int main(void)
{
    test_pitch();
    test_music_is_taken_out_of_the_text();
    test_other_sequences_pass();
    test_bare_delete_line_keeps_the_text();
    test_spaces_after_esc_m();
    test_m_then_n();
    test_split_tune();
    test_sharps_dots_rests_octaves();
    test_dots_compound();
    test_clamps();
    test_smile_effect();
    test_early_end();
    test_text_after_a_foreground_tune_waits();
    test_default_is_foreground();
    test_background_does_not_wait();
    test_background_persists_and_mid_string_mf();
    test_smile_never_waits();
    test_early_end_by_esc_waits();
    printf("ansimusic: all assertions passed\n");
    return 0;
}
