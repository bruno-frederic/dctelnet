/* ====================================================================== */
/* ============================= DC TELNET ============================== */
/* ====================================================================== */


#define DCTELNET_VERSION "1.8-dev"
const char __ver[] = "$VER: DCTelnet " DCTELNET_VERSION " " __AMIGADATE__;

static char MainWindowTitle[] =
  "DCTelnet " DCTELNET_VERSION " " __AMIGADATE__ " - A classic Amiga Telnet/BBS client";

#define __USE_SYSBASE

#include <proto/exec.h>               // OpenLibrary(), GetMsg(), ReplyMsg(), AllocMem()...
#include <proto/dos.h>                // Open(), Close(), Read(), Write(), PutStr()...
#include <proto/intuition.h>          // OpenWindow(),CloseWindow(), OnMenu(), OffMenu()...
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
#include <proto/gadtools.h>           // GT_GetIMsg(), GT_ReplyIMsg()...
#include <proto/diskfont.h>           // OpenDiskFont()
#include <proto/utility.h>            // GetTagData()
#include <proto/icon.h>               // GetDiskObjectNew(), FreeDiskObject()
#include <proto/wb.h>                 // AddAppIconA(), RemoveAppIcon()
#include <proto/keymap.h>             // MapRawKey()
#include <devices/conunit.h>          // CONU_SNIPMAP, CONU_CHARMAP, CONFLAG_DEFAULT
#include <libraries/reqtools.h>       // struct rtFileList, RT_FILEREQ, RT_Window
#include <proto/reqtools.h>           // rtAllocRequestA() rtScreenModeRequest() rtPaletteRequestA()
#include <proto/socket.h>             // send(), <CloseSocket>()
#include <libraries/xem.h>            // struct XEM_IO
#include <proto/xem.h>                // XEmulatorSetup(), XEmulatorWrite()...
#include "DCTelnet.h"
#include "guis.h"
#include "connect.h"
#include "Xfer.h"


// Stringify macro :
#define STR_(x) #x
#define STR(x) STR_(x)

static struct NewMenu mynewmenu[] =
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
        {  NM_ITEM, "Tool Bar",         "4", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, 0,},
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


static void GetWindowMsg(struct Window *wwin);

extern struct ExecBase *SysBase;
struct ReqToolsBase *ReqToolsBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *KeymapBase, *XEmulatorBase, *GadToolsBase, *SocketBase;
struct Library *DiskfontBase, *IconBase, *WorkbenchBase, *UtilityBase;

struct Window *win, *scrollbackWin, *toolBarWin;
static struct Window *pwin;
struct List *scrollbackList;
struct Screen *scr;
struct DrawInfo *drawInfo;
static struct Gadget screenToBackGadget; // In top right corner when title bar is hidden in full screen
struct NewGadget newGadget;
static struct TextAttr fontAttr;
struct TextFont *ansiFont;
static struct IOStdReq writeIOReq;
static struct MsgPort *WriteConPort;
struct Menu *menuStrip;
static struct DiskObject *diskObj;
struct MsgPort *iconPort;
static struct AppIcon *appIconOnWB;
static struct hostent *hostAddr;
static struct sockaddr_in inetSocketAddr;
static struct XEM_IO xemIO;
struct NewWindow newWin;

#define BUFSIZE 250
static UBYTE strBuffer[BUFSIZE+2];
static struct StringInfo strInfo;
static struct Gadget strGad;

enum	{	GAD_SCROLLER,
		GAD_UP,
		GAD_DOWN
	};

struct PrefsStruct prefs;

ULONG reqtoolsTags[5];
static BPTR fileHandle;
long nScrollbackLines;
static long indexInScrollBuffer;
long tcpSocket, nBytesReceived;
static long conectionTime, nBytesSent;
void *visualInfos;
char username[42], password[42];
unsigned char buf[2048], keys[1520];
static unsigned char conbuf[16], scrollbuf[402];
char server[64];
static ULONG lasttop;		// last topline of scrollback
UWORD tcpPort = 23;	// current tcp port
UWORD winTop;		// WinTop topEdge (titlebar height)
BOOL shouldQuitApp;	// program finished
static BOOL isConnected;	// tcp connected
static UBYTE passAll;		// passall telnet negotiation
static UBYTE passFlag;		// already sent 8bit info
static BOOL shouldRestart;	// prefs changed, restart
static BOOL shouldReopenScreen;	// flag
BOOL isRunningOnWB; // running in wb
BOOL isAppIconified;	// iconified
static BOOL shouldIconify;		// must iconify
BOOL shouldUniconify;		// must uniconify
static UBYTE drivertype;	// drivertype 0 - normal    1 - xem library
#define DRIVER_NORMAL  0
#define DRIVER_XEM_LIB 1
static BOOL isFingerRequest;		// isFingerRequest?

static UWORD colorPens[]  = { 1,4,1,1,6,4,1,0,5,4,1,6,65535 };
static UWORD color[] = { 0x0000, 0x0DDD, 0x00D0, 0x0DD0, 0x000D, 0x0D0D, 0x00DD, 0x0D00,
		  0x0555, 0x0FFF, 0x00F0, 0x0FF0, 0x000F, 0x0F0F, 0x00FF, 0x0F00, 65535 };
/*                 black,    white,  green, yellow, blue, purple, aqua,   red */

char const prefsFilename[] = "PROGDIR:DCTelnet.Prefs";
char const bookFilename[]  = "PROGDIR:DCTelnet.Book";
char const keysFilename[]  = "PROGDIR:DCTelnet.Keys";
static char *programName;            // Name of the program as provided by argv[0] or task::tc_Node.ln_Name


void mysprintf(char *Buffer, char *ctl, ...)
{
	RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75", Buffer);
}

static void ConWrite(char *data, long len)
{
	if(!isAppIconified)
	{
		if(drivertype)
			XEmulatorWrite(&xemIO, data, len);
		else {
			// Doc about passing requests to I/O device:
			// https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0006.html

			// An I/O request typically has three fields set for every command sent to a device:
			writeIOReq.io_Data = data;
			writeIOReq.io_Length = len;
			writeIOReq.io_Command = CMD_WRITE;
			DoIO((struct IORequest *)&writeIOReq); // DoIO() is a synchronous function
		}
	}
}

void LocalPrint(char *data)
{
	ConWrite(data, strlen(data));
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
	if(send(tcpSocket, buf, len, 0) < 0) return -1;
	nBytesSent += len;
	return len;
}

static void WindowSub(void (*Sub)(void))
{
	if(scrollbackWin) rtSetWaitPointer(scrollbackWin);
	if (toolBarWin) rtSetWaitPointer(toolBarWin);
	rtSetWaitPointer(win);
	Sub();
	if(scrollbackWin) ClearPointer(scrollbackWin);
	if (toolBarWin) ClearPointer(toolBarWin);
	ClearPointer(win);
	LEDs();
}

