/* tests/test_term_engine.c -- retro32-term (src/third_party/retro32-term/term-engine.c)
 * on the host, with the graphics.library calls it makes recorded. While
 * something covers the terminal (term_covered) and on a bitmap that is not
 * planar (RTG), it must draw only through the window's RastPort -- never
 * into the bitplanes, which then hold another window's pixels -- and
 * directly into its own area of the bitmap otherwise. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <exec/types.h>
#include "prefs.h"
#include "charset.h"

typedef char *STRPTR;
struct Library { UWORD lib_Version; };
struct GfxBase { struct Library LibNode; };
struct BitMap { UWORD BytesPerRow, Rows; UBYTE Flags, Depth; UWORD pad; UBYTE *Planes[8]; };
struct TextFont { UWORD tf_YSize, tf_XSize, tf_Baseline; UBYTE tf_LoChar, tf_HiChar; void *tf_CharData; UWORD tf_Modulo; void *tf_CharLoc; };
struct RastPort { struct BitMap *BitMap; UBYTE Mask, FgPen, BgPen, DrawMode; WORD cx, cy; struct TextFont *Font; };
struct Rectangle { WORD MinX, MinY, MaxX, MaxY; };
struct Layer { struct Layer *back; struct Rectangle bounds; };
struct Layer_Info { struct Layer *top_layer; };
struct ColorMap { int unused; };
struct ViewPort { struct ColorMap *ColorMap; };
struct Screen { WORD Width, Height; struct ViewPort ViewPort; struct RastPort RastPort; struct Layer_Info LayerInfo; };
struct Window { WORD LeftEdge, TopEdge, Width, Height; BYTE BorderLeft, BorderTop, BorderRight, BorderBottom; struct Layer *WLayer; ULONG IDCMPFlags; ULONG Flags; };
#define JAM1 0
#define JAM2 1
#define COMPLEMENT 2
#define BMA_DEPTH 4
#define BMA_FLAGS 12
#define BMF_STANDARD 8

#define W 640
#define H 256
#define BPR (W / 8)
static UBYTE planes[4][BPR * H];
static struct BitMap bm;
static struct GfxBase gfx = { { 40 } };
static struct GfxBase *GfxBase = &gfx;
static ULONG bitmapFlags = BMF_STANDARD;

/* The RastPort calls, one line each. */
static char calls[8192];
static void call(const char *fmt, long a, long b, long c, long d, long e)
{
    char line[160];
    snprintf(line, sizeof(line), fmt, a, b, c, d, e);
    strcat(calls, line);
}

/* The screen's LayerInfo, as Intuition sees it: the drawing that went into
 * the planes while it was not held, and the planes as they were given back. */
static int layerLocks, bltUnlocked, blitBusy, unlockedBusy;
static UBYTE atUnlock[4][BPR * H];
static void WaitBlit(void) { blitBusy = 0; }
static void LockLayerInfo(struct Layer_Info *li) { (void)li; layerLocks++; }
static void UnlockLayerInfo(struct Layer_Info *li)
{
    (void)li;
    assert(layerLocks > 0);
    layerLocks--;
    unlockedBusy += blitBusy;
    memcpy(atUnlock, planes, sizeof(atUnlock));
}
static ULONG GetBitMapAttr(struct BitMap *b, ULONG attr)
{
    return attr == BMA_FLAGS ? bitmapFlags : attr == BMA_DEPTH ? b->Depth : 0;
}
/* MA2-9: pens from the colour map, recorded. obtainFail makes the next
 * ones fail (-1); the pen handed out is 50 + the count so far, mod 200. */
#define TAG_END 0
#define OBP_Precision 1
#define PRECISION_EXACT 0
#define OBP_FailIfBad 2
static int obtainCalls, releaseCalls, obtainFail;
static ULONG obtainR, obtainG, obtainB;
static LONG obtainTags[4];
static LONG ObtainBestPen(struct ColorMap *cm, ULONG r, ULONG g, ULONG b, ...)
{
    (void)cm;
    obtainR = r; obtainG = g; obtainB = b;
    obtainTags[0] = OBP_Precision; obtainTags[1] = PRECISION_EXACT; obtainTags[2] = OBP_FailIfBad; obtainTags[3] = 1;
    obtainCalls++;
    return obtainFail ? -1 : 50 + (obtainCalls - 1) % 200;
}
static void ReleasePen(struct ColorMap *cm, ULONG pen) { (void)cm; (void)pen; releaseCalls++; }
static void SetFont(struct RastPort *rp, struct TextFont *tf) { rp->Font = tf; }
static void SetAPen(struct RastPort *rp, ULONG pen) { rp->FgPen = (UBYTE)pen; }
static void SetBPen(struct RastPort *rp, ULONG pen) { rp->BgPen = (UBYTE)pen; }
static void SetDrMd(struct RastPort *rp, ULONG mode) { rp->DrawMode = (UBYTE)mode; }
static void Move(struct RastPort *rp, LONG x, LONG y) { rp->cx = (WORD)x; rp->cy = (WORD)y; }
static void Text(struct RastPort *rp, STRPTR s, ULONG n)
{
    char line[160];
    snprintf(line, sizeof(line), "Text(%d,%d fg%d bg%d \"%.*s\")\n", rp->cx, rp->cy, rp->FgPen, rp->BgPen, (int)n, s);
    strcat(calls, line);
}
static void RectFill(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1)
{
    if (rp->DrawMode == COMPLEMENT)
        call("Complement(%ld,%ld,%ld,%ld mask%ld)\n", (long)x0, (long)y0, (long)x1, (long)y1, (long)rp->Mask);
    else
        call("RectFill(%ld,%ld,%ld,%ld pen%ld)\n", (long)x0, (long)y0, (long)x1, (long)y1, (long)rp->FgPen);
}
static void ScrollRaster(struct RastPort *rp, LONG dx, LONG dy, LONG x0, LONG y0, LONG x1, LONG y1)
{
    char line[160];
    snprintf(line, sizeof(line), "ScrollRaster(%ld,%ld %ld,%ld,%ld,%ld bg%d)\n", (long)dx, (long)dy, (long)x0, (long)y0, (long)x1, (long)y1, rp->BgPen);
    strcat(calls, line);
}

/* The engine's blits: minterm 0xC0 copies, 0xFF sets, 0x00 clears, in the
 * planes of mask; x and w are multiples of 8. Overlaps go through a copy. */
static LONG BltBitMap(struct BitMap *s, WORD sx, WORD sy, struct BitMap *d, WORD dx, WORD dy,
                      WORD w, WORD h, UBYTE minterm, UBYTE mask, UBYTE *tmp)
{
    static UBYTE buf[BPR * H];
    WORD p, y;

    (void)tmp;
    blitBusy = 1;
    if (!layerLocks)
        bltUnlocked++;
    for (p = 0; p < 4; p++) {
        if (!(mask & (1 << p)))
            continue;
        if (minterm == 0xC0) {
            for (y = 0; y < h; y++)
                memcpy(buf + y * BPR, s->Planes[p] + (sy + y) * BPR + sx / 8, w / 8);
            for (y = 0; y < h; y++)
                memcpy(d->Planes[p] + (dy + y) * BPR + dx / 8, buf + y * BPR, w / 8);
        } else {
            for (y = 0; y < h; y++)
                memset(d->Planes[p] + (dy + y) * BPR + dx / 8, minterm == 0xFF ? 0xFF : 0x00, w / 8);
        }
    }
    return 4;
}

/* An answer to the BBS. A menu opens meanwhile when menuOnSend is set. */
static struct Layer winLayer, menuLayer;
static struct Screen screen;
static int sentLocked, menuOnSend;
static UBYTE sentBuf[256];
static int sentLen, sentCalls;
static void send_data_(const UBYTE *d, LONG n)
{
    if (sentLen + n <= (int)sizeof(sentBuf))
        memcpy(sentBuf + sentLen, d, (size_t)n);
    sentLen += (int)n;
    sentCalls++;
    sentLocked += layerLocks;
    if (menuOnSend)
        screen.LayerInfo.top_layer = &menuLayer;
}
#define send_data(d, n) send_data_((d), (n))
#include "../src/third_party/retro32-term/term-engine.c"

