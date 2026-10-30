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

/* MA2-13: Unix with IBM PC becomes UTF-8; every other pair is unchanged. */
static void test_ma2_13_unix_with_cp437_swaps_to_utf8_only(void)
{
    UBYTE t, c;

    for (t = TERMTYPE_AUTO; t <= TERMTYPE_UNIX; t++)
        for (c = CHARSET_CP437; c <= CHARSET_UTF8; c++) {
            UBYTE want = (t == TERMTYPE_UNIX && c == CHARSET_CP437) ? CHARSET_UTF8 : c;
            assert(Vt_CharsetAfterTermType(t, c) == want);
        }
}

static struct PrefsStruct sniffPrefs;
static UBYTE sniffPre[3];
static long sniffNpre, sniffNdata;

static BOOL sniff(struct VtUtf8Sniffer *s, const char *b)
{
    return Vt_Utf8SnifferFeed(s, &sniffPrefs, (const UBYTE *)b, (long)strlen(b),
                              sniffPre, &sniffNpre, &sniffNdata);
}

/* MA2-14: the detector alone. */
static void test_ma2_14_first_valid_utf8_sequence_fires_once_invalid_bytes_never(void)
{
    struct VtUtf8Sniffer s;
    struct PrefsStruct p;

    Vt_Utf8SnifferReset(&s);
    assert(!sniff(&s, "plain ascii"));
    assert(sniff(&s, "\xE2\x96\x88"));
    assert(!sniff(&s, "\xE2\x96\x88"));              /* decided: fires once */

    Vt_Utf8SnifferReset(&s);                           /* split across reads */
    assert(!sniff(&s, "\xE2\x96"));
    assert(sniff(&s, "\x88"));

    Vt_Utf8SnifferReset(&s);                           /* split: prefix held, not drawn */
    assert(!sniff(&s, "ab\xE2"));
    assert(sniffNpre == 0 && sniffNdata == 2 && s.nheld == 1);
    assert(!sniff(&s, "\x96"));
    assert(sniffNpre == 0 && sniffNdata == 0 && s.nheld == 2);
    assert(sniff(&s, "\x88z"));
    assert(sniffNpre == 2 && sniffNdata == 2 && s.nheld == 0);

    Vt_Utf8SnifferReset(&s);                           /* held prefix turns invalid: given back */
    assert(!sniff(&s, "\xE2"));
    assert(!sniff(&s, "A"));
    assert(sniffNpre == 1 && sniffPre[0] == 0xE2 && sniffNdata == 1 && s.nheld == 0);

    Vt_Utf8SnifferReset(&s);                           /* CP437 art first: never */
    assert(!sniff(&s, "\xC9\xCD\xBB"));
    assert(!sniff(&s, "\xE2\x96\x88"));

    Vt_Utf8SnifferReset(&s);                           /* Latin-1 text: never */
    assert(!sniff(&s, "\xE9t\xE9"));
    assert(!sniff(&s, "\xE2\x96\x88"));

    Vt_Utf8SnifferReset(&s);                           /* lone high byte, then valid */
    assert(!sniff(&s, "\xB0"));
    assert(!sniff(&s, "\xC3\xA9"));

    Vt_Utf8SnifferReset(&s);                           /* reset on connect re-arms it */
    assert(sniff(&s, "\xC3\xA9"));

    memset(&p, 0, sizeof(p));
    p.TermType = TERMTYPE_UNIX; p.Charset = CHARSET_CP437;
    assert(Vt_Utf8SnifferActive(&p));
    p.Charset = CHARSET_LATIN1;
    assert(Vt_Utf8SnifferActive(&p));
    p.Charset = CHARSET_UTF8;
    assert(!Vt_Utf8SnifferActive(&p));
    p.Charset = CHARSET_CP437; p.TermType = TERMTYPE_AUTO;
    assert(!Vt_Utf8SnifferActive(&p));
    p.TermType = TERMTYPE_BBS;
    assert(!Vt_Utf8SnifferActive(&p));
}

/* MA2-14: a fired sniffer keeps UTF-8 across a personality recomputation. */
static void test_ma2_14_fired_sniffer_keeps_utf8_until_the_user_changes_prefs(void)
{
    struct VtUtf8Sniffer s;
    struct PrefsStruct p;

    memset(&p, 0, sizeof(p));
    p.TermType = TERMTYPE_UNIX; p.Charset = CHARSET_CP437;
    sniffPrefs = p;
    Vt_Utf8SnifferReset(&s);
    assert(Vt_PersonalityForConnection(&p, &s).charset == VT_CS_CP437);   /* not fired */
    assert(sniff(&s, "\xC3\xA9"));
    assert(Vt_PersonalityForConnection(&p, &s).charset == VT_CS_UTF8);
    assert(Vt_PersonalityForConnection(&p, &s).personality == VT_XTERM);
    p.Charset = CHARSET_LATIN1;                                          /* user changed it */
    assert(Vt_PersonalityForConnection(&p, &s).charset == VT_CS_LATIN1);
    p.Charset = CHARSET_CP437; p.TermType = TERMTYPE_BBS;                /* no longer Unix */
    assert(Vt_PersonalityForConnection(&p, &s).charset == VT_CS_CP437);
    Vt_Utf8SnifferReset(&s);
    p.TermType = TERMTYPE_UNIX;
    assert(Vt_PersonalityForConnection(&p, &s).charset == VT_CS_CP437);   /* new connection */
}

int main(void)
{
    test_terminal_type_picks_the_personality();
    test_engine_decodes_utf8_only_when_xterm_utf8_builtin();
    test_cp437_and_latin1_sites_are_pc_ansi_cp437();
    test_ma2_13_unix_with_cp437_swaps_to_utf8_only();
    test_ma2_14_first_valid_utf8_sequence_fires_once_invalid_bytes_never();
    test_ma2_14_fired_sniffer_keeps_utf8_until_the_user_changes_prefs();
    printf("vtpersonality: all assertions passed\n");
    return 0;
}
