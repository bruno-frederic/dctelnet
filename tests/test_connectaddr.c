/* test/test_connectaddr.c -- the Connect requester's "[user@]host[:port]". */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "connectaddr.h"

static int parse(const char *in, char *user, char *host, UWORD *port)
{
    return ConnectAddr_Parse(in, user, 42, host, 52, port, 23);
}

static void test_connect_requester_accepts_user_at_host(void)
{
    char u[42], h[52];
    UWORD p = 0;

    assert(parse("alice@bbs.example.org", u, h, &p) == CONNECTADDR_OK);
    assert(strcmp(u, "alice") == 0 && strcmp(h, "bbs.example.org") == 0 && p == 23);
}

static void test_connect_requester_accepts_host_port(void)
{
    char u[42], h[52];
    UWORD p = 0;

    assert(parse("bbs.example.org:2323", u, h, &p) == CONNECTADDR_OK);
    assert(u[0] == '\0' && strcmp(h, "bbs.example.org") == 0 && p == 2323);
    assert(parse("bbs.example.org,2323", u, h, &p) == CONNECTADDR_OK && p == 2323);
}

static void test_connect_requester_accepts_user_at_host_port(void)
{
    char u[42], h[52];
    UWORD p = 0;

    assert(parse("bob@bbs.example.org:65535", u, h, &p) == CONNECTADDR_OK);
    assert(strcmp(u, "bob") == 0 && strcmp(h, "bbs.example.org") == 0 && p == 65535);
}

static void test_connect_requester_keeps_plain_host_and_ipv6(void)
{
    char u[42], h[52];
    UWORD p = 0;

    assert(parse("bbs.example.org", u, h, &p) == CONNECTADDR_OK);
    assert(u[0] == '\0' && strcmp(h, "bbs.example.org") == 0 && p == 23);
    assert(parse("fe80::1", u, h, &p) == CONNECTADDR_OK);
    assert(strcmp(h, "fe80::1") == 0 && p == 23);
}

static void test_connect_requester_rejects_invalid_port(void)
{
    char u[42], h[52];
    UWORD p = 7;

    assert(parse("host:0", u, h, &p) == CONNECTADDR_BAD_PORT);
    assert(parse("host:65536", u, h, &p) == CONNECTADDR_BAD_PORT);
    assert(parse("host:99999999999999999999", u, h, &p) == CONNECTADDR_BAD_PORT);
    assert(parse("host:abc", u, h, &p) == CONNECTADDR_BAD_PORT);
    assert(parse("host:", u, h, &p) == CONNECTADDR_BAD_PORT);
    assert(p == 7);
}

static void test_connect_requester_rejects_empty_host(void)
{
    char u[42], h[52];
    UWORD p = 0;

    assert(parse("", u, h, &p) == CONNECTADDR_EMPTY_HOST);
    assert(parse("user@", u, h, &p) == CONNECTADDR_EMPTY_HOST);
    assert(parse("user@:23", u, h, &p) == CONNECTADDR_EMPTY_HOST);
    assert(parse(":23", u, h, &p) == CONNECTADDR_EMPTY_HOST);
}

static void test_connect_requester_rejects_overlong_parts(void)
{
    char u[42], h[52], in[200];
    UWORD p = 0;

    memset(in, 'a', 41); in[41] = '\0';
    strcat(in, "@host");
    assert(parse(in, u, h, &p) == CONNECTADDR_OK && strlen(u) == 41);
    memset(in, 'a', 42); in[42] = '\0';
    strcat(in, "@host");
    assert(parse(in, u, h, &p) == CONNECTADDR_TOO_LONG);

    memset(in, 'h', 52); in[52] = '\0';
    assert(parse(in, u, h, &p) == CONNECTADDR_TOO_LONG);
    memset(in, 'h', 51); in[51] = '\0';
    assert(parse(in, u, h, &p) == CONNECTADDR_OK && strlen(h) == 51);
}

int main(void)
{
    test_connect_requester_accepts_user_at_host();
    test_connect_requester_accepts_host_port();
    test_connect_requester_accepts_user_at_host_port();
    test_connect_requester_keeps_plain_host_and_ipv6();
    test_connect_requester_rejects_invalid_port();
    test_connect_requester_rejects_empty_host();
    test_connect_requester_rejects_overlong_parts();
    puts("test_connectaddr: all passed");
    return 0;
}