/* DCTelnet's side of vtbuiltin.c: its screen, the terminal window and
 * layers.library. */
static struct Library layers = { 40 };
static struct Library *LayersBase = &layers;
static struct Screen *scr = &screen;
static struct Window window, *win = &window;
/* MA3: the window title and IDCMP calls, recorded. */
#define IDCMP_MOUSEMOVE 0x10UL
#define IDCMP_RAWKEY 0x400UL
static char MainWindowTitle[] = "DCTelnet own title";
#define WFLG_BACKDROP 0x100UL
static char titleSet[256];
static const UBYTE *screenTitleSet;     /* (UBYTE *)-1 when the screen title stays */
static int titleCalls;
static int ledCalls;
static void SetWindowTitles(struct Window *w, UBYTE *t, UBYTE *s)
{
    (void)w;
    strcpy(titleSet, (const char *)t);
    screenTitleSet = s;
    titleCalls++;
}
static void LEDs(void) { ledCalls++; }
static void ModifyIDCMP(struct Window *w, ULONG flags) { w->IDCMPFlags = flags; }
/* The built-in renderer reads the user's preferences. */
struct PrefsStruct prefs;

/* The sentinel for what VtDraw hands the glyph writer: every call is kept
 * (the column, the row, the code, the resolved pens), then drawn for real. */
#define PROBE_MAX 20000
static struct { WORD x, y; UBYTE code, fg, bg; } probe[PROBE_MAX];
static int probes;
static void probe_glyph(WORD x, UBYTE ch)
{
    if (probes < PROBE_MAX) {
        probe[probes].x = x;
        probe[probes].y = cur_y;
        probe[probes].code = ch;
        probe[probes].fg = term_pen_fg;
        probe[probes].bg = term_pen_bg;
    }
    probes++;
    term_glyph(x, ch);
}
#define term_glyph(x, c) probe_glyph((x), (c))
#include "vtengine.h"
/* The sentinel for VtOpen: what the global vtTerm is while vt_new() draws. */
static int newCalls;
static int newSawTerm;
static int newSb;                   /* the scrollback line count VtOpen handed vt_new */
static vt_term *probe_new(int cols, int rows, int sb, const vt_callbacks *cb, void *user);
#define vt_new(c, r, sb, cb, u) probe_new((c), (r), (sb), (cb), (u))
#include "../src/vtbuiltin.c"
#undef term_glyph
#undef vt_new
static vt_term *probe_new(int cols, int rows, int sb, const vt_callbacks *cb, void *user)
{
    newCalls++;
    newSb = sb;
    newSawTerm = vtTerm != NULL;    /* vt_new() draws through VtDraw() right here */
    return vt_new(cols, rows, sb, cb, user);
}

/* A font whose glyphs all differ: row r of character c is c ^ (r * 37). */
static UBYTE fontData[8][256];
static ULONG fontLoc[256];
static struct TextFont font = { 8, 8, 6, 0, 255, fontData, 256, fontLoc };
/* The C64's size: 16x8 cells. Row r of character c is c << 8 | (c ^ r).
 * font_extract() reads 3 bytes at each glyph's byte offset, so the last glyph
 * of the last row reads 2 bytes past the 512 of a row: one slack row. */
static UBYTE wideData[9][512];
static ULONG wideLoc[256];
static struct TextFont wide = { 8, 16, 6, 0, 255, wideData, 512, wideLoc };
static struct RastPort winRp;

/* The terminal of a 640x245 window at y = 11, under a title bar: 30 rows. */
#define TOP 11
static void setup(void)
{
    WORD c, r, p;

    for (c = 0; c < 256; c++) {
        fontLoc[c] = (ULONG)(c * 8) << 16 | 8;
        wideLoc[c] = (ULONG)(c * 16) << 16 | 16;
        for (r = 0; r < 8; r++) {
            fontData[r][c] = (UBYTE)(c ^ (r * 37));
            wideData[r][2 * c] = (UBYTE)c;
            wideData[r][2 * c + 1] = (UBYTE)(c ^ r);
        }
    }
    bm.BytesPerRow = BPR; bm.Rows = H; bm.Depth = 4;
    for (p = 0; p < 4; p++) bm.Planes[p] = planes[p];
    memset(planes, 0x55, sizeof(planes));           /* "the title bar and other windows" */
    memset(&winRp, 0, sizeof(winRp));
    winRp.Mask = 0xFF;
    screen.Width = W; screen.Height = H;
    screen.RastPort.BitMap = &bm; screen.RastPort.Mask = 0xFF;
    /* the terminal window, the screen's only window: nothing over it */
    window.LeftEdge = 0; window.TopEdge = 0; window.Width = W; window.Height = H;
    window.BorderTop = TOP;
    window.WLayer = &winLayer;
    winLayer.bounds.MaxX = W - 1; winLayer.bounds.MaxY = H - 1;
    screen.LayerInfo.top_layer = &winLayer;
    /* a menu strip's layer over the terminal's top rows */
    menuLayer.back = &winLayer;
    menuLayer.bounds.MinX = 0; menuLayer.bounds.MinY = 0;
    menuLayer.bounds.MaxX = 199; menuLayer.bounds.MaxY = 40;
    layerLocks = bltUnlocked = blitBusy = unlockedBusy = sentLocked = menuOnSend = 0;
    assert(term_init_area(&screen, &winRp, 0, TOP, 0, TOP, W, H - TOP, &font) == 0);
    calls[0] = 0;
}

static void feed(const char *s)
{
    while (*s)
        term_feed((UBYTE)*s++);
    term_flush();
}

static void test_the_terminal_starts_below_the_title_bar_and_matches_the_window_rows(void)
{
    WORD p;

    setup();
    assert(term_rows == 30);                        /* 245 / 8, what DCTelnet tells the BBS */
    for (p = 0; p < 4; p++) {
        assert(planes[p][(TOP - 1) * BPR] == 0x55);  /* the bar is not drawn over */
        assert(planes[p][TOP * BPR] == 0x00);        /* the terminal is cleared from there */
    }
    feed("A");
    assert(planes[0][TOP * BPR] == 'A' && planes[0][(TOP + 1) * BPR] == ('A' ^ 37));
    assert(calls[0] == 0);                          /* direct: no RastPort call */
}

static void test_a_covered_terminal_draws_only_through_the_rastport(void)
{
    static UBYTE before[sizeof(planes)];
    int i;

    setup();
    term_covered(1);
    memcpy(before, planes, sizeof(planes));
    feed("Hello \x1b[31mred\x1b[0m");
    assert(memcmp(before, planes, sizeof(planes)) == 0);   /* (checked here too: later scrolls */
    for (i = 0; i < 30; i++)                                /*  would move a direct draw out)  */
        feed("\r\n");                               /* to the bottom and one line further */
    cursor_show();
    feed("\x1b[5;10H\x1b[2L\x1b[3P");
    assert(memcmp(before, planes, sizeof(planes)) == 0);   /* another window's pixels are untouched */
    assert(strstr(calls, "Text(0,6 fg7 bg0 \"Hello \")"));  /* runs, at the window's coordinates */
    assert(strstr(calls, "Text(48,6 fg1 bg0 \"red\")"));
    assert(strstr(calls, "ScrollRaster(0,8 0,0,639,239 bg0)\nRectFill(0,232,639,239 pen0)"));
                                  /* the line feed at the bottom, and the new row cleared (a
                                     backfill hook, not BgPen, would fill it otherwise) */
    assert(strstr(calls, "Complement(0,232,7,239 mask15)"));        /* the cursor: pen XOR 15 */
    assert(strstr(calls, "ScrollRaster(0,-16 0,32,639,239 bg0)"));   /* insert 2 lines at row 5 */
    assert(strstr(calls, "ScrollRaster(24,0 72,32,639,39 bg0)"));    /* delete 3 characters */
    assert(winRp.Mask == 0xFF);                     /* the mask is given back */
}

