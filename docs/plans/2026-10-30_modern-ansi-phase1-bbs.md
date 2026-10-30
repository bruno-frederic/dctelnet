# MA1 Phase 1: BBS, PC ANSI personality, CP437 line art, 16 colours, iCE

Parent: 2026-10-30_modern-ansi-engine.md. Do this phase first.

## Goal

A BBS that sends ANSI.SYS art shows it right in the built-in renderer: line art from
CP437, 16 colours (bold becomes bright), blink or iCE bright backgrounds, cursor save and
restore, and long art. Today the personality is already VT_PCANSI (vtbuiltin.c:160) but
the gaps below are unproven or missing.

## Files to touch

- src/vtbuiltin.c:152-165 VtOpen: personality and charset set here. Make them come from a
  function of the prefs (MA1-1) instead of constants. PCANSI default stays.
- src/vtbuiltin.c:29-60 VtDraw: `fg & 15`, `bg & 15` at 47-48. Blink (VT_ATTR_BLINK,
  vtengine.h:46) is not drawn. vt_resolve_colors (vtengine.h:205) already applies iCE and
  bold-as-bright; check what it returns for blink with iCE on and off.
- src/vtbuiltin.c:59 `term_glyph(x, code)`; glyph via vt_map_glyph(ch, VT_ENC_CP437)
  (src/third_party/vtcon/render/glyphmap.h:29).
- src/DCTelnet.c:3101-3105 DisplayBytes: for CHARSET_CP437 and CHARSET_LATIN1 it passes bytes
  through. Check Latin-1 under the PCANSI personality (the engine is told CP437 at
  vtbuiltin.c:161, so Latin-1 bytes would be read as CP437: decide, test).
- src/DCTelnet.c:821 and 2430: init strings "\e[20l" and "\e[0 p" written through
  ConWrite into the engine. Check the engine accepts both (no stray output).
- tests/test_term_engine.c: add the cases. tests/Makefile:108-111 builds it.
- New sample file: tests/ansi_art/ with one real BBS ANSI art file (see MA1-9).

## Checklist

- [ ] MA1-1 One function `Vt_PersonalityFor(prefs)` picks personality + charset. In MA1 it
      returns VT_PCANSI + VT_CS_CP437 for CP437 and Latin-1 sites. Test the function.
- [ ] MA1-2 Line art: bytes C9 CD BB / BA / C8 CD BC written through VtWrite land in the grid
      as the right cells and VtDraw calls term_glyph with codes 0xC9, 0xCD, 0xBB, 0xBA,
      0xC8, 0xBC (sentinel records the codes; assert the exact list).
- [ ] MA1-3 Block and shade glyphs DB, B0, B1, B2, DC, DF map to themselves (VT_GLYPH_FONT).
      Any code that maps to the '?' fallback at vtbuiltin.c:55 fails the test.
- [ ] MA1-4 16 colours: "\e[0;31m" gives fg pen 1; "\e[1;31m" gives pen 9; "\e[1;33;44m"
      gives fg 14, bg 4. Assert term_pen_fg and term_pen_bg at term_glyph time.
- [ ] MA1-5 iCE colours: with iCE on, "\e[5;44m" gives bg pen 12 and no blink flag; with
      iCE off, bg pen 4 and blink set. Find how the engine switches iCE (grep vtengine.h
      for the mode; vt_resolve_colors takes it from the term). Add the switch to DCTelnet
      if the engine has one (prefs knob, see MA1-6).
- [ ] MA1-6 Every new knob (iCE colours on/off) is a pref with a menu or requester label
      in full words ("iCE Colours" is the established name; spell "Bright Backgrounds"
      in the label if unsure). It moves the output (MA1-5) or carries a written exemption.
- [ ] MA1-7 Blink: attr BLINK cells are either drawn blinking by a timer through the
      existing cursor timer path, or drawn steady with a documented exemption. Owner decision
      (2026-10-30): steady plus iCE respected (no timer), exemption written.
- [ ] MA1-8 Cursor save/restore (ESC[s ESC[u), cursor position (ESC[row;colH), erase
      (ESC[2J, ESC[K): after the sample bytes, vt_cursor equals the expected x,y (assert
      numbers) and VtSyncCursor put cur_x, cur_y there.
- [ ] MA1-9 Sample: take a real BBS ANSI art file (80 columns, over 25 rows, with blink and
      line art). Put it in tests/ansi_art/ with its source named in a README line in the
      test. Host test feeds it in 1024-byte chunks (the ConWrite chunk size) and asserts
      the grid hash (CRC of all cells) equals a value fixed in the test, plus the count of
      VtDraw cells > 0 (sentinel).
- [ ] MA1-10 Rig test through DCTelnet: connect to the local test server that sends the
      sample; read the screen back (wasabi text) and compare the 80x25 text to the grid
      expected by MA1-9; assert the DCTelnet debug sentinel (add a VtWrite call counter to
      DCTelnet-debug.h) is above 0 and the console.device write count is 0.

## Done

All ten items DONE with evidence: host test output with the sentinel counts, rig run log
with the screen text diff empty. `make ci` and `make -C tests` pass. Owner check (looks):
"Run DCTelnet on the rig, connect to the test server, art file X shows without broken
line art or wrong colours." PASS: art matches the same file in a PC terminal. FAIL: any
'?' or wrong-coloured block.
