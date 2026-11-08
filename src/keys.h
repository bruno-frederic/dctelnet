/* src/keys.h -- keys typed in the terminal, as the BBS expects them.
 * Pure, unit-tested on the host.
 *
 * MapRawKey() turns a key into the Amiga console's bytes: F1-F10 are
 * CSI 0~ .. CSI 9~, Shift-F1-F10 CSI 10~ .. CSI 19~, the cursor keys CSI A-D,
 * Shift-Up/Down CSI T/S, Shift-Left/Right CSI " A"/" @", Help CSI ?~, and on
 * keymaps that know them Insert, Page Up/Down, Home, End CSI 40~ .. 45~.
 * DCTelnet read one byte after the CSI and sent the rest as text: Shift-F1
 * fired F2's macro and then sent "~". A sequence is now read whole. */
#ifndef KEYS_H
#define KEYS_H

#include <exec/types.h>
#include <string.h>

enum KeyId
{
    KEY_NONE,           /* an unknown sequence: nothing to send */
    KEY_TEXT,           /* one byte of text */
    KEY_F1,             /* KEY_F1 + n for F(n+1), n 0-9 */
    KEY_SHIFT_F1 = KEY_F1 + 10,
    KEY_UP = KEY_SHIFT_F1 + 10, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
    KEY_HOME, KEY_END, KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_INSERT, KEY_HELP
};

/* The next key in buf[0..len) (MapRawKey output). Returns the bytes it
 * takes (at least 1 when len > 0), sets *id, and *text for KEY_TEXT. An
 * incomplete sequence at the end takes the rest and is KEY_NONE. */
size_t Keys_Next(const UBYTE *buf, size_t len, int *id, UBYTE *text);

/* Keys that are not in every keymap, by raw key code (devices/rawkeycodes.h:
 * Insert $47, Page Up $48, Page Down $49, Home $70, End $71); KEY_NONE for
 * any other code. */
int Keys_FromRawCode(UWORD code);

/* The bytes a navigation or cursor key sends: ANSI-BBS keys (SyncTERM:
 * Home ESC[H, End ESC[K, Page Up ESC[V, Page Down ESC[U, Insert ESC[@) or
 * VT keys (ESC[1~ ESC[4~ ESC[5~ ESC[6~ ESC[2~). Cursor keys are ESC[A-D, or
 * ESC OA-OD when the host set cursor key mode (DECCKM). Shift-Up/Down are
 * Page Up/Down, Shift-Left/Right Home/End: the A500/A1200 keyboard has no
 * such keys. Returns the length (0: the key sends nothing). */
#define KEYS_MAX_BYTES 4
size_t Keys_Bytes(int id, BOOL vtKeys, BOOL cursorKeyMode, char out[KEYS_MAX_BYTES]);

#endif /* KEYS_H */
