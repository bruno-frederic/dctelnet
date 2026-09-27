/* src/petscii_dispatch.h */
#ifndef PETSCII_DISPATCH_H
#define PETSCII_DISPATCH_H

#include <stdint.h>

struct PetsciiDispatchState {
    int cols, rows;
    int cursor_row, cursor_col;
    int reverse;          /* reverse-video flag: RVS ON/OFF (18/146) */
    int shift_lowercase;  /* 0 = unshifted/graphics charset, 1 = shifted/lowercase */
    int bg_color;         /* current background color, VIC index 0-15 */
    int color;            /* current text colour: index into the colour table,
                           * -1 = console default (no colour code seen yet) */
};

void petscii_dispatch_init(struct PetsciiDispatchState *st, int cols, int rows);

/*
 * Feeds one PETSCII byte. Control bytes (0x00-0x1F, 0x80-0x9F) update
 * *st and set *out_is_control = 1; printable bytes set *out_is_control = 0
 * and write the screen code to render (reverse-video bit applied) into
 * *out_screencode.
 */
void petscii_dispatch_byte(struct PetsciiDispatchState *st, uint8_t byte,
                            uint8_t *out_screencode, int *out_is_control);

#include <stddef.h>

/*
 * Largest number of output bytes one input byte can produce in either
 * stream function below: a full attribute rebuild, e.g. RVS ON with a bright
 * colour active ("\x1b[0m\x1b[1m\x1b[37m\x1b[7m", 17). CR's CR+LF never
 * reaches it, because CR clears reverse. Callers translate input in chunks
 * of at most (output buffer / this) bytes, so output is never truncated.
 * test_petscii_dispatch checks every byte value against it.
 */
#define PETSCII_MAX_OUT_PER_BYTE 17

/*
 * Written to the console once when it opens in PETSCII mode: turns the
 * console's own auto-wrap (DECAWM) off. The stream functions below emit
 * the C64's 40-column wrap themselves; a console that also wrapped added a
 * second line feed on every full row once the 16-pixel PETSCII font made
 * the window exactly 40 columns wide (ibmcon.device wraps immediately
 * after the last column). ibmcon and console.device both support CSI ?7l.
 */
#define PETSCII_CONSOLE_SETUP "\x1b[?7l"

/*
 * Fallback rendering path, used when Petscii.font is not installed:
 * translates a PETSCII byte stream into CP437 lookalikes plus the
 * ANSI/ECMA-48 CSI sequences DCTelnet's existing ibmcon.device console
 * already understands (the same ESC_STR "[A".."[D" cursor codes DCTelnet.c
 * sends for its own cursor keys), so it reuses the existing renderer.
 *
 * Recognized control bytes: cursor up/down/left/right, home, clear+home,
 * reverse video on/off, CR (emitted as CR+LF). Anything else no-ops (dropped), same as
 * petscii_dispatch_byte's own unhandled-control-byte behavior.
 *
 * Returns the number of bytes written to out (truncates safely at out_max).
 */
size_t petscii_stream_to_ansi(struct PetsciiDispatchState *st,
                               const uint8_t *in, size_t in_len,
                               uint8_t *out, size_t out_max);

/*
 * Real-font rendering path: same control-code -> ANSI-CSI translation as
 * petscii_stream_to_ansi(), but printable bytes pass the RAW PETSCII byte
 * through unchanged (NOT the C64 hardware screen code -- the actual font
 * glyph table this pairs with, extracted from SyncTerm's allfonts.c, is
 * indexed by raw PETSCII byte value, confirmed by direct inspection:
 * slot 0x01 (the screencode for 'A') is blank, slot 0x41 (raw PETSCII/
 * ASCII 'A') holds the real letter shape. Reverse video is NOT folded
 * into the byte here -- it relies entirely on the ANSI ESC[7m/ESC[0m
 * append_control_ansi() already emits for RVS ON/OFF, same as the CP437
 * fallback path, since this font has no screencode-style +128 mirror to
 * exploit. Only correct when the caller has loaded a real 256-glyph
 * Amiga strike font indexed this way at the console's current font slot
 * -- callers fall back to petscii_stream_to_ansi() when no such font is
 * installed.
 */
size_t petscii_stream_to_rawglyphs(struct PetsciiDispatchState *st,
                                    const uint8_t *in, size_t in_len,
                                    uint8_t *out, size_t out_max);

#endif /* PETSCII_DISPATCH_H */
