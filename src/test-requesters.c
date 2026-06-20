/*

  +aos68km to be KS 1.* compatible

  Tested under Amigaos 1.2, 1.3, 2 and 3
  Untested under AmigaOS < 1.2
*/

/**
 * @file test-requesters.c
 * @brief Interactive test program for the Requesters module.
 *
 * This program exercises all public requester functions provided by requesters.c and is intended
 * for manual validation on real hardware, emulators, and different AmigaOS/Kickstart versions.
 *
  * Notes:
 *  - Can be executed on Kickstart 1.x, 2.x and later systems to validate runtime dependency checks
 *    and fallback behaviour.
 *
 *  - Built with VBCC using the +aos68km target for Kickstart 1.x compatibility.
 * vc +aos68km -g -D_DEBUG -o ../build/vbcc-68000-debug/test-req test-requesters.c requesters.c
 *
 *  - Verified on AmigaOS 1.2, 1.3, 2.* and 3.*. Not tested on AmigaOS versions earlier than 1.2.
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <proto/exec.h>               // OldOpenLibrary()
#include <proto/dos.h>                // Open(), Close(), Read(), Write(), WRITE_CONST_STR()...
#include <proto/intuition.h>          // OpenWindow(),CloseWindow(), OnMenu(), OffMenu()...
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
#include <proto/gadtools.h>           // GT_GetIMsg(), GT_ReplyIMsg()...
#include <string.h>                   // size_t
#include <stdlib.h>                   // exit()
#include "requesters.h"

struct GfxBase       *GfxBase       = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library       *GadToolsBase  = NULL;
struct Library       *AslBase       = NULL;
struct DosLibrary    *DOSBase       = NULL;

static BPTR Out = 0;   // Needed. DosLibrary must be opened and out initialized with Output() by caller.

// Macros evaluated at compile time. Works on const char [] literals.
// More efficient than other printf() function and less risky of buffer overflow.
// Example : WRITE_CONST_STR("Hello!\n");
#define WRITE_CONST_STR(s) Write(Out, s, sizeof(s) - 1)

void myVPrintf( const STRPTR formatString, const APTR argarray);

// Safer string copy than strcpy() and strncpy(). Note: this is a POSIX function.
size_t strlcpy(char *dst, const char *src, size_t dstSize);


int main(void)
{
    LONG result;
    TEXT buffer[128];
    TEXT buffer2[128];
    UWORD fontYSize;
    ULONG displayID;
    UWORD displayWidth, displayHeight, displayDepth;
    LONG argArray[4] = { 0 };

    DOSBase = (struct DosLibrary *) OldOpenLibrary("dos.library");
    if (DOSBase == NULL)  return RETURN_FAIL;
    Out = Output();

    // OldOpenLibrary() to be KS 1.0 compatible
    GfxBase = (struct GfxBase*) OldOpenLibrary("graphics.library");
    if (GfxBase == NULL)        WRITE_CONST_STR("graphics.library could not be opened.\n");
    else                        argArray[0] = (LONG) GfxBase->LibNode.lib_Version;

    IntuitionBase = (struct IntuitionBase *) OldOpenLibrary("intuition.library");
    if (IntuitionBase == NULL)  WRITE_CONST_STR("intuition.library could not be opened.\n");
    else                        argArray[1] = (LONG) IntuitionBase->LibNode.lib_Version;

    GadToolsBase = OldOpenLibrary("gadtools.library");
    if (GadToolsBase == NULL)   WRITE_CONST_STR("gadtools.library could not be opened.\n");
    else                        argArray[2] = (LONG) GadToolsBase->lib_Version;

    AslBase = OldOpenLibrary("asl.library");
    if (AslBase == NULL)        WRITE_CONST_STR("asl.library could not be opened.\n");
    else                        argArray[3] = (LONG) AslBase->lib_Version;

    myVPrintf("=== Library versions ===\n"
              "graphics.library  v%ld\n"
              "intuition.library v%ld\n"
              "gadtools.library  v%ld\n"
              "asl.library       v%ld\n", argArray);

    WRITE_CONST_STR("--> InfoReq()\n");
    InfoReq(NULL, "Exemple 1 : InfoReq() avec une chaine constante");

    InfoReq(NULL, "Exemple 2 : InfoReq() avec une chaine avec arguments :\n"
                  "IntuitionBase=%lx GfxBase=%lx AslBase=%lx",
                  IntuitionBase, GfxBase, AslBase);

    WRITE_CONST_STR("--> ConfirmRequester()\n");
    result = ConfirmRequester(NULL, "Yes|No", "Exemple 3 : ConfirmRequester()\n");
    myVPrintf("<-- ConfirmRequester() => %ld\n", &result);


    WRITE_CONST_STR("--> GetStringRequester()\n");
    buffer[0] = '\0';
    result = GetStringRequester(NULL, "Exemple 4 : GetStringRequester()", "Prompt:",
                                buffer, sizeof(buffer));
    if (result)
    {
        argArray[0] = (LONG) buffer;
        myVPrintf("<-- GetStringRequester() => %s\n", argArray);
    }


    WRITE_CONST_STR("--> FileRequester()\n");
    buffer[0] = '\0';
    buffer2[0] = '\0';
    result = FileRequester(NULL,
                    buffer, sizeof(buffer),     // dirname
                    buffer2, sizeof(buffer2),   // filename
                    "#?",
                    FILEREQ_LOAD);
    if (result)
    {
        argArray[0] = (LONG) buffer;
        argArray[1] = (LONG) buffer2;
        myVPrintf("<-- FileRequester() => %s %s\n", argArray);
    }


    WRITE_CONST_STR("--> DirectoryRequester()\n");
    buffer[0] = '\0';
    result = DirectoryRequester(NULL,
                                buffer, sizeof(buffer));     // dirname
    if (result)
    {
        argArray[0] = (LONG) buffer;
        myVPrintf("<-- DirectoryRequester() => %s\n", argArray);
    }

    WRITE_CONST_STR("--> FontRequester()\n");
    buffer[0] = '\0';
    fontYSize = 9;
    result = FontRequester(NULL,
                           buffer, sizeof(buffer),
                           &fontYSize);
    if (result)
    {
        argArray[0] = (LONG) buffer;
        argArray[1] = (LONG) fontYSize;
        myVPrintf("<-- FontRequester() => %s %ld\n", argArray);
    }


    WRITE_CONST_STR("--> ScreenModeRequester()\n");
    displayID = (PAL_MONITOR_ID | HIRES_KEY); // PAL High Res (640×256), no interlaced
    displayWidth = 640;
    displayHeight = 256;
    displayDepth = 4;
    result = ScreenModeRequester(NULL, &displayID, &displayWidth, &displayHeight, &displayDepth);
    if (result)
    {
        argArray[0] = (LONG) displayID;
        argArray[1] = (LONG) displayWidth;
        argArray[2] = (LONG) displayHeight;
        argArray[3] = (LONG) displayDepth;
        myVPrintf("<-- ScreenModeRequester() => displayID=0x%lx"
                  " displayWidth=%ld displayHeight=%ld displayDepth=%ld (bitplanes)\n", argArray);
    }

    RecoveryAlert("Exemple final de RecoveryAlert()");
    WRITE_CONST_STR("<-- RecoveryAlert()\n");


    if (AslBase)        CloseLibrary(AslBase);
    if (GadToolsBase)   CloseLibrary((struct Library *) GadToolsBase);
    if (IntuitionBase)  CloseLibrary((struct Library *) IntuitionBase);
    if (GfxBase)        CloseLibrary((struct Library *) GfxBase);
    if (DOSBase)        CloseLibrary((struct Library *) DOSBase);

    WRITE_CONST_STR("\nAll tests completed.\n");
    return RETURN_OK;
}


// Needed for RawDoFmt()
#define BUFFER_SIZE 128
static const ULONG PutChProc=0x16c04e75; // move.b d0,(a3)+ ; rts

// Prototype in 3.2 NDK : LONG VPrintf( CONST_STRPTR format, CONST_APTR argarray )
// Adapted to 1.3:
void myVPrintf( const STRPTR formatString, const APTR argarray)
{
#ifdef _DEBUG
    if (Out != Output())  exit(RETURN_FAIL);
#endif

    unsigned char buf[BUFFER_SIZE];
    RawDoFmt(formatString, argarray, (void (*)())&PutChProc, buf);

#ifdef _DEBUG
    if ( strlen(buf) > sizeof(buf) )
    {
        WRITE_CONST_STR("ERROR: Buffer overflow in myVPrintf() !!!\n");
        exit(RETURN_FAIL);
    }
#endif

    Write(Out, buf, strlen(buf));
}


/**
* @brief Safe string copy with truncation detection.
 *
 * Copies up to dstSize - 1 characters from src to dst and always
 * NUL-terminates dst when dstSize is non-zero (strncpy() does not ensure that)
 *
 * Unlike strncpy(), this function performs no unnecessary NUL padding
 * and guarantees a valid C string in the destination buffer.
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
