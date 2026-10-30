/* src/knownhosts.c -- the SSH host keys DCTelnet has seen (see knownhosts.h). */
#include "knownhosts.h"

/* Host names compare without case (DNS). */
static BOOL SameHost(const char *a, size_t n, const char *b)
{
    size_t i;
    if (strlen(b) != n) return FALSE;
    for (i = 0; i < n; i++)
    {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
        if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
        if (x != y) return FALSE;
    }
    return TRUE;
}

/* The line at text[at..end): is it host:port? *fp, *fpLen get its fingerprint. */
static BOOL LineIs(const char *line, size_t n, const char *host, UWORD port, const char **fp, size_t *fpLen)
{
    size_t i = 0, h;
    ULONG p = 0;
    while (i < n && line[i] != ' ') i++;
    h = i;
    if (i >= n || !SameHost(line, h, host)) return FALSE;
    i++;
    if (i >= n || line[i] < '0' || line[i] > '9') return FALSE;
    while (i < n && line[i] >= '0' && line[i] <= '9') p = p * 10 + (ULONG)(line[i++] - '0');
    if (p != port || i >= n || line[i] != ' ') return FALSE;
    i++;
    *fp = line + i;
    *fpLen = n - i;
    while (*fpLen && ((*fp)[*fpLen - 1] == '\r' || (*fp)[*fpLen - 1] == ' ')) (*fpLen)--;
    return TRUE;
}

/* Finds host:port's line: its start and length (with the newline), or FALSE. */
static BOOL Find(const char *text, size_t len, const char *host, UWORD port,
                 size_t *start, size_t *lineLen, const char **fp, size_t *fpLen)
{
    size_t at = 0;
    while (at < len)
    {
        size_t end = at;
        while (end < len && text[end] != '\n') end++;
        if (LineIs(text + at, end - at, host, port, fp, fpLen))
        {
            *start = at;
            *lineLen = end - at + (end < len);
            return TRUE;
        }
        at = end + 1;
    }
    return FALSE;
}

int KnownHosts_Check(const char *text, size_t len, const char *host, UWORD port, const char *fp)
{
    size_t start, lineLen, fpLen;
    const char *known;
    if (!Find(text, len, host, port, &start, &lineLen, &known, &fpLen)) return KNOWN_NEW;
    return fpLen == strlen(fp) && !memcmp(known, fp, fpLen) ? KNOWN_SAME : KNOWN_CHANGED;
}

size_t KnownHosts_Set(const char *text, size_t len, const char *host, UWORD port, const char *fp,
                      char *out, size_t max)
{
    char line[320], digits[6];
    size_t start, lineLen, fpLen, n = 0, total;
    const char *known;
    int d = 0;
    UWORD p = port;

    do { digits[d++] = (char)('0' + p % 10); p /= 10; } while (p);
    if (strlen(host) + strlen(fp) + 8 > sizeof(line)) return 0;
    memcpy(line, host, strlen(host)); n = strlen(host);
    line[n++] = ' ';
    while (d) line[n++] = digits[--d];
    line[n++] = ' ';
    memcpy(line + n, fp, strlen(fp)); n += strlen(fp);
    line[n++] = '\n';

    if (!Find(text, len, host, port, &start, &lineLen, &known, &fpLen))
    {
        BOOL needNl = len && text[len - 1] != '\n';
        start = len; lineLen = 0;
        total = len + needNl + n;
        if (total > max) return 0;
        memcpy(out, text, len);
        if (needNl) out[len++] = '\n';
        memcpy(out + len, line, n);
        return total;
    }
    total = len - lineLen + n;
    if (total > max) return 0;
    memmove(out, text, start);
    memcpy(out + start, line, n);
    memcpy(out + start + n, text + start + lineLen, len - start - lineLen);
    return total;
}
