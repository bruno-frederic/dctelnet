/* DCTelnet - Transfer Routines */

#define __USE_SYSBASE

#include <proto/exec.h>               // OpenLibrary(), GetMsg(), ReplyMsg(), SetSignal()...
#include <proto/dos.h>                // Open(), Close(), Read(), Write(), PutStr()...
#include <proto/intuition.h>          // OpenWindow(),CloseWindow(), OnMenu(), OffMenu()...
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
#include <proto/gadtools.h>           // GT_GetIMsg(), GT_ReplyIMsg()
#include <proto/asl.h>                // FileRequester
#include <workbench/workbench.h>      // struct AppMessage
#include <proto/Xpr.h>                // XProtocolSetup(), XProtocolSend(), XProtocolReceive(), ...
#include <proto/bsdsocket.h>          // WaitSelect(), recv(), IoctlSocket(), Errno()
#include <sys/ioctl.h>                // FIONBIO
#include "Xfer.h"
#include "DCTelnet.h"
#include "guis.h"
#include "requesters.h"

#define PATHLEN 256     // From third_party\Xpr\XprZmodem.h


static struct RastPort *xrp;
struct Library *XProtocolBase;
static struct Window *xferwin;
// An array of files to upload in batch when user select multiple file to upload:
static struct WBArg *uploadArray = NULL;
static LONG uploadArraySize = 0;
//static struct rtFileList *uplist, *upfirst;
static struct XPR_IO xio;

static UWORD xfer_gauge_width;
// Download : xfertype = 1        Upload : xfertype = 2
#define XFER_DOWNLOAD 1
#define XFER_UPLOAD   2
static UWORD xfertype;
// Set when a 0xFF escape byte is pending in the incoming transfer stream :
static UWORD ff_escape_pending;

long __SAVE_DS__ xpr_chkabort(void);     /* Check for abort */
static char XferWindow(void);


static int posX(WORD n)
{
    return((xrp->Font->tf_XSize*n)+4);
}

static int posY(WORD n)
{
    return(((xrp->Font->tf_YSize+1)*n)+3+winTop);
}


/**
 * @brief Remove Telnet escaped IAC bytes from a received data buffer.
 *
 * Telnet represents a literal IAC byte (0xFF) in the data stream by sending it twice (0xFF 0xFF).
 * This function collapses such escaped sequences back into a single 0xFF byte.
 *
 * The global @c ff_escape_pending flag is preserved across calls so that escape sequences split
 * across multiple TCP receive buffers are handled correctly.
 *
 * Raw connections (@c FLAG_RAW_CONNECTION) bypass this processing and the original buffer length is
 * returned unchanged.
 *
 * @param buff
 *        Buffer containing received data. The buffer is modified in place.
 *
 * @param length
 *        Number of valid bytes in @p buff.
 *
 * @return
 *        New buffer length after Telnet IAC unescaping.
 *
 * @note
 *        This function is primarily used during ZModem transfers where the
 *        Telnet parser is bypassed and only IAC escaping must be removed.
 *
 * @todo
 *        better buffer handling : each function call provoke an AllocMem.
 *        Eliminate the temporary tb buffer allocation on every call. The caller should
 *        provide a work buffer, or the unescaping should be performed in place.
 *
 * @todo
 *        Consider reusing the normal Telnet state machine during file transfers so that Telnet
 *        commands received mid-transfer are handled correctly while still unescaping IAC-IAC
 *        sequences.
 */
static long instrip(unsigned char *buff, long length)
{
    register long i = 0, j = 0;
    unsigned char *tb = NULL;
    if (prefs.flags & FLAG_RAW_CONNECTION)
        return length; // No stripping for raw connections

    tb = AllocMem(length+2, MEMF_PUBLIC);
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
    XProtocolBase = NULL;
    //ConWrite("", 1);
}

