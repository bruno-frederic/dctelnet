/* src/iconpens.c -- tool bar icons that carry their own colours. */
#include "iconpens.h"

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int IconPens_Parse(const char *value, ULONG rgb[ICONPENS_MAX])
{
    int n = 0;

    while (*value)
    {
        ULONG c = 0;
        int i;

        if (n == ICONPENS_MAX)
            return 0;
        for (i = 0; i < 6; i++)
        {
            int d = hexval(value[i]);

            if (d < 0)
                return 0;
            c = (c << 4) | (ULONG)d;
        }
        rgb[n++] = c;
        value += 6;
        if (*value == ',')
            value++;
        else if (*value)
            return 0;
    }
    return n;
}

/* Distance as the eye weighs it: brightness first, then hue. In plain RGB
 * distance the dark grey of an antialiased edge (717171) is nearer the
 * Workbench's blue (6688BB) than its grey, and edges came out blue. */
static void luma_chroma(ULONG c, LONG *y, LONG *cr, LONG *cb)
{
    LONG r = (LONG)((c >> 16) & 0xFF), g = (LONG)((c >> 8) & 0xFF), b = (LONG)(c & 0xFF);

    *y = (299 * r + 587 * g + 114 * b) / 1000;
    *cr = r - *y;
    *cb = b - *y;
}

ULONG IconPens_Distance(ULONG a, ULONG b)
{
    LONG y, cr, cb, py, pcr, pcb;

    luma_chroma(a, &y, &cr, &cb);
    luma_chroma(b, &py, &pcr, &pcb);
    return (ULONG)((y - py) * (y - py) + 4 * ((cr - pcr) * (cr - pcr) + (cb - pcb) * (cb - pcb)));
}

