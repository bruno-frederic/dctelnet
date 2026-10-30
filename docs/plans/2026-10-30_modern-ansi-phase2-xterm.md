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

- [x] MA2-1 Pref "Terminal Type" (Automatic, BBS, Unix) exists in PrefsStruct, default
      Automatic, saved and loaded (test_prefs_file: round trip; old files load as Automatic).
      DONE: test_prefs_file.c test_terminal_type_is_saved_and_an_older_file_loads_automatic
- [x] MA2-2 Site override: SITE_GROUP covers it; test_site_prefs: an entry with Unix
      applies, Restore puts the global back.
      DONE: test_site_prefs.c test_each_group_moves_only_its_members (TermType Unix applies, summary ", Unix", global restored)
- [x] MA2-3 Requester and menu labels in full words ("Terminal Type", "Automatic", "BBS", "Unix").
      Rig check: the requester shows and changes the value.
      DONE: host part: labels in menu table and Site Settings (DCTelnet.c, guis.c; compile-unverified on host). Rig check of the layout: PASS on rig 3 2026-10-30 (80x30 grid 640x240 px in a 642x240 inner area, nothing clipped); menu not rig-checked
- [x] MA2-4 VtOpen uses VT_XTERM + VT_CS_UTF8 for Unix. Host test: vt_personality() ==
      VT_XTERM after VtOpen with Unix prefs, VT_PCANSI with BBS prefs. Automatic: UTF-8
      Character Set gives VT_XTERM, CP437 and Latin-1 give VT_PCANSI (three asserts); an
      explicit BBS with UTF-8 gives VT_PCANSI (override wins).
      DONE: test_vtpersonality.c test_terminal_type_picks_the_personality
- [x] MA2-5 DisplayBytes skip: with builtin + Unix, the bytes E2 96 88 reach VtWrite
      unchanged (sentinel on VtWrite args), and U+2588 arrives in the cell as 0x2588.
      DONE: test_vtpersonality.c test_engine_decodes_utf8_only_when_xterm_utf8_builtin + test_term_engine.c test_ma2_5_11_utf8_reaches_the_grid_whole_even_split_across_writes (the DisplayBytes call itself is rig item MA2-12)
- [x] MA2-6 Glyph map: U+2588 -> font code 0xDB; U+2500 -> 0xC4; U+00E9 -> 0xE9 (or the
      code glyphmap.c returns; read it from the table and fix the numbers in the test).
      A code point not in the font (U+2603) draws the fallback glyph, count = 1, and the
      fallback code is asserted. Wide characters (CJK): decide width 2 or fallback, assert.
      DONE: test_term_engine.c test_ma2_6_glyph_map_and_fallback
- [x] MA2-7 Pen mapper `Vt_PenFor(vt_color, screen)`: palette 0-15 pass through; 16-255
      use vt_rgb_to_256 tables plus the xterm 256 RGB, then nearest of the screen's pens
      (16-pen screen: nearest by RGB distance; 256-pen screen: allocate or use index).
      Pure function in its own file, unit tested: (0xFF0000 -> index 196 -> nearest pen
      asserted), (0x5F87AF -> index 67), vt_rgb_to_256(0x000000)=16, (0xFFFFFF)=231.
      DONE: test_vtpens.c
- [x] MA2-8 VtDraw uses the mapper instead of `& 15`. Host test: "\e[38;5;196m" and
      "\e[38;2;255;0;0m" give the same term_pen_fg on a 16-pen screen; assert the number.
      DONE: test_term_engine.c test_ma2_8_256_colour_and_24_bit_give_the_same_pen
- [ ] MA2-9 24-bit on a true-colour (RTG) screen: RGB drawn directly or through
      allocated pens; rig test on an RTG screen: a cell written "\e[38;2;18;52;86m" reads
      back the right RGB from the bitmap pixel (assert 0x12 0x34 0x56).
- [ ] MA2-10 [16 pens DONE rig 3 2026-10-30: 0xFF0000 -> pen 9 = host Vt_PenFor on the rig palette, 0x123456 -> 8 both, no crash; 256 pens open] 24-bit on a palette screen (AGA, 16 and 256 pens): rig test, pen chosen is the
      mapper's answer (number from MA2-7), no crash, cell shows the colour.
- [x] MA2-11 UTF-8 split across reads: bytes E2 | 96 88 sent in two VtWrite calls give
      one cell 0x2588 (the engine keeps state).
      DONE: test_term_engine.c test_ma2_5_11_utf8_reaches_the_grid_whole_even_split_across_writes
