/* src/booklist.c -- SyncTERM's phone book (syncterm.lst). */
#include "booklist.h"

static BOOL same(const char *a, size_t n, const char *b)
{
    size_t i;

    for (i = 0; i < n && b[i]; i++)
    {
        char x = a[i], y = b[i];

        if (x >= 'a' && x <= 'z') x = (char)(x - 32);
        if (y >= 'a' && y <= 'z') y = (char)(y - 32);
        if (x != y) return FALSE;
    }
    return i == n && !b[i];
}

static void copy(char *out, size_t max, const char *s, size_t n)
{
    if (n >= max) n = max - 1;
    memcpy(out, s, n);
    out[n] = 0;
}

static UWORD type_of(const char *v, size_t n)
{
    if (same(v, n, "Telnet"))         return BBS_TELNET;
    if (same(v, n, "RLogin") || same(v, n, "RLoginReversed")) return BBS_RLOGIN;
    if (same(v, n, "Raw"))            return BBS_RAW;
    return BBS_OTHER;                               /* SSH, SSHNA, TelnetS, Modem, Serial ... */
}

BOOL BookList_Next(const char *text, size_t len, size_t *pos, struct ImportedBbs *out)
{
    size_t p = *pos;
    BOOL inEntry = FALSE;

    while (p < len)
    {
        size_t start = p, end, k, v;

        while (p < len && text[p] != '\n') p++;
        end = p;
        if (p < len) p++;
        while (start < end && (text[start] == ' ' || text[start] == '\t')) start++;
        while (end > start && (text[end - 1] == '\r' || text[end - 1] == ' ' || text[end - 1] == '\t')) end--;
        if (start == end || text[start] == ';' || text[start] == '#')
            continue;
        if (text[start] == '[')
        {
            if (inEntry && out->host[0])
            {
                *pos = start;                       /* this section is the next entry */
                goto done;
            }
            memset(out, 0, sizeof(*out));
            k = start + 1;
            while (k < end && text[k] != ']') k++;
            copy(out->name, sizeof(out->name), text + start + 1, k - start - 1);
            inEntry = TRUE;
            continue;
        }
        if (!inEntry)
            continue;
        for (k = start; k < end && text[k] != '='; k++) ;
        if (k == end)
            continue;
        v = k + 1;
        while (k > start && (text[k - 1] == ' ' || text[k - 1] == '\t')) k--;
        while (v < end && (text[v] == ' ' || text[v] == '\t')) v++;
        if (same(text + start, k - start, "Address"))        copy(out->host, sizeof(out->host), text + v, end - v);
        else if (same(text + start, k - start, "Username"))  copy(out->user, sizeof(out->user), text + v, end - v);
        else if (same(text + start, k - start, "Password"))  copy(out->pass, sizeof(out->pass), text + v, end - v);
        else if (same(text + start, k - start, "ConnectionType")) out->type = type_of(text + v, end - v);
        else if (same(text + start, k - start, "Port"))
        {
            UWORD port = 0;

            for (; v < end && text[v] >= '0' && text[v] <= '9'; v++) port = (UWORD)(port * 10 + (text[v] - '0'));
            out->port = port;
        }
    }
    *pos = len;
    if (!inEntry || !out->host[0])
        return FALSE;
done:
    if (!out->port)
        out->port = out->type == BBS_RLOGIN ? 513 : 23;
    return TRUE;
}
