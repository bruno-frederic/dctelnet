/* src/waitfor.h -- waiting for a text from the BBS. Pure, unit-tested on
 * the host.
 *
 * ARexx WAITFOR and the login macro's \w"text" wait until the BBS has sent
 * a text, e.g. "Password:". The BBS's bytes arrive in pieces and the text
 * may be split between them; colour codes may sit inside it (ESC[1mPass
 * ESC[0mword:). The matcher is fed every piece and skips escape sequences. */
#ifndef WAITFOR_H
#define WAITFOR_H

#include <exec/types.h>
#include <string.h>

#define WAITFOR_MAX 64

struct WaitFor
{
    char  text[WAITFOR_MAX];    /* "" = not waiting */
    UWORD len, matched;         /* its length, and how much of it came so far */
    UBYTE esc;                  /* 0 text, 1 after ESC, 2 in a CSI sequence */
};

/* Start waiting for text (at most WAITFOR_MAX-1 bytes). */
void WaitFor_Start(struct WaitFor *w, const char *text, size_t len);
void WaitFor_Stop(struct WaitFor *w);
BOOL WaitFor_Active(const struct WaitFor *w);

/* Feeds the BBS's bytes. TRUE when the text has now come (waiting ends). */
BOOL WaitFor_Feed(struct WaitFor *w, const UBYTE *data, size_t len);

#endif /* WAITFOR_H */
