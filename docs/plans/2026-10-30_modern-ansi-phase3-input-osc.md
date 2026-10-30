# MA3 Phase 3: mouse reporting, OSC title, bracketed paste

Parent: 2026-10-30_modern-ansi-engine.md. Needs MA2 (the xterm personality asks for these).

## Goal

Programs on the far side (tmux, vim, irssi) get mouse events, can set the window title, and
receive pasted text wrapped in bracketed-paste markers. All replies go out through the
existing reply path (VtReply, vtbuiltin.c:128-135, to send_data).

## Files to touch

- src/vtbuiltin.c:153 callback table: title slot (5th entry) is NULL. Add VtTitle.
- src/vtbuiltin.c:128-135 VtReply: reused unchanged for mouse bytes (same unlock rules).
- src/DCTelnet.c:3947 `case IDCMP_MOUSEBUTTONS` and the selection code at 3662-3900: mouse
  selection exists. When vt_modes() has VT_MODE_MOUSE_* bits (vtengine.h:120-124), send
  vt_encode_mouse (vtengine.h:255) instead of selecting; Shift keeps selecting.
- src/DCTelnet.c:5493 and 5619: IDCMP flags of the two window open paths. IDCMP_MOUSEMOVE
  is needed for button-motion and any-motion modes (add only while those modes are on).
- src/DCTelnet.c:3859 PasteClipboard (line 3880 conversion): wrap with vt_encode_paste
  (vtengine.h:257, begin=0 end=1) when VT_MODE_BRACKET_PASTE is on.
- Window title: where the main window title is set (grep -a SetWindowTitles / WA_Title in
  DCTelnet.c; none found for the terminal window at the time of writing, so find the
  window open call near 5493/5619 and add the setter). Title text is UTF-8: convert to
  Latin-1 for the Amiga title bar.
- src/third_party/vtcon/engine/vtengine.h:106-113 callback docs.
- tests/test_term_engine.c.

## Checklist

- [x] MA3-1 [DONE a1c56a7: test_ma3_1_2_osc_title_reaches_the_window_as_latin1] OSC title: "\e]0;hello\a" through VtWrite calls VtTitle once with "hello"
      (sentinel count 1, string asserted). Also OSC 2 and the ST terminator.
- [ ] MA3-2 [host DONE a1c56a7: test_ma3_1_2_osc_title_reaches_the_window_as_latin1 (host half; rig half open)] Title reaches the window: the setter is called with "hello"; rig check reads the
      window title text back. UTF-8 title "caf\xC3\xA9" is shown as Latin-1 0xE9.
- [x] MA3-3 [DONE a1c56a7: test_ma3_3_title_is_restored_on_reset_and_session_end] Title is restored to DCTelnet's own text when the session ends or resets.
- [x] MA3-4 [DONE a1c56a7: test_ma3_4_5_mouse_press_release_bytes_and_selection_fallthrough] Mouse press: with "\e[?1000h\e[?1006h" sent, a left click at cell (10,5) sends
      the bytes of "\e[<0;11;6M" (SGR, 1-based). Assert the exact bytes at send_data.
      Release gives "\e[<0;11;6m". Wheel and right button codes asserted (64/65, 2).
- [x] MA3-5 [DONE a1c56a7: test_ma3_4_5_mouse_press_release_bytes_and_selection_fallthrough] With no mouse mode on, a click still starts a selection (existing behaviour
      unchanged; sentinel: send_data not called).
- [x] MA3-6 [DONE a1c56a7: test_ma3_6_motion_modes_and_the_mouse_move_flag, test_ma3_6_idcmp_move_only_for_motion_modes] Motion modes: ?1002 reports drag only while a button is held, ?1003 reports all
      motion. IDCMP_MOUSEMOVE enabled only while the mode is on; asserted by the flag set
      on the window after the mode switch.
- [x] MA3-7 [DONE a1c56a7: test_ma3_7_bracketed_paste_wraps_exactly] Bracketed paste: with "\e[?2004h", pasting "abc" sends "\e[200~abc\e[201~"
      (exact bytes). Without the mode, only "abc".
- [x] MA3-8 [DONE a1c56a7, 8-bit CSI fix in the same test: test_ma3_8_paste_cannot_inject_the_end_marker strips ESC, 0x9B and UTF-8 C2 9B] Paste cannot inject the end marker: a clipboard text holding "\e[201~" has the
      ESC removed or the marker stripped; assert the output bytes.
- [ ] MA3-9 Reachability: rig run, DCTelnet connected to a test server that enables
      ?1000/?1006/?2004, sets a title, and logs what it receives. Click and paste on the rig
      (wasabi mouse and key); the server log equals the expected byte strings above.

## Done

9 of 9 with the server log as evidence for MA3-4, 5, 7, 9. Owner check: "tmux or vim over
ssh: click moves the cursor, title shows the host, paste does not trigger auto-indent."
