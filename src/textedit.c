/* src/textedit.c -- a field entered with Tab is replaced by what is typed. */
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
