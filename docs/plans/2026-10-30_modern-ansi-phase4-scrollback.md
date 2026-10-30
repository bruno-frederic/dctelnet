# MA4 Phase 4: scrollback decision

Parent: 2026-10-30_modern-ansi-engine.md.

## Owner question (answered 2026-10-30: option 1, engine scrollback)

Where does scrollback live for the built-in renderer?

1. Engine scrollback (recommended). vt_new takes a line count (vtengine.h:140; DCTelnet
   passes 0 at vtbuiltin.c:156). vt_scrollback_lines and vt_row(row < 0) read it
   (vtengine.h:165-177). One source of truth, resize reflow is the engine's, alt screen
   handled right (no scrollback in vim). Cost: DCTelnet must draw scrolled-back rows
   itself and add a scroll gesture; memory of lines x columns x cell size.
2. Keep DCTelnet's own scrollback window. No new code. Cost: it holds text copied from
   the screen, not cells, so colour and glyph fidelity differ from the grid and it is a
   second store.
3. Both. Most code, two stores to keep in step. Not recommended.

## Files to touch (option 1)

- src/vtbuiltin.c:156 vt_new(..., 0, ...): scrollback line count from a pref.
- src/vtbuiltin.c:29-60 VtDraw: draw rows from vt_row(negative) when scrolled back.
- src/DCTelnet.c: the existing scrollback window code (grep -a -i scroll DCTelnet.c;
  the engine route never used the engine's) and key handling at 4040 (Page Up / Shift
  wheel).
- tests/test_term_engine.c.

## Checklist

- [x] MA4-1 Owner answers the question above; the answer is written here (2026-10-30:
      option 1, engine scrollback).
- [x] MA4-2 (test_ma4_2_long_art_fills_the_engine_scrollback_and_only_when_asked: 170 lines, oldest = L001; pref off: vt_new gets 0) Test with art longer than a screen: 200 rows through VtWrite; vt_scrollback_lines
      equals 200 - term_rows (number asserted) and the top row text is row 1 of the art.
- [x] MA4-3 (test_ma4_3_shift_page_up_draws_the_previous_page_and_page_down_returns, test_ma4_3_a_scrollback_line_narrower_than_the_grid_is_blank_filled; the key is Shift+Page Up/Down and the wheel, plain Page Up still goes to the host) Scrolling back: Page Up draws the previous page; the rows on screen equal the
      art rows asserted by text; returning to the live view redraws the grid.
- [x] MA4-4 (test_ma4_4_the_alt_screen_leaves_the_scrollback_alone) Alt screen: with "\e[?1049h" the scrollback count does not change.
- [ ] MA4-5 Reachability: rig run, DCTelnet connected to the server sending the long art;
      scroll back with the real key; screen text read back equals the asserted rows;
      sentinel: scrollback draw count > 0.

## Memory and switch (owner constraint: 68000)

Scrollback is optional and off by default: vt_new gets prefs.nScrollbackLines when
APP_SCROLLBACK_ENABLED is set, else 0 (nothing allocated). Cost per line: 16 bytes per
cell x columns = 1280 bytes at 80 columns, plus about 12 bytes of header and ring slot.
The pref is read at every VtOpen; a toggle (menu Scrollback History, ScrollBack Lines..)
sets shouldRestart, the existing full-reopen path (CloseDisplay/OpenDisplay), so the
grid is cleared on a toggle as it is for the other built-in terminal settings.
The old console.device scrollback window has its own key path (scrollbackWin only); the
built-in renderer does not reuse it.

## Done

4 of 5. MA4-5 (rig) open.
