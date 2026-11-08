/* src/booklist.h -- SyncTERM's phone book (syncterm.lst). Pure, unit-tested
 * on the host.
 *
 * An INI file: one [BBS name] section per entry, then key=value lines
 * (indented with tabs): Address, Port, ConnectionType, Username, Password
 * and more that DCTelnet has no use for. */
#ifndef BOOKLIST_H
#define BOOKLIST_H

#include <exec/types.h>
#include <string.h>

enum { BBS_TELNET, BBS_RLOGIN, BBS_RAW, BBS_SSH, BBS_OTHER };   /* OTHER: modem, serial, TLS ... */

struct ImportedBbs
{
    char  name[32];
    char  host[52];
    char  user[42];
    char  pass[42];
    UWORD port;
    UWORD type;
};

/* The next entry with an address, from text[*pos] on; *pos moves past it.
 * FALSE when there is none. Port 0 in the file: the type's usual port. */
BOOL BookList_Next(const char *text, size_t len, size_t *pos, struct ImportedBbs *out);

#endif /* BOOKLIST_H */
