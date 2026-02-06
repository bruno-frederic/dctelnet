/* DCTelnet - Transfer Routines */

#define __USE_SYSBASE

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <libraries/reqtools.h>
#include <proto/reqtools.h>           // rtAllocRequestA (RT_FILEREQ…) in Upload()
#include <proto/Xpr.h>
#include <netdb.h>
#include <proto/bsdsocket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

struct RastPort *xrp;
struct Library *XProtocolBase;
struct Window *xferwin;
struct rtFileList *uplist, *upfirst;
struct XPR_IO xio;

extern struct Screen *scr;
extern struct DrawInfo *DrawInfo;
extern struct Window *win, *twin;
extern struct TextFont *ansifont;
extern struct Menu *menuStrip;
extern struct MsgPort *iconport;
extern struct NewWindow nwin;

extern long sok, bytes, sent;
extern unsigned char buf[2048];
extern char server[64];
extern UWORD WinTop;
extern UBYTE done, icon, unicon;

UWORD lastgs;
UWORD xfertype, inflag;


struct PrefsStruct
{
	ULONG	DisplayID;
        UWORD	DisplayWidth;
        UWORD	DisplayHeight;
        UWORD	DisplayDepth;
	UWORD	fontsize;
	char	fontname[32];
	char	downloadpath[52];
	char	xferlibrary[52];
	char	xferinit[52];
	UWORD	color[16];
	ULONG	flags;		/* BIT 0 = Hide Title Bar */
	UWORD	win_left,	/* BIT 1 = CRLF Correction */
		win_top,	/* BIT 2 = Hide LEDS */
		win_width,	/* BIT 3 = Use Workbench */
		win_height,	/* BIT 4 = BS/DEL Swap */
		sb_left,	/* BIT 5 = Disable Logging */
		sb_top,		/* BIT 6 = Strip Colour */
		sb_width,	/* BIT 7 = Simple Negotiation */
		sb_height;	/* BIT 8 = Packet Window */
	char	uploadpath[52];
};
extern struct PrefsStruct prefs;


extern void TextFmt(struct RastPort *rP, char *ctl, ...);
extern void LocalFmt(char *ctl, ...);
extern void mysprintf(char *Buffer,char *ctl, ...);
extern void LocalPrint(char *data);
extern void LEDs(void);
extern void ConWrite(char *data, long len);
extern char OpenDisplay(void);
extern void CloseDisplay(void);
extern void OpenIcon(void);
extern void CloseIcon(void);
extern void SavePrefs(void);
extern long TCPSend(char *buf, long len);
extern void SimpleReq(char *str);
extern void CheckDimensions(struct NewWindow *newwin);
long init_xpr(struct XPR_IO *IO);
LONG __SAVE_DS__ __ASM__ xpr_sflush(void);
LONG __SAVE_DS__ __ASM__ xpr_chkabort(void);
char XferWindow(void);


int posX(WORD n)
{
	return((xrp->Font->tf_XSize*n)+4);
}

int posY(WORD n)
{
	return(((xrp->Font->tf_YSize+1)*n)+3+WinTop);
}

long instrip(unsigned char *buff, long length)
{
	register long i = 0, j = 0;
	unsigned char *tb = AllocMem(length+2, MEMF_PUBLIC);
	if(tb)
	{
		while(i < length)
		{
			if(buff[i]==255 && !inflag)
				inflag = TRUE;
			else {
				tb[j] = buff[i];
				j++;
				inflag = FALSE;
			}
			i++;
		}
		CopyMem(tb, buff, j);
		FreeMem(tb, length+2);
		return(j);
	}
	return(length);
}

void ProtoClean(void)
{
	if(!icon)
	{
		OnMenu(xferwin, FULLMENUNUM(1, -1, 0));
		OnMenu(xferwin, FULLMENUNUM(2, -1, 0));
		OnMenu(xferwin, FULLMENUNUM(3, -1, 0));
		OnMenu(xferwin, FULLMENUNUM(4, -1, 0));
		OnMenu(xferwin, FULLMENUNUM(5, -1, 0));
		ClearMenuStrip(xferwin);
		CloseWindow(xferwin);
		LEDs();
	}

	XProtocolCleanup(&xio);
	CloseLibrary(XProtocolBase);
	xpr_sflush();
	//ConWrite("", 1);
}