/*
	Lightweight requester; does not use reqtools.library. Goal: remove the dependency on the
	unmaintained ReqTools library by using built-in requesters (EasyRequest / ASL) so the program
	runs on systems without ReqTools.
	Compatibility: works on KS 2.00 without ReqTools
*/
static void EZReq(struct Window *win, const char *str)
{
	struct EasyStruct es;
	es.es_StructSize   = sizeof(struct EasyStruct);
	es.es_Flags        = 0;
	es.es_Title        = "Information";
	es.es_TextFormat   = str;
	es.es_GadgetFormat = "OK";

	EasyRequestArgs(win,	// This can be NULL; requester will appear on the Workbench screen
					&es,
					NULL,
					NULL);
}

void SimpleReq(char *str)
{
	EZReq(win, str);	//rtEZRequestA(str, "OK", NULL, NULL, (struct TagItem *)&tags);
	LEDs();
}

static void DisConnect(char remote, char quiet)
{
	if(isConnected)
	{
		if(!quiet && !isAppIconified)
		{
			register long spent;
			if(remote)
				LocalPrint("›m\r\nConnection closed by foreign host");
			else
				LocalPrint("›m\r\nConnection closed");
			spent = mytime() - conectionTime;
			LocalFmt(". %02ld:%02ld:%02ld spent online.\r\n", spent/3600, (spent/60)%60, spent%60);
		}
		shutdown(tcpSocket, 2);
		CloseSocket(tcpSocket);

		isConnected = FALSE;
		passAll = FALSE;
		passFlag = FALSE;
		nBytesReceived = 0;
		nBytesSent = 0;

		if(isFingerRequest)
		{
			isFingerRequest = FALSE;
			OnMenu(win, FULLMENUNUM(3, -1, 0));
		}

		LEDs();
	}
}

void SavePrefs(void)
{
	fileHandle = Open(prefsFilename, MODE_NEWFILE);
	if(fileHandle)
	{
		Write(fileHandle, &prefs, sizeof(struct PrefsStruct));
		Close(fileHandle);
	}
}

static char ChooseScreen(char firsttime)
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

static char FileReq(char *dir, char *pat, char *file, char *title, char dodir, ULONG flags)
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

static void AddBuf(unsigned char *str, long size)
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
				scrollbuf[indexInScrollBuffer] = 0;
				indexInScrollBuffer += 2;
				node = AllocMem(sizeof(struct Scroll) + indexInScrollBuffer, MEMF_PUBLIC|MEMF_CLEAR);
				if(node)
				{
					node->nnode.ln_Name = (char *) (long)node + sizeof(struct Scroll);
					node->len = indexInScrollBuffer;
					CopyMem(scrollbuf, node->nnode.ln_Name, indexInScrollBuffer - 2);
					AddTail(scrollbackList, (struct Node *) node);
					nScrollbackLines++;
				}
				if(nScrollbackLines > prefs.sb_lines)
				{
					nScrollbackLines--;
					node = (struct Scroll *)scrollbackList->lh_Head;
					nextnode = (struct Scroll *)node -> nnode.ln_Succ;
					if(nextnode)
					{
						Remove((struct Node *) node);
						FreeMem(node, sizeof(struct Scroll)+node->len);
					}
				} else {
					if(scrollbackWin)
					{
						SetGadgetAttrs((struct Gadget *)Scroller, scrollbackWin, NULL,
							PGA_Total,	nScrollbackLines,
						TAG_DONE);
					}
				}
				indexInScrollBuffer = 0;
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
							if(indexInScrollBuffer > 400) goto add;
							scrollbuf[indexInScrollBuffer] = ' ';
							indexInScrollBuffer++;
						}
						break;
					case 'H':
					case 'B':
						goto add;
				}
				break;
			default:
				if(indexInScrollBuffer > 400) goto add;
				scrollbuf[indexInScrollBuffer] = str[i];
				indexInScrollBuffer++;
				break;
		}
		i++;
	}
}

