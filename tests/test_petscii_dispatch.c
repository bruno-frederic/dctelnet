/* test/test_petscii_dispatch.c -- control dispatch and the two render paths. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "petscii_dispatch.h"

/* Fresh state, translate `in` on the ANSI path, compare exactly. */
static void expect_ansi(const uint8_t *in, size_t in_len, const uint8_t *expected, size_t exp_len) {
    struct PetsciiDispatchState st;
    uint8_t out[256];
    size_t n;

    petscii_dispatch_init(&st, 40, 25);
    n = petscii_stream_to_ansi(&st, in, in_len, out, sizeof(out));
    assert(n == exp_len);
    assert(memcmp(out, expected, exp_len) == 0);
}
#define EXPECT_ANSI(in, expected) expect_ansi(in, sizeof(in), expected, sizeof(expected))

static void test_state_tracks_reverse_shift_and_cursor(void) {
    struct PetsciiDispatchState st;
    uint8_t code; int is_control;

    petscii_dispatch_init(&st, 40, 25);
    petscii_dispatch_byte(&st, 18, &code, &is_control);
    assert(is_control && st.reverse == 1);
    petscii_dispatch_byte(&st, 14, &code, &is_control);
    assert(st.shift_lowercase == 1);
    petscii_dispatch_byte(&st, 147, &code, &is_control); /* CLR keeps shift + reverse */
    assert(st.shift_lowercase == 1 && st.reverse == 1);
    petscii_dispatch_byte(&st, 'A', &code, &is_control);
    assert(!is_control && st.cursor_col == 1);
}

/* amiexpress-web sends ONE bare $0D per newline (sdk/petscii/
 * ascii-to-petscii.ts). On a C64 that moves to the next line; on the ANSI
 * console a bare CR only returns to column 0, so every line overwrote the
 * previous one. */
static void test_cr_starts_a_new_line(void) {
    const uint8_t plain[] = { 13 };
    const uint8_t plain_out[] = { 13, 10 };
    const uint8_t rvs[] = { 18, 13 };
    const uint8_t rvs_out[] = { 27,'[','0','m', 27,'[','7','m', 13, 10, 27,'[','0','m' };
    const uint8_t shifted[] = { 18, 141 }; /* shifted CR keeps reverse */
    const uint8_t shifted_out[] = { 27,'[','0','m', 27,'[','7','m', 13, 10 };

    EXPECT_ANSI(plain, plain_out);
    EXPECT_ANSI(rvs, rvs_out);
    EXPECT_ANSI(shifted, shifted_out);
}

/* The translator owns the 40-column wrap: after column 40 it emits CR+LF
 * itself, and DCTelnet turns the console's own auto-wrap off with
 * PETSCII_CONSOLE_SETUP. With both wrapping, a 40-column console (16-pixel
 * font in a 640-pixel window) got two line feeds per full row: every
 * second line of a logo was black. */
static void test_wraps_once_at_column_40(void) {
    struct PetsciiDispatchState st;
    uint8_t in[41], out[128];
    size_t n;

    memset(in, 'A', sizeof(in));
    petscii_dispatch_init(&st, 40, 25);
    n = petscii_stream_to_rawglyphs(&st, in, sizeof(in), out, sizeof(out));
    assert(n == 43);
    assert(out[40] == 13 && out[41] == 10 && out[42] == 'A');
    assert(strcmp(PETSCII_CONSOLE_SETUP, "\x1b[?7l") == 0);
}

static void test_cursor_and_clear(void) {
    const uint8_t cursor[] = { 145, 17, 29, 157 };
    const uint8_t cursor_out[] = { 27,'[','A', 27,'[','B', 27,'[','C', 27,'[','D' };
    const uint8_t clear[] = { 147 };
    const uint8_t clear_out[] = { 27,'[','2','J', 27,'[','H' };

    EXPECT_ANSI(cursor, cursor_out);
    EXPECT_ANSI(clear, clear_out);
}

static void test_raw_path_passes_the_petscii_byte(void) {
    struct PetsciiDispatchState st;
    const uint8_t in[] = { 0x41, 0xA0, 0x61 };
    uint8_t out[16];
    size_t n;

    petscii_dispatch_init(&st, 40, 25);
    n = petscii_stream_to_rawglyphs(&st, in, sizeof(in), out, sizeof(out));
    assert(n == 3 && memcmp(out, in, 3) == 0);
}

