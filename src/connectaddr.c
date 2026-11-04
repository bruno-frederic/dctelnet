/* src/connectaddr.c -- see connectaddr.h. */
#include "connectaddr.h"

int ConnectAddr_Parse(const char *input, char *user, size_t userMax, char *host, size_t hostMax,
                      UWORD *port, UWORD defaultPort)
{
    const char *at, *hostStart, *portStart = NULL, *hostEnd;
    size_t userLen, hostLen;
    unsigned long p = defaultPort;

    at = strrchr(input, '@');
    userLen = at ? (size_t)(at - input) : 0;
    hostStart = at ? at + 1 : input;
    hostEnd = hostStart + strlen(hostStart);

    portStart = strchr(hostStart, ',');
    if (!portStart)
    {
        const char *colon = strchr(hostStart, ':');

        if (colon && strchr(colon + 1, ':') == NULL)
            portStart = colon;
    }
    if (portStart)
        hostEnd = portStart;
    hostLen = (size_t)(hostEnd - hostStart);

    if (hostLen == 0)
        return CONNECTADDR_EMPTY_HOST;
    if (userLen >= userMax || hostLen >= hostMax)
        return CONNECTADDR_TOO_LONG;

    if (portStart)
    {
        const char *d = portStart + 1;

        if (*d == '\0')
            return CONNECTADDR_BAD_PORT;
        for (p = 0; *d; d++)
        {
            if (*d < '0' || *d > '9' || (p = p * 10 + (unsigned long)(*d - '0')) > 65535)
                return CONNECTADDR_BAD_PORT;
        }
        if (p == 0)
            return CONNECTADDR_BAD_PORT;
    }

    memcpy(user, input, userLen);
    user[userLen] = '\0';
    memcpy(host, hostStart, hostLen);
    host[hostLen] = '\0';
    *port = (UWORD)p;
    return CONNECTADDR_OK;
}
