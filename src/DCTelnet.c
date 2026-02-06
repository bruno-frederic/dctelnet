/* ====================================================================== */
/* ============================= DC TELNET ============================== */
/* ====================================================================== */


#define DCTELNET_VERSION "1.8-dev"
static const char __ver[] = "$VER: DCTelnet " DCTELNET_VERSION " " __AMIGADATE__;

static char MainWindowTitle[] =
  "DCTelnet " DCTELNET_VERSION " " __AMIGADATE__ " - A classic Amiga Telnet/BBS client";

#define __USE_SYSBASE

#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/diskfont.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/keymap.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <libraries/reqtools.h>
#include <proto/reqtools.h>           // rtAllocRequestA() rtScreenModeRequest() rtPaletteRequestA()
#include <sys/errno.h>
#include <netdb.h>
#include <proto/socket.h>
#include <xem.h>
#include <xem_proto.h>


// Stringify macro :
#define STR_(x) #x
#define STR(x) STR_(x)

struct NewMenu mynewmenu[] =
    {
        { NM_TITLE, "DC Telnet",	 0 , 0, 0, 0,},
        {  NM_ITEM, "About",		"A", 0, 0, 0,},
        {  NM_ITEM, NM_BARLABEL,	 0 , 0, 0, 0,},
        {  NM_ITEM, "Scroll Back",	"X", 0, 0, 0,},
        {  NM_ITEM, "Iconify",	 	"&", 0, 0, 0,},
        {  NM_ITEM, "Speed Test",	"Y", 0, 0, 0,},
        {  NM_ITEM, "Finger",		"@", 0, 0, 0,},
        {  NM_ITEM, NM_BARLABEL,	 0 , 0, 0, 0,},
        {  NM_ITEM, "Quit",		"Q", 0, 0, 0,},

        { NM_TITLE, "Transfer",		 0 , 0, 0, 0,},
        {  NM_ITEM, "Upload",       	"U", 0, 0, 0,},
        {  NM_ITEM, NM_BARLABEL,	 0 , 0, 0, 0,},
        {  NM_ITEM, "Download",		"D", 0, 0, 0,},
        {  NM_ITEM, NM_BARLABEL,	 0 , 0, 0, 0,},
        {  NM_ITEM, "ASCII Send",	"%", 0, 0, 0,},

        { NM_TITLE, "Connection",	 0 , 0, 0, 0,},
        {  NM_ITEM, "Connect",		"M", 0, 0, 0,},
        {  NM_ITEM, "Connect Thread",	"G", 0, 0, 0,},
        {  NM_ITEM, "Disconnect",	"H", 0, 0, 0,},
        {  NM_ITEM, NM_BARLABEL,	 0 , 0, 0, 0,},
        {  NM_ITEM, "Address Book",	"B", 0, 0, 0,},
        {  NM_ITEM, NM_BARLABEL,	 0 , 0, 0, 0,},
        {  NM_ITEM, "Information",	"^", 0, 0, 0,},

        { NM_TITLE, "Options",	 	 0 , 0, 0, 0,},		// 23
        {  NM_ITEM, "Use Workbench",	"W", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Disable LEDs",	"I", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Hide TitleBar",	"R", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "CRLF Correction",	"L", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "BS/DEL Swap",	"/", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Disable Scroll-B",	"E", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Strip Colour",	"J", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Simple Telnet",	"1", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Packet Window",	"2", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Use XEM Library",	"3", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Tool Window",	"4", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Return = CR + LF",	"5", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Local Echoback",	"6", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Raw Connection",	"7", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
        {  NM_ITEM, "Jump Scroll",	"8", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},

        { NM_TITLE, "Settings",		 0 , 0, 0, 0,},
        {  NM_ITEM, "Screen Mode..",	"S", 0, 0, 0,},
        {  NM_ITEM, "Screen Font..",	"F", 0, 0, 0,},
        {  NM_ITEM, "Screen Palette..",	"-", 0, 0, 0,},
        {  NM_ITEM, "Download Path..",	"O", 0, 0, 0,},
        {  NM_ITEM, "Transfer Protocol..","T", 0, 0, 0,},
        {  NM_ITEM, "Protocol Init..",	"*", 0, 0, 0,},
        {  NM_ITEM, "Function Keys..",	"K", 0, 0, 0,},
        {  NM_ITEM, "XEM Library..",	"#", 0, 0, 0,},
        {  NM_ITEM, "Telnet Display ID..","9", 0, 0, 0,},
        {  NM_ITEM, "ScrollBack Lines..","0", 0, 0, 0,},
	{  NM_ITEM, "Snapshot Windows",	"$", 0, 0, 0,},

        { NM_TITLE, "Login",		 0 , 0, 0, 0,},
        {  NM_ITEM, "Send Username",	"N", 0, 0, 0,},
        {  NM_ITEM, "Send Password",	"P", 0, 0, 0,},

        {   NM_END, NULL,		 0 , 0, 0, 0,},
    };

UWORD Connect_To_Server(char *servername, UWORD port);
void ConWrite(char *data, long len);
void LocalPrint(char *data);
void GetWindowMsg(struct Window *wwin);
void CloseDisplay(char);
char OpenDisplay(char);
void OpenIcon(void);
void CloseIcon(void);
void LEDs(void);

extern void CloseScrollBack(void);
extern void OpenScrollBack(UWORD sel);
extern void FunctionKeys(void);
extern void Upload(char *library);
extern void Download(char *library);
extern void AddressBook(void);
extern long mytime(void);
extern void RefreshListView(UWORD top);
extern void OpenToolWindow(char setmenus);
extern void CloseToolWindow(void);
extern void CheckDimensions(struct NewWindow *newwin);

extern LONG __saveds __asm xpr_swrite(register __a0 UBYTE *buffer, register __d0 LONG size);
extern LONG __saveds __asm xpr_sread(register __a0 UBYTE *buffer, register __d0 ULONG size, register __d1 ULONG timeout);
extern LONG __saveds __asm xpr_sflush(void);
extern LONG __asm xpr_gets(register __a0 STRPTR Prompt,register __a1 STRPTR Buffer);

extern APTR Scroller;
extern struct ExecBase *SysBase;
struct ReqToolsBase *ReqToolsBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *KeymapBase, *XEmulatorBase, *GadToolsBase, *SocketBase;
struct Library *DiskfontBase, *IconBase, *WorkbenchBase, *UtilityBase;
struct Window *win, *pwin, *sbwin, *twin;
struct List *slist;
struct Screen *scr;
struct DrawInfo *DrawInfo;
struct Gadget window_back;
struct NewGadget ng;
struct TextAttr fontattr;
struct TextFont *ansifont;
struct IOStdReq writeio;
struct MsgPort *WriteConPort;
struct Menu *menuStrip;
struct DiskObject *dobj;
struct MsgPort *iconport;
struct AppIcon *dcicon;
struct hostent *HostAddr;
struct sockaddr_in INetSocketAddr;
struct XEM_IO xemio;
struct NewWindow nwin;

#define BUFSIZE 250
UBYTE strBuffer[BUFSIZE+2];
struct StringInfo strInfo;
struct Gadget strGad;

enum	{	GAD_SCROLLER,
		GAD_UP,
		GAD_DOWN
	};

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
	ULONG	sb_lines;	/* BIT 11 = Return = CR&LF */
	char	displayidstr[32];/* IT 12 = Local Echoback */
	UWORD	twin_left,	/* BIT 13 = Raw Connection */
		twin_top;	/* BIT 14 = Jump Scroll */
} prefs;

ULONG tags[5];
BPTR fh;
long jj, lines;
long contime, sok, bytes, sent;
void *vi;
char username[42], password[42];
unsigned char buf[2048], conbuf[16], scrollbuf[402], keys[1520];
char server[64];
ULONG lasttop;		// last topline of scrollback
UWORD cport = 23;	// current tcp port
UWORD WinTop;		// WinTop topEdge (titlebar height)
UBYTE done;		// program finished
UBYTE connected;	// tcp connected
UBYTE passall;		// passall telnet negotiation
UBYTE passflag;		// already sent 8bit info
UBYTE restartflag;	// prefs changed, restart
UBYTE reopenscreen;	// flag
UBYTE wb;		// running in wb
UBYTE icon;		// iconified
UBYTE doicon;		// must iconify
UBYTE unicon;		// must uniconify
UBYTE drivertype;	// drivertype 0 - normal    1 - xem library
UBYTE finger;		// finger? BOOLEAN

UWORD colorpens[]  = { 1,4,1,1,6,4,1,0,5,4,1,6,65535 };
UWORD color[] = { 0x0000, 0x0DDD, 0x00D0, 0x0DD0, 0x000D, 0x0D0D, 0x00DD, 0x0D00,
		  0x0555, 0x0FFF, 0x00F0, 0x0FF0, 0x000F, 0x0F0F, 0x00FF, 0x0F00, 65535 };
/*                 black,    white,  green, yellow, blue, purple, aqua,   red */

