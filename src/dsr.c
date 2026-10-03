/* src/dsr.c -- ANSI Device Status Report requests in a BBS's text stream. */
#include "dsr.h"

void Dsr_Init(struct DsrScan *s)
{
    s->state = 0;
    s->param = 0;
    s->plain = 1;
}

size_t Dsr_Find(struct DsrScan *s, const UBYTE *buf, size_t len, int *kind)
{
    size_t i;

    *kind = DSR_NONE;
    for (i = 0; i < len; i++)
    {
        UBYTE c = buf[i];

        if (c == 0x18 || c == 0x1A)             /* CAN, SUB: abort a sequence */
        {
            Dsr_Init(s);
            continue;
        }
        switch (s->state)
        {
        case 0:
            if (c == 0x1B) s->state = 1;
            else if (c == 0x9B) { s->state = 2; s->param = 0; s->plain = 1; }
            break;
        case 1:
            if (c == '[') { s->state = 2; s->param = 0; s->plain = 1; }
            else s->state = (c == 0x1B) ? 1 : 0;
            break;
        default:
            if (c >= '0' && c <= '9')
                s->param = (UBYTE)(s->param > 24 ? 255 : s->param * 10 + (c - '0'));  /* 249 at most, else 255 */
            else if (c == ';' || c == '?' || c == '=' || c == '>' || c == ' ')
                s->plain = 0;
            else if (c == 0x1B)
                s->state = 1;
            else if (c >= 0x40 && c <= 0x7E)    /* final byte */
            {
                int found = c == 'n' && s->plain && (s->param == 5 || s->param == 6);

                s->state = 0;
                if (found)
                {
                    *kind = s->param;
                    return i + 1;
                }
            }
            break;
        }
    }
    return len;
}

static size_t put_num(char *out, UWORD n)
{
    char tmp[6];
    size_t k = 0, len = 0;

    do { tmp[k++] = (char)('0' + n % 10); n /= 10; } while (n);
    while (k) out[len++] = tmp[--k];
    return len;
}

size_t Dsr_Answer(int kind, UWORD row, UWORD col, char *out, size_t max)
{
    size_t len = 0;

    if (max < 16)
        return 0;
    out[len++] = 0x1B;
    out[len++] = '[';
    if (kind == DSR_STATUS)
        out[len++] = '0', out[len++] = 'n';
    else
    {
        len += put_num(out + len, row);
        out[len++] = ';';
        len += put_num(out + len, col);
        out[len++] = 'R';
    }
    return len;
}
