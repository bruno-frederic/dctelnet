/* src/vtpersonality.c -- see vtpersonality.h. */
#include "vtpersonality.h"
#include "charset.h"
#include <string.h>

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

const char *Vt_SshTermName(const struct PrefsStruct *p)
{
    return (p->State & APP_RENDERER_BUILTIN) && Vt_PersonalityFor(p).personality == VT_XTERM
         ? "xterm" : "ansi";
}

UBYTE Vt_BackspaceByte(const struct PrefsStruct *p)
{
    BOOL del = Vt_PersonalityFor(p).personality == VT_XTERM;

    if (p->State & APP_BACKSPACE_DEL_SWAPPED)
        del = !del;
    return del ? 0x7F : 0x08;
}

BOOL Vt_EngineDecodesUtf8(const struct PrefsStruct *p)
{
    return (p->State & APP_RENDERER_BUILTIN) != 0
        && Vt_PersonalityFor(p).charset == VT_CS_UTF8;
}

enum vt_font_enc Vt_GlyphEncoding(enum vt_charset cs, enum vt_font_enc fontEnc)
{
    return cs == VT_CS_CP437 ? VT_ENC_CP437 : fontEnc;
}

UBYTE Vt_CharsetAfterTermType(UBYTE termType, UBYTE charset)
{
    if (termType == TERMTYPE_UNIX && charset == CHARSET_CP437)
        return CHARSET_UTF8;
    return charset;
}

void Vt_Utf8SnifferReset(struct VtUtf8Sniffer *s)
{
    memset(s, 0, sizeof(*s));
}

BOOL Vt_Utf8SnifferActive(const struct PrefsStruct *p)
{
    return p->TermType == TERMTYPE_UNIX && p->Charset != CHARSET_UTF8;
}

/* Byte i of the held prefix followed by the new data. */
#define SNIFF_AT(s, data, i) ((i) < (s)->nheld ? (s)->held[i] : (data)[(i) - (s)->nheld])

BOOL Vt_Utf8SnifferFeed(struct VtUtf8Sniffer *s, const struct PrefsStruct *p,
                        const UBYTE *data, long len,
                        UBYTE pre[3], long *npre, long *ndata)
{
    long total = s->nheld + len;
    long i = 0, hold = 0;
    BOOL fired = FALSE;
    UBYTE oldHeld = s->nheld;

    memcpy(pre, s->held, 3);

    while (i < total && !s->done) {
        UBYTE c = SNIFF_AT(s, data, i);

        if (c >= 0xC2 && c <= 0xF4) {
            long need = c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
            long k, have = total - i - 1;

            for (k = 1; k <= need && k <= have; k++)
                if ((SNIFF_AT(s, data, i + k) & 0xC0) != 0x80) {   /* cut short: not UTF-8 */
                    s->done = TRUE;
                    break;
                }
            if (s->done)
                break;
            if (have < need) {                  /* so far valid, more to come */
                hold = total - i;
                break;
            }
            s->done = fired = TRUE;
            s->fired = TRUE;
            s->firedCharset = p->Charset;
        } else if (c >= 0x80) {                 /* stray continuation, C0, C1, F5..FF */
            s->done = TRUE;
        } else {
            i++;
        }
    }

    if (hold) {                                 /* the held prefix is part of the tail */
        UBYTE tail[3];
        long k;
        for (k = 0; k < hold; k++)
            tail[k] = SNIFF_AT(s, data, total - hold + k);
        memcpy(s->held, tail, (size_t)hold);
        s->nheld = (UBYTE)hold;
        *npre = 0;
        *ndata = len - (hold - oldHeld);
    } else {
        s->nheld = 0;
        *npre = oldHeld;
        *ndata = len;
    }
    return fired;
}

long Vt_Utf8SnifferTakeHeld(struct VtUtf8Sniffer *s, UBYTE pre[3])
{
    long n = s->nheld;

    memcpy(pre, s->held, 3);
    s->nheld = 0;                               /* incomplete is neither valid nor invalid: stays undecided */
    return n;
}

struct VtPersonality Vt_PersonalityForConnection(const struct PrefsStruct *p,
                                                 const struct VtUtf8Sniffer *s)
{
    struct VtPersonality r = Vt_PersonalityFor(p);

    if (s->fired && p->TermType == TERMTYPE_UNIX && p->Charset == s->firedCharset)
        r.charset = VT_CS_UTF8;
    return r;
}