static void Receive(void)
{
	register long length;

	if(passAll)
	{
		if((length = recv(tcpSocket, buf, sizeof buf, 0)) < 1)
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

			if(((prefs.flags&5) == 0)  &&  !isAppIconified) // if bits 0 and 2 are clear
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
					if(!(prefs.flags & FLAG_CRLF_CORRECTION)) goto norm;
					outbuf[j] = 13;
					j++;
					outbuf[j] = 10;
					j++;
					break;
				case 13:
					if(!(prefs.flags & FLAG_CRLF_CORRECTION)) goto norm;
					break;
				case 27:
					if(!(prefs.flags & FLAG_STRIP_COLOUR)) goto norm;
					temp = i;
					i += 2;
					while(i < length && buf[i]>='0' && buf[i]<=';') i++;
					if(buf[i] != 'm')
					{
						i = temp;
						goto norm;
					}
					break;
				case 255:     // The byte 0xff (255) means that the next byte is a Telnet command
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

			if(!(prefs.flags & FLAG_DISABLE_SCROLLBACK)) AddBuf(outbuf, j);

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

            // Draw connection activity indicator when Title bar AND LEDs are enabled
            // AND NOT iconified
			if(((prefs.flags & (FLAG_HIDE_TITLEBAR | FLAG_HIDE_LEDS)) == 0)  &&  !isAppIconified)
                EraseRect(&scr->RastPort, scr->Width-72, 2, scr->Width-60, prefs.fontsize-1);

			FreeMem(outbuf, length+256);

			if(download) Download(prefs.xferlibrary);
			if(upload) Upload(prefs.xferlibrary);

			nBytesReceived += length;
		}
	} else {

		if((recv(tcpSocket, buf, 1, 0)) < 1)
		{
			DisConnect(TRUE, FALSE);
			return;
		}

		// The byte 0xff (255) means that the next byte is a Telnet command. If you want to send
		// 0xff then you must send it twice to tell telnet that you don't intend to send a command.
		if(buf[0] == 255  &&  !(prefs.flags & FLAG_RAW_CONNECTION)) // NOT a Raw Connection
		{
			if((recv(tcpSocket, &buf[1], 2, 0)) < 1)
			{
				DisConnect(TRUE, FALSE);
				return;
			}
			nBytesReceived += 2;

			if(prefs.flags & FLAG_SIMPLE_TELNET) // Very simple telnet negotiation
			{
				if(buf[1] == 253)
				{
					if(!buf[2])
						passAll = TRUE;
					else {
						buf[1] = 252;
						TCPSend(buf, 3);
					}
				}
			} else {
				switch(buf[1])
				{
					case 250: //SubNeg
						recv(tcpSocket, &buf[8], 3, 0);
						nBytesReceived += 3;
						if(buf[2] == 24)
						{
							//recv(tcpSocket, buf, 3, 0);
							TCPSend("\377\372\030\000", 4);
							TCPSend(prefs.displayidstr, strlen(prefs.displayidstr));
							TCPSend("\377\360", 2);
						}
						break;
					case 253: //DO
						if(!buf[2])
							passAll = TRUE;
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
			nBytesReceived++;
			if(buf[0]) ConWrite(buf, 1);
			if(nBytesReceived > 128) passAll = TRUE;
		}
	}
}

static void SendMisc(char *str, long len)
{
	if(len == -1) len = strlen(str);

	if(isConnected)
		TCPSend(str, len);
	else
		ConWrite(str, len);
}

static void SendMacro(char *str)
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
	// Draw connection activity indicator when Title bar AND LEDs are enabled AND NOT iconified
	if((prefs.flags & (FLAG_HIDE_TITLEBAR | FLAG_HIDE_LEDS)) == 0  &&  !isAppIconified)
	{
		EraseRect(&scr->RastPort, scr->Width-72, 2, scr->Width-60, prefs.fontsize-1);
		EraseRect(&scr->RastPort, scr->Width-86, 2, scr->Width-74, prefs.fontsize-1);
		if(isConnected)
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

static void SpeedTest(void)
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

static void ClearScrollBack(void)
{
	struct Scroll *worknode, *nextnode;

	worknode = (struct Scroll *)scrollbackList->lh_Head;
	while(worknode)
	{
		nextnode = (struct Scroll *)worknode -> nnode.ln_Succ;
		if(!nextnode) break;

		FreeMem(worknode, sizeof(struct Scroll)+worknode->len);
		worknode = nextnode;
	}
	scrollbackList->lh_Tail = 0;
	scrollbackList->lh_TailPred = (struct Node *)scrollbackList;
	scrollbackList->lh_Head = (struct Node *)&scrollbackList->lh_Tail;
}

static void Finger(void)
{
	long oldflags;
	char tbuf[64] = "reiver@plan.cat";
	char *host;

	if(rtGetStringA(tbuf, 63, "Enter EMail Address:", 0, (struct TagItem *)&reqtoolsTags))
	{
		host = strchr(tbuf, '@');
		if(host)
		{
			host[0] = 0;
			*host++;
			oldflags = prefs.flags;
			prefs.flags = FLAG_RAW_CONNECTION;	// Raw Connection (NO telnet negotiation data)
			if(Connect_To_Server(host, 79) == 0)
			{
				mysprintf(buf, "/W %s\r\n", tbuf);
				send(tcpSocket, buf, strlen(buf), 0);
				OffMenu(win, FULLMENUNUM(3, -1, 0));
				isFingerRequest = TRUE;
			}
			prefs.flags = oldflags;
		}
	}
}

int main(int argc, char *argv[])
{
	ULONG iconsig = 0, sigmask, winsig;
	LONG i;
	struct timeval timer;
	fd_set rd;

	if(argc == 0) // Launched from Workbench (icon click)
	{
		struct Task *task = FindTask(NULL);
		if (task)
		{
			programName = task->tc_Node.ln_Name;
		}
	}
	else if(argc > 1) // Launched from Shell/CLI
	{
		programName=argv[0];

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

		if(argc > 2) tcpPort = atoi(argv[2]);

		/*if(argc > 3)
		{
			if(stricmp(argv[3], "debug")==0) debug = TRUE;
		}*/
	}

	// Needed right now for EZReq
    IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0);

	if (!(ReqToolsBase = (struct ReqToolsBase *)OpenLibrary (REQTOOLSNAME, 0)))
	{
		const char msg[] = "DCTelnet - Requirement Warning\n\n"
							"reqtools.library can not be loaded.\n"
							"DCTelnet requires ReqTools library to run.\n"
							"It is available on Aminet: util/libs/ReqToolsUsr\n"
							"Please install it and try again.";
		PutStr(msg);
		EZReq(NULL, msg);
		return RETURN_FAIL;
	}

	fileHandle = Open(prefsFilename, MODE_OLDFILE);
	if(fileHandle)
	{
		if(Read(fileHandle, &prefs, sizeof(struct PrefsStruct)) < 252) // IF OLD CONFIG FILE
		{
			/*//prefs.win_left = 0;
			prefs.win_top = 11;
			prefs.win_width = 640;
			prefs.win_height = 200;
			//prefs.sb_left = 0;
			prefs.sb_top = 12;
			prefs.sb_width = 640;
			prefs.sb_height = (prefs.DisplayHeight / 2) - 4;*/

			Close(fileHandle);
			goto fixprefs;
		}
		Close(fileHandle);
	} else {

		isAppIconified = TRUE;

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
			prefs.flags = FLAG_TOOL_BAR;
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

	fileHandle = Open(keysFilename, MODE_OLDFILE);
	if(fileHandle)
	{
		Read(fileHandle, keys, 1520);
		Close(fileHandle);
	}

	scrollbackList = AllocMem(sizeof(struct List), MEMF_CLEAR|MEMF_PUBLIC);
	if(!scrollbackList) goto xit;
	scrollbackList->lh_TailPred = (struct Node *)scrollbackList;
	scrollbackList->lh_Head = (struct Node *)&scrollbackList->lh_Tail;

	GfxBase = ReqToolsBase -> GfxBase;
	GadToolsBase = ReqToolsBase -> GadToolsBase;
	UtilityBase = ReqToolsBase -> UtilityBase;

	WorkbenchBase = OpenLibrary("workbench.library", 0);
	DiskfontBase = OpenLibrary("diskfont.library", 0);
	KeymapBase = OpenLibrary("keymap.library", 0);
	IconBase = OpenLibrary("icon.library", 0);
	SocketBase = OpenLibrary("bsdsocket.library", 0);

	shouldReopenScreen = TRUE;
restart:
	if(OpenDisplay(shouldReopenScreen))
	{
		if(!shouldRestart && server[0] != 0 && !isConnected) Connect_To_Server(server, tcpPort);
		shouldRestart = FALSE;
		shouldReopenScreen = FALSE;

		timer.tv_sec = 30;
		timer.tv_usec = 0;


/* ------ main loop ------ */

// BF: useless label, it is nevered used in function "main".
//startloop:
	while(!shouldQuitApp)
	{

		if(shouldIconify)
		{
			CloseDisplay(TRUE);
			// inconify the application:
			OpenIcon();
			shouldIconify = FALSE;
		}

		if(shouldUniconify)
		{
			CloseIcon();
			OpenDisplay(TRUE);
			shouldUniconify = FALSE;
		}

		if(isAppIconified)
		{
			if(iconPort) iconsig = 1L<<iconPort->mp_SigBit;

			if(isConnected)
			{
				FD_ZERO(&rd);
				FD_SET(tcpSocket, &rd);
				sigmask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | iconsig;

				i = WaitSelect(tcpSocket + 1, &rd, 0, 0, &timer, &sigmask);

			} else {
				i = 0;
				sigmask = Wait( SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | iconsig );
			}

	 		if(sigmask&SIGBREAKF_CTRL_F) shouldUniconify = TRUE;

			if(sigmask&SIGBREAKF_CTRL_C) shouldQuitApp = TRUE;

			if(sigmask&iconsig)
			{
				// Workbench sends AppMessage to the application's message port to notify it
				// https://wiki.amigaos.net/wiki/Workbench_Library#The_AppMessage_Structure
				register struct AppMessage *appmsg;
				while(appmsg = (struct AppMessage *)GetMsg(iconPort))
				{
					if(appmsg->am_NumArgs==0 && appmsg->am_ArgList==0) shouldUniconify = TRUE;
					ReplyMsg((struct Message *)appmsg);
				}
			}

			if(i != 0) Receive();

		} else { // app is not iconified

			winsig = 1L << win->UserPort->mp_SigBit;
			if(isConnected)
			{
				FD_ZERO(&rd);
				FD_SET(tcpSocket, &rd);

				sigmask = winsig;

				if(scrollbackWin) sigmask |= 1L << scrollbackWin->UserPort->mp_SigBit;
				if(pwin) sigmask |= 1L << pwin->UserPort->mp_SigBit;
				if (toolBarWin) sigmask |= 1L << toolBarWin->UserPort->mp_SigBit;

				i = WaitSelect(tcpSocket + 1, &rd, 0, 0, &timer, &sigmask);

				GetWindowMsg(win);

				if(scrollbackWin) GetWindowMsg(scrollbackWin);
				if(pwin) GetWindowMsg(pwin);
				if (toolBarWin) GetWindowMsg(toolBarWin);

				if(i != 0) Receive();

			} else {  // not connected
				ULONG sig;

				if(scrollbackWin)  sig = 1L << scrollbackWin->UserPort->mp_SigBit; else sig = 0;
				if(pwin) sig |= 1L << pwin->UserPort->mp_SigBit;
				if (toolBarWin) sig |= 1L << toolBarWin->UserPort->mp_SigBit;

				sigmask = Wait( sig | winsig | SIGBREAKF_CTRL_C );

				if(scrollbackWin)
				{
					if(sigmask&(1L << scrollbackWin->UserPort->mp_SigBit)) GetWindowMsg(scrollbackWin);
				}
				if(pwin)
				{
					if(sigmask&(1L << pwin->UserPort->mp_SigBit)) GetWindowMsg(pwin);
				}
				if (toolBarWin)
				{
					if(sigmask&(1L << toolBarWin->UserPort->mp_SigBit)) GetWindowMsg(toolBarWin);
				}

				if(sigmask&winsig) GetWindowMsg(win);
				if(sigmask&SIGBREAKF_CTRL_C) shouldQuitApp = TRUE;
			}
			if(shouldRestart)
			{
				CloseDisplay(shouldReopenScreen);
				goto restart;
			}

		}

	}

/* -- end of main loop -- */

		CloseDisplay(TRUE);
		SavePrefs();

		prefs.flags = FLAG_HIDE_TITLEBAR;
		DisConnect(FALSE, TRUE);
	}

	CloseLibrary(WorkbenchBase);
	CloseLibrary(IconBase);
	CloseLibrary(DiskfontBase);
	CloseLibrary(KeymapBase);
	CloseLibrary(SocketBase);

	ClearScrollBack();
	FreeMem(scrollbackList, sizeof(struct List));
xit:
	CloseLibrary((struct Library *)ReqToolsBase);
	if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

	return RETURN_OK;
}

static void SaveScrollBack(char *fname)
{
	struct Scroll *worknode, *nextnode;
	fileHandle = Lock(fname, SHARED_LOCK);
	if(fileHandle)
	{
		UnLock(fileHandle);
		if(!rtEZRequestA("File Already Exists.","OverWrite|Cancel", NULL, NULL, (struct TagItem *)&reqtoolsTags))
			return;
	}
	fileHandle = Open(fname, MODE_NEWFILE);
	if(fileHandle)
	{
		worknode = (struct Scroll *)scrollbackList->lh_Head;
		while(worknode)
		{
			nextnode = (struct Scroll *)worknode -> nnode.ln_Succ;
			if(!nextnode) break;

			Write(fileHandle, (char *) (long)worknode + sizeof(struct Scroll), worknode->len-2);
			Write(fileHandle, "\n", 1);
			worknode = nextnode;
		}
		Close(fileHandle);
	}
}

static void Connect(char thread)
{
	char tbuf[64];
	UWORD port = 0;

	if(!thread) strcpy(tbuf, server); else tbuf[0] = 0;

	if(rtGetStringA(tbuf, 63, "Enter host,port:", 0, (struct TagItem *)&reqtoolsTags))
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
				tcpPort = port;
				Connect_To_Server(tbuf, tcpPort);
			}
		}
	}
}