char prefsfile[] = "PROGDIR:DCTelnet.Prefs";
char bookfile[]  = "PROGDIR:DCTelnet.Book";
char keysfile[]  = "PROGDIR:DCTelnet.Keys";


void mysprintf(char *Buffer, char *ctl, ...)
{
	RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75", Buffer);
}

void LocalFmt(char *ctl, ...)
{
	RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75", buf);
	ConWrite(buf, strlen(buf));
}

void TextFmt(struct RastPort *rP, char *ctl, ...)
{
	RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75", buf);
	Text(rP, buf, strlen(buf));
}

long TCPSend(char *buf, long len)
{
	if(send(sok, buf, len, 0) < 0) return -1;
	sent += len;
	return len;
}

void WindowSub(void (*Sub)(void))
{
	if(sbwin) rtSetWaitPointer(sbwin);
	if(twin) rtSetWaitPointer(twin);
	rtSetWaitPointer(win);
	Sub();
	if(sbwin) ClearPointer(sbwin);
	if(twin) ClearPointer(twin);
	ClearPointer(win);
	LEDs();
}

void SimpleReq(char *str)
{
	rtEZRequestA(str, "OK", NULL, NULL, (struct TagItem *)&tags);
	LEDs();
}

void DisConnect(char remote, char quiet)
{
	if(connected)
	{
		if(!quiet && !icon)
		{
			register long spent;
			if(remote)
				LocalPrint("›m\r\nConnection closed by foreign host");
			else
				LocalPrint("›m\r\nConnection closed");
			spent = mytime() - contime;
			LocalFmt(". %02ld:%02ld:%02ld spent online.\r\n", spent/3600, (spent/60)%60, spent%60);
		}
		shutdown(sok, 2);
		CloseSocket(sok);

		connected = FALSE;
		passall = FALSE;
		passflag = FALSE;
		bytes = 0;
		sent = 0;

		if(finger)
		{
			finger = FALSE;
			OnMenu(win, FULLMENUNUM(3, -1, 0));
		}

		LEDs();
	}
}

void SavePrefs(void)
{
	fh = Open(prefsfile, MODE_NEWFILE);
	if(fh)
	{
		Write(fh, &prefs, sizeof(struct PrefsStruct));
		Close(fh);
	}
}

char ChooseScreen(char firsttime)
{
	struct rtScreenModeRequester *scrmodereq;

	if(scrmodereq = rtAllocRequestA (RT_SCREENMODEREQ, NULL))
	{
		if(firsttime)
		{
			scrmodereq->DisplayID = HIRES_KEY;//PAL_MONITOR_ID
			scrmodereq->DisplayDepth = 4;
			scrmodereq->DisplayWidth = 640;
			scrmodereq->DisplayHeight = 256;
		} else {
			//rtChangeReqAttr(scrmodereq, RTSC_ModeFromScreen, scr, TAG_END);
			scrmodereq->DisplayID = prefs.DisplayID;
			scrmodereq->DisplayWidth = prefs.DisplayWidth;
			scrmodereq->DisplayHeight = prefs.DisplayHeight;
			scrmodereq->DisplayDepth = prefs.DisplayDepth;
		}

		if (rtScreenModeRequest (scrmodereq, "Screen Mode..",
			RT_Window,	win,
			RTSC_Flags,	SCREQF_DEPTHGAD|SCREQF_SIZEGADS|SCREQF_GUIMODES,
			RTSC_MaxDepth,	4,
			TAG_END))
		{
			prefs.DisplayID = scrmodereq->DisplayID;
			prefs.DisplayWidth = scrmodereq->DisplayWidth;
			prefs.DisplayHeight = scrmodereq->DisplayHeight;
			prefs.DisplayDepth = scrmodereq->DisplayDepth;
			rtFreeRequest (scrmodereq);
			return(TRUE);
		}
		rtFreeRequest (scrmodereq);
	}
	return(FALSE);
}

char FileReq(char *dir, char *pat, char *file, char *title, char dodir, ULONG flags)
{
	struct rtFileRequester *filereq;
	char fbuf[128];

	if (filereq = rtAllocRequestA (RT_FILEREQ, NULL))
	{
		strcpy(fbuf, file);
		rtChangeReqAttr(filereq,
			RTFI_MatchPat,	pat,
			RTFI_Dir,	dir,
			TAG_END);
		if (rtFileRequest (filereq, fbuf, title,
			RT_Window,	win,
			RT_LeftOffset,	20,
			RT_TopOffset,	0,
			RTFI_Height,	600,
			RTFI_Flags,	flags,
			TAG_END))
		{
			strcpy(file, fbuf);
			if(dodir)
			{
				WORD l;
				strcpy(dir, filereq->Dir);
				l = strlen(dir) - 1;
				if(l!=-1 && dir[l] != '/' && dir[l] != ':')
				{
					l++;
					dir[l] = '/';
					l++;
					dir[l] = 0;
				}
			}
			rtFreeRequest(filereq);
			return(TRUE);
		}
		rtFreeRequest(filereq);
	}
	return(FALSE);
}

// Application-defined Exec List with Node-specific data
// https://wiki.amigaos.net/wiki/Exec_Lists_and_Queues#Finding_the_List_of_a_Node
struct Scroll
{
	struct Node	nnode;
	long		len;    // Node-specific data
};

void AddBuf(unsigned char *str, long size)
{
	register long i = 0, n;
	struct Scroll *node, *nextnode;
	char numb[32];

	while(i < size)
	{
		switch(str[i])
		{
			case 10:
add:
				scrollbuf[jj] = 0;
				jj += 2;
				node = AllocMem(sizeof(struct Scroll) + jj, MEMF_PUBLIC|MEMF_CLEAR);
				if(node)
				{
					node->nnode.ln_Name = (char *) (long)node + sizeof(struct Scroll);
					node->len = jj;
					CopyMem(scrollbuf, node->nnode.ln_Name, jj - 2);
					AddTail(slist, node);
					lines++;
				}
				if(lines > prefs.sb_lines)
				{
					lines--;
					node = (struct Scroll *)slist->lh_Head;
					nextnode = (struct Scroll *)node -> nnode.ln_Succ;
					if(nextnode)
					{
						Remove(node);
						FreeMem(node, sizeof(struct Scroll)+node->len);
					}
				} else {
					if(sbwin)
					{
						SetGadgetAttrs((struct Gadget *)Scroller, sbwin, NULL,
							PGA_Total,	lines,
						TAG_DONE);
					}
				}
				jj = 0;
				break;
			case 7:
			case 9:
			case '':
			case 13:
				break;
			case 27:
				i++;
			case 155:
				n = 0;
				i++;
				while(i < size && str[i]>='0' && str[i]<=';')
				{
					numb[n] = str[i];
					i++;
					n++;
					if(n > 30) n = 0;
				}
				switch(str[i])
				{
					case ' ':
						i++;
						break;
					case 'C':
						numb[n] = 0;
						for(n=0; n<atoi(numb); n++)
						{
							if(jj > 400) goto add;
							scrollbuf[jj] = ' ';
							jj++;
						}
						break;
					case 'H':
					case 'B':
						goto add;
				}
				break;
			default:
				if(jj > 400) goto add;
				scrollbuf[jj] = str[i];
				jj++;
				break;
		}
		i++;
	}
}