static void test_uncovered_again_it_draws_directly(void)
{
    setup();
    term_covered(1);
    feed("x");
    term_covered(0);
    calls[0] = 0;
    feed("\rA");
    assert(calls[0] == 0 && planes[0][TOP * BPR] == 'A');
}

static void test_an_rtg_bitmap_always_takes_the_rastport(void)
{
    static UBYTE before[sizeof(planes)];

    bitmapFlags = 0;                                /* not BMF_STANDARD: chunky */
    setup();
    term_covered(0);
    memcpy(before, planes, sizeof(planes));
    feed("rtg");
    assert(memcmp(before, planes, sizeof(planes)) == 0);
    assert(strstr(calls, "Text(0,6 fg7 bg0 \"rtg\")"));
    bitmapFlags = BMF_STANDARD;
    assert(term_init_area(&screen, NULL, 0, 0, 0, 0, W, H, &font) == 0);     /* planar, direct only */
    bitmapFlags = 0;
    assert(term_init_area(&screen, NULL, 0, 0, 0, 0, W, H, &font) != 0);     /* RTG needs a RastPort */
    bitmapFlags = BMF_STANDARD;
}

static void test_the_pen_map_colours_the_rastport_path_and_stops_direct_drawing(void)
{
    static const UBYTE pens[16] = { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55 };

    static UBYTE before[sizeof(planes)];

    setup();
    term_set_pens(pens);            /* the Workbench's shared pens: never written as numbers */
    term_covered(0);
    memcpy(before, planes, sizeof(planes));
    feed("\x1b[1;33mY");
    assert(memcmp(before, planes, sizeof(planes)) == 0);
    assert(strstr(calls, "Text(0,6 fg51 bg40 \"Y\")"));    /* bright yellow = ANSI 11 */
    term_set_pens(NULL);
    term_covered(0);
    calls[0] = 0;
    feed("\rZ");
    assert(calls[0] == 0 && planes[0][TOP * BPR] != before[TOP * BPR]);  /* pens 0-15: direct again */
}

/* 8 bitplanes (AGA): direct drawing writes planes 0-3 only and the upper
 * planes' bits showed through (a grey background), so the RastPort draws. */
static void test_an_8_plane_screen_takes_the_rastport(void)
{
    static UBYTE before[sizeof(planes)];

    setup();
    bm.Depth = 8;
    assert(term_init_area(&screen, &winRp, 0, TOP, 0, TOP, W, H - TOP, &font) == 0);
    calls[0] = 0;
    memcpy(before, planes, sizeof(planes));
    feed("aga");
    assert(memcmp(before, planes, sizeof(planes)) == 0);
    assert(strstr(calls, "Text(0,6 fg7 bg0 \"aga\")"));
    bm.Depth = 4;
}

/* The C64 fonts (Petscii.font) have 16x8 cells: 40 columns in 640 pixels,
 * two bytes per plane row, the wrap at column 40, a cursor two bytes wide. */
static void test_a_16_pixel_font_gives_40_columns(void)
{
    setup();
    assert(term_init_area(&screen, &winRp, 0, TOP, 0, TOP, W, H - TOP, &wide) == 0);
    assert(term_cols == 40);
    feed("\x41");
    assert(planes[0][TOP * BPR] == 0x41 && planes[0][TOP * BPR + 1] == 0x41);    /* row 0: c, c ^ 0 */
    assert(planes[0][(TOP + 3) * BPR + 1] == (0x41 ^ 3));
    assert(planes[1][TOP * BPR] == 0x41);                                         /* pen 7: planes 0-2 */
    assert(planes[3][TOP * BPR] == 0x00);
    {
        int i;
        for (i = 0; i < 39; i++) feed("B");
        feed("C");                                  /* column 41: the next row */
        assert(planes[0][(TOP + 8) * BPR] == 'C');
    }
    cursor_show();
    assert(planes[0][(TOP + 8) * BPR + 2] == 0xFF && planes[0][(TOP + 8) * BPR + 3] == 0xFF);
    cursor_hide();
    term_set_font(&wide);   /* the other C64 charset: new glyphs from here, the screen stays */
    assert(planes[0][TOP * BPR] == 0x41 && term_cols == 40);
    assert(calls[0] == 0);
}

/* vt_new() draws its blank screen through VtDraw() before it returns: on a
 * second VtOpen the global vtTerm must not still be the previous engine (freed
 * in the original bug), so VtDraw finds no terminal and reads no row. */
static void test_vtopen_twice_never_draws_through_the_previous_engine(void)
{
    setup();
    assert(term_init_area(&screen, &winRp, 0, TOP, 0, TOP, W, H - TOP, &font) == 0);
    assert(VtOpen() == 0);
    assert(vtTerm != NULL);
    newCalls = 0;
    assert(VtOpen() == 0);
    assert(newCalls == 1 && newSawTerm == 0);   /* the previous engine was closed first */
    assert(vtTerm != NULL);
    VtClose();
    assert(vtTerm == NULL);
}

static void vt_write_str(const char *s)
{
    VtWrite((const UBYTE *)s, (long)strlen(s));
}

/* Upstream todos: a menu opened between the look at the layers and the
 * drawing was drawn over. Intuition makes a menu's layer under the screen's
 * LayerInfo: while the terminal draws straight into the bitmap, the lock is
 * held from the look to the end of the drawing, the blitter's too. */
static void test_a_menu_cannot_open_while_the_terminal_draws_into_the_bitmap(void)
{
    int i;

    setup();
    assert(VtOpen() == 0);
    bltUnlocked = 0;                            /* (term_init_area's clear) */
    for (i = 0; i < 32; i++)
        vt_write_str("line\r\n");             /* scrolls: blits */
    assert(calls[0] == 0);                      /* drawn straight into the bitmap */
    assert(bltUnlocked == 0);                   /* every blit with the layers held */
    assert(unlockedBusy == 0);                  /* and done before they were given back */
    assert(memcmp(atUnlock, planes, sizeof(planes)) == 0);    /* nothing drawn after */
    assert(planes[0][(TOP + 29 * 8) * BPR + 1] == 0);         /* (the new bottom row, */
    assert(planes[0][(TOP + 29 * 8) * BPR] == 0xFF);          /*  the cursor in it)   */
    assert(planes[0][(TOP + 28 * 8) * BPR] == 'l');
    assert(layerLocks == 0);
    vt_write_str("\x1b[?25l\n");     /* a scroll and no cursor: the write ends on a blit */
    assert(bltUnlocked == 0 && unlockedBusy == 0);
    VtClose();
}

static void test_a_menu_open_over_the_terminal_is_drawn_around(void)
{
    static UBYTE before[sizeof(planes)];

    setup();
    assert(VtOpen() == 0);
    screen.LayerInfo.top_layer = &menuLayer;
    memcpy(before, planes, sizeof(planes));
    vt_write_str("Hello");
    assert(memcmp(before, planes, sizeof(planes)) == 0);
    assert(strstr(calls, "\"Hello\""));
    assert(layerLocks == 0);                    /* the RastPort's layer clips: not held */
    screen.LayerInfo.top_layer = &winLayer;     /* the menu closes */
    calls[0] = 0;
    vt_write_str("\rA");
    assert(calls[0] == 0 && planes[0][TOP * BPR] == 'A');
    VtClose();
}

/* A send can wait for the network, and every window on the screen with it:
 * the layers are given back for it, and looked at again after. */