- [x] MA2-12 (rig 3, 2026-10-30, Charset UTF-8: rows read back as box drawing + "café üñ" codepoints 250C 2500 2510 2502 2514 2518 E9 FC F1; VtWrite count 8, console writes 0, UTF-8->CP437 count 0; 256-colour 196 -> pen 9, 24-bit 0x123456 -> pen 8) Reachability: rig test through DCTelnet connected to tools/ssh_test_server.py
      (or a telnet echo server) sending a UTF-8 file with box drawing, accents, a 256 and a
      24-bit colour line. Screen text read back equals the expected rows; sentinel: VtWrite
      call count > 0, DisplayBytes UTF-8->CP437 count == 0 on this path.

## Done

12 of 12 with host and rig evidence. Existing UTF-8 behaviour for the console renderers
unchanged (MA0-1 still green). Owner check: "Unix site with UTF-8 shows box drawing and
accents; 24-bit gradient line looks smooth on the RTG screen." PASS: no '?' blocks for
characters the font has.

## Implementation decisions (MA2, host part)

Items MA2-1 to 8 and 11. MA2-9, 10, 12 (rig / RTG) stay open for a later session.

- Pref byte: PrefsStruct has NO free Reserved byte left (MA1 took the last). `UBYTE TermType`
  is appended after Blink with `UBYTE Reserved[3]` so the struct stays a multiple of 4. The
  struct grows by 4 bytes, which the v2 file format is built for (Prefs_Decode copies
  min(dataSize, sizeof) and zero-fills the rest), so no version bump: an old file loads with
  TermType 0 = Automatic. Verified in test_prefs_file (old-size file -> Automatic; round trip).
  Site files are the same (dataSize in the header). site_prefs.c lists every byte: F(TermType),
  F(Reserved). prefs.c load clamps a value above TERMTYPE_UNIX to Automatic.
- Values: `TERMTYPE_AUTO` 0 (default), `TERMTYPE_BBS` 1, `TERMTYPE_UNIX` 2.
- Resolution (Vt_PersonalityFor): AUTO -> Character Set UTF-8 gives VT_XTERM, CP437 and Latin-1
  give VT_PCANSI. BBS -> VT_PCANSI always. UNIX -> VT_XTERM always.
- Engine charset: PCANSI -> VT_CS_CP437 (as MA1; with Character Set UTF-8 DisplayBytes still
  converts to CP437 first). XTERM -> follows the Character Set: UTF-8 -> VT_CS_UTF8, Latin-1 ->
  VT_CS_LATIN1, CP437 -> VT_CS_CP437. The plan text says "Unix = VT_XTERM + VT_CS_UTF8"; an
  explicit Unix site whose Character Set still says IBM PC would otherwise decode CP437 bytes
  as UTF-8 and show garbage. OPEN QUESTION for the owner: should picking Unix imply UTF-8?
  Default taken: no, the Character Set stays authoritative for the byte decoding.
- DisplayBytes skip: new pure `Vt_EngineDecodesUtf8(prefs)` = built-in renderer AND engine
  charset VT_CS_UTF8. DisplayBytes returns the bytes untouched then. The scrollback keeps CP437
  text: AddBuf gets a CP437 conversion made with its own decoder (utf8Sb), so the scrollback
  viewer is unchanged.
- Menu: Terminal > "Terminal Type" submenu (Automatic, BBS, Unix; radio, CHECKED from
  prefs.TermType) right after Character Set. The handler sets prefs.TermType and calls
  `VtApplyPersonality()` (new, vtbuiltin.c, shared with VtOpen); the Character Set handler
  calls it too (the engine needs vt_set_charset on the same event). Site Settings window: a
  "Terminal Type" cycle gadget in the Terminal group row of Renderer and Characters (those two
  are narrowed to fit). Rig check of the terminal grid layout: PASS (rig 3, 2026-10-30).
- Glyph map: for every personality the built-in renderer's font is the IBM CP437 set (MA1
  assumption, unchanged), so VtDraw keeps VT_ENC_CP437. Any code point whose glyph is not
  VT_GLYPH_FONT draws the fallback `VT_FALLBACK_GLYPH` = '?' (0x3F), exactly one cell.
  Wide characters (cell width 2): the font has single-width cells, so the first cell draws the
  fallback glyph and the continuation cell (width 0) draws a space. OPEN QUESTION: a Latin-1
  system font with Unix would need VT_ENC_LATIN1; there is no font-encoding query yet.
