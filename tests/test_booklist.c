/* test/test_booklist.c -- SyncTERM's phone book. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "booklist.h"

static const char list[] =
    "; SyncTERM dialing directory\r\n"
    "[Level 29]\r\n"
    "\tConnectionType=Telnet\r\n"
    "\tAddress=bbs.fozztexx.com\r\n"
    "\tPort=23\r\n"
    "\tUsername=spot\r\n"
    "\tPassword=secret\r\n"
    "\tScreenMode=Current\r\n"
    "\r\n"
    "[Vertrauen]\r\n"
    "\tConnectionType=RLogin\r\n"
    "\tAddress=vert.synchro.net\r\n"
    "[Uprough]\r\n"
    "\tConnectionType=SSH\r\n"
    "\tAddress=bbs.uprough.net\r\n"
    "[Uprough 31337]\r\n"
    "\tConnectionType=SSHNA\r\n"
    "\tAddress=bbs.uprough.net\r\n"
    "\tPort=31337\r\n"
    "[A Modem Board]\r\n"
    "\tConnectionType=Modem\r\n"
    "\tAddress=555-1234\r\n"
    "[No Address]\r\n"
    "\tConnectionType=Telnet\r\n"
    "[Last]\n"
    "Address = last.example.org\n"
    "Port = 2323\n";

static void test_reads_every_entry(void)
{
    struct ImportedBbs b;
    size_t pos = 0;

    assert(BookList_Next(list, sizeof(list) - 1, &pos, &b));
    assert(strcmp(b.name, "Level 29") == 0 && strcmp(b.host, "bbs.fozztexx.com") == 0);
    assert(b.port == 23 && b.type == BBS_TELNET);
    assert(strcmp(b.user, "spot") == 0 && strcmp(b.pass, "secret") == 0);

    assert(BookList_Next(list, sizeof(list) - 1, &pos, &b));
    assert(strcmp(b.name, "Vertrauen") == 0 && b.type == BBS_RLOGIN && b.port == 513);

    /* SSH entries import now (port 22 unless given); they were skipped. */
    assert(BookList_Next(list, sizeof(list) - 1, &pos, &b));
    assert(strcmp(b.name, "Uprough") == 0 && b.type == BBS_SSH && b.port == 22);
    assert(BookList_Next(list, sizeof(list) - 1, &pos, &b));
    assert(strcmp(b.name, "Uprough 31337") == 0 && b.type == BBS_SSH && b.port == 31337);

    assert(BookList_Next(list, sizeof(list) - 1, &pos, &b));
    assert(strcmp(b.name, "A Modem Board") == 0 && b.type == BBS_OTHER);

    assert(BookList_Next(list, sizeof(list) - 1, &pos, &b));    /* [No Address] skipped */
    assert(strcmp(b.name, "Last") == 0 && strcmp(b.host, "last.example.org") == 0 && b.port == 2323);
    assert(b.type == BBS_TELNET);

    assert(!BookList_Next(list, sizeof(list) - 1, &pos, &b));
}

static void test_long_names_are_cut(void)
{
    static const char one[] = "[A very long BBS name that does not fit the Address Book]\nAddress=x.org\n";
    struct ImportedBbs b;
    size_t pos = 0;

    assert(BookList_Next(one, sizeof(one) - 1, &pos, &b));
    assert(strlen(b.name) == sizeof(b.name) - 1 && strcmp(b.host, "x.org") == 0);
}

int main(void)
{
    test_reads_every_entry();
    test_long_names_are_cut();
    printf("booklist: all assertions passed\n");
    return 0;
}