int IconPens_Nearest(ULONG rgb, const ULONG *palette, int n)
{
    int i, best = 0;
    ULONG bestDist = 0xFFFFFFFFUL;

    for (i = 0; i < n; i++)
    {
        ULONG d = IconPens_Distance(rgb, palette[i]);

        if (d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

void IconPens_TerminalMap(const ULONG *rgb, int n, const ULONG termRgb[16],
                          const UBYTE termPen[16], UBYTE bgPen, UBYTE *map)
{
    int k, i;

    for (k = 0; k < n; k++)
    {
        ULONG best = 0xFFFFFFFFUL;

        if (k == 0)
        {
            map[0] = bgPen;
            continue;
        }
        map[k] = bgPen;
        for (i = 0; i < 16; i++)
        {
            ULONG d;

            if (termPen[i] == bgPen)
                continue;
            d = IconPens_Distance(rgb[k], termRgb[i]);
            if (d < best)
            {
                best = d;
                map[k] = termPen[i];
            }
        }
    }
}

void IconPens_ExactMap(int n, const UBYTE *exact, const BOOL *have, UBYTE bgPen, UBYTE *map)
{
    int k;

    for (k = 1; k < n; k++)
        if (have[k] && exact[k] != bgPen)
            map[k] = exact[k];
}

static LONG channel(ULONG c, int shift)
{
    return (LONG)((c >> shift) & 0xFF);
}

ULONG IconPens_Translate(ULONG rgb, const ULONG *from, const ULONG *to, int n)
{
    int a, b, shift, bestA = 0, bestB = 0;
    LONG num = 0, den = 1;
    ULONG bestDist = 0xFFFFFFFFUL, bestLen = 0, out = 0;

    for (a = 0; a < n; a++)
        if (from[a] == rgb)
            return to[a];
    for (a = 0; a < n; a++)
        for (b = a + 1; b < n; b++)
        {
            LONG dot = 0, len = 0, t;
            ULONG dist = 0;

            for (shift = 0; shift <= 16; shift += 8)
            {
                LONG d = channel(from[b], shift) - channel(from[a], shift);

                dot += (channel(rgb, shift) - channel(from[a], shift)) * d;
                len += d * d;
            }
            if (len == 0)
                continue;
            if (dot < 0) dot = 0;
            if (dot > len) dot = len;
            for (shift = 0; shift <= 16; shift += 8)
            {
                t = channel(rgb, shift) - (channel(from[a], shift) +
                    (channel(from[b], shift) - channel(from[a], shift)) * dot / len);
                dist += (ULONG)(t * t);
            }
            if (dist < bestDist || (dist == bestDist && (ULONG)len < bestLen))
            {
                bestDist = dist;
                bestLen = (ULONG)len;
                bestA = a;
                bestB = b;
                num = dot;
                den = len;
            }
        }
    for (shift = 0; shift <= 16; shift += 8)
    {
        LONG ca = channel(to[bestA], shift), d = channel(to[bestB], shift) - ca;

        out |= (ULONG)(ca + (d * num + (d < 0 ? -den : den) / 2) / den) << shift;
    }
    return out;
}

UWORD IconPens_Depth(UBYTE maxPen)
{
    UWORD depth = 1;

    while (depth < 8 && (maxPen >> depth))
        depth++;
    return depth;
}

ULONG IconPens_PlaneSize(UWORD width, UWORD height)
{
    return (ULONG)((width + 15) >> 4) * 2 * height;
}

ULONG IconPens_RemapCount = 0;

int IconPens_IsIdentity(const UBYTE *map, int n, UWORD srcDepth, UWORD dstDepth)
{
    int i;

    if (srcDepth != dstDepth || (1 << srcDepth) > n)
        return 0;
    for (i = 0; i < n; i++)
        if (map[i] != i)
            return 0;
    return 1;
}

/* A word of a plane holds 16 pixels, so the pens of 16 pixels are found at once:
 * the mask of the pixels of pen p is the AND of each source plane's word (or its
 * complement), and the destination plane d gets the masks of all pens whose
 * mapped pen has bit d. Planes are walked by pointer: on a 68000 a multiply
 * costs more than the whole mask of a pen. */
void IconPens_Remap(const UWORD *src, UWORD width, UWORD height, UWORD srcDepth,
                    UBYTE pick, UBYTE onoff, const UBYTE *map, int n,
                    UWORD *dst, UWORD dstDepth)
{
    ULONG words = (ULONG)((width + 15) >> 4) * height;   /* one plane, in words */
    const UWORD *sp[8];       /* next word of each stored source plane */
    UWORD *dp[8];             /* next word of each destination plane */
    UBYTE out[256];
    UWORD pens = (UWORD)(1 << srcDepth), p;
    UWORD plane, stored = 0;
    ULONG w;

    IconPens_RemapCount++;
    for (plane = 0; plane < srcDepth; plane++)
        if (pick & (1 << plane))
        {
            sp[stored] = src + stored * words;      /* once per image, not per word */
            stored++;
        }
    for (plane = 0; plane < dstDepth; plane++)
        dp[plane] = dst + plane * words;
    for (p = 0; p < pens; p++)
        out[p] = map[p < n ? p : 0];
    for (w = 0; w < words; w++)
    {
        UWORD in[8], res[8];

        stored = 0;
        for (plane = 0; plane < srcDepth; plane++)
        {
            if (pick & (1 << plane))
            {
                in[plane] = *sp[stored]++;
                stored++;
            }
            else
                in[plane] = (onoff & (1 << plane)) ? 0xFFFF : 0;
        }
        for (plane = 0; plane < dstDepth; plane++)
            res[plane] = 0;
        for (p = 0; p < pens; p++)
        {
            UWORD mask = 0xFFFF, o = out[p];

            for (plane = 0; plane < srcDepth && mask; plane++)
                mask &= (p & (1 << plane)) ? in[plane] : (UWORD)~in[plane];
            if (!mask)
                continue;
            for (plane = 0; plane < dstDepth; plane++)
                if (o & (1 << plane))
                    res[plane] |= mask;
        }
        for (plane = 0; plane < dstDepth; plane++)
            *dp[plane]++ = res[plane];
    }
}
