/* test/test_knownhosts.c -- the SSH host key file. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "knownhosts.h"

static const char file[] =
    "bbs.uprough.net 31337 SHA256:UC6ixajz/XWu3qVldU+lz2jgKpwSYzrE6VF1tOXKh2c\n"
    "vert.synchro.net 22 SHA256:aaaa\r\n"
    "vert.synchro.net 2222 SHA256:bbbb\n";

static void test_a_known_key_is_the_same(void)
{
    assert(KnownHosts_Check(file, sizeof(file) - 1, "bbs.uprough.net", 31337,
                            "SHA256:UC6ixajz/XWu3qVldU+lz2jgKpwSYzrE6VF1tOXKh2c") == KNOWN_SAME);
    assert(KnownHosts_Check(file, sizeof(file) - 1, "BBS.UpRough.NET", 31337,
                            "SHA256:UC6ixajz/XWu3qVldU+lz2jgKpwSYzrE6VF1tOXKh2c") == KNOWN_SAME);
    /* A CR LF file (edited on a PC) still matches. */
    assert(KnownHosts_Check(file, sizeof(file) - 1, "vert.synchro.net", 22, "SHA256:aaaa") == KNOWN_SAME);
}

/* The same host on another port is another server; a changed key is flagged. */
static void test_port_and_key_are_part_of_the_match(void)
{
    assert(KnownHosts_Check(file, sizeof(file) - 1, "vert.synchro.net", 2222, "SHA256:bbbb") == KNOWN_SAME);
    assert(KnownHosts_Check(file, sizeof(file) - 1, "vert.synchro.net", 23, "SHA256:aaaa") == KNOWN_NEW);
    assert(KnownHosts_Check(file, sizeof(file) - 1, "vert.synchro.net", 22, "SHA256:aaab") == KNOWN_CHANGED);
    assert(KnownHosts_Check(file, sizeof(file) - 1, "vert.synchro.ne", 22, "SHA256:aaaa") == KNOWN_NEW);
    assert(KnownHosts_Check("", 0, "x", 22, "SHA256:aaaa") == KNOWN_NEW);
}

static void test_set_appends_a_new_host(void)
{
    char out[512];
    const char noNl[] = "a 22 SHA256:x";
    size_t n = KnownHosts_Set(file, sizeof(file) - 1, "xibalba.l33t.codes", 44510, "SHA256:cccc", out, sizeof(out));
    assert(n == sizeof(file) - 1 + strlen("xibalba.l33t.codes 44510 SHA256:cccc\n"));
    assert(!memcmp(out, file, sizeof(file) - 1));
    assert(!memcmp(out + sizeof(file) - 1, "xibalba.l33t.codes 44510 SHA256:cccc\n", n - (sizeof(file) - 1)));
    /* A file without a last newline gets one before the new line. */
    n = KnownHosts_Set(noNl, sizeof(noNl) - 1, "b", 1, "SHA256:y", out, sizeof(out));
    out[n] = 0;
    assert(!strcmp(out, "a 22 SHA256:x\nb 1 SHA256:y\n"));
}

static void test_set_replaces_a_changed_key_in_place(void)
{
    char out[512];
    size_t n = KnownHosts_Set(file, sizeof(file) - 1, "vert.synchro.net", 22, "SHA256:dddd", out, sizeof(out));
    out[n] = 0;
    assert(!strcmp(out, "bbs.uprough.net 31337 SHA256:UC6ixajz/XWu3qVldU+lz2jgKpwSYzrE6VF1tOXKh2c\n"
                        "vert.synchro.net 22 SHA256:dddd\n"
                        "vert.synchro.net 2222 SHA256:bbbb\n"));
    assert(KnownHosts_Set(file, sizeof(file) - 1, "vert.synchro.net", 22, "SHA256:dddd", out, 20) == 0);
}

int main(void)
{
    test_a_known_key_is_the_same();
    test_port_and_key_are_part_of_the_match();
    test_set_appends_a_new_host();
    test_set_replaces_a_changed_key_in_place();
    printf("knownhosts: all assertions passed\n");
    return 0;
}
