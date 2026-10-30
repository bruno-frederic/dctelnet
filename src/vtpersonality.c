/* src/vtpersonality.c -- see vtpersonality.h. */
#include "vtpersonality.h"
#include "charset.h"

struct VtPersonality Vt_PersonalityFor(const struct PrefsStruct *p)
{
    struct VtPersonality r;

    /* Nothing in the preferences changes it yet (MA1); the argument is the
     * place later phases read their choice from. */

    r.personality = VT_PCANSI;
    r.charset = VT_CS_CP437;
    return r;
}
