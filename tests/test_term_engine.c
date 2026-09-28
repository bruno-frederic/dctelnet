/* tests/test_term_engine.c -- retro32-term (src/third_party/retro32-term/term-engine.c)
 * on the host, with the graphics.library calls it makes recorded. While
 * something covers the terminal (term_covered) and on a bitmap that is not
 * planar (RTG), it must draw only through the window's RastPort -- never
 * into the bitplanes, which then hold another window's pixels -- and
 * directly into its own area of the bitmap otherwise. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <exec/types.h>

typedef char *STRPTR;
struct Library { UWORD lib_Version; };
struct GfxBase { struct Library LibNode; };
struct BitMap { UWORD BytesPerRow, Rows; UBYTE Flags, Depth; UWORD pad; UBYTE *Planes[8]; };
struct TextFont { UWORD tf_YSize, tf_Baseline; UBYTE tf_LoChar, tf_HiChar; void *tf_CharData; UWORD tf_Modulo; void *tf_CharLoc; };
struct RastPort { struct BitMap *BitMap; UBYTE Mask, FgPen, BgPen, DrawMode; WORD cx, cy; struct TextFont *Font; };
struct Screen { WORD Width, Height; struct RastPort RastPort; };
#define JAM1 0
#define JAM2 1
#define COMPLEMENT 2
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

static void WaitBlit(void) {}
static ULONG GetBitMapAttr(struct BitMap *b, ULONG attr) { (void)b; return attr == BMA_FLAGS ? bitmapFlags : 0; }
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

static void send_data_(const UBYTE *d, LONG n) { (void)d; (void)n; }
#define send_data(d, n) send_data_((d), (n))
#include "../src/third_party/retro32-term/term-engine.c"

/* A font whose glyphs all differ: row r of character c is c ^ (r * 37). */
static UBYTE fontData[8][256];
static ULONG fontLoc[256];
static struct TextFont font = { 8, 6, 0, 255, fontData, 256, fontLoc };
static struct Screen screen = { W, H, { &bm, 0xFF, 0, 0, 0, 0, 0, NULL } };
static struct RastPort winRp;

/* The terminal of a 640x245 window at y = 11, under a title bar: 30 rows. */
#define TOP 11
static void setup(void)
{
    WORD c, r, p;

    for (c = 0; c < 256; c++) {
        fontLoc[c] = (ULONG)(c * 8) << 16 | 8;
        for (r = 0; r < 8; r++)
            fontData[r][c] = (UBYTE)(c ^ (r * 37));
    }
    bm.BytesPerRow = BPR; bm.Rows = H; bm.Depth = 4;
    for (p = 0; p < 4; p++) bm.Planes[p] = planes[p];
    memset(planes, 0x55, sizeof(planes));           /* "the title bar and other windows" */
    memset(&winRp, 0, sizeof(winRp));
    winRp.Mask = 0xFF;
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
    assert(strstr(calls, "ScrollRaster(0,8 0,0,639,239 bg0)"));      /* the line feed at the bottom */
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

static void test_the_pen_map_colours_the_rastport_path(void)
{
    static const UBYTE pens[16] = { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55 };

    setup();
    term_set_pens(pens);
    term_covered(1);
    feed("\x1b[1;33mY");
    assert(strstr(calls, "Text(0,6 fg51 bg40 \"Y\")"));    /* bright yellow = ANSI 11 */
}

int main(void)
{
    test_the_terminal_starts_below_the_title_bar_and_matches_the_window_rows();
    test_a_covered_terminal_draws_only_through_the_rastport();
    test_uncovered_again_it_draws_directly();
    test_an_rtg_bitmap_always_takes_the_rastport();
    test_the_pen_map_colours_the_rastport_path();
    printf("term_engine: all assertions passed\n");
    return 0;
}
