/* src/vtpersonality.h -- which terminal the built-in renderer emulates and
 * which character set it reads, as a function of the site's preferences.
 * Pure, unit-tested on the host. */
#ifndef VTPERSONALITY_H
#define VTPERSONALITY_H

#include "prefs.h"
#include "vtengine.h"

struct VtPersonality {
    enum vt_personality personality;
    enum vt_charset charset;
};

/* Terminal Type (prefs.TermType) decides the personality: BBS is the IBM PC
 * ANSI.SYS terminal, Unix is xterm, Automatic takes xterm for Character Set
 * UTF-8 and the PC terminal for anything else. PC ANSI reads CP437 (a Latin-1
 * site gets its bytes through unchanged, the font decides the look); xterm
 * reads the Character Set. */
struct VtPersonality Vt_PersonalityFor(const struct PrefsStruct *p);

/* TRUE when the built-in renderer's engine reads UTF-8 itself: the bytes from
 * the host go to it as they are and DisplayBytes must not convert them. */
BOOL Vt_EngineDecodesUtf8(const struct PrefsStruct *p);

/* MA2-13: the Character Set the requester shows after Terminal Type changes.
 * Unix with IBM PC (CP437) becomes UTF-8, every other pair stays as it is. */
UBYTE Vt_CharsetAfterTermType(UBYTE termType, UBYTE charset);

/* MA2-14: finds out from the bytes of one connection whether a Unix host
 * speaks UTF-8 although the Character Set says otherwise. Reset on connect. */
struct VtUtf8Sniffer {
    UBYTE need;         /* continuation bytes still due for the sequence in progress */
    UBYTE done;         /* TRUE: decided (switched or an invalid high byte was seen) */
};
void Vt_Utf8SnifferReset(struct VtUtf8Sniffer *s);

/* TRUE while the sniffer applies: Terminal Type Unix, Character Set not UTF-8. */
BOOL Vt_Utf8SnifferActive(const struct PrefsStruct *p);

/* Feeds bytes; TRUE exactly once, when the first complete valid multi-byte
 * sequence arrives with no invalid high byte before it. */
BOOL Vt_Utf8SnifferFeed(struct VtUtf8Sniffer *s, const UBYTE *data, long len);

#endif /* VTPERSONALITY_H */
