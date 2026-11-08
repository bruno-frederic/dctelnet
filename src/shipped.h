/* src/shipped.h -- open the newest copy of a device or library DCTelnet
 * ships in its own drawer (Devs/, Libs/) or the system has (DEVS:, LIBS:). */
#ifndef SHIPPED_H
#define SHIPPED_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/io.h>

/* Where the last OpenNewest*() call took its copy from: "DCTelnet's
 * drawer", "the system", or "memory" (an older copy in use elsewhere). */
extern const char *ShippedFrom;

/* OpenLibrary() on the newest copy of name: DCTelnet's Libs/ or LIBS:,
 * whichever file's $VER is newer (DCTelnet's on a tie). A name with a path
 * is opened as it is. */
struct Library *OpenNewestLibrary(CONST_STRPTR name, ULONG version);

/* OpenDevice() on the newest copy of name, DCTelnet's Devs/ or DEVS:. */
BYTE OpenNewestDevice(STRPTR name, ULONG unit, struct IORequest *io, ULONG flags);

#endif /* SHIPPED_H */
