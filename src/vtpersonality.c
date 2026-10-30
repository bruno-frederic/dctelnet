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

UBYTE Vt_CharsetAfterTermType(UBYTE termType, UBYTE charset)
{
    if (termType == TERMTYPE_UNIX && charset == CHARSET_CP437)
        return CHARSET_UTF8;
    return charset;
}

void Vt_Utf8SnifferReset(struct VtUtf8Sniffer *s)
{
    s->need = 0;
    s->done = FALSE;
}

BOOL Vt_Utf8SnifferActive(const struct PrefsStruct *p)
{
    return p->TermType == TERMTYPE_UNIX && p->Charset != CHARSET_UTF8;
}

BOOL Vt_Utf8SnifferFeed(struct VtUtf8Sniffer *s, const UBYTE *data, long len)
{
    long i;

    for (i = 0; i < len && !s->done; i++) {
        UBYTE c = data[i];

        if (s->need) {
            if ((c & 0xC0) != 0x80) {           /* sequence cut short: not UTF-8 */
                s->done = TRUE;
                return FALSE;
            }
            if (--s->need == 0) {
                s->done = TRUE;
                return TRUE;
            }
        } else if (c >= 0xC2 && c <= 0xF4) {
            s->need = c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
        } else if (c >= 0x80) {                 /* stray continuation, C0, C1, F5..FF */
            s->done = TRUE;
            return FALSE;
        }
    }
    return FALSE;
}
