/* tests/test_vtshadow.c -- the console renderers' screen kept over a display
 * reopen (src/vtshadow.c). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vtshadow.h"
#include "ansiscan.h"

struct Sink { char buf[65536]; long len; int calls; };

static void SinkWrite(void *user, const char *data, long len)
{
    struct Sink *s = (struct Sink *)user;

    assert(s->len + len <= (long)sizeof(s->buf));
    memcpy(s->buf + s->len, data, (size_t)len);
    s->len += len;
    s->calls++;
}

/* BBS art: colours, bold, blink, reverse, underline, CP437 / Latin-1 high
 * bytes, cursor addressing, a full bottom row (whose last cell would scroll a
 * console that wraps), the cursor left mid-screen. */
static const char fixture[] =
    "\033[2J\033[H\033[1;33;44mTOP LINE\033[0m plain \033[5;31mblink\033[0m"
    "\r\n\033[7mreversed\033[0m \033[4munder\033[0m \033[1;37;41m\xb0\xb1\xb2\xdb\xc9\xcd\xbb\033[0m"
    "\033[10;30H\033[32mgreen at 10,30\033[0m"
    "\033[25;1H\033[36;45m"
    "0123456789012345678901234567890123456789012345678901234567890123456789012345678X"
    "\033[0m\033[12;40H";

static void assert_same_screen(vt_term *a, vt_term *b, BOOL cp437)
{
    int x, y, ax, ay, bx, by;

    assert(vt_cols(a) == vt_cols(b) && vt_rows(a) == vt_rows(b));
    for (y = 0; y < vt_rows(a); y++)
    {
        int na = 0, nb = 0;
        const vt_cell *ra = vt_row(a, y, &na), *rb = vt_row(b, y, &nb);

        for (x = 0; x < vt_cols(a); x++)
        {
            UBYTE ca[4] = { ' ', 7, 0, 0 }, cb[4] = { ' ', 7, 0, 0 };

            if (x < na) Shadow_Cell(a, &ra[x], cp437, ca);
            if (x < nb) Shadow_Cell(b, &rb[x], cp437, cb);
            if (memcmp(ca, cb, 4) != 0)
            {
                fprintf(stderr, "cell %d,%d: %02x %d %d %02x vs %02x %d %d %02x\n", x, y,
                        ca[0], ca[1], ca[2], ca[3], cb[0], cb[1], cb[2], cb[3]);
                assert(0);
            }
        }
    }
    vt_cursor(a, &ax, &ay);
    vt_cursor(b, &bx, &by);
    assert(ax == bx && ay == by);
}

static vt_term *Fed(BOOL cp437, const char *s, long n)
{
    vt_term *t = Shadow_NewTerm(80, 25, cp437);

    assert(t);
    vt_write(t, (const vt_u8 *)s, n);
    return t;
}

static UBYTE CellChar(vt_term *t, int x, int y, BOOL cp437)
{
    int n = 0;
    const vt_cell *r = vt_row(t, y, &n);
    UBYTE c[4];

    assert(x < n);
    Shadow_Cell(t, &r[x], cp437, c);
    return c[0];
}

static void round_trip(BOOL cp437)
{
    static struct Sink s1, s2;
    /* console.device wraps as soon as the last column is written (vtcon's amiga
     * personality does too): the fixture's full bottom row would scroll the
     * real console as well. With wrap off it stays where it was drawn. */
    static char amiga[sizeof(fixture) + 8] = "\033[?7l";
    vt_term *a, *b;
    UBYTE top[4];
    int n = 0, cx, cy;

    if (!amiga[6])
        strcat(amiga, fixture);
    a = cp437 ? Fed(TRUE, fixture, (long)sizeof(fixture) - 1) : Fed(FALSE, amiga, (long)strlen(amiga));

    s1.len = s2.len = 0;
    assert(Shadow_Serialize(a, cp437, SinkWrite, &s1) == s1.len && s1.len > 0);
    b = Fed(cp437, s1.buf, s1.len);
    assert_same_screen(a, b, cp437);
    /* The bottom row's last cell did not scroll the screen. */
    assert(CellChar(b, 0, 0, cp437) == 'T' && CellChar(b, 79, 24, cp437) == 'X');
    Shadow_Cell(b, &vt_row(b, 0, &n)[0], cp437, top);
    assert(top[1] == 3 && top[2] == 4 && (top[3] & 0x10));        /* bold yellow on blue */
    assert(CellChar(b, 0, 1, cp437) == 'r');
    if (cp437)
        assert(CellChar(b, 15, 1, TRUE) == 0xb0 && CellChar(b, 21, 1, TRUE) == 0xbb);
    else
        assert(CellChar(b, 15, 1, FALSE) == 0xb0 && CellChar(b, 18, 1, FALSE) == 0xdb);
    vt_cursor(b, &cx, &cy);
    assert(cx == 39 && cy == 11);
    /* Serialising the copy gives the same bytes: nothing drifts per reopen. */
    assert(Shadow_Serialize(b, cp437, SinkWrite, &s2) == s1.len && memcmp(s1.buf, s2.buf, (size_t)s1.len) == 0);
    vt_free(a);
    vt_free(b);
}

