/**
 * @file Xem_wrapper.c
 * @brief DCTelnet implementation of Xem library callback function
 *
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <proto/exec.h>               // OpenLibrary(), AllocMem()...
#include <proto/dos.h>                // RETURN_OK, VPrintf()
#include <proto/intuition.h>          // DisplayBeep()
#include "Xem_wrapper.h"
#include "DCTelnet.h"                 // win, scr, ansiFont, prefs.displaydriver, buf
#include "Xfer.h"                     // xpr_sread(), xpr_swrite(), xpr_sflush(), xpr_options()
#include "requesters.h"

struct Library *XEmulatorBase;
struct XEM_IO *xemIO;


// Unimplemented functions. We provide dummy implementations that just display a message
// when called, so we can detect if and when they are called by the XEM library.
#ifdef _DEBUG
    /* Send a break signal across the serial line.
       Produced by DEC VT340 F5 key
    */
    LONG __SAVE_DS__ xem_sbreak(VOID)
    {
        InfoReq(isRunningOnWB ? NULL : win, "xem_sbreak() is not implemented.");
        return RETURN_OK;
    }

    LONG __SAVE_DS__ xem_squery(VOID)
    {
        InfoReq(isRunningOnWB ? NULL : win, "xem_squery() is not implemented. when is it called ?");
        return -1L; // error
    }

    /* Restart serial read activity.
       xemvt340.library v4.1 call it after a file transfer initiated by DEC VT340 F4 key (Data/Talk
       mode)
    */
    VOID __SAVE_DS__ xem_sstart(VOID)
    {
        InfoReq(isRunningOnWB ? NULL : win, "xem_sstart() is not implemented.");
    }

    /* This function tells the comm program to stop the serial read activity. After this call, the
       XEm owns the serial line.
       xemvt340.library v4.1 call it after a file transfer initiated by DEC VT340 F4 key (Data/Talk
       mode)
    */
    LONG __SAVE_DS__ xem_sstop(VOID)
    {
        InfoReq(isRunningOnWB ? NULL : win, "xem_sstop() is not implemented.");
        return RETURN_OK;
    }

    /* Dispatch a macro key call.
       Requires macros to be defined beforehand using XEmulatorMacroKeyFilter().
    */
    LONG __SAVE_DS__ __ASM__ xem_process_macrokeys(__REG__(a0, struct XEmulatorMacroKey *key))
    {
        InfoReq(isRunningOnWB ? NULL : win, "xem_process_macrokeys() is not implemented.");
        return -1L; // What is the meaning of return value?
    }
#endif // _DEBUG


//Beep the terminal display.
VOID __SAVE_DS__ __ASM__ xem_tbeep(__REG__(d0, ULONG ntimes), __REG__(d1, ULONG delay))
{
    #ifdef _DEBUG
        static ULONG array[2];
        array[0] = ntimes;
        array[1] = delay;

        VPrintf("--> xem_tbeep(ntimes = %ld, delay = %ld)\n", array);
    #endif

    for (; ntimes > 0 ; ntimes--)
    {
        DisplayBeep(scr);

        Delay(delay);
    }
}

// This function is called by the XEM library when it needs to get a string from the user.
// With xemvt340.library v4.1, this function is only called during file transfer operations when the
// user presses the DEC VT340 F4 key (Data/Talk mode). It is used to prompt the user for information
// such as a filename or directory.
LONG __SAVE_DS__ __ASM__ xem_tgets(__REG__(a0, UBYTE *prompt),
                                __REG__(a1, UBYTE *buffer), __REG__(d0, ULONG buflen))
{
    return GetStringRequester(isRunningOnWB ? NULL : win,
                                 "XEM library request",
                                 prompt,
                                 buffer, buflen);
}


BOOL InitializeXemLibrary(void)
{
	xemIO = AllocMem(sizeof(struct XEM_IO), MEMF_PUBLIC|MEMF_CLEAR);
	if (xemIO == NULL)
	{
        RecoveryAlert("Not enough memory!");
		goto clean_and_return;
	}

	XEmulatorBase = OpenLibrary(prefs.displaydriver, 0);
	if (XEmulatorBase == NULL)
	{
		InfoReq(isRunningOnWB ? NULL : win, "Failed to open XEM library: %s", prefs.displaydriver);

		goto clean_and_return;
	}

    xemIO->xem_window      = win;
    xemIO->xem_font        = ansiFont;
    //xemIO->xem_console   = NULL;
    //xemIO->xem_signal    = 0;
    xemIO->xem_screendepth = scr->BitMap.Depth;

    xemIO->xem_sread             = xpr_sread;
    xemIO->xem_swrite            = xpr_swrite;
    xemIO->xem_sflush            = xpr_sflush;

    #ifdef _DEBUG
        // Unimplemented functions. We provide dummy implementations that just display a message
        // when called, so we can detect if and when they are called by the XEM library.
        xemIO->xem_sbreak            = xem_sbreak;
        xemIO->xem_squery            = xem_squery;
        xemIO->xem_sstart            = xem_sstart;
        xemIO->xem_sstop             = xem_sstop;
        xemIO->xem_process_macrokeys = xem_process_macrokeys;
    #endif

    xemIO->xem_tgets             = xem_tgets;
    // XEM_IO->xem_options is 100% compatible to XPR_IO->xpr_options
    xemIO->xem_toptions =
    (ULONG (* __ASM__ )(__REG__(d0, LONG n), __REG__(a0, struct xem_option *opt[])))xpr_options;

    xemIO->xem_tbeep             = xem_tbeep;

    // allocates and initializes emulator-private data :
    if ( ! XEmulatorSetup(xemIO) )
    {
        InfoReq(isRunningOnWB ? NULL : win, "XEmulatorSetup() failed!");
        goto clean_and_return;
    }

    // XEmulatorOpenConsole has to be called after a comm-proggy has opened (or changed) its screen
    // and/or window. XEmulatorOpenConsole resets its internal data structure and sets custom-fonts,
    // special drawing modes, etc..
    if ( ! XEmulatorOpenConsole(xemIO) )
    {
        XEmulatorCleanup(xemIO);
        InfoReq(isRunningOnWB ? NULL : win, "XEmulatorOpenConsole() failed!");
        goto clean_and_return;
    }

    // Install macro-key definitions. NULL disables the emulators key-filter.
    XEmulatorMacroKeyFilter(xemIO, NULL);

    // everything alright:
    return TRUE;


clean_and_return:
    if (xemIO != NULL)
    {
        FreeMem(xemIO, sizeof(struct XEM_IO));
        xemIO = NULL;
    }

    if (XEmulatorBase != NULL)
    {
        CloseLibrary(XEmulatorBase);
        XEmulatorBase = NULL;
    }

    return FALSE;
}

// Unitilize XEM library if it was initialized (does nothing if it was not initialized)
void UninitializeXemLibrary(void)
{
    #ifdef _DEBUG
        PutStr("--> UninitializeXemLibrary()\n");
    #endif

    if (XEmulatorBase)
    {
        XEmulatorCloseConsole(xemIO); // gives the emulator the chance to free window-specific data
        XEmulatorCleanup(xemIO);      // free internal structure used by the emulator
        CloseLibrary(XEmulatorBase);
        XEmulatorBase = NULL;
        FreeMem(xemIO,sizeof(struct XEM_IO));
        xemIO = NULL;
    }
}
