/* DCTelnet - Transfer Routines */

#define __USE_SYSBASE

#include <proto/exec.h>               // OpenLibrary(), GetMsg(), ReplyMsg(), SetSignal()...
#include <proto/dos.h>                // Open(), Close(), Read(), Write(), PutStr()...
#include <proto/intuition.h>          // OpenWindow(),CloseWindow(), OnMenu(), OffMenu()...
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
#include <proto/gadtools.h>           // GT_GetIMsg(), GT_ReplyIMsg()
#include <workbench/workbench.h>      // struct AppMessage
#include <libraries/reqtools.h>       // struct rtFileList, RT_FILEREQ, RT_Window
#include <proto/reqtools.h>           // rtAllocRequestA (RT_FILEREQ) in Upload()
#include <proto/Xpr.h>                // XProtocolSetup(), XProtocolSend(), XProtocolReceive(), ...
#include <proto/bsdsocket.h>          // WaitSelect(), recv(), IoctlSocket(), Errno()
#include <sys/ioctl.h>                // FIONBIO
#include "Xfer.h"
#include "DCTelnet.h"
#include "guis.h"

static struct RastPort *xrp;
struct Library *XProtocolBase;
static struct Window *xferwin;
static struct rtFileList *uplist, *upfirst;
static struct XPR_IO xio;

static UWORD xfer_gauge_width;
// Download : xfertype = 1        Upload : xfertype = 2
#define XFER_DOWNLOAD 1
#define XFER_UPLOAD   2
static UWORD xfertype;
// Set when a 0xFF escape byte is pending in the incoming transfer stream :
static UWORD ff_escape_pending;

long xpr_chkabort(void);     /* Check for abort */
static char XferWindow(void);


static int posX(WORD n)
{
	return((xrp->Font->tf_XSize*n)+4);
}

static int posY(WORD n)
{
	return(((xrp->Font->tf_YSize+1)*n)+3+winTop);
}

/*
Strip 0xFF escape bytes from the incoming stream.

Telnet escape a literal 0xFF byte by sending it twice (0xFF 0xFF). This function
collapses such sequences into a single 0xFF.

The ff_escape_pending flag is kept across calls to handle cases where an escape sequence spans
multiple received buffers.

Returns the new buffer length after stripping.
*/
static long instrip(unsigned char *buff, long length)
{
	register long i = 0, j = 0;
	unsigned char *tb = AllocMem(length+2, MEMF_PUBLIC);
	if(tb)
	{
		while(i < length)
		{
			if(buff[i]==255 && !ff_escape_pending)
				ff_escape_pending = TRUE;
			else {
				tb[j] = buff[i];
				j++;
				ff_escape_pending = FALSE;
			}
			i++;
		}
		CopyMem(tb, buff, j);
		FreeMem(tb, length+2);
		return(j);
	}
	return(length);
}