/* ibmcon.device emulates the IBM PC ANSI.SYS console, which has no SGR 27:
 * "\x1b[27m" was ignored, reverse video never ended, and a title screen of
 * reverse-space blocks came out as solid white. */
static void test_reverse_off_restores_colour_without_sgr27(void) {
    const uint8_t in[] = { 0x1C, 18, 146 }; /* red, RVS ON, RVS OFF */
    const uint8_t expected[] = {
        27,'[','0','m', 27,'[','3','1','m',
        27,'[','0','m', 27,'[','3','1','m', 27,'[','7','m',
        27,'[','0','m', 27,'[','3','1','m' };

    EXPECT_ANSI(in, expected);
}

static void test_no_byte_ever_emits_sgr27(void) {
    struct PetsciiDispatchState st;
    uint8_t out[64];
    size_t n, i;
    int b;

    for (b = 0; b < 256; b++) {
        uint8_t in[2];
        in[0] = 18; in[1] = (uint8_t)b;
        petscii_dispatch_init(&st, 40, 25);
        n = petscii_stream_to_ansi(&st, in, 2, out, sizeof(out));
        for (i = 0; i + 3 < n; i++)
            assert(!(out[i] == '[' && out[i+1] == '2' && out[i+2] == '7' && out[i+3] == 'm'));
    }
}

/* A plain colour after a bright one: "\x1b[31m" alone left the bold of the
 * previous white on, so red drew as light red. */
static void test_plain_colour_after_bright_drops_bold(void) {
    const uint8_t in[] = { 0x05, 0x1C }; /* white, then red */
    const uint8_t expected[] = {
        27,'[','0','m', 27,'[','1','m', 27,'[','3','7','m',
        27,'[','0','m', 27,'[','3','1','m' };

    EXPECT_ANSI(in, expected);
}

static void test_colour_change_keeps_reverse(void) {
    const uint8_t in[] = { 18, 0x1C }; /* RVS ON, red */
    const uint8_t expected[] = {
        27,'[','0','m', 27,'[','7','m',
        27,'[','0','m', 27,'[','3','1','m', 27,'[','7','m' };

    EXPECT_ANSI(in, expected);
}

static void test_delete_and_bell(void) {
    const uint8_t del[] = { 20 };
    const uint8_t del_out[] = { 8, ' ', 8 };
    const uint8_t bel[] = { 7 };
    const uint8_t bel_out[] = { 7 };

    EXPECT_ANSI(del, del_out);
    EXPECT_ANSI(bel, bel_out);
}

/* Receive() translates in chunks of (buffer / PETSCII_MAX_OUT_PER_BYTE)
 * bytes, so a byte that expands further would be truncated. Every byte,
 * both paths, at column 0 and 39, from a fresh state and from reverse on
 * with a bright colour (the longest attribute rebuild). */
static void test_no_byte_expands_past_the_declared_maximum(void) {
    struct PetsciiDispatchState st;
    uint8_t out[64];
    size_t n;
    int b, col, raw, attrs;

    for (attrs = 0; attrs < 2; attrs++)
        for (raw = 0; raw < 2; raw++)
            for (col = 0; col < 40; col += 39)
                for (b = 0; b < 256; b++) {
                    uint8_t in = (uint8_t)b;
                    petscii_dispatch_init(&st, 40, 25);
                    st.cursor_col = col;
                    if (attrs) { st.reverse = 1; st.color = 1; }
                    n = raw ? petscii_stream_to_rawglyphs(&st, &in, 1, out, sizeof(out))
                            : petscii_stream_to_ansi(&st, &in, 1, out, sizeof(out));
                    assert(n <= PETSCII_MAX_OUT_PER_BYTE);
                }
}

int main(void) {
    test_state_tracks_reverse_shift_and_cursor();
    test_cr_starts_a_new_line();
    test_wraps_once_at_column_40();
    test_cursor_and_clear();
    test_raw_path_passes_the_petscii_byte();
    test_reverse_off_restores_colour_without_sgr27();
    test_no_byte_ever_emits_sgr27();
    test_plain_colour_after_bright_drops_bold();
    test_colour_change_keeps_reverse();
    test_delete_and_bell();
    test_no_byte_expands_past_the_declared_maximum();
    printf("petscii_dispatch: all assertions passed\n");
    return 0;
}