static void Information(void)
{
	if(isConnected)
	{
		register long spent = mytime() - conectionTime;

		mysprintf(buf,	"     Host Name ... : %s\n"
				"    IP Address ... : %s\n"
				"      TCP Port ... : %ld\n\n"
				"   Online Time ... : %02ld:%02ld:%02ld\n"
				"    Bytes Sent ... : %ld\n"
				"Bytes Received ... : %ld",
			hostAddr->h_name,
			Inet_NtoA(inetSocketAddr.sin_addr.s_addr),
			tcpPort,
			spent/3600, (spent/60)%60, spent%60,
			nBytesSent,
			nBytesReceived);

		rtEZRequestTags(buf, "OK", NULL, NULL,
				RT_Window,	win,
				RT_ReqPos,	REQPOS_CENTERSCR,
				TAG_DONE);
	} else
		SimpleReq("Not isConnected");
}


/**
 * @brief Updates prefs.flags based on a menu item's checked state.
 *
 * If the menu item is checked, the prefs.flags bit is set. If it is unchecked, the flag bit is
 * cleared.
 *
 * @param item Pointer to the MenuItem whose CHECKED state is to be used.
 * @param flag The flag (bit) in prefs.flags to set or clear.
 */
static void UpdatePrefsFlagFromMenu(struct MenuItem *item, ULONG flag)
{
	if(item->Flags & CHECKED)
		prefs.flags |= flag;
	else
		prefs.flags &= ~flag;
}

static void OutKey(unsigned char key)
{
	if(prefs.flags & FLAG_BS_DEL_SWAP)
	{
		if(key == 8)
			key = 127;
		else {
			if(key == 127) key = 8;
		}
	}
	if(isConnected)
	{
		TCPSend((void *)&key, 1);

		// If you want to send 0xff then you must double it (0xff, 0xff) to tell telnet that you don't intend to send it a command.
		if(key==(unsigned char)255) TCPSend((void *)&key, 1);
		if(!passFlag)
		{
			TCPSend("\377\375\000\377\373\000", 6); // 8-bit data path
			passFlag = TRUE;
		}
		if(prefs.flags & FLAG_LOCAL_ECHO) goto cwrite;
	} else
cwrite:		ConWrite(&key, 1);
}