void Receive(void)
{
	register long length;

	if(passall)
	{
		if((length = recv(sok, buf, sizeof buf, 0)) < 1)
			DisConnect(TRUE, FALSE);
		else {
			char upload = FALSE;
			char download = FALSE;
			static char zm;
			UWORD i = 0, j = 0, temp;
			unsigned char *outbuf = AllocMem(length+256, MEMF_PUBLIC);
			if(!outbuf) return;

			/*fh = Open("ram:in.txt", MODE_READWRITE);
			if(fh)
			{
				Seek(fh, 0, OFFSET_END);
				Write(fh, buf, length);
				Close(fh);
			}*/

			if(((prefs.flags&5) == 0)  &&  !icon) // if bits 0 and 2 are clear
			{
				SetAPen(&scr->RastPort, 10);
				RectFill(&scr->RastPort, scr->Width-70, 3, scr->Width-62, prefs.fontsize-2);
			}

			while(i < length)
			{
				switch(buf[i])
				{
				case 0:
				case 14:
				case 128:
					break;
				case 10:
					if(!(prefs.flags&(1<<1))) goto norm;
					outbuf[j] = 13;
					j++;
					outbuf[j] = 10;
					j++;
					break;
				case 13:
					if(!(prefs.flags&(1<<1))) goto norm; // !CRLF Correction
					break;
				case 27:
					if(!(prefs.flags&(1<<6))) goto norm; // !Strip Color
					temp = i;
					i += 2;
					while(i < length && buf[i]>='0' && buf[i]<=';') i++;
					if(buf[i] != 'm')
					{
						i = temp;
						goto norm;
					}
					break;
				case 255:
					i += 2;
					break;
				case '1':
					if(zm == 5) upload = TRUE;
					goto pnorm;
				case '0':
					if(zm == 5) download = TRUE;
					if(zm == 4) zm = 5; else zm = 0;
					goto norm;
				case 'B':
					if(zm == 3) zm = 4; else zm = 0;
					goto norm;
				case '\030':
					if(zm == 2) zm = 3; else zm = 0;
					goto norm;
				case '*':
					if(zm < 3) zm++; else zm = 0;
					goto norm;
				default:
pnorm:					zm = 0;
norm:					outbuf[j] = buf[i];
					j++;
				}
				i++;
			}

			ConWrite(outbuf, j);

			if(!(prefs.flags&(1<<5))) AddBuf(outbuf, j);

			/*if(debug)
			{
				fh = Open("ram:out.txt", MODE_READWRITE);
				if(fh)
				{
					Seek(fh, 0, OFFSET_END);
					Write(fh, outbuf, j);
					Close(fh);
				}
			}*/

			if(((prefs.flags&5) == 0)  &&  !icon) EraseRect(&scr->RastPort, scr->Width-72, 2, scr->Width-60, prefs.fontsize-1);

			FreeMem(outbuf, length+256);

			if(download) Download(prefs.xferlibrary);
			if(upload) Upload(prefs.xferlibrary);

			bytes += length;
		}
	} else {

		if((recv(sok, buf, 1, 0)) < 1)
		{
			DisConnect(TRUE, FALSE);
			return;
		}

		if(buf[0] == 255  &&  !(prefs.flags&(1<<13))) // NOT RAW
		{
			if((recv(sok, &buf[1], 2, 0)) < 1)
			{
				DisConnect(TRUE, FALSE);
				return;
			}
			bytes += 2;

			if(prefs.flags&(1<<7)) // Simple Negotiation
			{
				if(buf[1] == 253)
				{
					if(!buf[2])
						passall = TRUE;
					else {
						buf[1] = 252;
						TCPSend(buf, 3);
					}
				}
			} else {
				switch(buf[1])
				{
					case 250: //SubNeg
						recv(sok, &buf[8], 3, 0);
						bytes += 3;
						if(buf[2] == 24)
						{
							//recv(sok, buf, 3, 0);
							TCPSend("\377\372\030\000", 4);
							TCPSend(prefs.displayidstr, strlen(prefs.displayidstr));
							TCPSend("\377\360", 2);
						}
						break;
					case 253: //DO
						if(!buf[2])
							passall = TRUE;
						else {
							if(buf[2] != 24) // !TERMTYPE
								buf[1] = 252; //WON'T
							else
								buf[1] = 251; //WILL
							TCPSend(buf, 3);
						}
						break;
					case 251: //WILL
						/*switch(buf[2])
						{
						case 0:
						case 1:
						case 3:
							break;
						default:
							buf[1] = 254;	// DONT
							TCPSend(buf, 3);
						}*/
						if(buf[2] > 1)
						{
							buf[1] = 254; //DON'T
							TCPSend(buf, 3);
						}
						break;
				}
			}
		} else {
			bytes++;
			if(buf[0]) ConWrite(buf, 1);
			if(bytes > 128) passall = TRUE;
		}
	}
}

void SendMisc(char *str, long len)
{
	if(len == -1) len = strlen(str);

	if(connected)
		TCPSend(str, len);
	else
		ConWrite(str, len);
}

void SendMacro(char *str)
{
	register UWORD t, i = 0, j = 0, len = strlen(str);

	while(i < len)
	{
cont:
		switch(str[i])
		{
			case '\\':
				i++;
				switch(str[i])
				{
					case 'n':
						buf[j] = '\n';
						j++;
						break;
					case 'r':
						buf[j] = '\r';
						j++;
						break;
					case 'e':
						buf[j] = 27;
						j++;
						break;
					default:
						t = 0;
						while(i + t < len)
						{
							if(str[i+t] < '0' || str[i+t] > '9') break;
							t++;
							if(t == 3)
							{
								UBYTE n = str[i+t];
								str[i+t] = 0;
								buf[j] = atoi((char *)&str[i]);
								j++;
								str[i+t] = n;
								i += 3;
								goto cont;
							}
						}
						i--;
						goto norm;
				}
				break;
			default:
norm:				buf[j] = str[i];
				j++;
		}
		i++;
	}

	SendMisc(buf, j);
}

void LEDs(void)
{
	if((prefs.flags&5) == 0  &&  !icon) // if bits 0 and 2 are clear
	{
		EraseRect(&scr->RastPort, scr->Width-72, 2, scr->Width-60, prefs.fontsize-1);
		EraseRect(&scr->RastPort, scr->Width-86, 2, scr->Width-74, prefs.fontsize-1);
		if(connected)
		{
			SetAPen(&scr->RastPort, 15);
			RectFill(&scr->RastPort, scr->Width-84, 3, scr->Width-76, prefs.fontsize-2);
		}
	}
}

/*void SpeedTest(void)
{
	long before;
	register long spent;
	register UWORD i;

	ConWrite("›0 p\233m\014", 7);

	before = mytime();

	for(i = 1; i < 201; i++) LocalFmt("Line %ld.\r\n", i);

	spent = mytime() - before;

	ConWrite("›1 p", 4);

	if(spent > 0)
	{
		//LocalFmt("›32mResult›33m: ›36m%ld ›mlines/second.\r\n", 200 / spent);
		before = 200 / spent;
		rtEZRequestA("Result: %ld lines per second.", "OK", NULL, (APTR)&before, (struct TagItem *)&tags);
	}
}*/

void SpeedTest(void)
{
	ULONG before_s, before_m;
	ULONG after_s, after_m;
	ULONG before, after;
	register UWORD i;

	ConWrite("›0 p\233m\014", 7);

	CurrentTime(&before_s, &before_m);

	for(i = 1; i < 201; i++) LocalFmt("Line %ld.\r\n", i);

	CurrentTime(&after_s, &after_m);

	ConWrite("›1 p", 4);

	before = (before_s * 1000000) + before_m;
	after  = (after_s  * 1000000) + after_m;

	after -= before;

	if(after > 0)
	{
		register char *rating;
		before = 200000000 / after;

		if(before < 20)
			rating = "Poor";
		else {
			if(before < 30)
				rating = "Average";
			else {
				if(before < 50)
					rating = "Good";
				else
					rating = "Excellent";
			}
		}

		mysprintf(buf,	"Result: %ld lines per second\n\n"
				"Rating: %s", before, rating);

		SimpleReq(buf);
	}
}

void ClearScrollBack(void)
{
	struct Scroll *worknode, *nextnode;

	worknode = (struct Scroll *)slist->lh_Head;
	while(worknode)
	{
		nextnode = (struct Scroll *)worknode -> nnode.ln_Succ;
		if(!nextnode) break;

		FreeMem(worknode, sizeof(struct Scroll)+worknode->len);
		worknode = nextnode;
	}
	slist->lh_Tail = 0;
	slist->lh_TailPred = (struct Node *)slist;
	slist->lh_Head = (struct Node *)&slist->lh_Tail;
}

void Finger(void)
{
	long oldflags;
	char tbuf[64];
	char *host;

	if(rtGetStringA(tbuf, 63, "Enter EMail Address:", 0, (struct TagItem *)&tags))
	{
		host = strchr(tbuf, '@');
		if(host)
		{
			host[0] = 0;
			*host++;
			oldflags = prefs.flags;
			prefs.flags = 1<<13;	// Raw Connection
			if(Connect_To_Server(host, 79) == 0)
			{
				mysprintf(buf, "/W %s\r\n", tbuf);
				send(sok, buf, strlen(buf), 0);
				OffMenu(win, FULLMENUNUM(3, -1, 0));
				finger = TRUE;
			}
			prefs.flags = oldflags;
		}
	}
}

