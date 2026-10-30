/* src/vtpersonality.c -- see vtpersonality.h. */
#include "vtpersonality.h"
#include "charset.h"

struct VtPersonality Vt_PersonalityFor(const struct PrefsStruct *p)
{
    struct VtPersonality r;

    (void)p;
    r.personality = VT_PCANSI;
    r.charset = VT_CS_CP437;
    return r;
}
