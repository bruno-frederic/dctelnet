/* DCTELNET CONNECT GUI

    Small window titled "Connection..." briefly displayed while connecting to server.
    Based on GadTools.
    Built with GadToolBox V2.0b according to the .gui file
*/

#include <proto/exec.h>                 // Wait(), Signal()
#include <proto/dos.h>                  // SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E
#include <proto/intuition.h>            // OpenWindowTags(), CloseWindow()
#include <proto/gadtools.h>             // GT_GetIMsg(), GT_ReplyIMsg(), <CreateContext()...
#include "connect.h"
#include "DCTelnet.h"
#include "guis.h"

static struct Window         *ConnectingWnd;           // "Connecting..." window
static struct Gadget         *ConnectingGList;         // "Connecting..." window GList
static struct Gadget         *ConnectingGadgets[5];    // "Connecting..." window Gadgets
#define ConnectingWidth 330
#define ConnectingHeight 105
#define ConnectingWdt "Connecting..."


static UBYTE ConnectingGTypes[] = {
    TEXT_KIND,
    TEXT_KIND,
    TEXT_KIND,
    BUTTON_KIND,
    TEXT_KIND
};


static struct MyNewGadget ConnectingNGad[] = {
    126, 6, 157, 14, (UBYTE *)"Connect to:",
    126, 25, 157, 15, (UBYTE *)"IP Address:",
    126, 45, 157, 14, (UBYTE *)"Real Host:",
    126, 82, 157, 15, (UBYTE *)"Abort",
    126, 64, 157, 14, (UBYTE *)"Status:",
};

static ULONG ConnectingGTags[] = {
    (GTTX_Border), TRUE, (TAG_DONE),
    (GTTX_Border), TRUE, (TAG_DONE),
    (GTTX_Border), TRUE, (TAG_DONE),
    (TAG_DONE),
    (GTTX_Border), TRUE, (TAG_DONE)
};


static int OpenConnectingWindow( void )
{
    struct Gadget *g;
    UWORD ww, wh;
    UWORD width, height;

    ComputeFont( ConnectingWidth, ConnectingHeight );

    ww = ComputeX( ConnectingWidth );
    wh = ComputeY( ConnectingHeight );

    if ( ! ( g = CreateContext( &ConnectingGList )))
        return( 1L );

    MakeGadgets(ConnectingNGad, ConnectingGadgets, ConnectingGTags, g, ConnectingGTypes, Connecting_CNT);

    width = ww + OffX + scr->WBorRight;
    height = wh + OffY + scr->WBorBottom;

    if ( ! ( ConnectingWnd = OpenWindowTags( NULL,
                WA_Left,    (scr->Width - width) / 2,
                WA_Top,        (scr->Height - height) / 2,
                WA_Width,    width,
                WA_Height,    height,
                WA_IDCMP,    TEXTIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,
                WA_Flags,    WFLG_ACTIVATE|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SMART_REFRESH,
                WA_Gadgets,    ConnectingGList,
                WA_Title,    ConnectingWdt,
                WA_PubScreen,    scr,
                TAG_DONE )))
    return( 4L );

    GT_RefreshWindow( ConnectingWnd, NULL );

    ComputeFont( ConnectingWidth, ConnectingHeight );

    DrawBevelBox( ConnectingWnd->RPort, OffX + ComputeX( 3 ),
                    OffY + ComputeY( 2 ),
                    ComputeX( 326 ),
                    ComputeY( 101 ),
                    GT_VisualInfo, visualInfos, TAG_DONE );

    return( 0L );
}

static void CloseConnectingWindow( void )
{
    if ( ConnectingWnd        ) {
        CloseWindow( ConnectingWnd );
        ConnectingWnd = NULL;
    }

    if ( ConnectingGList      ) {
        FreeGadgets( ConnectingGList );
        ConnectingGList = NULL;
    }
}



/**
 * @brief Entry point for the task handling the "Connecting..." window display.
 *
 * This function is executed in a newly created AmigaOS Task/Process (roughly equivalent to a
 * lightweight thread) when a new connection to a server is attempted.
 *
 * Its sole responsibility is to display and manage the "Connecting..." window while the connection
 * is in progress.
 *
 * The task runs an event loop waiting for:
 * - Intuition events from the window (user interaction)
 * - Exec signals sent by the parent task (connection status)
 *
 * The task terminates automatically when the connection attempt finishes (success or failure) or
 * when the user aborts the operation.
 *
 * Inter-task communication is done using Exec signals:
 * - SIGBREAKF_CTRL_C : user requested abort
 * - SIGBREAKF_CTRL_E : connection status update or completion
 */
void __SAVE_DS__ __ASM__ HandleConnectingWindowTask(void)
{
    // Open the "Connecting..." window.
    if( OpenConnectingWindow() == RETURN_OK)
    {
        ULONG winsig;                    // Signal mask for window events
        ULONG sig;                       // Signals received by Wait()
        ULONG class;                     // IDCMP message class
        struct IntuiMessage *message;

        // Main task loop:
        while(1)
        {
            winsig = 1L << ConnectingWnd->UserPort->mp_SigBit;

            sig = Wait( winsig | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E );

            // Handle Intuition window events
            if(sig & winsig)
            {
                while (message = GT_GetIMsg(ConnectingWnd->UserPort))
                {
                    class = message->Class;
                    GT_ReplyIMsg(message);
                    if(class == IDCMP_GADGETUP) // User clicked a gadget (typically "Cancel")
                    {
                        isConnectionAborted = 1;  // Mark connection as aborted by the user
                        Signal(mainTask, SIGBREAKF_CTRL_C);
                    }
                }
            }

            // Connection phase done: SIGBREAKF_CTRL_C indicates we must close the window
            // and notify the parent task:
            if(sig & SIGBREAKF_CTRL_C)
            {
                CloseConnectingWindow();
                Signal(mainTask, SIGBREAKF_CTRL_E);
                return;
            }

            // Connection status update from parent task:
            if(sig & SIGBREAKF_CTRL_E)
            {
                // Update the text displayed in the "Connecting..." window :
                GT_SetGadgetAttrs(ConnectingGadgets[connectMsgType], ConnectingWnd, 0, GTTX_Text, connectString, TAG_END);
                Signal(mainTask, SIGBREAKF_CTRL_E);
            }
        }
    }
    CloseConnectingWindow();
}
