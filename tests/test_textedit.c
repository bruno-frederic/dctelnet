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

/* The SSH password requester: typing shows stars, the secret holds the
 * text; backspace, delete in the middle, and a clear follow along. */
static void test_a_password_field_shows_stars(void) {
    char secret[16] = "", buf[16];
    strcpy(buf, "s");     TextEdit_Secret(secret, 16, buf, 1, 0, 1, TEXTEDIT_INSERT);
    assert(!strcmp(secret, "s") && !strcmp(buf, "*"));
    strcpy(buf, "*e");    TextEdit_Secret(secret, 16, buf, 2, 1, 2, TEXTEDIT_INSERT);
    strcpy(buf, "**s");   TextEdit_Secret(secret, 16, buf, 3, 2, 3, TEXTEDIT_INSERT);
    assert(!strcmp(secret, "ses") && !strcmp(buf, "***"));
    strcpy(buf, "*x**");  TextEdit_Secret(secret, 16, buf, 2, 3, 4, TEXTEDIT_INSERT);   /* cursor after 's' */
    assert(!strcmp(secret, "sxes"));
    strcpy(buf, "***");   TextEdit_Secret(secret, 16, buf, 1, 4, 3, TEXTEDIT_OTHER);    /* backspace over 'x' */
    assert(!strcmp(secret, "ses"));
    strcpy(buf, "**");    TextEdit_Secret(secret, 16, buf, 2, 3, 2, TEXTEDIT_OTHER);    /* Del at the end-1 */
    assert(!strcmp(secret, "se") && !strcmp(buf, "**"));
    strcpy(buf, "*q");    TextEdit_Secret(secret, 16, buf, 2, 2, 2, TEXTEDIT_REPLACE);
    assert(!strcmp(secret, "sq"));
    strcpy(buf, "");      TextEdit_Secret(secret, 16, buf, 0, 2, 0, TEXTEDIT_OTHER);    /* Amiga-X */
    assert(!strcmp(secret, ""));
}

/* Undo brings back text the secret no longer matches: both empty. */
static void test_an_edit_that_cannot_be_followed_empties_the_field(void) {
    char secret[16] = "ab", buf[16] = "****";
    TextEdit_Secret(secret, 16, buf, 4, 2, 4, TEXTEDIT_OTHER);
    assert(!strcmp(secret, "") && !strcmp(buf, ""));
}

int main(void) {
    test_a_password_field_shows_stars();
    test_an_edit_that_cannot_be_followed_empties_the_field();
    test_typing_after_tab_replaces_the_prefilled_value();
    test_typing_inside_a_field_edits_it();
    printf("textedit: all assertions passed\n");
    return 0;
}
