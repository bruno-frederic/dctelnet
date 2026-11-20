/* src/typed_send.h -- which bytes DCTelnet sends for text it types for the user.
 *
 * Pure: no AmigaOS, no globals. DCTelnet.c passes SendMisc as the sink and
 * its PETSCII_SESSION() as the flag, so the byte decisions are host-testable.
 * In a PETSCII session text goes out as the C64 keyboard sends it
 * (petscii_translate_text); otherwise unchanged. The caller's text is never
 * modified. The sink may be called several times per request. */
#ifndef TYPED_SEND_H
#define TYPED_SEND_H

#include <stddef.h>

typedef void (*TypedSendSink)(void *ctx, const char *buf, long len);

/* Typed text and login-macro segments. len -1 = strlen(str). */
void TypedSend_Text(int petscii, const char *str, long len, TypedSendSink sink, void *ctx);

/* ARexx SEND / SENDLN: arg is unescaped (RexxCmd_Unescape) into scratch
 * (scratch_size at least 3); SENDLN appends '\r' and, when crlf, '\n'. */
void TypedSend_Rexx(int petscii, const char *arg, int sendln, int crlf,
                    char *scratch, size_t scratch_size, TypedSendSink sink, void *ctx);

/* Send User Name / Send Password: the text, then a raw Return. */
void TypedSend_Credential(int petscii, const char *str, TypedSendSink sink, void *ctx);

#endif /* TYPED_SEND_H */