static void ProtoClean(void)
{
	if(!isAppIconified)
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

/*
This function returns information about a file given its name and the type of information requested.
typeofinfo = 1L : file size (bytes)
typeofinfo = 2L : file type (1L is binary ; 2L is text)

returns 0 on failure
*/
long __ASM__ xpr_finfo(__REG__(a0, char *filename),
                       __REG__(d0, long typeofinfo))
{
	struct FileInfoBlock *fib = AllocMem(sizeof(struct FileInfoBlock), 0);
 	BPTR lck;
	register long result = 0;

	if(!fib) return(0);

 	if(lck = Lock(filename, SHARED_LOCK))
	{
		Examine(lck, fib);
		UnLock(lck);
		result = fib->fib_Size;
		if(typeofinfo == 2) result = 1;            // file type is always binary
		else if(typeofinfo != 1) result = 0;       // returns failure with unknown typeofinfo
	}
	FreeMem(fib, sizeof(struct FileInfoBlock));
	return(result);
}

/* Put string to serial */
long __ASM__ xpr_swrite(__REG__(a0, char *buffer),
                        __REG__(d0, long size))
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

/* Get char from serial */
long __ASM__ xpr_sread(__REG__(a0, char *buffer),
                       __REG__(d0, long size),
                       __REG__(d1, long timeout))
{
	fd_set rd;
	ULONG sig;
	ULONG winsig, set, er;
	long insize;
	struct timeval timer;

	if(!isAppIconified) winsig = 1L << xferwin->UserPort->mp_SigBit; else winsig = 0;

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
			FD_SET(tcpSocket, &rd);

			if(WaitSelect(tcpSocket + 1, &rd, 0L, 0L, &timer, &sig) < 0) return(-1);

			if(xpr_chkabort() == -1) return(-1);

			if(FD_ISSET(tcpSocket, &rd))
			{
				insize = recv(tcpSocket, buffer, size, 0);
				if(insize == -1) return(-1);
				if(insize > 0)
				{
					insize = instrip(buffer, insize);
					nBytesReceived += insize;
					return(insize);
				}
			}
		}
	}

	// FIONBIO : A value of 1 enables non-blocking I/O on the socket, a value of 0 disables it.
	set = 1;
	IoctlSocket(tcpSocket, FIONBIO, (char *)&set);

	// recv() return the length (as a long integer) of the message on successful completion.
	// If no messages are available at the socket, the receive call waits for a message to arrive,
	// unless the socket is nonblocking (see IoctlSocket()) in which case the value -1 is returned
	insize = recv(tcpSocket, buffer, size, 0);
	if(insize == -1)
	{
		er = Errno();
		set = 0;
		IoctlSocket(tcpSocket, FIONBIO, (char *)&set);
		if(er == EWOULDBLOCK) return 0;
		return -1; // XPR interpret -1 as an error
	}
	set = 0;
	IoctlSocket(tcpSocket, FIONBIO, (char *)&set);
	insize = instrip(buffer, insize);
	nBytesReceived += insize;
	return(insize);

/*	FD_ZERO(&rd);
	FD_SET(tcpSocket, &rd);

	sig = winsig;

	timer.tv_sec = 0;
	timer.tv_usec = 1;

	if(WaitSelect(tcpSocket + 1, &rd, 0L, 0L, &timer, &sig) < 0) return(-1);

	if(xpr_chkabort() == -1) return(-1);

	if(FD_ISSET(stcpSocketok, &rd))
	{
		insize = recv(tcpSocket, buffer, size, 0);
		if(insize > 0)
		{
			insize = instrip(buffer, insize);
			bytes += insize;
			return(insize);
		}
	}
	return(0);*/
}

/* Flush serial input buffer */
long xpr_sflush(void)
{
	fd_set rd;
	struct timeval timer;

	FD_ZERO(&rd);
	FD_SET(tcpSocket, &rd);

	timer.tv_sec = 0;
	timer.tv_usec = 1;

	if(WaitSelect(tcpSocket + 1, &rd, 0L, 0L, &timer, 0L) < 0) return(-1);

	if(FD_ISSET(tcpSocket, &rd)) recv(tcpSocket, buf, sizeof buf, 0);

	return(0);
}

/* Find first file name */
long __ASM__ xpr_ffirst(__REG__(a0, char *buffer),
                        __REG__(a1, char *pattern))
{
	strcpy(buffer, prefs.uploadpath);
	strcat(buffer, upfirst->Name);
	return(1);
}

/* Find next file name */
long __ASM__ xpr_fnext(__REG__(d0, long oldstate),
				__REG__(a0, char *buffer),
				__REG__(a1, char *pattern))
{
	uplist = uplist->Next;
	if(uplist)
	{
		strcpy(buffer, prefs.uploadpath);
		strcat(buffer, uplist->Name);
		return(oldstate);
	}
	return(0);
}

/* Get string interactively */
long __ASM__ xpr_gets(__REG__(a0, char *prompt),
                      __REG__(a1, char *buffer))
{
	/* The first argument is a pointer to a string containing a prompt, to be displayed by the
	communications program in any manner it sees fit. The second argument should be a pointer to a
	buffer to receive the user's response. It should have a size of at least 256 bytes.
	The function returns 0L on failure or user cancellation, non-zero on success.
	*/
	return(0);
}