static void GetWindowMsg(struct Window *wwin)
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
	BOOL shouldCloseToolbarWin = FALSE;
	static char key_csi;
	static char key_macro;

	while (message = GT_GetIMsg(wwin->UserPort))
	{
		class = message->Class;
		code = message->Code;
		gad = (struct Gadget *)message->IAddress;
		qual = message->Qualifier;
		GT_ReplyIMsg(message);

		switch (class)
		{
		case IDCMP_GADGETUP:
			if(wwin == pwin)
			{
				RemoveGList(wwin, &strGad, 1);
				if(prefs.flags & FLAG_RETURN_CRLF)
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

			if(wwin == scrollbackWin)
			{
				GetAttr(PGA_Top, Scroller, (ULONG *)&lasttop);
				RefreshListView(lasttop);
			}

			// The gadget in top right corner when title bar is hidden in full screen mode
			if(gad->GadgetID == 20) ScreenToBack(scr);

			if(wwin == toolBarWin)
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
						if(isConnected)
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
							shouldQuitApp = TRUE;
						break;
				}
			}
			break;


		case IDCMP_NEWSIZE:
			//LocalPrint("\017\233\164\233\165\233\166\233\167");
			if(wwin == scrollbackWin) resize = TRUE;
			break;


		/*case IDCMP_REFRESHWINDOW:
			GT_RefreshWindow(wwin, NULL);
			break;*/


		case IDCMP_RAWKEY:
			if(wwin == scrollbackWin)
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
					if(rtEZRequestA("Print Scrollback?", "Print|Cancel", NULL, NULL, (struct TagItem *)&reqtoolsTags))
						SaveScrollBack("PRT:");
					break;
				case 80:
					ClearScrollBack();
					nScrollbackLines = 0;
					lasttop = 0;
					SetGadgetAttrs((struct Gadget *)Scroller, scrollbackWin, NULL,
						PGA_Total,	0,
					TAG_DONE);
					RefreshListView(0);
					break;
				}
			}
			if(wwin == win  ||  wwin == toolBarWin)
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
									if(conbuf[i] == '\r' && (prefs.flags & FLAG_RETURN_CRLF)) OutKey('\n');
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
				case 0: // Menu DC Telnet
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
						if(wwin != scrollbackWin)
						{
							CloseScrollBack();
							OpenScrollBack(lasttop);
						}
						break;
					case 3:
						shouldIconify = TRUE;
						break;

					case 4:
						SpeedTest();
						break;
					case 5:
						WindowSub(Finger);
						break;
					case 7:
						shouldQuitApp = TRUE;
						break;
					}
					break;
				case 1: // Menu Transfer
					if(isConnected)
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
								fileHandle = Open(buf, MODE_OLDFILE);
								if(fileHandle)
								{
									while(r = Read(fileHandle, buf, sizeof buf))
									{
										register long i;
										for(i=0; i<r; i++)
										{
											if(buf[i] == 10 && buf[i+1] != 13 && buf[i-1] != 13) buf[i] = 13;
										}
										TCPSend(buf, r);
									}
									Close(fileHandle);
								}
							}
							break;

						}
					} else
						SimpleReq("You better connect first.");
					break;
				case 2: // Menu Connection
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
				case 3: // Menu Options
					switch(itemNum)
					{
					case 0:
						UpdatePrefsFlagFromMenu(item, FLAG_USE_WORKBENCH);
						shouldRestart = TRUE;
						shouldReopenScreen = TRUE;
						break;
					case 1: // Disable LEDs
						if(item->Flags & CHECKED)
						{
							prefs.flags |= FLAG_HIDE_LEDS;

							if(!(prefs.flags & FLAG_HIDE_TITLEBAR))
							{
								SetAPen(&scr->RastPort, 1);
								RectFill(&scr->RastPort, scr->Width-86, 2, scr->Width-60, prefs.fontsize-1);
							}
						} else {
							prefs.flags &= ~FLAG_HIDE_LEDS;
							LEDs();
						}
						break;
					case 2:
						UpdatePrefsFlagFromMenu(item, FLAG_HIDE_TITLEBAR);
						shouldRestart = TRUE;
						shouldReopenScreen = TRUE;
						break;
					case 3:
						UpdatePrefsFlagFromMenu(item, FLAG_CRLF_CORRECTION);
						break;
					case 4:
						UpdatePrefsFlagFromMenu(item, FLAG_BS_DEL_SWAP);
						break;
					case 5:
						UpdatePrefsFlagFromMenu(item, FLAG_DISABLE_SCROLLBACK);
						break;
					case 6:
						UpdatePrefsFlagFromMenu(item, FLAG_STRIP_COLOUR);
						if(item->Flags & CHECKED) LocalPrint("›m");
						break;
					case 7:
						UpdatePrefsFlagFromMenu(item, FLAG_SIMPLE_TELNET);
						break;
					case 8:
						if (isRunningOnWB)
							SimpleReq("Packet Window cannot work in Workbench mode.");
						else
							shouldRestart = TRUE;

						UpdatePrefsFlagFromMenu(item, FLAG_PACKET_WINDOW);
						break;

					case 9:
						shouldRestart = TRUE;
						UpdatePrefsFlagFromMenu(item, FLAG_USE_XEM_LIBRARY);
						break;

					case 10:
						UpdatePrefsFlagFromMenu(item, FLAG_TOOL_BAR);
						if (isRunningOnWB)
						{
							if(item->Flags & CHECKED)
								OpenToolBarWindow(TRUE);
							else
								CloseToolBarWindow();
						} else
							shouldRestart = TRUE;
						break;

					case 11:
						UpdatePrefsFlagFromMenu(item, FLAG_RETURN_CRLF);
						break;

					case 12:
						UpdatePrefsFlagFromMenu(item, FLAG_LOCAL_ECHO);
						break;

					case 13:
						UpdatePrefsFlagFromMenu(item, FLAG_RAW_CONNECTION);
						break;
					case 14:
						UpdatePrefsFlagFromMenu(item, FLAG_JUMP_SCROLL);
						if(!isRunningOnWB && !(prefs.flags & FLAG_USE_XEM_LIBRARY)) shouldRestart = TRUE;
						break;
					}
					break;

				case 4: // Menu Settings
					switch(itemNum)
					{
					case 0: // Screen Mode...
						if(ChooseScreen(FALSE))
						{
							shouldRestart = TRUE;
							shouldReopenScreen = TRUE;
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
								shouldRestart = TRUE;
								shouldReopenScreen = TRUE;
							}
							rtFreeRequest(fontreq);
						}
						break;
					case 2:
						reqinfo = rtAllocRequestA(RT_REQINFO, NULL);
						if(reqinfo)
						{
							if(rtPaletteRequestA("Screen Palette..", reqinfo, (struct TagItem *)&reqtoolsTags) != -1)
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
						rtGetStringA(prefs.xferinit, 51, "Protocol Init..", 0, (struct TagItem *)&reqtoolsTags);
						break;

					case 6:
						WindowSub(FunctionKeys);
						break;

					case 7:
						if(FileReq("LIBS:", "xem#?.library", prefs.displaydriver, "XEM Library..", FALSE, FREQF_PATGAD))
						{
							if(prefs.flags & FLAG_USE_XEM_LIBRARY) shouldRestart = TRUE;
						}
						break;

					case 8:
						rtGetStringA(prefs.displayidstr, 31, "Telnet Display ID..", 0, (struct TagItem *)&reqtoolsTags);
						break;

					case 9:
						rtGetLongA(&prefs.sb_lines, "ScrollBack Lines..", NULL, (struct TagItem *)&reqtoolsTags);
						break;

					case 10: // Snapshot Windows
						prefs.win_top = win->TopEdge;
						prefs.win_left = win->LeftEdge;
						prefs.win_height = win->Height;
						prefs.win_width = win->Width;

						if(scrollbackWin)
						{
							prefs.sb_left = scrollbackWin->LeftEdge;
							prefs.sb_top = scrollbackWin->TopEdge;
							prefs.sb_width = scrollbackWin->Width;
							prefs.sb_height = scrollbackWin->Height;
						}
						if (toolBarWin)
						{
							prefs.toolBarWin_left = toolBarWin->LeftEdge;
							prefs.toolBarWin_top = toolBarWin->TopEdge;
						}

						break;
					}
					break;
				case 5: // Menu Login
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
			if(wwin == win) shouldQuitApp = TRUE;
			if(wwin == scrollbackWin) close = TRUE;
			if(wwin == toolBarWin) shouldCloseToolbarWin = TRUE;
			break;


		case IDCMP_IDCMPUPDATE:
			switch((UWORD)GetTagData(GA_ID, 0, (struct TagItem *)gad))
			{
			case GAD_UP:
up:				if(lasttop > 0) lasttop--;
				break;

			case GAD_DOWN:
down:				if(lasttop+((scrollbackWin->Height - (prefs.fontsize + scr->WBorTop + 2)) / prefs.fontsize) < nScrollbackLines) lasttop++;
				break;
			}
			SetGadgetAttrs((struct Gadget *)Scroller, scrollbackWin, NULL,
				PGA_Top,	lasttop,
			TAG_DONE);

			RefreshListView(lasttop);

		}
	}