LONG __SAVE_DS__ __ASM__ xpr_finfo(__REG__(a0, STRPTR name), __REG__(d0, LONG type))
{
	struct FileInfoBlock *fib = AllocMem(sizeof(struct FileInfoBlock), 0);
 	BPTR lck;
	register long result = 0;

	if(!fib) return(0);

 	if(lck = Lock(name, SHARED_LOCK))
	{
		Examine(lck, fib);
		UnLock(lck);
		result = fib->fib_Size;
		if(type == 2) result = 1;
		else if(type != 1) result = 0;
	}
	FreeMem(fib, sizeof(struct FileInfoBlock));
	return(result);
}

LONG __SAVE_DS__ __ASM__ xpr_swrite(__REG__(a0, UBYTE *buffer), __REG__(d0, LONG size))
{
	long ret = 0;
	register ULONG i = 0, j = 0;
	UBYTE *tb = AllocMem(size+size, MEMF_PUBLIC);
	if(tb)
	{
		while(i < size)
		{
			if(buffer[i] == 255)
			{
				tb[j] = buffer[i];
				j++;
			}
			tb[j] = buffer[i];
			j++;
			i++;
		}
		if(TCPSend(tb, j) < 0) ret = -1;
		FreeMem(tb, size+size);
	}
	return(ret);
}

LONG __SAVE_DS__ __ASM__ xpr_sread(__REG__(a0, UBYTE *buffer), __REG__(d0, ULONG size), __REG__(d1, ULONG timeout))
{
	fd_set rd;
	ULONG sig;
	ULONG winsig, set, er;
	long insize;
	struct timeval timer;

	if(!icon) winsig = 1L << xferwin->UserPort->mp_SigBit; else winsig = 0;

	if(timeout)
	{
		while(1)
		{
			if(timeout > 1000000)
			{
				timer.tv_sec = timeout / 1000000;
				timer.tv_usec = timeout % 1000000;
			} else {
				timer.tv_sec = 0;
				timer.tv_usec = timeout;
			}

			sig = winsig;

			FD_ZERO(&rd);
			FD_SET(sok, &rd);

			if(WaitSelect(sok + 1, &rd, 0L, 0L, &timer, &sig) < 0) return(-1);

			if(xpr_chkabort() == -1) return(-1);

			if(FD_ISSET(sok, &rd))
			{
				insize = recv(sok, buffer, size, 0);
				if(insize == -1) return(-1);
				if(insize > 0)
				{
					insize = instrip(buffer, insize);
					bytes += insize;
					return(insize);
				}
			}
		}
	}

	set = 1;
	IoctlSocket(sok, FIONBIO, (char *)&set);
	insize = recv(sok, buffer, size, 0);
	if(insize == -1)
	{
		er = Errno();
		set = 0;
		IoctlSocket(sok, FIONBIO, (char *)&set);
		if(er == EWOULDBLOCK) return 0;
		return -1;
	}
	set = 0;
	IoctlSocket(sok, FIONBIO, (char *)&set);
	insize = instrip(buffer, insize);
	bytes += insize;
	return(insize);

/*	FD_ZERO(&rd);
	FD_SET(sok, &rd);

	sig = winsig;

	timer.tv_sec = 0;
	timer.tv_usec = 1;

	if(WaitSelect(sok + 1, &rd, 0L, 0L, &timer, &sig) < 0) return(-1);

	if(xpr_chkabort() == -1) return(-1);

	if(FD_ISSET(sok, &rd))
	{
		insize = recv(sok, buffer, size, 0);
		if(insize > 0)
		{
			insize = instrip(buffer, insize);
			bytes += insize;
			return(insize);
		}
	}
	return(0);*/
}

LONG __SAVE_DS__ __ASM__
xpr_sflush(void)
{
	fd_set rd;
	struct timeval timer;

	FD_ZERO(&rd);
	FD_SET(sok, &rd);

	timer.tv_sec = 0;
	timer.tv_usec = 1;

	if(WaitSelect(sok + 1, &rd, 0L, 0L, &timer, 0L) < 0) return(-1);

	if(FD_ISSET(sok, &rd)) recv(sok, buf, sizeof buf, 0);

	return(0);
}

