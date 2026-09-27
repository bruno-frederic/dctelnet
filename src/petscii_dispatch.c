/* src/petscii_dispatch.c */
#include <string.h>
#include "petscii_dispatch.h"
#include "petscii_screencode.h"
#include "petscii_fallback.h"

void petscii_dispatch_init(struct PetsciiDispatchState *st, int cols, int rows) {
    st->cols = cols;
    st->rows = rows;
    st->cursor_row = 0;
    st->cursor_col = 0;
    st->reverse = 0;
    st->shift_lowercase = 0;
    st->bg_color = 6; /* C64 power-on default: blue background */
    st->color = -1;
}

/*
 * PETSCII colour control byte -> ANSI SGR foreground sequence.
 * Byte->VIC-II index verified against amiexpress-web's own
 * PETSCII_COLOR_TO_VIC table (sdk/petscii/c64-palette.ts); VIC index ->
 * classic 8-colour ANSI (30-37, +bold for the "light"/bright C64 colours)
 * is a common-sense correspondence for maximum BBS-terminal compatibility
 * (not every ANSI console supports the 90-97 bright codes; bold+30-37 is
 * the older, more universally supported convention).
 *
 * Every attribute change is rebuilt from SGR 0 (reset) + colour + SGR 7
 * (reverse), never toggled: ibmcon.device emulates the IBM PC ANSI.SYS
 * console, which has no SGR 27 (reverse off) and no SGR 22 (bold off), so
 * "\x1b[27m" left reverse on forever and a plain colour after a bright one
 * kept its bold. SyncTerm keeps the same attribute state and repaints from it.
 *
 * Bold+colour are two SEPARATE escape sequences (\x1b[1m\x1b[3Xm), not one
 * combined \x1b[1;3Xm -- DCTelnet's own existing debug output only ever
 * uses single-parameter SGR sequences, never combined ones, so this
 * assumes the console's parser may not handle multiple ';'-separated
 * parameters in one escape.
 */
struct petscii_color_entry { const char *sgr; uint8_t byte; };
static const struct petscii_color_entry COLOR_TABLE[] = {
    {"\x1b[30m",         0x90}, /* black */
    {"\x1b[1m\x1b[37m",  0x05}, /* white */
    {"\x1b[31m",         0x1C}, /* red */
    {"\x1b[36m",         0x9F}, /* cyan */
    {"\x1b[35m",         0x9C}, /* purple */
    {"\x1b[32m",         0x1E}, /* green */
    {"\x1b[34m",         0x1F}, /* blue */
    {"\x1b[1m\x1b[33m",  0x9E}, /* yellow */
    {"\x1b[1m\x1b[31m",  0x81}, /* orange (nearest: bright red) */
    {"\x1b[33m",         0x95}, /* brown (nearest: dim yellow) */
    {"\x1b[1m\x1b[31m",  0x96}, /* light red */
    {"\x1b[1m\x1b[30m",  0x97}, /* dark grey */
    {"\x1b[37m",         0x98}, /* grey (nearest: dim white) */
    {"\x1b[1m\x1b[32m",  0x99}, /* light green */
    {"\x1b[1m\x1b[34m",  0x9A}, /* light blue */
    {"\x1b[1m\x1b[37m",  0x9B}, /* light grey (nearest: bright white) */
};
#define COLOR_TABLE_LEN (sizeof(COLOR_TABLE) / sizeof(COLOR_TABLE[0]))

/* Index of a colour control byte in COLOR_TABLE, -1 if not a colour code. */
static int petscii_color_index(uint8_t byte) {
    size_t i;
    for (i = 0; i < COLOR_TABLE_LEN; i++) {
        if (COLOR_TABLE[i].byte == byte) return (int)i;
    }
    return -1;
}

static int is_control_byte(uint8_t b) {
    return (b <= 0x1F) || (b >= 0x80 && b <= 0x9F);
}

