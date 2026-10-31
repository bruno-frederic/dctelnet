/* src/textedit.c -- string gadget edits: Tab-entered fields, password fields. */
#include <string.h>
#include "textedit.h"

void TextEdit_FirstKey(char *buf, UWORD *pos, UWORD *num, BOOL fresh)
{
    char typed;

    if (!fresh || *pos == 0)
        return;
    typed = buf[*pos - 1];
    buf[0] = typed;
    buf[1] = 0;
    *pos = 1;
    *num = 1;
}

void TextEdit_Secret(char *secret, UWORD max, char *buf, UWORD pos, UWORD oldNum, UWORD num, int op)
{
    UWORD i, had = (UWORD)strlen(secret);

    if (had != oldNum)                              /* out of step: start again */
        num = 0;
    else if (op == TEXTEDIT_INSERT && num == oldNum + 1 && pos >= 1 && num < max)
    {
        memmove(secret + pos, secret + pos - 1, oldNum - (pos - 1));
        secret[pos - 1] = buf[pos - 1];
    }
    else if (op == TEXTEDIT_REPLACE && num == oldNum && pos >= 1)
        secret[pos - 1] = buf[pos - 1];
    else if (num < oldNum && pos + (oldNum - num) <= oldNum)
        memmove(secret + pos, secret + pos + (oldNum - num), num - pos);   /* deleted at pos */
    else if (num != oldNum || op != TEXTEDIT_OTHER)
        num = 0;
    secret[num] = 0;
    for (i = 0; i < num; i++) buf[i] = '*';
    buf[num] = 0;
}