LONG __SAVE_DS__ __ASM__ xpr_ffirst(__REG__(a0, STRPTR buffer), __REG__(a1, STRPTR pattern))
{
	strcpy(buffer, prefs.uploadpath);
	strcat(buffer, upfirst->Name);
	return(1);
}

LONG __SAVE_DS__ __ASM__ xpr_fnext(__REG__(d0, LONG oc), __REG__(a0, STRPTR buffer), __REG__(a1, STRPTR pattern))
{
	uplist = uplist->Next;
	if(uplist)
	{
		strcpy(buffer, prefs.uploadpath);
		strcat(buffer, uplist->Name);
		return(oc);
	}
	return(0);
}

LONG __SAVE_DS__ __ASM__ xpr_gets(__REG__(a0, STRPTR Prompt), __REG__(a1, STRPTR Buffer))
{
	/* The first argument is a pointer to a string containing a prompt, to be displayed by the
	communications program in any manner it sees fit. The second argument should be a pointer to a
	buffer to receive the user's response. It should have a size of at least 256 bytes.
	The function returns 0L on failure or user cancellation, non-zero on success.
	*/
	return(0);
}

LONG __SAVE_DS__ __ASM__ xpr_fopen(__REG__(a0, STRPTR FileName), __REG__(a1, STRPTR AccessMode))
{
	register long fh;

	if(!icon) EraseRect(xrp, 21, posY(9), xferwin->Width-21, posY(9)+9);
	lastgs = 0;

	switch(*AccessMode)
	{
	case 'r':
		return(Open(FileName, MODE_OLDFILE));

	case 'w':
		if(fh=Open(FileName, MODE_NEWFILE))
		{
			Close((BPTR)fh);
			SetComment(FileName, server);
			return(Open(FileName, MODE_OLDFILE));
		}
		break;

	case 'a':
		if(fh=Open(FileName, MODE_READWRITE))
		{
			Seek((BPTR)fh, 0, OFFSET_END);
			return(fh);
		}
	}
	return(0);
}

LONG __SAVE_DS__ __ASM__ xpr_fclose(__REG__(a0, BPTR File))
{
	if(File) Close(File);
	return(0);
}

LONG __SAVE_DS__ __ASM__ xpr_fread(__REG__(a0, APTR Buffer), __REG__(d0, LONG Size), __REG__(d1, LONG Count), __REG__(a1, BPTR File))
{
	if(Size==0 || Count==0) return(0);
	return(Read(File,Buffer,Size*Count));
}

LONG __SAVE_DS__ __ASM__ xpr_fwrite(__REG__(a0, APTR Buffer), __REG__(d0, LONG Size), __REG__(d1, LONG Count), __REG__(a1, BPTR File))
{
	if(Size==0 || Count==0) return(0);
	return(Write(File,Buffer,Size*Count));
}

LONG __SAVE_DS__ __ASM__ xpr_fseek(__REG__(a0, BPTR File), __REG__(d0, LONG Offset), __REG__(d1, LONG Origin))
{
	register long h;

	switch(Origin)
	{
		case 0: h=OFFSET_BEGINNING; break;
		case 1: h=OFFSET_CURRENT; break;
		case 2: h=OFFSET_END; break;
		default: return(-1);
	}
	return((Seek(File,Offset,h)!=-1)?0:-1);
}

LONG __SAVE_DS__ __ASM__ xpr_unlink(__REG__(a0, STRPTR FileName))
{
	return(DeleteFile(FileName));
}

