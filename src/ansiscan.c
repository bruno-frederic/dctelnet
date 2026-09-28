/* src/ansiscan.c -- reading and writing ANSI text. */
#include "ansiscan.h"

size_t Ansi_ParamsEnd(const UBYTE *s, size_t i, size_t size)
{
    while (i < size && s[i] >= '0' && s[i] <= '?')
        i++;
    return i;
}

static UBYTE fold(UBYTE c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 0xC0 && c <= 0xDE && c != 0xD7))
        return (UBYTE)(c + 0x20);
    return c;
}

long Ansi_FindText(const char *hay, size_t len, const char *needle)
{
    size_t n = strlen(needle), i, j;

    if (n == 0 || n > len)
        return -1;
    for (i = 0; i + n <= len; i++)
    {
        for (j = 0; j < n && fold((UBYTE)hay[i + j]) == fold((UBYTE)needle[j]); j++)
            ;
        if (j == n)
            return (long)i;
    }
    return -1;
}

#define CELL_ATTR(c) (((ULONG)(c)[1] << 16) | ((ULONG)(c)[2] << 8) | ((c)[3] & 0x75))

static UBYTE ansi_pen(UBYTE pen, BOOL swap17)
{
    UBYTE base = (UBYTE)(pen & 7);

    if (swap17 && (base == 1 || base == 7))
        base = (UBYTE)(8 - base);
    return base;
}

static size_t put_sgr(ULONG a, BOOL swap17, char *out)
{
    UBYTE fg = (UBYTE)(a >> 16), bg = (UBYTE)(a >> 8), flags = (UBYTE)a;
    size_t n = 0;

    memcpy(out, "\033[0", 3); n = 3;
    if (fg >= 8 || (flags & 0x10)) { memcpy(out + n, ";1", 2); n += 2; }
    if ((flags & 0x40) || bg >= 8)  { memcpy(out + n, ";5", 2); n += 2; }   /* blink / iCE */
    if (flags & 0x01)               { memcpy(out + n, ";4", 2); n += 2; }
    if (flags & 0x20)               { memcpy(out + n, ";7", 2); n += 2; }
    out[n++] = ';'; out[n++] = '3'; out[n++] = (char)('0' + ansi_pen(fg, swap17));
    out[n++] = ';'; out[n++] = '4'; out[n++] = (char)('0' + ansi_pen(bg, swap17));
    out[n++] = 'm';
    return n;
}

size_t Ansi_ScreenRow(const UBYTE *cells, UWORD cols, BOOL swap17, ULONG *attr, char *out)
{
    size_t n = 0;
    UWORD c, last = 0;

    for (c = 0; c < cols; c++)                     /* the last cell that shows */
    {
        const UBYTE *cell = cells + c * 4;

        if (cell[0] != ' ' || (cell[2] & 7) != 0 || (cell[3] & 0x20))
            last = (UWORD)(c + 1);
    }
    for (c = 0; c < last; c++)
    {
        const UBYTE *cell = cells + c * 4;
        ULONG a = CELL_ATTR(cell);

        if (a != *attr)
        {
            n += put_sgr(a, swap17, out + n);
            *attr = a;
        }
        out[n++] = (char)cell[0];
    }
    out[n++] = '\r';
    out[n++] = '\n';
    return n;
}
