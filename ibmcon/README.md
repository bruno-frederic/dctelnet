# ibmcon.device -- annotated disassembly (1.4 bug-fixed, 1.5 to 1.11)

The ANSI console DCTelnet draws its terminal with. `ibmcon.device` 1.4
(Mar 9 1998) was freeware and shipped as a binary only; this drawer holds
its reassembled, annotated source and the fixes and additions DCTelnet's
PETSCII, RTG and 256-colour work needed. `make` builds `ibmcon.device`;
DCTelnet ships it in its own `Devs` drawer and opens the newest copy it
finds there or in `DEVS:`.

Source: reassembled dump of `ibmcon.device` 1.4 (Mar 9 1998) from
https://www.computerdevices.it/vintage_amiga_page/download/ibmcon.device.asm

| File | Content |
|------|---------|
| `ibmcon.device.original.asm` | Untouched In_Go Reassembler dump (auto labels, no comments) |
| `ibmcon.device.bugfixed.asm` | Restructured version: reconstructed label names, struct/offset EQUs, full commentary, the 1.4 bug fixes (each marked `FIXED:` in the code, list in the file header) and the 1.5/1.6 additions below |

## What it is

An Exec device (SAS/C-compiled C code) that emulates an IBM-PC style
ANSI.SYS text console inside an Intuition window.

* `OpenDevice("ibmcon.device", unit, ioreq, 0)` with `io_Data` = an open
  `struct Window *`.
* Only `CMD_WRITE` is supported (`io_Length = -1` means NUL-terminated).
* The write stream is interpreted: CSI sequences (`ESC[` / 8-bit `$9B`),
  IBM control characters, SO/SI switching into the high-bit half of an
  IBM font (CP437 graphics).
* Rendering goes straight into the window RastPort via graphics.library.
* Each opener gets a private clone of the device base and a private
  handler process (`IBMCON_Handler`); `CloseDevice` kills it with the
  private command `$7FF0`.
* Unit number: 0 = Amiga pen order; >=1 swaps pens 1/7 (IBM colour
  order).

Full architecture description, struct layouts, the SAS/C autoinit
`$FC`-byte relocation trick, and the list of original bugs (fixed in
the bug-fixed version) are in the header of `ibmcon.device.bugfixed.asm`.

Fixed bugs, in short: CSI `S` scroll direction; SetSoftStyle italic
enable mask; erasing now uses the SGR background pen instead of pen 0;
text placed on the font's real baseline; UnitOpen failure handling +
port leak; DevOpen sets `io_Error`/`io_Device` properly; CreatePort
signal-allocation check sign-extended; column clamp + text-run flush
against 200-byte buffer overflows; CSI parameters saturate at 255
instead of wrapping; unit >=2 two-line bottom-scroll special case
removed; top-margin scroll no longer one line short; CSI `J` mode 1
no longer erases part of the line twice; CSI `r` is now a real
DECSTBM vertical scroll region (validated top;bottom, cursor homed,
bounds the margin scrolling and therefore LF and wrapping; IL/DL,
SU/SD and erases deliberately stay whole-window, ANSI.SYS style).

## Verification

`make` assembles `ibmcon.device.bugfixed.asm` with vasm to an installable
device binary (`ibmcon.device`; DCTelnet opens it from its own `Devs`
drawer or from `DEVS:`) and then runs `make check`:

    vasmm68k_mot -Fhunkexe -kick1hunks -nosym -o ibmcon.device ibmcon.device.bugfixed.asm

The package ships this build as `build/package/DCTelnet/Devs/ibmcon.device`;
`make check-ibmcon` in `tests/` fails when the two differ.

(`-kick1hunks` keeps the relocations as classic HUNK_RELOC32 like the
original SAS/C binary instead of the V37+ RELOC32SHORT form; `-nosym`
drops the symbol table hunks.)

Data sections assemble byte-exact to the original layout (Segment3 =
$140 bytes, all magic offsets preserved). The code section matches the
original except for the marked bug fixes.

The 1.5 and 1.6 changes were verified with DCTelnet on an emulated
A1200 (FS-UAE, AGA and Picasso96 screens) and on the Workbench.

## 1.5 (2026-09-27, for DCTelnet's RTG/256-colour work)

* **Depth 5 and deeper get colours.** The depth switch handled 1-4 only;
  256-colour AGA and RTG screens fell into the monochrome case.
* **Pen table (`IBMCMD_SETPENS`, io_Command `$7FE0`).** A client sends
  io_Data -> `UBYTE pens[16]` in ANSI order. Colour codes then stay logical
  0-15 and are mapped at every SetAPen/SetBPen/SetRast; the unit-1 pen 1<->7
  swap and the depth check of background pens no longer apply; clears use
  the mapped ANSI background instead of pen 0. Not reachable from the
  written byte stream, so a BBS cannot change it. Without the command the
  device behaves as 1.4 (plus the depth fix).
* **Cursor with a pen table:** COMPLEMENT with write mask fg^bg, so the
  cell swaps foreground and background exactly wherever the pens sit.
* Background-pen check: depth 32 no longer rejects every pen (bset is
  modulo 32).
* `make check` proves the code is plain 68000 despite the MC68020 directive.

## 1.6 (2026-09-27)

* **Deferred wrap.** A character in the last column no longer wraps at
  once: the cursor stays there with a wrap pending, and the next printable
  character wraps first (VT100/ANSI terminals). A BBS line of exactly 80
  characters followed by CR LF was two lines before -- an empty one after
  every full row. CR, LF, BS, HT, VT, FF and every CSI sequence except SGR
  cancel the pending wrap. The console record grows by a word
  (`con_WrapPending`, `CON_FRAME` $226).
