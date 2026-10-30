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
    UBYTE held[3];      /* trailing, so far valid, incomplete UTF-8 prefix not yet drawn */
    UBYTE nheld;
    UBYTE done;         /* TRUE: decided (switched or an invalid high byte was seen) */
    UBYTE fired;        /* TRUE: the stream turned out to be UTF-8 */
    UBYTE firedCharset; /* prefs.Charset when it fired: a later change by the user wins */
};
void Vt_Utf8SnifferReset(struct VtUtf8Sniffer *s);
/* End of a receive burst: hands back the held incomplete prefix (to draw as the
 * current charset) and clears it. The sniffer stays undecided. Returns the count. */
long Vt_Utf8SnifferTakeHeld(struct VtUtf8Sniffer *s, UBYTE pre[3]);

/* TRUE while the sniffer applies: Terminal Type Unix, Character Set not UTF-8. */
BOOL Vt_Utf8SnifferActive(const struct PrefsStruct *p);

/* Feeds bytes; TRUE exactly once, when the first complete valid multi-byte
 * sequence arrives with no invalid high byte before it. While undecided, a
 * trailing incomplete prefix (at most 3 bytes) is held back: *npre bytes of
 * pre[] (the prefix held from the last call) are to be drawn first, then the
 * first *ndata bytes of data; the rest of data is now held inside s and comes
 * back as pre[] of the next call. Held bytes that turn out invalid are given
 * back in pre[] like any other byte. */
BOOL Vt_Utf8SnifferFeed(struct VtUtf8Sniffer *s, const struct PrefsStruct *p,
                        const UBYTE *data, long len,
                        UBYTE pre[3], long *npre, long *ndata);

/* Vt_PersonalityFor(), but a Unix connection whose stream the sniffer found to
 * be UTF-8 keeps UTF-8 while Terminal Type and Character Set are as they were
 * when it fired. */
struct VtPersonality Vt_PersonalityForConnection(const struct PrefsStruct *p,
                                                 const struct VtUtf8Sniffer *s);

#endif /* VTPERSONALITY_H */
