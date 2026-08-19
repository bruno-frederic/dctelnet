/**
 * @file utils.c
 * @brief General-purpose utility functions for AmigaOS 2.0+.
 *
 * This module provides lightweight utility functions used throughout the application, including
 * time and date handling, safe string operations, formatted output, and compatibility functions for
 * C library features not consistently available across AmigaOS compilers and versions.
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#ifdef __VBCC__
    #pragma dontwarn 306
#endif
#include <proto/exec.h>               // RawDoFmt()
#include <proto/dos.h>                // DateToStr(), LEN_DATSTRING, TICKS_PER_SECOND
#include <proto/intuition.h>          // CurrentTime()
#ifdef __VBCC__
    #pragma popwarn
#endif
#include "utils.h"
#include "requesters.h"


// Unix time starts on 1970-01-01, while AmigaOS DateStamp time starts on 1978-01-01.
// 252460800 is the number of seconds between these two epochs.
#define UNIX_TO_AMIGA_EPOCH 252460800L

#define SECONDS_PER_DAY     86400
#define MINUTES_PER_DAY     1440
#define SECONDS_PER_MINUTE  60

/**
 * @brief Returns the current time as a Unix timestamp.
 *
 * Retrieves the current time using Intuition library and converts it to the number of
 * seconds elapsed since the Unix epoch (1970-01-01 00:00:00 UTC).
 * AmigaOS uses 1978-01-01 00:00:00 as its epoch. The conversion therefore adds the number
 * of seconds between the AmigaOS and Unix epochs.
 *
 * @return Current time in seconds since the Unix epoch.
 */
ULONG mytime(void)
{
    ULONG seconds;

    CurrentTime(&seconds, NULL); // works on OS 3.2 with 2nd arg = NULL

    return seconds + UNIX_TO_AMIGA_EPOCH;
}


/**
 * @brief Converts an Unix timestamp to a human-readable date and time string.
 *
 * Converts a Unix timestamp expressed in seconds since 1970-01-01 00:00:00 into an AmigaOS
 * DateStamp and formats it using DateToStr() from DOS.library.
 *
 * A timestamp of zero is displayed as "Never".
 *
 * @param secs     Unix timestamp in seconds.
 * @param outbuf   Destination buffer receiving the formatted date and time.
 * @param maxLen   Size of the destination buffer in bytes.
 */
void myctime(ULONG secs, char *outbuf, size_t maxLen)
{
    char strTime[LEN_DATSTRING + 1]; // one extra byte for ' ' character
    struct DateTime dt;

    if (secs == 0)
    {
        strlcpy(outbuf, "Never", maxLen);
        return;
    }

    // Convert Unix time (1970 epoch) to AmigaOS DateStamp time (1978 epoch).
    secs -= UNIX_TO_AMIGA_EPOCH;

    dt.dat_Stamp.ds_Days   = secs / SECONDS_PER_DAY;
    dt.dat_Stamp.ds_Minute = (secs / SECONDS_PER_MINUTE) % MINUTES_PER_DAY;
    dt.dat_Stamp.ds_Tick   = (secs % SECONDS_PER_MINUTE) * TICKS_PER_SECOND;

    dt.dat_Format = FORMAT_DOS;
    dt.dat_Flags  = 0;

    // DateToStr() requires output buffers of at least LEN_DATSTRING bytes.
    dt.dat_StrDay  = NULL;
    dt.dat_StrDate = outbuf;
    dt.dat_StrTime = strTime + 1;

    DateToStr(&dt);

    strTime[0] = ' ';

    strlcat(outbuf, strTime, maxLen);
}

void mysprintf(char *Buffer, char *ctl, ...)
{
    #ifdef __VBCC__
    #pragma dontwarn 79 // warning 79: offset equals size of object
    #endif
    RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75", Buffer);
    #ifdef __VBCC__
    #pragma popwarn
    #endif
}


