# Modern ANSI for DCTelnet through the embedded vtcon engine

Date: 2026-10-30. Status: plan only, nothing implemented.
Tree: ~/Code/DCTelnet/dctelnet-vtcon, branch feature/pr-vtcon-engine (HEAD b782687).
Design source: /Users/spot/Code/UP-Term/vtcon/thoughts/shared/research/2026-10-30_dctelnet-modern-ansi.md
Owner decision: the engine route. Not up-console.device.

## Goal

DCTelnet's built-in renderer (APP_RENDERER_BUILTIN) uses the vtcon engine to parse and
retro32-term to draw. Make it a modern terminal: BBS art done right, then Unix/ssh
sites with UTF-8 and wide colour, then mouse, title, paste.

## The phases (each is its own file)

| File | Phase | IDs |
|---|---|---|
| 2026-10-30_modern-ansi-phase0-housekeeping.md | MA0 housekeeping | MA0-1 .. MA0-7 |
| 2026-10-30_modern-ansi-phase1-bbs.md | MA1 BBS: PC ANSI, CP437, 16 colours, iCE | MA1-1 .. MA1-10 |
| 2026-10-30_modern-ansi-phase2-xterm.md | MA2 xterm personality, UTF-8, 256 and 24-bit | MA2-1 .. MA2-12 |
| 2026-10-30_modern-ansi-phase3-input-osc.md | MA3 mouse, OSC title, bracketed paste | MA3-1 .. MA3-9 |
| 2026-10-30_modern-ansi-phase4-scrollback.md | MA4 scrollback (owner decision first) | MA4-1 .. MA4-5 |

Total: 43 items. Order: MA0 and MA1 first, then MA2, MA3. MA4 is unblocked (owner chose engine scrollback).

## What the code looks like today (read before you start)

- DCTelnet.c is Latin-1 and grep treats it as binary. Use `grep -a`.
- Renderer choice: `STATE_IS(APP_RENDERER_BUILTIN)` in ConWrite, DCTelnet.c:417-465. Builtin
  calls VtWrite (vtbuiltin.c, included at DCTelnet.c:384). Console, ibmcon and Xem branch
  after it. These stay as the fallback.
- The engine is created in VtOpen, vtbuiltin.c:152-165 (called from DCTelnet.c:408 right after
  term_init_area at 402). It is fixed to VT_PCANSI (line 160) and VT_CS_CP437 (line 161).
  The callback table at line 153 has NULL for bell, title, layout, colors.
- Drawing is VtDraw, vtbuiltin.c:29-60. It cuts every colour with `fg & 15` and `bg & 15`
  (lines 47-48), so 256 and 24-bit colours are lost. Blink and every attribute except
  underline are ignored.
- Replies (DSR, DA) go out through VtReply to send_data. Mouse, paste and key encoders
  exist in the engine (vt_encode_mouse, vt_encode_paste, vt_encode_key, vtengine.h:250-257).
  DCTelnet calls none of them.
- A Character Set pref already exists: CHARSET_CP437/LATIN1/UTF8 (charset.h:22-24), per site
  (site_prefs.c:63, 130), menu at DCTelnet.c:164-167 and 4364-4368, edit in guis.c:1238, 1543.
  For UTF-8 the bytes are converted to CP437 BEFORE the engine, in DisplayBytes
  (DCTelnet.c:3101-3105, Charset_Utf8ToCp437, charset.c:87). The engine never sees UTF-8.
- Host tests: tests/test_term_engine.c builds vtbuiltin.c with stubs (tests/Makefile:6, 108-111)
  and drives VtOpen/VtWrite. It cannot link DCTelnet.c. See "Definition of done" below.

## Definition of done, all phases (REACHABILITY_PROTOCOL gates 1-4)

Source: ~/.claude/REACHABILITY_PROTOCOL.md sections 1, 8, 9.

1. Every item has its ID in the commit message that closes it.
2. Red first: the test is written and seen failing before the code.
3. One reachability test per phase. It drives the product's entry point, proves the new
   code ran with a counter or sentinel, and asserts concrete numbers (pens, glyph codes,
   bytes sent). Other behaviour is tested where it lives (VtDraw mapping, a pure function).
4. Entry point: DCTelnet.c cannot be linked on the host. So each phase has two proofs:
   - Host: test_term_engine drives VtWrite, the same function ConWrite calls at
     DCTelnet.c:426, with a sentinel counting VtDraw cells and VtReply bytes.
   - Rig: DCTelnet itself on an UP-Term rig (start with `UPTERM_RIG=<n> rig.py start --max
     --purpose "DCTelnet: <phase> check"`), connected to the local BBS test server
     (tools/ssh_test_server.py for ssh), feeding the sample file. Read pens and glyphs back
     from the screen text (wasabi), not by eye. A human screenshot check is listed under
     "Blocked on owner" only for looks.
5. Every new pref or knob moves the output, or carries a written exemption.
6. The plan closes at zero open items. Report "N of 43, M open", never "done".
7. Before the first run, `make ci` from src/ (AGENTS.md) and `make -C tests` pass.
   One test run on the machine at a time; max 2 workers.

## Owner decisions (2026-10-30)

1. MA4: scrollback lives in the engine (option 1 in the phase 4 file). DCTelnet's own
   scrollback store goes for the built-in renderer.
2. MA2: both. The Character Set pref derives the default personality (CP437 and Latin-1:
   BBS; UTF-8: Unix). A new per-site "Terminal Type" pref (Automatic, BBS, Unix; default
   Automatic) overrides it when set. Automatic follows the Character Set.
3. MA2: fallback glyph is the font's box ('?' only when the font has no box glyph).
4. MA1: blink is drawn steady with a written exemption; iCE colours respected; no timer.

## Findings against the research (checked in code)

- Research says UTF-8 is open and the xterm personality is not selected. True for the
  engine, but a UTF-8 to CP437 path already exists before the engine (DisplayBytes). MA2
  replaces it for the builtin renderer; the console renderers keep it.
- Research says a per-connection personality choice is missing. The Character Set pref is
  already per site. Only the personality is fixed (vtbuiltin.c:160).
- Research lists "24-bit on palette screens: vt_rgb_to_256" as done (4ffdf28). That commit
  only moved the submodule. vt_rgb_to_256 is declared (vtengine.h:218) and tested inside
  vtcon, but nothing in dctelnet-vtcon/src calls it, and VtDraw masks to 4 bits.
- repos.lock (UP-Term/upterm/repos.lock:19) pins dctelnet-vtcon to branch
  feature/vtcon-engine at a233a0d. The real branch is feature/pr-vtcon-engine at b782687.
