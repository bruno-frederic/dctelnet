/* test/test_ansimusic.c -- ANSI music. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ansimusic.h"

static void test_pitch(void)
{
    assert(AnsiMusic_Hz(46) == 440);        /* O3 A: octave 3 starts at middle C (GW-BASIC PLAY) */
    assert(AnsiMusic_Hz(37) == 262);        /* middle C */
    assert(AnsiMusic_Hz(58) == 880 && AnsiMusic_Hz(34) == 220);
    assert(AnsiMusic_Hz(0) == 0 && AnsiMusic_Hz(85) == 0);
}

static struct AnsiMusic m;
static UBYTE out[256];
static struct Note notes[64];
static int n;

static size_t filter(const char *s)
{
    return AnsiMusic_Filter(&m, (const UBYTE *)s, strlen(s), out, notes, 64, &n);
}

/* The tune leaves the text; the text around it stays. */
static void test_music_is_taken_out_of_the_text(void)
{
    size_t len;

    AnsiMusic_Init(&m);
    len = filter("Hi\x1b[MFT120L4O3AC\x0e!");
    assert(len == 3 && memcmp(out, "Hi!", 3) == 0);
    assert(n == 4);                                         /* 2 notes, 2 gaps */
    assert(notes[0].hz == 440 && notes[0].ms == 437);       /* O3 A, quarter at 120 = 500 ms x 7/8 */
    assert(notes[1].hz == 0 && notes[1].ms == 63);
    assert(notes[2].hz == 262);                             /* O3 C = middle C */
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

/* A tune split between two network reads. */
static void test_split_tune(void)
{
    AnsiMusic_Init(&m);
    assert(filter("a\x1b[") == 1 && n == 0);
    assert(filter("NL8O4C") == 0 && n == 0);
    assert(filter("D\x0e" "b") == 1 && out[0] == 'b' && n == 4);
    assert(notes[0].hz == 523 && notes[0].ms == 218);       /* O4 C eighth = 250 x 7/8 */
    assert(notes[2].hz == 587);                             /* D */
}

static void test_sharps_dots_rests_octaves(void)
{
    AnsiMusic_Init(&m);
    filter("\x1b[|MLO3A#4.P8>C\x0e");
    assert(n == 3);
    assert(notes[0].hz == 466 && notes[0].ms == 750);       /* legato dotted quarter, no gap */
    assert(notes[1].hz == 0 && notes[1].ms == 250);         /* P8 */
    assert(notes[2].hz == 523);                             /* > then C: octave 4 */
}

int main(void)
{
    test_pitch();
    test_music_is_taken_out_of_the_text();
    test_other_sequences_pass();
    test_bare_delete_line_keeps_the_text();
    test_split_tune();
    test_sharps_dots_rests_octaves();
    printf("ansimusic: all assertions passed\n");
    return 0;
}
