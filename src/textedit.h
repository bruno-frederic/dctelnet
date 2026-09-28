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

/* A password field shows a '*' per character; the typed text lives in
 * secret (max bytes with its 0). The gadget has just applied an edit to buf
 * (oldNum characters before, num now, cursor at pos): the edit is mirrored
 * into secret and buf masked again. An edit that cannot be mirrored (undo
 * brings the masked text back) empties both. */
enum { TEXTEDIT_INSERT, TEXTEDIT_REPLACE, TEXTEDIT_OTHER };
void TextEdit_Secret(char *secret, UWORD max, char *buf, UWORD pos, UWORD oldNum, UWORD num, int op);

#endif /* TEXTEDIT_H */
