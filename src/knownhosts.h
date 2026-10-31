/* src/knownhosts.h -- the SSH host keys DCTelnet has seen (PROGDIR:KnownHosts).
 * Pure text handling, unit-tested on the host.
 *
 * One line per server: "host port SHA256:fingerprint". A server whose key
 * changed may be an impostor, so the caller warns before replacing it. */
#ifndef KNOWNHOSTS_H
#define KNOWNHOSTS_H

#include <exec/types.h>
#include <string.h>

enum { KNOWN_NEW, KNOWN_SAME, KNOWN_CHANGED };

/* How the file (text, len bytes) knows host:port with this fingerprint. */
int KnownHosts_Check(const char *text, size_t len, const char *host, UWORD port, const char *fp);

/* The file with host:port's line set to fp (replaced, or appended), in out;
 * returns its length, 0 if it does not fit. */
size_t KnownHosts_Set(const char *text, size_t len, const char *host, UWORD port, const char *fp,
                      char *out, size_t max);

#endif /* KNOWNHOSTS_H */
