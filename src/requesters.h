#ifndef REQUESTERS_H
#define REQUESTERS_H

/**
 * @file requesters.h
 * @brief ASL and GadTools-based replacement for ReqTools requesters.
 *
 * This module implements lightweight requesters using only native AmigaOS 2.0 components.
 *
 * Its primary goal is to remove the dependency on the unmaintained ReqTools library.
 *
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <exec/types.h>

// Types
/**
 * @brief File requester operation mode.
 */
typedef enum
{
    FILEREQ_LOAD = 0, /**< Open/load an existing file. */
    FILEREQ_SAVE      /**< Save a file.                */
} FileRequesterMode;


// Global variables exported


// Functions exported
VOID RecoveryAlert(CONST_STRPTR msg);
VOID InfoReq(struct Window *parent, CONST_STRPTR str, ...); // varargs parameters
LONG ConfirmRequester(struct Window *parent, CONST_STRPTR gadgetFormat, CONST_STRPTR str,
                         ...); // varargs parameters
BOOL GetStringRequester(struct Window *parent, STRPTR title, STRPTR prompt,
                           STRPTR buffer, UWORD maxLen);
BOOL DirectoryRequester(struct Window *parent, STRPTR dirName, UWORD maxLen);
BOOL FileRequester(struct Window *parent, STRPTR dirName, UWORD dirMaxLen,
                   STRPTR fileName, UWORD fileMaxLen,
                   STRPTR pattern,
                   FileRequesterMode mode);
BOOL FontRequester(struct Window *parent, STRPTR fontName, UWORD maxLen, UWORD *fontYSize);
BOOL ScreenModeRequester(struct Window *parent, ULONG* displayID,
                         UWORD* displayWidth, UWORD* displayHeight, UWORD* displayDepth);

#endif /* REQUESTERS_H */