LONG __SAVE_DS__ __ASM__ xpr_update(__REG__(a0, struct XPR_UPDATE *xu))
{
	register long ud = xu->xpru_updatemask;
	register UWORD gs;

	if(icon) return(0);

	if(ud&XPRU_PROTOCOL)
	{
		Move(xrp, posX(12), posY(1));
		Text(xrp, xu->xpru_protocol, strlen(xu->xpru_protocol));
	}
	if(ud&XPRU_FILENAME)
	{
		Move(xrp, posX(12), posY(2));
		if(xfertype == 1)
			Text(xrp, prefs.downloadpath, strlen(prefs.downloadpath));
		else
			Text(xrp, prefs.uploadpath, strlen(prefs.uploadpath));

		Move(xrp, posX(12), posY(3));
		TextFmt(xrp, "%-30s", FilePart(xu->xpru_filename));
	}
	if(ud&XPRU_FILESIZE)
	{
		Move(xrp, posX(16), posY(4));
		TextFmt(xrp, "%-10ld", xu->xpru_filesize);
	}
	if(ud&XPRU_BYTES)
	{
		Move(xrp, posX(16), posY(5));
		TextFmt(xrp, "%-10ld", xu->xpru_bytes);

		if(xu->xpru_filesize > 0)
		{
			Move(xrp, posX(45), posY(6));
			TextFmt(xrp, "%ld%%  ", (xu->xpru_bytes*100)/xu->xpru_filesize);
			gs = (xu->xpru_bytes*(xferwin->Width-42))/xu->xpru_filesize;
		}
		if(gs > lastgs)
		{
			SetAPen(xrp, DrawInfo->dri_Pens[FILLPEN]);
			RectFill(xrp, 21+lastgs, posY(9), 21+gs, posY(9)+9);
			SetAPen(xrp, DrawInfo->dri_Pens[TEXTPEN]);
			lastgs = gs;
		}
	}
	if(ud&XPRU_BLOCKCHECK)
	{
		Move(xrp, posX(16), posY(6));
		TextFmt(xrp, "%-10s", xu->xpru_blockcheck);
	}
	if(ud&XPRU_ERRORS)
	{
		Move(xrp, posX(16), posY(7));
		TextFmt(xrp, "%-10ld", xu->xpru_errors);
	}
	if(ud&XPRU_ERRORMSG)
	{
		Move(xrp, posX(16), posY(8));
		TextFmt(xrp, "%-46.46s", xu->xpru_errormsg);
	}
	if(ud&XPRU_MSG)
	{
		Move(xrp, posX(16), posY(8));
		TextFmt(xrp, "%-46.46s", xu->xpru_msg);
	}


/* row 2 */

	if(ud&XPRU_ELAPSEDTIME)
	{
		Move(xrp, posX(45), posY(4));
		TextFmt(xrp, "%-16s", xu->xpru_elapsedtime);
	}
	if(ud&XPRU_EXPECTTIME)
	{
		Move(xrp, posX(45), posY(5));
		TextFmt(xrp, "%-16s", xu->xpru_expecttime);
	}
	if(ud&XPRU_DATARATE)
	{
		Move(xrp, posX(45), posY(7));
		TextFmt(xrp, "%-10ld", xu->xpru_datarate);
	}
	return(0);
}

long Checkwinmsg(struct Window *wwin)
{
	UWORD code;
	UWORD menuNumber, menuNum, itemNum;
	ULONG class;
	struct IntuiMessage *im;
	struct MenuItem *item;
	struct Gadget *gad;
	char close = FALSE;

	while(im=(struct IntuiMessage *)GetMsg(wwin->UserPort))
	{
		code = im->Code;
		class = im->Class;
		gad = (struct Gadget *)im->IAddress;
		ReplyMsg(im);
		switch(class)
		{
			case IDCMP_GADGETUP:
				switch(gad->GadgetID)
				{
					case 20:
						ScreenToBack(scr);
						break;
					case 6:
						done = TRUE;
					case 1:
						return(-1);
					default:
						DisplayBeep(scr);
				}
				break;

			case IDCMP_CLOSEWINDOW:
				if(wwin != twin)
				{
					TCPSend("", 10);
					return(-1);
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
					if(menuNum == 0 && itemNum == 3)
						close = TRUE;
					else
						DisplayBeep(scr);

					menuNumber = item->NextSelect;
				}
		}
	}

	if(close)
	{
		ClearMenuStrip(xferwin);
		CloseWindow(xferwin);
		CloseDisplay();
		OpenIcon();
	}
	return(0);
}

LONG __SAVE_DS__ __ASM__
xpr_chkabort(void)
{
	if(icon)
	{
		if(iconport)
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

		if(SetSignal(0L, SIGBREAKF_CTRL_F) & SIGBREAKF_CTRL_F) unicon = TRUE;

		if(unicon)
		{
			CloseIcon();
			OpenDisplay();
			XferWindow();
			lastgs = 0;
			unicon = FALSE;
		}

		return(0);
	}

	if(Checkwinmsg(xferwin) == -1) return( -1L );
	if(!icon)
	{
		if(Checkwinmsg(win) == -1) return( -1L );
		if(!icon && twin)
		{
			return(Checkwinmsg(twin));
		}
	}

	return(0);
}

