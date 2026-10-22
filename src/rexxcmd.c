/* src/rexxcmd.c -- DCTelnet's ARexx commands. */
#include "rexxcmd.h"

static const struct { const char *name; int verb; int args; } verbs[] =
{
    { "CONNECT", REXX_CONNECT, 1 }, { "DISCONNECT", REXX_DISCONNECT, 0 },
    { "SEND", REXX_SEND, 1 },       { "SENDLN", REXX_SENDLN, 1 },
    { "WAITFOR", REXX_WAITFOR, 1 }, { "CAPTURE", REXX_CAPTURE, 1 },
    { "GETSTATUS", REXX_GETSTATUS, 0 }, { "QUIT", REXX_QUIT, 0 }
};

/* The next word of *p into out (quotes kept together); FALSE at the end. */
static BOOL word(const char **p, char *out, size_t max)
{
    const char *s = *p;
    size_t n = 0;

    while (*s == ' ' || *s == '\t') s++;
    if (!*s) { *p = s; out[0] = 0; return FALSE; }
    if (*s == '"')
    {
        s++;
        while (*s && *s != '"') { if (n + 1 < max) out[n++] = *s; s++; }
        if (*s == '"') s++;
    }
    else
        while (*s && *s != ' ' && *s != '\t') { if (n + 1 < max) out[n++] = *s; s++; }
    out[n] = 0;
    *p = s;
    return TRUE;
}

static BOOL same_word(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
    {
        char x = (char)(*a >= 'a' && *a <= 'z' ? *a - 32 : *a);

        if (x != *b) return FALSE;
    }
    return *a == *b;
}

BOOL RexxCmd_Parse(const char *line, struct RexxCmd *out)
{
    char verb[16];
    const char *p = line;
    size_t i;

    out->verb = REXX_UNKNOWN;
    out->arg1[0] = out->arg2[0] = 0;
    if (!word(&p, verb, sizeof(verb)))
        return FALSE;
    for (i = 0; i < sizeof(verbs) / sizeof(verbs[0]); i++)
        if (same_word(verb, verbs[i].name))
        {
            BOOL has1 = word(&p, out->arg1, sizeof(out->arg1));

            word(&p, out->arg2, sizeof(out->arg2));
            if (verbs[i].args && !has1)
                return FALSE;
            out->verb = verbs[i].verb;
            return TRUE;
        }
    return FALSE;
}

size_t RexxCmd_Unescape(const char *in, char *out, size_t max)
{
    size_t n = 0;

    for (; *in && n < max; in++)
    {
        if (*in == '\\' && in[1])
        {
            in++;
            switch (*in)
            {
            case 'r': out[n++] = '\r'; break;
            case 'n': out[n++] = '\n'; break;
            case 'e': out[n++] = 0x1B; break;
            default:  out[n++] = *in;  break;
            }
        }
        else
            out[n++] = *in;
    }
    return n;
}
