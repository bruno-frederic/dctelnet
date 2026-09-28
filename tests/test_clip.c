/* test/test_clip.c -- terminal text to and from the clipboard. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "clip.h"
#include "charset.h"

static void test_cp437_keeps_letters_and_draws_boxes_in_ascii(void)
{
    assert(Charset_Cp437ToLatin1('A') == 'A');
    assert(Charset_Cp437ToLatin1(0x82) == 0xE9);        /* e acute */
    assert(Charset_Cp437ToLatin1(0x9C) == 0xA3);        /* pound */
    assert(Charset_Cp437ToLatin1(0xE1) == 0xDF);        /* sharp s */
    assert(Charset_Cp437ToLatin1(0xC4) == '-' && Charset_Cp437ToLatin1(0xB3) == '|');
    assert(Charset_Cp437ToLatin1(0xDA) == '+' && Charset_Cp437ToLatin1(0xDB) == '#');
    assert(Charset_Cp437ToLatin1(0xFF) == 0xA0 && Charset_Cp437ToLatin1(0x07) == ' ');
}

static void test_petscii_letters_follow_the_case_set(void)
{
    assert(Charset_PetsciiToLatin1('A', FALSE) == 'A');
    assert(Charset_PetsciiToLatin1('A', TRUE) == 'a');
    assert(Charset_PetsciiToLatin1(0xC1, TRUE) == 'A');
    assert(Charset_PetsciiToLatin1(0xC1, FALSE) == '#');     /* a graphic */
    assert(Charset_PetsciiToLatin1('5', TRUE) == '5' && Charset_PetsciiToLatin1(0x5C, FALSE) == 0xA3);
}

static void test_selection_is_ordered_and_spans_rows(void)
{
    struct ClipRange r;
    UWORD from, to;

    Clip_Order(5, 10, 3, 20, &r);              /* dragged up */
    assert(r.startRow == 3 && r.startCol == 20 && r.endRow == 5 && r.endCol == 10);
    assert(Clip_RowSpan(&r, 3, 80, &from, &to) && from == 20 && to == 80);
    assert(Clip_RowSpan(&r, 4, 80, &from, &to) && from == 1 && to == 80);
    assert(Clip_RowSpan(&r, 5, 80, &from, &to) && from == 1 && to == 10);
    assert(!Clip_RowSpan(&r, 6, 80, &from, &to));
    Clip_Order(2, 9, 2, 4, &r);                /* dragged left on one row */
    assert(Clip_RowSpan(&r, 2, 80, &from, &to) && from == 4 && to == 9);
}

static void test_row_text_drops_trailing_blanks_and_converts(void)
{
    static const UBYTE row[] = { 'H',7,0,0, 0x82,7,0,0, ' ',7,0,0, 0xC4,7,0,0, ' ',7,0,0, ' ',7,0,0 };
    char out[16];
    size_t n = Clip_RowText(row, 6, 1, 6, FALSE, FALSE, out);

    assert(n == 4 && memcmp(out, "H\xe9 -", 4) == 0);
    assert(Clip_RowText(row, 6, 2, 2, FALSE, FALSE, out) == 1 && (UBYTE)out[0] == 0xE9);
    assert(Clip_RowText(row, 6, 5, 6, FALSE, FALSE, out) == 0);
}

static void test_ftxt_round_trip(void)
{
    UBYTE iff[64];
    char text[64];
    size_t n = Clip_BuildFtxt("odd", 3, iff, sizeof(iff));

    assert(n == 24 && memcmp(iff, "FORM", 4) == 0 && iff[7] == 16);
    assert(memcmp(iff + 8, "FTXTCHRS", 8) == 0 && iff[19] == 3 && iff[23] == 0);
    assert(Clip_ParseFtxt(iff, n, text, sizeof(text)) == 3 && memcmp(text, "odd", 3) == 0);
    assert(Clip_BuildFtxt("odd", 3, iff, 23) == 0);
}

/* Another program's clip: other chunks around the text, two CHRS. */
static void test_parse_skips_other_chunks(void)
{
    static const UBYTE iff[] = {
        'F','O','R','M', 0,0,0,38, 'F','T','X','T',
        'F','O','N','S', 0,0,0,3, 1,2,3,0,
        'C','H','R','S', 0,0,0,2, 'h','i',
        'C','H','R','S', 0,0,0,2, '!','!' };
    char text[16];

    assert(Clip_ParseFtxt(iff, sizeof(iff), text, sizeof(text)) == 4 && memcmp(text, "hi!!", 4) == 0);
    assert(Clip_ParseFtxt((const UBYTE *)"FORM\0\0\0\4ILBM", 12, text, sizeof(text)) == 0);
}

/* A clip larger than the read buffer: the text that was read is pasted. */
static void test_a_clip_larger_than_the_buffer_pastes_its_start(void)
{
    UBYTE iff[64];
    char text[64];
    size_t n = Clip_BuildFtxt("a long line", 11, iff, sizeof(iff));

    assert(n == 32);
    assert(Clip_ParseFtxt(iff, 25, text, sizeof(text)) == 5 && memcmp(text, "a lon", 5) == 0);
    assert(Clip_ParseFtxt(iff, 20, text, sizeof(text)) == 0);
}

/* Pasted lines are typed: Return is CR (CR LF with Return = CR + LF), and
 * on telnet a 255 byte is doubled, as OutKey() does for a key. */
static void test_paste_types_returns_and_doubles_iac(void)
{
    char out[32];
    size_t n = Clip_PasteBytes("a\nb\r\nc\r", 7, FALSE, TRUE, out);

    assert(n == 6 && memcmp(out, "a\rb\rc\r", 6) == 0);
    n = Clip_PasteBytes("x\n", 2, TRUE, TRUE, out);
    assert(n == 3 && memcmp(out, "x\r\n", 3) == 0);
    n = Clip_PasteBytes("\xff", 1, FALSE, TRUE, out);
    assert(n == 2 && (UBYTE)out[0] == 0xFF && (UBYTE)out[1] == 0xFF);
    assert(Clip_PasteBytes("\xff", 1, FALSE, FALSE, out) == 1);
}

int main(void)
{
    test_cp437_keeps_letters_and_draws_boxes_in_ascii();
    test_petscii_letters_follow_the_case_set();
    test_selection_is_ordered_and_spans_rows();
    test_row_text_drops_trailing_blanks_and_converts();
    test_ftxt_round_trip();
    test_parse_skips_other_chunks();
    test_a_clip_larger_than_the_buffer_pastes_its_start();
    test_paste_types_returns_and_doubles_iac();
    printf("clip: all assertions passed\n");
    return 0;
}
