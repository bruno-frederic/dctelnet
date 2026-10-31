/* src/menu_checks.h -- which menu items carry a check mark, derived from prefs.
 *
 * The Settings menu shows `prefs`. `prefs` changes while the menu exists
 * (an Address Book entry's settings are applied on connect and restored on
 * disconnect), so the check marks are derived in ONE place and used both when
 * the menu is built (CreateAppMenus) and when it is brought up to date
 * (SyncMenuChecks).
 *
 * Pure functions on struct PrefsStruct, unit-tested on the host.
 */
#ifndef MENU_CHECKS_H
#define MENU_CHECKS_H

#include <exec/types.h>
#include "prefs.h"
#include "DCTelnet.h"   /* enum MenuItemID */

/*
 * Is menu item `id` one whose check mark follows `p`? FALSE: it is not (a
 * command, a title, or MENU_CAPTURE, which follows the capture file and not
 * prefs); *checked is left alone. TRUE: *checked is the mark it must show.
 */
BOOL MenuChecks_Checked(const struct PrefsStruct *p, enum MenuItemID id, BOOL *checked);

#endif
