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

int main(void)
{
    test_cp437_and_latin1_sites_are_pc_ansi_cp437();
    printf("vtpersonality: all assertions passed\n");
    return 0;
}
