
// DCTelnet - ADDRESS BOOK, EDITPROFILE, FUNCTION KEYS AND SCROLLBACK GUI

#define __USE_SYSBASE

#include <proto/dos.h>                // Open(), Close(), FRead(), FWrite()
#include <proto/exec.h>               // AllocMem(), AddTail(), FreeMem(), WaitPort(), Remove()
#include <proto/intuition.h>          // OpenWindow(),CloseWindow(), NewObjectA() but no NewObject()
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
#include <proto/gadtools.h>           // LISTVIEW_KIND, BUTTON_KIND, GTLV_Labels...
#include <proto/wb.h>               // BF: no errors compiling without it
#include <proto/icon.h>               // GetDiskObjectNew(), FreeDiskObject()
#include <dos/dos.h>                // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <exec/nodes.h>             // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <exec/lists.h>             // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <exec/memory.h>            // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <intuition/intuition.h>    // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <intuition/gadgetclass.h>  // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <libraries/gadtools.h>     // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <string.h>                   // memcpy(), strcpy(), strcat(), strlen()
#include <ctype.h>	                  // tolower(), toupper()
#include <libraries/reqtools.h>     // BF: no errors compiling without it on VBCC + NDK3.2R4
#include <proto/reqtools.h>           // rtEZRequestA(), rtSetWaitPointer()
#include "abook.h"                    // required
#include "edit.h"                     // required


void CheckDimensions(struct NewWindow *newwin);


struct PrefsStruct
{
	ULONG	DisplayID;
        UWORD	DisplayWidth,
		DisplayHeight,
		DisplayDepth,
		fontsize;
	char	fontname[32],
		downloadpath[52],
		xferlibrary[52],
		xferinit[52];
	UWORD	color[16];
	ULONG	flags;	/* --->    BIT 0 = Hide Title Bar */
	UWORD	win_left,	/* BIT 1 = CRLF Correction */
		win_top,	/* BIT 2 = Hide LEDS */
		win_width,	/* BIT 3 = Use Workbench */
		win_height,	/* BIT 4 = BS/DEL Swap */
		sb_left,	/* BIT 5 = Disable Logging */
		sb_top,		/* BIT 6 = Strip Colour */
		sb_width,	/* BIT 7 = Simple Negotiation */
		sb_height;	/* BIT 8 = Packet Window */
	char	uploadpath[52],	/* BIT 9 = Display Driver */
		displaydriver[32];/*IT 10 = Tool Window */
	ULONG	sb_lines;
	char	displayidstr[32];
	UWORD	twin_left,
		twin_top;
};

extern struct PrefsStruct prefs;

extern char prefsfile[];
extern char keysfile[];
extern char bookfile[];


char EditProfile(struct BookStruct *book);


extern ULONG tags[3];
extern char username[42], password[42];
extern unsigned char buf[2048];
extern struct Window *win, *twin;
extern struct NewWindow nwin;
extern void		*vi;
extern struct Screen	*scr;
extern struct NewGadget	ng;
extern UWORD cport, WinTop;
struct Window         *Project0Wnd;
struct Gadget         *Project0GList;
struct Gadget         *Project0Gadgets[6];
#define Project0Width 420
#define Project0Height 132
struct TextAttr		Attr;
UWORD                  FontX, FontY;
UWORD                  OffX, OffY;

ULONG lastsec, lasttic;

UBYTE *SORT0Labels[] = {
	(UBYTE *)"Name",
	(UBYTE *)"Name Reverse",
	(UBYTE *)"Last Connect",
	NULL };


UBYTE Project0GTypes[] = {
	LISTVIEW_KIND,
	BUTTON_KIND,
	BUTTON_KIND,
	BUTTON_KIND,
	CYCLE_KIND,
	BUTTON_KIND
};

struct MyNewGadget
{
    WORD ng_LeftEdge, ng_TopEdge;       /* gadget position */
    WORD ng_Width, ng_Height;           /* gadget size */
    UBYTE *ng_GadgetText;               /* gadget label */
};

struct MyNewGadget Project0NGad[] = {
	10, 5, 401, 72, NULL,
	19, 84, 93, 13, (UBYTE *)"_Connect",
	115, 84, 93, 13, (UBYTE *)"_Edit",
	212, 84, 93, 13, (UBYTE *)"_Add",
	190, 103, 173, 13, (UBYTE *)"List Sorted By:",
	308, 84, 93, 13, (UBYTE *)"_Delete",
};

ULONG Project0GTags[] = {
	GTLV_Labels, 0, (GTLV_ShowSelected), NULL, GTLV_Selected, 0, (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE),
	(GTCY_Labels), (ULONG)&SORT0Labels[ 0 ], (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE)
};

UWORD ComputeX( UWORD value )
{
	return(( UWORD )((( FontX * value ) + 4 ) / 8 ));
}

UWORD ComputeY( UWORD value )
{
	return(( UWORD )((( FontY * value ) + 4 ) / 8 ));
}

char MakeGadgets(struct MyNewGadget ProjectNGad[], struct Gadget *ProjectGadgets[], ULONG ProjectGTags[], struct Gadget *g, UBYTE ProjectGTypes[], UWORD Count)
{
	UWORD lc, tc;

	ng.ng_VisualInfo = vi;
	ng.ng_TextAttr   = &Attr;

	for( lc = 0, tc = 0; lc < Count; lc++ )
	{
		//CopyMem((char * )&ProjectNGad[ lc ], (char * )&ng, (long)sizeof( struct MyNewGadget ));

		memcpy((char * )&ng, (char * )&ProjectNGad[ lc ], sizeof( struct MyNewGadget ));

		ng.ng_GadgetID	 = lc;
		ng.ng_LeftEdge   = OffX + ComputeX( ng.ng_LeftEdge );
		ng.ng_TopEdge    = OffY + ComputeY( ng.ng_TopEdge );
		ng.ng_Width      = ComputeX( ng.ng_Width );
		ng.ng_Height     = ComputeY( ng.ng_Height);

		ProjectGadgets[ lc ] = g = CreateGadgetA((ULONG)ProjectGTypes[ lc ], g, &ng, ( struct TagItem * )&ProjectGTags[ tc ] );

		while( ProjectGTags[ tc ] ) tc += 2;
		tc++;

		if ( NOT g ) return( 2 );
	}
	return(0);
}


