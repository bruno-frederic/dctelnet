/* test/test_listsel.c -- which list row is selected after one is deleted. */
#include <assert.h>
#include <stdio.h>
#include "listsel.h"

/* Deleting the first Address Book entry decremented the UWORD selection
 * from 0 to 65535: nothing was selected, and the next Delete did nothing. */
static void test_deleting_the_first_row_keeps_a_row_selected(void) {
    assert(ListSel_AfterDelete(0, 3) == 0);     /* the next row moves up into place */
    assert(ListSel_AfterDelete(0, 0) == LISTSEL_NONE);  /* the list is empty now */
}

static void test_deleting_the_last_row_selects_the_new_last(void) {
    assert(ListSel_AfterDelete(4, 4) == 3);
    assert(ListSel_AfterDelete(2, 5) == 2);
}

int main(void) {
    test_deleting_the_first_row_keeps_a_row_selected();
    test_deleting_the_last_row_selects_the_new_last();
    printf("listsel: all assertions passed\n");
    return 0;
}