LONG __SAVE_DS__ __ASM__
xpr_squery(void)
{
	fd_set rd;
	struct timeval timer;
	long oldsize;

	FD_ZERO(&rd);
	FD_SET(sok, &rd);

	timer.tv_sec = 0;
	timer.tv_usec = 1;

	if(WaitSelect(sok + 1, &rd, 0L, 0L, &timer, 0L) < 0) return(-1);

	if(FD_ISSET(sok, &rd))
	{
		oldsize = recv(sok, buf, sizeof buf, MSG_PEEK);
		if(oldsize == -1) return -1;
		if(oldsize < 1) return(0);

		return(instrip(buf, oldsize));
	}
	return(0);
}

char ProtoStart(char *library, char *firstfile)
{
	XProtocolBase = OpenLibrary(library, 0);
	if(!XProtocolBase)
	{
		LocalPrint("\r\n›0;31mERROR: ›mCould not open transfer library.\r\n");
		return(0);
	}

	xio.xpr_fopen     = xpr_fopen;
	xio.xpr_fclose    = xpr_fclose;
	xio.xpr_fread     = xpr_fread;
	xio.xpr_fwrite    = xpr_fwrite;
	xio.xpr_sread     = xpr_sread;
	xio.xpr_swrite    = xpr_swrite;
	xio.xpr_sflush    = xpr_sflush;
	xio.xpr_update    = xpr_update;
	xio.xpr_chkabort  = xpr_chkabort;
	xio.xpr_ffirst    = xpr_ffirst;
	xio.xpr_fnext     = xpr_fnext;
	xio.xpr_finfo     = xpr_finfo;
	xio.xpr_fseek     = xpr_fseek;
	xio.xpr_gets      = xpr_gets;
	xio.xpr_unlink    = xpr_unlink;
	xio.xpr_squery    = xpr_squery;
	xio.xpr_extension = 1L;

	xio.xpr_filename = prefs.xferinit;//"TC,OR,B32,FO,AN,DN,KY,SN,RN";
	XProtocolSetup(&xio);
	xio.xpr_filename = firstfile;

	if(icon) return(TRUE); else return(XferWindow());
}