static void test_a_menu_opened_while_an_answer_is_sent_is_drawn_around(void)
{
    static UBYTE before[sizeof(planes)];

    setup();
    assert(VtOpen() == 0);
    menuOnSend = 1;
    memcpy(before, planes, sizeof(planes));
    vt_write_str("A\x1b[6nB");                  /* the BBS asks where the cursor is */
    assert(sentLocked == 0);
    assert(memcmp(before, planes, sizeof(planes)) == 0);
    assert(strstr(calls, "\"AB\""));
    assert(layerLocks == 0);
    VtClose();
}


/* --- MA1: the built-in renderer as a PC ANSI terminal for BBS art --- */

/* A fresh terminal, the probe empty, the preferences at their defaults. */
static void vt_begin(void)
{
    setup();
    memset(&prefs, 0, sizeof(prefs));
    assert(VtOpen() == 0);
    probes = 0;
}

static void vt_put(const char *s)
{
    VtWrite((const UBYTE *)s, (long)strlen(s));
}

static void probe_codes(char *out, size_t max)
{
    int i;
    size_t n = 0;

    for (i = 0; i < probes && i < PROBE_MAX && n + 8 < max; i++)
        n += (size_t)snprintf(out + n, max - n, "%02X ", probe[i].code);
    out[n] = 0;
}

static void test_ma1_2_line_art_lands_in_the_grid_and_reaches_the_glyph_writer(void)
{
    char codes[256];
    int n;
    const vt_cell *row;

    vt_begin();
    vt_put("\xC9\xCD\xBB\r\n\xBA\r\n\xC8\xCD\xBC");
    probe_codes(codes, sizeof(codes));
    assert(strcmp(codes, "C9 CD BB BA C8 CD BC ") == 0);
    assert(probe[0].x == 0 && probe[0].y == 0 && probe[1].x == 1 && probe[2].x == 2);
    assert(probe[3].x == 0 && probe[3].y == 1);
    assert(probe[4].x == 0 && probe[4].y == 2 && probe[6].x == 2);
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].ch == 0x2554 && row[1].ch == 0x2550 && row[2].ch == 0x2557);
    VtClose();
}

static void test_ma1_3_blocks_and_shades_map_to_themselves_never_to_the_fallback(void)
{
    static const UBYTE want[] = { 0xDB, 0xB0, 0xB1, 0xB2, 0xDC, 0xDF };
    int i;

    vt_begin();
    vt_put("\xDB\xB0\xB1\xB2\xDC\xDF");
    assert(probes == 6);
    for (i = 0; i < 6; i++)
        assert(probe[i].code == want[i] && probe[i].code != '?');
    VtClose();
}

static void test_ma1_1_latin1_byte_reaches_the_glyph_writer_unchanged(void)
{
    vt_begin();
    vt_put("\xE9");
    assert(probes == 1 && probe[0].code == 0xE9);
    VtClose();
}

static void test_ma1_4_sixteen_colours(void)
{
    vt_begin();
    vt_put("\x1b[0;31mA");
    assert(probes == 1 && probe[0].fg == 1);
    probes = 0;
    vt_put("\x1b[1;31mA");
    assert(probes == 1 && probe[0].fg == 9);
    probes = 0;
    vt_put("\x1b[0m\x1b[1;33;44mA");
    assert(probes == 1 && probe[0].fg == 11 && probe[0].bg == 4);   /* bright yellow: ANSI order, red is 1 and 9 */
    VtClose();
}

static void expect_cursor(int x, int y);

static void test_ma1_5_6_7_ice_colours_decide_what_blink_draws_as(void)
{
    const vt_cell *row;
    int n;

    vt_begin();                                  /* default: iCE colours */
    assert(prefs.Blink == BLINK_ICE);
    vt_put("\x1b[5;44mA");
    assert(probes == 1 && probe[0].bg == 12);
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].attr & VT_ATTR_BLINK);        /* the grid keeps what was sent */
    VtClose();

    vt_begin();
    prefs.Blink = BLINK_STEADY;                  /* iCE colours off: drawn as sent */
    vt_put("\x1b[5;44mA");
    assert(probes == 1 && probe[0].bg == 4);
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].attr & VT_ATTR_BLINK);

    /* The menu toggle: the cells already on the screen are drawn again. */
    prefs.Blink = BLINK_ICE;
    probes = 0;
    VtRedraw();
    assert(probes == term_cols * term_rows && probe[0].code == 'A' && probe[0].bg == 12);
    prefs.Blink = BLINK_STEADY;
    probes = 0;
    VtRedraw();
    assert(probe[0].bg == 4);
    /* The redraw leaves the cursor where the engine has it, not on the
     * last row it drew. */
    expect_cursor(1, 0);
    VtClose();
}

static void expect_cursor(int x, int y)
{
    int cx, cy;

    vt_cursor(vtTerm, &cx, &cy);
    assert(cx == x && cy == y);
    assert(cur_x == x && cur_y == y);           /* VtSyncCursor put them there */
}

static void test_ma1_8_cursor_save_restore_position_and_erase(void)
{
    vt_begin();
    vt_put("\x1b[5;10H");
    expect_cursor(9, 4);
    vt_put("\x1b[s\x1b[1;1H");
    expect_cursor(0, 0);
    vt_put("\x1b[u");
    expect_cursor(9, 4);
    vt_put("XYZ\x1b[K");                         /* erase to the end of the line */
    expect_cursor(12, 4);
    vt_put("\x1b[2J");                           /* ANSI.SYS: clear and home */
    expect_cursor(0, 0);
    VtClose();
}

static void test_ma1_init_strings_draw_nothing_and_leave_the_cursor(void)
{
    vt_begin();
    vt_put("\x1b[20l");
    vt_put("\x1b[0 p");
    assert(probes == 0);
    expect_cursor(0, 0);
    VtClose();
}

/* MA1-9: a real BBS ANSI art file (tests/ansi_art/README names its source),
 * fed the way ConWrite feeds the engine: 1024 bytes at a time. */