static void test_the_replayed_screen_matches_cell_for_cell_ibmcon(void) { round_trip(TRUE); }
static void test_the_replayed_screen_matches_cell_for_cell_console_device(void) { round_trip(FALSE); }

static void test_an_empty_screen_replays_nothing(void)
{
    static struct Sink s;
    vt_term *t = Shadow_NewTerm(80, 25, TRUE);

    s.len = 0;
    assert(Shadow_Serialize(t, TRUE, SinkWrite, &s) == 0 && s.calls == 0);
    vt_free(t);
}

/* A replay into a fresh engine: what the reopened console would show. */
static vt_term *Replayed(const struct Sink *s, BOOL cp437, int cols, int rows)
{
    vt_term *t = Shadow_NewTerm(cols, rows, cp437);

    vt_write(t, (const vt_u8 *)s->buf, s->len);
    return t;
}

static void FeedBack(void *user, const char *data, long len)
{
    SinkWrite(user, data, len);
    Shadow_Feed((const UBYTE *)data, len);     /* ConWrite feeds the shadow too */
}

static void test_a_display_reopen_on_a_console_renderer_replays_the_screen(void)
{
    static struct Sink s;
    vt_term *ref = Fed(TRUE, fixture, (long)sizeof(fixture) - 1), *shown;

    Shadow_Close();
    s.len = 0; s.calls = 0;
    assert(!Shadow_DisplayOpened(ROUTE_IBMCON, 80, 25, SinkWrite, &s) && Shadow_Term() && s.calls == 0);
    Shadow_Feed((const UBYTE *)fixture, (long)sizeof(fixture) - 1);
    /* Tool Bar on: CloseDisplay, OpenDisplay -> the new unit gets the screen. */
    assert(Shadow_DisplayOpened(ROUTE_IBMCON, 80, 25, FeedBack, &s) && s.calls > 0);
    shown = Replayed(&s, TRUE, 80, 25);
    assert_same_screen(ref, shown, TRUE);
    assert_same_screen(ref, Shadow_Term(), TRUE);   /* its own replay did not go back in */
    vt_free(shown);
    /* Iconified: nothing drawn, the shadow stays. */
    assert(!Shadow_DisplayOpened(ROUTE_NONE, 80, 25, SinkWrite, &s) && Shadow_Term());
    vt_free(ref);
}

static void test_switching_to_built_in_frees_the_shadow_and_replays_nothing(void)
{
    static struct Sink s;

    Shadow_Close();
    Shadow_DisplayOpened(ROUTE_CONSOLE, 80, 25, SinkWrite, &s);
    Shadow_Feed((const UBYTE *)fixture, (long)sizeof(fixture) - 1);
    s.len = 0; s.calls = 0;
    assert(!Shadow_DisplayOpened(ROUTE_BUILTIN, 80, 25, SinkWrite, &s) && s.calls == 0 && !Shadow_Term());
    Shadow_Feed((const UBYTE *)"x", 1);             /* no engine: nothing happens */
    assert(!Shadow_DisplayOpened(ROUTE_XEM, 80, 25, SinkWrite, &s) && s.calls == 0 && !Shadow_Term());
}

