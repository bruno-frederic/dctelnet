/* tests/test_vtpersonality.c -- MA1-1: the personality and character set
 * the built-in renderer takes from the preferences. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vtpersonality.h"
#include "charset.h"

static void test_cp437_and_latin1_sites_are_pc_ansi_cp437(void)
{
    struct PrefsStruct p;
    struct VtPersonality r;

    memset(&p, 0, sizeof(p));
    p.Charset = CHARSET_CP437;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_PCANSI && r.charset == VT_CS_CP437);
    p.Charset = CHARSET_LATIN1;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_PCANSI && r.charset == VT_CS_CP437);
}

/* MA2-1: Terminal Type picks the personality. */
static void test_terminal_type_picks_the_personality(void)
{
    struct PrefsStruct p;
    struct VtPersonality r;

    memset(&p, 0, sizeof(p));
    /* Automatic: xterm for UTF-8 only */
    p.TermType = TERMTYPE_AUTO;
    p.Charset = CHARSET_UTF8;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_XTERM && r.charset == VT_CS_UTF8);
    p.Charset = CHARSET_LATIN1;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_PCANSI && r.charset == VT_CS_CP437);
    /* BBS: always the PC terminal, CP437 */
    p.TermType = TERMTYPE_BBS;
    p.Charset = CHARSET_UTF8;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_PCANSI && r.charset == VT_CS_CP437);
    /* Unix: xterm, reading the Character Set */
    p.TermType = TERMTYPE_UNIX;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_XTERM && r.charset == VT_CS_UTF8);
    p.Charset = CHARSET_LATIN1;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_XTERM && r.charset == VT_CS_LATIN1);
    p.Charset = CHARSET_CP437;
    r = Vt_PersonalityFor(&p);
    assert(r.personality == VT_XTERM && r.charset == VT_CS_CP437);
}

/* MA2-5: the engine decodes UTF-8 itself (DisplayBytes then passes the bytes
 * on) only on the built-in renderer with an xterm UTF-8 site. */
static void test_engine_decodes_utf8_only_when_xterm_utf8_builtin(void)
{
    struct PrefsStruct p;

    memset(&p, 0, sizeof(p));
    p.State = APP_RENDERER_BUILTIN;
    p.Charset = CHARSET_UTF8;
    p.TermType = TERMTYPE_UNIX;
    assert(Vt_EngineDecodesUtf8(&p));
    p.TermType = TERMTYPE_AUTO;
    assert(Vt_EngineDecodesUtf8(&p));
    p.TermType = TERMTYPE_BBS;                  /* PC terminal: CP437 in */
    assert(!Vt_EngineDecodesUtf8(&p));
    p.TermType = TERMTYPE_UNIX;
    p.Charset = CHARSET_LATIN1;
    assert(!Vt_EngineDecodesUtf8(&p));
    p.Charset = CHARSET_UTF8;
    p.State = 0;                                /* the console renderer */
    assert(!Vt_EngineDecodesUtf8(&p));
}

int main(void)
{
    test_terminal_type_picks_the_personality();
    test_engine_decodes_utf8_only_when_xterm_utf8_builtin();
    test_cp437_and_latin1_sites_are_pc_ansi_cp437();
    printf("vtpersonality: all assertions passed\n");
    return 0;
}
