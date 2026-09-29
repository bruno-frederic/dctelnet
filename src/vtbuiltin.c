/* --- the built-in renderer's terminal: vtcon parses, retro32-term draws ---
 *
 * Included by DCTelnet.c after term-engine.c, whose drawing it uses: the
 * glyph writer, the cell-aligned fill and scroll, the cursor flip and the
 * covered/RastPort switch stay exactly as they were. What changed is the
 * parser: bytes from the BBS go to vtcon's engine (third_party/vtcon, the
 * pcansi personality: ANSI.SYS / BBS art), which keeps the cell grid and
 * tells this file what to draw:
 *
 * - damage: cells whose content changed, drawn from the grid, one glyph
 *   each through term_glyph() with the cell's resolved pens;
 * - scroll: rows that moved, one term_rect_scroll(), the vacated rows in
 *   the default background (the engine's scroll contract);
 * - reply: DSR / DA answers, sent to the BBS.
 *
 * The engine orders every scroll before the damage that follows it, so a
 * damaged cell is drawn at once from the grid as it is now.
 *
 * term-engine.c's own parser (term_feed) is no longer called. Its cursor
 * position (cur_x, cur_y) is kept equal to the engine's after every write,
 * because the cursor flip and the RastPort glyph runs read it.
 */
#include "vtengine.h"
#include "glyphmap.h"

static vt_term *vtTerm;

/* Draw cells x0 <= x < x1 of rows y0 <= y < y1 from the grid. */
static void VtDraw(void *user, int x0, int y0, int x1, int y1)
{
    int x, y, n;
    for (y = y0; y < y1; y++) {
        const vt_cell *c = vt_row(vtTerm, y, &n);
        if (!c)
            continue;
        cur_y = (WORD)y;
        term_sync_row_base();
        for (x = x0; x < x1 && x < n; x++) {
            vt_u16 fg, bg;
            vt_u32 ch = c[x].ch;
            UBYTE code;
            vt_resolve_colors(vtTerm, &c[x], &fg, &bg);
            term_pen_fg = (UBYTE)(fg & 15);
            term_pen_bg = (UBYTE)(bg & 15);
            atr_under = (c[x].attr & VT_ATTR_UNDERLINE) != 0;
            if (ch >= 0x20 && ch < 0x7F) {
                code = (UBYTE)ch;
            } else {
                /* The font is drawn as the BBS sent it: every byte the
                 * engine took as a CP437 character maps back to itself. */
                vt_glyph g = vt_map_glyph(ch, VT_ENC_CP437);
                code = g.kind == VT_GLYPH_FONT ? g.code : (UBYTE)'?';
            }
            term_glyph((WORD)x, code);
        }
    }
    atr_under = 0;
}

static void VtScroll(void *user, int top, int bottom, int n)
{
    /* pcansi's default background is colour 0 */
    term_rect_scroll(0, (WORD)top, term_cols, (WORD)(bottom - top), 0, (WORD)-n, 0);
}

static void VtReply(void *user, const vt_u8 *buf, long len)
{
    send_data((const UBYTE *)buf, len);
}

/* After the engine has drawn: the cursor where the engine has it. */
static void VtSyncCursor(void)
{
    int x, y;
    vt_cursor(vtTerm, &x, &y);
    cur_x = (WORD)x;
    cur_y = (WORD)y;
    term_sync_row_base();
    cursor_visible = (vt_modes(vtTerm) & VT_MODE_CURSOR_VISIBLE) != 0;
}

/* A new terminal for the grid term_init_area() has just laid out (and
 * cleared). 0 when it is ready. */
static int VtOpen(void)
{
    static const vt_callbacks cb = { VtDraw, VtScroll, VtReply, NULL, NULL, NULL };

    if (vtTerm)
        vt_free(vtTerm);
    vtTerm = vt_new(term_cols, term_rows, 0, &cb, NULL);
    if (!vtTerm)
        return 1;
    vt_set_personality(vtTerm, VT_PCANSI);
    vt_set_charset(vtTerm, VT_CS_CP437);
    VtSyncCursor();
    return 0;
}

static void VtClose(void)
{
    if (vtTerm) {
        vt_free(vtTerm);
        vtTerm = NULL;
    }
}

/* Bytes from the BBS (or DCTelnet's own messages): parse, draw what
 * changed, put the cursor back. Drawing happens with the cursor hidden. */
static void VtWrite(const UBYTE *data, long len)
{
    cursor_hide();
    vt_write(vtTerm, data, len);
    term_flush();
    VtSyncCursor();
    cursor_show();
}
