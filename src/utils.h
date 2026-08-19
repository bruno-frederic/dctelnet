#ifndef UTILS_H
#define UTILS_H

/**
 * @file utils.h
 * @brief General-purpose utility functions for AmigaOS 2.0+.
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <exec/types.h>
#include <string.h>

// Types


// Global variables exported


// Functions exported
ULONG mytime(void);
void myctime(ULONG secs, char *outbuf, size_t maxLen);
size_t strlcpy(char *dst, const char *src, size_t dstSize);
size_t strlcat(char *dst, const char *src, size_t dstSize);
void mysprintf(char *Buffer, char *ctl, ...);

#ifdef __VBCC__
int stricmp(const char *a, const char *b);
#endif

#endif /* UTILS_H */
