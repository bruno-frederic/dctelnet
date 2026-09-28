/* src/listsel.c -- which list row is selected after one is deleted. */
#include "listsel.h"

UWORD ListSel_AfterDelete(UWORD deleted, UWORD remaining)
{
    if (remaining == 0)
        return LISTSEL_NONE;
    return deleted < remaining ? deleted : (UWORD)(remaining - 1);
}