/*
The following xpr_fopen(), xpr_fclose() xpr_fread(), xpr_fwrite(), xpr_fseek(), xpr_unlink()
call-back function works in most respects identically to the stdio function fopen(),etc...
Enables external protocols to manipulate files via the communication program.
*/
long __ASM__ xpr_fopen(__REG__(a0, char *filename),
                       __REG__(a1, char *accessmode))
{
	register long fh;

	if(!isAppIconified) EraseRect(xrp, 21, posY(9), xferwin->Width-21, posY(9)+9);
	xfer_gauge_width = 0;

	switch(*accessmode)
	{
	case 'r':
		return(Open(filename, MODE_OLDFILE));

	case 'w':
		if(fh=Open(filename, MODE_NEWFILE))
		{
			Close((BPTR)fh);
			SetComment(filename, server);
			return(Open(filename, MODE_OLDFILE));
		}
		break;

	case 'a':
		if(fh=Open(filename, MODE_READWRITE))
		{
			Seek((BPTR)fh, 0, OFFSET_END);
			return(fh);
		}
	}
	return(0);
}

long __ASM__ xpr_fclose(__REG__(a0, long filepointer))
{
	if(filepointer) Close(filepointer);
	return(0);
}

long __ASM__ xpr_fread(__REG__(a0, char *buffer),
                       __REG__(d0, long size),
                       __REG__(d1, long count),
                       __REG__(a1, long fileptr))
{
	if(size==0 || count==0) return(0);
	return(Read(fileptr,buffer,size*count));
}

long __ASM__ xpr_fwrite(__REG__(a0, char *buffer),
                        __REG__(d0, long size),
                        __REG__(d1, long count),
                        __REG__(a1, long fileptr))
{
	if(size==0 || count==0) return(0);
	return(Write(fileptr,buffer,size*count));
}

long __ASM__ xpr_fseek(__REG__(a0, long fileptr),
                       __REG__(d0, long offset),
                       __REG__(d1, long origin))
{
	register long h;

	switch(origin)
	{
		case 0: h=OFFSET_BEGINNING; break;
		case 1: h=OFFSET_CURRENT; break;
		case 2: h=OFFSET_END; break;
		default: return(-1);
	}
	return((Seek(fileptr,offset,h)!=-1)?0:-1);
}

/* Delete a file. */
long __ASM__ xpr_unlink(__REG__(a0, char *filename))
{
	return(DeleteFile(filename));
}

/*
    This call-back function is intended to communicate a variety of values and strings from the
    external protocol to the communications program for display. Hence, the display format itself
    (requester, text-I/O) is left to the implementer of the communications program.
*/
long __ASM__ xpr_update(__REG__(a0,
                        struct XPR_UPDATE * updatestruct))
{
    /*
		The mask xpru_updatemask indicates which of the other fields are valid, i.e. have had their
		value updated. It is possible to update a single or multiple values.
    */
	register long ud = updatestruct->xpru_updatemask;
	register UWORD new_xfer_gauge_width=0;

	if(isAppIconified) return(0);

    // xpru_protocol    -- a string that indicates the name of the protocol used
	if(ud&XPRU_PROTOCOL)
	{
		Move(xrp, posX(12), posY(1));
		Text(xrp, updatestruct->xpru_protocol, strlen(updatestruct->xpru_protocol));
	}

	// xpru_filename    -- the name of the file currently sent or received
	if(ud&XPRU_FILENAME)
	{
		Move(xrp, posX(12), posY(2));
		if(xfertype == XFER_DOWNLOAD)
			Text(xrp, prefs.downloadpath, strlen(prefs.downloadpath));
		else        // XFER_UPLOAD
			Text(xrp, prefs.uploadpath, strlen(prefs.uploadpath));

		Move(xrp, posX(12), posY(3));
		TextFmt(xrp, "%-30s", FilePart(updatestruct->xpru_filename));
	}

    // xpru_filesize    -- the size of the file
	if(ud&XPRU_FILESIZE)
	{
		Move(xrp, posX(16), posY(4));
		TextFmt(xrp, "%-10ld", updatestruct->xpru_filesize);
	}

    // xpru_bytes       -- number of transferred bytes
	if(ud&XPRU_BYTES)
	{
		Move(xrp, posX(16), posY(5));
		TextFmt(xrp, "%-10ld", updatestruct->xpru_bytes);

		if(updatestruct->xpru_filesize > 0)
		{
			Move(xrp, posX(45), posY(6));
			TextFmt(xrp, "%ld%%  ", (updatestruct->xpru_bytes*100)/updatestruct->xpru_filesize);
			new_xfer_gauge_width = (updatestruct->xpru_bytes*(xferwin->Width-42))/updatestruct->xpru_filesize;
		}
		if(new_xfer_gauge_width > xfer_gauge_width)
		{
			// Fill newly progressed part of the gauge:
			SetAPen(xrp, drawInfo->dri_Pens[FILLPEN]);
			RectFill(xrp, 21+xfer_gauge_width, posY(9), 21+new_xfer_gauge_width, posY(9)+9);
			SetAPen(xrp, drawInfo->dri_Pens[TEXTPEN]);
			xfer_gauge_width = new_xfer_gauge_width;
		}
	}

