/* src/textedit.h -- a field entered with Tab is replaced by what is typed.
 * Pure, unit-tested on the host. */
#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <exec/types.h>

/* The string gadget already inserted the typed character at pos-1 of buf
 * (num characters). When the field was just entered with Tab (fresh), the
 * character replaces the whole old value -- as a selected-all field on a
 * modern system -- and buf, pos and num describe that one character. */
void TextEdit_FirstKey(char *buf, UWORD *pos, UWORD *num, BOOL fresh);

#endif /* TEXTEDIT_H */
