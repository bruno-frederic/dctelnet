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

int main(void)
{
    test_private_parameters_are_skipped_whole();
    test_find_ignores_case();
    test_screen_row_as_ansi();
    test_coloured_blanks_are_kept_and_ice_is_blink();
    test_unit_order_pens_are_swapped_back();
    printf("ansiscan: all assertions passed\n");
    return 0;
}
