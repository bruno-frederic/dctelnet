/* src/vtshadow.h -- the screen of the console renderers, kept over a display
 * reopen. Pure, unit-tested on the host (tests/test_vtshadow.c).
 *
 * console.device and ibmcon.device keep their characters in the console
 * unit, which belongs to the window: CloseDisplay() closes the unit with it
 * and OpenDisplay() (Tool Bar, Title Bar, Full-screen, iconify, a font
 * change) opens an empty one. While one of them is the renderer, every byte
 * ConWrite() hands it also goes into a hidden vtcon engine sized to the
 * terminal grid, scrollback 0, in the matching dialect (console.device: the
 * engine's amiga personality, Latin-1; ibmcon.device: pcansi, CP437). After
 * the new unit opens, the engine's cells go back to the console as ANSI
 * (Ansi_ScreenRow, the Save Screen writer), the cursor last.
 *
 * The built-in renderer keeps its own engine (vtbuiltin.c) and has no
 * shadow; XEM has none either. */
#ifndef VTSHADOW_H
#define VTSHADOW_H

#include <exec/types.h>
#include "renderer.h"
#include "vtengine.h"

typedef void (*ShadowWriteFn)(void *user, const char *data, long len);

/* The bytes a console renderer got (ConWrite, before the console.device
 * colour swap): into the shadow engine. Nothing without one, or while
 * Shadow_DisplayOpened replays. */
void Shadow_Feed(const UBYTE *data, long len);

/* A display (re)opened on `route` with a cols x rows grid. Built-in and XEM
 * free the shadow. console.device / ibmcon.device: the first time, an empty
 * engine; then it is resized to the grid (vtcon's vt_resize: the rows from
 * the cursor's up are kept) and, when it holds any text, replayed through
 * write. Switching between the two console renderers carries the screen
 * into an engine of the new dialect. Returns TRUE when it replayed. */
BOOL Shadow_DisplayOpened(enum RendererRoute route, int cols, int rows,
                          ShadowWriteFn write, void *user);

/* The program ends: the engine goes. */
void Shadow_Close(void);

/* A shadow engine: cols x rows, no scrollback, the amiga (console.device) or
 * pcansi (ibmcon, cp437) personality, music off, replies dropped. */
vt_term *Shadow_NewTerm(int cols, int rows, BOOL cp437);

/* The live engine (tests), NULL without one. */
vt_term *Shadow_Term(void);

/* t's screen as an escape stream that draws it on an empty console: wrap off,
 * home, clear, each row that shows something at its row (Ansi_ScreenRow, no
 * CR LF), attributes reset, the cursor at its place, wrap as t has it. cp437:
 * the characters go out as CP437 bytes (pcansi), else Latin-1. Calls write
 * once a row (and for the head and tail). Returns the bytes written; 0 when
 * out of memory or the screen is empty. */
long Shadow_Serialize(vt_term *t, BOOL cp437, ShadowWriteFn write, void *user);

/* One engine cell as an ibmcon READTEXT cell (char, fg, bg, flags), the form
 * Ansi_ScreenRow writes: what the replay keeps of a cell. */
void Shadow_Cell(const vt_term *t, const vt_cell *c, BOOL cp437, UBYTE out[4]);

#endif
