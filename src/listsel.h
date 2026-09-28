/* src/listsel.h -- which list row is selected after one is deleted.
 * Pure, unit-tested on the host. */
#ifndef LISTSEL_H
#define LISTSEL_H

#include <exec/types.h>

#define LISTSEL_NONE ((UWORD)~0)    /* GTLV_Selected: no row selected */

/* After deleting row `deleted`, with `remaining` rows left: the row now in
 * its place, else the last row, else LISTSEL_NONE. */
UWORD ListSel_AfterDelete(UWORD deleted, UWORD remaining);

#endif /* LISTSEL_H */
