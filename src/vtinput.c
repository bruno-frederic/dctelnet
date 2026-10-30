/* src/vtinput.c -- see vtinput.h. */
#include <string.h>
#include "vtinput.h"

#define MOUSE_MODES (VT_MODE_MOUSE_X10 | VT_MODE_MOUSE_NORMAL | VT_MODE_MOUSE_BUTTON | VT_MODE_MOUSE_ANY)

void Vt_MouseReset(struct VtMouse *m)
{
    m->held = 0;
    m->button = 0;
    m->lastX = m->lastY = -1;
}

BOOL Vt_MouseOwned(const vt_term *t, BOOL shift)
{
    return t && !shift && (vt_modes(t) & MOUSE_MODES) != 0;
}

int Vt_MouseButton(struct VtMouse *m, const vt_term *t, int button, BOOL down,
                   int x, int y, int mods, UBYTE *out)
{
    if (button < 3) {
        if (down) {
            m->held |= (UBYTE)(1 << button);
            m->button = (UBYTE)button;
        } else
            m->held &= (UBYTE)~(1 << button);
    }
    m->lastX = (WORD)x;
    m->lastY = (WORD)y;
    return vt_encode_mouse(t, button, button >= 64 ? 0 : (down ? 0 : 1), x, y, mods, out);
}

int Vt_WheelButton(UWORD rawkey)
{
    if (rawkey == 0x7A) return 64;
    if (rawkey == 0x7B) return 65;
    return -1;
}

int Vt_MouseMove(struct VtMouse *m, const vt_term *t, int x, int y, int mods, UBYTE *out)
{
    vt_u32 modes = vt_modes(t);
    int button;

    if (x == m->lastX && y == m->lastY)
        return 0;
    if (!(modes & VT_MODE_MOUSE_ANY) && !((modes & VT_MODE_MOUSE_BUTTON) && m->held))
        return 0;
    m->lastX = (WORD)x;
    m->lastY = (WORD)y;
    button = m->held ? m->button : 3;   /* 3: no button */
    return vt_encode_mouse(t, button, 2, x, y, mods, out);
}

ULONG Vt_IdcmpFor(ULONG base, ULONG moveFlag, const vt_term *t, BOOL keepMove)
{
    if (keepMove || (t && (vt_modes(t) & (VT_MODE_MOUSE_BUTTON | VT_MODE_MOUSE_ANY))))
        return base | moveFlag;
    return base & ~moveFlag;
}

void Vt_TitleText(const char *utf8, const char *own, char *out, size_t max)
{
    const unsigned char *s = (const unsigned char *)utf8;
    size_t n = 0;

    if (max == 0)
        return;
    while (*s && n + 1 < max) {
        unsigned long cp;
        int need;

        if (*s < 0x80) { cp = *s; need = 0; }
        else if ((*s & 0xE0) == 0xC0) { cp = *s & 0x1F; need = 1; }
        else if ((*s & 0xF0) == 0xE0) { cp = *s & 0x0F; need = 2; }
        else if ((*s & 0xF8) == 0xF0) { cp = *s & 0x07; need = 3; }
        else { cp = 0xFFFD; need = 0; }
        s++;
        while (need > 0 && (*s & 0xC0) == 0x80) {
            cp = (cp << 6) | (*s & 0x3F);
            s++;
            need--;
        }
        if (need > 0)
            cp = 0xFFFD;
        if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
            continue;
        out[n++] = cp <= 0xFF ? (char)cp : '?';
    }
    out[n] = 0;
    if (n == 0) {
        strncpy(out, own, max - 1);
        out[max - 1] = 0;
    }
}

size_t Vt_PasteBracket(const vt_term *t, char *buf, size_t len, int utf8)
{
    UBYTE pre[8], post[8];
    int np = vt_encode_paste(t, 0, pre);
    int nq = vt_encode_paste(t, 1, post);
    size_t i, n = 0;

    if (np == 0)
        return len;
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)buf[i];

        if (c == 0x1B)
            continue;                   /* ESC [ 2 0 1 ~ would end the paste */
        if (!utf8 && c == 0x9B)
            continue;                   /* 8-bit CSI, same effect on a C1 host */
        if (utf8 && c == 0xC2 && i + 1 < len && (unsigned char)buf[i + 1] == 0x9B) {
            i++;                        /* U+009B, the UTF-8 form of 8-bit CSI */
            continue;
        }
        buf[n++] = buf[i];
    }
    memmove(buf + np, buf, n);
    memcpy(buf, pre, (size_t)np);
    memcpy(buf + np + n, post, (size_t)nq);
    return (size_t)np + n + (size_t)nq;
}
