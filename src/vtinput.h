/* src/vtinput.h -- what the built-in renderer's terminal sends and shows for
 * the mouse, the clipboard and the window title (Modern ANSI phase 3). Pure,
 * unit-tested on the host: DCTelnet.c only feeds it events and sends the bytes
 * it returns. The reply path (send_data) is the caller's. */
#ifndef VTINPUT_H
#define VTINPUT_H

#include <exec/types.h>
#include <stddef.h>
#include "vtengine.h"

/* MA3-4..6: the mouse buttons the program has been told are held, and the
 * cell the last report named (motion is reported once per cell). */
struct VtMouse {
    UBYTE held;         /* bit 0 left, 1 middle, 2 right */
    UBYTE button;       /* the button a drag reports as (0 left, 1 middle, 2 right) */
    WORD  lastX, lastY; /* last reported cell, -1 none */
};
void Vt_MouseReset(struct VtMouse *m);

/* TRUE while the mouse belongs to the program on the far side: a mouse mode
 * is on and Shift is not held (Shift keeps selecting). */
BOOL Vt_MouseOwned(const vt_term *t, BOOL shift);

/* A button went down or up (button 0 left, 1 middle, 2 right) or the wheel
 * turned (64 up, 65 down; down is TRUE). Cell x, y from 0. mods are
 * VT_MOD_* bits. Returns the bytes to send (written to out, at least 32),
 * 0 for nothing. */
int Vt_MouseButton(struct VtMouse *m, const vt_term *t, int button, BOOL down,
                   int x, int y, int mods, UBYTE *out);

/* NewMouse wheel: IDCMP_RAWKEY code 0x7A (up) or 0x7B (down) gives the xterm
 * wheel button 64 or 65, anything else -1. The wheel has a press only. */
int Vt_WheelButton(UWORD rawkey);

/* The pointer moved to cell x, y. ?1002 reports only while a button is held,
 * ?1003 always; both once per cell. */
int Vt_MouseMove(struct VtMouse *m, const vt_term *t, int x, int y, int mods, UBYTE *out);

/* The IDCMP flags the terminal window needs: base, plus moveFlag
 * (IDCMP_MOUSEMOVE) only while ?1002 or ?1003 is on or keepMove (selection). */
ULONG Vt_IdcmpFor(ULONG base, ULONG moveFlag, const vt_term *t, BOOL keepMove);

/* MA3-2/3: the window title for an OSC title (UTF-8) as Latin-1 for the
 * Amiga title bar; an empty title gives `own`. Control characters are dropped,
 * what Latin-1 cannot show becomes '?'. Always terminated; max includes the NUL. */
void Vt_TitleText(const char *utf8, const char *own, char *out, size_t max);

/* The screen title to set along with a window title. A backdrop window (DCTelnet's
 * own screen) has no title bar, so the title shows only in the screen bar: the
 * screen title is the title then. A normal window keeps the screen title: NULL,
 * the caller passes -1. backdropFlag is WFLG_BACKDROP. */
const char *Vt_ScreenTitleFor(ULONG winFlags, ULONG backdropFlag, const char *title);

/* MA3-7/8: pasted bytes in buf (len used, 12 bytes spare) wrapped in the
 * bracketed-paste markers when ?2004 is on; ESC is removed from the text then,
 * so it cannot hold the end marker. Returns the new length (len when off). */
size_t Vt_PasteBracket(const vt_term *t, char *buf, size_t len, int utf8);

#endif