void petscii_dispatch_byte(struct PetsciiDispatchState *st, uint8_t byte,
                            uint8_t *out_screencode, int *out_is_control) {
    if (!is_control_byte(byte)) {
        *out_is_control = 0;
        {
            uint8_t code = petscii_to_screencode(byte);
            if (st->reverse) code |= 0x80;
            *out_screencode = code;
        }
        st->cursor_col++;
        if (st->cursor_col >= st->cols) { st->cursor_col = 0; st->cursor_row++; }
        return;
    }

    *out_is_control = 1;
    switch (byte) {
        case 13:  /* CR -- also cancels reverse video (SyncTerm petscii_cr(),
                   * cross-checked against a live amiexpress-web connection) */
            st->reverse = 0;
            st->cursor_col = 0; st->cursor_row++;
            break;
        case 141: /* "shifted CR" / LF -- no reverse-video change (SyncTerm
                   * petscii_lf(), distinct from byte 13) */
            st->cursor_col = 0; st->cursor_row++;
            break;
        case 14:  /* shift to lowercase/uppercase charset */
            st->shift_lowercase = 1;
            break;
        case 142: /* shift to unshifted/graphics charset */
            st->shift_lowercase = 0;
            break;
        case 18:  /* RVS ON */
            st->reverse = 1;
            break;
        case 146: /* RVS OFF */
            st->reverse = 0;
            break;
        case 19:  /* HOME (cursor only, no clear) */
            st->cursor_row = 0; st->cursor_col = 0;
            break;
        case 147: /* CLR/HOME: clears screen, resets cursor -- Review
                    * Focus 4: does NOT touch shift or reverse state. */
            st->cursor_row = 0; st->cursor_col = 0;
            break;
        case 17:  /* cursor down */
            st->cursor_row++;
            break;
        case 145: /* cursor up */
            if (st->cursor_row > 0) st->cursor_row--;
            break;
        case 29:  /* cursor right */
            st->cursor_col++;
            break;
        case 157: /* cursor left */
            if (st->cursor_col > 0) st->cursor_col--;
            break;
        default: {
            int color = petscii_color_index(byte);
            if (color >= 0) st->color = color;
            /* Other control bytes: no state change modeled -- explicit
             * no-op, not a crash. */
            break;
        }
    }
}

static size_t append(uint8_t *out, size_t out_max, size_t len, const uint8_t *bytes, size_t n) {
    size_t i;
    for (i = 0; i < n && len < out_max; i++) out[len++] = bytes[i];
    return len;
}

static size_t append_control_ansi(uint8_t byte, uint8_t *out, size_t out_max, size_t len) {
    switch (byte) {
        /* Cross-checked against SyncTerm's cterm_petscii.c (proven working
         * against amiexpress-web): byte 13 is CR *and* turns reverse video
         * off (petscii_cr()); byte 141 is CR with NO reverse-video change
         * (petscii_lf()). Both move to the start of the NEXT line, as on a
         * C64 -- the ANSI console needs the LF for that, a bare CR only
         * returns to column 0 of the same line. */
        case 13:  { const uint8_t seq[] = {13,10}; len = append(out, out_max, len, seq, 2); break; }
        case 141: { const uint8_t seq[] = {13,10}; len = append(out, out_max, len, seq, 2); break; }
        case 145: { const uint8_t seq[] = {27,'[','A'}; len = append(out, out_max, len, seq, 3); break; }
        case 17:  { const uint8_t seq[] = {27,'[','B'}; len = append(out, out_max, len, seq, 3); break; }
        case 29:  { const uint8_t seq[] = {27,'[','C'}; len = append(out, out_max, len, seq, 3); break; }
        case 157: { const uint8_t seq[] = {27,'[','D'}; len = append(out, out_max, len, seq, 3); break; }
        case 19:  { const uint8_t seq[] = {27,'[','H'}; len = append(out, out_max, len, seq, 3); break; }
        case 147: { const uint8_t seq[] = {27,'[','2','J',27,'[','H'}; len = append(out, out_max, len, seq, 7); break; }
        /* Reverse on/off (18/146), CR's reverse-off and the colour codes
         * produce no bytes here: stream_translate() rebuilds the
         * attributes after any byte that changed them. */
        /* Byte 20 (DELETE, server-echoed backspace): SyncTerm's reference
         * shifts the rest of the line left (movetext) -- not replicated
         * here (would need ANSI DCH support this console's compatibility
         * is unverified for). Simple back-erase-back covers the common
         * case (backspacing at end of line, e.g. correcting a typed
         * username) without risking an unsupported escape sequence. */
        case 20:  { const uint8_t seq[] = {8,' ',8}; len = append(out, out_max, len, seq, 3); break; }
        /* Bell (SyncTerm's petscii_bell()): pass the raw BEL byte through
         * -- treating 0x07 as an audible/visual bell is close to
         * universal across terminal implementations, safer to rely on
         * than inventing an escape sequence for it. */
        case 7:   { uint8_t bel = 7; len = append(out, out_max, len, &bel, 1); break; }
        default:
            /* Unrecognized control byte (bell, insert, reserved slots,
             * etc.): no ANSI equivalent modeled yet, dropped rather
             * than printed as garbage -- matches SyncTerm's own documented
             * "drop" behaviour for reserved/unhandled control bytes. */
            break;
    }
    return len;
}