int main(int argc, char *argv[])
{
	ULONG iconsig, sigmask, winsig;
	LONG i;
	struct timeval timer;
	fd_set rd;

	if(argc > 1)
	{
		if(strcmp(argv[1], "?") == 0 || strcmp(argv[1], "/?") == 0 || strcmp(argv[1], "-?") == 0 ||
		   strcmp(argv[1], "-h")  == 0 || strcmp(argv[1], "--help")  == 0)
		{
			PutStr(
		"DCTelnet "DCTELNET_VERSION" ("__DATE__") - A classic Amiga Telnet/BBS client with Zmodem\n"
		"compiled with: " STR(COMPILER_STRING) "\n"
		"\n"
		"Usage: DCTelnet <host> [<port>]\n"
		);
			return RETURN_OK;
		}

		strcpy(server, argv[1]);

		if(argc > 2) cport = atoi(argv[2]);

		/*if(argc > 3)
		{
			if(stricmp(argv[3], "debug")==0) debug = TRUE;
		}*/
	}

	if (!(ReqToolsBase = (struct ReqToolsBase *)OpenLibrary (REQTOOLSNAME, 0)))
	{
		PutStr("I need reqtools.library\n");
		return RETURN_FAIL;
	}

	fh = Open(prefsfile, MODE_OLDFILE);
	if(fh)
	{
		if(Read(fh, &prefs, sizeof(struct PrefsStruct)) < 252) // IF OLD CONFIG FILE
		{
			/*//prefs.win_left = 0;
			prefs.win_top = 11;
			prefs.win_width = 640;
			prefs.win_height = 200;
			//prefs.sb_left = 0;
			prefs.sb_top = 12;
			prefs.sb_width = 640;
			prefs.sb_height = (prefs.DisplayHeight / 2) - 4;*/

			Close(fh);
			goto fixprefs;
		}
		Close(fh);
	} else {

		icon = TRUE;

		SimpleReq(	"This is the first time you've run DCTelnet."	"\n"
										"\n"
				"You will now have to select a screen mode"	"\n"
				"for DCTelnet to open on. The recommended"	"\n"
				"mode is 640*256*16 for good ANSI emulation."	"\n"
										"\n"
				"Once DCTelnet has started, you can make"	"\n"
				"the program open a window on the Workbench"	"\n"
				"screen, instead of opening its own screen."
			);

		if(ChooseScreen(TRUE))
		{
			prefs.fontsize = 8;
			strcpy(prefs.fontname, "topaz.font");
			strcpy(prefs.xferlibrary, "xprzmodem.library");
			strcpy(prefs.xferinit, "TC,OR,B32,FO,AN,DN,KY,SN,RN");
			memcpy(&prefs.color[0], &color[0], 32);
			//CopyMem(&color[0], &prefs.color[0], 32);
			prefs.flags = 1<<10;
fixprefs:		//prefs.win_left = 0;
			prefs.win_top = 11;
			prefs.win_width = 640;
			prefs.win_height = 200;
			//prefs.sb_left = 0;
			prefs.sb_top = 12;
			prefs.sb_width = 640;
			prefs.sb_height = (prefs.DisplayHeight / 2) - 4;
			SavePrefs();
		} else
			goto xit;
	}

	if(prefs.sb_lines == 0) prefs.sb_lines = 300;
	if(prefs.displayidstr[0] == 0) strcpy(prefs.displayidstr, "VT102");

	fh = Open(keysfile, MODE_OLDFILE);
	if(fh)
	{
		Read(fh, keys, 1520);
		Close(fh);
	}

	slist = AllocMem(sizeof(struct List), MEMF_CLEAR|MEMF_PUBLIC);
	if(!slist) goto xit;
	slist->lh_TailPred = (struct Node *)slist;
	slist->lh_Head = (struct Node *)&slist->lh_Tail;

	GfxBase = ReqToolsBase -> GfxBase;
	GadToolsBase = ReqToolsBase -> GadToolsBase;
	IntuitionBase = ReqToolsBase -> IntuitionBase;
	UtilityBase = ReqToolsBase -> UtilityBase;

	WorkbenchBase = OpenLibrary("workbench.library", 0);
	DiskfontBase = OpenLibrary("diskfont.library", 0);
	KeymapBase = OpenLibrary("keymap.library", 0);
	IconBase = OpenLibrary("icon.library", 0);
	SocketBase = OpenLibrary("bsdsocket.library", 0);

	reopenscreen = TRUE;
restart:
	if(OpenDisplay(reopenscreen))
	{
		if(!restartflag && server[0] != 0 && !connected) Connect_To_Server(server, cport);
		restartflag = FALSE;
		reopenscreen = FALSE;

		timer.tv_sec = 30;
		timer.tv_usec = 0;


/* ------ main loop ------ */

// BF: useless label, it is nevered used in function "main".
startloop:
	while(!done)
	{

		if(doicon)
		{
			CloseDisplay(TRUE);
			OpenIcon();
			doicon = FALSE;
		}

		if(unicon)
		{
			CloseIcon();
			OpenDisplay(TRUE);
			unicon = FALSE;
		}

		if(icon) // ICONIFIED
		{
			if(iconport) iconsig = 1L<<iconport->mp_SigBit;

			if(connected)
			{
				FD_ZERO(&rd);
				FD_SET(sok, &rd);
				sigmask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | iconsig;

				i = WaitSelect(sok + 1, &rd, 0, 0, &timer, &sigmask);

			} else {
				i = 0;
				sigmask = Wait( SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | iconsig );
			}

	 		if(sigmask&SIGBREAKF_CTRL_F) unicon = TRUE;

			if(sigmask&SIGBREAKF_CTRL_C) done = TRUE;

			if(sigmask&iconsig)
			{
				// Workbench sends AppMessage to the application's message port to notify it
				// https://wiki.amigaos.net/wiki/Workbench_Library#The_AppMessage_Structure
				register struct AppMessage *appmsg;
				while(appmsg = (struct AppMessage *)GetMsg(iconport))
				{
					if(appmsg->am_NumArgs==0 && appmsg->am_ArgList==0) unicon = TRUE;
					ReplyMsg(appmsg);
				}
			}

			if(i != 0) Receive();

		} else {

			winsig = 1L << win->UserPort->mp_SigBit;
			if(connected)
			{
				FD_ZERO(&rd);
				FD_SET(sok, &rd);

				sigmask = winsig;

				if(sbwin) sigmask |= 1L << sbwin->UserPort->mp_SigBit;
				if(pwin) sigmask |= 1L << pwin->UserPort->mp_SigBit;
				if(twin) sigmask |= 1L << twin->UserPort->mp_SigBit;

				i = WaitSelect(sok + 1, &rd, 0, 0, &timer, &sigmask);

				GetWindowMsg(win);

				if(sbwin) GetWindowMsg(sbwin);
				if(pwin) GetWindowMsg(pwin);
				if(twin) GetWindowMsg(twin);

				if(i != 0) Receive();

			} else {
				ULONG sig;

				if(sbwin)  sig = 1L << sbwin->UserPort->mp_SigBit; else sig = 0;
				if(pwin) sig |= 1L << pwin->UserPort->mp_SigBit;
				if(twin) sig |= 1L << twin->UserPort->mp_SigBit;

				sigmask = Wait( sig | winsig | SIGBREAKF_CTRL_C );

				if(sbwin)
				{
					if(sigmask&(1L << sbwin->UserPort->mp_SigBit)) GetWindowMsg(sbwin);
				}
				if(pwin)
				{
					if(sigmask&(1L << pwin->UserPort->mp_SigBit)) GetWindowMsg(pwin);
				}
				if(twin)
				{
					if(sigmask&(1L << twin->UserPort->mp_SigBit)) GetWindowMsg(twin);
				}

				if(sigmask&winsig) GetWindowMsg(win);
				if(sigmask&SIGBREAKF_CTRL_C) done = TRUE;
			}
			if(restartflag)
			{
				CloseDisplay(reopenscreen);
				goto restart;
			}

		}

	}

/* -- end of main loop -- */

		CloseDisplay(TRUE);
		SavePrefs();

		prefs.flags = 1;
		DisConnect(FALSE, TRUE);
	}

	CloseLibrary(WorkbenchBase);
	CloseLibrary(IconBase);
	CloseLibrary(DiskfontBase);
	CloseLibrary(KeymapBase);
	CloseLibrary(SocketBase);

	ClearScrollBack();
	FreeMem(slist, sizeof(struct List));
xit:
	CloseLibrary((struct Library *)ReqToolsBase);

	return RETURN_OK;
}

void LocalPrint(char *data)
{
	ConWrite(data, strlen(data));
}

void ConWrite(char *data, long len)
{
	if(!icon)
	{
		if(drivertype)
			XEmulatorWrite(&xemio, data, len);
		else {
			writeio.io_Data = data;
			writeio.io_Length = len;
			writeio.io_Command = CMD_WRITE;
			DoIO((struct IORequest *)&writeio);
		}
	}
}

void SaveScrollBack(char *fname)
{
	struct Scroll *worknode, *nextnode;
	fh = Lock(fname, SHARED_LOCK);
	if(fh)
	{
		UnLock(fh);
		if(!rtEZRequestA("File Already Exists.","OverWrite|Cancel", NULL, NULL, (struct TagItem *)&tags))
			return;
	}
	fh = Open(fname, MODE_NEWFILE);
	if(fh)
	{
		worknode = (struct Scroll *)slist->lh_Head;
		while(worknode)
		{
			nextnode = (struct Scroll *)worknode -> nnode.ln_Succ;
			if(!nextnode) break;

			Write(fh, (char *) (long)worknode + sizeof(struct Scroll), worknode->len-2);
			Write(fh, "\n", 1);
			worknode = nextnode;
		}
		Close(fh);
	}
}

void Connect(char thread)
{
	char tbuf[64];
	UWORD port = 0;

	if(!thread) strcpy(tbuf, server); else tbuf[0] = 0;

	if(rtGetStringA(tbuf, 63, "Enter host,port:", 0, (struct TagItem *)&tags))
	{
		if(tbuf[0] != 0)
		{
			register char *po;
			if(po = strchr(tbuf, ','))
			{
				po[0] = 0;
				port = atoi((char *)&po[1]);
			}

			if(!port) port = 23;

			if(thread)
			{
				mysprintf(buf, "run DCTelnet %s %ld <>NIL:", tbuf, port);
				Execute(buf, 0, 0);
			} else {
				cport = port;
				Connect_To_Server(tbuf, cport);
			}
		}
	}
}

