/* test/test_ansiscan.c -- reading and writing ANSI text. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ansiscan.h"

/* CSI ?25h: the scroll back stopped at the '?' and kept "25h" as text. */
static void test_private_parameters_are_skipped_whole(void)
{
    const UBYTE s[] = "\x9b?25hX";

    assert(Ansi_ParamsEnd(s, 1, 6) == 4 && s[4] == 'h');
    assert(Ansi_ParamsEnd((const UBYTE *)"\x9b" "1;31mA", 1, 7) == 5);
    assert(Ansi_ParamsEnd((const UBYTE *)"\x9b=3l", 1, 4) == 3);
}

static void test_find_ignores_case(void)
{
    assert(Ansi_FindText("Welcome to the BBS", 18, "bbs") == 15);
    assert(Ansi_FindText("\xc4rger", 5, "\xe4rg") == 0);          /* Latin-1 letters */
    assert(Ansi_FindText("abc", 3, "abcd") == -1);
    assert(Ansi_FindText("abc", 3, "") == -1);
}

static void cell(UBYTE *c, char ch, UBYTE fg, UBYTE bg, UBYTE flags)
{
    c[0] = (UBYTE)ch; c[1] = fg; c[2] = bg; c[3] = flags;
}

static void test_screen_row_as_ansi(void)
{
    UBYTE row[6 * 4];
    char out[256];
    ULONG attr = ANSI_ATTR_RESET;
    size_t n;

    cell(row, 'H', 7, 0, 0);
    cell(row + 4, 'i', 7, 0, 0);
    cell(row + 8, '!', 9, 4, 0x10);               /* bold red on blue */
    cell(row + 12, ' ', 7, 0, 0);
    cell(row + 16, ' ', 7, 0, 0);
    cell(row + 20, ' ', 7, 0, 0);
    n = Ansi_ScreenRow(row, 6, FALSE, &attr, out);
    out[n] = 0;
    assert(strcmp(out, "\033[0;37;40mHi\033[0;1;31;44m!\r\n") == 0);
    /* The next row starts in the attributes the last one ended with. */
    cell(row, 'x', 9, 4, 0x10);
    n = Ansi_ScreenRow(row, 1, FALSE, &attr, out);
    out[n] = 0;
    assert(strcmp(out, "x\r\n") == 0);
}

/* Blank cells with a coloured background show: they are kept. */
static void test_coloured_blanks_are_kept_and_ice_is_blink(void)
{
    UBYTE row[2 * 4];
    char out[128];
    ULONG attr = ANSI_ATTR_RESET;
    size_t n;

    cell(row, ' ', 7, 12, 0);                      /* iCE bright blue background */
    cell(row + 4, ' ', 7, 0, 0);
    n = Ansi_ScreenRow(row, 2, FALSE, &attr, out);
    out[n] = 0;
    assert(strcmp(out, "\033[0;5;37;44m \r\n") == 0);
}

/* Without a pen table ibmcon's unit order swaps pens 1 and 7. */
static void test_unit_order_pens_are_swapped_back(void)
{
    UBYTE row[4];
    char out[64];
    ULONG attr = ANSI_ATTR_RESET;

    cell(row, 'r', 7, 0, 0);                       /* pen 7 is red in unit order */
    out[Ansi_ScreenRow(row, 1, TRUE, &attr, out)] = 0;
    assert(strcmp(out, "\033[0;31;40mr\r\n") == 0);
}

static void test_bells_are_taken_out(void)
{
    UBYTE text[] = "a\ab\a\a";
    size_t len = sizeof(text) - 1;

    assert(Ansi_StripByte(text, &len, 7) == 3 && len == 2 && memcmp(text, "ab", 2) == 0);
    assert(Ansi_StripByte(text, &len, 7) == 0 && len == 2);
}

static void swap(struct AnsiSwap17 *st, const char *in, char *out)
{
    size_t l = strlen(out), n = Ansi_Swap17(st, (const UBYTE *)in, strlen(in), (UBYTE *)out + l);
    out[l + n] = 0;
}

/* console.device: SGR 31 is pen 1, which holds white (ibmcon's order) -- red
 * and white were swapped. The colours of SGR sequences swap 1 and 7; nothing
 * else changes. */
static void test_console_colours_swap_red_and_white_in_sgr_only(void)
{
    struct AnsiSwap17 st;
    char out[256];

    memset(&st, 0, sizeof(st)); out[0] = 0;
    swap(&st, "\x1b[31mred\x1b[37;41mw\x1b[1;91;107m\x1b[0m", out);
    assert(strcmp(out, "\x1b[37mred\x1b[31;47mw\x1b[1;97;101m\x1b[0m") == 0);

    memset(&st, 0, sizeof(st)); out[0] = 0;
    swap(&st, "\x1b[31;37H\x1b[17m\x1b[3;1m\x1b[32;47m", out);     /* a move, 17, 3;1, green */
    assert(strcmp(out, "\x1b[31;37H\x1b[17m\x1b[3;1m\x1b[32;41m") == 0);

    memset(&st, 0, sizeof(st)); out[0] = 0;
    swap(&st, "\x1b[38;5;31;41m\x1b[48;2;31;37;41;37m", out);   /* 256 and true colour kept */
    assert(strcmp(out, "\x1b[38;5;31;47m\x1b[48;2;31;37;41;31m") == 0);

    memset(&st, 0, sizeof(st)); out[0] = 0;
    swap(&st, "a\x1b[3", out);                                    /* split between reads */
    assert(strcmp(out, "a") == 0);
    swap(&st, "1mb\x9b" "47m\x1b" "c", out);                          /* 8-bit CSI; ESC c */
    assert(strcmp(out, "a\x1b[37mb\x9b" "41m\x1b" "c") == 0);
}

int main(void)
{
    test_bells_are_taken_out();
    test_console_colours_swap_red_and_white_in_sgr_only();
    test_private_parameters_are_skipped_whole();
    test_find_ignores_case();
    test_screen_row_as_ansi();
    test_coloured_blanks_are_kept_and_ice_is_blink();
    test_unit_order_pens_are_swapped_back();
    printf("ansiscan: all assertions passed\n");
    return 0;
}