// BF: useless label, it is never used in function "GetWindowMsg"
//xit:

	if(resize)
	{
		SetRast(scrollbackWin->RPort, 0);
		RefreshWindowFrame(scrollbackWin);
		RefreshListView(lasttop);
		SetGadgetAttrs((struct Gadget *)Scroller, scrollbackWin, NULL,
			PGA_Visible,	(scrollbackWin->Height - (prefs.fontsize + scr->WBorTop + 2)) / prefs.fontsize,
		TAG_END);
	}
	if(close) CloseScrollBack();
	if(shouldCloseToolbarWin)
	{
		CloseToolBarWindow();
		prefs.flags &= ~FLAG_TOOL_BAR;
	}
}

static void CheckError(void)
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

// An AmigaOS Task is roughly equivalent to a thread within the program's address space
struct Task *parentTask;
static struct Task *childTask;

// This flag is set by the "Connecting..." window task when the user cancels the operation:
BOOL isConnectionAborted;
UWORD connectMsgType;
char *connectString;

static UWORD Connect_To_ServerA(char *servername, UWORD port);

UWORD Connect_To_Server(char *servername, UWORD port)
{
	UWORD ret;

    parentTask = FindTask(NULL);   // Get current Task


    // In V36 (AmigaOS 2.00 & 2.02), NP_Arguments was broken in a number of ways, and probably
    // should be avoided.
	childTask = (struct Task *)CreateNewProcTags(NP_Entry, HandleConnectingWindowTask, TAG_DONE);

	ret = Connect_To_ServerA(servername, port);
	Signal(childTask, SIGBREAKF_CTRL_C);

	Wait(SIGBREAKF_CTRL_E);

	// Check & clear CTRL_C signal
	while((SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C))
	{
	}

	return(ret);
}

// Notify HandleConnectingWindowTask that a new message should be displayed
static void c_msg(char *msg, UWORD type)
{
	connectString = msg;
	connectMsgType = type;
	Signal(childTask, SIGBREAKF_CTRL_E);

	// Wait for HandleConnectingWindowTask to acknowledge the signal
	Wait(SIGBREAKF_CTRL_E);
}

static UWORD Connect_To_ServerA(char *servername, UWORD port)
{
	if(!SocketBase) SocketBase = OpenLibrary("bsdsocket.library", 0);

	if(!SocketBase)
	{
		SimpleReq("bsdsocket.library can not be loaded.\n\n"
					"You must start the TCP/IP stack first.");
		return(255);
	}

	//  Draw connection activity indicator when Title bar AND LEDs are enabled
	if((prefs.flags & (FLAG_HIDE_TITLEBAR | FLAG_HIDE_LEDS)) == 0)
	{
		SetAPen(&scr->RastPort, 11);
		RectFill(&scr->RastPort, scr->Width-84, 3, scr->Width-76, prefs.fontsize-2);
	}

	DisConnect(FALSE, FALSE);

	c_msg("Looking up...", 4);
	c_msg(servername, 0);

	LocalFmt("\r\nLooking up ›32m%s›m...\r\n", servername);

	strcpy(server, servername);

	isConnectionAborted = 0;
	hostAddr = gethostbyname(server);
	if(!hostAddr)
	{
		if(isConnectionAborted)
			LocalPrint("Host lookup aborted.\r\n");
		else
			LocalPrint("Unknown host. Maybe you misspelt it?\r\n");
		LEDs();
		return(1);
	}

	inetSocketAddr.sin_len = sizeof(inetSocketAddr);
	inetSocketAddr.sin_port = port;
	inetSocketAddr.sin_family = AF_INET;
	inetSocketAddr.sin_addr.s_addr = 0;

	memcpy(&inetSocketAddr.sin_addr, hostAddr->h_addr, hostAddr->h_length);
	//CopyMem(hostAddr->h_addr, &inetSocketAddr.sin_addr, hostAddr->h_length);

	c_msg(Inet_NtoA(inetSocketAddr.sin_addr.s_addr), 1);
	c_msg(hostAddr->h_name, 2);

	LocalFmt("Connecting to ›32m%s›m (›36m%s›m) port ›35m%ld›m...\r\n",
		hostAddr->h_name,
		Inet_NtoA(inetSocketAddr.sin_addr.s_addr),
		port);

	tcpSocket = socket(hostAddr->h_addrtype, SOCK_STREAM, 0);
	if(tcpSocket == -1)
	{
		LocalPrint("Cannot Open Socket.\r\n");
		LEDs();
		return(2);
	}

	c_msg("Connecting...", 4);

	// connect() expects a generic sockaddr, so cast the INet socket address
	if(connect(tcpSocket, (struct sockaddr *)&inetSocketAddr, sizeof(inetSocketAddr)) == -1)
	{
		CheckError();
		shutdown(tcpSocket, 2);
		CloseSocket(tcpSocket);
		LEDs();
		return(3);
	}

	LocalPrint("Connected.\r\n");

	if(!(prefs.flags & FLAG_RAW_CONNECTION))
		TCPSend("\377\375\003", 3);
	else {
		passAll = TRUE;
		passFlag = TRUE;
	}

	if (isRunningOnWB) WindowToFront(win); else ScreenToFront(scr);

	conectionTime = mytime();

	isConnected = TRUE;

	LEDs();

	return(0);
}


 /**
 * @brief Open or reopen the application's display environment.
 *
 *
  * The @p manageScreen parameter controls whether the screen-level resources
 * (screen, font, GadTools visual info, Intuition DrawInfo) must be (re)opened.

 * When @p manageScreen is TRUE, the function:
 * - Opens or locks the screen (custom screen or Workbench screen)
 * - Loads the screen font
 * - Allocates visual and drawing resources
 * - Used at program startup, after a screen mode change,
 *
 * When @p manageScreen is FALSE, the function:
 * - Assumes the screen is already available
 * - Only recreate windows, menus and console bindings
 * - Used when restarting the UI without changing the screen
 *
 * @param manageScreen
 *        TRUE  to (re)open and initialize the screen and all related resources
 *        FALSE to reuse the existing screen and only recreate windows
 *
 * @return TRUE on success, FALSE if the display could not be opened.
 */