char XferWindow(void)
{
	WORD x, y, reuse, right;

	x = 64 * win->RPort->Font->tf_XSize;
	y = (10 * (win->RPort->Font->tf_YSize+1)) + 12;

	nwin.LeftEdge = (scr->Width - x) / 2;
	nwin.TopEdge = (scr->Height - y) / 2;
	nwin.Width = x;
	nwin.Height = y + WinTop;
	nwin.IDCMPFlags = IDCMP_CLOSEWINDOW|IDCMP_MENUPICK;
	nwin.Flags = WFLG_NEWLOOKMENUS|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET;
	nwin.FirstGadget = 0;
	nwin.Title = "Transfer in Progress...";

	CheckDimensions(&nwin);
	xferwin = OpenWindow(&nwin);

/*	xferwin = OpenWindowTags(NULL,
		WA_Title,		"Transfer in Progress...",
		WA_Left,		(scr->Width - x) / 2,
		WA_Top,			(scr->Height - y) / 2,
		WA_Width,		x,
		WA_InnerHeight,		y,
		WA_CustomScreen,	scr,
		WA_IDCMP,		IDCMP_CLOSEWINDOW|IDCMP_MENUPICK,
		WA_Flags,		WFLG_NEWLOOKMENUS|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET,//|WFLG_RMBTRAP,
		TAG_END);*/

	if(!xferwin)
	{
		CloseLibrary(XProtocolBase);
		return(0);
	}

	ResetMenuStrip(xferwin, menuStrip);
	OffMenu(xferwin, FULLMENUNUM(1, -1, 0));
	OffMenu(xferwin, FULLMENUNUM(2, -1, 0));
	OffMenu(xferwin, FULLMENUNUM(3, -1, 0));
	OffMenu(xferwin, FULLMENUNUM(4, -1, 0));
	OffMenu(xferwin, FULLMENUNUM(5, -1, 0));

	xrp = xferwin->RPort;

	SetFont(xrp, ansifont);

	SetAPen(xrp, DrawInfo->dri_Pens[HIGHLIGHTTEXTPEN]);

	/*if(wb)
		SetAPen(xrp, DrawInfo->dri_Pens[SHINEPEN]);
	else
		SetAPen(xrp, DrawInfo->dri_Pens[SHADOWPEN]);*/

	Move(xrp, posX(2), posY(1));
	Text(xrp, "Protocol:", 9);
	Move(xrp, posX(6), posY(2));
	Text(xrp, "Path:", 5);
	Move(xrp, posX(6), posY(3));
	Text(xrp, "Name:", 5);
	Move(xrp, posX(5), posY(4));
	Text(xrp, "File size:", 10);
	Move(xrp, posX(2), posY(5));
	Text(xrp, "Bytes xfer'd:", 13);
	Move(xrp, posX(3), posY(6));
	Text(xrp, "Block check:", 12);
	Move(xrp, posX(8), posY(7));
	Text(xrp, "Errors:", 7);
	Move(xrp, posX(2), posY(8));
	Text(xrp, "Status/Error:", 13);

	Move(xrp, posX(37), posY(4));
	Text(xrp, "Actual:", 7);
	Move(xrp, posX(35), posY(5));
	Text(xrp, "Expected:", 9);
	Move(xrp, posX(34), posY(6));
	Text(xrp, "%  xfer'd:", 10);
	Move(xrp, posX(34), posY(7));
	Text(xrp, "Chars/sec:", 10);

	reuse = posY(9)-1;
	right = xferwin->Width-20;

	SetAPen(xrp, DrawInfo->dri_Pens[SHADOWPEN]);
	Move(xrp, 20, reuse);
	Draw(xrp, right, reuse);
	Move(xrp, 20, reuse);
	Draw(xrp, 20, posY(9)+10);

	SetAPen(xrp, DrawInfo->dri_Pens[SHINEPEN]);
	Move(xrp, 20, posY(9)+10);
	Draw(xrp, right, posY(9)+10);
	Move(xrp, right, posY(9)+10);
	Draw(xrp, right, reuse);

	SetAPen(xrp, DrawInfo->dri_Pens[TEXTPEN]);

	return(1);
}

void Upload(char *library)
{
	struct rtFileRequester *filereq;
	struct rtFileList *flist;

	if(icon) return;

	inflag = FALSE;
	xfertype = 2;

	if (filereq = rtAllocRequestA (RT_FILEREQ, NULL))
	{
		buf[0] = 0;
		rtChangeReqAttr(filereq, RTFI_Dir, prefs.uploadpath, TAG_END);
		flist = rtFileRequest (filereq, buf, "Files to send...",
			RT_Window,	win,
			RT_LeftOffset,	20,
			RT_TopOffset,	11,
			RTFI_Height,	300,
			RTFI_OkText,	"Send",
			RTFI_Flags,	FREQF_MULTISELECT,
			TAG_END);
		if(flist)
		{
			WORD l;

			strcpy(buf, filereq->Dir);
			l = strlen(buf)-1;
			if(l != -1 && buf[l]!='/' && buf[l]!=':')
			{
				l++;
				buf[l] = '/';
				l++;
				buf[l] = 0;
			}

			uplist = flist;
			upfirst = flist;

			strcpy(prefs.uploadpath, buf);
			strcat(buf, flist->Name);
			SavePrefs();

			if(ProtoStart(library, buf))
			{
				XProtocolSend(&xio);
				ProtoClean();
			}
			rtFreeFileList(flist);
		} else {
			xpr_sflush();
			TCPSend("", 10);
		}
		rtFreeRequest(filereq);
	}
}

void Download(char *library)
{
	BPTR old, lck;

	inflag = FALSE;

	lck = Lock(prefs.downloadpath, SHARED_LOCK);
	if(lck)
	{
		old = CurrentDir(lck);

		xfertype = 1;

		if(ProtoStart(library, 0))
		{
			XProtocolReceive(&xio);
			ProtoClean();
		}
		CurrentDir(old);
		UnLock(lck);
	} else {
		TCPSend("", 10);
		SimpleReq("Download path does not exist.");
		xpr_sflush();
	}
}
