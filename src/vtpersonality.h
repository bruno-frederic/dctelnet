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

#endif /* VTPERSONALITY_H */