/*
This function returns information about a file given its name and the type of information requested.
typeofinfo = 1L : file size (bytes)
typeofinfo = 2L : file type (1L is binary ; 2L is text)

returns 0 on failure
*/
long __SAVE_DS__ __ASM__ xpr_finfo(__REG__(a0, char *filename),
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

/* This function writes a buffer with the given size to the socket/serial port.
   It returns 0L on success,
   non-zero on failure. */
#define FLAG_RAW_CONNECTION      (1 << 13)  // BIT 13 = Raw Connection (NO telnet negotiation data)
long __SAVE_DS__ __ASM__ xpr_swrite(__REG__(a0, char *buffer),
                        __REG__(d0, long size))
{
    long ret = -1;
    register ULONG i = 0, j = 0;
    UBYTE *tb = AllocMem(size+size, MEMF_PUBLIC);
    if(tb)
    {
        while(i < size)
        {
            // The byte 0xff (255) means that the next byte is a Telnet command. If you want to send
            // 0xff then you must send it twice to tell telnet that you don't intend to send a
            // command. This escaping is only required for Telnet connections and must not be
            // applied to raw TCP connections.
            if((unsigned char) buffer[i] == 255 && !(prefs.flags & FLAG_RAW_CONNECTION))
            {
                tb[j] = buffer[i];
                j++;
            }
            tb[j] = buffer[i];
            j++;
            i++;
        }
        if(TCPSend(tb, j) >= 0) ret = 0;
        FreeMem(tb, size+size);
    }
    return(ret);
}

/* Get char from socket/serial */
long __SAVE_DS__ __ASM__ xpr_sread(__REG__(a0, char *buffer),
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

            // TODO: check if this the responsability of the XPR library ?
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

/*    FD_ZERO(&rd);
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

/* Flush socket/serial input buffer

Used at the start of a transfer and when performing error recovery and resync when transferring
files.

Also used by Xem library.
*/
#define MAX_FLUSH_ITERATIONS 32
long __SAVE_DS__ xpr_sflush(void)
{
    LONG len;
    int i = 0;

    // Set temporarly socket to nonblocking to quickly flush.
    LONG mode = 1;  // The argument must be (long *)
    IoctlSocket(tcpSocket, FIONBIO, &mode);

    do
    {
        /*
         If no messages are available at the socket, the receive call waits for a message to arrive,
         unless the socket is nonblocking (see IoctlSocket()) in which case the value -1 is returned
         and the external variable errno set to EAGAIN.
        */
        // TODO: Consider calling Receive() to continue processing pending Telnet IAC sequences.
        len = recv(tcpSocket, recvBuffer, sizeof(recvBuffer), 0);

        i++;
    } while (len > 0 && i < MAX_FLUSH_ITERATIONS);

    // Set socket back to standard blocking mode:
    mode = 0;
    IoctlSocket(tcpSocket, FIONBIO, &mode);

    #ifdef _DEBUG
        // Must remain at the end since the execution time of this call
        // can influence timing-sensitive behavior in preceding code
        PutStr("›32m<-- xpr_sflush()›m\n");
    #endif

    return 0; // The returned long value seems to be unused
}

/* Find first file name to upload */
long __SAVE_DS__ __ASM__ xpr_ffirst(__REG__(a0, char *buffer),
                        __REG__(a1, char *pattern))
{
    #ifdef _DEBUG
        Printf("›32m--> xpr_ffirst(buffer = ..., pattern = %s)›m\n", pattern);
    #endif

    if (uploadArray == NULL || uploadArraySize < 1)
        return 0L;

    strlcpy(buffer, prefs.uploadpath, PATHLEN);
    AddPart(buffer, uploadArray[0].wa_Name, PATHLEN);

    // Return index number of the next element:
    return 1L;
}

/* Find next file name to upload */
long __SAVE_DS__ __ASM__ xpr_fnext(__REG__(d0, long oldstate),
                __REG__(a0, char *buffer),
                __REG__(a1, char *pattern))
{
    #ifdef _DEBUG
        Printf("›32m--> xpr_fnext(oldstate = %ld, buffer = ..., pattern = %s)›m\n",
               oldstate, pattern);
    #endif

    if (oldstate < uploadArraySize && uploadArray != NULL)
    {
        strlcpy(buffer, prefs.uploadpath, PATHLEN);
        AddPart(buffer, uploadArray[oldstate].wa_Name, PATHLEN);

        // Return index number of the next element:
        return oldstate+1;
    }
    else
    {
        return 0L; // No more file
    }
}

/* Get string interactively */
long __SAVE_DS__ __ASM__ xpr_gets(__REG__(a0, char *prompt),
                      __REG__(a1, char *buffer))
{
    /* The first argument is a pointer to a string containing a prompt, to be displayed by the
    communications program in any manner it sees fit. The second argument should be a pointer to a
    buffer to receive the user's response. It should have a size of at least 256 bytes.
    The function returns 0L on failure or user cancellation, non-zero on success.
    */
    #ifdef _DEBUG
        SimpleReq("TODO : xpr_gets() is not implemented yet.");
    #endif

    return(0);
}

/*
The following xpr_fopen(), xpr_fclose() xpr_fread(), xpr_fwrite(), xpr_fseek(), xpr_unlink()
call-back function works in most respects identically to the stdio function fopen(),etc...
Enables external protocols to manipulate files via the communication program.
*/
long __SAVE_DS__ __ASM__ xpr_fopen(__REG__(a0, char *filename),
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

long __SAVE_DS__ __ASM__ xpr_fclose(__REG__(a0, long filepointer))
{
    if(filepointer) Close(filepointer);
    return(0);
}

long __SAVE_DS__ __ASM__ xpr_fread(__REG__(a0, char *buffer),
                       __REG__(d0, long size),
                       __REG__(d1, long count),
                       __REG__(a1, long fileptr))
{
    if(size==0 || count==0) return(0);
    return(Read(fileptr,buffer,size*count));
}

long __SAVE_DS__ __ASM__ xpr_fwrite(__REG__(a0, char *buffer),
                        __REG__(d0, long size),
                        __REG__(d1, long count),
                        __REG__(a1, long fileptr))
{
    if(size==0 || count==0) return(0);
    return(Write(fileptr,buffer,size*count));
}

long __SAVE_DS__ __ASM__ xpr_fseek(__REG__(a0, long fileptr),
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
long __SAVE_DS__ __ASM__ xpr_unlink(__REG__(a0, char *filename))
{
    return(DeleteFile(filename));
}

/*
    This call-back function is intended to communicate a variety of values and strings from the
    external protocol to the communications program for display. Hence, the display format itself
    (requester, text-I/O) is left to the implementer of the communications program.

    DCTelnet updates "Transfer in Progress" window with the values received in the XPR_UPDATE
    structure passed to this function.
*/
long __SAVE_DS__ __ASM__ xpr_update(__REG__(a0,
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
        // Update field "Bytes xfer'd"
        Move(xrp, posX(16), posY(5));
        TextFmt(xrp, "%-10ld", updatestruct->xpru_bytes);

        if(updatestruct->xpru_filesize > 0)
        {
            ULONG bytes = updatestruct->xpru_bytes;
            ULONG size  = updatestruct->xpru_filesize;

            // Update field "% xfer'd"
            Move(xrp, posX(45), posY(6));
            TextFmt(xrp, "%ld%%  ", (bytes*100)/size);


            if(size > 4096)
            {
                bytes >>= 12;   // Divide by 4096 to avoid overflow in the multiplication below.
                size  >>= 12;   // The progress bar should work up to 36 GB files with this.
            }
            new_xfer_gauge_width =  (bytes * (xferwin->Width - 42)) / size;

            #ifdef _DEBUG
                 if(new_xfer_gauge_width < xfer_gauge_width)
                 {
                    TextFmt(xrp, "%-10ld", new_xfer_gauge_width);
                    SimpleReq("FIXME : new_xfer_gauge_width < xfer_gauge_width, should not happen");
                 }
            #endif
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


void SendZmodemCancelSequence(void)
{
    static const UBYTE ZMODEM_CANCEL_SEQUENCE[] =
        { 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18 };

    #ifdef _DEBUG
        PutStr("--> SendZmodemCancelSequence()\n");
    #endif

    TCPSend(ZMODEM_CANCEL_SEQUENCE, sizeof(ZMODEM_CANCEL_SEQUENCE));
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
                    case GADGET_SCREEN_TO_BACK:
                        ScreenToBack(scr);
                        break;
                    case BUTTON_QUIT:
                        shouldQuitApp = TRUE;
                    case BUTTON_DISCONNECT:
                        return(-1);
                    default:
                        DisplayBeep(scr);
                }
                break;

            case IDCMP_CLOSEWINDOW:
                if(wwin != toolBarWin)
                {
                    SendZmodemCancelSequence();
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
long __SAVE_DS__ xpr_chkabort(void)
{
    // If the application is iconified on the Workbench
    if(isAppIconified)
    {
        // Process pending AppMessages from Workbench to detect a "shouldUniconifyify" request
        if(iconPort)
        {
            // Workbench sends AppMessage to the application's message port to notify it
            // https://wiki.amigaos.net/wiki/Workbench_Library#The_AppMessage_Structure
            register struct Message *msg;
            while(msg = GetMsg(iconPort))
            {
                if (  ((struct AppMessage *)msg)->am_NumArgs == 0
                   && ((struct AppMessage *)msg)->am_ArgList == NULL)
                    shouldUniconify = TRUE;  // User requested to restore the window
                ReplyMsg(msg);
            }
        }

        // Check & clear CTRL_F signal
        if(SetSignal(0L, SIGBREAKF_CTRL_F) & SIGBREAKF_CTRL_F) shouldUniconify = TRUE;

        // If shouldUniconifyify requested, restore the DCTelnet window:
        if(shouldUniconify)
        {
            CloseIcon();             // Close the icon on the Workbench screen
            OpenDisplay();           // Reopen the complete DCTelnet Window
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

/**
 * @brief Query the socket or serial device for available incoming data.
 *
 * This function calls recv() with MSG_PEEK option to inspect the received bytes without consuming
 * them.
 * The raw data is passed to instrip(), which removes the Telnet IAC sequences before returning the
 * cleaned payload length.
 *
 * @return Negative value on error,
 *         0 if no data is available,
 *         or a positive value representing the cleaned data length after Telnet IAC removal.
 */
long __SAVE_DS__ xpr_squery(void)
{
    fd_set rd;
    struct timeval timer;
    long oldsize;

    #ifdef _DEBUG
        SimpleReq("xpr_squery() called!!! TODO: Determine when? why?");
    #endif

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
    xio.xpr_options   = xpr_options;
    xio.xpr_unlink    = xpr_unlink;
    xio.xpr_squery    = xpr_squery;
    xio.xpr_extension = 1L;

    xio.xpr_filename = prefs.xferinit;//"TC,OR,B32,FO,AN,DN,KY,SN,RN";
    XProtocolSetup(&xio);
    xio.xpr_filename = firstfile;

    if(isAppIconified) return(TRUE); else return(XferWindow());
}

/* TODO MAKE A CLEAN FUNCTION NOT REDUNDANT WITH ProtoStart() */
void XferOptions(char *library)
{
    XProtocolBase = OpenLibrary(library, 0);
    if(!XProtocolBase)
    {
        LocalFmt("\r\n›0;31mERROR: ›mCould not open transfer library: %s\r\n", library);
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
    xio.xpr_options   = xpr_options;
    xio.xpr_unlink    = xpr_unlink;
    xio.xpr_squery    = xpr_squery;
    xio.xpr_extension = 1L;

    xio.xpr_filename = NULL; // This provoke the opening of XPR Options dialog box
    XProtocolSetup(&xio);
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

/*    xferwin = OpenWindowTags(NULL,
        WA_Title,        "Transfer in Progress...",
        WA_Left,        (scr->Width - x) / 2,
        WA_Top,            (scr->Height - y) / 2,
        WA_Width,        x,
        WA_InnerHeight,        y,
        WA_CustomScreen,    scr,
        WA_IDCMP,        IDCMP_CLOSEWINDOW|IDCMP_MENUPICK,
        WA_Flags,        WFLG_NEWLOOKMENUS|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET,//|WFLG_RMBTRAP,
        TAG_END);*/

    if(!xferwin)
    {
        CloseLibrary(XProtocolBase);
        XProtocolBase = NULL;
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
    struct FileRequester *fr;

    if(isAppIconified) return;

    ff_escape_pending = FALSE;
    xfertype = XFER_UPLOAD;

    fr = (struct FileRequester *) AllocAslRequestTags(ASL_FileRequest,    // type of requester
                                        ASL_Window, isRunningOnWB ? NULL : win,
                                        ASL_Hail,  "Select one or more files",

                                        // Supply initial values for requester:
                                        ASL_Dir,     prefs.uploadpath,
                                        ASL_Pattern, "#?",

                                        ASL_FuncFlags, FILF_PATGAD  // Enable pattern match gadget
                                                       | FILF_MULTISELECT,
                                        TAG_DONE);

    if (fr == NULL)
    {
        InfoReq(isRunningOnWB ? NULL : win, "AllocAslRequestTags() => NULL (failed)");
        goto clean_and_return;
    }

    if (! AslRequest(fr, NULL))
    {
        // The file dialog was closed by the user with no file selected
        SendZmodemCancelSequence();
        goto clean_and_return;
    }

    // Save the directory in which files where selected to be uploaded:
    strlcpy(prefs.uploadpath, fr->rf_Dir, sizeof(prefs.uploadpath));
    SavePrefs();

    strlcpy(buf, fr->rf_Dir,  sizeof(buf));
    AddPart(buf, fr->rf_File, sizeof(buf));

    uploadArray     = fr->rf_ArgList;
    uploadArraySize = fr->rf_NumArgs;


    #ifdef _DEBUG
        PutStr("›32m--> ProtoStart()›m\n");
    #endif
    if(ProtoStart(library, buf))
    {
        if (XProtocolSend(&xio) != XPRS_SUCCESS)
        {
            #ifdef _DEBUG
                PutStr("›32m<-- XProtocolSend() => FAIL ›m\n");
            #endif
            SendZmodemCancelSequence();

            // Pause to let the user read the Xfer window error
            // and to allow the cancel sequence to be transmitted to the server
            Delay(3*TICKS_PER_SECOND);

            xpr_sflush();   // remove garbage received which prevent them to be displayed.
        }

        #ifdef _DEBUG
            PutStr("›32m--> ProtoClean()›m\n");
        #endif

        ProtoClean();
    }


clean_and_return:
    if (fr != NULL)
    {
        uploadArray     = NULL;
        uploadArraySize = 0L;
        FreeAslRequest(fr);
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
            if(XProtocolReceive(&xio) != XPRS_SUCCESS)
            {
                SendZmodemCancelSequence();

                // Pause to let the user read the Xfer window error
                // and to allow the cancel sequence to be transmitted to the server
                Delay(3*TICKS_PER_SECOND);

                xpr_sflush();   // remove garbage received which prevent them to be displayed.
            }

            ProtoClean();
        }
        CurrentDir(old);
        UnLock(lck);
    } else {
        SendZmodemCancelSequence();
        SimpleReq("Download path does not exist.");
    }
}


 /*************************************************************************************************
 *
 * This section contains the implementation of xpr_options() implementation and helper functions :
 * CreateOptionGadgets() and GetOptionMode().
 *
 * Inspired from XEm_SampleCode.c (Xem 2.0 source code)
 *
 **************************************************************************************************/

/**
 * @brief Determines the boolean value of an option based on its string value.
 *
 * Checks the value of the given xpr_option and returns TRUE if it represents an enabled/true state
 * ("ON", "TRUE", "YES", etc.), or FALSE otherwise.
 *
 * @param Option Pointer to the xpr_option structure to evaluate.
 * @return BOOL TRUE if the option is enabled, FALSE if disabled.
 */
static BOOL GetOptionMode(struct xpr_option *Option)
{
    if(!stricmp(Option->xpro_value, "OFF"))
        return(0);

    if(!stricmp(Option->xpro_value, "FALSE"))
        return(0);

    if(!stricmp(Option->xpro_value, "F"))
        return(0);

    if(!stricmp(Option->xpro_value, "NO"))
        return(0);

    if(!stricmp(Option->xpro_value, "N"))
        return(0);


    if(!stricmp(Option->xpro_value, "ON"))
        return(1);

    if(!stricmp(Option->xpro_value, "TRUE"))
        return(1);

    if(!stricmp(Option->xpro_value, "T"))
        return(1);

    if(!stricmp(Option->xpro_value, "YES"))
        return(1);

    if(!stricmp(Option->xpro_value, "Y"))
        return(1);

    return(0);
}

/*
*
*/
/**
 * @brief Create a set of Gadtools gadgets for xpr_options()
 *
 * This function builds a context of gadgets based on the xpr_option array provided by the Xpr/Xem
 * library.
 * It computes layout metrics for either a single or two-column presentation, creates appropriate
 * gadget types (checkbox, integer, string, text, button) and appends them to the provided gadget
 * list.
 *
 * @param[out] count   Receives the number of gadgets created.
 * @param[out] width   Receives the computed window width required.
 * @param[out] height  Receives the computed window height required.
 * @param[in]  numopts Number of options in opts[].
 * @param[in]  opts    Array of pointers to xpr_option describing each option.
 * @param[out] gadgetarray Array to receive pointers to created gadgets (indexed by option).
 * @param[in,out] gadgetlist Pointer to gadget context/list to append to.
 * @param[in]  topedge Starting top edge Y coordinate for placement.
 *
 * @return Pointer to the last created gadget on success, or NULL on failure.
 */
static struct Gadget *CreateOptionGadgets(LONG *count, UWORD *width, UWORD *height, LONG numopts, struct xpr_option *opts[], struct Gadget *gadgetarray[], struct Gadget **gadgetlist, UWORD topedge)
{
    //IMPORT APTR VisualInfo;   -> visualInfos dans DCTelnet
    //IMPORT struct TextAttr DefaultAttr; -> fontAttr dans DCTelnet

    struct Gadget *gadget;
    struct NewGadget newgad = {0};
    LONG i;
    UWORD leftedge;
    UWORD command_len, leftstring_len, rightstring_len, header_len;
    UWORD leftgadget_width, rightgadget_width;
    BOOL split, right;

    if(numopts == 0)        /* AETSCH..!! */
        return(NULL);

    *count = 0;

    //memset(&newgad, 0, sizeof(struct NewGadget));
    if(gadget = CreateContext(gadgetlist))
    {
        newgad.ng_Height        = 12;
        newgad.ng_TextAttr    =   scr->Font;
        newgad.ng_VisualInfo    = visualInfos;
        newgad.ng_Flags        = NG_HIGHLABEL;

    /* is there a 2 column display? */
        split = (numopts > 11);

        header_len = 0;
        command_len = 0;
        leftstring_len = 0;
        rightstring_len = 0;

        leftgadget_width = 20;    /* BOOL-Gadgets first! */
        rightgadget_width = 20;

        right = FALSE;    /* we start at the left side.. */
        for(i=0; i<numopts; i++)
        {
            UWORD len = (strlen(opts[i]->xpro_description) + 1) << 3;

            switch(opts[i]->xpro_type)
            {
                case XPRO_HEADER:
                    if(header_len < len)
                        header_len = len;

                    right = TRUE;        /* we need the right column, too */
                break;

                case XPRO_COMMAND:
                    if(command_len < len)
                        command_len = len + 50;

                    right = TRUE;        /* we need the right column, too */
                break;

                case XPRO_STRING:
                case XPRO_LONG:
                case XPRO_COMMPAR:
                    if(split  &&  right)
                        rightgadget_width = 68;
                    else
                        leftgadget_width = 68;

                /* break thru */

                case XPRO_BOOLEAN:
                    if(split  &&  right)
                    {
                        if(rightstring_len < len)
                            rightstring_len = len;
                    }
                    else
                    {
                        if(leftstring_len < len)
                            leftstring_len = len;
                    }
                break;

            }

            right = !right;    /* swap it.. */
        }

#define LEFTEDGE 15
#define ROW 14


        right = FALSE;    /* we start at the left side of life  ;-) */
        for(i=0; i<numopts; i++)
        {
            leftedge = LEFTEDGE + leftstring_len;
            newgad.ng_Width = leftgadget_width;

            if(split  &&  right)
            {
                leftedge += leftgadget_width;
                leftedge += (LEFTEDGE + rightstring_len);
                newgad.ng_Width = rightgadget_width;
            }
            else
                topedge += ROW;


            newgad.ng_GadgetText    = opts[i]->xpro_description;
            newgad.ng_GadgetID    = i;
            newgad.ng_LeftEdge    = leftedge;
            newgad.ng_TopEdge        = topedge;

            switch(opts[i]->xpro_type)
            {
                case XPRO_BOOLEAN:
                    gadgetarray[i] = gadget = CreateGadget(CHECKBOX_KIND,gadget,&newgad,
                        GTCB_Checked,    GetOptionMode(opts[i]),
                    TAG_DONE);
                break;

                case XPRO_LONG:
                    gadgetarray[i] = gadget = CreateGadget(INTEGER_KIND,gadget,&newgad,
                        GTIN_Number,    atol(opts[i]->xpro_value),
                    TAG_DONE);
                break;

                case XPRO_STRING:
                    gadgetarray[i] = gadget = CreateGadget(STRING_KIND,gadget,&newgad,
                        GTST_String,    opts[i]->xpro_value,
                        GTST_MaxChars,    opts[i]->xpro_length,
                    TAG_DONE);
                break;

                case XPRO_COMMPAR:
                    newgad.ng_Width = LEFTEDGE + command_len - leftedge;

                    gadgetarray[i] = gadget = CreateGadget(STRING_KIND,gadget,&newgad,
                        GTST_String,    opts[i]->xpro_value,
                        GTST_MaxChars,    opts[i]->xpro_length,
                    TAG_DONE);
                break;

                case XPRO_HEADER:
                    if(split  &&  right)
                        topedge += ROW;

                    newgad.ng_GadgetText    = NULL;
                    newgad.ng_Width        = header_len;
                    newgad.ng_LeftEdge    = LEFTEDGE;
                    newgad.ng_TopEdge        = topedge;

                    right = TRUE;    /* we need the right column, too */

                    gadgetarray[i] = gadget = CreateGadget(TEXT_KIND,gadget,&newgad,
                        GTTX_Text,    opts[i]->xpro_description,
                    TAG_DONE);
                break;

                case XPRO_COMMAND:
                    leftedge = LEFTEDGE;

                    if(split  &&  right)
                    {
                        leftedge += command_len;
                        leftedge <<= 1;
                    }
                    newgad.ng_Width     = command_len;
                    newgad.ng_LeftEdge = leftedge;

                    gadgetarray[i] = gadget = CreateGadget(BUTTON_KIND,gadget,&newgad,
                    TAG_DONE);
                break;

                default:
                    ;
                break;
            }

            right = !right;    /* swap it.. */
        }


        *width = leftstring_len + leftgadget_width;
        if(split)
            *width += (LEFTEDGE + rightstring_len + rightgadget_width);

        if(split)
        {
            UWORD cw;

            cw = (command_len << 1) + LEFTEDGE;
            if(*width < cw)
                *width = cw;
        }
        else
        {
            if(*width < command_len)
                *width = command_len;
        }

        if(*width < header_len)
            *width = header_len;

        *width += (LEFTEDGE << 1);    /* left + right distance.. */

        newgad.ng_GadgetText    = "Continue";
        newgad.ng_Width        = 88;
        newgad.ng_LeftEdge    = (*width >> 1) - 44;
        newgad.ng_TopEdge        = gadget->TopEdge + gadget->Height + 9;
        newgad.ng_GadgetID    = numopts;
        newgad.ng_Flags        = NG_HIGHLABEL | PLACETEXT_IN;

        gadget = CreateGadget(BUTTON_KIND,gadget,&newgad,
            TAG_DONE);

        *height= gadget->TopEdge + gadget->Height + 5;
        *count = i;
    }

    return(gadget);
}

/**
 * @brief Display and manage the XPR and Xem libraries options dialog.
 *
 * Open a window containing interactive option gadgets, allows the user to modify option values,
 * processes input events, updates the options state, and returns a flag indicating which options
 * were modified.
 *
 * This function was `coded' for xpr usage, but it works well with xem, too
 * Called by XProtocolSetup() from XPR lib and XEmulatorOptions() from XEM lib.
 *
 * @param numopts Number of options.
 * @param opts Pointer to an array of option structures.
 * @return Bitmask of modified option flags.
  */
long __SAVE_DS__ __ASM__ xpr_options(__REG__(d0, LONG numopts), __REG__(a0, struct xpr_option **opts))
{
    struct Gadget    *gadgetlist;
    struct Gadget    *gadgetarray[33];
    struct Window    *window;
    ULONG flags = 0;
    UWORD left, top, width, height;
    LONG    i, count;

    if(CreateOptionGadgets(&count,&width,&height,numopts,opts,&gadgetarray[0],&gadgetlist,1))
    {
        // FIXME : implement GetWindowPosition()
        //GetWindowPosition(&left, &top, width, height);
        left = 50; top = 50;

        if(window = OpenWindowTags(NULL,
            WA_Width,            width,
            WA_Height,            height,
            WA_Left,            left,
            WA_Top,                top,
            WA_Activate,        TRUE,
            WA_DragBar,            TRUE,
            WA_DepthGadget,    TRUE,
            WA_RMBTrap,            TRUE,
            WA_CustomScreen,    scr,
            WA_IDCMP,            IDCMP_GADGETUP | CHECKBOXIDCMP | IDCMP_RAWKEY,
            WA_Title,            "Options",
        TAG_DONE))
        {
            struct IntuiMessage    *imsg;
            struct Gadget        *gadget;
            ULONG class, code;
            BOOL quit;

            AddGList(window,gadgetlist,(UWORD)-1,(UWORD)-1,NULL);
            RefreshGList(gadgetlist,window,NULL,(UWORD)-1);
            GT_RefreshWindow(window,NULL);

            for(quit=FALSE; quit==FALSE; )
            {
                WaitPort(window->UserPort);

                while(imsg = GT_GetIMsg(window->UserPort))
                {
                    class    = imsg->Class;
                    code    = imsg->Code;
                    gadget    = (struct Gadget *)imsg->IAddress;

                    GT_ReplyIMsg(imsg);


                    if(class == IDCMP_RAWKEY)
                    {
                        //FIXME: implement CheckAbort()
                        //if(CheckAbort(code))
                        //    class = IDCMP_CLOSEWINDOW;
                    }

                    if(class == IDCMP_GADGETUP)
                    {
                        UWORD id;

                        if((id = gadget->GadgetID) >= numopts)
                            class = IDCMP_CLOSEWINDOW;

                        if(opts[id]->xpro_type == XPRO_COMMAND)
                        {
                            flags |= (1 << id);
                            quit = TRUE;
                        }
                        else
                        {
                            if(opts[id]->xpro_type == XPRO_COMMPAR)
                            {
                                if(strcmp(opts[id]->xpro_value, ((struct StringInfo *)gadget->SpecialInfo)->Buffer))
                                {
                                    flags |= (1 << id);
                                    strcpy(opts[id]->xpro_value,((struct StringInfo *)gadget->SpecialInfo)->Buffer);
                                }
                                quit = TRUE;
                            }
                        }
                    }


                    if(class == IDCMP_CLOSEWINDOW)
                    {
                        for(i=0 ; i<numopts ; i++)
                        {
                            switch(opts[i]->xpro_type)
                            {
                                case XPRO_BOOLEAN:
                                    if(((gadgetarray[i]->Flags & SELECTED)  &&  !GetOptionMode(opts[i])) || (!(gadgetarray[i]->Flags & SELECTED)  &&  GetOptionMode(opts[i])))
                                    {
                                        flags |= (1 << i);

                                        if(gadgetarray[i]->Flags & SELECTED)
                                            strcpy(opts[i]->xpro_value, "yes");
                                        else
                                            strcpy(opts[i]->xpro_value, "no");
                                    }
                                break;

                                case XPRO_LONG:
                                case XPRO_STRING:
                                    if(strcmp(opts[i]->xpro_value, ((struct StringInfo *)gadgetarray[i]->SpecialInfo)->Buffer))
                                    {
                                        flags |= (1 << i);
                                        strcpy(opts[i]->xpro_value,((struct StringInfo *)gadgetarray[i]->SpecialInfo)->Buffer);
                                    }
                                break;
                            }
                        }
                        quit = TRUE;
                    }
                }
            }

            RemoveGList(window, gadgetlist, (UWORD)-1);
            CloseWindow(window);
        }
        FreeGadgets(gadgetlist);
    }

    return (long) flags;
}

/************************************************************************/

