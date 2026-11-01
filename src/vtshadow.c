/* src/vtshadow.c -- the screen of the console renderers, kept over a display
 * reopen (vtshadow.h). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vtshadow.h"
#include "ansiscan.h"

static vt_term *shadow;         /* NULL unless a console renderer is up */
static BOOL shadowCp437;        /* its dialect: pcansi (ibmcon) or amiga (console.device) */
static BOOL replaying;          /* Shadow_Feed ignores the replay's own bytes */

/* The engine's replies (DSR, DA...) never go to the BBS: the console renderer
 * answers for itself. Only Shadow_Serialize's DECRQM probe reads one. */
static char replyBuf[32];
static long replyLen;

static void ShadowReply(void *user, const vt_u8 *buf, long len)
{
    (void)user;
    while (len-- > 0 && replyLen < (long)sizeof(replyBuf) - 1)
        replyBuf[replyLen++] = (char)*buf++;
    replyBuf[replyLen] = 0;
}

vt_term *Shadow_NewTerm(int cols, int rows, BOOL cp437)
{
    static vt_callbacks cb;         /* zero: no damage, scroll, bell... */
    vt_term *t;

    cb.reply = ShadowReply;
    t = vt_new(cols, rows, 0, &cb, NULL);

    if (t)
    {
        vt_set_personality(t, cp437 ? VT_PCANSI : VT_AMIGA);
        vt_set_music(t, 0);         /* AnsiMusic_Filter took it out already */
    }
    return t;
}

void Shadow_Feed(const UBYTE *data, long len)
{
    if (shadow && !replaying && len > 0)
        vt_write(shadow, (const vt_u8 *)data, len);
}

static UBYTE PaletteIndex(const vt_term *t, vt_color c, UBYTE def)
{
    if (c & VT_COLOR_RGB)
        c = (vt_color)vt_rgb_to_256(VT_RGB_OF(c));
    (void)t;
    return c < 16 ? (UBYTE)c : def;     /* default, and 256-colour beyond 16: the default */
}

void Shadow_Cell(const vt_term *t, const vt_cell *c, BOOL cp437, UBYTE out[4])
{
    vt_u16 ch = c->ch;
    UBYTE b = '?';

    if ((c->pad & VT_CELL_IMAGE) || c->width == 0)
        b = ' ';
    else if (VT_CELL_IS_CLUSTER(c))
        b = '?';
    else if (ch >= 0x20 && ch < 0x7F)
        b = (UBYTE)ch;
    else if (cp437 && ch >= 0x80)
    {
        const vt_u16 *hi = vt_cp437_table();
        int i;

        for (i = 0; i < 128; i++)
            if (hi[i] == ch) { b = (UBYTE)(0x80 + i); break; }
    }
    else if (!cp437 && ch >= 0xA0 && ch <= 0xFF)
        b = (UBYTE)ch;
    out[0] = b;
    out[1] = PaletteIndex(t, c->fg, 7);
    out[2] = PaletteIndex(t, c->bg, 0);
    out[3] = (UBYTE)(((c->attr & VT_ATTR_UNDERLINE) ? 0x01 : 0)
                     | ((c->attr & VT_ATTR_BOLD) ? 0x10 : 0)
                     | ((c->attr & VT_ATTR_INVERSE) ? 0x20 : 0)
                     | ((c->attr & VT_ATTR_BLINK) ? 0x40 : 0));
}

/* DECRQM ?7 on t itself: is autowrap on? (A query, no state changes.) */
static BOOL AutowrapOn(vt_term *t)
{
    static const vt_u8 q[] = "\033[?7$p";

    replyLen = 0;
    replyBuf[0] = 0;
    vt_write(t, q, (long)sizeof(q) - 1);
    return strstr(replyBuf, "?7;2$y") == NULL;     /* no answer: on, the default */
}

long Shadow_Serialize(vt_term *t, BOOL cp437, ShadowWriteFn write, void *user)
{
    static const char head[] = "\033[?7l\033[0m\033[H\033[J";
    int cols = vt_cols(t), rows = vt_rows(t), x, y, cx, cy;
    BOOL wrap = AutowrapOn(t), started = FALSE;
    ULONG attr = ANSI_ATTR_RESET;
    UBYTE *cells = (UBYTE *)malloc((size_t)cols * 4);
    char *buf = (char *)malloc((size_t)cols * 24 + 32);
    char tail[40];
    long total = 0;

    if (!cells || !buf)
    {
        free(cells);
        free(buf);
        return 0;
    }
    vt_cursor(t, &cx, &cy);
    for (y = 0; y < rows; y++)
    {
        int n = 0, pos;
        const vt_cell *row = vt_row(t, y, &n);
        size_t len;

        for (x = 0; x < cols; x++)
        {
            if (x < n)
                Shadow_Cell(t, &row[x], cp437, cells + x * 4);
            else
            {
                cells[x * 4] = ' '; cells[x * 4 + 1] = 7; cells[x * 4 + 2] = 0; cells[x * 4 + 3] = 0;
            }
        }
        pos = sprintf(buf, "\033[%d;1H", y + 1);
        len = Ansi_ScreenRow(cells, (UWORD)cols, FALSE, &attr, buf + pos) - 2;  /* no CR LF */
        if (len == 0)
            continue;
        if (!started)
        {
            write(user, head, (long)sizeof(head) - 1);
            total += (long)sizeof(head) - 1;
            started = TRUE;
        }
        write(user, buf, (long)(pos + len));
        total += (long)(pos + len);
    }
    if (started || cx != 0 || cy != 0)
    {
        int m;

        if (!started)
        {
            write(user, head, (long)sizeof(head) - 1);
            total += (long)sizeof(head) - 1;
        }
        m = sprintf(tail, "\033[0m\033[%d;%dH%s", cy + 1, cx + 1, wrap ? "\033[?7h" : "");
        write(user, tail, m);
        total += m;
    }
    free(cells);
    free(buf);
    return total;
}

static void FeedInto(void *user, const char *data, long len)
{
    vt_write((vt_term *)user, (const vt_u8 *)data, len);
}

BOOL Shadow_DisplayOpened(enum RendererRoute route, int cols, int rows,
                          ShadowWriteFn write, void *user)
{
    BOOL cp437 = route == ROUTE_IBMCON, done;

    if (route == ROUTE_NONE)
        return FALSE;               /* iconified: nothing is shown, the shadow keeps going */
    if (route != ROUTE_CONSOLE && route != ROUTE_IBMCON)
    {
        Shadow_Close();             /* the built-in renderer keeps its own engine */
        return FALSE;
    }
    if (!shadow)
    {
        shadow = Shadow_NewTerm(cols, rows, cp437);
        shadowCp437 = cp437;
        return FALSE;               /* a new screen: nothing to show yet */
    }
    if (cp437 != shadowCp437)       /* ibmcon <-> console.device: the screen moves over */
    {
        vt_term *next = Shadow_NewTerm(vt_cols(shadow), vt_rows(shadow), cp437);

        if (next)
            Shadow_Serialize(shadow, shadowCp437, FeedInto, next);
        vt_free(shadow);
        shadow = next;
        shadowCp437 = cp437;
        if (!shadow)
            return FALSE;
    }
    vt_resize(shadow, cols, rows);
    replaying = TRUE;
    done = Shadow_Serialize(shadow, cp437, write, user) > 0;
    replaying = FALSE;
    return done;
}

void Shadow_Close(void)
{
    if (shadow)
        vt_free(shadow);
    shadow = NULL;
}

vt_term *Shadow_Term(void)
{
    return shadow;
}
