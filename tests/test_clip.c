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
    size_t n = Clip_RowText(row, 6, 1, 6, CLIP_CP437, out);

    assert(n == 4 && memcmp(out, "H\xe9 -", 4) == 0);
    assert(Clip_RowText(row, 6, 2, 2, CLIP_CP437, out) == 1 && (UBYTE)out[0] == 0xE9);
    assert(Clip_RowText(row, 6, 5, 6, CLIP_CP437, out) == 0);
}

/* An Amiga BBS sends ISO-8859-1: copied as CP437 its a-umlaut (E4) became
 * a Greek letter. With the Amiga character set it stays. */
static void test_amiga_bbs_text_is_copied_as_it_is(void)
{
    static const UBYTE row[] = { 0xE4,7,0,0, 'x',7,0,0 };
    char out[4];

    assert(Clip_RowText(row, 2, 1, 2, CLIP_LATIN1, out) == 2 && (UBYTE)out[0] == 0xE4);
    assert(Clip_RowText(row, 2, 1, 1, CLIP_CP437, out) == 1 && out[0] == 'S');   /* sigma */
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

/* A UTF-8 BBS drew its boxes in UTF-8: three bytes a line piece, shown
 * as three wrong characters. They are CP437's box characters now. */
static void test_utf8_boxes_become_cp437(void)
{
    struct Utf8Decoder d;
    UBYTE out[32];
    const UBYTE in[] = "\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x90 caf\xc3\xa9 \xe2\x82\xac";
    size_t n;

    Charset_Utf8Init(&d);
    n = Charset_Utf8ToCp437(&d, in, sizeof(in) - 1, out);
    assert(n == 10);
    assert(out[0] == 0xDA && out[1] == 0xC4 && out[2] == 0xBF);         /* corners, line */
    assert(memcmp(out + 3, " caf", 4) == 0 && out[7] == 0x82);         /* e acute */
    assert(out[8] == ' ' && out[9] == '?');                             /* no euro in CP437 */
}

/* A character split between two network reads still decodes. */
static void test_utf8_split_between_reads(void)
{
    struct Utf8Decoder d;
    UBYTE out[8];

    Charset_Utf8Init(&d);
    assert(Charset_Utf8ToCp437(&d, (const UBYTE *)"\xe2\x96", 2, out) == 0);
    assert(Charset_Utf8ToCp437(&d, (const UBYTE *)"\x88x", 2, out) == 2 && out[0] == 0xDB && out[1] == 'x');
    assert(Charset_Utf8ToCp437(&d, (const UBYTE *)"\xc3" "A", 2, out) == 2 && out[0] == '?' && out[1] == 'A');
    /* Cut short by the next read's first byte: 2 bytes out of 1 in. */
    assert(Charset_Utf8ToCp437(&d, (const UBYTE *)"\xc3", 1, out) == 0);
    assert(Charset_Utf8ToCp437(&d, (const UBYTE *)"A", 1, out) == 2 && out[0] == '?' && out[1] == 'A');
}

static void test_typed_latin1_as_utf8(void)
{
    UBYTE out[8];
    size_t n = Charset_Latin1ToUtf8((const UBYTE *)"a\xe4", 2, out);

    assert(n == 3 && out[0] == 'a' && out[1] == 0xC3 && out[2] == 0xA4);
}

/* Copy Screen of a screen with one line of text pasted 24 Returns. */
static void test_blank_rows_below_the_text_are_dropped(void)
{
    static const char screen[] = "one\ntwo\n\n\n", blank[] = "\n\n", inner[] = "a\n\nb";

    assert(Clip_TrimEmptyLines(screen, sizeof(screen) - 1) == 7);
    assert(Clip_TrimEmptyLines(blank, sizeof(blank) - 1) == 0);
    assert(Clip_TrimEmptyLines(inner, sizeof(inner) - 1) == 4);    /* inner blank lines stay */
}

int main(void)
{
    test_blank_rows_below_the_text_are_dropped();
    test_utf8_boxes_become_cp437();
    test_utf8_split_between_reads();
    test_typed_latin1_as_utf8();
    test_cp437_keeps_letters_and_draws_boxes_in_ascii();
    test_petscii_letters_follow_the_case_set();
    test_selection_is_ordered_and_spans_rows();
    test_row_text_drops_trailing_blanks_and_converts();
    test_amiga_bbs_text_is_copied_as_it_is();
    test_ftxt_round_trip();
    test_parse_skips_other_chunks();
    test_a_clip_larger_than_the_buffer_pastes_its_start();
    test_paste_types_returns_and_doubles_iac();
    printf("clip: all assertions passed\n");
    return 0;
}
