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
#include "vtpersonality.h"

static vt_term *vtTerm;

/* Draw cells x0 <= x < x1 of rows y0 <= y < y1 from the grid. */
static void VtDraw(void *user, int x0, int y0, int x1, int y1)
{
    int x, y, n;
    /* vt_new() draws its blank screen before it returns the terminal:
     * term_init_area() has cleared the area already. */
    if (!vtTerm)
        return;
    for (y = y0; y < y1; y++) {
        const vt_cell *c = vt_row(vtTerm, y, &n);
        if (!c)
            continue;
        cur_y = (WORD)y;
        term_sync_row_base();
        for (x = x0; x < x1 && x < n; x++) {
            vt_color fg, bg;
            vt_u32 ch = c[x].ch;
            vt_cell cell = c[x];
            UBYTE code;
            /* Blink is never animated (no timer; the owner's decision, MA1-7
             * exemption). With iCE colours on (BLINK_ICE) the engine turns
             * blink on a dark background into the bright background; with
             * them off (BLINK_STEADY) the flag is cleared here, so the cell is
             * drawn as it was sent. */
            if (prefs.Blink == BLINK_STEADY)
                cell.attr &= (vt_attr)~VT_ATTR_BLINK;
            vt_resolve_colors(vtTerm, &cell, &fg, &bg);
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

/* The screen's layers while the engine draws straight into its bitmap.
 * It does only while no layer lies over the terminal (a requester, another
 * DCTelnet window, an open menu): such a layer owns those pixels. Intuition
 * takes the screen's LayerInfo to make a layer, so holding it from the look
 * to the end of the drawing (the blitter's too) keeps a menu or window from
 * opening in between and being drawn over. */
static BOOL vtLayersHeld;

/* Is a layer in front of the terminal window over its inner area? The
 * caller holds the screen's LayerInfo. */
static BOOL VtCovered(void)
{
    struct Layer *l;
    WORD x0 = win->LeftEdge + win->BorderLeft;
    WORD y0 = win->TopEdge + win->BorderTop;
    WORD x1 = win->LeftEdge + win->Width - win->BorderRight - 1;
    WORD y1 = win->TopEdge + win->Height - win->BorderBottom - 1;

    for (l = scr->LayerInfo.top_layer; l && l != win->WLayer; l = l->back)
        if (l->bounds.MinX <= x1 && l->bounds.MaxX >= x0
         && l->bounds.MinY <= y1 && l->bounds.MaxY >= y0)
            return TRUE;
    return FALSE;
}

/* Before drawing. Covered, the engine draws through the window's RastPort,
 * whose layer clips, and the lock is given back at once; otherwise it draws
 * straight into the bitmap and the layers stay locked until
 * VtLayersUnlock(). Without layers.library: the RastPort. */
static void VtLayersLock(void)
{
    BOOL covered = TRUE;

    if (!scr || !win)
        return;
    if (LayersBase) {
        LockLayerInfo(&scr->LayerInfo);
        covered = VtCovered();
    }
    term_covered(covered);
    if (covered && LayersBase)
        UnlockLayerInfo(&scr->LayerInfo);
    else if (!covered)
        vtLayersHeld = TRUE;
}

static void VtLayersUnlock(void)
{
    if (!vtLayersHeld)
        return;
    term_blit_sync();       /* the blitter is done with the planes too */
    vtLayersHeld = FALSE;
    UnlockLayerInfo(&scr->LayerInfo);
}

/* An answer to the BBS, in the middle of a write: sent with the layers
 * unlocked (a send that waits would hold up every window on the screen),
 * looked at again after. */
static void VtReply(void *user, const vt_u8 *buf, long len)
{
    BOOL held = vtLayersHeld;

    VtLayersUnlock();
    send_data((const UBYTE *)buf, len);
    if (held)
        VtLayersLock();
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
    static const vt_callbacks cb = { VtDraw, VtScroll, VtReply, NULL, NULL, NULL, NULL };
    struct VtPersonality pers;

    if (vtTerm)
        vt_free(vtTerm);
    vtTerm = vt_new(term_cols, term_rows, 0, &cb, NULL);
    if (!vtTerm)
        return 1;
    pers = Vt_PersonalityFor(&prefs);
    vt_set_personality(vtTerm, pers.personality);
    vt_set_charset(vtTerm, pers.charset);
    VtSyncCursor();
    return 0;
}

/* Every cell again from the grid, after a preference that changes how the
 * cells look (Terminal > iCE Colours). */
static void VtRedraw(void)
{
    if (!vtTerm)
        return;
    VtLayersLock();
    cursor_hide();
    VtDraw(NULL, 0, 0, term_cols, term_rows);
    term_flush();
    VtSyncCursor();
    cursor_show();
    VtLayersUnlock();
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
    VtLayersLock();
    cursor_hide();
    vt_write(vtTerm, data, len);
    term_flush();
    VtSyncCursor();
    cursor_show();
    VtLayersUnlock();
}