* **CSI t measures the window like a resize.** Its default counted a
  GimmeZeroZero window's title bar and borders as rows; it now uses the
  same measure as a resize, clamps an explicit row count to the rows that
  fit, and ignores the multi-parameter forms (xterm window operations,
  PabloDraw 24-bit colour) that are not a page length.
* **Resize.** The grid is measured from a GimmeZeroZero window's inner
  area and measured again when the window changes size.
* **Depth.** The screen depth is read with `GetBitMapAttr(BMA_DEPTH)` on
  V39+ (an RTG bitmap's `bm_Depth` is not its depth).
* Version 1.6 (lib_Revision 6).

## 1.7 (2026-09-27)

* **CSI L (Insert Lines) moves the lines.** The scroll's bottom edge was
  `rows-1` -- a row count used as a pixel row -- so only the top pixel
  lines moved; a fullscreen editor scrolling up (ABBS, MBBS) left stale
  lines on screen. It is now `rows*YSize-1`, as in CSI M. On the last row
  it also inserted nothing (N was clamped to the rows below the cursor);
  the cursor's row now counts, as in CSI M.
* Version 1.7 (lib_Revision 7).

## 1.8 (2026-09-28)

* **CloseDevice no longer frees the opener's signal.** The handler process
  creates its command port with `CreatePort` (the signal bit is allocated
  in the handler's task: a new process's first `AllocSignal(-1)` is 31),
  but `UnitClose` deleted that port in the opener's task, so `FreeSignal`
  freed the opener's bit of the same number -- usually bsdsocket.library's
  bit 31. Connections then hung after a display change (DCTelnet issue #3).
  The handler now deletes its own port before it replies to `CMD_DIE`.
  Measured on an emulated A1200: a task holding bit 31 opens and closes the
  device -- 1.7 frees the bit, 1.8 leaves it allocated.
* Version 1.8 (lib_Revision 8).

## 1.9 (2026-09-28)

* **Cursor Left/Right stop at the margins.** CUB at column 1 went up to the
  end of the previous line and CUF at the right edge wrapped to the next
  one (with auto-wrap on); ANSI.SYS and VT100 stop there. A BBS moving back
  with `CSI 79 D` from short of column 80 drew a line too high (Absinthe's
  ticker, DCTelnet issue #11).
* **`IBMCMD_GETCURSOR` ($7FE1).** Replies with `io_Actual` = row << 16 |
  column (1-based), so a client can answer a BBS's Device Status Report
  (`CSI 6 n`): the console itself cannot send anything back.
* Version 1.9 (lib_Revision 9).

## 1.10 (2026-09-28)

* **iCE colours.** `CSI ?33h` switches iCE mode on (as in SyncTERM): SGR 5
  (blink) then gives a bright background (pens 8-15), which ANSI art drawn
  for iCE uses for its 16 background colours. SGR 25 ends it. Without iCE
  mode SGR 5 still draws nothing (there is no blink yet).
* **`CSI X` erases characters** (ECH): N cells from the cursor, which stays.
  It took the dispatch table slot of the `CSI R` stub: the table is full
  (the assembler now fails if it grows past `$DA`, or if the `$VER` string
  moves the table).
* **`ESC 7` / `ESC 8`** save and restore the cursor (DECSC/DECRC), as
  `CSI s` / `CSI u`; the restored position is clamped to a grid that
  shrank since the save.
* **DECCKM (`CSI ?1h/l`)** is kept in mode bit 3, and the new
  `IBMCMD_GETMODES` ($7FE2) replies with `io_Actual` = the mode word, so a
  client can send cursor keys as `ESC O x` when a host asks for it.
* **A private prefix applies to the whole sequence.** `h`/`l` read the
  prefix of parameter N from the Nth raw character, so `CSI ?1;7l` changed
  mode 7 without its `?`.
* **`CSI P` and `CSI @` in the last column** now act on that column (they
  did nothing).
* Version 1.10 (lib_Revision 10).

## 1.11 (2026-09-28)

* **Screen buffer.** ibmcon drew straight into the RastPort and kept
  nothing, so no client could read the screen back. It now keeps every
  cell (character, fg and bg pen, attribute flags; 4 bytes a cell,
  allocated for the grid in `MeasureGrid` and kept where a resized grid
  overlaps). Text, the erase commands (`J`, `K`, `X`), the scrolls
  (`S`, `T`, `L`, `M`, line feed and cursor up at the margins) and
  `@`/`P` all mirror themselves in it. Without memory for it the device
  works as before.
* **`IBMCMD_READTEXT` ($7FE3).** Copies row `io_Offset` (1-based) of the
  buffer to `io_Data`, at most `io_Length` bytes; `io_Actual` = bytes (0
  for a row outside the screen), `IOERR_NOCMD` without a buffer. For
  clients copying text to the clipboard or saving the screen.
* **Blink.** Cells drawn with SGR 5 blink every half second (a
  `timer.device` request in the handler, redrawn from the buffer). In iCE
  mode blink stays a bright background. Nothing is redrawn while no cell
  blinks.
* `tests/readtext_probe.c` checks the buffer on an Amiga: 17 cases, each
  drawing path read back through `IBMCMD_READTEXT`.
* Version 1.11 (lib_Revision 11).
