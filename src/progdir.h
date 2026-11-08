/* src/progdir.h -- DCTelnet's own drawer before the system's.
 *
 * DCTelnet ships its devices, fonts and libraries in its own drawer
 * (Devs/, Fonts/, Libs/). Those are tried first -- PROGDIR:Devs/ibmcon.device
 * before DEVS:ibmcon.device -- so the copy that came with DCTelnet wins over
 * an older one installed in the system, and nothing needs installing.
 * Pure, unit-tested on the host.
 */
#ifndef PROGDIR_H
#define PROGDIR_H

#include <exec/types.h>
#include <string.h>

/* "PROGDIR:<drawer>/<name>" in out. FALSE (out untouched) when name already
 * carries a path (a ':' or '/': the user's own choice) or does not fit. */
BOOL ProgDir_Path(const char *drawer, const char *name, char *out, size_t max);

/* The version in a "$VER: name v.r ..." string found in buf (a file's
 * contents). FALSE when there is none. */
BOOL ProgDir_ParseVersion(const UBYTE *buf, size_t len, ULONG *version, ULONG *revision);

/* TRUE when v1.r1 is newer than v2.r2. */
BOOL ProgDir_Newer(ULONG v1, ULONG r1, ULONG v2, ULONG r2);

#endif /* PROGDIR_H */
