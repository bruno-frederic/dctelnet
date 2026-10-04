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

/* DCTelnet's own messages ("Connection closed", "Looking up ...") are ASCII.
 * Printed through a C64 font, lower-case ASCII landed on graphics glyphs and
 * the text was unreadable. Translated, the letters read as intended; CSI and
 * ESC sequences (their final bytes are letters too) pass untouched. */
static void test_local_text_reads_in_the_upper_case_set(void) {
    struct PetsciiLocalText lt;
    char buf[] = "\x9b" "0mConnection closed\r\n";
    const char want[] = "\x9b" "0mCONNECTION CLOSED\r\n";

    petscii_local_text_init(&lt, 0);
    petscii_local_text(&lt, buf, sizeof(buf) - 1);
    assert(memcmp(buf, want, sizeof(want) - 1) == 0);
}

static void test_local_text_reads_in_the_lower_case_set(void) {
    struct PetsciiLocalText lt;
    /* The lower-case set shows a-z at $41-$5A and A-Z at $61-$7A: swap case. */
    char buf[] = "\x1b[1mLooking up";
    const char want[] = "\x1b[1mlOOKING UP";

    petscii_local_text_init(&lt, 1);
    petscii_local_text(&lt, buf, sizeof(buf) - 1);
    assert(memcmp(buf, want, sizeof(want) - 1) == 0);
}

/* A sequence split across two writes is still left alone. */
static void test_local_text_escape_spans_calls(void) {
    struct PetsciiLocalText lt;
    char a[] = "x\x1b[3", b[] = "2mok";

    petscii_local_text_init(&lt, 0);
    petscii_local_text(&lt, a, 5);
    petscii_local_text(&lt, b, 4);
    assert(a[0] == 'X');
    assert(b[0] == '2' && b[1] == 'm' && b[2] == 'O' && b[3] == 'K');
}

/* A packet with both charsets in it: each part is drawn in its own font,
 * so it is cut after the switch (it was drawn whole in the last one). */
static void test_a_packet_is_cut_after_each_charset_switch(void)
{
    const uint8_t both[] = { 'H', 'I', 14, 'l', 'o', 142, 'X' };
    const uint8_t none[] = { 'A', 'B', 'C' };

    assert(petscii_part_length(both, sizeof(both)) == 3);
    assert(petscii_part_length(both + 3, sizeof(both) - 3) == 3);
    assert(petscii_part_length(both + 6, 1) == 1);
    assert(petscii_part_length(none, sizeof(none)) == 3);
    assert(petscii_part_length(none, 0) == 0);
}


/* ---- Screen model and repaint: a C64 charset switch redraws the screen ---- */

static void feed(struct PetsciiDispatchState *st, const uint8_t *in, size_t n) {
    uint8_t out[4096];
    petscii_stream_to_rawglyphs(st, in, n, out, sizeof(out));
}

/* What a console shows after the repaint output: CSI r;c H, SGR text
 * (kept per cell as written since the last SGR 0) and printable bytes. */
struct VScreen { uint8_t ch[25][40]; char sgr[25][40][24]; int row, col; char cur[24]; };

static void vscreen_run(struct VScreen *v, const uint8_t *s, size_t n) {
    size_t i = 0;
    memset(v, 0, sizeof(*v));
    while (i < n) {
        if (s[i] == 27) {
            size_t j = i + 2;
            int p0 = 0, p1 = 0, second = 0;
            assert(i + 1 < n && s[i + 1] == '[');
            while (j < n && ((s[j] >= '0' && s[j] <= '9') || s[j] == ';')) {
                if (s[j] == ';') second = 1;
                else if (second) p1 = p1 * 10 + (s[j] - '0');
                else p0 = p0 * 10 + (s[j] - '0');
                j++;
            }
            assert(j < n);
            if (s[j] == 'H') { v->row = p0 - 1; v->col = p1 - 1; }
            else {
                assert(s[j] == 'm');
                if (p0 == 0) v->cur[0] = 0;
                assert(strlen(v->cur) + (j + 1 - i) < sizeof(v->cur));
                strncat(v->cur, (const char *)s + i, j + 1 - i);
            }
            i = j + 1;
            continue;
        }
        assert(v->row >= 0 && v->row < 25 && v->col >= 0 && v->col < 40);
        v->ch[v->row][v->col] = s[i];
        strcpy(v->sgr[v->row][v->col], v->cur);
        v->col++;
        i++;
    }
}

/* Printed cells land in the model with the colour and reverse they were
 * printed in; DEL blanks the cell left of the cursor. */