BOOL OpenDisplay(BOOL manageScreen)
{
	long i;

	if(!manageScreen) goto skip_screen_open;

	if(prefs.flags & FLAG_USE_WORKBENCH) isRunningOnWB = TRUE; else isRunningOnWB = FALSE;

	fontAttr.ta_Name = prefs.fontname;
	fontAttr.ta_YSize = prefs.fontsize;
	ansiFont = OpenDiskFont(&fontAttr);
	if(!ansiFont)
	{
		fontAttr.ta_Name = "topaz.font";
		fontAttr.ta_YSize = 8;
		ansiFont = OpenFont(&fontAttr);
	}

	if (isRunningOnWB)
	{
		prefs.flags |= FLAG_HIDE_LEDS;
		prefs.flags &= ~FLAG_PACKET_WINDOW;		   // Not Packet Window
		scr = LockPubScreen(0L);
	} else {
		register UWORD *pens;
		static struct NewScreen newscr;

		if(prefs.DisplayDepth < 3) pens = &colorPens[12]; else pens = colorPens;

		memcpy(&newscr.Width, &prefs.DisplayWidth, 6);
		newscr.BlockPen = 1;
		newscr.Type = CUSTOMSCREEN;
		newscr.Font = &fontAttr;
		// Main window title in full screen mode:
		newscr.DefaultTitle = MainWindowTitle;

		scr = OpenScreenTags(&newscr,
			SA_DisplayID,	prefs.DisplayID,
       	       	        SA_Pens,	(ULONG)pens,
			SA_ShowTitle,	!(prefs.flags & FLAG_HIDE_TITLEBAR),
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
			SA_Font,	&fontAttr,
			SA_AutoScroll,	TRUE,
			SA_Interleaved,	TRUE,
       	       	        TAG_END);*/
	}

	if(scr)
	{
		winTop = (scr->WBorTop)+(scr->Font->ta_YSize)+1;
		visualInfos = GetVisualInfoA(scr, 0);
		drawInfo = GetScreenDrawInfo(scr);
skip_screen_open:
		newWin.Screen = scr;
		newWin.Type = PUBLICSCREEN;
		newWin.DetailPen = 255;
		newWin.BlockPen = 255;

		if (isRunningOnWB)
		{
			static UWORD sizes[4] = { 200, 50, 1600, 1200 };

			mynewmenu[24].nm_Flags |= CHECKED;	   // WB
			//mynewmenu[38].nm_Flags = NM_ITEMDISABLED;  // Jump Scroll
			mynewmenu[40].nm_Flags = NM_ITEMDISABLED;  // ScreenMode
			mynewmenu[42].nm_Flags = NM_ITEMDISABLED;  // ScreenPalette

			memcpy(&newWin, &prefs.win_left, 8);
			memcpy(&newWin.MinWidth, &sizes, 8);
			newWin.IDCMPFlags = IDCMP_RAWKEY | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK;
			//newWin.Flags = WFLG_GIMMEZEROZERO|WFLG_NEWLOOKMENUS|WFLG_SIMPLE_REFRESH|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
			newWin.Flags = WFLG_GIMMEZEROZERO|WFLG_NEWLOOKMENUS|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
			// Main window title in windowed workbench mode:
			newWin.Title = MainWindowTitle;
			newWin.FirstGadget = 0;

			CheckDimensions(&newWin);

			win = OpenWindow(&newWin);
			UnlockPubScreen(0L, scr);
			if(prefs.flags & FLAG_TOOL_BAR) OpenToolBarWindow(FALSE);
		} else { // running in full screen
			struct Gadget *backgad;
			UWORD top, height;

			mynewmenu[24].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;	// WB
			//mynewmenu[38].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;	// Jump Scroll
			mynewmenu[40].nm_Flags = 0;	// ScreenMode
			mynewmenu[42].nm_Flags = 0;	// ScreenPalette

			LoadRGB4(&scr->ViewPort, (UWORD *)&prefs.color, 16);

			if(prefs.flags & FLAG_TOOL_BAR) OpenToolBarWindow(FALSE);

			if(prefs.flags&1) // HIDE TITLE
			{
				top = 0;
				height = scr->Height;
				backgad = &screenToBackGadget;
				screenToBackGadget.Width = 20;
				screenToBackGadget.Height = 9;
				screenToBackGadget.Activation = RELVERIFY;
				screenToBackGadget.GadgetType = BOOLGADGET;
				screenToBackGadget.LeftEdge = scr->Width - 20;
				screenToBackGadget.GadgetID = 20;
			} else {
				top = prefs.fontsize + 3;
				height = scr->Height - (prefs.fontsize + 3);
				backgad = 0;
			}

			if (toolBarWin)	// Tool Window
			{
				top = toolBarWin->TopEdge + toolBarWin->Height + 1;
				height = scr->Height - top;
				if(backgad)
				{
					AddGadget(toolBarWin, backgad, -1);
					backgad = 0;
				}
			}

			newWin.LeftEdge = 0;
			newWin.Title = 0;
			newWin.Width = scr->Width;

			if(prefs.flags & FLAG_PACKET_WINDOW)	// Packet
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

				newWin.TopEdge = top+height;
				newWin.Height = prefs.fontsize+2,
				newWin.FirstGadget = &strGad;
				newWin.IDCMPFlags =	IDCMP_MENUPICK |
							IDCMP_GADGETUP;
				newWin.Flags =	WFLG_NEWLOOKMENUS |
						WFLG_BORDERLESS |
						WFLG_BACKDROP;

				pwin = OpenWindow(&newWin);

				SetAPen(pwin->RPort, 1);
				Draw(pwin->RPort, pwin->Width, 0);
			}

			newWin.TopEdge = top;
			newWin.Height = height;
			newWin.FirstGadget = backgad;
			newWin.IDCMPFlags =	IDCMP_GADGETUP |
						IDCMP_RAWKEY |
						IDCMP_CLOSEWINDOW |
						IDCMP_MENUPICK;
			newWin.Flags =	WFLG_SMART_REFRESH |
					WFLG_NEWLOOKMENUS |
					WFLG_BORDERLESS |
					WFLG_ACTIVATE |
					WFLG_BACKDROP;

			win = OpenWindow(&newWin);
		}

		if(win)
		{
			if((prefs.flags & FLAG_USE_XEM_LIBRARY) && prefs.displaydriver[0])
			{
				drivertype = DRIVER_XEM_LIB;
				mynewmenu[38].nm_Flags = NM_ITEMDISABLED;
			} else {
				drivertype = DRIVER_NORMAL;
				mynewmenu[38].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
				prefs.flags &= ~FLAG_USE_XEM_LIBRARY;
			}

			if(prefs.flags & FLAG_HIDE_TITLEBAR)
				mynewmenu[26].nm_Flags |= CHECKED;
			else
				mynewmenu[26].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;

			if(prefs.flags & FLAG_CRLF_CORRECTION) // CRLF
				mynewmenu[27].nm_Flags |= CHECKED;
			else
				mynewmenu[27].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;

			if(prefs.flags & FLAG_HIDE_LEDS)
				mynewmenu[25].nm_Flags |= CHECKED;
			else
				mynewmenu[25].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;

			for(i=4; i<15; i++)
			{
				if(prefs.flags & (1<<i))
					mynewmenu[i+24].nm_Flags |= CHECKED;
				else
					mynewmenu[i+24].nm_Flags &= ~CHECKED;
					//mynewmenu[i+24].nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
			}

			reqtoolsTags[0] = RT_Window;
			reqtoolsTags[1] = (ULONG)win;
			reqtoolsTags[2] = RT_WaitPointer;
			reqtoolsTags[3] = TRUE;

			SetFont(win->RPort, ansiFont);

			if(menuStrip = CreateMenusA(mynewmenu, 0))
			{
				static ULONG ltags[] = { GTMN_NewLookMenus, TRUE, TAG_END };

				register struct MenuItem *item = menuStrip->FirstItem->NextItem->NextItem->NextItem->NextItem->NextItem->NextItem->NextItem;
				if(prefs.DisplayDepth > 1) ((struct IntuiText *)item->ItemFill)->FrontPen = 15;
				//item->Flags |= HIGHBOX;
				//item->Flags &= ~HIGHCOMP;
				item->Flags = 150;

				ltags[1] = isRunningOnWB;
 				LayoutMenusA(menuStrip, visualInfos, (struct TagItem *)&ltags);

				SetMenuStrip(win, menuStrip);

				if(pwin) ResetMenuStrip(pwin, menuStrip);
				if (toolBarWin) ResetMenuStrip(toolBarWin, menuStrip);

				if(drivertype == DRIVER_XEM_LIB) goto lib;
cantfind:
                // Exec Device I/O Functions docs:
                // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node02A5.html

				// needed for async IO replies, but we will do sync IO anyway, so unused:
				WriteConPort = CreateMsgPort();
				if(WriteConPort)
				{
					char *dev;
					UWORD unit;

				   // Doc about OpenDevice() to open a console device :
		   // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node029E.html
           // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0509.html

		   // The console device :
		   // https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0080.html

					//the window that is used by the console device for output:
					writeIOReq.io_Data = win;
					writeIOReq.io_Length = sizeof(struct Window);

					// needed for async IO replies, but we will do sync IO anyway, so useless:
					writeIOReq.io_Message.mn_ReplyPort = WriteConPort;

					// The unit number that is a standard parameter for an open call is used
					// specially by this device.
					if (isRunningOnWB)
					{
						dev = "console.device";
						unit = CONU_SNIPMAP;
					} else {
						dev = "ibmcon.device";
						if(prefs.flags & FLAG_JUMP_SCROLL)
							unit = 2; // unit 2 is undocumented in the NDK and AmigaOS docs
						else
							unit = CONU_CHARMAP;
					}


					i = OpenDevice(dev, unit, (struct IORequest *)&writeIOReq, CONFLAG_DEFAULT);

					if(i == 0)
					{
lib:
						if(drivertype == DRIVER_XEM_LIB)
						{
							XEmulatorBase = OpenLibrary(prefs.displaydriver, 0);
							if(!XEmulatorBase)
							{
								drivertype = DRIVER_NORMAL;
								mysprintf(buf,	"Failed to open XEM library: %s",
												prefs.displaydriver);
								SimpleReq(buf);

								goto cantfind;
							}
							xemIO.xem_window	= win;
							xemIO.xem_font 		= ansiFont;
							//xemIO.xem_signal	= 0;
							xemIO.xem_screendepth	= scr->BitMap.Depth;
							xemIO.xem_swrite	= (long (* )(UBYTE * , LONG ))xpr_swrite;
							xemIO.xem_sread		= (long (* )(UBYTE * , LONG , LONG ))xpr_sread;
							// BF : xpr_gets() does nothing. just a return 0. should we put a NULL instead ?
							// seems to be the usual value for unimplemented callback functions
							xemIO.xem_sbreak	= (long (* )(void))xpr_gets;
							xemIO.xem_sstart	= (void (* )(void))xpr_gets;
							xemIO.xem_sstop		= (long (* )(void))xpr_gets;
							xemIO.xem_sflush	= (long (* )(void))xpr_sflush;
							xemIO.xem_toptions	= NULL; 	// (unsigned long (* )(LONG , struct xem_option ** ))xpr_gets;
							xemIO.xem_tgets		= (long (* )(UBYTE * , UBYTE * , ULONG ))xpr_gets;
							xemIO.xem_tbeep		= (void (* )(ULONG , ULONG ))xpr_gets;
							//xemIO.xem_console	= 0;
							//xemIO.xem_process_macrokeys = 0;

							XEmulatorSetup(&xemIO);
							XEmulatorOpenConsole(&xemIO);
							XEmulatorMacroKeyFilter(&xemIO, NULL);
						}

						isAppIconified = FALSE;

						LEDs();

						if(!isConnected)
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

/**
 * @brief Close the application's display environment.
 *
 * This function closes all application windows and releases display-related
 * resources.
 *
 * The 'manageScreen' parameter controls whether screen-level resources
 * should be released or preserved.
 *
 * When @p manageScreen is TRUE, the function also:
 * - Frees visual and drawing resources
 * - Closes the screen (if not running on the Workbench screen)
 * - Releases the screen font
 * - Used when quitting the program, changing screen mode.
 *
 * When @p manageScreen is FALSE, only windows are closed, allowing the screen to remain open
 *
 * @param manageScreen
 *        TRUE  to fully close the screen and all display resources
 *        FALSE to close windows only and keep the screen open
 */
void CloseDisplay(BOOL manageScreen)
{
	if(drivertype == DRIVER_XEM_LIB)
	{
		if(win)
		{
			XEmulatorCloseConsole(&xemIO);
			XEmulatorCleanup(&xemIO);
			CloseLibrary(XEmulatorBase);
		}
	} else {
		// https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0190.html
		if(writeIOReq.io_Error == 0) CloseDevice((struct IORequest *)&writeIOReq);
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
	CloseToolBarWindow();

	if(menuStrip)	FreeMenus(menuStrip);

	if(manageScreen)
	{
		if(visualInfos)		FreeVisualInfo(visualInfos);
		if(drawInfo)	FreeScreenDrawInfo(scr, drawInfo);
		if(!isRunningOnWB && scr)	CloseScreen(scr);
		if(ansiFont)	CloseFont(ansiFont);
	}

	isAppIconified = TRUE;
}

// inconify the application
void OpenIcon(void)
{
	if(iconPort = CreateMsgPort())
	{
		// reads in a Workbench disk object in from disk. The name parameter will have ".info"
		// postpended to it, and the icon file of that name will be read.
		// If the call fails, it will return zero.
		diskObj = GetDiskObjectNew(programName);
		if(diskObj)
		{
			// Add an icon on Workbench backdrop to inconify the application:
			// Return immediately if AddAppIconA() succeeds.
			if(appIconOnWB = AddAppIconA(0, 0, "DCTelnet", iconPort, 0, diskObj, 0)) return;

			FreeDiskObject(diskObj);
		}
		DeleteMsgPort(iconPort);
	}
	iconPort = 0;
}

// shouldUniconifyify the application
void CloseIcon(void)
{
	if(iconPort)
	{
		RemoveAppIcon(appIconOnWB);
		FreeDiskObject(diskObj);
		DeleteMsgPort(iconPort);
	}
}
