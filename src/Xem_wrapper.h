#ifndef XEM_WRAPPER_H
#define XEM_WRAPPER_H

/**
 * @file Xem_wrapper.h
 * @brief DCTelnet implementation of Xem library callback function
 *
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <libraries/xem.h>            // struct XEM_IO
#include <proto/xem.h>                // XEmulatorWrite()...

#include <exec/types.h>

// Types


// Global variables exported
extern struct XEM_IO *xemIO;


// Functions exported
BOOL InitializeXemLibrary(void);
void UninitializeXemLibrary(void);

// XEmulatorWrite() is invoked directly from DCTelnet.c via this macro to avoid the overhead of an
// extra function call.
// Internally, the XEM library relies on graphics.library functions (such as Text()) to render text
// into the console window's RasterPort
#define XemWrite(string, len) XEmulatorWrite((xemIO), (string), (len))


#endif /* XEM_WRAPPER_H */
