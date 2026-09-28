/* test/test_rlogin.c -- the rlogin protocol. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rlogin.h"

static void test_handshake_with_a_password_logs_in_like_synchronet(void)
{
    char out[64];
    size_t n = Rlogin_Handshake("spot", "secret", "ansi-bbs", "38400", out, sizeof(out));
    static const char want[] = "\0secret\0spot\0ansi-bbs/38400";

    assert(n == sizeof(want));                  /* the terminating NUL counts */
    assert(memcmp(out, want, n) == 0);
}

static void test_handshake_without_a_password_or_a_name(void)
{
    char out[64];
    static const char want[] = "\0spot\0spot\0ANSI/9600";
    static const char guest[] = "\0guest\0guest\0ANSI/9600";

    assert(Rlogin_Handshake("spot", "", "ANSI", "9600", out, sizeof(out)) == sizeof(want));
    assert(memcmp(out, want, sizeof(want)) == 0);
    assert(Rlogin_Handshake("", "", "ANSI", "9600", out, sizeof(out)) == sizeof(guest));
    assert(memcmp(out, guest, sizeof(guest)) == 0);
    assert(Rlogin_Handshake("spot", "", "ANSI", "9600", out, 10) == 0);    /* too small */
}

int main(void)
{
    test_handshake_with_a_password_logs_in_like_synchronet();
    test_handshake_without_a_password_or_a_name();
    printf("rlogin: all assertions passed\n");
    return 0;
}
