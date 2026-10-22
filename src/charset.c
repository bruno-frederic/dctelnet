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
