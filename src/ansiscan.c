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

size_t Ansi_StripByte(UBYTE *data, size_t *len, UBYTE b)
{
    size_t i, o = 0;

    for (i = 0; i < *len; i++)
        if (data[i] != b)
            data[o++] = data[i];
    i = *len - o;
    *len = o;
    return i;
}

/* The held CSI sequence ends in m: swap the colour parameters in place. */
static void Swap17Sgr(UBYTE *seq, UWORD n)
{
    UWORD i = seq[0] == 0x9B ? 1 : 2, start;
    int skip = 0;           /* after 38/48: -1 = the form (5 or 2) is next, n > 0 = arguments left */

    while (i < n - 1)
    {
        UWORD len;
        unsigned v = 0;

        start = i;
        while (i < n - 1 && seq[i] >= '0' && seq[i] <= '9')
            v = v * 10 + (unsigned)(seq[i++] - '0');
        len = (UWORD)(i - start);
        if (skip < 0)
            skip = v == 5 ? 1 : v == 2 ? 3 : 0;     /* 38;5;n or 38;2;r;g;b */
        else if (skip > 0)
            skip--;
        else if (v == 38 || v == 48)
            skip = -1;
        else if (len >= 2 && (seq[i - 1] == '1' || seq[i - 1] == '7')
                 && ((len == 2 && (seq[start] == '3' || seq[start] == '4' || seq[start] == '9'))
                     || (len == 3 && seq[start] == '1' && seq[start + 1] == '0')))
            seq[i - 1] = seq[i - 1] == '1' ? '7' : '1';
        if (i < n - 1)
            i++;                                    /* the ';' (or another byte) */
    }
}

size_t Ansi_Swap17(struct AnsiSwap17 *st, const UBYTE *in, size_t len, UBYTE *out)
{
    size_t o = 0, k;

    for (k = 0; k < len; k++)
    {
        UBYTE b = in[k];

        if (st->state == 0)
        {
            if (b == 0x1B || b == 0x9B)
            {
                st->held[0] = b;
                st->n = 1;
                st->state = b == 0x1B ? 1 : 2;
            }
            else
                out[o++] = b;
            continue;
        }
        if (st->state == 1)
        {
            if (b == '[')
            {
                st->held[st->n++] = b;
                st->state = 2;
                continue;
            }
            memcpy(out + o, st->held, st->n);       /* not a CSI: as it was */
            o += st->n;
            st->n = 0;
            st->state = 0;
            k--;                                    /* b again, as text */
            continue;
        }
        /* in a CSI sequence */
        if (b >= 0x20 && b <= 0x3F && st->n < ANSI_SWAP17_HELD - 1)
        {
            st->held[st->n++] = b;
            continue;
        }
        if (b >= 0x40 && b <= 0x7E)
        {
            st->held[st->n++] = b;
            if (b == 'm')
                Swap17Sgr(st->held, st->n);
            memcpy(out + o, st->held, st->n);
            o += st->n;
            st->n = 0;
            st->state = 0;
            continue;
        }
        memcpy(out + o, st->held, st->n);           /* broken off or too long: as it was */
        o += st->n;
        st->n = 0;
        st->state = 0;
        k--;
    }
    return o;
}
