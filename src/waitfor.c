/* src/waitfor.c -- waiting for a text from the BBS. */
#include "waitfor.h"

void WaitFor_Start(struct WaitFor *w, const char *text, size_t len)
{
    if (len >= WAITFOR_MAX) len = WAITFOR_MAX - 1;
    memcpy(w->text, text, len);
    w->text[len] = 0;
    w->len = (UWORD)len;
    w->matched = 0;
    w->esc = 0;
}

void WaitFor_Stop(struct WaitFor *w)
{
    w->text[0] = 0;
    w->len = w->matched = 0;
}

BOOL WaitFor_Active(const struct WaitFor *w)
{
    return w->len != 0;
}

BOOL WaitFor_Feed(struct WaitFor *w, const UBYTE *data, size_t len)
{
    size_t i;

    for (i = 0; i < len && w->len; i++)
    {
        UBYTE c = data[i];

        if (w->esc == 1)                            /* after ESC */
        {
            w->esc = (UBYTE)(c == '[' ? 2 : 0);
            continue;
        }
        if (w->esc == 2)                            /* CSI: to its final byte */
        {
            if (c >= 0x40 && c <= 0x7E) w->esc = 0;
            continue;
        }
        if (c == 0x1B) { w->esc = 1; continue; }
        if (c == 0x9B) { w->esc = 2; continue; }
        if (c == (UBYTE)w->text[w->matched])
            w->matched++;
        else
        {
            /* Start again; a repeated first character may begin the match
             * (the texts are short: re-check from each later start). */
            UWORD start;

            for (start = 1; start <= w->matched; start++)
            {
                UWORD k = 0, m = (UWORD)(w->matched - start);

                while (k < m && w->text[start + k] == w->text[k]) k++;
                if (k == m && (UBYTE)w->text[m] == c) { w->matched = (UWORD)(m + 1); break; }
            }
            if (start > w->matched)
                w->matched = (UWORD)(c == (UBYTE)w->text[0] ? 1 : 0);
        }
        if (w->matched == w->len)
        {
            WaitFor_Stop(w);
            return TRUE;
        }
    }
    return FALSE;
}