static void test_ibmcon_to_console_device_carries_the_screen_over(void)
{
    static struct Sink s;
    const char text[] = "\033[1;32mhello\033[0m\033[3;5Hworld";
    vt_term *shown;

    Shadow_Close();
    Shadow_DisplayOpened(ROUTE_IBMCON, 80, 25, SinkWrite, &s);
    Shadow_Feed((const UBYTE *)text, (long)sizeof(text) - 1);
    s.len = 0;
    assert(Shadow_DisplayOpened(ROUTE_CONSOLE, 80, 25, SinkWrite, &s));
    shown = Replayed(&s, FALSE, 80, 25);
    assert(CellChar(shown, 0, 0, FALSE) == 'h' && CellChar(shown, 4, 2, FALSE) == 'w');
    vt_free(shown);
    Shadow_Close();
}

/* Full-screen off -> on (25 -> 32 rows) and a smaller window (25 -> 20): the
 * grid follows; on shrinking, vt_resize keeps the rows up to the cursor's. */
static void test_a_new_grid_size_resizes_before_the_replay(void)
{
    static struct Sink s;
    vt_term *shown;
    int i;
    char line[16];

    Shadow_Close();
    Shadow_DisplayOpened(ROUTE_IBMCON, 80, 25, SinkWrite, &s);
    for (i = 1; i <= 25; i++)
    {
        int n = sprintf(line, "\033[%d;1HL%d", i, i);
        Shadow_Feed((const UBYTE *)line, n);
    }
    s.len = 0;
    assert(Shadow_DisplayOpened(ROUTE_IBMCON, 80, 32, SinkWrite, &s));
    shown = Replayed(&s, TRUE, 80, 32);
    assert(vt_rows(Shadow_Term()) == 32 && CellChar(shown, 1, 0, TRUE) == '1');
    vt_free(shown);
    s.len = 0;
    assert(Shadow_DisplayOpened(ROUTE_IBMCON, 80, 20, SinkWrite, &s));
    shown = Replayed(&s, TRUE, 80, 20);
    /* The cursor was on L25 (row 24): the bottom 20 rows stay, L6 on top. */
    assert(CellChar(shown, 0, 0, TRUE) == 'L' && CellChar(shown, 1, 0, TRUE) == '6');
    printf("shrink 32->20: top row now L%c%c\n", CellChar(shown, 1, 0, TRUE), CellChar(shown, 2, 0, TRUE));
    vt_free(shown);
    Shadow_Close();
}

/* The cost of the extra parse, against Ansi_Swap17 (the per-byte pass
 * console.device's bytes take already). Printed, not asserted. */
static void measure_feed_cost(void)
{
    static UBYTE big[1 << 20], out[(1 << 20) + ANSI_SWAP17_HELD];
    struct AnsiSwap17 sw;
    vt_term *t = Shadow_NewTerm(80, 25, TRUE);
    long i, n = 0;
    clock_t c0, c1, c2;
    int rep;

    while (n + (long)sizeof(fixture) < (long)sizeof(big))
    {
        memcpy(big + n, fixture, sizeof(fixture) - 1);
        n += (long)sizeof(fixture) - 1;
    }
    memset(&sw, 0, sizeof(sw));
    c0 = clock();
    for (rep = 0; rep < 8; rep++)
        for (i = 0; i < n; i += 1024)
            vt_write(t, big + i, n - i < 1024 ? n - i : 1024);
    c1 = clock();
    for (rep = 0; rep < 8; rep++)
        for (i = 0; i < n; i += 1024)
            Ansi_Swap17(&sw, big + i, (size_t)(n - i < 1024 ? n - i : 1024), out);
    c2 = clock();
    printf("host feed: shadow vt_write %.1f MB/s, Ansi_Swap17 %.1f MB/s (%ld bytes x 8)\n",
           8.0 * n / 1e6 / ((double)(c1 - c0) / CLOCKS_PER_SEC),
           8.0 * n / 1e6 / ((double)(c2 - c1) / CLOCKS_PER_SEC + 1e-9), n);
    vt_free(t);
}

int main(void)
{
    test_the_replayed_screen_matches_cell_for_cell_ibmcon();
    test_the_replayed_screen_matches_cell_for_cell_console_device();
    test_an_empty_screen_replays_nothing();
    test_a_display_reopen_on_a_console_renderer_replays_the_screen();
    test_switching_to_built_in_frees_the_shadow_and_replays_nothing();
    test_ibmcon_to_console_device_carries_the_screen_over();
    test_a_new_grid_size_resizes_before_the_replay();
    measure_feed_cost();
    printf("vtshadow: all assertions passed\n");
    return 0;
}
