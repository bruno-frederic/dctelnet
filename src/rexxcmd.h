/* src/rexxcmd.h -- DCTelnet's ARexx commands. Pure, unit-tested on the host.
 *
 *   CONNECT host [port]   connect (port 23 when left out)
 *   DISCONNECT
 *   SEND text             send text as it is ("\r" in it is Return)
 *   SENDLN text           send text and Return
 *   WAITFOR text [secs]   reply when the BBS has sent text (RC 5: timed out)
 *   CAPTURE file | OFF    Capture to File
 *   GETSTATUS             RESULT "CONNECTED host port" or "DISCONNECTED"
 *   QUIT
 * An argument with spaces is written in double quotes. */
#ifndef REXXCMD_H
#define REXXCMD_H

#include <exec/types.h>
#include <string.h>

enum RexxVerb
{
    REXX_UNKNOWN, REXX_CONNECT, REXX_DISCONNECT, REXX_SEND, REXX_SENDLN,
    REXX_WAITFOR, REXX_CAPTURE, REXX_GETSTATUS, REXX_QUIT
};

struct RexxCmd
{
    int  verb;
    char arg1[256];     /* host, text, file */
    char arg2[16];      /* port, seconds */
};

/* Parses a command line. FALSE for an unknown verb or a missing argument. */
BOOL RexxCmd_Parse(const char *line, struct RexxCmd *out);

/* C-style escapes of SEND text: \r Return, \n line feed, \e ESC, \\ '\'.
 * Returns the length written to out (at most max). */
size_t RexxCmd_Unescape(const char *in, char *out, size_t max);

#endif /* REXXCMD_H */
