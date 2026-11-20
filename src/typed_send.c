/* src/typed_send.c -- which bytes DCTelnet sends for text it types for the user. */
#include <string.h>
#include "typed_send.h"
#include "petscii_keymap.h"
#include "rexxcmd.h"

void TypedSend_Text(int petscii, const char *str, long len, TypedSendSink sink, void *ctx)
{
    char chunk[128];
    long n;

    if (len == -1) len = (long)strlen(str);
    if (!petscii)
    {
        sink(ctx, str, len);
        return;
    }
    while (len > 0)
    {
        n = len < (long)sizeof(chunk) ? len : (long)sizeof(chunk);
        memcpy(chunk, str, (size_t)n);
        petscii_translate_text(chunk, (unsigned long)n);
        sink(ctx, chunk, n);
        str += n;
        len -= n;
    }
}

void TypedSend_Rexx(int petscii, const char *arg, int sendln, int crlf,
                    char *scratch, size_t scratch_size, TypedSendSink sink, void *ctx)
{
    size_t n = RexxCmd_Unescape(arg, scratch, scratch_size - 2);

    if (sendln)
    {
        scratch[n++] = '\r';
        if (crlf) scratch[n++] = '\n';
    }
    TypedSend_Text(petscii, scratch, (long)n, sink, ctx);
}

void TypedSend_Credential(int petscii, const char *str, TypedSendSink sink, void *ctx)
{
    TypedSend_Text(petscii, str, -1, sink, ctx);
    sink(ctx, "\r", 1);
}
