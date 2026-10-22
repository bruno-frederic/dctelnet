/* src/clip.c -- terminal text to and from the clipboard. */
#include "clip.h"
#include "charset.h"

void Clip_Order(UWORD downRow, UWORD downCol, UWORD row, UWORD col, struct ClipRange *out)
{
    if (row < downRow || (row == downRow && col < downCol))
    {
        out->startRow = row;     out->startCol = col;
        out->endRow   = downRow; out->endCol   = downCol;
    }
    else
    {
        out->startRow = downRow; out->startCol = downCol;
        out->endRow   = row;     out->endCol   = col;
    }
}

BOOL Clip_RowSpan(const struct ClipRange *r, UWORD row, UWORD cols, UWORD *from, UWORD *to)
{
    if (row < r->startRow || row > r->endRow)
        return FALSE;
    *from = row == r->startRow ? r->startCol : 1;
    *to   = row == r->endRow   ? r->endCol   : cols;
    if (*to > cols) *to = cols;
    return *from <= *to;
}

size_t Clip_RowText(const UBYTE *cells, UWORD cols, UWORD from, UWORD to,
                    BOOL petscii, BOOL lowerCase, char *out)
{
    size_t n = 0, kept = 0;
    UWORD c;

    if (to > cols) to = cols;
    for (c = from; c >= 1 && c <= to; c++)
    {
        UBYTE ch = cells[(c - 1) * CLIP_CELL];

        ch = petscii ? Charset_PetsciiToLatin1(ch, lowerCase) : Charset_Cp437ToLatin1(ch);
        out[n++] = (char)ch;
        if (ch != ' ' && ch != 0xA0)
            kept = n;
    }
    return kept;
}

static void put32(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24); p[1] = (UBYTE)(v >> 16); p[2] = (UBYTE)(v >> 8); p[3] = (UBYTE)v;
}

static ULONG get32(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3];
}

size_t Clip_BuildFtxt(const char *text, size_t len, UBYTE *out, size_t max)
{
    size_t chunk = len + (len & 1);                     /* chunks are word aligned */
    size_t total = 12 + 8 + chunk;

    if (max < total)
        return 0;
    memcpy(out, "FORM", 4);
    put32(out + 4, (ULONG)(total - 8));
    memcpy(out + 8, "FTXT", 4);
    memcpy(out + 12, "CHRS", 4);
    put32(out + 16, (ULONG)len);
    memcpy(out + 20, text, len);
    if (len & 1)
        out[20 + len] = 0;
    return total;
}

size_t Clip_ParseFtxt(const UBYTE *iff, size_t len, char *out, size_t max)
{
    size_t at = 12, n = 0, end;

    if (len < 12 || memcmp(iff, "FORM", 4) != 0 || memcmp(iff + 8, "FTXT", 4) != 0)
        return 0;
    end = 8 + get32(iff + 4);
    if (end > len) end = len;
    while (at + 8 <= end)
    {
        ULONG size = get32(iff + at + 4);

        if (size > end - at - 8)
            size = end - at - 8;                        /* cut short: what is there */
        if (memcmp(iff + at, "CHRS", 4) == 0)
        {
            size_t take = size < max - n ? size : max - n;

            memcpy(out + n, iff + at + 8, take);
            n += take;
        }
        at += 8 + size + (size & 1);
    }
    return n;
}

size_t Clip_PasteBytes(const char *text, size_t len, BOOL crlf, BOOL doubleIac, char *out)
{
    size_t i, n = 0;

    for (i = 0; i < len; i++)
    {
        UBYTE c = (UBYTE)text[i];

        if (c == '\r' || c == '\n')
        {
            if (c == '\r' && i + 1 < len && text[i + 1] == '\n')
                i++;                                    /* CR LF: one Return */
            out[n++] = '\r';
            if (crlf) out[n++] = '\n';
        }
        else
        {
            out[n++] = (char)c;
            if (c == 0xFF && doubleIac) out[n++] = (char)c;
        }
    }
    return n;
}