void Information(void)
{
	if(connected)
	{
		register long spent = mytime() - contime;

		mysprintf(buf,	"     Host Name ... : %s\n"
				"    IP Address ... : %s\n"
				"      TCP Port ... : %ld\n\n"
				"   Online Time ... : %02ld:%02ld:%02ld\n"
				"    Bytes Sent ... : %ld\n"
				"Bytes Received ... : %ld",
			HostAddr->h_name,
			Inet_NtoA(INetSocketAddr.sin_addr.s_addr),
			cport,
			spent/3600, (spent/60)%60, spent%60,
			sent,
			bytes);

		rtEZRequestTags(buf, "OK", NULL, NULL,
				RT_Window,	win,
				RT_ReqPos,	REQPOS_CENTERSCR,
				TAG_DONE);
	} else
		SimpleReq("Not connected");
}

void CheckFlag(struct MenuItem *item, char bit)
{
	if(item->Flags & CHECKED)
		prefs.flags |= 1<<bit;
	else
		prefs.flags &= ~(1<<bit);
}

void OutKey(unsigned char key)
{
	if(prefs.flags&(1<<4)) // BS/DEL Swap
	{
		if(key == 8)
			key = 127;
		else {
			if(key == 127) key = 8;
		}
	}
	if(connected)
	{
		TCPSend((void *)&key, 1);
		if(key==(unsigned char)255) TCPSend((void *)&key, 1);
		if(!passflag)
		{
			TCPSend("\377\375\000\377\373\000", 6); // 8-bit data path
			passflag = TRUE;
		}
		if(prefs.flags&(1<<12)) goto cwrite;	// Local Echoback
	} else
cwrite:		ConWrite(&key, 1);
}


