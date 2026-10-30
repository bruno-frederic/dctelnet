# MA2 Phase 2: xterm personality, UTF-8 through the glyph map, 256 and 24-bit colour

Parent: 2026-10-30_modern-ansi-engine.md. Needs MA1 (MA1-1 especially).

## Goal

Unix and ssh sites get the VT_XTERM personality, read UTF-8 in the engine, map code points
to the screen font with a fallback glyph, and show 256-colour and 24-bit colour on the
pens the screen really has.

## Files to touch

- src/vtbuiltin.c:152-165 VtOpen: choose VT_XTERM and VT_CS_UTF8 from the function made in
  MA1-1. Today only VT_PCANSI (line 160).
- src/vtbuiltin.c:29-60 VtDraw: glyph lookup uses VT_ENC_CP437 (line 52-54). For xterm
  use the encoding the font has; fallback glyph where g.kind is not VT_GLYPH_FONT
  (today '?', line 55).
- src/vtbuiltin.c:47-48 colour: `fg & 15`. Replace with a pen mapper (MA2-7).
- src/DCTelnet.c:3101-3105 DisplayBytes: converts UTF-8 to CP437 before the engine. With
  the builtin renderer and the xterm personality, skip it (engine decodes). Keep it for the
  console renderers. Callers: DCTelnet.c:2164, 2287, 3116 (echo).
- src/DCTelnet.c:1268-1269, 4364-4368: utf8In and utf8Echo init on Character Set change;
  the engine needs vt_set_charset on the same event (menu handler).
- src/DCTelnet.c:3449 (typed char >= 0x80 with CHARSET_UTF8): sending side; check it
  matches what vt_encode_key (vtengine.h:250) produces.
- src/prefs.h, src/prefs_file.c, src/site_prefs.c:63,130, src/guis.c:1238,1543: the new
  "Terminal Type" pref (owner decision 2: Automatic, BBS, Unix; default Automatic, which
  derives BBS or Unix from the Character Set; BBS and Unix override it). Add the field, the file format migration
  (prefs_file.c has an old[OLD_...] reader at line 127), site override, requester.
- src/DCTelnet.c:1679, 5055, 406: where ansiColourPens are set (SetRGB32, term_set_pens);
  the mapper reads these.
- src/third_party/vtcon/engine/vtengine.h:218 vt_rgb_to_256; render/glyphmap.h:29.
- tests/test_term_engine.c, tests/test_prefs_file.c, tests/test_site_prefs.c.

## Checklist

- [ ] MA2-1 Pref "Terminal Type" (Automatic, BBS, Unix) exists in PrefsStruct, default
      Automatic, saved and loaded (test_prefs_file: round trip; old files load as Automatic).
- [ ] MA2-2 Site override: SITE_GROUP covers it; test_site_prefs: an entry with Unix
      applies, Restore puts the global back.
- [ ] MA2-3 Requester and menu labels in full words ("Terminal Type", "Automatic", "BBS", "Unix").
      Rig check: the requester shows and changes the value.
- [ ] MA2-4 VtOpen uses VT_XTERM + VT_CS_UTF8 for Unix. Host test: vt_personality() ==
      VT_XTERM after VtOpen with Unix prefs, VT_PCANSI with BBS prefs. Automatic: UTF-8
      Character Set gives VT_XTERM, CP437 and Latin-1 give VT_PCANSI (three asserts); an
      explicit BBS with UTF-8 gives VT_PCANSI (override wins).
- [ ] MA2-5 DisplayBytes skip: with builtin + Unix, the bytes E2 96 88 reach VtWrite
      unchanged (sentinel on VtWrite args), and U+2588 arrives in the cell as 0x2588.
- [ ] MA2-6 Glyph map: U+2588 -> font code 0xDB; U+2500 -> 0xC4; U+00E9 -> 0xE9 (or the
      code glyphmap.c returns; read it from the table and fix the numbers in the test).
      A code point not in the font (U+2603) draws the fallback glyph, count = 1, and the
      fallback code is asserted. Wide characters (CJK): decide width 2 or fallback, assert.
- [ ] MA2-7 Pen mapper `Vt_PenFor(vt_color, screen)`: palette 0-15 pass through; 16-255
      use vt_rgb_to_256 tables plus the xterm 256 RGB, then nearest of the screen's pens
      (16-pen screen: nearest by RGB distance; 256-pen screen: allocate or use index).
      Pure function in its own file, unit tested: (0xFF0000 -> index 196 -> nearest pen
      asserted), (0x5F87AF -> index 67), vt_rgb_to_256(0x000000)=16, (0xFFFFFF)=231.
- [ ] MA2-8 VtDraw uses the mapper instead of `& 15`. Host test: "\e[38;5;196m" and
      "\e[38;2;255;0;0m" give the same term_pen_fg on a 16-pen screen; assert the number.
- [ ] MA2-9 24-bit on a true-colour (RTG) screen: RGB drawn directly or through
      allocated pens; rig test on an RTG screen: a cell written "\e[38;2;18;52;86m" reads
      back the right RGB from the bitmap pixel (assert 0x12 0x34 0x56).
- [ ] MA2-10 24-bit on a palette screen (AGA, 16 and 256 pens): rig test, pen chosen is the
      mapper's answer (number from MA2-7), no crash, cell shows the colour.
- [ ] MA2-11 UTF-8 split across reads: bytes E2 | 96 88 sent in two VtWrite calls give
      one cell 0x2588 (the engine keeps state).
- [ ] MA2-12 Reachability: rig test through DCTelnet connected to tools/ssh_test_server.py
      (or a telnet echo server) sending a UTF-8 file with box drawing, accents, a 256 and a
      24-bit colour line. Screen text read back equals the expected rows; sentinel: VtWrite
      call count > 0, DisplayBytes UTF-8->CP437 count == 0 on this path.

## Done

12 of 12 with host and rig evidence. Existing UTF-8 behaviour for the console renderers
unchanged (MA0-1 still green). Owner check: "Unix site with UTF-8 shows box drawing and
accents; 24-bit gradient line looks smooth on the RTG screen." PASS: no '?' blocks for
characters the font has.
