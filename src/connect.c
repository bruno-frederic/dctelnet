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

static struct Window         *Project8Wnd;
static struct Gadget         *Project8GList;
static struct Gadget         *Project8Gadgets[5];
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
					GT_VisualInfo, vi, TAG_DONE );

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




void __SAVE_DS__ __ASM__ Connect_To_Server_Child(void)
{
	if( OpenProject8Window() == 0)
	{
		ULONG winsig, sig, class;
		struct IntuiMessage *message;

		while(1)
		{
			winsig = 1L << Project8Wnd->UserPort->mp_SigBit;

			sig = Wait( winsig | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E );

			if(sig & winsig)
			{
				while (message = GT_GetIMsg(Project8Wnd->UserPort))
				{
					class = message->Class;
					GT_ReplyIMsg(message);
					if(class == IDCMP_GADGETUP)
					{
						abort_flag = 1;
						Signal(parent, SIGBREAKF_CTRL_C);
					}
				}
			}

			if(sig & SIGBREAKF_CTRL_C)
			{
				CloseProject8Window();
				Signal(parent, SIGBREAKF_CTRL_E);
				return;
			}

			if(sig & SIGBREAKF_CTRL_E)
			{
				GT_SetGadgetAttrs(Project8Gadgets[connect_msg_type], Project8Wnd, 0, GTTX_Text, connect_string, TAG_END);
				Signal(parent, SIGBREAKF_CTRL_E);
			}
		}
	}
	CloseProject8Window();
}