void GetWindowMsg(struct Window *wwin)
{
	struct MenuItem *item;
	struct IntuiMessage *message;
	struct rtFontRequester *fontreq;
	UWORD menuNumber, menuNum, itemNum;
	APTR reqinfo;
	struct Gadget *gad;
	ULONG class;
	UWORD code;
	UWORD qual;
	char fbuf[128];
	char close = FALSE;
	char resize = FALSE;
	char closetwin = FALSE;
	static char key_csi;
	static char key_macro;

	while (message = (struct IntuiMessage *)GetMsg(wwin->UserPort))
	{
		class = message->Class;
		code = message->Code;
		gad = (struct Gadget *)message->IAddress;
		qual = message->Qualifier;
		ReplyMsg(message);

		switch (class)
		{
		case IDCMP_GADGETUP:
			if(wwin == pwin)
			{
				RemoveGList(wwin, &strGad, 1);
				if(prefs.flags&(1<<11))
					strcat(strBuffer, "\r\n");
				else
					strcat(strBuffer, "\r");
				SendMacro(strBuffer);
				strBuffer[0] = 0;
				((struct StringInfo *)(strGad.SpecialInfo))->BufferPos = 0;
				((struct StringInfo *)(strGad.SpecialInfo))->DispPos = 0;
				AddGList(wwin, &strGad, ~0, 1, NULL);
				RefreshGList(&strGad, wwin, NULL, 1);
				ActivateGadget(&strGad, pwin, 0);
			}

			if(wwin == sbwin)
			{
				GetAttr(PGA_Top, Scroller, (ULONG *)&lasttop);
				RefreshListView(lasttop);
			}

			if(gad->GadgetID == 20) ScreenToBack(scr);

			if(wwin == twin)
			{
				switch(gad->GadgetID)
				{
					case 0:
						Connect(FALSE);
						break;
					case 1:
						DisConnect(FALSE, FALSE);
						break;
					case 2:
						WindowSub(AddressBook);
						break;
					case 3:
						WindowSub(Information);
						break;
					case 4:
					case 5:
						if(connected)
						{
							if(gad->GadgetID == 4)
								Upload(prefs.xferlibrary);
							else
								Download(prefs.xferlibrary);
						} else
							SimpleReq("You better connect first.");
						break;
					case 6:
						//if(rtEZRequestA("Quit?", "Quit|Cancel", NULL, NULL, (struct TagItem *)&tags))
							done = TRUE;
						break;
				}
			}
			break;


		case IDCMP_NEWSIZE:
			//LocalPrint("\017\233\164\233\165\233\166\233\167");
			if(wwin == sbwin) resize = TRUE;
			break;


		/*case IDCMP_REFRESHWINDOW:
			GT_RefreshWindow(wwin, NULL);
			break;*/


		case IDCMP_RAWKEY:
			if(wwin == sbwin)
			{
				switch(code)
				{
				case 76:
					goto up;
				case 77:
					goto down;
				case 84:
					buf[0] = 0;
					strcpy(fbuf, "DCTelnet.Cap");
					if(FileReq(buf, "#?", fbuf, "Save Scroll Back", TRUE, 0))
					{
						strcat(buf, fbuf);
						SaveScrollBack(buf);
					}
					break;
				case 82:
					if(rtEZRequestA("Print Scrollback?", "Print|Cancel", NULL, NULL, (struct TagItem *)&tags))
						SaveScrollBack("PRT:");
					break;
				case 80:
					ClearScrollBack();
					lines = 0;
					lasttop = 0;
					SetGadgetAttrs((struct Gadget *)Scroller, sbwin, NULL,
						PGA_Total,	0,
					TAG_DONE);
					RefreshListView(0);
					break;
				}
			}
			if(wwin == win  ||  wwin == twin)
			{
				struct InputEvent ie;
				register ULONG i, length;

				if(!(message->Code & IECODE_UP_PREFIX))
				{
					ie.ie_Class		= IECLASS_RAWKEY;
					ie.ie_SubClass		= 0;
					ie.ie_Code		= code;
					ie.ie_Qualifier		= qual;
					ie.ie_position.ie_addr	= gad;

					length = MapRawKey(&ie, conbuf, 16, NULL);

					for(i=0; i<length; i++)
					{
						switch(conbuf[i])
						{
						case 155:
							key_csi = TRUE;
							break;
						/*case 'v':
						case 'V':
							if(qual&IEQUALIFIER_RCOMMAND)
							{
								ConWrite("› v", 3);
								break;
							}*/
						default:
							if(key_csi)
							{
								key_csi = FALSE;
								if(conbuf[i] > 47 && conbuf[i] < 58)
								{
									key_macro = TRUE;
									SendMacro(&keys[(conbuf[i]-48)*152]);
								}

								switch(conbuf[i])
								{
								case 65:
									SendMisc("[A", 3);
									break;
								case 66:
									SendMisc("[B", 3);
									break;
								case 67:
									SendMisc("[C", 3);
									break;
								case 68:
									SendMisc("[D", 3);
									break;
								}

							} else {
								if(key_macro)
									key_macro = FALSE;
								else
								{
									OutKey(conbuf[i]);
									if(conbuf[i] == '\r' && (prefs.flags&(1<<11))) OutKey('\n');
								}
							}
						}
					}
				}
			}
			break;


		case IDCMP_MENUPICK:
			LEDs();
			menuNumber = code;
			while (menuNumber != MENUNULL)
			{
				item = ItemAddress(menuStrip, menuNumber);
				menuNum = MENUNUM(menuNumber);
				itemNum = ITEMNUM(menuNumber);
				/*subNum  = SUBNUM(menuNumber);*/
				switch(menuNum)
				{
				case 0:
					switch(itemNum)
					{
					case 0:
						rtEZRequestTags(
								"DCTelnet - A classic Amiga Telnet/BBS client with Zmodem"     "\n"
								"                          v"DCTELNET_VERSION                  "\n"
								"         Last Compiled .... : "__DATE__""                     "\n"
								"         First Compiled ... : May 17 1997"                    "\n"
								"         Compilers Used ... : "STR(COMPILER_STRING)           "\n"
								                                                               "\n"
								"            Original author : ZED^DC"                         "\n"
								                                                               "\n"
								"            Recompiled by   : Bruno FREDERIC"                 "\n"
								                                                               "\n"
								"                   More info/sources:"                        "\n"
								"           github.com/bruno-frederic/dctelnet"                "\n",
								"OK", NULL, NULL,
								RT_Window,	win,
								RT_ReqPos,	REQPOS_CENTERSCR,
								TAG_DONE);
						break;

					case 2:
						if(wwin != sbwin)
						{
							CloseScrollBack();
							OpenScrollBack(lasttop);
						}
						break;
					case 3:
						doicon = TRUE;
						break;

					case 4:
						SpeedTest();
						break;
					case 5:
						WindowSub(Finger);
						break;
					case 7:
						done = TRUE;
						break;
					}
					break;
				case 1:
					if(connected)
					{
						switch(itemNum)
						{
						case 0:
							Upload(prefs.xferlibrary);
							break;
						case 2:
							Download(prefs.xferlibrary);
							break;

						case 4:
							fbuf[0] = 0;
							strcpy(buf, prefs.uploadpath);
							if(FileReq(buf, "#?", fbuf, "ASCII Send", TRUE, 0))
							{
								register long r;
								strcat(buf, fbuf);
								fh = Open(buf, MODE_OLDFILE);
								if(fh)
								{
									while(r = Read(fh, buf, sizeof buf))
									{
										register long i;
										for(i=0; i<r; i++)
										{
											if(buf[i] == 10 && buf[i+1] != 13 && buf[i-1] != 13) buf[i] = 13;
										}
										TCPSend(buf, r);
									}
									Close(fh);
								}
							}
							break;

						}
					} else
						SimpleReq("You better connect first.");
					break;
				case 2:
					switch(itemNum)
					{
					case 0:
						Connect(FALSE);
						break;

					case 1:
						Connect(TRUE);
						break;

					case 2:
						DisConnect(FALSE, FALSE);
						break;

					case 4:
						WindowSub(AddressBook);
						break;

					case 6:
						WindowSub(Information);
						break;

					}
					break;
				case 3:
					switch(itemNum)
					{
					case 0:
						CheckFlag(item, 3);
						restartflag = TRUE;
						reopenscreen = TRUE;
						break;
					case 1:
						if(item->Flags & CHECKED)
						{
							prefs.flags |= 1<<2;

							if(!(prefs.flags&1))
							{
								SetAPen(&scr->RastPort, 1);
								RectFill(&scr->RastPort, scr->Width-86, 2, scr->Width-60, prefs.fontsize-1);
							}
						} else {
							prefs.flags &= ~(1<<2);
							LEDs();
						}
						break;
					case 2:
						CheckFlag(item, 0);
						restartflag = TRUE;
						reopenscreen = TRUE;
						break;
					case 3:
						CheckFlag(item, 1);
						break;
					case 4:
						CheckFlag(item, 4);
						break;
					case 5:
						CheckFlag(item, 5);
						break;
					case 6:
						CheckFlag(item, 6);
						if(item->Flags & CHECKED) LocalPrint("›m");
						break;
					case 7:
						CheckFlag(item, 7);
						break;
					case 8:
						if(wb)
							SimpleReq("Packet Window cannot work in Workbench mode.");
						else
							restartflag = TRUE;

						CheckFlag(item, 8);
						break;

					case 9:
						restartflag = TRUE;
						CheckFlag(item, 9);
						break;

					case 10:
						CheckFlag(item, 10);
						if(wb)
						{
							if(item->Flags & CHECKED)
								OpenToolWindow(TRUE);
							else
								CloseToolWindow();
						} else
							restartflag = TRUE;
						break;

					case 11:
						CheckFlag(item, 11);
						break;

					case 12:
						CheckFlag(item, 12);
						break;

					case 13:
						CheckFlag(item, 13);
						break;
					case 14:
						CheckFlag(item, 14);
						if(!wb && !(prefs.flags&(1<<9))) restartflag = TRUE;
						break;
					}
					break;

				case 4:
					switch(itemNum)
					{
					case 0:
						if(ChooseScreen(FALSE))
						{
							restartflag = TRUE;
							reopenscreen = TRUE;
						}
						break;

					case 1:
						if(fontreq = rtAllocRequestA (RT_FONTREQ, NULL))
						{
							rtChangeReqAttr(fontreq,
								RTFO_FontName,		prefs.fontname,
								RTFO_FontHeight,	prefs.fontsize,
								TAG_END);

							if(rtFontRequest(fontreq, "Screen Font..",
								RT_Window,	win,
								RTFO_Flags,	FREQF_FIXEDWIDTH,
								TAG_DONE))
							{
								strcpy(prefs.fontname, fontreq->Attr.ta_Name);
								prefs.fontsize = fontreq->Attr.ta_YSize;
								restartflag = TRUE;
								reopenscreen = TRUE;
							}
							rtFreeRequest(fontreq);
						}
						break;
					case 2:
						reqinfo = rtAllocRequestA(RT_REQINFO, NULL);
						if(reqinfo)
						{
							if(rtPaletteRequestA("Screen Palette..", reqinfo, (struct TagItem *)&tags) != -1)
							{
								UWORD i = 0;

								while(i < 16)
								{
									prefs.color[i] = GetRGB4(scr->ViewPort.ColorMap, i);
									i++;
								}
							}
							rtFreeRequest(reqinfo);
						}
						break;

					case 3:
						fbuf[0] = 0;
						strcpy(buf, prefs.downloadpath);
						if(FileReq(buf, "#?", fbuf, "Download Path..", TRUE, FREQF_NOFILES))
						{
							strcpy(prefs.downloadpath, buf);
						}
						break;

					case 4:
						FileReq("LIBS:", "xpr#?.library", prefs.xferlibrary, "Transfer Protocol..", FALSE, FREQF_PATGAD);
						break;

					case 5:
						rtGetStringA(prefs.xferinit, 51, "Protocol Init..", 0, (struct TagItem *)&tags);
						break;

					case 6:
						WindowSub(FunctionKeys);
						break;

					case 7:
						if(FileReq("LIBS:", "xem#?.library", prefs.displaydriver, "XEM Library..", FALSE, FREQF_PATGAD))
						{
							if(prefs.flags&(1<<9)) restartflag = TRUE;
						}
						break;

					case 8:
						rtGetStringA(prefs.displayidstr, 31, "Telnet Display ID..", 0, (struct TagItem *)&tags);
						break;

					case 9:
						rtGetLongA(&prefs.sb_lines, "ScrollBack Lines..", NULL, (struct TagItem *)&tags);
						break;

					case 10:
						prefs.win_top = win->TopEdge;
						prefs.win_left = win->LeftEdge;
						prefs.win_height = win->Height;
						prefs.win_width = win->Width;

						if(sbwin)
						{
							prefs.sb_left = sbwin->LeftEdge;
							prefs.sb_top = sbwin->TopEdge;
							prefs.sb_width = sbwin->Width;
							prefs.sb_height = sbwin->Height;
						}
						if(twin)
						{
							prefs.twin_left = twin->LeftEdge;
							prefs.twin_top = twin->TopEdge;
						}

						break;
					}
					break;
				case 5:
					switch(itemNum)
					{
					case 0:
						SendMisc(username, -1);
						SendMisc("\r", 1);
						break;
					case 1:
						SendMisc(password, -1);
						SendMisc("\r", 1);
						break;
					}
					break;
				}
				menuNumber = item->NextSelect;
			}
			break;


		case IDCMP_CLOSEWINDOW:
			if(wwin == win) done = TRUE;
			if(wwin == sbwin) close = TRUE;
			if(wwin == twin) closetwin = TRUE;
			break;


		case IDCMP_IDCMPUPDATE:
			switch((UWORD)GetTagData(GA_ID, 0, (struct TagItem *)gad))
			{
			case GAD_UP:
up:				if(lasttop > 0) lasttop--;
				break;

			case GAD_DOWN:
down:				if(lasttop+((sbwin->Height - (prefs.fontsize + scr->WBorTop + 2)) / prefs.fontsize) < lines) lasttop++;
				break;
			}
			SetGadgetAttrs((struct Gadget *)Scroller, sbwin, NULL,
				PGA_Top,	lasttop,
			TAG_DONE);

			RefreshListView(lasttop);

		}
	}
// BF: useless label, it is never used in function "GetWindowMsg"
xit:

	if(resize)
	{
		SetRast(sbwin->RPort, 0);
		RefreshWindowFrame(sbwin);
		RefreshListView(lasttop);
		SetGadgetAttrs((struct Gadget *)Scroller, sbwin, NULL,
			PGA_Visible,	(sbwin->Height - (prefs.fontsize + scr->WBorTop + 2)) / prefs.fontsize,
		TAG_END);
	}
	if(close) CloseScrollBack();
	if(closetwin)
	{
		CloseToolWindow();
		prefs.flags &= ~(1<<10);
	}
}

void CheckError(void)
{
	register long en = Errno();

	switch(en)
	{
		case EINTR:
			LocalPrint("ERROR: Interrupted system call.\r\n"); break;
		case EHOSTUNREACH:
			LocalPrint("ERROR: No route to host.\r\n"); break;
		case ECONNREFUSED:
			LocalPrint("ERROR: Connection refused.\r\n"); break;
		case ETIMEDOUT:
			LocalPrint("ERROR: Connection timeout.\r\n"); break;
		default:
			LocalFmt("Connection failed. Error %ld.\r\n", en);
	}
}

#include <dos/dostags.h>

struct Task *parent, *child;

extern void __saveds __asm Connect_To_Server_Child(void);

UWORD abort_flag;
UWORD connect_msg_type;
char *connect_string;

UWORD Connect_To_ServerA(char *servername, UWORD port);