void ComputeFont( UWORD width, UWORD height )
{
	//Font = &Attr;
	Attr.ta_Name = (STRPTR)scr->RastPort.Font->tf_Message.mn_Node.ln_Name;
	Attr.ta_YSize = FontY = scr->RastPort.Font->tf_YSize;
	FontX = scr->RastPort.Font->tf_XSize;

	OffX = scr->WBorLeft;
	OffY = scr->RastPort.TxHeight + scr->WBorTop + 1;

	if (( ComputeX( width ) + OffX + scr->WBorRight ) > scr->Width )
		goto UseTopaz;
	if (( ComputeY( height ) + OffY + scr->WBorBottom ) > scr->Height )
		goto UseTopaz;
	return;

UseTopaz:
	Attr.ta_Name = "topaz.font";
	FontX = FontY = Attr.ta_YSize = 8;
}

int __inline OpenProject0Window( void )
{
	struct Gadget	*g;
	UWORD		ww, wh;
	long	x,y;

	ComputeFont( Project0Width, Project0Height );

	ww = ComputeX( Project0Width );
	wh = ComputeY( Project0Height );

	if ( ! ( g = CreateContext( &Project0GList )))
		return( 1L );

	if(MakeGadgets(Project0NGad, Project0Gadgets, Project0GTags, g, Project0GTypes, Project0_CNT) != 0) return( 2L );

	x = ww + OffX + scr->WBorRight;
	y = wh + OffY + scr->WBorBottom;

	nwin.LeftEdge = (scr->Width - x) / 2;
	nwin.TopEdge = (scr->Height - y) / 2;
	nwin.Width = x;
	nwin.Height = y;
	nwin.IDCMPFlags = LISTVIEWIDCMP|BUTTONIDCMP|CYCLEIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY;
	nwin.Flags = WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP;
	nwin.FirstGadget = Project0GList;
	nwin.Title = "DCTelnet: Address Book";

	Project0Wnd = OpenWindow(&nwin);
	if(!Project0Wnd) return( 4L );

/*	if ( ! ( Project0Wnd = OpenWindowTags( NULL,
				WA_Left,	(scr->Width - x) / 2,
				WA_Top,		(scr->Height - y) / 2,
				WA_Width,	x,
				WA_Height,	y,
				WA_IDCMP,	LISTVIEWIDCMP|BUTTONIDCMP|CYCLEIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY,
				WA_Flags,	WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP,
				WA_Gadgets,	Project0GList,
				WA_Title,	"DCTelnet: Address Book",
				WA_CustomScreen,scr,
				TAG_DONE )))
	return( 4L );*/

	GT_RefreshWindow( Project0Wnd, NULL );

	ComputeFont( Project0Width, Project0Height );

	DrawBevelBox( Project0Wnd->RPort, OffX + ComputeX( 11 ),
					OffY + ComputeY( 80 ),
					ComputeX( 399 ),
					ComputeY( 43 ),
					GT_VisualInfo, vi, GTBB_Recessed, TRUE, TAG_DONE );
	DrawBevelBox( Project0Wnd->RPort, OffX + ComputeX( 3 ),
					OffY + ComputeY( 2 ),
					ComputeX( 415 ),
					ComputeY( 128 ),
					GT_VisualInfo, vi, TAG_DONE );

	return( 0L );
}

long mytime(void)
{
	struct DateStamp stamp;
	DateStamp(&stamp);
	return( (stamp.ds_Days*24*60*60) + (stamp.ds_Minute*60) + (stamp.ds_Tick/50) + 252460800 );
}

struct BookStruct
{
	char	name[32];
	char	host[52];
	UWORD	port;
	long	last;
	char	username[42];
	char	password[42];
	char	res[82];
};

extern UWORD Connect_To_Server(char *servername, UWORD port);
extern void LocalPrint(char *data);
extern void mysprintf(char *Buffer, char *ctl, ...);


struct Node *FindNode(struct List *listviewlist, UWORD lastcode)
{
	struct Node *worknode;
	UWORD i = 0;

	// BF: Why this test? lastcode is UWORD, how could it be negative?
	if(lastcode != -1  &&  listviewlist->lh_TailPred != (struct Node *)listviewlist)
	{
		worknode = listviewlist->lh_Head;
		while(i < lastcode  &&  worknode)
		{
			i++;
			worknode = worknode->ln_Succ;
		}
		return(worknode);
	}
	return(0);
}

void Sort(struct List *list, char reverse, char lastconnect)
{
	register char *temp;
	struct Node *worknode, *nextnode;
	struct Node *inworknode, *innextnode;

	worknode = list -> lh_Head;
	while(worknode)
	{
		nextnode = worknode -> ln_Succ;
		if(!nextnode) break;

		inworknode = list -> lh_Head;
		while(inworknode)
		{
			innextnode = inworknode -> ln_Succ;
			if(!innextnode) goto jump;

			if(lastconnect)
			{
				if(((struct BookStruct *)inworknode->ln_Name)->last > ((struct BookStruct *)worknode->ln_Name)->last) goto swap;
			} else {
				if(reverse)
				{
					if(stricmp(inworknode->ln_Name, worknode->ln_Name) < 0) goto swap;
				} else {
					if(stricmp(inworknode->ln_Name, worknode->ln_Name) > 0)
					{
swap:						temp = inworknode->ln_Name;
						inworknode->ln_Name = worknode->ln_Name;
						worknode->ln_Name = temp;
					}
				}
			}

                	inworknode = innextnode;
		}
jump:
                worknode = nextnode;
	}
}

