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
- [ ] MA4-2 Test with art longer than a screen: 200 rows through VtWrite; vt_scrollback_lines
      equals 200 - term_rows (number asserted) and the top row text is row 1 of the art.
- [ ] MA4-3 Scrolling back: Page Up draws the previous page; the rows on screen equal the
      art rows asserted by text; returning to the live view redraws the grid.
- [ ] MA4-4 Alt screen: with "\e[?1049h" the scrollback count does not change.
- [ ] MA4-5 Reachability: rig run, DCTelnet connected to the server sending the long art;
      scroll back with the real key; screen text read back equals the asserted rows;
      sentinel: scrollback draw count > 0.

## Done

5 of 5. MA4-2 to MA4-5 stay OPEN, not exempted, until MA4-1 is answered.
