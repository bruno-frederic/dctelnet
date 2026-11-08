/* test/test_rexx.c -- ARexx commands and waiting for a text. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rexxcmd.h"
#include "waitfor.h"

static void test_parses_commands(void)
{
    struct RexxCmd c;

    assert(RexxCmd_Parse("connect bbs.example.com 6400", &c) && c.verb == REXX_CONNECT);
    assert(strcmp(c.arg1, "bbs.example.com") == 0 && strcmp(c.arg2, "6400") == 0);
    assert(RexxCmd_Parse("WAITFOR \"Password: \" 30", &c) && c.verb == REXX_WAITFOR);
    assert(strcmp(c.arg1, "Password: ") == 0 && strcmp(c.arg2, "30") == 0);
    assert(RexxCmd_Parse("SENDLN \"guest\"", &c) && c.verb == REXX_SENDLN && strcmp(c.arg1, "guest") == 0);
    assert(RexxCmd_Parse("GETSTATUS", &c) && c.verb == REXX_GETSTATUS);
    assert(!RexxCmd_Parse("CONNECT", &c));          /* host missing */
    assert(!RexxCmd_Parse("FROBNICATE x", &c));
    assert(!RexxCmd_Parse("   ", &c));
}

static void test_unescapes_send_text(void)
{
    char out[16];
    size_t n = RexxCmd_Unescape("a\\rb\\\\\\e", out, sizeof(out));

    assert(n == 5 && memcmp(out, "a\rb\\\x1b", 5) == 0);
}

static int feed(struct WaitFor *w, const char *s)
{
    return WaitFor_Feed(w, (const UBYTE *)s, strlen(s));
}

static void test_waitfor_across_pieces_and_colours(void)
{
    struct WaitFor w;

    WaitFor_Start(&w, "Password:", 9);
    assert(!feed(&w, "Welcome\r\nPass"));
    assert(WaitFor_Active(&w));
    assert(feed(&w, "\x1b[1;33mword\x1b[0m: "));           /* colour inside it */
    assert(!WaitFor_Active(&w));
}

/* "aab" in "aaab": a restart must not lose the overlap. */
static void test_waitfor_overlapping_start(void)
{
    struct WaitFor w;

    WaitFor_Start(&w, "aab", 3);
    assert(feed(&w, "aaab"));
    WaitFor_Start(&w, "abab", 4);
    assert(feed(&w, "ababab"));
    WaitFor_Start(&w, "xyz", 3);
    assert(!feed(&w, "xyxy") && feed(&w, "z"));
}

static void test_waitfor_amiga_csi(void)
{
    struct WaitFor w;

    WaitFor_Start(&w, "ok", 2);
    assert(feed(&w, "o\x9b" "31mk"));
}

int main(void)
{
    test_parses_commands();
    test_unescapes_send_text();
    test_waitfor_across_pieces_and_colours();
    test_waitfor_overlapping_start();
    test_waitfor_amiga_csi();
    printf("rexx: all assertions passed\n");
    return 0;
}
