/* tests/test_typed_send.c -- which bytes the typed-send entry points emit.
 * Drives TypedSend_*, the functions DCTelnet.c's login macro, ARexx SEND/
 * SENDLN and Send User Name / Send Password call. The sink counts calls, so a
 * passing assert proves the code ran. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "typed_send.h"

struct Rec { char buf[1024]; long len; int calls; };

static void rec_sink(void *ctx, const char *buf, long len)
{
    struct Rec *r = ctx;

    assert(r->len + len <= (long)sizeof(r->buf));
    memcpy(r->buf + r->len, buf, (size_t)len);
    r->len += len;
    r->calls++;
}

static void rec_init(struct Rec *r) { memset(r, 0, sizeof(*r)); }

static void test_petscii_text_case(void)
{
    struct Rec r;
    char src[] = "Pass1";

    rec_init(&r);
    TypedSend_Text(1, src, -1, rec_sink, &r);
    assert(r.calls == 1);
    assert(r.len == 5);
    assert((unsigned char)r.buf[0] == 0xD0);            /* 'P' */
    assert(r.buf[1] == 0x41 && r.buf[2] == 0x53 && r.buf[3] == 0x53); /* a s s */
    assert(r.buf[4] == '1');
    assert(strcmp(src, "Pass1") == 0);                  /* caller's text unchanged */
}

static void test_ansi_text_unchanged(void)
{
    struct Rec r;

    rec_init(&r);
    TypedSend_Text(0, "Pass1", -1, rec_sink, &r);
    assert(r.calls == 1 && r.len == 5 && memcmp(r.buf, "Pass1", 5) == 0);
    rec_init(&r);
    TypedSend_Text(0, "abcdef", 3, rec_sink, &r);      /* explicit length honoured */
    assert(r.len == 3 && memcmp(r.buf, "abc", 3) == 0);
}

static void test_long_text_crosses_chunks(void)
{
    struct Rec r;
    char src[301], copy[301];
    int i;

    for (i = 0; i < 300; i++) src[i] = (i % 2) ? 'a' : 'A';
    src[300] = 0;
    memcpy(copy, src, sizeof(src));
    rec_init(&r);
    TypedSend_Text(1, src, 300, rec_sink, &r);
    assert(r.len == 300);
    assert(r.calls == 3);                               /* 128 + 128 + 44 */
    for (i = 0; i < 300; i++)
        assert((unsigned char)r.buf[i] == ((i % 2) ? 0x41 : 0xC1));
    assert(memcmp(src, copy, sizeof(src)) == 0);
    rec_init(&r);
    TypedSend_Text(0, src, 300, rec_sink, &r);          /* ANSI: one raw call */
    assert(r.calls == 1 && r.len == 300 && memcmp(r.buf, src, 300) == 0);
}

static void test_rexx_send_and_sendln(void)
{
    struct Rec r;
    char scratch[64];

    rec_init(&r);
    TypedSend_Rexx(1, "Pass1", 0, 0, scratch, sizeof(scratch), rec_sink, &r);
    assert(r.calls >= 1 && r.len == 5 && (unsigned char)r.buf[0] == 0xD0 && r.buf[1] == 0x41);

    rec_init(&r);
    TypedSend_Rexx(1, "Pass1", 1, 0, scratch, sizeof(scratch), rec_sink, &r);
    assert(r.len == 6 && (unsigned char)r.buf[0] == 0xD0 && r.buf[5] == '\r');

    rec_init(&r);
    TypedSend_Rexx(1, "Pass1", 1, 1, scratch, sizeof(scratch), rec_sink, &r);
    assert(r.len == 7 && r.buf[5] == '\r' && r.buf[6] == '\n');

    rec_init(&r);                                       /* SEND never adds a line end */
    TypedSend_Rexx(1, "a", 0, 1, scratch, sizeof(scratch), rec_sink, &r);
    assert(r.len == 1 && r.buf[0] == 0x41);

    rec_init(&r);                                       /* ANSI: escapes resolved, bytes raw */
    TypedSend_Rexx(0, "Pass1\\r", 1, 1, scratch, sizeof(scratch), rec_sink, &r);
    assert(r.len == 8 && memcmp(r.buf, "Pass1\r\r\n", 8) == 0);
}

static void test_credentials(void)
{
    struct Rec r;

    rec_init(&r);
    TypedSend_Credential(1, "Pass1", rec_sink, &r);
    assert(r.calls == 2 && r.len == 6);
    assert((unsigned char)r.buf[0] == 0xD0 && r.buf[1] == 0x41 && r.buf[5] == '\r');
    rec_init(&r);
    TypedSend_Credential(0, "Pass1", rec_sink, &r);
    assert(r.len == 6 && memcmp(r.buf, "Pass1\r", 6) == 0);
}

int main(void)
{
    test_petscii_text_case();
    test_ansi_text_unchanged();
    test_long_text_crosses_chunks();
    test_rexx_send_and_sendln();
    test_credentials();
    printf("test_typed_send: OK\n");
    return 0;
}
