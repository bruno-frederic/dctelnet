/* src/rlogin.c -- the rlogin protocol (RFC 1282). */
#include "rlogin.h"

static size_t put(char *out, size_t n, size_t max, const char *s, BOOL *ok)
{
    size_t len = strlen(s);

    if (n + len + 1 > max) { *ok = FALSE; return n; }
    memcpy(out + n, s, len);
    out[n + len] = 0;
    return n + len + 1;
}

size_t Rlogin_Handshake(const char *user, const char *password, const char *termType,
                        const char *speed, char *out, size_t max)
{
    const char *server = user[0] ? user : "guest";
    const char *client = password[0] ? password : server;
    char term[64];
    size_t n = 0, t;
    BOOL ok = TRUE;

    t = strlen(termType);
    if (t > sizeof(term) - 16) t = sizeof(term) - 16;
    memcpy(term, termType, t);
    term[t] = '/';
    strncpy(term + t + 1, speed, sizeof(term) - t - 2);
    term[sizeof(term) - 1] = 0;

    if (max < 1) return 0;
    out[n++] = 0;
    n = put(out, n, max, client, &ok);
    n = put(out, n, max, server, &ok);
    n = put(out, n, max, term, &ok);
    return ok ? n : 0;
}