/* SGR 0, then the active colour, then reverse -- see COLOR_TABLE. */
static size_t append_attributes(const struct PetsciiDispatchState *st,
                                uint8_t *out, size_t out_max, size_t len) {
    static const uint8_t reset[] = {27,'[','0','m'};
    static const uint8_t reverse[] = {27,'[','7','m'};

    len = append(out, out_max, len, reset, sizeof(reset));
    if (st->color >= 0) {
        const char *sgr = COLOR_TABLE[st->color].sgr;
        len = append(out, out_max, len, (const uint8_t *)sgr, strlen(sgr));
    }
    if (st->reverse) len = append(out, out_max, len, reverse, sizeof(reverse));
    return len;
}

/*
 * Shared loop of both render paths. raw_glyphs selects what a printable
 * byte becomes: the raw PETSCII byte (real C64 font, indexed by raw byte)
 * or its CP437 lookalike (fallback when that font is not installed).
 */
static size_t stream_translate(struct PetsciiDispatchState *st,
                               const uint8_t *in, size_t in_len,
                               uint8_t *out, size_t out_max, int raw_glyphs) {
    size_t i, len = 0;

    for (i = 0; i < in_len; i++) {
        uint8_t byte = in[i];
        uint8_t screencode;
        int is_control;
        int old_color = st->color, old_reverse = st->reverse;

        petscii_dispatch_byte(st, byte, &screencode, &is_control);

        if (!is_control) {
            uint8_t glyph = raw_glyphs ? byte : (uint8_t)petscii_fallback_cp437(byte);
            len = append(out, out_max, len, &glyph, 1);
            /* A real C64 screen auto-wraps at column 40 with no CR/LF on
             * the wire -- petscii_dispatch_byte() already tracked that
             * wrap internally (cursor_col back to 0), but nothing sends
             * it to the renderer yet. Amiga's wider console won't wrap
             * there on its own, so without this every physical line runs
             * on into the next, collapsing a 25-line screen into a
             * handful of overlapping ones. */
            if (st->cursor_col == 0) {
                const uint8_t crlf[] = {13, 10};
                len = append(out, out_max, len, crlf, 2);
            }
            continue;
        }

        len = append_control_ansi(byte, out, out_max, len);
        if (st->color != old_color || st->reverse != old_reverse)
            len = append_attributes(st, out, out_max, len);
    }

    return len;
}

size_t petscii_stream_to_ansi(struct PetsciiDispatchState *st,
                               const uint8_t *in, size_t in_len,
                               uint8_t *out, size_t out_max) {
    return stream_translate(st, in, in_len, out, out_max, 0);
}

size_t petscii_stream_to_rawglyphs(struct PetsciiDispatchState *st,
                                    const uint8_t *in, size_t in_len,
                                    uint8_t *out, size_t out_max) {
    return stream_translate(st, in, in_len, out, out_max, 1);
}
