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

/* Today every site is an IBM PC ANSI.SYS terminal reading CP437: a Latin-1
 * site gets its bytes through unchanged and the font decides the look. */
struct VtPersonality Vt_PersonalityFor(const struct PrefsStruct *p);

#endif /* VTPERSONALITY_H */
