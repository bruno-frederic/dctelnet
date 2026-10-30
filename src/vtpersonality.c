/* src/vtpersonality.c -- see vtpersonality.h. */
#include "vtpersonality.h"
#include "charset.h"

struct VtPersonality Vt_PersonalityFor(const struct PrefsStruct *p)
{
    struct VtPersonality r;
    BOOL xterm;

    switch (p->TermType)
    {
        case TERMTYPE_BBS:  xterm = FALSE; break;
        case TERMTYPE_UNIX: xterm = TRUE;  break;
        default:            xterm = (p->Charset == CHARSET_UTF8); break;   /* Automatic */
    }

    if (!xterm)
    {
        /* Latin-1 sites get their bytes through unchanged (the font decides
         * the look); a UTF-8 site that is forced to BBS is converted to CP437
         * before the engine (DisplayBytes). */
        r.personality = VT_PCANSI;
        r.charset = VT_CS_CP437;
        return r;
    }
    r.personality = VT_XTERM;
    r.charset = p->Charset == CHARSET_UTF8   ? VT_CS_UTF8
              : p->Charset == CHARSET_LATIN1 ? VT_CS_LATIN1 : VT_CS_CP437;
    return r;
}

BOOL Vt_EngineDecodesUtf8(const struct PrefsStruct *p)
{
    return (p->State & APP_RENDERER_BUILTIN) != 0
        && Vt_PersonalityFor(p).charset == VT_CS_UTF8;
}