    // xpru_blockcheck  -- block check type (e.g. "Checksum", "CRC-16", "CRC-32")
	if(ud&XPRU_BLOCKCHECK)
	{
		Move(xrp, posX(16), posY(6));
		TextFmt(xrp, "%-10s", updatestruct->xpru_blockcheck);
	}

    // xpru_errors      -- number of errors
	if(ud&XPRU_ERRORS)
	{
		Move(xrp, posX(16), posY(7));
		TextFmt(xrp, "%-10ld", updatestruct->xpru_errors);
	}

    // xpru_errormsg    -- an "error" message  (50 characters or less)
	if(ud&XPRU_ERRORMSG)
	{
		Move(xrp, posX(16), posY(8));
		TextFmt(xrp, "%-46.46s", updatestruct->xpru_errormsg);
	}

    // xpru_msg         -- a "generic" message (50 characters or less)
	if(ud&XPRU_MSG)
	{
		Move(xrp, posX(16), posY(8));
		TextFmt(xrp, "%-46.46s", updatestruct->xpru_msg);
	}


/* row 2 */

    // xpru_elapsedtime -- elapsed time from start of transfer (see xpru_expecttime)
	if(ud&XPRU_ELAPSEDTIME)
	{
		Move(xrp, posX(45), posY(4));
		TextFmt(xrp, "%-16s", updatestruct->xpru_elapsedtime);
	}

    // xpru_expecttime  -- expected transfer time (e.g. "5 min 20 sec", "00:05:30")
	if(ud&XPRU_EXPECTTIME)
	{
		Move(xrp, posX(45), posY(5));
		TextFmt(xrp, "%-16s", updatestruct->xpru_expecttime);
	}

    // xpru_datarate    -- rate of data transfer expressed in characters per second.
	if(ud&XPRU_DATARATE)
	{
		Move(xrp, posX(45), posY(7));
		TextFmt(xrp, "%-10ld", updatestruct->xpru_datarate);
	}
	return(0);
}