void AddressBook(void)
{
	struct IntuiMessage *message;
	struct Gadget *gad;
	struct BookStruct *book, *conbook;
	struct List *listviewlist;
	struct Node *worknode, *nextnode;
	UWORD lastcode = 0;
	UWORD code;
	ULONG class;
	ULONG sec, tic;
	BPTR fh;
	char readfin = FALSE;
	char ret = FALSE;
	char save = FALSE;
	char subdone = FALSE;

	listviewlist = AllocMem(sizeof(struct List), MEMF_PUBLIC|MEMF_CLEAR);
	if(!listviewlist) return;
	listviewlist->lh_TailPred = (struct Node *)listviewlist;
	listviewlist->lh_Head = (struct Node *)&listviewlist->lh_Tail;

	//Loads existing address book
	fh = Open(bookfile, MODE_OLDFILE);
	if(fh)
	{
		while(!readfin)
		{
			book = AllocMem(sizeof(struct BookStruct), MEMF_PUBLIC);
			if(book)
			{
				if(FRead(fh, book, sizeof(struct BookStruct), 1))
				{
					worknode = AllocMem(sizeof(struct Node), MEMF_PUBLIC|MEMF_CLEAR);
					if(worknode)
					{
						worknode->ln_Name = book->name;
						AddTail(listviewlist, worknode);
					} else {
						FreeMem(book, sizeof(struct BookStruct));
						readfin = TRUE;
					}
				} else {
					FreeMem(book, sizeof(struct BookStruct));
					readfin = TRUE;
				}
			} else {
				readfin = TRUE;
			}
		}
		Close(fh);
	}

	Project0GTags[1] = (unsigned long)listviewlist;

	if(OpenProject0Window() == 0)
	{
		//GT_SetGadgetAttrs(Project0Gadgets[GD_LIST],Project0Wnd,0,GTLV_Labels,listviewlist,TAG_DONE);
		while(!subdone)
		{
			WaitPort(Project0Wnd->UserPort);
			while (message = GT_GetIMsg(Project0Wnd->UserPort))
			{
			        gad   = (struct Gadget *)message->IAddress;
				class = message->Class;
				code  = message->Code;
				GT_ReplyIMsg(message);
				switch (class)
				{
				case IDCMP_VANILLAKEY:
					switch(toupper(code))
					{
						// Directly jump to the part that manage the required button
						case 'C':
							goto connect;
						case 'E':
							goto edit;
						case 'A':
							goto add;
						case 'D':
							goto delete;
					}
					break;

				case IDCMP_CLOSEWINDOW:
					subdone = TRUE;
					break;

				// A action button has been pressed in the Address Book:
				case IDCMP_GADGETUP:
					switch(gad->GadgetID)
					{
					case GD_SORT:
						if(code == 2)
							Sort(listviewlist, 0, TRUE);
						else
							Sort(listviewlist, code, FALSE);

						GT_SetGadgetAttrs(Project0Gadgets[GD_LIST],Project0Wnd,0,GTLV_Labels,listviewlist,GTLV_Selected,lastcode,TAG_DONE);
						save = TRUE;
						break;

					case GD_LIST:
						CurrentTime(&sec, &tic);
						if(DoubleClick(lastsec, lasttic, sec, tic) && lastcode == code)
						{
							lastcode = code;
							goto connect;
						}
						lastsec = sec;
						lasttic = tic;
						lastcode = code;
						break;

					case GD_CONNECT:
connect:
						if(worknode = FindNode(listviewlist, lastcode))
						{
							conbook = (struct BookStruct *)worknode->ln_Name;
							subdone = TRUE;
							ret = TRUE;
							save = TRUE;
						}
						break;

					case GD_DELETE:
delete:
						if(worknode = FindNode(listviewlist, lastcode))
						{
							mysprintf(buf, "Delete \042%s\042?", worknode->ln_Name);
							// Open a confirmation Dialog :
							if(rtEZRequestA(buf, "Delete|Cancel", NULL, NULL, (struct TagItem *)&tags))
							{
								Remove(worknode);
								FreeMem(worknode->ln_Name, sizeof(struct BookStruct));
								FreeMem(worknode, sizeof(struct Node));
								lastcode--;
								// BF: Why this test? lastcode is UWORD, how could it be negative?
								if(lastcode < 0) lastcode = 0;
								GT_SetGadgetAttrs(Project0Gadgets[GD_LIST],Project0Wnd,0,GTLV_Labels,listviewlist,GTLV_Selected,lastcode,TAG_DONE);
								save = TRUE;
							}
						}
						break;

					case GD_EDIT:
edit:
						if(worknode = FindNode(listviewlist, lastcode))
						{
							if(EditProfile((struct BookStruct *)worknode->ln_Name))
							{
								GT_SetGadgetAttrs(Project0Gadgets[GD_LIST],Project0Wnd,0,GTLV_Labels,listviewlist,GTLV_Selected,lastcode,TAG_DONE);
								save = TRUE;
							}
						}
						break;

					case GD_NEW:
add:
						book = AllocMem(sizeof(struct BookStruct), MEMF_PUBLIC|MEMF_CLEAR);
						if(book)
						{
							strcpy(book->name, "*new site*");
							strcpy(book->host, "*ip/host here*");
							book->port = 23;
							if(EditProfile(book))
							{
								worknode = AllocMem(sizeof(struct Node), MEMF_PUBLIC|MEMF_CLEAR);
								if(worknode)
								{
									worknode->ln_Name = book->name;
									AddTail(listviewlist, worknode);
									GT_SetGadgetAttrs(Project0Gadgets[GD_LIST],Project0Wnd,0,GTLV_Labels,listviewlist,GTLV_Selected,lastcode,TAG_DONE);
									save = TRUE;
								} else
									FreeMem(book, sizeof(struct BookStruct));
							} else
								FreeMem(book, sizeof(struct BookStruct));
						}
						break;
					}
				}
			}
		}
	}

	if ( Project0Wnd ) CloseWindow( Project0Wnd );

	if ( Project0GList ) FreeGadgets( Project0GList );

	if(ret)
	{
		cport = conbook->port;
		if(Connect_To_Server(conbook->host, conbook->port) == 0)
		{
			conbook->last = mytime();
			strcpy(username, conbook->username);
			strcpy(password, conbook->password);
		}
	}

	if(save) fh = Open(bookfile, MODE_NEWFILE); else fh = 0;

        worknode = listviewlist -> lh_Head;
	while(1)
        {
                nextnode = worknode -> ln_Succ;
                if(!nextnode) break;

		if(fh) FWrite(fh, worknode->ln_Name, sizeof(struct BookStruct), 1);

		FreeMem(worknode->ln_Name, sizeof(struct BookStruct));
                FreeMem(worknode, sizeof(struct Node));
                worknode = nextnode;
        }
        FreeMem(listviewlist, sizeof(struct List));
	if(fh) Close(fh);
}



