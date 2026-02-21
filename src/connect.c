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

static struct Window         *Project8Wnd;           // "Connecting..." window
static struct Gadget         *Project8GList;         // "Connecting..." window GList
static struct Gadget         *Project8Gadgets[5];    // "Connecting..." window Gadgets
#define Project8Width 330
#define Project8Height 105
#define Project8Wdt "Connecting..."


static UBYTE Project8GTypes[] = {
	TEXT_KIND,
	TEXT_KIND,
	TEXT_KIND,
	BUTTON_KIND,
	TEXT_KIND
};


static struct MyNewGadget Project8NGad[] = {
	126, 6, 157, 14, (UBYTE *)"Connect to:",
	126, 25, 157, 15, (UBYTE *)"IP Address:",
	126, 45, 157, 14, (UBYTE *)"Real Host:",
	126, 82, 157, 15, (UBYTE *)"Abort",
	126, 64, 157, 14, (UBYTE *)"Status:",
};

static ULONG Project8GTags[] = {
	(GTTX_Border), TRUE, (TAG_DONE),
	(GTTX_Border), TRUE, (TAG_DONE),
	(GTTX_Border), TRUE, (TAG_DONE),
	(TAG_DONE),
	(GTTX_Border), TRUE, (TAG_DONE)
};


static int OpenProject8Window( void )
{
	struct Gadget *g;
	UWORD ww, wh;
	UWORD width, height;

	ComputeFont( Project8Width, Project8Height );

	ww = ComputeX( Project8Width );
	wh = ComputeY( Project8Height );

	if ( ! ( g = CreateContext( &Project8GList )))
		return( 1L );

	MakeGadgets(Project8NGad, Project8Gadgets, Project8GTags, g, Project8GTypes, Project8_CNT);

	width = ww + OffX + scr->WBorRight;
	height = wh + OffY + scr->WBorBottom;

	if ( ! ( Project8Wnd = OpenWindowTags( NULL,
				WA_Left,	(scr->Width - width) / 2,
				WA_Top,		(scr->Height - height) / 2,
				WA_Width,	width,
				WA_Height,	height,
				WA_IDCMP,	TEXTIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,
				WA_Flags,	WFLG_ACTIVATE|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SMART_REFRESH,
				WA_Gadgets,	Project8GList,
				WA_Title,	Project8Wdt,
				WA_PubScreen,	scr,
				TAG_DONE )))
	return( 4L );

	GT_RefreshWindow( Project8Wnd, NULL );

	ComputeFont( Project8Width, Project8Height );

	DrawBevelBox( Project8Wnd->RPort, OffX + ComputeX( 3 ),
					OffY + ComputeY( 2 ),
					ComputeX( 326 ),
					ComputeY( 101 ),
					GT_VisualInfo, visualInfos, TAG_DONE );

	return( 0L );
}

static void CloseProject8Window( void )
{
	if ( Project8Wnd        ) {
		CloseWindow( Project8Wnd );
		Project8Wnd = NULL;
	}

	if ( Project8GList      ) {
		FreeGadgets( Project8GList );
		Project8GList = NULL;
	}
}



/**
 * @brief Entry point for task handling the "Connecting..." window display.
 *
 * This function is executed in a newly created AmigaOS Task (roughly equivalent to a lightweight
 * thread) when a new connection to a server is attempted.
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
	if( OpenProject8Window() == 0)
	{
        ULONG winsig;                    // Signal mask for window events
        ULONG sig;                       // Signals received by Wait()
        ULONG class;                     // IDCMP message class
		struct IntuiMessage *message;

        // Main task loop:
		while(1)
		{
			winsig = 1L << Project8Wnd->UserPort->mp_SigBit;

			sig = Wait( winsig | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E );

			// Handle Intuition window events
			if(sig & winsig)
			{
				while (message = GT_GetIMsg(Project8Wnd->UserPort))
				{
					class = message->Class;
					GT_ReplyIMsg(message);
					if(class == IDCMP_GADGETUP) // User clicked a gadget (typically "Cancel")
					{
						isConnectionAborted = 1;  // Mark connection as aborted by the user
						Signal(parentTask, SIGBREAKF_CTRL_C);
					}
				}
			}

			// Connection phase done: SIGBREAKF_CTRL_C indicates we must close the window
			// and notify the parent task:
			if(sig & SIGBREAKF_CTRL_C)
			{
				CloseProject8Window();
				Signal(parentTask, SIGBREAKF_CTRL_E);
				return;
			}

			// Connection status update from parent task:
			if(sig & SIGBREAKF_CTRL_E)
			{
				// Update the text displayed in the "Connecting..." window :
				GT_SetGadgetAttrs(Project8Gadgets[connectMsgType], Project8Wnd, 0, GTTX_Text, connectString, TAG_END);
				Signal(parentTask, SIGBREAKF_CTRL_E);
			}
		}
	}
	CloseProject8Window();
}