UWORD Connect_To_Server(char *servername, UWORD port)
{
	UWORD ret;

	parent = FindTask(0);

	child = (struct Task *)CreateNewProcTags(NP_Entry, Connect_To_Server_Child, TAG_DONE);

	ret = Connect_To_ServerA(servername, port);
	Signal(child, SIGBREAKF_CTRL_C);

	Wait(SIGBREAKF_CTRL_E);

	while((SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C))
	{
	}

	return(ret);
}

void c_msg(char *msg, UWORD type)
{
	connect_string = msg;
	connect_msg_type = type;
	Signal(child, SIGBREAKF_CTRL_E);
	Wait(SIGBREAKF_CTRL_E);
}

UWORD Connect_To_ServerA(char *servername, UWORD port)
{
	if(!SocketBase) SocketBase = OpenLibrary("bsdsocket.library", 0);

	if(!SocketBase)
	{
		SimpleReq("You must start AmiTCP/Miami first.");
		return(255);
	}

	if((prefs.flags&5) == 0) // if bits 0 and 2 are clear
	{
		SetAPen(&scr->RastPort, 11);
		RectFill(&scr->RastPort, scr->Width-84, 3, scr->Width-76, prefs.fontsize-2);
	}

	DisConnect(FALSE, FALSE);

	c_msg("Looking up...", 4);
	c_msg(servername, 0);

	LocalFmt("\r\nLooking up ›32m%s›m...\r\n", servername);

	strcpy(server, servername);

	abort_flag = 0;
	HostAddr = gethostbyname(server);
	if(!HostAddr)
	{
		if(abort_flag)
			LocalPrint("Host lookup aborted.\r\n");
		else
			LocalPrint("Unknown host. Maybe you misspelt it?\r\n");
		LEDs();
		return(1);
	}

	INetSocketAddr.sin_len = sizeof(INetSocketAddr);
	INetSocketAddr.sin_port = port;
	INetSocketAddr.sin_family = AF_INET;
	INetSocketAddr.sin_addr.s_addr = 0;

	memcpy(&INetSocketAddr.sin_addr, HostAddr->h_addr, HostAddr->h_length);
	//CopyMem(HostAddr->h_addr, &INetSocketAddr.sin_addr, HostAddr->h_length);

	c_msg(Inet_NtoA(INetSocketAddr.sin_addr.s_addr), 1);
	c_msg(HostAddr->h_name, 2);

	LocalFmt("Connecting to ›32m%s›m (›36m%s›m) port ›35m%ld›m...\r\n",
		HostAddr->h_name,
		Inet_NtoA(INetSocketAddr.sin_addr.s_addr),
		port);

	sok = socket(HostAddr->h_addrtype, SOCK_STREAM, 0);
	if(sok == -1)
	{
		LocalPrint("Cannot Open Socket.\r\n");
		LEDs();
		return(2);
	}

	c_msg("Connecting...", 4);

	// connect() expects a generic sockaddr, so cast the INet socket address
	if(connect(sok, (struct sockaddr *)&INetSocketAddr, sizeof(INetSocketAddr)) == -1)
	{
		CheckError();
		shutdown(sok, 2);
		CloseSocket(sok);
		LEDs();
		return(3);
	}

	LocalPrint("Connected.\r\n");

	if(!(prefs.flags&(1<<13)))
		TCPSend("\377\375\003", 3);
	else {
		passall = TRUE;
		passflag = TRUE;
	}

	if(wb) WindowToFront(win); else ScreenToFront(scr);

	contime = mytime();

	connected = TRUE;

	LEDs();

	return(0);
}