struct Window         *Project1Wnd;
struct Gadget         *Project1GList;
struct Gadget         *Project1Gadgets[8];
#define Project1Width 450
#define Project1Height 103

UBYTE Project1GTypes[] = {
	STRING_KIND,
	STRING_KIND,
	TEXT_KIND,
	BUTTON_KIND,
	BUTTON_KIND,
	INTEGER_KIND,
	STRING_KIND,
	STRING_KIND
};

struct MyNewGadget Project1NGad[] = {
	120, 5, 317, 13, (UBYTE *)"_Site Name:",
	120, 21, 317, 13, (UBYTE *)"_Address:",
	121, 37, 177, 13, (UBYTE *)"Last Called:",
	3, 88, 101, 13, (UBYTE *)"_Ok",
	345, 88, 101, 13, (UBYTE *)"_Cancel",
	365, 37, 72, 13, (UBYTE *)"_Port:",
	120, 53, 317, 13, (UBYTE *)"_Username:",
	120, 68, 317, 13, (UBYTE *)"Pass_word:",
};

ULONG Project1GTags[] = {
	GTST_String, 0, (GTST_MaxChars), 31, (GT_Underscore), '_', (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 51, (GT_Underscore), '_', (TAG_DONE),
	GTTX_Text, 0, (GTTX_Border), TRUE, (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE),
	(GTIN_Number), 0, (GTIN_MaxChars), 9, (GT_Underscore), '_', (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 41, (GT_Underscore), '_', (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 41, (GT_Underscore), '_', (TAG_DONE)
};

int __inline OpenProject1Window( void )
{
	struct Gadget	*g;
	UWORD		ww, wh;
	long x,y;

	ComputeFont( Project1Width, Project1Height );

	ww = ComputeX( Project1Width );
	wh = ComputeY( Project1Height );

	if ( ! ( g = CreateContext( &Project1GList )))
		return( 1L );

	if(MakeGadgets(Project1NGad, Project1Gadgets, Project1GTags, g, Project1GTypes, Project1_CNT) != 0) return( 2L );

	x = ww + OffX + scr->WBorRight;
	y = wh + OffY + scr->WBorBottom;

	nwin.LeftEdge = (scr->Width - x) / 2;
	nwin.TopEdge = (scr->Height - y) / 2;
	nwin.Width = x;
	nwin.Height = y;
	nwin.IDCMPFlags = STRINGIDCMP|TEXTIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY;
	nwin.Flags = WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP;
	nwin.FirstGadget = Project1GList;
	nwin.Title = "Edit Address Book Profile";

	Project1Wnd = OpenWindow(&nwin);
	if(!Project1Wnd) return( 4L );

	/*if ( ! ( Project1Wnd = OpenWindowTags( NULL,
				WA_Left,	(scr->Width - x) / 2,
				WA_Top,		(scr->Height - y) / 2,
				WA_Width,	x,
				WA_Height,	y,
				WA_IDCMP,	STRINGIDCMP|TEXTIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY,
				WA_Flags,	WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP,
				WA_Gadgets,	Project1GList,
				WA_Title,	"Edit Address Book Profile",
				WA_CustomScreen,	scr,
				TAG_DONE )))
	return( 4L );*/

	GT_RefreshWindow( Project1Wnd, NULL );

	ComputeFont( Project1Width, Project1Height );

	DrawBevelBox( Project1Wnd->RPort, OffX + ComputeX( 3 ),
					OffY + ComputeY( 1 ),
					ComputeX( 444 ),
					ComputeY( 86 ),
					GT_VisualInfo, vi, TAG_DONE );
	return( 0L );
}

void myctime(long secs, char *outbuf)
{
	char buf1[16];
	struct DateTime tostr;

	if(secs == 0)
	{
		strcpy(outbuf, "Never");
		return;
	}

	secs -= 252460800;

	tostr.dat_Stamp.ds_Days = secs / 86400;
	tostr.dat_Stamp.ds_Minute = (secs / 60) % 1440;
	tostr.dat_Stamp.ds_Tick =	secs
					- (60*tostr.dat_Stamp.ds_Minute)
					- (86400*tostr.dat_Stamp.ds_Days);
	tostr.dat_Stamp.ds_Tick *= 50;
	tostr.dat_Format	= FORMAT_DOS;
	tostr.dat_StrDay	= 0;
	tostr.dat_StrDate	= outbuf;
	tostr.dat_StrTime	= &buf1[1];
	tostr.dat_Flags		= 0;

	DateToStr(&tostr);
	buf1[0] = ' ';
	strcat(outbuf, buf1);
}

char EditProfile(struct BookStruct *book)
{
	char lasttime[32];
	struct IntuiMessage *message;
	struct Gadget *gad;
	ULONG class;
	UWORD code;
	char subdone = FALSE;
	char ret;

	Project1GTags[1] = (unsigned long)book->name;
	Project1GTags[8] = (unsigned long)book->host;
	myctime(book->last, lasttime);
	Project1GTags[15] = (unsigned long)lasttime;
	Project1GTags[26] = (unsigned long)book->port;
	Project1GTags[33] = (unsigned long)book->username;
	Project1GTags[40] = (unsigned long)book->password;

	if(OpenProject1Window() == 0)
	{
		rtSetWaitPointer(Project0Wnd);
		ActivateGadget(Project1Gadgets[GD_SITE], Project1Wnd, 0);
		while(!subdone)
		{
			WaitPort(Project1Wnd->UserPort);
			while (message = GT_GetIMsg(Project1Wnd->UserPort))
			{
				register struct Gadget *vgad = 0;
			        gad   = (struct Gadget *)message->IAddress;
				class = message->Class;
				code  = message->Code;
				GT_ReplyIMsg(message);
				switch (class)
				{
				case IDCMP_CLOSEWINDOW:
					subdone = TRUE;
					ret = FALSE;
					break;

				case IDCMP_VANILLAKEY:
					switch(toupper(code))
					{
						case 'O':
							goto ok;
						case 'C':
							goto cancel;
						case 'S':
							vgad = Project1Gadgets[GD_SITE];
							break;
						case 'A':
							vgad = Project1Gadgets[GD_ADDRESS];
							break;
						case 'P':
							vgad = Project1Gadgets[GD_PORT];
							break;
						case 'U':
							vgad = Project1Gadgets[GD_USERNAME];
							break;
						case 'W':
							vgad = Project1Gadgets[GD_PASSWORD];
							break;
					}
					if(vgad) ActivateGadget(vgad, Project1Wnd, 0);
					break;

				case IDCMP_GADGETUP:
					switch(gad->GadgetID)
					{
					case GD_USERNAME:
						strcpy(book->username, ((struct StringInfo *)gad->SpecialInfo)->Buffer);
 						ActivateGadget(Project1Gadgets[GD_PASSWORD], Project1Wnd, 0);
						break;
					case GD_PASSWORD:
						strcpy(book->password, ((struct StringInfo *)gad->SpecialInfo)->Buffer);
 						//ActivateGadget(Project1Gadgets[GD_SITE], Project1Wnd, 0);
						break;
					case GD_PORT:
						book->port = ((struct StringInfo *)gad->SpecialInfo)->LongInt;
 						ActivateGadget(Project1Gadgets[GD_USERNAME], Project1Wnd, 0);
						break;
					case GD_SITE:
						strcpy(book->name, ((struct StringInfo *)gad->SpecialInfo)->Buffer);
						ActivateGadget(Project1Gadgets[GD_ADDRESS], Project1Wnd, 0);
						break;
					case GD_ADDRESS:
						strcpy(book->host, ((struct StringInfo *)gad->SpecialInfo)->Buffer);
						ActivateGadget(Project1Gadgets[GD_PORT], Project1Wnd, 0);
						break;
					case GD_OK:
ok:
						subdone = TRUE;
						ret = TRUE;
						break;
					case GD_CANCEL:
cancel:
						subdone = TRUE;
						ret = FALSE;
						break;
					}
				}
			}
		}
		ClearPointer(Project0Wnd);
	}

	if ( Project1Wnd ) CloseWindow( Project1Wnd );

	if ( Project1GList ) FreeGadgets( Project1GList );

	return(ret);
}


#include <intuition/imageclass.h>
#include <intuition/icclass.h>

extern struct TextAttr fontattr;
extern struct Window *sbwin;
extern struct Menu *menuStrip;
extern struct List *slist;
extern struct DrawInfo *DrawInfo;
extern long lines;

enum	{	GAD_SCROLLER,
		GAD_UP,
		GAD_DOWN
	};

APTR UpImage, DownImage;
APTR UpArrow, DownArrow;
APTR Scroller;

void CloseScrollBack(void)
{
	if(sbwin)
	{
		ClearMenuStrip(sbwin);
		CloseWindow(sbwin);
		DisposeObject(Scroller);
		DisposeObject(UpArrow);
		DisposeObject(DownArrow);
		DisposeObject(UpImage);
		DisposeObject(DownImage);
		sbwin = 0;
	}
}

void RefreshListView(UWORD top)
{
	register struct RastPort *rp = sbwin->RPort;
	struct Node *node = FindNode(slist, top);
	UWORD WWinTop = rp->Font->tf_YSize + scr->WBorTop + 2;
	UWORD y, i = 0;
	char print = TRUE;

	Move(rp, 5, WWinTop);
	WWinTop += rp->Font->tf_YSize;
	SetAPen(rp, DrawInfo->dri_Pens[TEXTPEN]);
	while(1)
	{
		UWORD chars = (sbwin->Width - 28) / rp->Font->tf_XSize;
		UWORD len;

		if(print)
		{
			if(!node->ln_Succ)
				print = FALSE;
			else {
				len = strlen(node->ln_Name);
				if(chars > len) chars = len;
			}
		}

		y = WWinTop + (i * rp->Font->tf_YSize);

		if(y > (sbwin->Height-5)) break;

		Move(rp, 5, y);
		if(print) Text(rp, node->ln_Name, chars);
		EraseRect(rp, rp->cp_x, (rp->cp_y-rp->Font->tf_YSize)+2, sbwin->Width - 24, rp->cp_y+1);

		if(print) node = node->ln_Succ;
		i++;
	}
}

void OpenScrollBack(UWORD sel)
{
	STATIC struct TagItem ArrowMappings[] =
	{
		GA_ID,	GA_ID,
		TAG_END
	};

	ULONG ArrowHeight;
	LONG SizeType;
	Object *SizeImage;

	if(prefs.sb_width > scr->Width) prefs.sb_width = scr->Width;
	if(prefs.sb_height > scr->Height) prefs.sb_height = scr->Height;

	if(scr->Flags & SCREENHIRES)
		SizeType = SYSISIZE_MEDRES;
	else
		SizeType = SYSISIZE_LOWRES;

	/*
	 NewObject() allows an arbitrary number of tags. It is a varargs stub for NewObjectA().
	 You specify a class either as a pointer (for a private class) or by its ID string (for public
	 classes).  If the class pointer is NULL, then the classID is used.
	*/
	if(SizeImage = NewObject(NULL,SYSICLASS,  // class
		SYSIA_Size,	SizeType,                 // 1st  tag (= key/value pair = property)
		SYSIA_Which,	SIZEIMAGE,            // 2nd  tag
		SYSIA_DrawInfo,	DrawInfo,             // ...
	TAG_DONE))                                // terminator tag
	{
		ULONG SizeWidth, SizeHeight;

		GetAttr(IA_Width, SizeImage, &SizeWidth);
		GetAttr(IA_Height, SizeImage, &SizeHeight);

		DisposeObject(SizeImage);

		if(UpImage = NewObject(NULL, SYSICLASS,
			SYSIA_Size,	SizeType,
			SYSIA_Which,	UPIMAGE,
			SYSIA_DrawInfo,	DrawInfo,
		TAG_DONE))
		{
			GetAttr(IA_Height, UpImage, &ArrowHeight);

			if(DownImage = NewObject(NULL, SYSICLASS,
				SYSIA_Size,	SizeType,
				SYSIA_Which,	DOWNIMAGE,
				SYSIA_DrawInfo,	DrawInfo,
			TAG_DONE))
			{
				if(Scroller = NewObject(NULL, PROPGCLASS,
					GA_ID,		GAD_SCROLLER,
					GA_Top,		scr->WBorTop + scr->Font->ta_YSize + 2,
					GA_RelHeight,	-(scr->WBorTop + scr->Font->ta_YSize + 2 + SizeHeight + 1 + 2 * ArrowHeight),
					GA_Width,	SizeWidth - 8,
					GA_RelRight,	-(SizeWidth - 5),
					GA_Immediate,	TRUE,
					GA_FollowMouse,	TRUE,
					GA_RelVerify,	TRUE,
					GA_RightBorder,	TRUE,
					PGA_Freedom,	FREEVERT,
					PGA_NewLook,	TRUE,
					PGA_Borderless,	TRUE,
					PGA_Top,	sel,
					PGA_Visible,	(prefs.sb_height - (prefs.fontsize + scr->WBorTop + 2)) / prefs.fontsize,
					PGA_Total,	lines,
				TAG_DONE))
				{
					if(UpArrow = NewObject(NULL, BUTTONGCLASS,
						GA_ID,		GAD_UP,
						GA_Image,	UpImage,
						GA_RelRight,	-(SizeWidth - 1),
						GA_RelBottom,	-(SizeHeight - 1 + 2 * ArrowHeight),
						GA_Height,	ArrowHeight,
						GA_Width,	SizeWidth,
						GA_Previous,	Scroller,
						GA_RightBorder,	TRUE,
						ICA_TARGET,	ICTARGET_IDCMP,
						ICA_MAP,	ArrowMappings,
					TAG_DONE))
					{
						if(DownArrow = NewObject(NULL, BUTTONGCLASS,
							GA_ID,		GAD_DOWN,
							GA_Image,	DownImage,
							GA_RelRight,	-(SizeWidth - 1),
							GA_RelBottom,	-(SizeHeight - 1 + ArrowHeight),
							GA_Height,	ArrowHeight,
							GA_Width,	SizeWidth,
							GA_Previous,	UpArrow,
							GA_RightBorder,	TRUE,
							ICA_TARGET,	ICTARGET_IDCMP,
							ICA_MAP,	ArrowMappings,
						TAG_DONE))
						{
							memcpy(&nwin, &prefs.sb_left, 8);
							nwin.IDCMPFlags = IDCMP_IDCMPUPDATE | LISTVIEWIDCMP | IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_CLOSEWINDOW | BUTTONIDCMP | IDCMP_RAWKEY;
							nwin.Flags = WFLG_NOCAREREFRESH | WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
							nwin.FirstGadget = Scroller;
							nwin.Title = "Scroll Back:  F1 - Clear  F3 - Print  F5 - Save";
							nwin.MinWidth = 180;
							nwin.MinHeight = 50;
							nwin.MaxWidth = 1600;
							nwin.MaxHeight = 1200;
							CheckDimensions(&nwin);
							sbwin = OpenWindow(&nwin);
							/*sbwin = OpenWindowTags(NULL,
								WA_Title,		"Scroll Back:  F1 - Clear  F3 - Print  F5 - Save",
								WA_Left,		prefs.sb_left,
								WA_Top,			prefs.sb_top,
								WA_Width,		prefs.sb_width,
								WA_Height,		prefs.sb_height,
								WA_MinHeight,		50,
								WA_MinWidth,		200,
								WA_MaxHeight,		1200,
								WA_MaxWidth,		1600,
								WA_CustomScreen,	scr,
								WA_Gadgets,		Scroller,
								WA_IDCMP,		IDCMP_IDCMPUPDATE | LISTVIEWIDCMP | IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_CLOSEWINDOW | BUTTONIDCMP | IDCMP_RAWKEY,
								WA_Flags,		WFLG_NOCAREREFRESH | WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET,
								TAG_END);*/
							if(sbwin)
							{
								//GT_RefreshWindow(sbwin, NULL);
								RefreshListView(sel);
								ResetMenuStrip(sbwin, menuStrip);
							}
						}
					}
				}
			}
		}
	}
}


#include "fkey.h"

struct Window         *Project2Wnd;
struct Gadget         *Project2GList;
struct Gadget         *Project2Gadgets[13];
#define Project2Width 503
#define Project2Height 199


UBYTE *MOD0Labels[] = {
	(UBYTE *)"None",
	(UBYTE *)"Shift",
	NULL };

UBYTE Project2GTypes[] = {
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	STRING_KIND,
	CYCLE_KIND,
	BUTTON_KIND,
	BUTTON_KIND
};


struct MyNewGadget Project2NGad[] = {
	43, 21, 443, 15, (UBYTE *)"F1:",
	43, 37, 443, 15, (UBYTE *)"F2:",
	43, 53, 443, 15, (UBYTE *)"F3:",
	43, 69, 443, 15, (UBYTE *)"F4:",
	43, 85, 443, 15, (UBYTE *)"F5:",
	43, 101, 443, 15, (UBYTE *)"F6:",
	43, 117, 443, 15, (UBYTE *)"F7:",
	43, 133, 443, 15, (UBYTE *)"F8:",
	43, 149, 443, 15, (UBYTE *)"F9:",
	43, 165, 443, 15, (UBYTE *)"F0:",
	171, 4, 187, 14, (UBYTE *)"Modifier:",
	7, 183, 101, 14, (UBYTE *)"_Save",
	394, 183, 101, 14, (UBYTE *)"_Cancel",
};

ULONG Project2GTags[] = {
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	GTST_String, 0, (GTST_MaxChars), 151, (TAG_DONE),
	(GTCY_Labels), (ULONG)&MOD0Labels[ 0 ], (GA_Disabled), TRUE, (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE),
	(GT_Underscore), '_', (TAG_DONE)
};

int __inline OpenProject2Window( void )
{
	struct Gadget	*g;
	UWORD		ww, wh;
	long x, y;

	ComputeFont( Project2Width, Project2Height );

	ww = ComputeX( Project2Width );
	wh = ComputeY( Project2Height );

	if ( ! ( g = CreateContext( &Project2GList )))
		return( 1L );

	if(MakeGadgets(Project2NGad, Project2Gadgets, Project2GTags, g, Project2GTypes, Project2_CNT) != 0) return( 2L );

	x = ww + OffX + scr->WBorRight;
	y = wh + OffY + scr->WBorBottom;

	/*nwin.LeftEdge = (scr->Width - x) / 2;
	nwin.TopEdge = (scr->Height - y) / 2;
	nwin.Width = x;
	nwin.Height = y;
	nwin.IDCMPFlags = CYCLEIDCMP|STRINGIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY;
	nwin.Flags = WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP;
	nwin.FirstGadget = Project2GList;
	nwin.Title = "Function Keys";

	CheckDimensions(&nwin);

	Project2Wnd = OpenWindow(&nwin);
	if(!Project2Wnd) return( 4L );*/

	if ( ! ( Project2Wnd = OpenWindowTags( NULL,
				WA_Left,	(scr->Width - x) / 2,
				WA_Top,		(scr->Height - y) / 2,
				WA_Width,	x,
				WA_Height,	y,
				WA_IDCMP,	CYCLEIDCMP|STRINGIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY,
				WA_Flags,	WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP,
				WA_Gadgets,	Project2GList,
				WA_Title,	"Function Keys",
				WA_CustomScreen,scr,
				TAG_DONE )))
	return( 4L );

	GT_RefreshWindow( Project2Wnd, NULL );

	return( 0L );
}

extern unsigned char keys[1520];

/*
Function Keys dialog (pure GadTools implementation).

Allows the user to store strings that are sent to the Telnet server
when the corresponding function key is pressed.
*/
void FunctionKeys(void)
{
	struct IntuiMessage *message;
	struct Gadget *gad;
	ULONG class;
	UWORD code;
	char subdone = FALSE;
	char save = FALSE;

	memcpy(buf, keys, 1520);

	Project2GTags[1] = (ULONG)buf;
	Project2GTags[6] = (ULONG)&buf[152];
	Project2GTags[11] = (ULONG)&buf[152*2];
	Project2GTags[16] = (ULONG)&buf[152*3];
	Project2GTags[21] = (ULONG)&buf[152*4];
	Project2GTags[26] = (ULONG)&buf[152*5];
	Project2GTags[31] = (ULONG)&buf[152*6];
	Project2GTags[36] = (ULONG)&buf[152*7];
	Project2GTags[41] = (ULONG)&buf[152*8];
	Project2GTags[46] = (ULONG)&buf[152*9];

	if(OpenProject2Window() == 0)
	{
		while(!subdone)
		{
			WaitPort(Project2Wnd->UserPort);
			while (message = GT_GetIMsg(Project2Wnd->UserPort))
			{
			        gad   = (struct Gadget *)message->IAddress;
				class = message->Class;
				code  = message->Code;
				GT_ReplyIMsg(message);
				switch (class)
				{
				case IDCMP_CLOSEWINDOW:
					subdone = TRUE;
					break;

				case IDCMP_VANILLAKEY:
					switch(toupper(code))
					{
						case 'O':
							save = TRUE;
						case 'C':
							subdone = TRUE;
					}
					break;

				case IDCMP_GADGETUP:
					switch(gad->GadgetID)
					{
					case GD_SAVEE:
						save = TRUE;
					case GD_CANCELL:
						subdone = TRUE;
						break;
					}
					if(gad->GadgetID < 10)
					{
						strcpy(&buf[gad->GadgetID*152], ((struct StringInfo *)gad->SpecialInfo)->Buffer);
					}
				}
			}
		}
	}

	if ( Project2Wnd ) CloseWindow( Project2Wnd );

	if ( Project2GList ) FreeGadgets( Project2GList );

	if(save)
	{
		register BPTR fh = Open(keysfile, MODE_NEWFILE);
		if(fh)
		{
			Write(fh, buf, 1520);
			Close(fh);
		}
		memcpy(keys, buf, 1520);
		//CopyMem(buf, keys, 1520);
	}
}


#define BUTTONS 7

char *icons[] = {	"Connect",
			"Disconnect",
			"AddressBook",
			"Information",
			"Upload",
			"Download",
			"Quit"			};

struct DiskObject *dob[BUTTONS];
extern struct TextFont *ansifont;
extern UBYTE wb;
extern void SimpleReq(char *str);


void CheckDimensions(struct NewWindow *newwin)
{
	if(newwin->Width > scr->Width) newwin->Width = scr->Width;
	if(newwin->Height > scr->Height) newwin->Height = scr->Height;

	if(newwin->LeftEdge + newwin->Width > scr->Width) newwin->LeftEdge = 0;
	if(newwin->TopEdge + newwin->Height > scr->Height) newwin->TopEdge = 0;
}

void CloseToolWindow(void)
{
	if(twin)
	{
		register struct MenuItem *item;
 		register UWORD i;

		ClearMenuStrip(twin);
		CloseWindow(twin);
		twin = 0;

		for(i=0; i<BUTTONS; i++)
		{
			if(dob[i]) FreeDiskObject(dob[i]);
		}

		item = ItemAddress(menuStrip, FULLMENUNUM(3, 10, 0));
		if(item) item->Flags &= ~CHECKED;
	}
}

void OpenToolWindow(char setmenus)
{
	if(!twin)
	{
		register struct Gadget *firstgad = 0;
		register struct Gadget *gad = 0;
		UWORD nextleft = scr->WBorLeft + 1, maxheight = 0, i = 0;
		WORD wintop, spacing = 5;

		if(wb)
			wintop = WinTop;
		else
			wintop = 0;

		do
		{
			if(wb)
				strcpy(buf, "PROGDIR:WBIcons/");
			else
				strcpy(buf, "PROGDIR:SCIcons/");

			strcat(buf, icons[i]);
			dob[i] = GetDiskObjectNew(buf);
			if(dob[i])
			{
				if(gad) gad->NextGadget = &dob[i]->do_Gadget;
				gad = &dob[i]->do_Gadget;
				gad->NextGadget = 0;
				gad->LeftEdge = nextleft;
				gad->TopEdge = wintop + 1;
				if(gad->SelectRender == 0)
					gad->Flags = GFLG_GADGIMAGE | GFLG_GADGHCOMP;
				else
					gad->Flags = GFLG_GADGIMAGE | GFLG_GADGHIMAGE;
				gad->Activation = GACT_RELVERIFY;
				gad->GadgetType = GTYP_BOOLGADGET;
				//gad->GadgetText = 0;
				gad->GadgetID = i;
				gad->UserData = (APTR)dob[i]->do_ToolTypes[0];
				nextleft = gad->LeftEdge + gad->Width + spacing;
				if(!firstgad) firstgad = gad;
				if(gad->Height > maxheight) maxheight = gad->Height;
			}
			i++;
		}
		while(i != BUTTONS);

		if(!firstgad)
		{
			SimpleReq("No icons available.");
			prefs.flags &= ~(1<<10);
			return;
		}

		if(wb)
		{
			memcpy(&nwin, &prefs.twin_left, 4);

			nwin.Width = gad->LeftEdge + gad->Width + scr->WBorRight + 1;
			nwin.Height = maxheight + wintop + scr->WBorBottom + 3 + scr->RastPort.Font->tf_YSize;
			nwin.Flags = WFLG_NOCAREREFRESH|WFLG_NEWLOOKMENUS|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET;
			nwin.IDCMPFlags = IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_GADGETUP;
			nwin.Title = "Tool Window";
		} else {

			spacing = scr->Width / i;
			nextleft = (scr->Width - ((scr->Width - spacing) + gad->Width)) / 2;
			i = 0;
			gad = firstgad;
			while(gad)
			{
				gad->LeftEdge = nextleft + (spacing * i);
				gad = gad->NextGadget;
				i++;
			}

			nwin.LeftEdge = 0;
			if(prefs.flags&(1<<0))
				nwin.TopEdge = 0;
			else
				nwin.TopEdge = prefs.fontsize + 3;

			nwin.Width = scr->Width;
			nwin.Height = maxheight + scr->RastPort.Font->tf_YSize + 4;
			nwin.Flags = WFLG_NOCAREREFRESH|WFLG_NEWLOOKMENUS|WFLG_BACKDROP|WFLG_BORDERLESS;
			nwin.IDCMPFlags = IDCMP_MENUPICK | IDCMP_GADGETUP | IDCMP_RAWKEY;
			nwin.Title = 0;
		}
		nwin.FirstGadget = firstgad;

		CheckDimensions(&nwin);

		twin = OpenWindow(&nwin);
		if(twin)
		{
			if(setmenus) ResetMenuStrip(twin, menuStrip);
			SetFont(twin->RPort, scr->RastPort.Font);
			SetAPen(twin->RPort, DrawInfo->dri_Pens[TEXTPEN]);
			gad = firstgad;
			while(gad)
			{
				register UWORD len = strlen((char *)gad->UserData);

				Move(twin->RPort, gad->LeftEdge + ((gad->Width - (len*scr->RastPort.Font->tf_XSize)) / 2), wintop + maxheight + scr->RastPort.Font->tf_YSize - 1);
				Text(twin->RPort, (char *)gad->UserData, len);

				gad = gad->NextGadget;
			}
			if(!wb)
			{
				SetAPen(twin->RPort, DrawInfo->dri_Pens[SHINEPEN]);
				Move(twin->RPort, 0, twin->Height-2);
				Draw(twin->RPort, twin->Width, twin->Height-2);
				SetAPen(twin->RPort, DrawInfo->dri_Pens[FILLPEN]);
				Move(twin->RPort, 0, twin->Height-1);
				Draw(twin->RPort, twin->Width, twin->Height-1);
			}
		}
	}
}