- Pen mapper: new pure src/vtpens.c/.h, `UBYTE Vt_PenFor(vt_color c, const UWORD ansi[16])`.
  ansi is the screen's 16 pens in RGB4 (prefs.AnsiColors; pen n holds ANSI colour n).
  Formula: VT_COLOR_DEFAULT -> 7, VT_COLOR_DEFAULT_BG -> 0; c < 16 -> c; 16 <= c < 256 ->
  xterm RGB of c (6x6x6 cube levels 0,95,135,175,215,255; greys 8+10k); VT_COLOR_RGB -> its
  0xRRGGBB. Then the pen with the least squared RGB distance among the 16 (RGB4 nibble n ->
  n*17), ties to the lowest pen. A true-colour or 256-pen screen would use its own paths
  (MA2-9/10); on those screens the 16-pen answer is what the host part gives. OSC 4 palette
  changes are not followed (the table is xterm's).
- Tests: tests/test_vtpersonality.c (MA2-4, MA2-5 predicate), test_vtpens.c (MA2-7),
  test_term_engine.c (MA2-5 path, 6, 8, 11 through VtOpen/VtWrite/VtDraw), test_prefs_file.c
  (MA2-1), test_site_prefs.c (MA2-2). MA2-3 host part: label strings only (rig open).

### Owner decisions (2026-10-30, vtcon-06)

- Unix follows the site's Character Set (the agent's version stays; the plan's "Unix = UTF-8 always" is dropped).
- Smart character set swap, both parts:
  - MA2-13 Swap when chosen: setting Terminal Type to Unix while Character Set is CP437 changes Character Set to UTF-8 in the same requester (global settings and Site Settings). The user can change it back. No swap for any other combination.
  - MA2-14 Stream detection: while connected with Terminal Type Unix and Character Set not UTF-8, the first complete valid UTF-8 multi-byte sequence, with no invalid high byte before it in this connection, switches decoding to UTF-8 for the rest of the connection. Not saved to prefs. A connection that has already seen an invalid sequence never switches.
- [x] MA2-13 Test: requester logic (pure function) maps (Unix, CP437) to UTF-8, leaves every other pair unchanged. DONE: tests/test_vtpersonality.c test_ma2_13_unix_with_cp437_swaps_to_utf8_only; wired in guis.c Site Settings Terminal Type gadget (Vt_CharsetAfterTermType).
- [x] MA2-14 Test: "\xE2\x96\x88" after plain ASCII switches to UTF-8 (cell = full block); CP437 art bytes "\xC9\xCD\xBB" before it keep CP437; Latin-1 "\xE9t\xE9" never switches. DONE: test_vtpersonality.c test_ma2_14_first_valid_utf8_sequence_fires_once_invalid_bytes_never; test_term_engine.c test_ma2_14_stream_switch_to_utf8_through_vtwrite.
  - MA2-14 fix 1 (split sequence): while the sniffer is undecided, VtWrite holds back a trailing incomplete-but-valid UTF-8 prefix (max 3 bytes; Vt_Utf8SnifferFeed reports pre/ndata) and feeds it with the next write, as UTF-8 on a switch, as CP437 if invalid; connection reset drops it. Tests: test_term_engine.c test_ma2_14_split_sequence_held_back_through_vtwrite; test_vtpersonality.c (hold counts).
  - MA2-14 fix 2 (apply keeps the switch): VtApplyPersonality uses Vt_PersonalityForConnection(prefs, sniffer): a fired sniffer keeps UTF-8 while TermType is Unix and prefs.Charset is as it was when it fired. Tests: test_term_engine.c test_ma2_14_apply_personality_keeps_the_switch; test_vtpersonality.c test_ma2_14_fired_sniffer_keeps_utf8_until_the_user_changes_prefs.
  - MA2-14 fix 3 (hold only within one receive burst): VtFlushHeld() (vtbuiltin.c; pure Vt_Utf8SnifferTakeHeld in vtpersonality.c) draws a held prefix as the current charset when main() finds nothing more pending (WaitSelect returned 0 and SshConn_Buffered() is FALSE; one site covers telnet, rlogin and SSH). An incomplete sequence is neither valid nor invalid, so the sniffer stays undecided. Test: test_term_engine.c test_ma2_14_flush_draws_held_byte_split_block_still_whole (fails rc 134 without the flush call).