static unsigned long crc32_update(unsigned long crc, const UBYTE *p, size_t n)
{
    int k;

    while (n--) {
        crc ^= *p++;
        for (k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
    }
    return crc;
}

#define ART_GRID_CRC 0xD5F60475UL

static void test_ma1_9_the_sample_art_file_gives_the_known_grid(void)
{
    static UBYTE art[8192];
    FILE *f = fopen("ansi_art/up_rough_banner.ans", "rb");
    size_t len, off, end;
    unsigned long crc = 0xFFFFFFFFUL;
    int x, y, n;

    assert(f);
    len = fread(art, 1, sizeof(art), f);
    fclose(f);
    for (end = 0; end < len && art[end] != 0x1A; end++)     /* the SAUCE record follows ^Z */
        ;
    assert(end > 3000);
    vt_begin();
    for (off = 0; off < end; off += 1024)
        VtWrite(art + off, (long)(end - off < 1024 ? end - off : 1024));
    assert(probes > 0);
    for (y = 0; y < vt_rows(vtTerm); y++) {
        const vt_cell *row = vt_row(vtTerm, y, &n);

        for (x = 0; x < n; x++) {
            UBYTE b[5];
            b[0] = (UBYTE)(row[x].ch & 255); b[1] = (UBYTE)(row[x].ch >> 8);
            b[2] = (UBYTE)row[x].fg; b[3] = (UBYTE)row[x].bg; b[4] = (UBYTE)row[x].attr;
            crc = crc32_update(crc, b, sizeof(b));
        }
    }
    crc ^= 0xFFFFFFFFUL;
    assert(crc == ART_GRID_CRC);
    VtClose();
}

/* --- Font probe: UTF-8 box drawing and accents in the font's own encoding --- */

/* A terminal whose font has glyph 0xC4 as the A-diaeresis of Latin-1
 * (ibm = 0) or as the IBM horizontal line (ibm = 1), opened through VtOpen
 * with the given terminal type and character set. */
static void probe_begin(int ibm, UBYTE termType, UBYTE charset)
{
    static const UBYTE a_diaeresis[8] = { 0x66, 0x00, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x00 };
    int r;

    setup();
    for (r = 0; r < 8; r++)
        fontData[r][0xC4] = ibm ? (UBYTE)(r == 4 ? 0xFF : 0x00) : a_diaeresis[r];
    assert(term_init_area(&screen, &winRp, 0, TOP, 0, TOP, W, H - TOP, &font) == 0);
    memset(&prefs, 0, sizeof(prefs));
    prefs.State = APP_RENDERER_BUILTIN;
    prefs.TermType = termType;
    prefs.Charset = charset;
    assert(VtOpen() == 0);
    probes = 0;
    calls[0] = 0;
}

/* TRUE when row r of the first cell holds the byte b in the foreground pen 7. */
static int cell_row_is(int r, UBYTE b)
{
    return planes[0][(TOP + r) * BPR] == b && planes[1][(TOP + r) * BPR] == b &&
           planes[2][(TOP + r) * BPR] == b && planes[3][(TOP + r) * BPR] == 0;
}

/* The font decides what a decoded code point looks like. Through VtWrite:
 * a Latin-1 font gets the box line drawn, not its A-diaeresis at 0xC4; the
 * CP437 byte stream is still passed through untouched; an IBM font gets the
 * accent at its CP437 code. The rasteriser, the encoding rule and the probe
 * have their own tests; this one proves they are wired into the draw path. */
static void test_font_probe_reaches_the_grid_through_vtwrite(void)
{
    static const UBYTE a_diaeresis[8] = { 0x66, 0x00, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x00 };
    int r;

    probe_begin(0, TERMTYPE_UNIX, CHARSET_UTF8);
    vt_put("\xE2\x94\x80");                             /* U+2500 */
    for (r = 0; r < 8; r++)
        assert(cell_row_is(r, r == 4 ? 0xFF : 0x00));    /* the drawn line, not A-diaeresis */
    VtClose();

    probe_begin(0, TERMTYPE_BBS, CHARSET_CP437);
    vt_put("\xC4");                                     /* the BBS byte stays the font's glyph */
    for (r = 0; r < 8; r++)
        assert(cell_row_is(r, a_diaeresis[r]));
    VtClose();

    probe_begin(1, TERMTYPE_UNIX, CHARSET_UTF8);
    vt_put("\xC3\xA9");                                 /* U+00E9 e-acute: CP437 0x82 */
    for (r = 0; r < 8; r++)
        assert(cell_row_is(r, (UBYTE)(0x82 ^ (r * 37))));
    VtClose();
}

/* The covered terminal has no bitmap to write to: the drawn line goes out as
 * RectFill runs (the cell background, then one fg run on row 4). */
static void test_a_drawn_cell_on_a_covered_terminal_goes_out_as_rectfills(void)
{
    static UBYTE before[sizeof(planes)];

    probe_begin(0, TERMTYPE_UNIX, CHARSET_UTF8);
    screen.LayerInfo.top_layer = &menuLayer;        /* a menu strip over the terminal */
    memcpy(before, planes, sizeof(planes));
    calls[0] = 0;
    vt_put("\xE2\x94\x80");
    assert(memcmp(before, planes, sizeof(planes)) == 0);
    assert(strstr(calls, "RectFill(0,0,7,7 pen0)\nRectFill(0,4,7,4 pen7)"));
    VtClose();
}

/* --- MA2: the xterm personality, UTF-8 and the 256-colour / 24-bit pens --- */

/* A fresh Unix terminal: xterm, UTF-8 characters, the default sixteen pens. */
static void xterm_begin_on(UBYTE depth, ULONG flags)
{
    static const UWORD ansi[16] = {
        0x000, 0xA00, 0x0A0, 0xA50, 0x00A, 0xA0A, 0x0AA, 0xAAA,
        0x555, 0xF55, 0x5F5, 0xFF5, 0x55F, 0xF5F, 0x5FF, 0xFFF
    };

    setup();
    bm.Depth = depth;
    bitmapFlags = flags;
    assert(term_init_area(&screen, &winRp, 0, TOP, 0, TOP, W, H - TOP, &font) == 0);
    obtainCalls = releaseCalls = obtainFail = 0;
    memset(&prefs, 0, sizeof(prefs));
    prefs.State = APP_RENDERER_BUILTIN;
    prefs.TermType = TERMTYPE_UNIX;
    prefs.Charset = CHARSET_UTF8;
    memcpy(prefs.AnsiColors, ansi, sizeof(ansi));
    assert(VtOpen() == 0);
    /* The MA2 tests below assert the IBM-font mapping (UTF-8 box and blocks
     * reach term_glyph as CP437 codes); this test font's 0xC4 is not a line,
     * so the probe would say Latin-1. Force the IBM font here; the probe
     * itself is covered by the font-probe reachability test. */
    vtFontEnc = VT_ENC_CP437;
    VtApplyPersonality();
    probes = 0;
    calls[0] = 0;
}

static void xterm_begin(void)
{
    xterm_begin_on(4, BMF_STANDARD);
}

/* MA2-5 / MA2-11: the engine reads the UTF-8 bytes itself, also when a
 * sequence is split between two packets; the glyph writer gets the CP437
 * code of the code point. */
static void test_ma2_5_11_utf8_reaches_the_grid_whole_even_split_across_writes(void)
{
    int n;
    const vt_cell *row;

    xterm_begin();
    assert(Vt_EngineDecodesUtf8(&prefs));
    vt_put("\xE2\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].ch == 0x2588 && row[1].ch == ' ');
    assert(probes == 1 && probe[0].code == 0xDB);
    VtClose();

    xterm_begin();
    vt_put("\xE2");
    assert(probes == 0);                        /* half a character draws nothing */
    vt_put("\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].ch == 0x2588 && row[1].ch == ' ');
    assert(probes == 1 && probe[0].code == 0xDB && probe[0].x == 0);
    VtClose();
}

/* MA2-14 through VtWrite: Unix + IBM PC site; the first valid UTF-8 sequence
 * switches the engine for the rest of the connection, prefs untouched. */
static void unix_cp437_begin(void)
{
    VtSniffReset();
    xterm_begin();
    prefs.Charset = CHARSET_CP437;
    VtApplyPersonality();
}

static void test_ma2_14_stream_switch_to_utf8_through_vtwrite(void)
{
    int n;
    const vt_cell *row;

    unix_cp437_begin();
    vt_put("abc");
    vt_put("\xE2\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[3].ch == 0x2588 && row[4].ch == ' ');       /* full block, one cell */
    vt_put("\xE2\x96\x88");                               /* stays UTF-8 afterwards */
    row = vt_row(vtTerm, 0, &n);
    assert(row[4].ch == 0x2588);
    assert(prefs.Charset == CHARSET_CP437);                /* never saved */
    VtClose();

    unix_cp437_begin();                                    /* art first: CP437 stays */
    vt_put("\xC9\xCD\xBB");
    vt_put("\xE2\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].ch == 0x2554 && row[3].ch != 0x2588);
    VtClose();

    unix_cp437_begin();                                    /* Latin-1 text: never */
    vt_put("\xE9t\xE9");
    vt_put("\xC3\xA9");
    row = vt_row(vtTerm, 0, &n);
    assert(row[3].ch != 0xE9 || row[4].ch != ' ');
    VtClose();

    unix_cp437_begin();                                    /* new connection re-arms */
    vt_put("a\xE9\xE9");                                   /* E9 E9: invalid, decided */
    VtSniffReset();
    vt_put("\xE2\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[3].ch == 0x2588);
    VtClose();
}

/* MA2-14: a valid sequence split across two writes is not drawn as CP437 first;
 * an invalid one still shows both bytes. */
static void test_ma2_14_split_sequence_held_back_through_vtwrite(void)
{
    int n;
    const vt_cell *row;

    unix_cp437_begin();
    vt_put("abc\xE2");
    row = vt_row(vtTerm, 0, &n);
    assert(row[3].ch == ' ');                              /* lead byte not drawn yet */
    vt_put("\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[3].ch == 0x2588 && row[4].ch == ' ');
    VtClose();

    unix_cp437_begin();
    vt_put("abc\xE2");
    vt_put("A");                                           /* invalid: both bytes CP437 */
    row = vt_row(vtTerm, 0, &n);
    assert(row[3].ch == 0x0393 && row[4].ch == 'A' && row[5].ch == ' ');
    VtClose();

    unix_cp437_begin();                                    /* reset drops the held byte */
    vt_put("\xE2");
    VtSniffReset();
    vt_put("\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].ch != 0x2588);
    VtClose();
}

/* MA2-14: a held prefix lives only within one receive burst. The loop found no
 * more data (VtFlushHeld): a lone Latin-1/CP437 byte is drawn at once, and the
 * sniffer stays undecided; a split full block with no flush in between is whole. */
static void test_ma2_14_flush_draws_held_byte_split_block_still_whole(void)
{
    int n;
    const vt_cell *row;

    unix_cp437_begin();
    vt_put("a\xE9");
    VtFlushHeld();
    row = vt_row(vtTerm, 0, &n);
    assert(row[1].ch == 0x0398 && row[2].ch == ' ');       /* CP437 theta, column 1 */
    vt_put("\xE2\x96\x88");                               /* undecided: a valid one still switches */
    row = vt_row(vtTerm, 0, &n);
    assert(row[2].ch == 0x2588);
    VtClose();

    unix_cp437_begin();
    vt_put("a\xE2");
    vt_put("\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[1].ch == 0x2588 && row[2].ch == ' ');
    VtClose();
}

/* MA2-14: a menu change or settings apply must not undo the switch. */
static void test_ma2_14_apply_personality_keeps_the_switch(void)
{
    int n;
    const vt_cell *row;

    unix_cp437_begin();
    vt_put("\xC3\xA9");                                   /* fires */
    VtApplyPersonality();
    vt_put("\xE2\x96\x88");
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].ch == 0x2588 && row[1].ch == ' ');     /* the apply reset the screen */
    prefs.Charset = CHARSET_LATIN1;                        /* the user's change wins */
    VtApplyPersonality();
    vt_put("\xE9");
    row = vt_row(vtTerm, 0, &n);
    assert(row[0].ch == 0xE9);
    VtClose();
}

/* MA2-6: what the IBM font has is drawn, the rest is one '?' per cell, a wide
 * character fills its second cell with a blank. */
static void test_ma2_6_glyph_map_and_fallback(void)
{
    xterm_begin();
    vt_put("\xE2\x94\x80");                    /* U+2500 */
    assert(probes == 1 && probe[0].code == 0xC4);
    probes = 0;
    vt_put("\xC3\xA9");                         /* U+00E9 */
    assert(probes == 1 && probe[0].code == 0x82);
    probes = 0;
    vt_put("\xE2\x98\x83");                    /* U+2603: no glyph in the font */
    assert(probes == 1 && probe[0].code == '?');
    probes = 0;
    vt_put("\xE4\xB8\xAD");                    /* U+4E2D: two cells wide */
    assert(probes == 2);
    assert(probe[0].code == '?' && probe[0].x == 3);
    assert(probe[1].code == ' ' && probe[1].x == 4);
    VtClose();
}

/* MA2-8: 256-colour and 24-bit red reach the glyph writer as the same pen,
 * the nearest of the sixteen. */
static void test_ma2_8_256_colour_and_24_bit_give_the_same_pen(void)
{
    xterm_begin();
    vt_put("\x1b[38;5;196mA");
    assert(probes == 1 && probe[0].fg == 1);
    probes = 0;
    vt_put("\x1b[38;2;255;0;0mA");
    assert(probes == 1 && probe[0].fg == 1);
    probes = 0;
    vt_put("\x1b[48;5;21mA");
    assert(probes == 1 && probe[0].bg == 4);
    probes = 0;
    vt_put("\x1b[0mA");                          /* the defaults: pens 7 and 0 */
    assert(probes == 1 && probe[0].fg == 7 && probe[0].bg == 0);
    VtClose();
}

/* ---- MA2-9: 24-bit colour on a true-colour (RTG) screen ---- */
static void tc_end(void)
{
    VtClose();
    bm.Depth = 4;
    bitmapFlags = BMF_STANDARD;
}

static void test_ma2_9_rtg_rgb_gets_an_obtained_pen_once_and_releases_it(void)
{
    xterm_begin_on(24, 0);
    vt_put("\x1b[38;2;18;52;86mX");
    assert(obtainCalls == 1);
    assert(obtainR == 0x12121212UL && obtainG == 0x34343434UL && obtainB == 0x56565656UL);
    assert(obtainTags[1] == PRECISION_EXACT && obtainTags[3] == 1);
    assert(probes == 1 && probe[0].fg == 16 && term_pen_map[16] == 50);
    assert(strstr(calls, "fg50 bg0"));                  /* SetAPen got the stub's pen */
    vt_put("\x1b[38;2;18;52;86mY");
    assert(obtainCalls == 1);                           /* cached */
    vt_put("\x1b[38;5;196mZ");                          /* a second colour: a second pen */
    assert(obtainCalls == 2 && probe[2].fg == 17);
    tc_end();
    assert(releaseCalls == 2);                          /* every obtained pen */
    assert(vtTcN == 0);
}

static void test_ma2_9_planar_4_bit_never_asks_for_a_pen(void)
{
    xterm_begin();
    vt_put("\x1b[38;2;255;0;0mA\x1b[38;5;21mB\x1b[31mC");
    assert(obtainCalls == 0);
    assert(probe[0].fg == Vt_PenFor(VT_COLOR_RGB | 0xFF0000UL, prefs.AnsiColors));
    assert(probe[1].fg == Vt_PenFor(21, prefs.AnsiColors));
    assert(probe[2].fg == 1);
    VtClose();
    assert(releaseCalls == 0);
    /* the contrast: the same bytes on a true-colour screen do ask */
    xterm_begin_on(24, 0);
    vt_put("\x1b[38;2;255;0;0mA");
    assert(obtainCalls == 1 && probe[0].fg == 16);
    tc_end();
}

static void test_ma2_9_the_241st_colour_falls_back_to_the_16_pen_answer(void)
{
    char seq[40];
    int i;

    xterm_begin_on(24, 0);
    for (i = 0; i < 240; i++) {
        snprintf(seq, sizeof(seq), "\x1b[38;2;%d;%d;1mX", i, i / 2);
        vt_put(seq);
        calls[0] = 0;                                   /* the log is a fixed buffer */
    }
    assert(obtainCalls == 240);
    probes = 0;
    vt_put("\x1b[38;2;255;0;0mX");
    assert(obtainCalls == 240);                         /* the cache is full: no 241st pen */
    assert(probes == 1 && probe[0].fg == Vt_PenFor(VT_COLOR_RGB | 0xFF0000UL, prefs.AnsiColors));
    tc_end();
    assert(releaseCalls == 240);
}

static void test_ma2_9_a_failed_obtain_falls_back_to_the_16_pen_answer(void)
{
    xterm_begin_on(24, 0);
    obtainFail = 1;
    vt_put("\x1b[38;2;255;0;0mX");
    assert(obtainCalls == 1 && probes == 1);
    assert(probe[0].fg == Vt_PenFor(VT_COLOR_RGB | 0xFF0000UL, prefs.AnsiColors));
    tc_end();
    assert(releaseCalls == 0);
}

/* ---- MA3: mouse, title, bracketed paste through the real entry points ---- */
static void ma3_reset_sentinels(void)
{
    sentLen = sentCalls = titleCalls = 0;
    titleSet[0] = 0;
}

static void test_ma3_1_2_osc_title_reaches_the_window_as_latin1(void)
{
    xterm_begin();
    ma3_reset_sentinels();
    vt_put("\x1b]0;hello\x07");
    assert(titleCalls == 1 && strcmp(titleSet, "hello") == 0);          /* MA3-1 */
    vt_put("\x1b]2;two\x07");
    assert(titleCalls == 2 && strcmp(titleSet, "two") == 0);
    vt_put("\x1b]0;st\x1b\\");                                          /* ST terminator */
    assert(titleCalls == 3 && strcmp(titleSet, "st") == 0);
    vt_put("\x1b]0;caf\xC3\xA9\x07");                                    /* MA3-2: UTF-8 in, Latin-1 out */
    assert(strcmp(titleSet, "caf\xE9") == 0);
    VtClose();
}

static void test_ma3_3_title_is_restored_on_reset_and_session_end(void)
{
    xterm_begin();
    ma3_reset_sentinels();
    vt_put("\x1b]0;hello\x07");
    vt_put("\x1b" "c");                                                  /* RIS */
    assert(strcmp(titleSet, MainWindowTitle) == 0);
    vt_put("\x1b]0;again\x07");
    assert(strcmp(titleSet, "again") == 0);
    VtTitleRestore();                                                     /* the session ended */
    assert(strcmp(titleSet, MainWindowTitle) == 0);
    ma3_reset_sentinels();
    vt_put("after");                                                      /* the old title does not come back */
    assert(titleCalls == 0);
    VtClose();
}

/* LIVE A1200: on DCTelnet's own screen the window is a backdrop with no title
 * bar, so an OSC title must reach the screen bar; on Workbench it must not. */
static void test_ma3_2_title_shows_in_screen_bar_on_own_screen(void)
{
    xterm_begin();
    ma3_reset_sentinels();
    ledCalls = 0;
    window.Flags = WFLG_BACKDROP;
    vt_put("\x1b]0;own screen\x07");
    assert(titleCalls == 1 && screenTitleSet != (const UBYTE *)(size_t)0xFFFFFFFFUL && strcmp((const char *)screenTitleSet, "own screen") == 0);
    assert(ledCalls == 1);                                                /* the bar redraw erased the LEDs */
    window.Flags = 0;
    vt_put("\x1b]0;workbench\x07");
    assert(screenTitleSet == (const UBYTE *)(size_t)0xFFFFFFFFUL && ledCalls == 1);
    VtClose();
}

static void test_ma3_4_5_mouse_press_release_bytes_and_selection_fallthrough(void)
{
    xterm_begin();
    ma3_reset_sentinels();
    assert(!VtMouseOwned(FALSE));                                         /* MA3-5: no mode, selects */
    VtMouseButton(0, TRUE, 10, 5, 0);
    assert(sentCalls == 0);
    vt_put("\x1b[?1000h\x1b[?1006h");
    assert(VtMouseOwned(FALSE) && !VtMouseOwned(TRUE));                   /* Shift keeps selecting */
    VtMouseButton(0, TRUE, 10, 5, 0);                                     /* MA3-4 */
    assert(sentLen == 10 && memcmp(sentBuf, "\x1b[<0;11;6M", 10) == 0);
    sentLen = 0;
    VtMouseButton(0, FALSE, 10, 5, 0);
    assert(sentLen == 10 && memcmp(sentBuf, "\x1b[<0;11;6m", 10) == 0);
    sentLen = 0;
    VtMouseButton(2, TRUE, 10, 5, 0);
    assert(memcmp(sentBuf, "\x1b[<2;11;6M", 10) == 0);
    sentLen = 0;
    VtMouseButton(64, TRUE, 10, 5, 0);
    assert(sentLen == 11 && memcmp(sentBuf, "\x1b[<64;11;6M", 11) == 0);
    VtClose();
}

static void test_ma3_6_motion_modes_and_the_mouse_move_flag(void)
{
    xterm_begin();
    ma3_reset_sentinels();
    window.IDCMPFlags = IDCMP_RAWKEY | IDCMP_MOUSEMOVE;
    VtOpen();                                                             /* built-in: no selection to drag */
    assert(!(window.IDCMPFlags & IDCMP_MOUSEMOVE));
    vt_put("\x1b[?1000h\x1b[?1006h");
    assert(!(window.IDCMPFlags & IDCMP_MOUSEMOVE));
    vt_put("\x1b[?1002h");
    assert(window.IDCMPFlags & IDCMP_MOUSEMOVE);
    VtMouseMove(3, 3, 0);                                                 /* no button held: nothing */
    assert(sentCalls == 0);
    VtMouseButton(0, TRUE, 3, 3, 0);
    sentLen = sentCalls = 0;
    VtMouseMove(4, 3, 0);                                                 /* drag */
    assert(sentLen == 10 && memcmp(sentBuf, "\x1b[<32;5;4M", 10) == 0);
    VtMouseButton(0, FALSE, 4, 3, 0);
    sentLen = sentCalls = 0;
    VtMouseMove(5, 3, 0);
    assert(sentCalls == 0);
    vt_put("\x1b[?1002l\x1b[?1003h");
    VtMouseMove(6, 3, 0);                                                 /* ?1003: all motion */
    assert(sentLen == 10 && memcmp(sentBuf, "\x1b[<35;7;4M", 10) == 0);
    vt_put("\x1b[?1003l");
    assert(!(window.IDCMPFlags & IDCMP_MOUSEMOVE));
    VtClose();
}

/* --- MA4: engine scrollback for the built-in renderer --- */

#define SB_ART_ROWS 200

static void sb_begin(ULONG state, ULONG lines)
{
    setup();
    memset(&prefs, 0, sizeof(prefs));
    prefs.State = state;
    prefs.nScrollbackLines = lines;
    assert(VtOpen() == 0);
    probes = 0;
}

/* 200 numbered rows ("L001" ... "L200") through VtWrite, no trailing newline. */
static void sb_art(int rows)
{
    char line[16];
    int i;

    for (i = 1; i <= rows; i++) {
        snprintf(line, sizeof(line), "%sL%03d", i > 1 ? "\r\n" : "", i);
        vt_put(line);
    }
}

/* The text of grid row y as last drawn: the probe's glyphs of that row. */
static void sb_screen_row(int y, char out[5])
{
    int i;

    memset(out, ' ', 4);
    out[4] = 0;
    for (i = 0; i < probes && i < PROBE_MAX; i++)
        if (probe[i].y == y && probe[i].x < 4)
            out[probe[i].x] = (char)probe[i].code;
}

static void sb_expect_rows(int first)
{
    char want[8], got[5];
    int y;

    for (y = 0; y < term_rows; y++) {
        snprintf(want, sizeof(want), "L%03d", first + y);
        sb_screen_row(y, got);
        assert(strcmp(want, got) == 0);
    }
}

static void test_ma4_2_long_art_fills_the_engine_scrollback_and_only_when_asked(void)
{
    int n;
    const vt_cell *r;

    sb_begin(APP_SCROLLBACK_ENABLED, 500);
    assert(newSb == 500 && term_rows == 30);
    sb_art(SB_ART_ROWS);
    assert(vt_scrollback_lines(vtTerm) == SB_ART_ROWS - term_rows);    /* 170 */
    r = vt_row(vtTerm, -(SB_ART_ROWS - term_rows), &n);                  /* the oldest line */
    assert(r && r[0].ch == 'L' && r[1].ch == '0' && r[2].ch == '0' && r[3].ch == '1');
    r = vt_row(vtTerm, 0, &n);                                           /* grid top: row 171 */
    assert(r && r[1].ch == '1' && r[2].ch == '7' && r[3].ch == '1');
    VtClose();

    /* off by default: no scrollback memory is asked for, none is kept */
    sb_begin(0, 500);
    assert(newSb == 0);
    sb_art(SB_ART_ROWS);
    assert(vt_scrollback_lines(vtTerm) == 0);
    VtClose();
    sb_begin(APP_SCROLLBACK_ENABLED, 0);                                  /* on, zero lines */
    assert(newSb == 0);
    VtClose();

    /* the preference is read at every VtOpen: the full reopen DCTelnet does on a toggle */
    sb_begin(APP_SCROLLBACK_ENABLED, 40);
    assert(newSb == 40);
    prefs.State = 0;
    assert(VtOpen() == 0 && newSb == 0);
    prefs.State = APP_SCROLLBACK_ENABLED;
    prefs.nScrollbackLines = 100000UL;
    assert(VtOpen() == 0 && newSb == 32767);                              /* clamped to the engine's int */
    VtClose();
}

static void test_ma4_3_shift_page_up_draws_the_previous_page_and_page_down_returns(void)
{
    char got[5];
    int i;

    sb_begin(APP_SCROLLBACK_ENABLED, 500);
    sb_art(SB_ART_ROWS);                       /* grid shows L171..L200 */
    probes = 0;
    assert(VtScrollKey(0x48, FALSE) == FALSE); /* plain Page Up belongs to the host */
    assert(probes == 0 && vtScrollOff == 0);

    assert(VtScrollKey(0x48, TRUE) == TRUE);   /* sentinel: the scrollback draw ran */
    assert(vtScrollOff == term_rows - 1 && probes == term_rows * term_cols);
    sb_expect_rows(171 - (term_rows - 1));     /* L142 .. L171 */
    assert(cursor_visible == 0);               /* no cursor in the history */

    for (i = 0; i < 20; i++)                   /* far past the top: clamped */
        VtScrollKey(0x48, TRUE);
    assert(vtScrollOff == SB_ART_ROWS - term_rows);
    probes = 0;
    VtScrollView(0);
    VtScrollKey(0x48, TRUE);                   /* already at the top: nothing redrawn */
    assert(probes == 0);
    probes = 0;
    VtScrollView(-vtScrollOff);                /* a full redraw of the top view first */
    VtScrollView(SB_ART_ROWS);
    sb_expect_rows(1);                         /* the art's first row is the top row */

    probes = 0;
    while (vtScrollOff)
        VtScrollKey(0x49, TRUE);
    probes = 0;
    VtScrollView(0);
    VtRedraw();                                /* the live grid, as the engine holds it */
    sb_expect_rows(171);
    assert(vtScrollOff == 0);

    /* output while scrolled back snaps to the live view first */
    VtScrollKey(0x48, TRUE);
    assert(vtScrollOff != 0);
    probes = 0;
    vt_put("\r\nL201");
    assert(vtScrollOff == 0);
    sb_screen_row(term_rows - 1, got);
    assert(strcmp(got, "L201") == 0);

    /* the wheel: 3 lines per click, not while a program owns the mouse */
    assert(VtScrollKey(0x7A, FALSE) == TRUE && vtScrollOff == 3);
    assert(VtScrollKey(0x7B, FALSE) == TRUE && vtScrollOff == 0);
    vt_put("\033[?1000h");
    assert(VtScrollKey(0x7A, FALSE) == FALSE && vtScrollOff == 0);
    VtClose();

    /* scrollback off: the keys are not taken, they go to the host */
    sb_begin(0, 500);
    sb_art(SB_ART_ROWS);
    assert(VtScrollKey(0x48, TRUE) == FALSE && vtScrollOff == 0);
    VtClose();
}

static void test_ma4_3_a_scrollback_line_narrower_than_the_grid_is_blank_filled(void)
{
    sb_begin(APP_SCROLLBACK_ENABLED, 500);
    sb_art(60);
    vt_resize(vtTerm, term_cols - 10, term_rows);
    probes = 0;
    VtDraw(NULL, 0, 0, term_cols, term_rows);
    assert(probes == term_rows * term_cols);
    assert(probe[term_cols - 1].code == ' ' && probe[term_cols - 1].x == term_cols - 1);
    VtClose();
}

static void test_ma4_4_the_alt_screen_leaves_the_scrollback_alone(void)
{
    int before;

    sb_begin(APP_SCROLLBACK_ENABLED, 500);
    sb_art(100);
    before = vt_scrollback_lines(vtTerm);
    assert(before == 100 - term_rows);
    vt_put("\033[?1049h");
    sb_art(SB_ART_ROWS);                       /* a full-screen program scrolling */
    assert(vt_scrollback_lines(vtTerm) == before);
    probes = 0;
    assert(VtScrollKey(0x48, TRUE) == TRUE && vtScrollOff == 0 && probes == 0);  /* nothing to show */
    vt_put("\033[?1049l");
    assert(vt_scrollback_lines(vtTerm) == before);
    VtClose();
}

int main(void)
{
    test_the_terminal_starts_below_the_title_bar_and_matches_the_window_rows();
    test_a_covered_terminal_draws_only_through_the_rastport();
    test_uncovered_again_it_draws_directly();
    test_an_rtg_bitmap_always_takes_the_rastport();
    test_the_pen_map_colours_the_rastport_path_and_stops_direct_drawing();
    test_an_8_plane_screen_takes_the_rastport();
    test_a_16_pixel_font_gives_40_columns();
    test_vtopen_twice_never_draws_through_the_previous_engine();
    test_a_menu_cannot_open_while_the_terminal_draws_into_the_bitmap();
    test_a_menu_open_over_the_terminal_is_drawn_around();
    test_a_menu_opened_while_an_answer_is_sent_is_drawn_around();
    test_ma1_2_line_art_lands_in_the_grid_and_reaches_the_glyph_writer();
    test_ma1_3_blocks_and_shades_map_to_themselves_never_to_the_fallback();
    test_ma1_1_latin1_byte_reaches_the_glyph_writer_unchanged();
    test_ma1_4_sixteen_colours();
    test_ma1_5_6_7_ice_colours_decide_what_blink_draws_as();
    test_ma1_8_cursor_save_restore_position_and_erase();
    test_ma1_init_strings_draw_nothing_and_leave_the_cursor();
    test_ma1_9_the_sample_art_file_gives_the_known_grid();
    test_font_probe_reaches_the_grid_through_vtwrite();
    test_a_drawn_cell_on_a_covered_terminal_goes_out_as_rectfills();
    test_ma2_5_11_utf8_reaches_the_grid_whole_even_split_across_writes();
    test_ma2_14_stream_switch_to_utf8_through_vtwrite();
    test_ma2_14_split_sequence_held_back_through_vtwrite();
    test_ma2_14_flush_draws_held_byte_split_block_still_whole();
    test_ma2_14_apply_personality_keeps_the_switch();
    test_ma2_6_glyph_map_and_fallback();
    test_ma2_8_256_colour_and_24_bit_give_the_same_pen();
    test_ma2_9_rtg_rgb_gets_an_obtained_pen_once_and_releases_it();
    test_ma2_9_planar_4_bit_never_asks_for_a_pen();
    test_ma2_9_the_241st_colour_falls_back_to_the_16_pen_answer();
    test_ma2_9_a_failed_obtain_falls_back_to_the_16_pen_answer();
    test_ma3_1_2_osc_title_reaches_the_window_as_latin1();
    test_ma3_3_title_is_restored_on_reset_and_session_end();
    test_ma3_2_title_shows_in_screen_bar_on_own_screen();
    test_ma3_4_5_mouse_press_release_bytes_and_selection_fallthrough();
    test_ma3_6_motion_modes_and_the_mouse_move_flag();
    test_ma4_2_long_art_fills_the_engine_scrollback_and_only_when_asked();
    test_ma4_3_shift_page_up_draws_the_previous_page_and_page_down_returns();
    test_ma4_3_a_scrollback_line_narrower_than_the_grid_is_blank_filled();
    test_ma4_4_the_alt_screen_leaves_the_scrollback_alone();
    printf("term_engine: all assertions passed\n");
    return 0;
}
