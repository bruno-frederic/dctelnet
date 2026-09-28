/* test/test_progdir.c -- DCTelnet's own drawer before the system's. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "progdir.h"

/* ibmcon.device, the fonts and the XEM/XPR libraries ship in DCTelnet's
 * drawer, but DCTelnet only looked in DEVS:, FONTS: and LIBS: -- an older
 * ibmcon installed there was used instead of the one that came with it. */
static void test_a_bare_name_is_looked_up_in_the_own_drawer_first(void) {
    char out[64];

    assert(ProgDir_Path("Devs", "ibmcon.device", out, sizeof(out)));
    assert(strcmp(out, "PROGDIR:Devs/ibmcon.device") == 0);
    assert(ProgDir_Path("Fonts", "TopazPro.font", out, sizeof(out)));
    assert(strcmp(out, "PROGDIR:Fonts/TopazPro.font") == 0);
}

static void test_a_path_the_user_chose_is_kept(void) {
    char out[64] = "untouched";

    assert(!ProgDir_Path("Libs", "Work:Libs/xemvt340.library", out, sizeof(out)));
    assert(!ProgDir_Path("Libs", "xem/xemvt340.library", out, sizeof(out)));
    assert(!ProgDir_Path("Libs", "", out, sizeof(out)));
    assert(strcmp(out, "untouched") == 0);
}

static void test_a_path_that_does_not_fit_is_refused(void) {
    char out[16] = "untouched";

    assert(!ProgDir_Path("Libs", "xprzmodem.library", out, sizeof(out)));
    assert(strcmp(out, "untouched") == 0);
}

#define VER(s) ProgDir_ParseVersion((const UBYTE *)(s), sizeof(s) - 1, &v, &r)

/* With a copy in both places, the newer one is opened: the version comes
 * from the file's $VER string (inside the binary, among other bytes). */
static void test_the_version_is_read_from_the_ver_string(void) {
    ULONG v = 99, r = 99;

    assert(VER("\0\1junk$VER: ibmcon.device 1.6 (Sep 27 2026)\0") && v == 1 && r == 6);
    assert(VER("$VER: xemvt340.library 4.12") && v == 4 && r == 12);
    assert(VER("$VER: reqtools.library 39 (1.1.94)") && v == 39 && r == 0);
    assert(!VER("no version here"));
    assert(!VER("$VER: broken"));
    assert(ProgDir_Newer(1, 6, 1, 5) && ProgDir_Newer(2, 0, 1, 9));
    assert(!ProgDir_Newer(1, 5, 1, 5) && !ProgDir_Newer(1, 4, 1, 5));
}

int main(void) {
    test_the_version_is_read_from_the_ver_string();
    test_a_bare_name_is_looked_up_in_the_own_drawer_first();
    test_a_path_the_user_chose_is_kept();
    test_a_path_that_does_not_fit_is_refused();
    printf("progdir: all assertions passed\n");
    return 0;
}
