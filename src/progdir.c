/* src/progdir.c -- DCTelnet's own drawer before the system's. */
#include "progdir.h"

BOOL ProgDir_Path(const char *drawer, const char *name, char *out, size_t max)
{
    static const char prefix[] = "PROGDIR:";
    size_t need;
    const char *c;

    for (c = name; *c; c++)
        if (*c == ':' || *c == '/')
            return FALSE;
    if (!*name)
        return FALSE;
    need = sizeof(prefix) - 1 + strlen(drawer) + 1 + strlen(name) + 1;
    if (need > max)
        return FALSE;
    strcpy(out, prefix);
    strcat(out, drawer);
    strcat(out, "/");
    strcat(out, name);
    return TRUE;
}

static const UBYTE *parse_num(const UBYTE *p, const UBYTE *end, ULONG *out)
{
    ULONG n = 0;
    const UBYTE *start = p;

    while (p < end && *p >= '0' && *p <= '9')
        n = n * 10 + (ULONG)(*p++ - '0');
    *out = n;
    return p == start ? NULL : p;
}

BOOL ProgDir_ParseVersion(const UBYTE *buf, size_t len, ULONG *version, ULONG *revision)
{
    const UBYTE *end = buf + len, *p;
    size_t i;

    for (i = 0; i + 5 <= len; i++)
    {
        if (memcmp(buf + i, "$VER:", 5) != 0)
            continue;
        p = buf + i + 5;
        while (p < end && *p == ' ') p++;
        while (p < end && *p != ' ' && *p != 0) p++;    /* the name */
        while (p < end && *p == ' ') p++;
        p = parse_num(p, end, version);
        if (!p) continue;
        *revision = 0;
        if (p < end && *p == '.')
            if (!parse_num(p + 1, end, revision)) *revision = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL ProgDir_Newer(ULONG v1, ULONG r1, ULONG v2, ULONG r2)
{
    return v1 > v2 || (v1 == v2 && r1 > r2);
}