/**
* @brief Safe string copy with truncation detection.
 *
 * Copies up to dstSize - 1 characters from src to dst and always NUL-terminates dst when dstSize is
 * non-zero (strncpy() does not ensure that)
 *
 * Unlike strncpy(), this function performs no unnecessary NUL padding and guarantees a valid C
 * string in the destination buffer.
 *
 *
 * This is a BSD extension and is not part of the ISO C or POSIX standards.
 *
 * @param dstSize  Size of destination buffer in bytes.
 *
 * @return Length of src, excluding the terminating NUL.
 *         A return value >= dstSize indicates truncation.
 */
size_t strlcpy(char *dst, const char *src, size_t dstSize)
{
    char *d   = dst;
    const char *s = src;
    size_t n = dstSize;

    #ifdef _DEBUG
        if (src == NULL)
        {
            InfoReq(NULL, "strlcpy(): src == NULL");
            return 0;
        }

        if (dst == NULL && dstSize != 0)
        {
            InfoReq(NULL, "strlcpy(): dst == NULL");
            return 0;
        }
    #endif

    if (n!=0 && --n!=0)
    {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    if (n == 0)
    {
        if (dstSize != 0)
            *d = '\0';
        while (*s++) { }
    }

    return (size_t) (s - src - 1);
}


/**
 * @brief Safe string concatenation with truncation detection.
 *
 * Appends up to dstSize - strlen(dst) - 1 characters from src to dst and always NUL-terminates
 * dst when dstSize is non-zero.
 *
 * Unlike strcat(), this function never writes beyond the destination buffer. Unlike strncat(),
 * dstSize specifies the total size of the destination buffer rather than the maximum number of
 * characters to append.
 *
 * If dst is not NUL-terminated within the first dstSize bytes, no characters are appended and
 * the function returns dstSize + strlen(src).
 *
 * This is a BSD extension and is not part of the ISO C or POSIX standards.
 *
 * @param dst      Destination C string to which src is appended.
 * @param src      Source C string to append.
 * @param dstSize  Size of destination buffer in bytes.
 *
 * @return The total length of the string they tried to create, excluding the terminating NUL.
 *
 * ```
 * A return value >= dstSize indicates truncation.
 * ```
 */
size_t strlcat(char *dst, const char *src, size_t dstSize)
{
    const char *odst = dst;
    const char *osrc = src;
    size_t n = dstSize;
    size_t dlen;

#ifdef _DEBUG
    if (src == NULL)
    {
        InfoReq(NULL, "strlcat(): src == NULL");
        return 0;
    }

    if (dst == NULL && dstSize != 0)
    {
        InfoReq(NULL, "strlcat(): dst == NULL");
        return 0;
    }
#endif

    // Find the end of dst and adjust bytes left but don't go past end.
    while (n-- != 0 && *dst != '\0')
        dst++;

    dlen = dst - odst;
    n = dstSize - dlen;

    if (n == 0)
        return dlen + strlen(src);

    n--;    // Reserve space for the terminating '\0'

    while (*src != '\0')
    {
        if (n != 0)
        {
            *dst++ = *src;
            n--;
        }
        src++;
    }

    *dst = '\0';

    return dlen + (src - osrc);     // Count does not include NUL
}


#ifdef __VBCC__
#include <ctype.h>                      // tolower()

/**
 * @brief Case-insensitive string comparison implementation for VBCC.
 *
 * stricmp() is not a standard C function.
 * It is used in the AddressBook sorting algorithm when clicking the List Sorted By button.
 * SAS/C provides a vendor-specific stricmp() implementation in string.h.
 *
 * AmigaOS provides Stricmp() but starting in 2.04, so this implementation is used
 * to keep AmigaOS 2.00 compatibility with VBCC.
 *
 * @param a Pointer to the first NUL-terminated string.
 * @param b Pointer to the second NUL-terminated string.
 * @return Negative value if a < b, zero if a == b, positive value if a > b.
 */
int stricmp(const char *a, const char *b)
{
    int ca, cb;

    while (*a && *b)  // asserts that pointers are not NULL
    {
        ca = tolower((unsigned char)*a++);
        cb = tolower((unsigned char)*b++);

        if (ca != cb)
            return ca - cb;
    }

    return (unsigned char)*a - (unsigned char)*b;
}
#endif
