/* src/charset.c -- character set conversions. */
#include "charset.h"

/* CP437 128-255 in ISO-8859-1, closest ASCII where Latin-1 has none. */
static const UBYTE cp437High[128] =
{
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5,
    0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5, 'P',  'f',
    0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, 0xBF, '-',  0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB,
    '#',  '#',  '#',  '|',  '+',  '+',  '+',  '+',  '+',  '+',  '|',  '+',  '+',  '+',  '+',  '+',
    '+',  '+',  '+',  '+',  '-',  '+',  '+',  '+',  '+',  '+',  '+',  '+',  '+',  '=',  '+',  '+',
    '+',  '+',  '+',  '+',  '+',  '+',  '+',  '+',  '+',  '+',  '+',  '#',  '#',  '#',  '#',  '#',
    'a',  0xDF, 'G',  'p',  'S',  's',  0xB5, 't',  'F',  'T',  'O',  'd',  '8',  'f',  'e',  'n',
    '=',  0xB1, '>',  '<',  '(',  ')',  0xF7, '~',  0xB0, 0xB7, 0xB7, 'v',  'n',  0xB2, '#',  0xA0
};

UBYTE Charset_Cp437ToLatin1(UBYTE c)
{
    if (c < 0x20)
        return ' ';
    if (c < 0x80)
        return c == 0x7F ? '#' : c;         /* 7F is a house glyph */
    return cp437High[c - 0x80];
}

UBYTE Charset_PetsciiToLatin1(UBYTE c, BOOL lowerCase)
{
    if (c == 0xA0 || c == 0xE0)
        return ' ';                         /* shifted space */
    if (c >= 0x20 && c <= 0x40)
        return c;                           /* space, digits, punctuation, @ */
    if (c >= 0x41 && c <= 0x5A)
        return (UBYTE)(lowerCase ? c + 0x20 : c);
    if (lowerCase && ((c >= 0x61 && c <= 0x7A) || (c >= 0xC1 && c <= 0xDA)))
        return (UBYTE)((c & 0x1F) + 0x40);  /* the upper case letters */
    switch (c)
    {
    case 0x5B: return '[';
    case 0x5C: return 0xA3;                 /* pound */
    case 0x5D: return ']';
    case 0x5E: return '^';                  /* up arrow */
    case 0x5F: return '<';                  /* left arrow */
    }
    return c < 0x20 ? ' ' : '#';
}

/* CP437 128-255 as Unicode, for text from a UTF-8 host. */
static const UWORD cp437Unicode[128] =
{
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

/* The CP437 byte showing Unicode code point u, '?' when there is none. */
static UBYTE unicode_to_cp437(ULONG u)
{
    int i;

    if (u < 0x80)
        return (UBYTE)u;
    for (i = 0; i < 128; i++)
        if (cp437Unicode[i] == u)
            return (UBYTE)(0x80 + i);
    return '?';
}

void Charset_Utf8Init(struct Utf8Decoder *d)
{
    d->need = 0;
    d->code = 0;
}

size_t Charset_Utf8ToCp437(struct Utf8Decoder *d, const UBYTE *in, size_t len, UBYTE *out)
{
    size_t i, n = 0;

    for (i = 0; i < len; i++)
    {
        UBYTE c = in[i];

        if (d->need)
        {
            if ((c & 0xC0) == 0x80)
            {
                d->code = (d->code << 6) | (c & 0x3F);
                if (--d->need == 0)
                    out[n++] = unicode_to_cp437(d->code);
                continue;
            }
            out[n++] = '?';                     /* a sequence cut short */
            d->need = 0;
        }
        if (c < 0x80)       out[n++] = c;       /* ASCII, and the ESC codes */
        else if ((c & 0xE0) == 0xC0) { d->code = c & 0x1F; d->need = 1; }
        else if ((c & 0xF0) == 0xE0) { d->code = c & 0x0F; d->need = 2; }
        else if ((c & 0xF8) == 0xF0) { d->code = c & 0x07; d->need = 3; }
        else                out[n++] = '?';     /* not UTF-8 */
    }
    return n;
}

size_t Charset_Latin1ToUtf8(const UBYTE *in, size_t len, UBYTE *out)
{
    size_t i, n = 0;

    for (i = 0; i < len; i++)
    {
        if (in[i] < 0x80)
            out[n++] = in[i];
        else
        {
            out[n++] = (UBYTE)(0xC0 | (in[i] >> 6));
            out[n++] = (UBYTE)(0x80 | (in[i] & 0x3F));
        }
    }
    return n;
}