static long Checkwinmsg(struct Window *wwin)
{
	UWORD code;
	UWORD menuNumber, menuNum, itemNum;
	ULONG class;
	struct IntuiMessage *im;
	struct MenuItem *item;
	struct Gadget *gad;
	char close = FALSE;

	while(im=GT_GetIMsg(wwin->UserPort))
	{
		code = im->Code;
		class = im->Class;
		gad = (struct Gadget *)im->IAddress;
		GT_ReplyIMsg(im);
		switch(class)
		{
			case IDCMP_GADGETUP:
				switch(gad->GadgetID)
				{
					case 20:
						ScreenToBack(scr);
						break;
					case 6:
						shouldQuitApp = TRUE;
					case 1:
						return(-1);
					default:
						DisplayBeep(scr);
				}
				break;

			case IDCMP_CLOSEWINDOW:
				if(wwin != toolBarWin)
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
		CloseDisplay(TRUE);       // Close the complete DCTelnet Window
		OpenIcon();               // Iconify on the Workbench screen
	}
	return(0);
}

/**
 * @brief Checks if the user has requested to abort the file transfer.
 *
 * This callback function is called frequently during file transfers. It handles both windowed and
 * iconified states of the application.
 *
 * This ensures responsive abort handling without blocking the transfer process.
 *
 * @return LONG -1 if an abort is requested, 0 otherwise.
 */
long xpr_chkabort(void)
{
	// If the application is iconified on the Workbench
	if(isAppIconified)
	{
		// Process pending AppMessages from Workbench to detect a "shouldUniconifyify" request
		if(iconPort)
		{
			// Workbench sends AppMessage to the application's message port to notify it
			// https://wiki.amigaos.net/wiki/Workbench_Library#The_AppMessage_Structure
			register struct AppMessage *appmsg;
			while(appmsg = (struct AppMessage *)GetMsg(iconPort))
			{
				if(appmsg->am_NumArgs==0 && appmsg->am_ArgList==0)
					shouldUniconify = TRUE;  // User requested to restore the window
				ReplyMsg((struct Message *)appmsg);
			}
		}

		// Check & clear CTRL_F signal
		if(SetSignal(0L, SIGBREAKF_CTRL_F) & SIGBREAKF_CTRL_F) shouldUniconify = TRUE;

		// If shouldUniconifyify requested, restore the DCTelnet window:
		if(shouldUniconify)
		{
			CloseIcon();             // Close the icon on the Workbench screen
			OpenDisplay(TRUE);       // Reopen the complete DCTelnet Window
			XferWindow();            // Recreate the transfer window
			xfer_gauge_width = 0;
			shouldUniconify = FALSE;
		}

		return(0);   // Transfer continues
	}

	// Check messages from the transfer window
	if(Checkwinmsg(xferwin) == -1) return( -1L );

	// If not iconified, also check messages from the main window (win) and tool bar window (toolBarWin)
	if(!isAppIconified)
	{
		if(Checkwinmsg(win) == -1) return( -1L );
		if(!isAppIconified && toolBarWin)
		{
			return(Checkwinmsg(toolBarWin));
		}
	}

	return(0);
}

/* Query serial device */
long xpr_squery(void)
{
	fd_set rd;
	struct timeval timer;
	long oldsize;

	FD_ZERO(&rd);
	FD_SET(tcpSocket, &rd);

	timer.tv_sec = 0;
	timer.tv_usec = 1;

	if(WaitSelect(tcpSocket + 1, &rd, 0L, 0L, &timer, 0L) < 0) return(-1);

	if(FD_ISSET(tcpSocket, &rd))
	{
		oldsize = recv(tcpSocket, buf, sizeof buf, MSG_PEEK);
		if(oldsize == -1) return -1;
		if(oldsize < 1) return(0);

		return(instrip(buf, oldsize));
	}
	return(0);
}

static char ProtoStart(char *library, char *firstfile)
{
	XProtocolBase = OpenLibrary(library, 0);
	if(!XProtocolBase)
	{
		LocalFmt("\r\n›0;31mERROR: ›mCould not open transfer library: %s\r\n", library);
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

	if(isAppIconified) return(TRUE); else return(XferWindow());
}

static char XferWindow(void)
{
	WORD x, y, reuse, right;

	x = 64 * win->RPort->Font->tf_XSize;
	y = (10 * (win->RPort->Font->tf_YSize+1)) + 12;

	newWin.LeftEdge = (scr->Width - x) / 2;
	newWin.TopEdge = (scr->Height - y) / 2;
	newWin.Width = x;
	newWin.Height = y + winTop;
	newWin.IDCMPFlags = IDCMP_CLOSEWINDOW|IDCMP_MENUPICK;
	newWin.Flags = WFLG_NEWLOOKMENUS|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET;
	newWin.FirstGadget = 0;
	newWin.Title = "Transfer in Progress...";

	CheckDimensions(&newWin);
	xferwin = OpenWindow(&newWin);

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

	SetFont(xrp, ansiFont);

	SetAPen(xrp, drawInfo->dri_Pens[HIGHLIGHTTEXTPEN]);

	/*if (isRunningOnWB)
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

	SetAPen(xrp, drawInfo->dri_Pens[SHADOWPEN]);
	Move(xrp, 20, reuse);
	Draw(xrp, right, reuse);
	Move(xrp, 20, reuse);
	Draw(xrp, 20, posY(9)+10);

	SetAPen(xrp, drawInfo->dri_Pens[SHINEPEN]);
	Move(xrp, 20, posY(9)+10);
	Draw(xrp, right, posY(9)+10);
	Move(xrp, right, posY(9)+10);
	Draw(xrp, right, reuse);

	SetAPen(xrp, drawInfo->dri_Pens[TEXTPEN]);

	return(1);
}

void Upload(char *library)
{
	struct rtFileRequester *filereq;
	struct rtFileList *flist;

	if(isAppIconified) return;

	ff_escape_pending = FALSE;
	xfertype = XFER_UPLOAD;

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

	ff_escape_pending = FALSE;

	lck = Lock(prefs.downloadpath, SHARED_LOCK);
	if(lck)
	{
		old = CurrentDir(lck);

		xfertype = XFER_DOWNLOAD;

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