char OpenDisplay(char doscreen)
{
	long i;

	if(!doscreen) goto jump1;

	if(prefs.flags&(1<<3)) wb = TRUE; else wb = FALSE;

	fontattr.ta_Name = prefs.fontname;
	fontattr.ta_YSize = prefs.fontsize;
	ansifont = OpenDiskFont(&fontattr);
	if(!ansifont)
	{
		fontattr.ta_Name = "topaz.font";
		fontattr.ta_YSize = 8;
		ansifont = OpenFont(&fontattr);
	}

	if(wb)
	{
		prefs.flags |= (1<<2);		   // Disable Leds
		prefs.flags &= ~(1<<8);		   // Not Packet Window
		scr = LockPubScreen(0L);
	} else {
		register UWORD *pens;
		static struct NewScreen nscr;

		if(prefs.DisplayDepth < 3) pens = &colorpens[12]; else pens = colorpens;

		memcpy(&nscr.Width, &prefs.DisplayWidth, 6);
		nscr.BlockPen = 1;
		nscr.Type = CUSTOMSCREEN;
		nscr.Font = &fontattr;
		// Main window title in full screen mode:
		nscr.DefaultTitle = MainWindowTitle;

		scr = OpenScreenTags(&nscr,
			SA_DisplayID,	prefs.DisplayID,
       	       	        SA_Pens,	(ULONG)pens,
			SA_ShowTitle,	!prefs.flags&1,
			SA_AutoScroll,	TRUE,
			SA_Interleaved,	TRUE,
			TAG_END);
/*
		scr = OpenScreenTags(NULL,
			SA_Title,	"DCTelnet 1.5 © "__DATE__" By ZED^DC",
			SA_Width,	prefs.DisplayWidth,
			SA_Height,	prefs.DisplayHeight,
			SA_DisplayID,	prefs.DisplayID,
       	       	        SA_Depth,	prefs.DisplayDepth,
			SA_ShowTitle,	!prefs.flags&1,
			SA_Type,	CUSTOMSCREEN,
       	       	        SA_Pens,	(ULONG)pens,
			SA_Font,	&fontattr,
			SA_AutoScroll,	TRUE,
			SA_Interleaved,	TRUE,
       	       	        TAG_END);*/
	}

	if(scr)
	{
		WinTop = (scr->WBorTop)+(scr->Font->ta_YSize)+1;
		vi = GetVisualInfoA(scr, 0);
		DrawInfo = GetScreenDrawInfo(scr);
jump1:
		nwin.Screen = scr;
		nwin.Type = PUBLICSCREEN;
		nwin.DetailPen = 255;
		nwin.BlockPen = 255;

		if(wb)
		{
			static UWORD sizes[4] = { 200, 50, 1600, 1200 };

			mynewmenu[24].nm_Flags |= CHECKED;	   // WB
			//mynewmenu[38].nm_Flags = NM_ITEMDISABLED;  // Jump Scroll
			mynewmenu[40].nm_Flags = NM_ITEMDISABLED;  // ScreenMode
			mynewmenu[42].nm_Flags = NM_ITEMDISABLED;  // ScreenPalette

			memcpy(&nwin, &prefs.win_left, 8);
			memcpy(&nwin.MinWidth, &sizes, 8);
			nwin.IDCMPFlags = IDCMP_RAWKEY | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK;
			//nwin.Flags = WFLG_GIMMEZEROZERO|WFLG_NEWLOOKMENUS|WFLG_SIMPLE_REFRESH|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
			nwin.Flags = WFLG_GIMMEZEROZERO|WFLG_NEWLOOKMENUS|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
			// Main window title in windowed workbench mode:
			nwin.Title = MainWindowTitle;
			nwin.FirstGadget = 0;

			CheckDimensions(&nwin);

			win = OpenWindow(&nwin);
			UnlockPubScreen(0L, scr);
			if(prefs.flags&(1<<10)) OpenToolWindow(FALSE);
		} else {
			struct Gadget *backgad;
			UWORD top, height;

			mynewmenu[24].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;	// WB
			//mynewmenu[38].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;	// Jump Scroll
			mynewmenu[40].nm_Flags = 0;	// ScreenMode
			mynewmenu[42].nm_Flags = 0;	// ScreenPalette

			LoadRGB4(&scr->ViewPort, (UWORD *)&prefs.color, 16);

			if(prefs.flags&(1<<10)) OpenToolWindow(FALSE);

			if(prefs.flags&1) // HIDE TITLE
			{
				top = 0;
				height = scr->Height;
				backgad = &window_back;
				window_back.Width = 20;
				window_back.Height = 9;
				window_back.Activation = RELVERIFY;
				window_back.GadgetType = BOOLGADGET;
				window_back.LeftEdge = scr->Width - 20;
				window_back.GadgetID = 20;
			} else {
				top = prefs.fontsize + 3;
				height = scr->Height - (prefs.fontsize + 3);
				backgad = 0;
			}

			if(twin)	// Tool Window
			{
				top = twin->TopEdge + twin->Height + 1;
				height = scr->Height - top;
				if(backgad)
				{
					AddGadget(twin, backgad, -1);
					backgad = 0;
				}
			}

			nwin.LeftEdge = 0;
			nwin.Title = 0;
			nwin.Width = scr->Width;

			if(prefs.flags&(1<<8))	// Packet
			{
				height -= (prefs.fontsize + 2);
				strInfo.Buffer = strBuffer;
				strInfo.MaxChars = BUFSIZE;
				strGad.TopEdge = 2;
				strGad.Activation = GACT_RELVERIFY | GACT_STRINGLEFT;
				strGad.GadgetType = GTYP_STRGADGET;
				strGad.SpecialInfo = &strInfo;
				strGad.Width = scr->Width;
				strGad.Height = prefs.fontsize;

				nwin.TopEdge = top+height;
				nwin.Height = prefs.fontsize+2,
				nwin.FirstGadget = &strGad;
				nwin.IDCMPFlags =	IDCMP_MENUPICK |
							IDCMP_GADGETUP;
				nwin.Flags =	WFLG_NEWLOOKMENUS |
						WFLG_BORDERLESS |
						WFLG_BACKDROP;

				pwin = OpenWindow(&nwin);

				SetAPen(pwin->RPort, 1);
				Draw(pwin->RPort, pwin->Width, 0);
			}

			nwin.TopEdge = top;
			nwin.Height = height;
			nwin.FirstGadget = backgad;
			nwin.IDCMPFlags =	IDCMP_GADGETUP |
						IDCMP_RAWKEY |
						IDCMP_CLOSEWINDOW |
						IDCMP_MENUPICK;
			nwin.Flags =	WFLG_SMART_REFRESH |
					WFLG_NEWLOOKMENUS |
					WFLG_BORDERLESS |
					WFLG_ACTIVATE |
					WFLG_BACKDROP;

			win = OpenWindow(&nwin);
		}

		if(win)
		{
			if((prefs.flags&(1<<9)) && prefs.displaydriver[0])
			{
				drivertype = 1;
				mynewmenu[38].nm_Flags = NM_ITEMDISABLED;
			} else {
				drivertype = 0;
				mynewmenu[38].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
				prefs.flags &= ~(1<<9);
			}

			if(prefs.flags&(1<<0))
				mynewmenu[26].nm_Flags |= CHECKED;
			else
				mynewmenu[26].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;

			if(prefs.flags&(1<<1)) // CRLF
				mynewmenu[27].nm_Flags |= CHECKED;
			else
				mynewmenu[27].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;

			if(prefs.flags&(1<<2))
				mynewmenu[25].nm_Flags |= CHECKED;
			else
				mynewmenu[25].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;

			for(i=4; i<15; i++)
			{
				if(prefs.flags&(1<<i))
					mynewmenu[i+24].nm_Flags |= CHECKED;
				else
					mynewmenu[i+24].nm_Flags &= ~CHECKED;
					//mynewmenu[i+24].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
			}

			tags[0] = RT_Window;
			tags[1] = (ULONG)win;
			tags[2] = RT_WaitPointer;
			tags[3] = TRUE;

			SetFont(win->RPort, ansifont);

			if(menuStrip = CreateMenusA(mynewmenu, 0))
			{
				static ULONG ltags[] = { GTMN_NewLookMenus, TRUE, TAG_END };

				register struct MenuItem *item = menuStrip->FirstItem->NextItem->NextItem->NextItem->NextItem->NextItem->NextItem->NextItem;
				if(prefs.DisplayDepth > 1) ((struct IntuiText *)item->ItemFill)->FrontPen = 15;
				//item->Flags |= HIGHBOX;
				//item->Flags &= ~HIGHCOMP;
				item->Flags = 150;

				ltags[1] = wb;
 				LayoutMenusA(menuStrip, vi, (struct TagItem *)&ltags);

				SetMenuStrip(win, menuStrip);

				if(pwin) ResetMenuStrip(pwin, menuStrip);
				if(twin) ResetMenuStrip(twin, menuStrip);

				if(drivertype) goto lib;
cantfind:
				WriteConPort = CreateMsgPort();
				if(WriteConPort)
				{
					char *dev;
					UWORD unit;

					writeio.io_Message.mn_ReplyPort = WriteConPort;
					writeio.io_Data = win;
					writeio.io_Length = sizeof(struct Window);

					if(wb)
					{
						dev = "console.device";
						unit = 3;
					} else {
						dev = "ibmcon.device";
						if(prefs.flags&(1<<14))
							unit = 2;
						else
							unit = 1;
					}

					i = OpenDevice(dev, unit, (struct IORequest *)&writeio, 0);

					if(i == 0)
					{
lib:
						if(drivertype)
						{
							XEmulatorBase = OpenLibrary(prefs.displaydriver, 0);
							if(!XEmulatorBase)
							{
								drivertype = 0;
								SimpleReq("Failed to open XEM library.");
								goto cantfind;
							}
							xemio.xem_window	= win;
							xemio.xem_font 		= ansifont;
							//xemio.xem_signal	= 0;
							xemio.xem_screendepth	= scr->BitMap.Depth;
							xemio.xem_swrite	= (long (* )(UBYTE * , LONG ))xpr_swrite;
							xemio.xem_sread		= (long (* )(UBYTE * , LONG , LONG ))xpr_sread;
							// BF : xpr_gets() does nothing. just a return 0. should we put a NULL instead ?
							// seems to be the usual value for unimplemented callback functions
							xemio.xem_sbreak	= (long (* )(void))xpr_gets;
							xemio.xem_sstart	= (void (* )(void))xpr_gets;
							xemio.xem_sstop		= (long (* )(void))xpr_gets;
							xemio.xem_sflush	= (long (* )(void))xpr_sflush;
							xemio.xem_toptions	= (unsigned long (* )(LONG , struct xem_option ** ))xpr_gets;
							xemio.xem_tgets		= (long (* )(UBYTE * , UBYTE * , ULONG ))xpr_gets;
							xemio.xem_tbeep		= (void (* )(ULONG , ULONG ))xpr_gets;
							//xemio.xem_console	= 0;
							//xemio.xem_process_macrokeys = 0;

							XEmulatorSetup(&xemio);
							XEmulatorOpenConsole(&xemio);
							XEmulatorMacroKeyFilter(&xemio, NULL);
						}

						icon = FALSE;

						LEDs();

						if(!connected)
						{
							register UWORD flags;
							register char cpu;

							flags = SysBase->AttnFlags;
							cpu = '0';
							if(flags & AFF_68010) cpu = '1';
							if(flags & AFF_68020) cpu = '2';
							if(flags & AFF_68030) cpu = '3';
							if(flags & AFF_68040) cpu = '4';
							if(flags & AFF_68060) cpu = '6';

							LocalFmt(
	"›0;1;36m\014\r\n\r\n"
	"Processor: ›37m680%lc0\r\n\r\n›36m"
	"Kickstart: ›37m%ld.%ld\r\n\r\n›36m"
	"TCP Stack: ›37m", cpu, ((struct Library *)SysBase)->lib_Version, SysBase->SoftVer);

							if(SocketBase)
							{
								register char *po;
								strcpy(buf, SocketBase->lib_IdString);
								po = strchr(buf, '\n');
								if(po) po[0] = 0;
								LocalPrint(buf);
								LocalPrint("›m\r\n\r\n");
							} else
								LocalPrint("›31mNot active›m\r\n\r\n");
						}

						return(TRUE);
					} else
						SimpleReq("I need ibmcon.device");
				}
			}
		}
	} else
		rtEZRequestA("Screen failed to open.", "OK", NULL, NULL, 0);

	CloseDisplay(TRUE);

	return(FALSE);
}

void CloseDisplay(char doscreen)
{
	if(drivertype)
	{
		if(win)
		{
			XEmulatorCloseConsole(&xemio);
			XEmulatorCleanup(&xemio);
			CloseLibrary(XEmulatorBase);
		}
	} else {
		if(writeio.io_Error == 0) CloseDevice((struct IORequest *)&writeio);
		if(WriteConPort) DeleteMsgPort(WriteConPort);
	}

	if(pwin)
	{
		ClearMenuStrip(pwin);
		CloseWindow(pwin);
		pwin = 0;
	}

	if(win)
	{
		ClearMenuStrip(win);
		CloseWindow(win);
	}

	CloseScrollBack();
	CloseToolWindow();

	if(menuStrip)	FreeMenus(menuStrip);

	if(doscreen)
	{
		if(vi)		FreeVisualInfo(vi);
		if(DrawInfo)	FreeScreenDrawInfo(scr, DrawInfo);
		if(!wb && scr)	CloseScreen(scr);
		if(ansifont)	CloseFont(ansifont);
	}

	icon = TRUE;
}

void OpenIcon(void)
{
	if(iconport = CreateMsgPort())
	{
		prefsfile[16] = 0;
		dobj = GetDiskObjectNew(prefsfile);
		prefsfile[16] = '.';
		if(dobj)
		{
			if(dcicon = AddAppIconA(0, 0, "DCTelnet", iconport, 0, dobj, 0)) return;

			FreeDiskObject(dobj);
		}
		DeleteMsgPort(iconport);
	}
	iconport = 0;
}

void CloseIcon(void)
{
	if(iconport)
	{
		RemoveAppIcon(dcicon);
		FreeDiskObject(dobj);
		DeleteMsgPort(iconport);
	}
}