static void test_screen_model_records_cells(void) {
    struct PetsciiDispatchState st;
    const uint8_t in[] = { 18, 'A', 146, 0x1C, 'B', 'C', 20 };

    petscii_dispatch_init(&st, 40, 25);
    feed(&st, in, sizeof(in));
    assert(st.cells[0][0].byte == 'A' && st.cells[0][0].reverse == 1 && st.cells[0][0].color == -1);
    assert(st.cells[0][1].byte == 'B' && st.cells[0][1].reverse == 0 && st.cells[0][1].color == 2);
    assert(st.cells[0][2].byte == 0x20);          /* 'C' deleted */
    assert(st.cursor_col == 2);
}

/* A line feed on row 25 scrolls the screen, as the console does; cursor
 * down and right stop at the edges, as CSI B / CSI C do. */
static void test_screen_model_scrolls_and_clamps(void) {
    struct PetsciiDispatchState st;
    uint8_t in[64];
    int i, n = 0;

    petscii_dispatch_init(&st, 40, 25);
    in[n++] = 'X';
    for (i = 0; i < 24; i++) in[n++] = 13;        /* to row 25 */
    in[n++] = 'Y';
    in[n++] = 13;                                 /* scrolls */
    feed(&st, in, (size_t)n);
    assert(st.cursor_row == 24);
    assert(st.cells[0][0].byte == 0x20);          /* X scrolled off */
    assert(st.cells[23][0].byte == 'Y');
    for (i = 0; i < 5; i++) { uint8_t d = 17; feed(&st, &d, 1); }
    assert(st.cursor_row == 24);
    for (i = 0; i < 50; i++) { uint8_t r = 29; feed(&st, &r, 1); }
    assert(st.cursor_col == 39);
    { uint8_t clr = 147; feed(&st, &clr, 1); }
    assert(st.cells[23][0].byte == 0x20 && st.cursor_row == 0 && st.cursor_col == 0);
}

/* Dino's report: C*Base answers CTRL+L with $0E/$8E and on a C64 the whole
 * screen changes case. The repaint redraws every cell with its own
 * attributes and puts the cursor and the current attributes back. */
static void test_repaint_redraws_every_cell(void) {
    static struct VScreen v;
    static uint8_t out[PETSCII_REPAINT_MAX];
    struct PetsciiDispatchState st;
    const uint8_t in[] = { 18, 'A', 146, 0x1C, 'B', 13, 0x05, 'h', 'i', 14 };
    size_t n;
    int r, c, r2, c2;

    petscii_dispatch_init(&st, 40, 25);
    feed(&st, in, sizeof(in));
    n = petscii_repaint(&st, 1, out, sizeof(out));
    vscreen_run(&v, out, n);

    for (r = 0; r < 25; r++)
        for (c = 0; c < 40; c++)
            assert(v.ch[r][c] == st.cells[r][c].byte);
    /* same attributes <=> same SGR text, across the whole screen */
    for (r = 0; r < 25; r++) for (c = 0; c < 40; c++)
        for (r2 = 0; r2 < 2; r2++) for (c2 = 0; c2 < 4; c2++) {
            int same = st.cells[r][c].color == st.cells[r2][c2].color
                    && st.cells[r][c].reverse == st.cells[r2][c2].reverse;
            assert(same == (strcmp(v.sgr[r][c], v.sgr[r2][c2]) == 0));
        }
    assert(strcmp(v.sgr[0][0], "\x1b[0m\x1b[7m") == 0);
    assert(strcmp(v.sgr[0][1], "\x1b[0m\x1b[31m") == 0);
    assert(v.row == st.cursor_row && v.col == st.cursor_col);  /* cursor back */
    assert(strcmp(v.cur, "\x1b[0m\x1b[1m\x1b[37m") == 0);        /* white again */
}

/* Worst case -- every cell a different attribute from its neighbour, the
 * longest SGR text -- still fits PETSCII_REPAINT_MAX. */
static void test_repaint_fits_its_bound(void) {
    static uint8_t out[PETSCII_REPAINT_MAX + 64];
    struct PetsciiDispatchState st;
    int r, c;
    size_t n;

    petscii_dispatch_init(&st, 40, 25);
    for (r = 0; r < 25; r++)
        for (c = 0; c < 40; c++) {
            st.cells[r][c].byte = 'x';
            st.cells[r][c].color = (int8_t)((c & 1) ? 1 : 7);   /* bright white / bright yellow */
            st.cells[r][c].reverse = 1;
        }
    st.cursor_row = 24; st.cursor_col = 39; st.color = 1; st.reverse = 1;
    n = petscii_repaint(&st, 1, out, sizeof(out));
    assert(n <= PETSCII_REPAINT_MAX);
}

int main(void) {
    test_screen_model_records_cells();
    test_screen_model_scrolls_and_clamps();
    test_repaint_redraws_every_cell();
    test_repaint_fits_its_bound();
    test_local_text_reads_in_the_upper_case_set();
    test_local_text_reads_in_the_lower_case_set();
    test_local_text_escape_spans_calls();
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
    test_a_packet_is_cut_after_each_charset_switch();
    printf("petscii_dispatch: all assertions passed\n");
    return 0;
}
