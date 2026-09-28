/* test/test_textedit.c -- a field entered with Tab is replaced by what is typed. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "textedit.h"

/* Tabbing into the Address Book's Address field and typing appended to the
 * old value ("bbs.uprough.netx"); the first key replaces it. */
static void test_typing_after_tab_replaces_the_prefilled_value(void) {
    char buf[32] = "bbs.uprough.netx";          /* 'x' inserted at the end */
    UWORD pos = 16, num = 16;

    TextEdit_FirstKey(buf, &pos, &num, TRUE);
    assert(strcmp(buf, "x") == 0 && pos == 1 && num == 1);
}

static void test_typing_inside_a_field_edits_it(void) {
    char buf[32] = "bbsx.uprough.net";           /* cursor was after "bbs" */
    UWORD pos = 4, num = 16;

    TextEdit_FirstKey(buf, &pos, &num, FALSE);   /* not fresh: clicked or typed before */
    assert(strcmp(buf, "bbsx.uprough.net") == 0 && pos == 4 && num == 16);
}

int main(void) {
    test_typing_after_tab_replaces_the_prefilled_value();
    test_typing_inside_a_field_edits_it();
    printf("textedit: all assertions passed\n");
    return 0;
}
