/* ====================================================================== */
/* ============================= DC TELNET ============================== */
/* ====================================================================== */


#define DCTELNET_VERSION "2.0-dev"
const char __ver[] = "$VER: DCTelnet " DCTELNET_VERSION " " __AMIGADATE__;

#ifndef BUILD_HASH
#define BUILD_HASH unknown
#endif

// Stringify macro :
#define STR_(x) #x
#define STR(x) STR_(x)

static char MainWindowTitle[] =
#ifdef _DEBUG
  "DCTelnet " DCTELNET_VERSION " (" STR(BUILD_HASH) ") " __AMIGADATE__ " - A classic Amiga Telnet/BBS client";
#else
  "DCTelnet " DCTELNET_VERSION " " __AMIGADATE__ " - A classic Amiga Telnet/BBS client";
#endif

#define __USE_SYSBASE

// For telnet debugging purpose:
#ifdef _DEBUG
#define TELCMDS
#define TELOPTS
#endif

#ifdef __VBCC__
    #pragma dontwarn 306
#endif
#include <proto/exec.h>               // OpenLibrary(), GetMsg(), ReplyMsg(), AllocMem()...
#include <proto/dos.h>                // Open(), Close(), Read(), Write(), PutStr()...
#include <proto/intuition.h>          // OpenWindow(),CloseWindow(), OnMenu(), OffMenu()...
#include <graphics/videocontrol.h>    // VTAG_BORDERBLANK_SET
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
#include <proto/layers.h>             // InstallLayerInfoHook()
// cybergraphics: vbcc ships the inline calls but not clib/cybergraphics_protos.h
extern struct Library *CyberGfxBase;
#include <inline/cybergraphics_protos.h> // ReadPixelArray(), WritePixelArray()
#include <proto/gadtools.h>           // GT_GetIMsg(), GT_ReplyIMsg()...
#include <proto/diskfont.h>           // OpenDiskFont()
#include <proto/utility.h>            // GetTagData()
#include <proto/icon.h>               // GetDiskObjectNew(), FreeDiskObject()
#include <proto/wb.h>                 // AddAppIconA(), RemoveAppIcon()
#include <proto/keymap.h>             // MapRawKey(), RAWKEY_UP, RAWKEY_DOWN, RAWKEY_F1...
#include <devices/conunit.h>          // CONU_SNIPMAP, CONU_CHARMAP, CONFLAG_DEFAULT
#include <libraries/reqtools.h>       // struct rtFileList, RT_FILEREQ, RT_Window
#include <proto/reqtools.h>           // rtAllocRequestA() rtScreenModeRequest() rtPaletteRequestA()
#include <proto/socket.h>             // send(), <CloseSocket>()
#include <arpa/telnet.h>
#include "petscii_dispatch.h"
#include "petscii_keymap.h"
#include "site_prefs.h"
#include "screenfont.h"
#include "progdir.h"
#include "dsr.h"
#include "shipped.h"
#ifdef __VBCC__
    #pragma popwarn
#endif
#include "DCTelnet.h"
#include "guis.h"
#include "connect.h"
#include "Xfer.h"
#include "Xem_wrapper.h"
#include "requesters.h"
#include "utils.h"
#include "prefs.h"
#include "prefs_file.h"
#include "palette.h"

#define ESC_CHAR '\x1B'  // ASCII Escape character (decimal 27, octal 033)
#define ESC_STR  "\x1B"  // ASCII Escape character (decimal 27, octal 033) as a C string
#define CSI_CHAR '\x9B'  // Amiga console CSI=Control Sequence Introducer ('›', decimal 155,
                         // octal 233) cf. Amiga ROM Kernel Reference Manual v2.04 - Devices (1991),
                         // section "Control Sequences for Window Output"
#define DEL_CHAR '\x7F'  // ASCII DEL character (decimal 127, octal 177)


/**
 * @brief Definition of the application's main menu.
 *
 * Each entry stores its corresponding MenuItemID value in nm_UserData. Menu entries may be added,
 * removed, or reordered without affecting event handling, provided their identifiers remain unique.
 *
 * WARNING: MenuItemID values MUST NOT be used as indexes into mainMenuDesc[]. An identifier is a
 * logical ID, not an array index. Its numeric value does not correspond to the entry's position in
 * the array, and this position may change whenever menu entries are inserted, removed, reordered.
 */
#ifdef __VBCC__
#pragma dontwarn 81 // warning: only 0 should be cast to pointer
#endif
static struct NewMenu mainMenuDesc[] =
{
    { NM_TITLE, "DC Telnet",  0, 0, 0, (APTR)MENU_DCTELNET},
    {    NM_ITEM, "About",                          "A",             0,               0, (APTR)MENU_ABOUT},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Scrollback window",              "X",             0,               0, (APTR)MENU_SCROLLBACK_WIN},
    {    NM_ITEM, "Iconify",                        "&",             0,               0, (APTR)MENU_ICONIFY},
    {    NM_ITEM, "Display Speed Test",             "Y",             0,               0, (APTR)MENU_DISPLAY_SPEED_TEST},
    {    NM_ITEM, "Finger",                         "@",             0,               0, (APTR)MENU_FINGER},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Reset Screen",                   "C",             0,               0, (APTR)MENU_RESET_SCREEN},
    {    NM_ITEM, "Quit",                           "Q",             0,               0, (APTR)MENU_QUIT},

    { NM_TITLE, "Transfer",  0 , 0, 0, (APTR)MENU_TRANSFER},
    {    NM_ITEM, "Upload",                         "U",             0,               0, (APTR)MENU_UPLOAD},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Download",                       "D",             0,               0, (APTR)MENU_DOWNLOAD},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "ASCII Send",                     "%",             0,               0, (APTR)MENU_ASCII_SEND},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Download Path..",                "O",             0,               0, (APTR)MENU_DOWNLOAD_PATH},
    {    NM_ITEM, "Transfer Protocol..",            "T",             0,               0, (APTR)MENU_TRANSFER_PROTOCOL},
    {    NM_ITEM, "Protocol Options..",             "*",             0,               0, (APTR)MENU_PROTOCOL_OPTIONS},

    { NM_TITLE, "Connection",  0 , 0, 0, (APTR)MENU_CONNECTION},
    {    NM_ITEM, "Connect",                        "M",             0,               0, (APTR)MENU_CONNECT},
    {    NM_ITEM, "Connect (New instance)",         "G",             0,               0, (APTR)MENU_CONNECT_NEW_INSTANCE},
    {    NM_ITEM, "Disconnect",                     "H",             0,               0, (APTR)MENU_DISCONNECT},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Address Book",                   "B",             0,               0, (APTR)MENU_ADDRESS_BOOK},
    {    NM_ITEM, "Save Settings to Address Book Entry", 0,          0,               0, (APTR)MENU_SAVE_ENTRY_SETTINGS},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Information",                    "^",             0,               0, (APTR)MENU_INFORMATION},

    { NM_TITLE, "Terminal",  0, 0, 0, (APTR)MENU_TERMINAL},
    {    NM_ITEM, "Telnet terminal type..",         "9",             0,               0, (APTR)MENU_TELNET_TERM_TYPE},
    {    NM_ITEM, "Raw Connection",                 "7", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_RAW_CONNECTION},
//  {    NM_ITEM, "Convert incoming LF to CRLF",    "L", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_INCOMING_LF_TO_CRLF},
    {    NM_ITEM, "PETSCII Mode",                    0 , HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_PETSCII_MODE},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Local Echo",                     "6", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_LOCAL_ECHO},
    {    NM_ITEM, "Swap BackSpace & Del keys",      "/", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_BACKSPACE_DEL_SWAP},
    {    NM_ITEM, "Return key send CR+LF",          "5", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_RETURN_SENDING_CRLF},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Packet Window",                  "2", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_PACKET_WINDOW},
    {    NM_ITEM, "Scrollback History",              "E", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_SCROLLBACK},
    {    NM_ITEM, "ScrollBack Lines..",             "0",             0,               0, (APTR)MENU_SCROLLBACK_LINES},
    {    NM_ITEM, "Function Keys..",                "K",             0,               0, (APTR)MENU_FUNCTION_KEYS},

    { NM_TITLE, "Display",                          0 ,             0,               0, (APTR)MENU_DISPLAY},
    {    NM_ITEM, "Renderer",                        0 ,             0,               0, (APTR)MENU_RENDERER},
    {       NM_SUB, "Built-in (experimental)",      "1", HIGHCOMP|CHECKIT, ~(1L<<0), (APTR)MENU_BUILTIN_RENDERER},
    {       NM_SUB, "AmigaOS console.device",        0 , HIGHCOMP|CHECKIT, ~(1L<<1), (APTR)MENU_CONSOLE_DEVICE},
    {       NM_SUB, "External XEM Library",         "3", HIGHCOMP|CHECKIT, ~(1L<<2), (APTR)MENU_XEM_LIBRARY},
    {       NM_SUB, "Zed's ibmcon.device",           0 , HIGHCOMP|CHECKIT, ~(1L<<3), (APTR)MENU_IBMCON_DEVICE},
    {       NM_ITEM, NM_BARLABEL,                    0 ,             0,               0, (APTR)MENU_BAR},
    {       NM_SUB, "Fast Scroll (ibmcon.device)",  "8", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_FAST_SCROLL},
    {       NM_ITEM, "XEM Library path..",          "#",             0,               0, (APTR)MENU_XEM_LIBRARY_PATH},
    {       NM_ITEM, "XEM Lib Options..",           "+",             0,               0, (APTR)MENU_XEM_LIB_OPTIONS},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Screen Mode..",                  "S",             0,               0, (APTR)MENU_SCREEN_MODE},
    {    NM_ITEM, "Screen Font..",                  "F",             0,               0, (APTR)MENU_SCREEN_FONT},
    {    NM_ITEM, "Screen Palette..",               "-",             0,               0, (APTR)MENU_SCREEN_PALETTE},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR},
    {    NM_ITEM, "Full-screen",                    "W", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_FULLSCREEN},
    {    NM_ITEM, "Title Bar",                      "R", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_TITLE_BAR},
    {    NM_ITEM, "Tool Bar",                       "4", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_TOOL_BAR},
    {    NM_ITEM, "LEDs",                           "I", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_LEDS},
    {    NM_ITEM, "Snapshot Windows",               "$",             0,               0, (APTR)MENU_SNAPSHOT_WINDOWS},

    { NM_TITLE, "Login",  0, 0, 0, (APTR)MENU_LOGIN},
    {    NM_ITEM, "Send Username",                  "N",             0,               0, (APTR)MENU_SEND_USERNAME},
    {    NM_ITEM, "Send Password",                  "P",             0,               0, (APTR)MENU_SEND_PASSWORD},

    { NM_END, NULL,  0, 0, 0, (APTR)MENU_END}
};
#ifdef __VBCC__
#pragma popwarn
#endif

static void GetWindowMsg(struct Window *wwin);
static void SendMisc(char *str, long len);
static void ReopenTerminal(void);
static void OpenAnsiFont(void);
static void RenewWorkbenchPens(void);
static void ResetTelnetContext(void);
static void ResetZmodemContext(void);
static void SetLocalEchoBack(BOOL wantedState);

extern struct ExecBase *SysBase;
#ifdef __GNUC__
    struct Library      *ReqToolsBase = NULL;
#else
    struct ReqToolsBase *ReqToolsBase = NULL;
#endif
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *KeymapBase, *GadToolsBase, *AslBase, *SocketBase;
struct Library *DiskfontBase, *IconBase, *WorkbenchBase, *UtilityBase;
struct Library *LayersBase;     // V39, optional: the own screen's black background
struct Library *CyberGfxBase;   // optional: recolours a true-colour terminal live

struct Window *win, *scrollbackWin, *toolBarWin;
static struct Window *packetWin;
struct List *scrollbackList;
struct Screen *scr;
struct DrawInfo *drawInfo;
static struct Gadget screenToBackGadget; // In top right corner when title bar is hidden in full screen
struct NewGadget newGadget;
static struct TextAttr fontAttr;    // describes the desired font
struct TextFont *ansiFont;          // actual font loaded via OpenFont(), ready to use
static struct TextFont *petsciiFont = NULL;      // upper/graphics charset font
static struct TextFont *petsciiFontLower = NULL; // shifted/lowercase charset font
// PETSCII only makes sense in a PETSCII BBS session: the C64 fonts and console
// setup are up only while a connection is being made or live (connectionDisplay)
// with PETSCII Mode on. Start-up and disconnected text keep the normal font.
static BOOL connectionDisplay = FALSE;   // a connection is being prepared or is live
// The Workbench window's place when the display was last closed: a reopen
// (an entry's settings, a font change) keeps it. The saved place
// (prefs.win_*) is only what Snapshot Windows stored.
static struct IBox wbWindowBox;
static BOOL wbWindowBoxValid = FALSE;
// The height of the screen title bar's text: the LEDs are drawn inside the
// bar as the screen made it (BarHeight is its height minus one, text plus
// two border lines), not from prefs.FontSize, which is only the setting.
#define BAR_TEXT_HEIGHT (scr->BarHeight - 2)

static const char *consoleFrom = "the system";   // where ibmcon.device came from
static char fontMissing[48];    // the font setting that could not be opened ("" = none)
static BOOL displayIsPetscii = FALSE;    // the display was opened for PETSCII
#define PETSCII_SESSION() (connectionDisplay && STATE_IS(APP_PETSCII_MODE))
// 32+ colours with ibmcon.device: the 16 ANSI colours get pens of their own
// (Palette_AnsiPens), handed to ibmcon with IBMCMD_SETPENS; the UI keeps
// Intuition's pens 0-7. On the Workbench they are 16 shared pens (V39).
static UBYTE ansiColourPens[16];
static BOOL  ansiOwnPens = FALSE;
static BOOL  wbPensObtained = FALSE;    // ansiColourPens came from ObtainBestPen on the Workbench
static BOOL  wbPenOwned[16];            // this pen was obtained (and is released on close)
static BYTE  penTableError = 0;         // io_Error of IBMCMD_SETPENS (IOERR_NOCMD: ibmcon < 1.5)
#define IBMCMD_SETPENS 0x7FE0          // ibmcon.device 1.5
static const UWORD intuitionPens[] = { 0xFFFF };   // SA_Pens: Intuition's own UI pens
static BOOL isConDeviceOpened = FALSE;
static struct IOStdReq *writeConsoleReq = NULL;
static struct MsgPort  *writeConsoleMP  = NULL;
struct Menu *mainMenuStrip;
static struct DiskObject *diskObj;
struct MsgPort *iconPort;
static struct AppIcon *appIconOnWB;
static struct sockaddr_in inetSocketAddr;
struct NewWindow newWin;

#define BUFSIZE 250
static UBYTE strBuffer[BUFSIZE+2];
static struct StringInfo strInfo;
static struct Gadget strGad;

enum    {    GAD_SCROLLER,
        GAD_UP,
        GAD_DOWN
    };

// prefs (prefs.c) holds the live settings: what every part of DCTelnet reads.
// Global settings kept aside while connected to an Address Book entry that has
// settings of its own (issue #10); only these are ever saved. Valid while
// sessionSettingsId != 0.
struct PrefsStruct globalPrefs;
ULONG sessionSettingsId = 0;      // settings id of the connected entry, 0 = global settings
// The global function keys, kept aside while an entry's own (Keyboard group) are live.
static TEXT globalFKeys[F_KEY_COUNT * F_KEY_SIZE];
static BOOL fKeysFromEntry = FALSE;
static ULONG sessionGroups = 0;   // SITE_GROUP_* the connected entry overrides
// The global settings and keys as they were when the current connection was
// prepared: Save Settings to Address Book Entry moves what changed since into
// the entry and puts these back.
static struct PrefsStruct connectBasePrefs;
static TEXT connectBaseKeys[F_KEY_COUNT * F_KEY_SIZE];

#define XEM_GLOBAL_OPTIONS "PROGDIR:DCTelnet.XEM"

/**
 * @brief The XEM options file for the current connection: the entry's own
 *        (PROGDIR:Sites/<id>.xem) when it overrides the Terminal group,
 *        else the global PROGDIR:DCTelnet.XEM.
 */
const char *CurrentXemOptionsPath(void)
{
    static char path[40];

    if (sessionSettingsId && (sessionGroups & SITE_GROUP_TERMINAL))
    {
        EntryXemOptionsPath(sessionSettingsId, path);
        return path;
    }
    return XEM_GLOBAL_OPTIONS;
}
// An entry's settings file stores the function keys with this size.
typedef char FKeysMatchSiteSettings[SITE_FKEY_BYTES == F_KEY_COUNT * F_KEY_SIZE ? 1 : -1];

static BPTR fileHandle;
long nScrollbackLines;
static long indexInScrollBuffer;
long tcpSocket, nBytesReceived;
static ULONG conectionTime;
static long nBytesSent;
void *visualInfos;
char username[42], password[42];
// TCP Receive buffer, used in Receive(), xpr_sflush(). Cauntion: these functs destroy the content
UBYTE recvBuffer[4096];
unsigned char buf[2048];
static unsigned char conbuf[16], scrollbuf[402];
char server[64];
static ULONG lasttop;        // last topline of scrollback
UWORD tcpPort = 23;    // current tcp port
UWORD winTop;        // WinTop topEdge (titlebar height)
BOOL shouldQuitApp;    // program finished
static BOOL isConnected;    // tcp connected
static BOOL shouldRestart;    // prefs changed, restart
static BOOL shouldReopenScreen;    // flag
static BOOL shouldReopenConsole;   // only the C64 display changes: reopen the console in place
static BOOL shouldIconify;        // must iconify
BOOL shouldUniconify;        // must uniconify
struct PetsciiDispatchState g_petsciiState;

static BOOL isFingerRequest;        // isFingerRequest?

static char *programName = NULL;   // Name provided by argv[0] or task::tc_Node.ln_Name
struct Task *mainTask = NULL;
BYTE dontUseSig31 = -1; // don't use it, ibmcon.device will destroy it.

#include "DCTelnet-debug.h"
#define send_data(data, len) TCPSend((data), (len))
#include "term-engine.c"


static void ConWrite(char *data, long len)
{
    if(STATE_IS_NOT(APP_ICONIFIED))
    {
        if (STATE_IS(APP_RENDERER_BUILTIN))
        {
            // With retro32-term, drawing only ever happens with the cursor hidden, so glyphs never
            // land on an inverted cell.
            cursor_hide();
            while (len-- > 0)
                term_feed((UBYTE)*data++);
            cursor_show();
            return; // direct return because maximal optimization is needed for this frequently called function
        }
        else if (STATE_IS(APP_RENDERER_CONSOLE_DEVICE | APP_RENDERER_IBMCON_DEVICE))
        {
            #ifdef _DEBUG
                if (!writeConsoleReq) RecoveryAlert(
                                       "Error writing to console: console device is unavailable.");
            #endif

            // Doc about passing requests to I/O device:
            // https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0006.html

            // An I/O request typically has three fields set for every command sent to a device:
            writeConsoleReq->io_Data = data;
            writeConsoleReq->io_Length = len;
            writeConsoleReq->io_Command = CMD_WRITE;
            DoIO((struct IORequest *)writeConsoleReq); // DoIO() is a synchronous function
            return;
        }
        else // APP_RENDERER_XEM_LIB:
        {
            XemWrite(data, len);

            return;
        }
    }
}

/*
 * DCTelnet's own messages. While the C64 display is up they are rewritten so
 * the letters read as intended in the active C64 charset (petscii_local_text).
 */
static void LocalWrite(char *data, long len)
{
    static struct PetsciiLocalText lt;
    static char chunk[256];

    if (!displayIsPetscii || !petsciiFont)
    {
        ConWrite(data, len);
        return;
    }
    petscii_local_text_init(&lt, win && win->RPort->Font == petsciiFontLower);
    while (len > 0)
    {
        long n = len < (long)sizeof(chunk) ? len : (long)sizeof(chunk);

        memcpy(chunk, data, n);
        petscii_local_text(&lt, chunk, (size_t)n);
        ConWrite(chunk, n);
        data += n;
        len -= n;
    }
}

void LocalPrint(char *data)
{
    LocalWrite(data, strlen(data));
}

// WARNING: This function uses the same global buffer "buf" that is also used by recv() to receive
// data from the TCP socket.
void LocalFmt(char *ctl, ...)
{
    #ifdef __VBCC__
    #pragma dontwarn 79 // warning 79: offset equals size of object
    #endif
    RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75", buf);
    #ifdef __VBCC__
    #pragma popwarn
    #endif
    LocalWrite(buf, strlen(buf));
}

// WARNING: This function uses the same global buffer "buf" that is also used by recv() to receive
// data from the TCP socket.
void TextFmt(struct RastPort *rP, char *ctl, ...)
{
    #ifdef __VBCC__
    #pragma dontwarn 79 // warning 79: offset equals size of object
    #endif
    RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75", buf);
    #ifdef __VBCC__
    #pragma popwarn
    #endif
    Text(rP, buf, strlen(buf));
}

// Wrapper around send() from bsdsocket.library that maintains the nBytesSent counter.
long TCPSend(const UBYTE *buf, long len)
{
    // Some SDKs declare send() with const buf, others without; this mismatch triggers SAS/C
    // warning 104, temporarily ignored here until properly handled.
    #ifdef __SASC
        #pragma msg 104 ignore push
    #endif
    if(send(tcpSocket, buf, len, 0) < 0) return -1;
    #ifdef __SASC
        #pragma msg 104 pop
    #endif
    nBytesSent += len;
    return len;
}

#define IBMCMD_GETCURSOR 0x7FE1         // ibmcon.device 1.9: io_Actual = row<<16 | column

// Answer a BBS's Device Status Report request (upstream #11: Absinthe and
// 20 For Beers stalled waiting for it). CSI 5 n: "OK". CSI 6 n: where the
// cursor is, asked from ibmcon 1.9; an older ibmcon and console.device
// cannot tell, and get no answer, as before. The built-in renderer and XEM
// (xem_swrite, see xpr_swrite) answer both themselves: DCTelnet does not
// answer twice.
static void AnswerDsr(int kind)
{
    char answer[16];
    UWORD row = 1, col = 1;
    size_t n;

    if (STATE_IS(APP_RENDERER_BUILTIN | APP_RENDERER_XEM_LIB))
        return;
    if (kind == DSR_POSITION)
    {
        if (!STATE_IS(APP_RENDERER_IBMCON_DEVICE) || !isConDeviceOpened || STATE_IS(APP_ICONIFIED))
            return;
        writeConsoleReq->io_Command = IBMCMD_GETCURSOR;
        writeConsoleReq->io_Data    = NULL;
        writeConsoleReq->io_Length  = 0;
        DoIO((struct IORequest *)writeConsoleReq);
        if (writeConsoleReq->io_Error)
            return;
        row = (UWORD)(writeConsoleReq->io_Actual >> 16);
        col = (UWORD)(writeConsoleReq->io_Actual & 0xFFFF);
    }
    n = Dsr_Answer(kind, row, col, answer, sizeof(answer));
    if (n)
        TCPSend(answer, (long)n);
}

// A BBS's text to the console. The text up to a Device Status Report
// request is drawn first, so the position answered is the one asked about.
static struct DsrScan bbsDsr;

static void BbsWrite(char *data, long len)
{
    while (len > 0)
    {
        int kind;
        size_t n = Dsr_Find(&bbsDsr, (const UBYTE *)data, (size_t)len, &kind);

        ConWrite(data, (long)n);
        if (kind != DSR_NONE && isConnected)
            AnswerDsr(kind);
        data += n;
        len  -= (long)n;
    }
}


/**
 * @brief Display a busy/wait mouse pointer in the specified window.
 *
 * Replaces the window's current pointer with a custom 16x16 "wait" pointer to indicate that a modal
 * operation or lengthy processing is in progress.
 *
 * The pointer remains active until removed with ClearPointer().
 *
  * @note The pointer bitmap is stored in Chip RAM because it is accessed directly by the Amiga
 *       custom hardware.
 *
 * @note This function only affects the specified window and does not modify the global Workbench
 *       pointer.
 *
 * @see ClearPointer()
 *
 * @see Amiga ROM Kernel Reference Manual: Libraries, 2nd Edition (v2.04),
 *      Chapter "Intuition Gadgets and Mouse Pointers",
 *      section "Pointer Example".
 */
void SetWaitPointer(struct Window *window)
{
    // VBCC requires static storage duration for __chip data.
    // This does not work when the array is declared const (reason unknown).
    static UWORD __chip waitPointerImage[] =
    {
        0x0000, 0x0000, // reserved, must be NULL
        0x0400, 0x07C0, // 1st line of the sprite image
        0x0000, 0x07C0,
        0x0100, 0x0380,
        0x0000, 0x07E0,
        0x07C0, 0x1FF8,
        0x1FF0, 0x3FEC,
        0x3FF8, 0x7FDE,
        0x3FF8, 0x7FBE,
        0x7FFC, 0xFF7F,
        0x7EFC, 0xFFFF,
        0x7FFC, 0xFFFF,
        0x3FF8, 0x7FFE,
        0x3FF8, 0x7FFE,
        0x1FF0, 0x3FFC,
        0x07C0, 0x1FF8,
        0x0000, 0x07E0, // last line of the sprite image
        0x0000, 0x0000, // reserved, must be NULL
    };

    if (window == NULL)
        return;

    SetPointer(window, waitPointerImage, 16, 16, -6, 0);
}


static void WindowSub(void (*Sub)(void))
{
    if(scrollbackWin) SetWaitPointer(scrollbackWin);
    if (toolBarWin)   SetWaitPointer(toolBarWin);
    SetWaitPointer(win);
    Sub();
    if(scrollbackWin) ClearPointer(scrollbackWin);
    if (toolBarWin) ClearPointer(toolBarWin);
    ClearPointer(win);
    LEDs();
}


void SimpleReq(char *str)
{
    InfoReq(win, str);
    LEDs();
}


/**
 * @brief Finds a menu item by its stable MenuItemID identifier.
 *
 * Searches all items in the application's main menu strip and compares their GadTools nm_UserData
 * values with the specified identifier.
 *
 * @param id Identifier of the menu item to find.
 * @return Pointer to the matching MenuItem, or NULL if not found.
 */
struct MenuItem *GetMenuItemFromID(enum MenuItemID id)
{
    struct Menu *menu;
    struct MenuItem *item;

    if (mainMenuStrip == NULL)
        return NULL;

    for (menu = mainMenuStrip; menu != NULL; menu = menu->NextMenu)
    {
        for (item = menu->FirstItem; item != NULL; item = item->NextItem)
        {
            if ((ULONG)GTMENUITEM_USERDATA(item) == (ULONG)id)
                return item;
        }
    }

    return NULL;
}

/**
 * @brief Finds a GadTools' NewMenu item by its stable MenuItemID identifier.
 *
 * Searches all items in the application's main NewMenu description and compares their GadTools
 * nm_UserData values with the specified identifier.
 *
 * @param id Identifier of the NewMenu item to find.
 * @return Pointer to the matching NewMenu item, or NULL if not found.
 */
struct NewMenu *GetNewMenuItemFromID(enum MenuItemID id)
{
    int i;

    for (i = 0; i < sizeof(mainMenuDesc); i++)
    {
        if ((enum MenuItemID) mainMenuDesc[i].nm_UserData == id)
            return &mainMenuDesc[i];
    }

    #ifdef _DEBUG
        SimpleReq("GetNewMenuItemFromID() failed!");
    #endif

    return NULL;
}



/**
 * @brief Finds the Intuition menu number associated with a MenuItemID.
 *
 * Searches the application's main menu strip for the menu whose GadTools nm_UserData value matches
 * the specified identifier.
 *
 * @param id Identifier of the menu to find.
 * @return The menu number, or -1 if no matching menu is found.
 */
WORD GetMenuNumberFromID(enum MenuItemID id)
{
    struct Menu *menu;
    WORD menuNumber = 0;

    for (menu = mainMenuStrip; menu != NULL; menu = menu->NextMenu)
    {
        if ((ULONG)GTMENU_USERDATA(menu) == (ULONG)id)
            return menuNumber;

        menuNumber++;
    }

    return -1;
}


// The disconnect notice, kept when ending the connection reopens the display
// (the C64 display goes, or an entry's settings end): printed again once the
// display is back, which clears the console. Empty = nothing to reprint.
static char disconnectNote[96];

static void DisConnect(char remote, char quiet)
{
    if(isConnected)
    {
        disconnectNote[0] = 0;
        if(!quiet && STATE_IS_NOT(APP_ICONIFIED))
        {
            register ULONG spent;

            spent = mytime() - conectionTime;
            mysprintf(disconnectNote, "›m\r\n%s. %02ld:%02ld:%02ld spent online.\r\n",
                      remote ? "Connection closed by foreign host" : "Connection closed",
                      spent/3600, (spent/60)%60, spent%60);
            LocalPrint(disconnectNote);
        }
        shutdown(tcpSocket, 2);
        CloseSocket(tcpSocket);

        isConnected = FALSE;
        ResetTelnetContext();
        ResetZmodemContext();
        nBytesReceived = 0;
        nBytesSent = 0;

        if(isFingerRequest)
        {
            WORD optionsMenuNumber = GetMenuNumberFromID(MENU_TERMINAL);

            ConWrite("\x1b[20l", 5);          // newline mode off: BBSes send CR LF

            if (optionsMenuNumber >= 0)
                OnMenu(win, FULLMENUNUM(optionsMenuNumber, NOITEM, NOSUB));

            isFingerRequest = FALSE;
        }

        LEDs();

        // The connection ends, and with it any Address Book entry's settings.
        ForgetConnectedEntry();
        EndEntrySession();
        if (!shouldRestart && !shouldReopenConsole)
            disconnectNote[0] = 0;      // the display stays: the notice is still there
    }
}

// Settings changed by hand while connected: kept for the run, never saved
// (SavePrefs in prefs.c takes them back).
struct SiteHandChanges handChanges;

// The settings a menu pick started from: what the pick changed by hand is the
// difference (IDCMP_MENUPICK). An entry session beginning or ending inside
// the pick (an Address Book connect, Disconnect) moves it, so the entry's
// settings coming or going are never taken for a change made by hand.
static struct PrefsStruct handBaseline;

// Address Book connect that waits for the display to be reopened with the
// entry's settings (see BeginEntrySession); run by the main loop.
static struct
{
    BOOL  active;
    char  name[32];
    char  host[52];
    UWORD port;
    ULONG settingsId;
    char  username[42], password[42];
    char  loginMacro[SITE_LOGIN_MACRO_SIZE];
    char  finger[64];       // a Finger query ("user@host") instead of a connect
} pendingConnect;

// Reopen the display when the C64 display it shows no longer matches what
// the connection state and PETSCII Mode want.
static void SyncPetsciiDisplay(void)
{
    if ((PETSCII_SESSION() != 0) != displayIsPetscii)
        shouldReopenConsole = TRUE;     // ReopenTerminal() in the main loop
}

// The screen mode's pixel shape (DisplayInfo.Resolution), from OpenAppScreen:
// which of topaz and Topaz Pro a font setting opens as (ScreenFont_ForMode).
static UWORD modeResX, modeResY;

// The font a setting actually opens on the current mode.
static void EffectiveFont(struct PrefsStruct *p)
{
    struct ScreenFontChoice pick;

    // (The built-in renderer keeps its 8x8 font: OpenAnsiFont.)
    if (p->State & APP_RENDERER_BUILTIN)
        return;
    ScreenFont_ForMode((const char *)p->FontName, p->FontSize, modeResX, modeResY, &pick);
    if (pick.name != (const char *)p->FontName)
        strlcpy((char *)p->FontName, pick.name, sizeof(p->FontName));
    p->FontSize = pick.size;
}

static void RequestDisplayReopen(const struct PrefsStruct *before)
{
    // Compared as they open: topaz and Topaz Pro are one face, so an entry
    // naming the other one reopens nothing.
    static struct PrefsStruct was, now;
    BOOL reopenScreen;

    was = *before;
    now = prefs;
    EffectiveFont(&was);
    EffectiveFont(&now);
    if (SitePrefs_DisplayDiffers(&was, &now, &reopenScreen))
    {
        // A new font or palette needs only the console reopened
        // (ReopenTerminal): the window, and DCTelnet's own screen, stay. On
        // the own screen the title bar and menus keep the screen's font
        // until the screen is next opened.
        if (SitePrefs_OnlyLookDiffers(&was, &now))
        {
            shouldReopenConsole = TRUE;
            return;
        }
        shouldRestart = TRUE;
        if (reopenScreen) shouldReopenScreen = TRUE;
    }
}

/**
 * @brief Prepare the settings for the next connect; call before every
 *        user-initiated connect.
 *
 * Ends any current connection (and with it any previous entry session). With
 * an entry (settingsId != 0), keeps the global settings aside in globalPrefs
 * and applies the entry's (window geometry stays global); menu changes from
 * now on are session-only and only globalPrefs is saved. With entry == NULL
 * the connect uses the global settings.
 *
 * @return TRUE when the display must be reopened before connecting: the
 *         caller queues the connect with DeferConnect().
 */
BOOL BeginEntrySession(ULONG settingsId, const struct SiteSettings *entry)
{
    static struct PrefsStruct before;

    if (isConnected) DisConnect(FALSE, FALSE);
    EndEntrySession();

    connectBasePrefs = prefs;
    memcpy(connectBaseKeys, fKeys, sizeof(connectBaseKeys));

    connectionDisplay = TRUE;
    if (entry)
    {
        before = prefs;
        globalPrefs = prefs;
        SitePrefs_ApplyEntry(&prefs, &globalPrefs, entry);
        fKeysFromEntry = SitePrefs_SwapInKeys(fKeys, globalFKeys, entry);
        sessionSettingsId = settingsId;
        sessionGroups = entry->groups;
        ReloadXemOptions();
        RequestDisplayReopen(&before);
    }
    SyncPetsciiDisplay();
    handBaseline = prefs;
    return shouldRestart || shouldReopenConsole;
}

/**
 * @brief The live settings were just saved as an entry's own (Settings > Save
 *        Settings to Address Book Entry): treat the rest of this connection as
 *        that entry's session. A session already running for it only takes
 *        the new group set.
 */
void AdoptEntrySession(ULONG settingsId, ULONG groups)
{
    if (!sessionSettingsId)
    {
        // What was changed since connecting now belongs to the entry: the
        // global settings go back to how they were then.
        globalPrefs = connectBasePrefs;
    }
    sessionSettingsId = settingsId;
    sessionGroups = groups;
    if ((groups & SITE_GROUP_KEYBOARD) && !fKeysFromEntry)
    {
        memcpy(globalFKeys, connectBaseKeys, sizeof(globalFKeys));
        fKeysFromEntry = TRUE;
    }
}

// The settings and keys the current connection started from (see AdoptEntrySession).
const struct PrefsStruct *ConnectBaseSettings(void)
{
    return &connectBasePrefs;
}

const TEXT *ConnectBaseFKeys(void)
{
    return connectBaseKeys;
}

// The global settings and function keys, also while an entry's are live.
const struct PrefsStruct *GlobalSettings(void)
{
    return sessionSettingsId ? &globalPrefs : &prefs;
}

const TEXT *GlobalFKeys(void)
{
    return fKeysFromEntry ? globalFKeys : fKeys;
}

BOOL SessionOverridesKeyboard(void)
{
    return fKeysFromEntry;
}

/**
 * @brief End a connection's settings (disconnect or a failed connect): the
 *        global settings come back after an entry session, and the C64
 *        display goes. Requests a display reopen when anything visible changes.
 */
void EndEntrySession(void)
{
    static struct PrefsStruct before;

    if (fKeysFromEntry)
    {
        SitePrefs_SwapOutKeys(fKeys, globalFKeys);
        fKeysFromEntry = FALSE;
    }

    connectionDisplay = FALSE;
    if (sessionSettingsId)
    {
        ULONG endedGroups = sessionGroups;

        before = prefs;
        SitePrefs_Restore(&prefs, &globalPrefs);
        sessionSettingsId = 0;
        sessionGroups = 0;
        if (endedGroups & SITE_GROUP_TERMINAL)
            ReloadXemOptions();
        RequestDisplayReopen(&before);
    }
    SyncPetsciiDisplay();
    handBaseline = prefs;
}

void DeferConnect(const char *name, const char *host, UWORD port, ULONG settingsId,
                  const char *user, const char *pass, const char *loginMacro)
{
    strlcpy(pendingConnect.loginMacro, loginMacro, sizeof(pendingConnect.loginMacro));
    strlcpy(pendingConnect.name, name, sizeof(pendingConnect.name));
    strlcpy(pendingConnect.host, host, sizeof(pendingConnect.host));
    pendingConnect.port = port;
    pendingConnect.settingsId = settingsId;
    strlcpy(pendingConnect.username, user, sizeof(pendingConnect.username));
    strlcpy(pendingConnect.password, pass, sizeof(pendingConnect.password));
    pendingConnect.active = TRUE;
}

/**
 * @brief Send an Address Book entry's login macro after connecting: \u, \p
 *        are the entry's username and password, \r Return, \d a one-second
 *        wait (SitePrefs_NextMacroSegment).
 */
void SendLoginMacro(const char *macro)
{
    static char segment[256];
    const char *cursor = macro;
    BOOL wait;
    size_t n;

    while (*cursor && isConnected)
    {
        n = SitePrefs_NextMacroSegment(&cursor, username, password, segment, sizeof(segment), &wait);
        if (n) SendMisc(segment, (long)n);
        if (wait) Delay(50);
    }
}

static void StartFinger(char *query);

static void RunPendingConnect(void)
{
    pendingConnect.active = FALSE;
    if (pendingConnect.finger[0])       // queued behind the display reopen
    {
        static char query[64];

        strlcpy(query, pendingConnect.finger, sizeof(query));
        pendingConnect.finger[0] = 0;
        StartFinger(query);
        return;
    }
    tcpPort = pendingConnect.port;
    if (BeginServerConnection(pendingConnect.host, pendingConnect.port) == RETURN_OK)
    {
        if (pendingConnect.name[0])     // "" = Connection > Connect, not an entry
        {
            RememberConnectedEntry(pendingConnect.name, pendingConnect.host, pendingConnect.port);
            StampConnectedEntry();
            strlcpy(username, pendingConnect.username, sizeof(username));
            strlcpy(password, pendingConnect.password, sizeof(password));
            SendLoginMacro(pendingConnect.loginMacro);
        }
    }
    else
        EndEntrySession();
}


// Ensures reqtools.library is available and initializes a requester TagList.
// Returns FALSE if reqtools.library cannot be opened or if reqtoolsTags is NULL.
static BOOL InitializeReqToolsLib(ULONG reqtoolsTags[5])
{
    BOOL result = FALSE;

    if (ReqToolsBase)
    {
        result=TRUE;    // ReqTools was already loaded.
    }
    else // ReqTools needs to be loaded now.
    {
        ReqToolsBase = (struct ReqToolsBase *)OpenNewestLibrary(REQTOOLSNAME, 0);

        if (ReqToolsBase)
        {
            result=TRUE;
            #ifdef _DEBUG
                PutStr("InitializeReqToolsLib() : ReqTools library loaded\n");
            #endif
        }
        else
        {
            InfoReq(win,
                    "DCTelnet - Missing Library\n\n"
                    "Unable to open reqtools.library.\n"
                    "This feature requires ReqTools.\n\n"
                    "Available from Aminet:\n"
                    "util/libs/ReqToolsUsr");
        }
    }


    if (reqtoolsTags)
    {
        reqtoolsTags[0] = RT_Window;
        reqtoolsTags[1] = (ULONG)win;
        reqtoolsTags[2] = RT_WaitPointer;
        reqtoolsTags[3] = TRUE;
        reqtoolsTags[4] = TAG_DONE;
    }
    else
    {
        result=FALSE;
        #ifdef _DEBUG
            InfoReq(win,
                    "InitializeReqToolsLib() called with a NULL argument!");
        #endif
    }

    return result;
}

/**
 * @brief Screen mode requester writing into target (the live settings, or an
 *        Address Book entry's copy). TRUE when the user chose a mode.
 */
BOOL ScreenModeInto(struct PrefsStruct *target)
{
    BOOL result = FALSE;

    if (AslBase && AslBase->lib_Version >= 38) // ASL screen mode requester introduced with AmigaOS 2.1
    {
        result = ScreenModeRequester(win, &target->DisplayID,
                                    &target->DisplayWidth, &target->DisplayHeight, &target->DisplayDepth,
                                    Prefs_MaxDepth(target));
    }
    else    // fallback to legacy ReqTools library
    {
        struct rtScreenModeRequester *scrmodereq;
        ULONG reqtoolsTags[5];

        InitializeReqToolsLib(reqtoolsTags);

        if(scrmodereq = rtAllocRequestA (RT_SCREENMODEREQ, NULL))
        {
            scrmodereq->DisplayID     = target->DisplayID;
            scrmodereq->DisplayWidth  = target->DisplayWidth;
            scrmodereq->DisplayHeight = target->DisplayHeight;
            scrmodereq->DisplayDepth  = target->DisplayDepth;

            if (rtScreenModeRequest (scrmodereq, "Screen Mode..",
                                     RT_Window,    win,
                                     RTSC_Flags,    SCREQF_DEPTHGAD|SCREQF_SIZEGADS|SCREQF_GUIMODES,
                                     RTSC_MaxDepth,    Prefs_MaxDepth(target) < 8 ? Prefs_MaxDepth(target) : 8,
                                     TAG_END))
            {
                target->DisplayID     = scrmodereq->DisplayID;
                target->DisplayWidth  = scrmodereq->DisplayWidth;
                target->DisplayHeight = scrmodereq->DisplayHeight;
                target->DisplayDepth  = scrmodereq->DisplayDepth;
                result = TRUE;
            }
            rtFreeRequest (scrmodereq);
        }
    }
    return result;
}

// The screen mode requester for the live settings (exported: LoadPrefs asks
// for a mode on first run).
BOOL ChooseScreen(void)
{
    // On first time init, returning FALSE prevents the preferences from being written to disk.
    return ScreenModeInto(&prefs);
}

/**
 * @brief The depth of a screen: GetBitMapAttr() on V39+ (an RTG bitmap's
 *        struct fields are not to be read), struct BitMap below.
 */
UWORD AppScreenDepth(struct Screen *s)
{
    if (GfxBase->LibNode.lib_Version >= 39)
        return (UWORD)GetBitMapAttr(s->RastPort.BitMap, BMA_DEPTH);
    return s->BitMap.Depth;
}

/**
 * @brief The text grid of the terminal window, as ibmcon draws it: the
 *        window's text area (inside a Workbench window's title bar and
 *        borders) divided by the font cell.
 */
static void TerminalGrid(UWORD *cols, UWORD *rows)
{
    struct TextFont *cell = win->RPort->Font;

    ScreenFont_Grid(win->Width, win->Height, win->GZZWidth, win->GZZHeight,
                    (win->Flags & WFLG_GIMMEZEROZERO) != 0,
                    cell->tf_XSize, cell->tf_YSize, cols, rows);
}

// The columns BBS art is drawn for in this font: 40 in the C64 fonts, else 80.
static UWORD ArtColumns(struct TextFont *cell)
{
    return (cell == petsciiFont || cell == petsciiFontLower)
         ? SCREENFONT_PETSCII_COLUMNS : SCREENFONT_ANSI_COLUMNS;
}

/**
 * @brief Size the Workbench window to cols x rows characters of its current
 *        font (the BBS size is 80x25), kept on the screen. Waits for
 *        Intuition to apply it (at most a second).
 */
static void SizeWorkbenchWindow(UWORD cols, UWORD rows)
{
    struct TextFont *cell = win->RPort->Font;
    WORD width  = (WORD)(cols * cell->tf_XSize + win->BorderLeft + win->BorderRight);
    WORD height = (WORD)(rows * cell->tf_YSize + win->BorderTop + win->BorderBottom);
    WORD left = win->LeftEdge, top = win->TopEdge;
    int wait;

    if (width > scr->Width)   width = scr->Width;
    if (height > scr->Height) height = scr->Height;
    if (left + width > scr->Width)   left = scr->Width - width;
    if (top + height > scr->Height)  top = scr->Height - height;
    // Room to grow: the width up to this one (LimitTerminalWidth sets it again),
    // the height up to the screen's -- a maximum of this height kept the window
    // from being made taller by hand afterwards.
    WindowLimits(win, 0, 0, (UWORD)width, (UWORD)scr->Height);
    ChangeWindowBox(win, left, top, width, height);
    for (wait = 0; wait < 50 && (win->Width != width || win->Height != height); wait++)
        Delay(1);
}

/**
 * @brief Keep the terminal at most 80 text columns wide (40 in PETSCII
 *        Mode): BBS art is drawn for exactly that width and wraps at its
 *        edge. A Workbench window stops there (WindowLimits); on DCTelnet's
 *        own screen the window is that wide and centred, the screen's
 *        background around it ANSI black (see OpenAppWindow). Called once the
 *        console's font is final and after every Workbench resize.
 */
static void LimitTerminalWidth(void)
{
    struct TextFont *cell;
    UWORD columns, maxWidth, width, left;
    int wait;

    if (!win)
        return;
    cell = win->RPort->Font;
    columns = ArtColumns(cell);
    maxWidth = ScreenFont_MaxWindowWidth(columns, cell->tf_XSize,
                                         (UWORD)(win->BorderLeft + win->BorderRight));
    if (STATE_IS_NOT(APP_FULLSCREEN))
    {
        width = win->Width > maxWidth ? maxWidth : win->Width;
        left  = win->LeftEdge;
    }
    else
    {
        width = maxWidth > scr->Width ? scr->Width : maxWidth;
        left  = (UWORD)((scr->Width - width) / 2);
    }
    if (win->Width != width || win->LeftEdge != left)
    {
        ChangeWindowBox(win, left, win->TopEdge, width, win->Height);
        // Intuition applies it asynchronously: wait, so the console measures
        // the new width before the next write (at most a second).
        for (wait = 0; wait < 50 && (win->Width != width || win->LeftEdge != left); wait++)
            Delay(1);
    }
    // WindowLimits() ignores a maximum below the current width: after the
    // shrink above (or on the IDCMP_NEWSIZE that follows) it takes.
    if (STATE_IS_NOT(APP_FULLSCREEN))
        WindowLimits(win, 0, 0, maxWidth, 0);
}

/*
 * Backfill hook of the terminal window (layers.library protocol: A0 hook,
 * A2 RastPort, A1 message; the NDK's __REG__ macros say it for every
 * compiler, as Xfer.c's XPR callbacks do). Whatever the window exposes -- a resize on the
 * Workbench, a depth arrange -- is terminal background: ANSI black on its
 * own pen, not the Workbench's grey pen 0 (ibmcon clears to ANSI black).
 */
struct LayerBackFillMsg
{
    struct Layer    *layer;
    struct Rectangle bounds;
    LONG             offsetX, offsetY;
};

static ULONG __SAVE_DS__ __ASM__ TerminalBackFill(__REG__(a0, struct Hook *hook),
                                                  __REG__(a2, struct RastPort *rp),
                                                  __REG__(a1, struct LayerBackFillMsg *msg))
{
    struct RastPort crp = *rp;

    crp.Layer = NULL;           // straight into the bitmap: layers holds the lock
    SetDrMd(&crp, JAM2);
    SetAPen(&crp, ansiOwnPens ? ansiColourPens[0] : 0);
    RectFill(&crp, msg->bounds.MinX, msg->bounds.MinY, msg->bounds.MaxX, msg->bounds.MaxY);
    return 0;
}

static struct Hook terminalBackFill = { { NULL, NULL }, (HOOKFUNC)TerminalBackFill, NULL, NULL };

// A pen DCTelnet draws with itself (LEDs, the Quit item), given as an ANSI
// colour in ibmcon's order: on 32+ colours it lives on the ANSI pens, with the
// built-in and XEM renderers pens 0-15 hold the ANSI palette in ANSI order.
static UWORD LegacyPen(UWORD ibmconIndex)
{
    if (ansiOwnPens)
        return ansiColourPens[Palette_IbmconToAnsi(ibmconIndex)];
    if (STATE_IS(APP_RENDERER_BUILTIN | APP_RENDERER_XEM_LIB))
        return (UWORD)Palette_IbmconToAnsi(ibmconIndex);
    return ibmconIndex;
}

// Load target's palette onto the screen: the ANSI pens (in ANSI order, 8 bits
// a gun) on 32+ colours, pens 0-15 otherwise.
static void LoadAnsiPalette(struct PrefsStruct *target)
{
    if (STATE_IS_NOT(APP_FULLSCREEN))
        return;                 // the Workbench's pens are shared: never recolour them
    if (ansiOwnPens && GfxBase->LibNode.lib_Version >= 39)
    {
        UWORD i;

        for (i = 0; i < 16; i++)
        {
            ULONG c = Palette_AnsiColour(target->DeviceColors, i);
            SetRGB32(&scr->ViewPort, ansiColourPens[i], ((c >> 16) & 0xFF) * 0x01010101UL,
                     ((c >> 8) & 0xFF) * 0x01010101UL, (c & 0xFF) * 0x01010101UL);
        }
    }
    else
        LoadRGB4(&scr->ViewPort, Prefs_Palette(target), 16);
}

/*
 * The 16 ANSI colours editor: a colour picker holding exactly the 16 ANSI
 * colours in ANSI order, their name, and Red/Green/Blue sliders (0-15, the
 * RGB4 the settings keep). It edits the palette target's renderer shows
 * (Palette_Get/Put: AnsiColors or DeviceColors) of the live settings, or of
 * an Address Book entry's copy. On DCTelnet's own screen the colours are
 * shown on the terminal's own pens, so the terminal recolours while the
 * sliders move; on the Workbench on 16 exclusive pens borrowed for the
 * preview (the shared pens the terminal uses are never recoloured) and
 * given back.
 */
#define RECTFMT_ARGB 2          // cybergraphics: 4 bytes per pixel, 0xAARRGGBB
#define RECOLOUR_STRIP 16       // rows read and written at a time

// TRUE when the terminal's pixels hold colours rather than pen numbers (an
// RTG screen deeper than 8 bits): recolouring a pen then changes nothing
// already drawn, and RecolourTerminal() rewrites the pixels instead.
static BOOL TerminalIsTrueColour(void)
{
    return CyberGfxBase && win && AppScreenDepth(scr) > 8;
}

// Every pixel of the terminal in colour from (0x00RRGGBB) becomes to.
static void RecolourTerminal(ULONG from, ULONG to)
{
    BOOL gzz = (win->Flags & WFLG_GIMMEZEROZERO) != 0;
    UWORD w = gzz ? win->GZZWidth : win->Width;
    UWORD h = gzz ? win->GZZHeight : win->Height;
    ULONG *px = AllocVec((ULONG)w * RECOLOUR_STRIP * 4, MEMF_ANY);
    UWORD y, n;
    ULONG i;

    if (!px || from == to)
    {
        if (px) FreeVec(px);
        return;
    }
    for (y = 0; y < h; y += RECOLOUR_STRIP)
    {
        n = (UWORD)(h - y < RECOLOUR_STRIP ? h - y : RECOLOUR_STRIP);
        ReadPixelArray(px, 0, 0, (UWORD)(w * 4), win->RPort, 0, y, w, n, RECTFMT_ARGB);
        for (i = 0; i < (ULONG)w * n; i++)
            if ((px[i] & 0xFFFFFF) == from)
                px[i] = (px[i] & 0xFF000000) | to;
        WritePixelArray(px, 0, 0, (UWORD)(w * 4), win->RPort, 0, y, w, n, RECTFMT_ARGB);
    }
    FreeVec(px);
}

enum { PE_PALETTE, PE_NAME, PE_RED, PE_GREEN, PE_BLUE, PE_USE, PE_DEFAULT, PE_CANCEL, PE_COUNT };

static UBYTE editPens[16];          // pen of each ANSI colour in the editor
static BOOL  editPenOwned[16];      // borrowed for the preview: given back on close
static BOOL  editLive;              // SetRGB32/SetRGB4 on editPens shows the colour

static void ShowEditedColour(int ansi, UWORD rgb4)
{
    if (!editLive)
        return;
    SetRGB4(&scr->ViewPort, editPens[ansi], (rgb4 >> 8) & 0xF, (rgb4 >> 4) & 0xF, rgb4 & 0xF);
}

// The preview swatch right of the colour's name. On a true-colour screen
// (RTG 15-bit and up) recolouring a pen does not change what is already
// drawn, so the swatch and the colour picker are drawn again after every
// change; on 256 colours and fewer they recolour by themselves.
static struct { WORD x0, y0, x1, y1; } editSwatch;

static void DrawEditPreview(struct Window *w, struct Gadget *picker, int sel)
{
    SetAPen(w->RPort, editPens[sel]);
    SetDrMd(w->RPort, JAM1);
    RectFill(w->RPort, editSwatch.x0, editSwatch.y0, editSwatch.x1, editSwatch.y1);
    RefreshGList(picker, w, NULL, 1);
}

static void ShowSelectedColour(struct Window *w, struct Gadget **gads, const UWORD *pal, int sel)
{
    DrawEditPreview(w, gads[PE_PALETTE], sel);
    GT_SetGadgetAttrs(gads[PE_NAME], w, NULL, GTTX_Text, (ULONG)Palette_Name(sel), TAG_DONE);
    GT_SetGadgetAttrs(gads[PE_RED],   w, NULL, GTSL_Level, Palette_Channel(pal[sel], 0), TAG_DONE);
    GT_SetGadgetAttrs(gads[PE_GREEN], w, NULL, GTSL_Level, Palette_Channel(pal[sel], 1), TAG_DONE);
    GT_SetGadgetAttrs(gads[PE_BLUE],  w, NULL, GTSL_Level, Palette_Channel(pal[sel], 2), TAG_DONE);
}

BOOL EditPalette(struct PrefsStruct *target)
{
    static UWORD pal[16], inUse[16];
    static const char *labels[PE_COUNT] = { NULL, NULL, "Red", "Green", "Blue", "Use", "Default", "Cancel" };
    struct Gadget *glist = NULL, *g, *gads[PE_COUNT];
    struct NewGadget ng;
    struct Window *pw = NULL;
    struct TextFont *font = scr->RastPort.Font;
    WORD fh = font->tf_YSize, cw = font->tf_XSize;
    WORD x0 = cw, width = 34 * cw, labelW = 7 * cw, levelW = 4 * cw, y;
    BOOL v39 = GfxBase->LibNode.lib_Version >= 39, done = FALSE, kept = FALSE;
    BOOL onWorkbench = STATE_IS_NOT(APP_FULLSCREEN);
    BOOL live = target == &prefs && TerminalIsTrueColour();  // recolour the terminal's pixels
    BOOL deviceOrder = Prefs_Palette(&prefs) == prefs.DeviceColors;  // pens 0-15 hold ibmcon order
    BOOL recolour = FALSE;
    static ULONG shown[16];     // the colours the terminal's pixels have now
    int sel = 0, i;

    if (!v39 && onWorkbench)
    {
        InfoReq(win, "The ANSI colours can be edited on the Workbench from OS 3.0 on.");
        return FALSE;
    }
    Palette_Get(target, pal);
    Palette_Get(&prefs, inUse);
    for (i = 0; i < 16; i++)
        shown[i] = Palette_RGB32(pal[i]);

    // The pens the editor shows the colours on.
    editLive = TRUE;
    for (i = 0; i < 16; i++)
    {
        editPenOwned[i] = FALSE;
        if (onWorkbench)
        {
            ULONG c = Palette_RGB32(pal[i]);
            LONG pen = ObtainPen(scr->ViewPort.ColorMap, (ULONG)-1, ((c >> 16) & 0xFF) * 0x01010101UL,
                                 ((c >> 8) & 0xFF) * 0x01010101UL, (c & 0xFF) * 0x01010101UL,
                                 PEN_EXCLUSIVE);
            editPenOwned[i] = pen >= 0;
            if (pen >= 0) editPens[i] = (UBYTE)pen;
            else
            {
                // No free pen: the nearest colour already there, which cannot
                // be recoloured and is not ours to give back.
                editPens[i] = (UBYTE)FindColor(scr->ViewPort.ColorMap,
                                  ((c >> 16) & 0xFF) * 0x01010101UL, ((c >> 8) & 0xFF) * 0x01010101UL,
                                  (c & 0xFF) * 0x01010101UL, -1);
                editLive = FALSE;
            }
        }
        else
            editPens[i] = ansiOwnPens ? ansiColourPens[i]
                        : (UBYTE)(deviceOrder ? Palette_IbmconToAnsi(i) : i);
    }
    for (i = 0; i < 16; i++)
        ShowEditedColour(i, pal[i]);

    // The gadgets, laid out in the screen font's cells.
    y = scr->WBorTop + scr->Font->ta_YSize + 1 + fh / 2;
    g = CreateContext(&glist);
    for (i = 0; g && i < PE_COUNT; i++)
    {
        memset(&ng, 0, sizeof(ng));
        ng.ng_TextAttr   = scr->Font;
        ng.ng_VisualInfo = visualInfos;
        ng.ng_GadgetID   = i;
        ng.ng_GadgetText = (UBYTE *)labels[i];
        switch (i)
        {
        case PE_PALETTE:
            ng.ng_LeftEdge = x0; ng.ng_TopEdge = y; ng.ng_Width = width; ng.ng_Height = 2 * fh + 4;
            g = v39 ? CreateGadget(PALETTE_KIND, g, &ng, GTPA_ColorTable, (ULONG)editPens,
                                   GTPA_NumColors, 16, GTPA_Color, editPens[0], TAG_DONE)
                    : CreateGadget(PALETTE_KIND, g, &ng, GTPA_Depth, 4, GTPA_Color, editPens[0], TAG_DONE);
            y += 2 * fh + 4 + fh / 2;
            break;
        case PE_NAME:
            ng.ng_LeftEdge = x0; ng.ng_TopEdge = y; ng.ng_Width = width - 9 * cw; ng.ng_Height = fh + 4;
            editSwatch.x0 = x0 + width - 8 * cw; editSwatch.x1 = x0 + width - 1;
            editSwatch.y0 = y;                   editSwatch.y1 = y + fh + 3;
            g = CreateGadget(TEXT_KIND, g, &ng, GTTX_Text, (ULONG)Palette_Name(0), GTTX_Border, TRUE, TAG_DONE);
            y += fh + 4 + fh / 2;
            break;
        case PE_RED: case PE_GREEN: case PE_BLUE:
            ng.ng_LeftEdge = x0 + labelW; ng.ng_TopEdge = y;
            ng.ng_Width = width - labelW - levelW; ng.ng_Height = fh + 2;
            ng.ng_Flags = PLACETEXT_LEFT;
            g = CreateGadget(SLIDER_KIND, g, &ng, GTSL_Min, 0, GTSL_Max, 15,
                             GTSL_Level, Palette_Channel(pal[0], i - PE_RED),
                             GTSL_MaxLevelLen, 2, GTSL_LevelFormat, (ULONG)"%2ld",
                             GTSL_LevelPlace, PLACETEXT_RIGHT, GA_RelVerify, TRUE,
                             GA_Immediate, TRUE, TAG_DONE);
            y += fh + 2 + fh / 2;
            break;
        default:        // Use, Default, Cancel in one row
            ng.ng_Width = (width - 2 * cw) / 3; ng.ng_Height = fh + 6;
            ng.ng_LeftEdge = x0 + (i - PE_USE) * (ng.ng_Width + cw); ng.ng_TopEdge = y;
            ng.ng_Flags = PLACETEXT_IN;
            g = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
            break;
        }
        gads[i] = g;
    }
    if (g)
        pw = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)scr, WA_Gadgets, (ULONG)glist,
                            WA_Title, (ULONG)"ANSI Colours",
                            WA_InnerWidth, width + 2 * cw, WA_InnerHeight, y + fh + 6 + fh / 2 - scr->WBorTop - scr->Font->ta_YSize - 1,
                            WA_Left, (scr->Width - width) / 2, WA_Top, scr->BarHeight + 20,
                            WA_IDCMP, PALETTEIDCMP | SLIDERIDCMP | BUTTONIDCMP | TEXTIDCMP
                                      | IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW,
                            WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE,
                            WA_Activate, TRUE, WA_RMBTrap, TRUE, WA_AutoAdjust, TRUE, TAG_DONE);
    if (pw)
    {
        GT_RefreshWindow(pw, NULL);
        DrawEditPreview(pw, gads[PE_PALETTE], sel);
        while (!done)
        {
            struct IntuiMessage *m;

            WaitPort(pw->UserPort);
            while (!done && (m = GT_GetIMsg(pw->UserPort)))
            {
                ULONG class = m->Class;
                UWORD code = m->Code;
                struct Gadget *gad = (struct Gadget *)m->IAddress;

                GT_ReplyIMsg(m);
                if (class == IDCMP_CLOSEWINDOW) done = TRUE;
                else if (class == IDCMP_REFRESHWINDOW)
                {
                    GT_BeginRefresh(pw);
                    GT_EndRefresh(pw, TRUE);
                    DrawEditPreview(pw, gads[PE_PALETTE], sel);
                }
                else if (class == IDCMP_GADGETUP || class == IDCMP_MOUSEMOVE || class == IDCMP_GADGETDOWN)
                {
                    switch (gad->GadgetID)
                    {
                    case PE_PALETTE:            // code = the pen clicked
                        for (i = 0; i < 16; i++)
                            if (editPens[i] == code) { sel = i; break; }
                        if (!v39) sel = deviceOrder ? Palette_IbmconToAnsi(code) : code;
                        ShowSelectedColour(pw, gads, pal, sel);
                        break;
                    case PE_RED: case PE_GREEN: case PE_BLUE:
                        pal[sel] = Palette_WithChannel(pal[sel], gad->GadgetID - PE_RED, (UBYTE)code);
                        ShowEditedColour(sel, pal[sel]);
                        DrawEditPreview(pw, gads[PE_PALETTE], sel);
                        recolour = live;        // once the queued moves are read
                        break;
                    case PE_DEFAULT:
                        Palette_DefaultFor(target, pal);
                        for (i = 0; i < 16; i++) ShowEditedColour(i, pal[i]);
                        for (i = 0; live && i < 16; i++)
                            if (Palette_SafeRecolour(shown, i, Palette_RGB32(pal[i])))
                            {
                                RecolourTerminal(shown[i], Palette_RGB32(pal[i]));
                                shown[i] = Palette_RGB32(pal[i]);
                            }
                        ShowSelectedColour(pw, gads, pal, sel);     // redraws the preview
                        break;
                    case PE_USE:
                        Palette_Put(target, pal);
                        kept = done = TRUE;
                        break;
                    case PE_CANCEL:
                        done = TRUE;
                        break;
                    }
                }
            }
            // A drag queues many moves: the pixels follow once they are read.
            if (recolour && Palette_SafeRecolour(shown, sel, Palette_RGB32(pal[sel])))
            {
                RecolourTerminal(shown[sel], Palette_RGB32(pal[sel]));
                shown[sel] = Palette_RGB32(pal[sel]);
            }
            recolour = FALSE;
        }
        CloseWindow(pw);
    }
    FreeGadgets(glist);

    // Not kept: the terminal's pixels get the colours in use back.
    for (i = 0; live && !kept && i < 16; i++)
        if (Palette_SafeRecolour(shown, i, Palette_RGB32(inUse[i])))
        {
            RecolourTerminal(shown[i], Palette_RGB32(inUse[i]));
            shown[i] = Palette_RGB32(inUse[i]);
        }

    // Back to the live colours: the borrowed pens go back, and on the own
    // screen the terminal's pens show the settings in use.
    for (i = 0; i < 16; i++)
        if (editPenOwned[i])
            ReleasePen(scr->ViewPort.ColorMap, editPens[i]);
    if (!onWorkbench)
        LoadAnsiPalette(&prefs);
    return kept;
}

static void ChoosePalette(void)
{
    // On the own screen the terminal already shows the new colours; on the
    // Workbench the running console takes new shared pens for them (a
    // reopen would clear it).
    if (EditPalette(&prefs) && STATE_IS_NOT(APP_FULLSCREEN))
        RenewWorkbenchPens();
}


// Application-defined Exec List with Node-specific data
// https://wiki.amigaos.net/wiki/Exec_Lists_and_Queues#Finding_the_List_of_a_Node
struct Scroll
{
    struct Node    nnode;
    long        len;    // Node-specific data
};

/**
 * @brief Append characters from a raw input buffer into the scrollback buffer.
 *
 * AddBuf() processes a sequence of characters and stores them into an internal scrollback buffer.
 * The function interprets control characters (newline, tab, bell, form feed, carriage return) and a
 * subset of Amiga console escape sequences (CSI), converting them into scrollback entries.
 *
 * The function accumulates characters in a temporary buffer until a newline or a CSI command that
 * forces a line break is encountered. At that point, a new Scroll node is allocated, filled, and
 * appended to the scrollback list. If the scrollback exceeds the configured limit, the oldest entry
 * is removed to keep memory usage bounded.
 *
 * Some CSI sequences (notably 'C', 'H', and 'B') require prematurely flushing the current buffer.
 * To avoid duplicating the line?finalization logic, the function uses three goto jumps that
 * redirect execution to the common "add" block. This structure triggers VBCC warning 175 ("this
 * code is weird") when optimization is enabled, hence the conditional suppression pragma.
 *
 * @param str   Pointer to the raw input byte buffer.
 * @param size  Number of bytes to process from the buffer.
 */
#ifdef __VBCC__
    #ifndef _DEBUG
        #pragma dontwarn 175  // function "AddBuf": this code is weird
    #endif
#endif
static void AddBuf(unsigned char *str, long size)
{
    register long i = 0, n;
    struct Scroll *node, *nextnode;
    char numb[32];

    while(i < size)
    {
        switch(str[i])
        {
            case '\n':
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
                if(nScrollbackLines > prefs.nScrollbackLines)
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
                            PGA_Total,    nScrollbackLines,
                        TAG_DONE);
                    }
                }
                indexInScrollBuffer = 0;
                break;
            case '\a':  // 	Alert (Bell)
            case '\t':
            case '\f':  // Form Feed (clear the window)
            case '\r':
                break;
            case ESC_CHAR:
                i++;
            case CSI_CHAR:   // Amiga console CSI
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


#include "DCTelnet-protocol.h"

static void Receive(void)
{
    static UBYTE outBuffer[sizeof(recvBuffer)];
    LONG len;
    LONG i;
    LONG outLen = 0;

    len = recv(tcpSocket, recvBuffer, sizeof(recvBuffer), 0);

    #ifdef _DEBUG_WAITSELECT
        Printf("   --> Receive() => %ld\n", len);
    #endif

    if (len <= 0) // Connection closed or error
    {
        DisConnect(TRUE, FALSE);
        return;
    }

    nBytesReceived += len;

    #ifdef _DEBUG
        // Generate a capture file of all received data, unprocessed:
        fileHandle = Open("PROGDIR:capture_in.bin", MODE_READWRITE);
        if(fileHandle)
        {
            Seek(fileHandle, 0, OFFSET_END);
            Write(fileHandle, recvBuffer, len);
            Close(fileHandle);
        }
    #endif

    if (STATE_IS(APP_RAW_CONNECTION))
    {
        BbsWrite(recvBuffer, len);
        if (STATE_IS(APP_SCROLLBACK_ENABLED))
            AddBuf(recvBuffer, len);

        for (i = 0; i < len; i++)
        {
            if (ZmodemDetect(recvBuffer[i]))
                break;
        }
    }
    else // Telnet connection
    {
        // High-frequency loop: called once per received character; avoid branching or allocations:
        for(i = 0; i < len; i++)
        {
            #ifdef _DEBUG
                if (outLen >= sizeof(outBuffer))
                {
                    InfoReq(win, "outBuffer overflow in Receive()!");
                    return;
                }
            #endif

            // Optimization as most of the time this will prevent to make a function call to fully
            // process the received character:
            if (  telnetCtx.state  == TELNET_STATE_DATA && recvBuffer[i] != IAC
                && zmodemCtx.state == ZMODEM_IDLE       && recvBuffer[i] != '*')
            {
                outBuffer[outLen++] = recvBuffer[i];
                continue;   // directly jump to the process of next received char
            }

            // Full process of telnet IAC commands and Zmodem detection:
            if (TelnetParseByte(recvBuffer[i]))
            {
                outBuffer[outLen++] = recvBuffer[i];

                ZmodemDetect(recvBuffer[i]);
                // The Zmodem transfer will start later, once the remaining recvBuffer has been
                // fully processed for any pending IAC Telnet command sequences
            }
        }

        if (outLen > 0)
        {
            if (PETSCII_SESSION())
            {
                /* Translated in chunks sized so even the worst-case expansion
                 * (PETSCII_MAX_OUT_PER_BYTE) fits: output is never truncated. */
                static UBYTE petsciiOut[sizeof(recvBuffer)];
                const LONG chunkMax = sizeof(petsciiOut) / PETSCII_MAX_OUT_PER_BYTE;
                LONG done;

                for (done = 0; done < outLen; done += chunkMax)
                {
                    LONG chunk = (outLen - done < chunkMax) ? outLen - done : chunkMax;
                    LONG petsciiOutLen;

                    if (petsciiFont) {
                        petsciiOutLen = (LONG)petscii_stream_to_rawglyphs(&g_petsciiState,
                                                                             outBuffer + done, (size_t)chunk,
                                                                             petsciiOut, sizeof(petsciiOut));
                        /* Charset-shift control code (14/142) toggles between the
                         * upper/graphics and shifted/lowercase C64 charsets --
                         * swap the active console font to match. */
                        if (win && petsciiFontLower) {
                            struct TextFont *wanted = g_petsciiState.shift_lowercase ? petsciiFontLower : petsciiFont;
                            if (win->RPort->Font != wanted) SetFont(win->RPort, wanted);
                        }
                    } else {
                        petsciiOutLen = (LONG)petscii_stream_to_ansi(&g_petsciiState,
                                                                      outBuffer + done, (size_t)chunk,
                                                                      petsciiOut, sizeof(petsciiOut));
                    }
                    ConWrite((char *)petsciiOut, petsciiOutLen);
                    if (STATE_IS(APP_SCROLLBACK_ENABLED))
                        AddBuf((char *)petsciiOut, petsciiOutLen);
                }
            }
            else
            {
                BbsWrite(outBuffer, outLen);
                if (STATE_IS(APP_SCROLLBACK_ENABLED))
                    AddBuf(outBuffer, outLen);
            }
        }

        // Detect end of the server's initial negotiation sequence. Trigger client-side negotiation
        // once the threshold is reached.
        if (!telnetCtx.isClientNegotiationTriggered && telnetCtx.isServerNegotiationSeen
            && outLen > 80)
        {
            TelnetNegotiateRequiredOptions();
            telnetCtx.isClientNegotiationTriggered = TRUE;
        }
    }

    if (   zmodemCtx.state == ZMODEM_DOWNLOAD
        || zmodemCtx.state == ZMODEM_UPLOAD)
    {
        // Some BBSes never respond to Telnet option negotiation; this is for informational purposes
        // only:
        if (STATE_IS_NOT(APP_RAW_CONNECTION))
            IsTelnetSessionReadyForXfer();

        if (zmodemCtx.state == ZMODEM_DOWNLOAD) Download(prefs.XferLibrary);
        if (zmodemCtx.state == ZMODEM_UPLOAD)     Upload(prefs.XferLibrary);

        ResetZmodemContext();
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
                        buf[j] = ESC_CHAR;
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
norm:                // Typed text in PETSCII Mode goes out case-swapped (petscii_keymap);
                // a macro's text is typed text too. Escapes (\r, \123) stay raw.
                buf[j] = PETSCII_SESSION()
                         ? (char)petscii_translate_key((unsigned char)str[i], 0) : str[i];
                j++;
        }
        i++;
    }

    SendMisc(buf, j);
}

void LEDs(void)
{
    // Draw connection activity indicator when Title bar AND LEDs are enabled AND NOT iconified
    if (STATE_ARE_ALL(APP_TITLE_BAR_ENABLED | APP_LEDS_ENABLED)  &&  STATE_IS_NOT(APP_ICONIFIED))
    {
        EraseRect(&scr->RastPort, scr->Width-72, 2, scr->Width-60, BAR_TEXT_HEIGHT-1);
        EraseRect(&scr->RastPort, scr->Width-86, 2, scr->Width-74, BAR_TEXT_HEIGHT-1);
        if(isConnected)
        {
            SetAPen(&scr->RastPort, LegacyPen(15));
            RectFill(&scr->RastPort, scr->Width-84, 3, scr->Width-76, BAR_TEXT_HEIGHT-2);
        }
    }
}


/**
 * @brief Run a terminal output speed test and display the elapsed time.
 *
 * The test outputs either the contents of PROGDIR:speedtest.ans or, if the file cannot be opened,
 * 200 colored ASCII lines. The elapsed time is measured with CurrentTime().
 *
 * The terminal cursor is temporarily hidden during the test.
 *
 */
static ULONG SpeedTest(void)
{
    ULONG before_s, before_micros;
    ULONG after_s, after_micros;
    ULONG elapsed_tenths;
    BPTR fh;

    // "›0 p" = 9B 30 20 70 : Set Cursor Rendition -> make cursor invisible
    //                        (disabling the cursor slightly improves output speed)
    // "›m"   = 9B 6D        : Select Graphic Rendition -> reset attributes (white on black)
    // "\f"   = 0x0C (FF)    : Form Feed -> clear the window
    // Reference: Amiga ROM Kernel Reference Manual v2.04 - Devices (1991),
    //            section "Control Sequences for Window Output"
    ConWrite("›0 p›m\f", 7);

    if (STATE_IS(APP_RENDERER_XEM_LIB))
    {
        LocalPrint("WARNING: The Xem library may hang the terminal window during this test!\r\n"
                   "If this happens, use \"Reset Screen\" from the DCTelnet menu.\r\n");
        Delay(3 * TICKS_PER_SECOND);
    }

    fh = Open("PROGDIR:speedtest.ans", MODE_OLDFILE);

    CurrentTime(&before_s, &before_micros);

    if (fh == 0) // File open error, so simply print ASCII lines
    {
        int i;
        int color;

        for (i = 1; i < 201; i++)
        {
            color = i % 8;  // cycle through ANSI colors 0-7
            LocalFmt("›3%ldmLine %ld.\r\n", color, i);  // ESC[3Xm -> set foreground color
        }
    }
    else
    {
        LONG len;

        while ((len = Read(fh, recvBuffer, sizeof(recvBuffer))) > 0)
            ConWrite(recvBuffer, len);

        Close(fh);
    }

    CurrentTime(&after_s, &after_micros);

    // Convert the elapsed time to tenths of a second, rounding to the nearest tenth using integer
    // arithmetic.
    // CurrentTime() updates the time at most 60 times per second, so the sub-second value has a
    // maximum resolution of about 0.01667 s, which is sufficient for a tenth-of-a-second
    // measurement.
    if (after_micros >= before_micros)
    {
        elapsed_tenths = (after_s - before_s) * 10
                       + (after_micros - before_micros + 50000) / 100000;
    }
    else
    {
        elapsed_tenths = (after_s - before_s - 1) * 10
                       + (1000000 - before_micros + after_micros + 50000) / 100000;
    }

    LocalFmt("›0m›255B\r\nDuration: %ld.%ld seconds\r\n",
             elapsed_tenths / 10,
             elapsed_tenths % 10);

    if (fh == 0)
    {
        if (elapsed_tenths == 0)
        {
            LocalPrint("Rating: Incredible\r\n");
        }
        else
        {
            // Round to the nearest integer using integer arithmetic.
            LONG lines_per_second = (2000 + elapsed_tenths / 2) / elapsed_tenths;

            STRPTR rating;

            if      (lines_per_second < 20) rating = "Poor";
            else if (lines_per_second < 30) rating = "Average";
            else if (lines_per_second < 50) rating = "Good";
            else                             rating = "Excellent";

            LocalFmt("Speed: %ld lines/second\r\nRating: %s\r\n", lines_per_second, rating);
        }
    }

    #ifdef _DEBUG
        LocalPrint("WARNING: Use an optimized release binary for representative results!\r\n");
    #endif

    // Restore cursor visibility after the test.
    ConWrite("›1 p", 4);

    return elapsed_tenths;
}

static void ClearScrollBack(void)
{
    struct Scroll *worknode, *nextnode;

    if (scrollbackList == NULL) return;

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

// Send a Finger query ("user@host", modified in place) to host port 79 as a
// raw connection: plain text, no telnet negotiation.
static void StartFinger(char *query)
{
    char *host = strchr(query, '@');
    BOOL originalState;

    if (!host)
        return;
    originalState = STATE_IS(APP_RAW_CONNECTION);
    *host++ = 0;

    STATE_SET(APP_RAW_CONNECTION);     // Enable flag (NO telnet negotiation)
    if(BeginServerConnection(host, 79) == RETURN_OK)
    {
        WORD optionsMenuNumber = GetMenuNumberFromID(MENU_TERMINAL);

        mysprintf(buf, "/W %s\r\n", query);
        send(tcpSocket, buf, strlen(buf), 0);

        // Finger servers end lines with a bare LF (Unix), which on a
        // terminal only moves down: the reply came out as a staircase.
        // Newline mode (LNM, CSI 20 h) makes LF return to column 1 too,
        // for the reply only.
        ConWrite("\x1b[20h", 5);

        // Prevent the user from toggling Raw Connection (or any other Terminal option)
        // while this finger exchange relies on it. Re-enabled in DisConnect().
        if (optionsMenuNumber >= 0)
            OffMenu(win, FULLMENUNUM(optionsMenuNumber, NOITEM, NOSUB));

        isFingerRequest = TRUE;
    }
    if (!originalState)
        STATE_UNSET(APP_RAW_CONNECTION);    // Restore state
}

static void Finger(void)
{
    static char tbuf[64] = "reiver@plan.cat";

    if (GetStringRequester(win,
                              "Finger",
                              "Enter EMail Address:",
                              tbuf, sizeof(tbuf))
       && strchr(tbuf, '@'))
    {
        // End any entry session (and its connection) BEFORE touching the
        // flag: ending it later, inside the connect, would restore the
        // global settings over APP_RAW_CONNECTION.
        // Finger is plain text: no C64 display, whatever PETSCII Mode says.
        if (isConnected) DisConnect(FALSE, FALSE);
        EndEntrySession();
        if (shouldRestart || shouldReopenConsole)
        {
            // The display reopens for the global settings first; a finger
            // sent now would have its answer wiped by that reopen.
            strlcpy(pendingConnect.finger, tbuf, sizeof(pendingConnect.finger));
            pendingConnect.active = TRUE;
        }
        else
        {
            static char query[64];

            strlcpy(query, tbuf, sizeof(query));
            StartFinger(query);
        }
    }
}



int main(int argc, char *argv[])
{
    ULONG iconsig = 0, sigmask, winsig;
    LONG i;
    struct timeval timeout;
    fd_set rd;
    int returnCode = RETURN_FAIL;
    #ifdef _DEBUG
        ULONG beforeSigAlloc;
        ULONG afterSigAlloc;
    #endif
    mainTask = FindTask(NULL);


    // Launched from Shell/CLI (including cases where the program is started from a Workbench icon
    // with "Shell" selected in the "Start from" dropdown list)
    if(argc >= 1)
    {
        programName=argv[0];
    }
    else    // Launched from Workbench
    {
        if (mainTask != NULL)
            programName = mainTask->tc_Node.ln_Name;
    }

    if(argc >= 2)
    {
        if(strcmp(argv[1], "?") == 0 || strcmp(argv[1], "/?") == 0 || strcmp(argv[1], "-?") == 0 ||
           strcmp(argv[1], "-h")  == 0 || strcmp(argv[1], "--help")  == 0)
        {
            PutStr(
                "DCTelnet "DCTELNET_VERSION " (build " STR(BUILD_HASH) ") ("__DATE__
                                               ") - A classic Amiga Telnet/BBS client with Zmodem\n"
                "compiled with: " STR(COMPILER_STRING) "\n"
                "\n"
                "Usage: DCTelnet <host> [<port>]\n"
            );

            returnCode = RETURN_OK;
            goto clean_exit;
        }

        strlcpy(server, argv[1], sizeof(server));

        if(argc > 2) tcpPort = atoi(argv[2]);

        //if(argc > 3) if(stricmp(argv[3], "debug")==0) debug = TRUE;
    }

    // Needed right now for InfoReq() information requesters:
    IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 36);
    if (IntuitionBase == NULL) {
        RecoveryAlert(
                   "DCTel requires Intuition lib v36 which is available in AmigaOS 2.0 and later.");
        goto clean_exit;
    }

    GfxBase = (struct GfxBase*) OpenLibrary("graphics.library", 0);  // intuition.library uses it
    if (GfxBase == NULL)
    {
        RecoveryAlert(
            "The graphics.library could not be opened. DCTelnet requires graphics.library.");
        goto clean_exit;
    }

    if (mainTask == NULL)
    {
        const char msg[] = "ERROR: cannot FindTask()!";
        PutStr(msg);
        InfoReq(NULL, msg);
        goto clean_exit;
    }

    if (programName == NULL)
    {
        const char msg[] = "ERROR: cannot determine program name!";
        PutStr(msg);
        InfoReq(NULL, msg);
        goto clean_exit;
    }


    // Workaround for connection freeze after changing display settings: ibmcon.device before
    // 1.8 frees signal bit 31 when being closed (its UnitClose deleted the handler's port in our
    // task, issue #3). ibmcon 1.8 and later (built from ibmcon/) fix it; the reservation stays
    // for every older ibmcon -- the one in the package's Devs drawer, one in DEVS:, one still
    // in memory and in use elsewhere, which DCTelnet then shares.
    dontUseSig31 = AllocSignal(31L);
    if (dontUseSig31 != 31)
        InfoReq(NULL, "ERROR: cannot allocate sigbit 31!");

    AslBase = OpenLibrary("asl.library", 0);
    if (AslBase == NULL)
    {
        InfoReq(NULL, "The ASL library could not be opened.\n"
                     "DCTelnet requires asl.library, which is available in AmigaOS 2.0 and later.");
        goto clean_exit;
    }

    GadToolsBase = OpenLibrary("gadtools.library", 0);
    if (GadToolsBase == NULL)
    {
        InfoReq(NULL, "GadTools library could not be opened.\n"
                "DCTelnet requires gadtools.library, which is available in AmigaOS 2.0 and later.");
        goto clean_exit;
    }

    LayersBase = OpenLibrary("layers.library", 39);
    CyberGfxBase = OpenLibrary("cybergraphics.library", 40);   // Picasso96 / CyberGraphX

    UtilityBase = OpenLibrary("utility.library", 0);
    if (UtilityBase == NULL)
    {
        InfoReq(NULL, "utility library could not be opened.\n"
                "DCTelnet requires utility.library.");
        goto clean_exit;
    }


    if (! LoadPrefs()) goto clean_exit;
    SitePrefs_HandInit(&handChanges, &prefs);

    scrollbackList = AllocMem(sizeof(struct List), MEMF_CLEAR|MEMF_PUBLIC);
    if(!scrollbackList) goto clean_exit;

    scrollbackList->lh_TailPred = (struct Node *)scrollbackList;
    scrollbackList->lh_Head = (struct Node *)&scrollbackList->lh_Tail;


    WorkbenchBase = OpenLibrary("workbench.library", 0);
    if (WorkbenchBase == NULL) { InfoReq(NULL,"Unable to open workbench.library"); goto clean_exit; }

    DiskfontBase = OpenLibrary("diskfont.library", 0);
    if (DiskfontBase == NULL) { InfoReq(NULL,"Unable to open diskfont.library"); goto clean_exit; }

    KeymapBase = OpenLibrary("keymap.library", 0);
    if (KeymapBase == NULL) { InfoReq(NULL,"Unable to open keymap.library"); goto clean_exit; }

    IconBase = OpenLibrary("icon.library", 0);
    if (IconBase == NULL) { InfoReq(NULL,"Unable to open icon.library"); goto clean_exit; }

    #ifdef _DEBUG
        PutStr("--> OpenLibrary(bsdsocket.library, 0)\n");
        beforeSigAlloc = mainTask->tc_SigAlloc;
    #endif
    SocketBase = OpenLibrary("bsdsocket.library", 0);
    #ifdef _DEBUG
        PutStr("<-- OpenLibrary(bsdsocket.library, 0)\n");
        afterSigAlloc = mainTask->tc_SigAlloc;
        socketLibSigBit = BitPosition(beforeSigAlloc ^ afterSigAlloc); // XOR detect the differences
        Printf("                  socketLibSigBit = %lu\n", (LONG) socketLibSigBit);
    #endif
    // We retry opening the bsdsocket.library later as user might load a TCP/IP stack later.


    if (! OpenDisplay())
        goto clean_exit;

    #ifdef _DEBUG
        PutStr("<-- OpenDisplay()\n");
        PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
        LogWindowsSigBit();
    #endif

    shouldRestart = FALSE;
    shouldReopenScreen = FALSE;

    // Connect to server if it was specified in the command line. It needs an opened display,
    // and in PETSCII Mode the C64 one: the main loop reopens it, then connects.
    if (server[0] != '\0')
    {
        if (stricmp(server, "--test-speed") == 0)
        {
            ULONG elapsed_tenths = SpeedTest();

            Printf("{\"renderer_duration_seconds\":%ld.%ld}\r\n", elapsed_tenths / 10, elapsed_tenths % 10);

            returnCode = RETURN_OK;

            goto clean_exit;
        }
        else
        {
            if (BeginEntrySession(0, NULL))
                DeferConnect("", server, tcpPort, 0, "", "", "");
            else
                BeginServerConnection(server, tcpPort);
        }
    }

/* ------ main loop ------ */
    shouldQuitApp = FALSE;
    while(! shouldQuitApp)
    {
        if(shouldIconify)
        {
            #ifdef _DEBUG
            if (!win) RecoveryAlert("Error: no window to iconify!");
            #endif
            CloseDisplay(TRUE);
            // inconify the application:
            OpenIcon();
            shouldIconify = FALSE;
        }

        if(shouldUniconify)
        {
            #ifdef _DEBUG
            if (!appIconOnWB) RecoveryAlert("Error: no icon on WB!");
            #endif
            CloseIcon();
            OpenDisplay();

            #ifdef _DEBUG
                PutStr("<-- OpenDisplay()\n");
                PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
                LogWindowsSigBit();
            #endif

            shouldUniconify = FALSE;
        }

        if(STATE_IS(APP_ICONIFIED))
        {
            if(iconPort) iconsig = 1L<<iconPort->mp_SigBit;

            if(isConnected)
            {
                FD_ZERO(&rd);
                FD_SET(tcpSocket, &rd);
                sigmask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | iconsig;

                // https://wiki.amigaos.net/amiga/autodocs/bsdsocket.doc.txt (at the end)
                // WaitSelect() should probably return the time remaining from the original timeout,
                // if any, by modifying the time value in place. This may be implemented in future
                // versions of the system. Thus, it is unwise to assume that the timeout value will
                // be unmodified by the WaitSelect() call.
                // WaitSelect() returns the number of ready descriptors that are contained in the
                // descriptor sets,
                // or -1 if an error occurred.
                // If the time limit expires, WaitSelect() returns 0.
                // Reception of a user signal with no socket ready will cause WaitSelect() to stop
                // and to return 0.
                timeout.tv_sec = 30; timeout.tv_usec = 0;
                i = WaitSelect(tcpSocket + 1, &rd, 0, 0, &timeout, &sigmask);

                #ifdef _DEBUG
                    if (i <  0)  InfoReq(win,
                                         "WaitSelect() returns < 0 (error) ! Why???");
                #endif

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
                register struct Message *msg;
                while(msg = GetMsg(iconPort))
                {
                    if (  ((struct AppMessage *)msg)->am_NumArgs == 0
                       && ((struct AppMessage *)msg)->am_ArgList == NULL)
                        shouldUniconify = TRUE;
                    ReplyMsg(msg);
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
                if(packetWin) sigmask |= 1L << packetWin->UserPort->mp_SigBit;
                if (toolBarWin) sigmask |= 1L << toolBarWin->UserPort->mp_SigBit;

                timeout.tv_sec = 30; timeout.tv_usec = 0;
                i = WaitSelect(tcpSocket + 1, &rd, 0, 0, &timeout, &sigmask);

                #ifdef _DEBUG_WAITSELECT
                    if      (i <  0)
                    {
                        InfoReq(win,
                                "WaitSelect() returns < 0 (error) ! Why???");
                    }
                    else if (i == 0)
                    {
                        PutStr("<-- WaitSelect() => 0 (= timeout or signal received)\n    sigs=");
                        LogWaitSelectResult(sigmask);

                        if (FD_ISSET(tcpSocket, &rd))
                            InfoReq(win,
                                    "WaitSelect() returns 0 but data received! Why???");
                    }
                    else
                    {
                        Printf("<-- WaitSelect() => %ld (= data received)", i);
                    }
                #endif

                GetWindowMsg(win);

                if(scrollbackWin) GetWindowMsg(scrollbackWin);
                if(packetWin) GetWindowMsg(packetWin);
                if (toolBarWin) GetWindowMsg(toolBarWin);

                if(i != 0)
                {
                    // Draw when Title bar AND LEDs are enabled :
                    if (STATE_ARE_ALL(APP_TITLE_BAR_ENABLED | APP_LEDS_ENABLED))
                    {
                        SetAPen(&scr->RastPort, LegacyPen(10));
                        RectFill(&scr->RastPort, scr->Width-70, 3, scr->Width-62, BAR_TEXT_HEIGHT-2);
                    }
                    Receive();
                    if (STATE_ARE_ALL(APP_TITLE_BAR_ENABLED | APP_LEDS_ENABLED))
                        EraseRect(&scr->RastPort, scr->Width-72, 2, scr->Width-60, BAR_TEXT_HEIGHT-1);
                }

            } else {  // not connected
                ULONG sig;

                if(scrollbackWin)  sig = 1L << scrollbackWin->UserPort->mp_SigBit; else sig = 0;
                if(packetWin) sig |= 1L << packetWin->UserPort->mp_SigBit;
                if (toolBarWin) sig |= 1L << toolBarWin->UserPort->mp_SigBit;

                // A display reopen or a connect already queued (an entry's
                // settings, PETSCII Mode) runs now, not after the next event.
                if (shouldRestart || shouldReopenConsole || pendingConnect.active)
                    sigmask = SetSignal(0L, sig | winsig | SIGBREAKF_CTRL_C)
                            & (sig | winsig | SIGBREAKF_CTRL_C);
                else
                    sigmask = Wait( sig | winsig | SIGBREAKF_CTRL_C );

                if(scrollbackWin)
                {
                    if(sigmask&(1L << scrollbackWin->UserPort->mp_SigBit)) GetWindowMsg(scrollbackWin);
                }
                if(packetWin)
                {
                    if(sigmask&(1L << packetWin->UserPort->mp_SigBit)) GetWindowMsg(packetWin);
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
                if (! OpenDisplay())
                    goto clean_exit;

                shouldRestart = FALSE;
                shouldReopenScreen = FALSE;
                shouldReopenConsole = FALSE;    // the full reopen covered it
            }
            else if (shouldReopenConsole)
            {
                ReopenTerminal();
                shouldReopenConsole = FALSE;
            }

            // A disconnect that reopened the display: its notice again.
            if (disconnectNote[0] && !shouldRestart && !shouldReopenConsole && !isConnected)
            {
                LocalPrint(disconnectNote);
                disconnectNote[0] = 0;
            }

            // After the reopen, so the connect sees the entry's display settings.
            if (pendingConnect.active)
                RunPendingConnect();
        }
    } /* -- end of main loop -- */

    SavePrefs();

    STATE_UNSET(APP_TITLE_BAR_ENABLED);

    returnCode = RETURN_OK;

clean_exit:
    DisConnect(FALSE, TRUE);
    CloseDisplay(TRUE);

    ClearScrollBack();
    FreeMem(scrollbackList, sizeof(struct List));

    if (SocketBase)    CloseLibrary(SocketBase);
    if (IconBase)      CloseLibrary(IconBase);
    if (KeymapBase)    CloseLibrary(KeymapBase);
    if (DiskfontBase)  CloseLibrary(DiskfontBase);
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (UtilityBase)   CloseLibrary(UtilityBase);
    if (LayersBase)    CloseLibrary(LayersBase);
    if (CyberGfxBase)  CloseLibrary(CyberGfxBase);
    if (GadToolsBase)  CloseLibrary(GadToolsBase);
    if (ReqToolsBase)  CloseLibrary((struct Library *) ReqToolsBase);
    if (AslBase)       CloseLibrary(AslBase);
    if (GfxBase)       CloseLibrary((struct Library *) GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *) IntuitionBase);

    if (dontUseSig31 != -1) FreeSignal(dontUseSig31);

    #ifdef _DEBUG
        PutStr("<-- clean finished... will return...\n");
        if ((mainTask->tc_SigAlloc & 0xFFFF0000UL) != 0)
        {
            PutStr("ERROR: Some signal bits were not unallocated!");
            PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
        }
    #endif

    return returnCode;
}

static void SaveScrollBack(char *fname)
{
    struct Scroll *worknode, *nextnode;
    fileHandle = Lock(fname, SHARED_LOCK);
    if(fileHandle)
    {
        UnLock(fileHandle);
        if (! ConfirmRequester(win, "OverWrite|Cancel",
                               "File Already Exists."))
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

/**
 * @brief Handles the user "Connect" action from the UI.
 *
 * This function is triggered when the user clicks the Connect button.
 * It prompts for a "host:port" string, parses the input, and determines the target server and TCP
 * port (defaulting to port 23 if not specified).
 *
 * Depending on the execution mode:
 * - In spawnInstance mode, it launches a new instance of DCTelnet via Execute().
 * - otherwise, it starts the connection task through BeginServerConnection().
 *
 * @param spawnInstance If non-zero, runs the connection in a separate DCTelnet instance.
 * If zero, connects in the current DCTelnet instance.
 */
static void OnConnectClicked(char spawnInstance)
{
    char tbuf[64];
    UWORD port = 0;

    if(!spawnInstance)  strlcpy(tbuf, server, sizeof(tbuf));
    else                tbuf[0] = '\0';


    if (GetStringRequester(win, "Connect",
                           "Enter host:port",  tbuf, sizeof(tbuf))
       )
    {
        if(tbuf[0] != 0)
        {
            register char *po;

            po = strchr(tbuf, ',');

            if(po == NULL) // No ',' found. Check whether the input uses the "host:port" syntax
            {
                char *p = strchr(tbuf, ':');

                // A ':' is treated as a port separator only if it is the only one in the string.
                // This avoids confusing IPv6 addresses with "hostname:port" syntax.
                if(p && strchr(p + 1, ':') == NULL)
                    po = p;
            }

            if(po)
            {
                *po++ = '\0';
                port = (UWORD)atoi(po);
            }

            if(!port) port = 23;

            if(spawnInstance)
            {
                mysprintf(buf, "run %s %s %ld <>NIL:", programName, tbuf, port);
                #ifdef _DEBUG
                    PutStr("--> Execute(");
                    PutStr(buf);
                    PutStr(")\n");
                #endif
                // This function attempts to execute the string commandString as a Shell command
                Execute(buf, (BPTR) 0, (BPTR) 0);
            } else if (BeginEntrySession(0, NULL)) {
                // Ending an entry session reopens the display: connect after it.
                DeferConnect("", tbuf, port, 0, "", "", "");
            } else {
                tcpPort = port;
                BeginServerConnection(tbuf, tcpPort);
            }
        }
    }
}

static void Information(void)
{
    if(isConnected)
    {
        register ULONG spent = mytime() - conectionTime;

        InfoReq(win,
                "     Host Name ... : %s\n"
                "    IP Address ... : %s\n"
                "      TCP Port ... : %ld\n\n"
                "   Online Time ... : %02ld:%02ld:%02ld\n"
                "    Bytes Sent ... : %ld\n"
                "Bytes Received ... : %ld",
                server,
                Inet_NtoA(inetSocketAddr.sin_addr.s_addr),
                tcpPort,
                spent/3600, (spent/60)%60, spent%60,
                nBytesSent,
                nBytesReceived);
    }
    else
        SimpleReq("Not isConnected");
}


/**
 * @brief Updates a NewMenu item's checked state based on a preference flag.
 *
 * If the specified flag is set in prefs.State, the menu item is checked.
 * Otherwise, the menu item is unchecked.
 *
 * @param id Identifier of the NewMenu item whose CHECKED state is to be set.
 * @param flag The flag in prefs.State controlling the menu item's checked state.
 */
static void SetNewMenuCheckFromPref(enum MenuItemID id, ULONG flag)
{
    struct NewMenu *newMenuItem = GetNewMenuItemFromID(id);
    if (newMenuItem == NULL)
    {
        #ifdef _DEBUG
            SimpleReq("SetNewMenuCheckFromPref() failed!");
        #endif
        return;
    }

    if (STATE_IS(flag))
        newMenuItem->nm_Flags |= CHECKED;
    else
        newMenuItem->nm_Flags &= ~CHECKED;
}


/**
 * @brief Updates prefs.State based on a menu item's checked state.
 *
 * If the menu item is checked, the prefs.State bit is set.
 * If it is unchecked, the flag bit is cleared.
 *
 * @param item Pointer to the MenuItem whose CHECKED state is to be used.
 * @param flag The flag (bit) in prefs.State to set or clear.
 */
static void UpdatePrefsFromMenu(struct MenuItem *item, ULONG flag)
{
    if(item->Flags & CHECKED)
        STATE_SET(flag);
    else
        STATE_UNSET(flag);
}

/*
 Uncheck a menu item and clear the corresponding flag in prefs.State,
 or check the menu item and set the flag, depending on the "wantedState" parameter.
 https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node024A.html
 https://www.amiga-news.de/en/news/AN-2023-10-00017-EN.html
*/
static void SetLocalEchoBack(BOOL wantedState)
{
    struct MenuItem *item = NULL;

    BOOL currentState = STATE_IS(APP_LOCAL_ECHO);

    #ifdef _DEBUG
        PutStr("›34m--> SetLocalEchoBack()›m\n");
    #endif

    if (currentState != wantedState)
    {
        #ifdef _DEBUG
            PutStr("›34mcurrentState != wantedState›m\n");
        #endif
        ClearMenuStrip(win);

        item = GetMenuItemFromID(MENU_LOCAL_ECHO);
        if (item != NULL)
        {
            if (wantedState)
            {
                item->Flags |= CHECKED;
                STATE_SET(APP_LOCAL_ECHO);
            }
            else
            {
                item->Flags &= ~CHECKED;
                STATE_UNSET(APP_LOCAL_ECHO);
            }
        }
        ResetMenuStrip(win, mainMenuStrip);
    }
}

static void OutKey(unsigned char key)
{
    if(STATE_IS(APP_BACKSPACE_DEL_SWAPPED))
    {
        if(key == '\b') // Backspace
            key = DEL_CHAR;
        else {
            if(key == DEL_CHAR) key = '\b';
        }
    }
    if(isConnected)
    {
        TCPSend((void *)&key, 1);

        // If you want to send 0xff then you must double it (0xff, 0xff) to tell telnet that you
        // don't intend to send it a command.
        if(key == (unsigned char) IAC)
            if (STATE_IS_NOT(APP_RAW_CONNECTION))
                TCPSend((void *)&key, 1);

        if(STATE_IS(APP_LOCAL_ECHO))
            goto cwrite;
    } else
cwrite:        ConWrite(&key, 1);
}


static void GetWindowMsg(struct Window *wwin)
{
    struct IntuiMessage *message;
    struct Gadget *gad;
    ULONG class;
    UWORD code;
    UWORD qual;
    char fbuf[128];
    char close = FALSE;
    char resize = FALSE;
    BOOL shouldCloseToolbarWin = FALSE;

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
            if(wwin == packetWin)  // A line has been validated in the packet window;
            {                      // send it to the server.
                RemoveGList(wwin, &strGad, 1);
                strlcat(strBuffer, STATE_IS(APP_RETURN_SENDING_CRLF) ? "\r\n" : "\r",
                        sizeof(strBuffer));
                SendMacro(strBuffer);
                strBuffer[0] = 0;
                ((struct StringInfo *)(strGad.SpecialInfo))->BufferPos = 0;
                ((struct StringInfo *)(strGad.SpecialInfo))->DispPos = 0;
                AddGList(wwin, &strGad, (ULONG)~0, 1, NULL);
                RefreshGList(&strGad, wwin, NULL, 1);
                ActivateGadget(&strGad, packetWin, 0);
            }

            if(wwin == scrollbackWin)
            {
                GetAttr(PGA_Top, Scroller, (ULONG *)&lasttop);
                RefreshListView(lasttop);
            }

            // The gadget in top right corner when title bar is hidden in full screen mode
            if(gad->GadgetID == GADGET_SCREEN_TO_BACK) ScreenToBack(scr);

            if(wwin == toolBarWin)
            {
                switch(gad->GadgetID)
                {
                    case BUTTON_CONNECT:
                        OnConnectClicked(FALSE);
                        break;
                    case BUTTON_DISCONNECT:
                        DisConnect(FALSE, FALSE);
                        break;
                    case BUTTON_ADDRESS_BOOK:
                        WindowSub(AddressBook);
                        break;
                    case BUTTON_INFORMATION:
                        WindowSub(Information);
                        break;
                    case BUTTON_UPLOAD:
                    case BUTTON_DOWNLOAD:
                        if(isConnected)
                        {
                            if(gad->GadgetID == BUTTON_UPLOAD)
                                Upload(prefs.XferLibrary);
                            else
                                Download(prefs.XferLibrary);
                        } else
                            SimpleReq("You better connect first.");
                        break;
                    case BUTTON_QUIT:
                        shouldQuitApp = TRUE;
                    break;
                }
            }
            break;


        case IDCMP_NEWSIZE:
            //LocalPrint("\017\233\164\233\165\233\166\233\167");
            if(wwin == scrollbackWin) resize = TRUE;
            else if (wwin == win)
            {
                // A Workbench resize: keep the width limit, and tell the BBS
                // the new grid so its next screen fits (the console does not
                // reflow what is already drawn).
                LimitTerminalWidth();
                if (isConnected) TelnetSendWindowSize();
            }
            break;


        /*case IDCMP_REFRESHWINDOW:
            GT_RefreshWindow(wwin, NULL);
            break;*/


        case IDCMP_RAWKEY:
            if(wwin == scrollbackWin)
            {
                switch(code)
                {
                case RAWKEY_CRSRUP:
                    goto up;
                case RAWKEY_CRSRDOWN:
                    goto down;
                case RAWKEY_F5:
                    buf[0] = '\0';
                    strlcpy(fbuf, "DCTelnet.Cap", sizeof(fbuf));
                    if (FileRequester(win,
                                      buf,  sizeof(buf),
                                      fbuf, sizeof(fbuf),
                                      "#?",
                                      FILEREQ_SAVE))
                    {
                        AddPart(buf, fbuf, sizeof(buf));
                        //strcat(buf, fbuf);
                        SaveScrollBack(buf);
                    }
                    break;
                case RAWKEY_F3:
                    if (ConfirmRequester(win, "Print|Cancel",
                                         "Print Scrollback?"))
                        SaveScrollBack("PRT:");
                    break;
                case RAWKEY_F1:
                    ClearScrollBack();
                    nScrollbackLines = 0;
                    lasttop = 0;
                    SetGadgetAttrs((struct Gadget *)Scroller, scrollbackWin, NULL,
                        PGA_Total,    0,
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
                    static char key_csi;
                    static char key_macro;

                    ie.ie_Class        = IECLASS_RAWKEY;
                    ie.ie_SubClass        = 0;
                    ie.ie_Code        = code;
                    ie.ie_Qualifier        = qual;
                    ie.ie_position.ie_addr    = gad;

                    length = MapRawKey(&ie, conbuf, 16, NULL);

                    for(i=0; i<length; i++)
                    {
                        switch(conbuf[i])
                        {
                        case CSI_CHAR:   // Amiga console CSI
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
                                /* The Amiga console reports F1-F10 as CSI <digit> ~ with
                                 * digit 0-9 (F1 = 0). A key with a macro sends the macro,
                                 * PETSCII Mode or not. In PETSCII Mode a key WITHOUT a
                                 * macro sends the C64 function-key byte instead (F1-F8
                                 * only; F9/F10 have no C64 counterpart). */
                                if (conbuf[i] >= '0' && conbuf[i] <= '9'
                                    && fKeys[(conbuf[i] - '0') * F_KEY_SIZE] != 0)
                                {
                                    key_macro = TRUE;
                                    SendMacro(&fKeys[(conbuf[i] - '0') * F_KEY_SIZE]);
                                }
                                else if (PETSCII_SESSION()
                                         && petscii_fkey_from_console_digit(conbuf[i]) >= 0)
                                {
                                    key_macro = TRUE;
                                    OutKey((unsigned char)petscii_fkey_from_console_digit(conbuf[i]));
                                }

                                if (PETSCII_SESSION())
                                {
                                    switch(conbuf[i])
                                    {
                                    case 'A': OutKey((unsigned char)petscii_translate_key(PETSCII_KEY_UP, 1));    break;
                                    case 'B': OutKey((unsigned char)petscii_translate_key(PETSCII_KEY_DOWN, 1));  break;
                                    case 'C': OutKey((unsigned char)petscii_translate_key(PETSCII_KEY_RIGHT, 1)); break;
                                    case 'D': OutKey((unsigned char)petscii_translate_key(PETSCII_KEY_LEFT, 1));  break;
                                    }
                                }
                                else switch(conbuf[i])
                                {
                                case 'A':
                                    SendMisc(ESC_STR "[A", 3);
                                    break;
                                case 'B':
                                    SendMisc(ESC_STR "[B", 3);
                                    break;
                                case 'C':
                                    SendMisc(ESC_STR "[C", 3);
                                    break;
                                case 'D':
                                    SendMisc(ESC_STR "[D", 3);
                                    break;
                                }

                            } else {
                                if(key_macro)
                                    key_macro = FALSE;
                                else
                                {
                                    if (PETSCII_SESSION()
                                        && (conbuf[i] == DEL_CHAR || conbuf[i] == '\b'))
                                        /* PETSCII's own DEL byte (20), not ASCII BS/DEL --
                                         * APP_BACKSPACE_DEL_SWAPPED is an ASCII-only concept and
                                         * does not apply here. */
                                        OutKey((unsigned char)petscii_translate_key(PETSCII_KEY_DEL, 1));
                                    else if (PETSCII_SESSION() && conbuf[i] != '\r')
                                        OutKey((unsigned char)petscii_translate_key(conbuf[i], 0));
                                    else
                                        OutKey(conbuf[i]);
                                    if(conbuf[i] == '\r' && STATE_IS(APP_RETURN_SENDING_CRLF))
                                        OutKey('\n');
                                }
                            }
                        }
                    }
                }
            }
            break;


        case IDCMP_MENUPICK:
        {
            UWORD menuNumber = code;
            struct MenuItem *item = NULL;
            UWORD nextMenuNumber = MENUNULL;
            enum MenuItemID menuID;
            // What the user changes by hand while connected to an entry
            // stays for the rest of the run, but is not saved (below).
            handBaseline = prefs;
            LEDs();

            while (menuNumber != MENUNULL)
            {
                item = ItemAddress(mainMenuStrip, menuNumber);

                if (item == NULL)
                    break;

                // Save this before a handler potentially rebuilds or frees the menu
                nextMenuNumber = item->NextSelect;

                menuID = (enum MenuItemID)(ULONG)GTMENUITEM_USERDATA(item);

                switch (menuID)
                {
                    UWORD oldDepth;
                    ULONG oldDispID;
                    ULONG reqtoolsTags[5]; // for rtGetLongA()

                    case MENU_ABOUT:
                        InfoReq(win,
                            "DCTelnet - A classic Amiga Telnet/BBS client with Zmodem"        "\n"
                            "                  v"DCTELNET_VERSION " (build " STR(BUILD_HASH) ")\n"
                            "         Last Compiled .... : "__DATE__""                        "\n"
                            "         First Compiled ... : May 17 1997"                       "\n"
                            "         Compilers Used ... : "STR(COMPILER_STRING)              "\n"
                                                                                              "\n"
                            "            Original author : ZED^DC"                            "\n"
                                                                                              "\n"
                            "            Recompiled by   : Bruno FREDERIC"                    "\n"
                                                                                              "\n"
                            "                   More info/sources:"                           "\n"
                            "           github.com/bruno-frederic/dctelnet"                   "\n");
                        break;

                    case MENU_SCROLLBACK_WIN:
                        if(wwin != scrollbackWin)
                        {
                            CloseScrollBack();
                            OpenScrollBack(lasttop);
                        }
                        break;

                    case MENU_ICONIFY:
                        shouldIconify = TRUE;
                        break;

                    case MENU_DISPLAY_SPEED_TEST:
                        SpeedTest();
                        break;

                    case MENU_FINGER:
                        WindowSub(Finger);
                        break;

                    case MENU_RESET_SCREEN:
                        shouldRestart = TRUE;
                        shouldReopenScreen = TRUE;
                        break;

                    case MENU_QUIT:
                        shouldQuitApp = TRUE;
                        break;

                    case MENU_UPLOAD:
                        if(isConnected)  Upload(prefs.XferLibrary);
                        else             SimpleReq("You better connect first.");
                        break;

                    case MENU_DOWNLOAD:
                        if(isConnected)  Download(prefs.XferLibrary);
                        else             SimpleReq("You better connect first.");
                        break;

                    case MENU_ASCII_SEND:
                        if(isConnected)
                        {
                            fbuf[0] = '\0';
                            if (FileRequester(win,
                                              prefs.UploadPath, sizeof(prefs.UploadPath),
                                              fbuf, sizeof(fbuf),
                                              "#?",
                                              FILEREQ_LOAD))
                            {
                                register long r;
                                strlcpy(buf, prefs.UploadPath, sizeof(buf));
                                AddPart(buf, fbuf, sizeof(buf));
                                SimpleReq(buf);
                                fileHandle = Open(buf, MODE_OLDFILE);
                                if(fileHandle)
                                {
                                    while(r = Read(fileHandle, buf, sizeof buf))
                                    {
                                        register long i;
                                        for(i=0; i<r; i++)
                                        {
                                            if(buf[i] == '\n' && buf[i+1] != '\r' && buf[i-1] != '\r') buf[i] = '\r';
                                        }
                                        TCPSend(buf, r);
                                    }
                                    Close(fileHandle);
                                }
                            }
                        }
                        else
                        {
                            SimpleReq("You better connect first.");
                        }
                        break;

                    case MENU_CONNECT:
                        OnConnectClicked(FALSE);
                        break;

                    case MENU_CONNECT_NEW_INSTANCE:
                        OnConnectClicked(TRUE); // spawn a new DCTelnet instance
                        break;

                    case MENU_DISCONNECT:
                        DisConnect(FALSE, FALSE);
                        break;

                    case MENU_ADDRESS_BOOK:
                        WindowSub(AddressBook);
                        break;

                    case MENU_INFORMATION:
                        WindowSub(Information);
                        break;

                    case MENU_FULLSCREEN:
                        UpdatePrefsFromMenu(item, APP_FULLSCREEN);
                        shouldRestart = TRUE;
                        shouldReopenScreen = TRUE;
                        break;

                    case MENU_LEDS:
                        UpdatePrefsFromMenu(item, APP_LEDS_ENABLED);
                        if(item->Flags & CHECKED)
                        {
                            LEDs();
                        }
                        else
                        {
                            if(STATE_IS(APP_TITLE_BAR_ENABLED))
                            {
                                SetAPen(&scr->RastPort, drawInfo->dri_Pens[BARBLOCKPEN]);
                                RectFill(&scr->RastPort, scr->Width-86, 2, scr->Width-60, BAR_TEXT_HEIGHT-1);
                            }
                        }
                        break;

                    case MENU_TITLE_BAR:
                        UpdatePrefsFromMenu(item, APP_TITLE_BAR_ENABLED);
                        shouldRestart = TRUE;
                        shouldReopenScreen = TRUE;
                        break;

                    // case MENU_INCOMING_LF_TO_CRLF:
                    //     UpdatePrefsFromMenu(item, APP_INCOMING_LF_TO_CRLF);
                    //     break;

                    case MENU_BACKSPACE_DEL_SWAP:
                        UpdatePrefsFromMenu(item, APP_BACKSPACE_DEL_SWAPPED);
                        break;

                    case MENU_SCROLLBACK:
                        UpdatePrefsFromMenu(item, APP_SCROLLBACK_ENABLED);
                        break;

                    case MENU_PACKET_WINDOW:
                        UpdatePrefsFromMenu(item, APP_PACKET_WINDOW_ENABLED);
                        if (STATE_IS(APP_FULLSCREEN))
                            shouldRestart = TRUE;
                        else
                            SimpleReq("Packet Window cannot work in Workbench mode.");
                        break;

                    case MENU_TOOL_BAR:
                        UpdatePrefsFromMenu(item, APP_TOOL_BAR_ENABLED);
                        if (STATE_IS(APP_FULLSCREEN))
                            shouldRestart = TRUE;
                        else
                        {
                            if(item->Flags & CHECKED)
                                OpenToolBarWindow(TRUE);
                            else
                                CloseToolBarWindow();
                        }
                        break;

                    case MENU_RETURN_SENDING_CRLF:
                        UpdatePrefsFromMenu(item, APP_RETURN_SENDING_CRLF);
                        break;

                    case MENU_LOCAL_ECHO:
                        UpdatePrefsFromMenu(item, APP_LOCAL_ECHO);
                        break;

                    case MENU_RAW_CONNECTION:
                        UpdatePrefsFromMenu(item, APP_RAW_CONNECTION);
                        break;

                    case MENU_FAST_SCROLL:
                        #ifdef _DEBUG
                            // This item must be disabled when ibmcon.device is not in use.
                            if (STATE_IS_NOT(APP_RENDERER_IBMCON_DEVICE))
                                SimpleReq("Error: unexpected MENU_JUMP_SCROLL path!" );
                        #endif
                        UpdatePrefsFromMenu(item, APP_FAST_SCROLL_ENABLED);
                        shouldRestart = TRUE;
                        break;

                    case MENU_BUILTIN_RENDERER:
                        if (STATE_IS_NOT(APP_RENDERER_BUILTIN))
                        {
                            ULONG was = prefs.State & APP_RENDERER_ALL;

                            // Update Prefs State bits:
                            STATE_UNSET(APP_RENDERER_ALL);
                            STATE_SET(APP_RENDERER_BUILTIN);

                            // Changing the pen mapping requires reopening the screen,
                            // because Intuition only applies SA_Pens during screen creation
                            // (was: the renderer being left).
                            shouldReopenScreen = (was & (APP_RENDERER_CONSOLE_DEVICE
                                                         | APP_RENDERER_IBMCON_DEVICE)) != 0;

                            // A screen mode chosen for another renderer (a 256-colour
                            // ibmcon screen) is not one the built-in renderer can draw:
                            // it draws 4 bitplanes, so pen 0 was not ANSI black.
                            if (STATE_IS(APP_FULLSCREEN) && !Prefs_ScreenFits(&prefs))
                            {
                                ULONG oldID = prefs.DisplayID;
                                UWORD oldW = prefs.DisplayWidth, oldH = prefs.DisplayHeight;
                                UWORD oldD = prefs.DisplayDepth;

                                if (ChooseScreen() && Prefs_ScreenFits(&prefs))
                                    shouldReopenScreen = TRUE;
                                else
                                {
                                    // (the requester wrote its choice into prefs: undone)
                                    prefs.DisplayID = oldID;
                                    prefs.DisplayWidth = oldW;
                                    prefs.DisplayHeight = oldH;
                                    prefs.DisplayDepth = oldD;
                                    SimpleReq("The built-in renderer needs a 640 pixel wide\n"
                                              "screen of 200 to 256 lines with 16 colours.");
                                    STATE_UNSET(APP_RENDERER_ALL);
                                    STATE_SET(was);
                                    shouldReopenScreen = FALSE;
                                }
                            }
                            shouldRestart = TRUE;       // (also puts the menu check back)
                        }
                    break;

                    case MENU_CONSOLE_DEVICE:
                        if (STATE_IS_NOT(APP_RENDERER_CONSOLE_DEVICE))
                        {
                            ULONG was = prefs.State & APP_RENDERER_ALL;

                            // Update Prefs State bits:
                            STATE_UNSET(APP_RENDERER_ALL);
                            STATE_SET(APP_RENDERER_CONSOLE_DEVICE);

                            shouldReopenScreen = (was & (APP_RENDERER_BUILTIN
                                                         | APP_RENDERER_XEM_LIB)) != 0;
                            shouldRestart = TRUE;
                        }
                    break;

                    case MENU_XEM_LIBRARY:
                        if (prefs.XemLibrary[0] == '\0')
                        {
                            SimpleReq("No XEM library has been selected yet.\n"
                                      "Please choose one first from the Settings menu.");
                        }
                        else
                        {
                            ULONG was = prefs.State & APP_RENDERER_ALL;

                            // Update Prefs State bits:
                            STATE_UNSET(APP_RENDERER_ALL);
                            STATE_SET(APP_RENDERER_XEM_LIB);

                            shouldReopenScreen = (was & (APP_RENDERER_CONSOLE_DEVICE
                                                         | APP_RENDERER_IBMCON_DEVICE)) != 0;
                        }

                        // Restart even when prefs.displaydriver[0] == '\0', this forces a menu
                        // refresh so the CHECKED state of the item is properly reverted :
                        shouldRestart = TRUE;
                        break;

                    case MENU_IBMCON_DEVICE:
                        if (STATE_IS_NOT(APP_RENDERER_IBMCON_DEVICE))
                        {
                            ULONG was = prefs.State & APP_RENDERER_ALL;

                            // Update Prefs State bits:
                            STATE_UNSET(APP_RENDERER_ALL);
                            STATE_SET(APP_RENDERER_IBMCON_DEVICE);

                            shouldReopenScreen = (was & (APP_RENDERER_BUILTIN
                                                         | APP_RENDERER_XEM_LIB)) != 0;
                            shouldRestart = TRUE;
                        }
                    break;

                    case MENU_PETSCII_MODE:
                        UpdatePrefsFromMenu(item, APP_PETSCII_MODE);
                        petscii_dispatch_init(&g_petsciiState, 40, 25);
                        /* The C64 display is up only during a connection. Toggled
                         * while connected, the display reopens (the console fixes its
                         * cell size at OpenDevice() time, as for MENU_SCREEN_FONT);
                         * toggled while disconnected, nothing visible changes. */
                        SyncPetsciiDisplay();
                        break;

                    case MENU_SCREEN_MODE:
                        oldDispID = prefs.DisplayID;
                        oldDepth  = prefs.DisplayDepth;
                        if (ChooseScreen()
                            && ((oldDispID  != prefs.DisplayID) || (oldDepth != prefs.DisplayDepth))
                           )
                        {
                            shouldRestart = TRUE;
                            shouldReopenScreen = TRUE;
                        }
                        break;

                    case MENU_SCREEN_FONT:
                    {
                        // The same reopen as an entry's font: on the Workbench
                        // the console only, and a window at the BBS size stays
                        // 80x25 in the new font (a 7x11 font came back in the
                        // old 8x16 window: 91x36, cut to 80 columns).
                        static struct PrefsStruct before;

                        before = prefs;
                        if (FontRequester(win,
                                          prefs.FontName, sizeof(prefs.FontName),
                                          &prefs.FontSize))
                            RequestDisplayReopen(&before);
                        break;
                    }

                    case MENU_SCREEN_PALETTE:
                        ChoosePalette();
                        break;

                    case MENU_DOWNLOAD_PATH:
                        DirectoryRequester(win,
                                           prefs.DownloadPath, sizeof(prefs.DownloadPath));
                        break;

                    case MENU_TRANSFER_PROTOCOL:
                        FileRequester(win,
                                      "LIBS:", 0, // 0 because we don't want to get the dirname
                                      prefs.XferLibrary, sizeof(prefs.XferLibrary),
                                      "xpr#?.library",
                                      FILEREQ_LOAD);
                        break;

                    case MENU_PROTOCOL_OPTIONS:
                        GetStringRequester(win,
                                                    "XPR Protocol Options..",
                                                    "Options string:",
                                                    prefs.XferOptions, sizeof(prefs.XferOptions));
                        // TODO Open XPR options Dialog : XferOptions(prefs.XferLibrary);
                        break;

                    case MENU_FUNCTION_KEYS:
                        WindowSub(FunctionKeys);
                        break;

                    case MENU_XEM_LIBRARY_PATH:
                        if (FileRequester(win,
                                          "LIBS:", 0, // 0 because we don't want to get the dirname
                                          prefs.XemLibrary, sizeof(prefs.XemLibrary),
                                          "xem#?.library",
                                          FILEREQ_LOAD))
                        {
                            if (STATE_IS(APP_RENDERER_XEM_LIB)) shouldRestart = TRUE;
                        }
                        break;

                    case MENU_XEM_LIB_OPTIONS:
                        if (xemIO)
                        {
                            XEmulatorOptions(xemIO);
                            // Saved as the global options unless an entry's own Terminal
                            // settings are live: then the change is session-only.
                            if (!(sessionSettingsId && (sessionGroups & SITE_GROUP_TERMINAL)))
                                SaveXemOptions(XEM_GLOBAL_OPTIONS);
                        }
                        else
                            InfoReq(win, "The XEM library is currently "
                                           "disabled, so related functionality is unavailable.");
                        break;

                    case MENU_TELNET_TERM_TYPE:
                        GetStringRequester(win,
                                           "Telnet Terminal Type",
                                           "Terminal type sent to telnet server:",
                                           prefs.TelnetTermType, sizeof(prefs.TelnetTermType));
                        break;

                    case MENU_SCROLLBACK_LINES:
                        InitializeReqToolsLib(reqtoolsTags);
                        rtGetLongA(&prefs.nScrollbackLines, "ScrollBack Lines..", NULL, (struct TagItem *)&reqtoolsTags);
                        break;

                    case MENU_SNAPSHOT_WINDOWS:
                        if (STATE_IS_NOT(APP_FULLSCREEN))   // this size, not the 80x25 BBS default
                            STATE_SET(APP_WINDOW_SNAPSHOT);
                        prefs.MainWinTopEdge  = win->TopEdge;
                        prefs.MainWinLeftEdge = win->LeftEdge;
                        prefs.MainWinHeight   = win->Height;
                        prefs.MainWinWidth    = win->Width;

                        if(scrollbackWin)
                        {
                            prefs.ScrollbackWinLeftEdge = scrollbackWin->LeftEdge;
                            prefs.ScrollbackWinTopEdge  = scrollbackWin->TopEdge;
                            prefs.ScrollbackWinWidth    = scrollbackWin->Width;
                            prefs.ScrollbackWinHeight   = scrollbackWin->Height;
                        }
                        if (toolBarWin)
                        {
                            prefs.ToolBarWinLeftEdge    = toolBarWin->LeftEdge;
                            prefs.ToolBarWinTopEdge     = toolBarWin->TopEdge;
                        }

                        break;

                    case MENU_SAVE_ENTRY_SETTINGS:
                        switch (SaveSettingsToConnectedEntry())
                        {
                            case SAVE_ENTRY_NOT_CONNECTED:
                                InfoReq(win,
                                        "Connect to an Address Book entry first.");
                                break;
                            case SAVE_ENTRY_NOTHING_CHANGED:
                                InfoReq(win,
                                        "Nothing was changed since connecting,\n"
                                        "so there is nothing to save to this entry.");
                                break;
                            case SAVE_ENTRY_WRITE_ERROR:
                                InfoReq(win,
                                        "Could not save the settings to PROGDIR:Sites.");
                                break;
                            default:
                                break;
                        }
                        break;

                    case MENU_SEND_USERNAME:
                        SendMisc(username, -1);
                        SendMisc("\r", 1);
                        break;

                    case MENU_SEND_PASSWORD:
                        SendMisc(password, -1);
                        SendMisc("\r", 1);
                        break;

                    default:
                        break;
                }

                menuNumber = nextMenuNumber;
            } // while
            // Changed while connected to an entry: kept for this run, not saved.
            SitePrefs_HandChange(&handChanges, &globalPrefs, &handBaseline, &prefs,
                                 sessionSettingsId != 0);
            break;
        }  // case IDCMP_MENUPICK


        case IDCMP_CLOSEWINDOW:
            if(wwin == win) shouldQuitApp = TRUE;
            if(wwin == scrollbackWin) close = TRUE;
            if(wwin == toolBarWin) shouldCloseToolbarWin = TRUE;
            break;


        case IDCMP_IDCMPUPDATE:
            switch((UWORD)GetTagData(GA_ID, 0, (struct TagItem *)gad))
            {
            case GAD_UP:
up:                if(lasttop > 0) lasttop--;
                break;

            case GAD_DOWN:
down:                if(lasttop+((scrollbackWin->Height - (scr->Font->ta_YSize + scr->WBorTop + 2)) / scr->Font->ta_YSize) < nScrollbackLines) lasttop++;
                break;
            }
            SetGadgetAttrs((struct Gadget *)Scroller, scrollbackWin, NULL,
                PGA_Top,    lasttop,
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
            PGA_Visible,    (scrollbackWin->Height - (scr->Font->ta_YSize + scr->WBorTop + 2)) / scr->Font->ta_YSize,
        TAG_END);
    }
    if(close) CloseScrollBack();
    if(shouldCloseToolbarWin)
    {
        CloseToolBarWindow();
        STATE_UNSET(APP_TOOL_BAR_ENABLED);
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
static struct Task *connectingWindowTask;

// This flag is set by the "Connecting..." window task when the user cancels the operation:
BOOL isConnectionAborted;
UWORD connectMsgType;
char *connectString;

static UWORD EstablishTCPConnection(char *servername, UWORD port);

/**
 * @brief Establishes a TCP connection while displaying a connection progress window.
 *
 * Spawns a dedicated task responsible for displaying and updating the "Connecting..." window during
 * the connection attempt. The function then performs the actual TCP connection through
 * EstablishTCPConnection().
 *
 * Once the connection attempt completes, the UI task is signaled to terminate and the function
 * waits for its acknowledgement before returning.
 *
 * @param servername Hostname or IP address of the remote server.
 * @param port TCP port number to connect to.
 *
 * @return Connection result returned by EstablishTCPConnection().
 */
UWORD BeginServerConnection(char *servername, UWORD port)
{
    UWORD ret;

    // In V36 (AmigaOS 2.00 & 2.02), NP_Arguments was broken in a number of ways, and probably
    // should be avoided.
    connectingWindowTask = (struct Task *) CreateNewProcTags(NP_Entry, HandleConnectingWindowTask,
                                                             TAG_DONE);

    ret = EstablishTCPConnection(servername, port);

    // the UI task is signaled to terminate and the function waits for its acknowledgement.
    Signal(connectingWindowTask, SIGBREAKF_CTRL_C);

    Wait(SIGBREAKF_CTRL_E);

    // Check & clear CTRL_C signal
    while((SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C))
    { }

    return ret;
}

// Notify HandleConnectingWindowTask that a new message should be displayed
static void UpdateConnectingWindowMessage(char *msg, UWORD type)
{
    connectString = msg;
    connectMsgType = type;
    Signal(connectingWindowTask, SIGBREAKF_CTRL_E);

    // Wait for HandleConnectingWindowTask to acknowledge the signal
    Wait(SIGBREAKF_CTRL_E);
}

static UWORD EstablishTCPConnection(char *servername, UWORD port)
{
    struct hostent *hostAddr;
    char strIPAddress[16];    // 16 : 255.255.255.255 + '\0'
    char strOfficialName[20]; // Limited by the "Connecting" window field

    if(!SocketBase) SocketBase = OpenLibrary("bsdsocket.library", 0);

    if(!SocketBase)
    {
        SimpleReq("bsdsocket.library can not be loaded.\n\n"
                    "You must start the TCP/IP stack first.");
        return(255);
    }

    //  Draw connection activity indicator when Title bar AND LEDs are enabled
    if (STATE_ARE_ALL(APP_TITLE_BAR_ENABLED | APP_LEDS_ENABLED))
    {
        SetAPen(&scr->RastPort, LegacyPen(11));
        RectFill(&scr->RastPort, scr->Width-84, 3, scr->Width-76, BAR_TEXT_HEIGHT-2);
    }

    DisConnect(FALSE, FALSE);

    UpdateConnectingWindowMessage("Looking up...", 4);
    UpdateConnectingWindowMessage(servername, 0);

    LocalFmt("\r\nLooking up ›32m%s›m... ", servername);

    if (servername != server)
        strlcpy(server, servername, sizeof(server));
    #ifdef _DEBUG
    else  SimpleReq("servername == global server variable!!! could crash strcpy()");
    #endif

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

    #ifdef _DEBUG
        Printf("<-- gethostbyname(%s) => type=%ld, len=%ld\r\n",
                server,
                hostAddr->h_addrtype,
                hostAddr->h_length);
    #endif

    // Sanity check: expected IPv4 response; protects against unexpected resolver data.
    if (   hostAddr->h_addrtype != AF_INET
        || hostAddr->h_length   != sizeof(inetSocketAddr.sin_addr))
    {
        LocalPrint("Host lookup returned an unsupported address type.\r\n");
        LEDs();
        return(1);
    }

    memset(&inetSocketAddr, 0, sizeof(inetSocketAddr));
    inetSocketAddr.sin_len = sizeof(inetSocketAddr);
    inetSocketAddr.sin_family = AF_INET;
    inetSocketAddr.sin_port = htons(port);
    memcpy(&inetSocketAddr.sin_addr, hostAddr->h_addr, hostAddr->h_length);

    strlcpy(strIPAddress, Inet_NtoA(inetSocketAddr.sin_addr.s_addr), sizeof(strIPAddress));
    strlcpy(strOfficialName, hostAddr->h_name, sizeof(strOfficialName));

    LocalFmt("Found ›36m%s›m (official name: ›32m%s)›m\r\n", strIPAddress, strOfficialName);

    UpdateConnectingWindowMessage(strIPAddress, 1);
    UpdateConnectingWindowMessage(strOfficialName, 2);

    LocalFmt("Connecting to ›36m%s›m port ›35m%ld›m...\r\n", strIPAddress, port);

    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(tcpSocket == -1)
    {
        LocalPrint("Cannot Open Socket.\r\n");
        LEDs();
        return(2);
    }

    UpdateConnectingWindowMessage("Connecting...", 4);

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

    ResetTelnetContext();
    ResetZmodemContext();

    if (STATE_IS(APP_FULLSCREEN))
        ScreenToFront(scr);
    else
        WindowToFront(win);

    conectionTime = mytime();

    /* Fresh C64 screen per connection. Without this a start with PETSCII
     * mode already saved in prefs ran on a zeroed state (cols = 0), so
     * every printable byte "wrapped" and got its own line. */
    petscii_dispatch_init(&g_petsciiState, 40, 25);
    if (PETSCII_SESSION() && !petsciiFont)
        LocalPrint("\r\nPETSCII Mode: Petscii.font not found in FONTS: or PROGDIR:Fonts/, "
                   "showing CP437 lookalikes.\r\n");

    Dsr_Init(&bbsDsr);      // a request cut off by the last disconnect is not this BBS's
    isConnected = TRUE;

    LEDs();

    return(0);
}


/**
 * @brief Open a font DCTelnet ships (Petscii, PetsciiLower, TopazPro) from
 *        FONTS:, else from the Fonts drawer next to the program.
 *
 * The release archive carries them in DCTelnet/Fonts/, and not every user copies them into
 * FONTS:. diskfont.library accepts a path in ta_Name.
 */
static struct TextFont *OpenBundledFont(STRPTR name, STRPTR progdirPath, UWORD ysize)
{
    struct TextAttr attr;
    struct TextFont *font;

    attr.ta_Name  = progdirPath;     // DCTelnet's own drawer first
    attr.ta_YSize = ysize;
    attr.ta_Style = FS_NORMAL;
    attr.ta_Flags = 0;
    font = OpenDiskFont(&attr);
    if (!font)
    {
        attr.ta_Name = name;         // then FONTS:
        font = OpenDiskFont(&attr);
    }
    return font;
}

/**
 * @brief Open the C64 fonts when the display should be the C64 one (a PETSCII
 *        session on a console device), and record which display this is. The
 *        built-in renderer and XEM draw with their own glyphs: there the
 *        PETSCII stream is translated to ANSI (Receive) and no C64 font opens.
 */
static void OpenPetsciiFonts(void)
{
    displayIsPetscii = PETSCII_SESSION() != 0;
    if (displayIsPetscii && STATE_IS(APP_RENDERER_IBMCON_DEVICE | APP_RENDERER_CONSOLE_DEVICE))
    {
        /* Petscii/PetsciiLower.font: real C64 glyphs indexed by raw PETSCII
         * byte, double-width (16x8) cells so 40 columns fill roughly the
         * physical width the normal 80-column font needs. Not installed:
         * both stay NULL and Receive() renders CP437 lookalikes instead. */
        petsciiFont = OpenBundledFont("Petscii.font", "PROGDIR:Fonts/Petscii.font", 8);
        petsciiFontLower = petsciiFont
            ? OpenBundledFont("PetsciiLower.font", "PROGDIR:Fonts/PetsciiLower.font", 8) : NULL;
    }
}

static void ClosePetsciiFonts(void)
{
    if(petsciiFont)           { CloseFont(petsciiFont);             petsciiFont = NULL; }
    if(petsciiFontLower)      { CloseFont(petsciiFontLower);        petsciiFontLower = NULL; }
}

// The ANSI colours for ibmcon on the Workbench: 16 shared pens of exactly
// their colours from its palette (V39), given back with ReleaseWorkbenchPens().
static void ObtainWorkbenchPens(void)
{
    UWORD i;

    if (!STATE_IS(APP_RENDERER_IBMCON_DEVICE) || STATE_IS(APP_FULLSCREEN)
        || GfxBase->LibNode.lib_Version < 39)
        return;
    for (i = 0; i < 16; i++)
    {
        ULONG c = Palette_AnsiColour(prefs.DeviceColors, i);
        ULONG r = ((c >> 16) & 0xFF) * 0x01010101UL, g = ((c >> 8) & 0xFF) * 0x01010101UL,
              bl = (c & 0xFF) * 0x01010101UL;
        LONG pen = ObtainBestPen(scr->ViewPort.ColorMap, r, g, bl,
                                 OBP_Precision, PRECISION_EXACT, TAG_DONE);

        // No pen to share (-1): the nearest colour already there, which
        // is not ours to release. -1 cast to UBYTE was pen 255.
        wbPenOwned[i] = pen >= 0;
        if (pen < 0)
            pen = FindColor(scr->ViewPort.ColorMap, r, g, bl, -1);
        ansiColourPens[i] = (UBYTE)pen;
    }
    wbPensObtained = TRUE;
    ansiOwnPens = TRUE;
}

static void ReleaseWorkbenchPens(void)
{
    UWORD i;

    if (!wbPensObtained)
        return;
    for (i = 0; i < 16; i++)
        if (wbPenOwned[i])
            ReleasePen(scr->ViewPort.ColorMap, ansiColourPens[i]);
    wbPensObtained = FALSE;
    ansiOwnPens = FALSE;
}

// ibmcon 1.5: the ANSI colours' own pens. An older ibmcon answers
// IOERR_NOCMD and keeps pens 0-15. Also sent to an open console.
static void SendPenTable(void)
{
    if (!ansiOwnPens || !STATE_IS(APP_RENDERER_IBMCON_DEVICE))
        return;
    writeConsoleReq->io_Command = IBMCMD_SETPENS;
    writeConsoleReq->io_Data    = ansiColourPens;
    writeConsoleReq->io_Length  = sizeof(ansiColourPens);
    DoIO((struct IORequest *)writeConsoleReq);
    penTableError = writeConsoleReq->io_Error;
}

// New ANSI colours on the Workbench without reopening the console (which
// would clear it): new shared pens, handed to the running ibmcon.
static void RenewWorkbenchPens(void)
{
    if (!isConDeviceOpened || !wbPensObtained)
        return;
    ReleaseWorkbenchPens();
    ObtainWorkbenchPens();
    SendPenTable();
}

/**
 * @brief Open the renderer's console device (ibmcon.device, or console.device)
 *        on the main window, with the RastPort's current font.
 */
static BOOL OpenConsoleDevice(void)
{
    #ifdef _DEBUG
        ULONG beforeSigAlloc;
        ULONG afterSigAlloc;
        UBYTE conDeviceSigBit;
    #endif
    UWORD unitNumber;
    char *devName = STATE_IS(APP_RENDERER_CONSOLE_DEVICE) ? "console.device" : "ibmcon.device";
    BOOL b;

    // Exec Device I/O Functions docs:
    // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node02A5.html

    // CreateIORequest() requires a message port.
    writeConsoleMP = CreateMsgPort();
    if (!writeConsoleMP) { InfoReq(win,
                                 "Unable to create message port for console device!");
                         return FALSE; }

    // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0344.html
    writeConsoleReq = CreateIORequest(writeConsoleMP, sizeof(struct IOStdReq));
    if (!writeConsoleReq)       // the console is reopened at run time too
    {
        DeleteMsgPort(writeConsoleMP);
        writeConsoleMP = NULL;
        InfoReq(win, "Unable to create the console device request!");
        return FALSE;
    }

    // The unit number that is a standard parameter for an open call is used
    // specially by this device.
    if (STATE_IS(APP_RENDERER_CONSOLE_DEVICE))
    {
        unitNumber = CONU_SNIPMAP;
    } else {
        if(STATE_IS(APP_FAST_SCROLL_ENABLED))
            unitNumber = 2; // Unit 2 is a non-standard unit specific to ibmcon.device
        else
            unitNumber = CONU_CHARMAP;
    }

    // ibmcon on the Workbench: the ANSI colours take 16 shared pens of exactly
    // their colours from its palette (V39); they go back in CloseConsoleDevice().
    ObtainWorkbenchPens();

    //the window that is used by the console device for output:
    writeConsoleReq->io_Data = win;
    writeConsoleReq->io_Length = sizeof(struct Window);

    #ifdef _DEBUG
        PutStr("   --> OpenDevice()\n");
        beforeSigAlloc = mainTask->tc_SigAlloc;
        PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
        LogWindowsSigBit();
    #endif

    b = OpenNewestDevice(devName, unitNumber, (struct IORequest *)writeConsoleReq, CONFLAG_DEFAULT);
    consoleFrom = ShippedFrom;      // for the start-up line

    #ifdef _DEBUG
        PutStr("   <-- OpenDevice()\n");
        PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
        afterSigAlloc = mainTask->tc_SigAlloc;
        conDeviceSigBit = BitPosition(beforeSigAlloc ^ afterSigAlloc); // XOR help detect the difference
        Printf("                   conDeviceSigBit = %lu\n", (LONG) conDeviceSigBit);
        LogWindowsSigBit();
    #endif

    if(b == RETURN_OK)
    {
        isConDeviceOpened = TRUE;
        LimitTerminalWidth();       // the font is final now

        SendPenTable();
    }
    else
    {
        // Device open failed; falling back to console.device for the next DCTelnet launch.
        // console.device is the most compatible renderer.
        STATE_UNSET(APP_RENDERER_ALL);
        STATE_SET(APP_RENDERER_CONSOLE_DEVICE);

        isConDeviceOpened = FALSE;

        InfoReq(win, "Failed to open device: %s", devName);

        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Close the console device and free its request and port (and give
 *        the Workbench back its pens).
 */
static void CloseConsoleDevice(void)
{
    ReleaseWorkbenchPens();

    // https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0190.html
    if (isConDeviceOpened)
    {
        #ifdef _DEBUG
            PutStr("   --> CloseDevice(&writeConsoleReq)\n");
            PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
            LogWindowsSigBit();

            if (! (mainTask->tc_SigAlloc & (1L << 31)))
            {
                InfoReq(win,
                        "ERROR: sigbit 31 has disappeared before CloseDevice()! Why???");
            }
        #endif

        CloseDevice((struct IORequest *)writeConsoleReq);

        if (mainTask->tc_SigAlloc & (1L << 31))
        {
            #ifdef _DEBUG
                PutStr("   <-- CloseDevice(&writeConsoleReq) => sigbit 31 preserved.\n");
                PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
            #endif
        }
        else
        {
            #ifdef _DEBUG
                PutStr("   <-- CloseDevice(&writeConsoleReq) => ERROR: sigbit 31 destroyed!!!\n");
                PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
                PutStr("   --> AllocSignal(31L)\n");
            #endif

            dontUseSig31 = AllocSignal(31L);
            if (dontUseSig31 != 31)
                InfoReq(win, "ERROR: cannot allocate sigbit 31!");

            #ifdef _DEBUG
                PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
            #endif
        }

        isConDeviceOpened = FALSE;
    }

    if (writeConsoleReq)
    {
        DeleteIORequest(writeConsoleReq);
        writeConsoleReq=NULL;
    }

    if (writeConsoleMP)
    {
        DeleteMsgPort(writeConsoleMP);
        writeConsoleMP = NULL;
    }
}

/**
 * @brief Switch between the normal and the C64 display without closing the
 *        screen or the window: the console fixes its cell size at OpenDevice()
 *        time, so only the console is reopened after the fonts change (ibmcon
 *        reads the RastPort font live). The built-in and XEM renderers take
 *        their font at setup: they get the full reopen instead.
 */
static void ReopenTerminal(void)
{
    if (!STATE_IS(APP_RENDERER_IBMCON_DEVICE | APP_RENDERER_CONSOLE_DEVICE) || !win)
    {
        shouldRestart = TRUE;
        shouldReopenScreen = TRUE;
        return;
    }
    {
        struct TextFont *oldAnsi = ansiFont;
        UWORD cols, rows;
        BOOL bbsSize;

        TerminalGrid(&cols, &rows);
        bbsSize = STATE_IS_NOT(APP_FULLSCREEN) && cols == ArtColumns(win->RPort->Font) && rows == SCREENFONT_BBS_ROWS;
        CloseConsoleDevice();
        OpenAnsiFont();                 // the settings' font (an entry's, the global one)
        if (!STATE_IS_NOT(APP_FULLSCREEN))
            LoadAnsiPalette(&prefs);    // the settings' colours on the terminal's pens
        ClosePetsciiFonts();
        OpenPetsciiFonts();
        SetFont(win->RPort, petsciiFont ? petsciiFont : ansiFont);
        if (oldAnsi) CloseFont(oldAnsi);
        if (bbsSize)                    // still the BBS size: stays it in the new font
            SizeWorkbenchWindow(ArtColumns(win->RPort->Font), SCREENFONT_BBS_ROWS);
    }
    if (!OpenConsoleDevice())
    {
        shouldRestart = TRUE;       // could not reopen it in place: reopen everything
        shouldReopenScreen = TRUE;
        return;
    }
    if (displayIsPetscii)
        ConWrite(PETSCII_CONSOLE_SETUP, sizeof(PETSCII_CONSOLE_SETUP) - 1);
    // The grid changed with the font: tell the BBS (on the Workbench the
    // window's resize does it too, on the own screen nothing else would).
    if (isConnected)
        TelnetSendWindowSize();
}

/**
 * @brief Open the terminal's ANSI font for the settings in use (ansiFont,
 *        fontAttr): topaz and Topaz Pro are one face -- Topaz Pro on square
 *        pixels (every RTG mode), topaz on the Amiga's tall ones -- for the
 *        screen (title bar, menus) and the terminal alike.
 */
static void OpenAnsiFont(void)
{
    fontAttr.ta_Name = prefs.FontName;
    fontAttr.ta_YSize = prefs.FontSize;
    ansiFont = NULL;
    {
        struct DisplayInfo di;
        struct ScreenFontChoice pick;
        UWORD resX = 0, resY = 0;
        ULONG modeID = prefs.DisplayID;

        if (STATE_IS_NOT(APP_FULLSCREEN))
        {
            struct Screen *wb = LockPubScreen(NULL);

            modeID = wb ? GetVPModeID(&wb->ViewPort) : INVALID_ID;
            if (wb) UnlockPubScreen(NULL, wb);
        }
        if (modeID != INVALID_ID
            && GetDisplayInfoData(NULL, (UBYTE *)&di, sizeof(di), DTAG_DISP, modeID))
        {
            resX = (UWORD)di.Resolution.x;
            resY = (UWORD)di.Resolution.y;
        }
        modeResX = resX;
        modeResY = resY;
        // The built-in renderer draws 8x8 cells from the font it is given:
        // the font stays as chosen (ValidateAndInitPrefs keeps it at 8).
        if (STATE_IS_NOT(APP_RENDERER_BUILTIN))
        {
            ScreenFont_ForMode((const char *)prefs.FontName, prefs.FontSize, resX, resY, &pick);
            fontAttr.ta_Name  = (STRPTR)pick.name;
            fontAttr.ta_YSize = pick.size;
            if (pick.topazPro)      // bundled: in the font list once open, so OpenScreen finds it
                ansiFont = OpenBundledFont(TOPAZ_PRO_NAME, "PROGDIR:Fonts/" TOPAZ_PRO_NAME, TOPAZ_PRO_SIZE);
        }
    }
    if (!ansiFont)
    {
        // DCTelnet's Fonts drawer first (the fonts it ships), then FONTS:
        static char own[64];

        if (ProgDir_Path("Fonts", (const char *)fontAttr.ta_Name, own, sizeof(own)))
            ansiFont = OpenBundledFont(fontAttr.ta_Name, own, fontAttr.ta_YSize);
        else
            ansiFont = OpenDiskFont(&fontAttr);
    }
    fontMissing[0] = 0;
    if(!ansiFont)
    {
        // The chosen font could not be opened: topaz, in the form the
        // screen's pixels want (Topaz Pro on square ones), and say so.
        struct ScreenFontChoice pick;

        mysprintf(fontMissing, "%s %ld", prefs.FontName, (LONG)prefs.FontSize);
        ScreenFont_ForMode("topaz.font", 8, modeResX, modeResY, &pick);
        if (pick.topazPro)
            ansiFont = OpenBundledFont(TOPAZ_PRO_NAME, "PROGDIR:Fonts/" TOPAZ_PRO_NAME, TOPAZ_PRO_SIZE);
        fontAttr.ta_Name  = ansiFont ? (STRPTR)TOPAZ_PRO_NAME : (STRPTR)"topaz.font";
        fontAttr.ta_YSize = ansiFont ? TOPAZ_PRO_SIZE : 8;
        if (!ansiFont)
            ansiFont = OpenFont(&fontAttr);
    }
}

struct Screen* OpenAppScreen(void)
{
    struct Screen *scr;

    OpenAnsiFont();
    OpenPetsciiFonts();

    if (STATE_IS_NOT(APP_FULLSCREEN))
    {
        STATE_UNSET(APP_LEDS_ENABLED);
        STATE_UNSET(APP_PACKET_WINDOW_ENABLED);
        scr = LockPubScreen(0L);
        STATE_UNSET(APP_CUSTOM_SCREEN_OPENED);
    }
    else
    {
        // Screen colors are set at the time the screen is opened with the SA_Colors tag.
        // If the colors need to be changed after the screen is opened, the graphics library
        // function, LoadRGB4() should be used.
        // To change a single entry in the color table, use SetRGB4() and SetRGB4CM().
        // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node00F2.html

        // Custom pen specification is applied when the screen is opened by providing the SA_Pens
        // tag along with a pointer to a pen array.
        // Do not use SA_DetailPen or SA_BlockPen anymore; SA_Pens replaces them.
        const UWORD *pens;

        const UWORD *colorsRGB4;
        struct ColorSpec colors[17];
        int i;

        UBYTE deepPens[16];
        BOOL  deep = STATE_IS(APP_RENDERER_IBMCON_DEVICE)
                  && Palette_AnsiPens(prefs.DisplayDepth, deepPens);

        if (STATE_IS(APP_RENDERER_BUILTIN | APP_RENDERER_XEM_LIB))
        {
            colorsRGB4 = prefs.AnsiColors;
            pens       = ansiPens;
        }
        else
        {
            colorsRGB4 = prefs.DeviceColors;
            pens       = devicePens;
        }

        pens = (prefs.DisplayDepth < 3) ? defaultPens : pens;
        // ibmcon with 32+ colours: the UI keeps Intuition's own pens and
        // colours, the ANSI colours get pens of their own (in ANSI order).
        if (deep)
            pens = intuitionPens;

        // Convert RGB4 colors array to ColorSpec array needed for SA_Colors during screen open:
        for (i = 0; i < 16; i++) {
            UWORD c = deep ? colorsRGB4[Palette_IbmconToAnsi(i)] : colorsRGB4[i];

            colors[i].ColorIndex = deep ? deepPens[i] : i;
            colors[i].Red   = (c >> 8) & 0xF;
            colors[i].Green = (c >> 4) & 0xF;
            colors[i].Blue  =  c       & 0xF;
        }
        colors[16].ColorIndex = -1; /* -1 terminates an array of ColorSpec	*/
        colors[16].Red = colors[16].Green = colors[16].Blue = 0;

        // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node00E5.html
        scr = OpenScreenTags(NULL,
            SA_Title,         (ULONG) MainWindowTitle,
            SA_Width,         prefs.DisplayWidth,
            SA_Height,        prefs.DisplayHeight,
            SA_DisplayID,     prefs.DisplayID,
            SA_Depth,         prefs.DisplayDepth,
            SA_Colors,        (ULONG) colors,
            SA_Pens,          (ULONG) pens,
            SA_Type,          CUSTOMSCREEN,
            SA_Font,          (ULONG) &fontAttr,
            SA_ShowTitle,     STATE_IS(APP_TITLE_BAR_ENABLED),
            SA_AutoScroll,    TRUE,
            SA_Interleaved,   TRUE,
            SA_SharePens,     TRUE,        // V39: pens beyond the UI stay free
            SA_FullPalette,   TRUE,        // V39: all colours initialised
            TAG_END);

        if (scr == NULL)
            STATE_UNSET(APP_CUSTOM_SCREEN_OPENED);
        else
            STATE_SET(APP_CUSTOM_SCREEN_OPENED);
    }

    // Own screen with 32+ colours and ibmcon: the ANSI colours on pens of their own.
    ansiOwnPens = scr && STATE_IS(APP_FULLSCREEN) && STATE_IS(APP_RENDERER_IBMCON_DEVICE)
               && Palette_AnsiPens(AppScreenDepth(scr), ansiColourPens);

    // Border blank (V39, ECS/AGA): the overscan border shows colour 0, which
    // on 32+ colours is the UI's grey pen 0 -- keep it black like the terminal.
    if (scr && STATE_IS(APP_FULLSCREEN) && GfxBase->LibNode.lib_Version >= 39)
    {
        VideoControlTags(scr->ViewPort.ColorMap, VTAG_BORDERBLANK_SET, TRUE, TAG_DONE);
        MakeScreen(scr);
        RethinkDisplay();
    }
    return scr;
}


/*
 * Opens the main application window. In Workbench mode, it opens a window on the public screen.
 In full screen mode, it creates a backdrop window that covers the entire screen
 (except for the title bar if enabled).

 The global variable 'win" is set to the opened window
 */
void OpenAppWindow(void)
{
    winTop = (scr->WBorTop)+(scr->Font->ta_YSize)+1;
    newWin.Screen = scr;
    newWin.Type = PUBLICSCREEN;
    newWin.DetailPen = 255;
    newWin.BlockPen = 255;

    if (STATE_IS_NOT(APP_FULLSCREEN))
    {
        newWin.LeftEdge   = prefs.MainWinLeftEdge;
        newWin.TopEdge    = prefs.MainWinTopEdge;
        newWin.Width      = prefs.MainWinWidth;
        newWin.Height     = prefs.MainWinHeight;
        if (wbWindowBoxValid)   // a reopen: where the window was, not the snapshot
        {
            newWin.LeftEdge = wbWindowBox.Left;
            newWin.TopEdge  = wbWindowBox.Top;
            newWin.Width    = wbWindowBox.Width;
            newWin.Height   = wbWindowBox.Height;
        }
        newWin.MinWidth   = WIN_MIN_WIDTH;
        newWin.MinHeight  = WIN_MIN_HEIGHT;
        newWin.MaxWidth   = DISP_MAX_WIDTH;
        newWin.MaxHeight  = DISP_MAX_HEIGHT;
        newWin.IDCMPFlags = IDCMP_RAWKEY
                          | IDCMP_CLOSEWINDOW
                          | IDCMP_MENUPICK
                          | IDCMP_NEWSIZE;      // the BBS is told the new text area
        newWin.Flags      = WFLG_GIMMEZEROZERO
                          | WFLG_NEWLOOKMENUS   // Requests new-look menu treatment (V39)
                          | WFLG_SMART_REFRESH  // WFLG_SIMPLE_REFRESH
                          | WFLG_ACTIVATE
                          | WFLG_CLOSEGADGET
                          | WFLG_DRAGBAR
                          | WFLG_DEPTHGADGET
                          | WFLG_SIZEGADGET;
        // Main window title in windowed workbench mode:
        newWin.Title = MainWindowTitle;
        newWin.FirstGadget = 0;

        CheckDimensions(&newWin);

        {
            // No snapshot and not a reopen: the BBS size, 80x25 characters
            // (40x25 in PETSCII Mode) of the terminal's font, on any mode.
            struct TextFont *cell = petsciiFont ? petsciiFont : ansiFont;
            BOOL bbsSize = !wbWindowBoxValid && STATE_IS_NOT(APP_WINDOW_SNAPSHOT);

            win = OpenWindowTags(&newWin, WA_BackFill, (ULONG)&terminalBackFill,
                                 bbsSize ? WA_InnerWidth : TAG_IGNORE,
                                     (ULONG)(ArtColumns(cell) * cell->tf_XSize),
                                 bbsSize ? WA_InnerHeight : TAG_IGNORE,
                                     (ULONG)(SCREENFONT_BBS_ROWS * cell->tf_YSize),
                                 WA_AutoAdjust, TRUE,
                                 TAG_END);
        }

        // Be sure to unlock the public screen when done with it.  Note that once a window is open
        // on the screen the program does not need to hold the screen lock, as the window acts as a
        // lock on the screen.  The pointer to the screen structure is valid as long as a lock on
        // the screen is held by the application, or the application has a window open on the
        // screen (Amiga ROM Kernel Reference Manual, § Accessing a public screen by name)
        UnlockPubScreen(0L, scr);

        if (STATE_IS(APP_TOOL_BAR_ENABLED))
            OpenToolBarWindow(FALSE);
    }
    else  // running in custom full screen
    {
        struct Gadget *backgad;
        UWORD top, height;

        LoadAnsiPalette(&prefs);        // an entry's palette may differ from the screen's

        if (STATE_IS(APP_TOOL_BAR_ENABLED))
            OpenToolBarWindow(FALSE);

        if(STATE_IS_NOT(APP_TITLE_BAR_ENABLED))
        {
            top = 0;
            height = scr->Height;
            backgad = &screenToBackGadget;
            screenToBackGadget.Width = 20;
            screenToBackGadget.Height = 9;
            screenToBackGadget.Activation = RELVERIFY;
            screenToBackGadget.GadgetType = BOOLGADGET;
            screenToBackGadget.LeftEdge = scr->Width - 20;
            screenToBackGadget.GadgetID = GADGET_SCREEN_TO_BACK;
        } else {
            // Below the title bar as the screen draws it: prefs.FontSize is the
            // setting, not the opened font (Topaz Pro 16 for topaz 8), and the
            // first rows went under the bar.
            top = scr->BarHeight + 1;
            height = scr->Height - top;
            backgad = 0;
        }

        if (toolBarWin)    // Tool Window
        {
            top = toolBarWin->TopEdge + toolBarWin->Height + 1;
            height = scr->Height - top;
            if(backgad)
            {
                AddGadget(toolBarWin, backgad, (ULONG) ~0);
                backgad = 0;
            }
        }

        newWin.LeftEdge = 0;
        newWin.Title = 0;
        newWin.Width = scr->Width;

        // The terminal window is 80 columns wide and centred
        // (LimitTerminalWidth): the screen around it is terminal background,
        // ANSI black, not the UI's pen 0 (grey on 32+ colours).
        if (LayersBase)
            InstallLayerInfoHook(&scr->LayerInfo, &terminalBackFill);

        if(STATE_IS(APP_PACKET_WINDOW_ENABLED))
        {
            height -= (scr->Font->ta_YSize + 2);
            strInfo.Buffer = strBuffer;
            strInfo.MaxChars = BUFSIZE;
            strGad.TopEdge = 2;
            strGad.Activation = GACT_RELVERIFY | GACT_STRINGLEFT;
            strGad.GadgetType = GTYP_STRGADGET;
            strGad.SpecialInfo = &strInfo;
            strGad.Width = scr->Width;
            strGad.Height = scr->Font->ta_YSize;

            newWin.TopEdge = top+height;
            newWin.Height = scr->Font->ta_YSize + 2,
            newWin.FirstGadget = &strGad;
            newWin.IDCMPFlags  = IDCMP_MENUPICK
                               | IDCMP_GADGETUP;
            newWin.Flags       = WFLG_NEWLOOKMENUS
                               | WFLG_BORDERLESS
                               | WFLG_BACKDROP;

            packetWin = OpenWindow(&newWin);

            SetAPen(packetWin->RPort, 1);
            Draw(packetWin->RPort, packetWin->Width, 0);
        }

        newWin.TopEdge     = top;
        newWin.Height      = height;
        newWin.FirstGadget = backgad;
        newWin.IDCMPFlags  = IDCMP_GADGETUP
                           | IDCMP_RAWKEY
                           | IDCMP_CLOSEWINDOW
                           | IDCMP_MENUPICK;
        newWin.Flags       = WFLG_SMART_REFRESH
                           | WFLG_NEWLOOKMENUS
                           | WFLG_BORDERLESS
                           | WFLG_ACTIVATE
                           | WFLG_BACKDROP;

        win = OpenWindowTags(&newWin, WA_BackFill, (ULONG)&terminalBackFill, TAG_END);
    }

    SetFont(win->RPort, petsciiFont ? petsciiFont : ansiFont);
}

void CreateAppMenus(void)
{
    register struct MenuItem *item;
    static ULONG ltags[] = {
        GTMN_NewLookMenus, TRUE,  // Required for applications using V39+ new-look menus
        TAG_END
    };
    #ifdef _DEBUG
        BOOL res;
        PutStr("   --> CreateAppMenus()\n");
    #endif

    if (STATE_IS(APP_FULLSCREEN))
    {
        // Reset nm_Flags to GadTools defaults (item becomes enabled).
        GetNewMenuItemFromID(MENU_SCREEN_MODE   )->nm_Flags = 0;
        GetNewMenuItemFromID(MENU_SCREEN_PALETTE)->nm_Flags = 0;
    }
    else
    {
        GetNewMenuItemFromID(MENU_SCREEN_MODE   )->nm_Flags = NM_ITEMDISABLED;
        // The ANSI colours editor works on the Workbench too with ibmcon (OS
        // 3.0): its colours are on shared pens of their own, not the Workbench's.
        GetNewMenuItemFromID(MENU_SCREEN_PALETTE)->nm_Flags =
            (STATE_IS(APP_RENDERER_IBMCON_DEVICE) && GfxBase->LibNode.lib_Version >= 39) ? 0 : NM_ITEMDISABLED;
    }


    GetNewMenuItemFromID(MENU_BUILTIN_RENDERER)->nm_Flags &= ~CHECKED;
    GetNewMenuItemFromID(MENU_CONSOLE_DEVICE  )->nm_Flags &= ~CHECKED;
    GetNewMenuItemFromID(MENU_XEM_LIBRARY     )->nm_Flags &= ~CHECKED;
    GetNewMenuItemFromID(MENU_IBMCON_DEVICE   )->nm_Flags &= ~CHECKED;

    // Disable menu items that are only relevant for specific renderers.
    GetNewMenuItemFromID(MENU_FAST_SCROLL     )->nm_Flags = NM_ITEMDISABLED;
    GetNewMenuItemFromID(MENU_XEM_LIB_OPTIONS )->nm_Flags = NM_ITEMDISABLED;

    if (STATE_IS(APP_RENDERER_BUILTIN))
    {
        GetNewMenuItemFromID(MENU_BUILTIN_RENDERER)->nm_Flags |= CHECKED;
    }
    else if (STATE_IS(APP_RENDERER_CONSOLE_DEVICE))
    {
        GetNewMenuItemFromID(MENU_CONSOLE_DEVICE)->nm_Flags |= CHECKED;
    }
    else if (STATE_IS(APP_RENDERER_XEM_LIB))
    {
        GetNewMenuItemFromID(MENU_XEM_LIBRARY)->nm_Flags |= CHECKED;

        // Enable "XEM Lib Options": setting nm_Flags to 0 restores GadTools' default flags.
        GetNewMenuItemFromID(MENU_XEM_LIB_OPTIONS)->nm_Flags = 0;
    }
    else  // APP_RENDERER_IBMCON_DEVICE
    {
        GetNewMenuItemFromID(MENU_IBMCON_DEVICE)->nm_Flags |= CHECKED;

        GetNewMenuItemFromID(MENU_FAST_SCROLL)->nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
    }

    // The NewMenu item CHECKED flag will be set according to saved Prefs flags. Note: these flags
    // are already set: HIGHCOMP|CHECKIT|MENUTOGGLE for every item in Options menu in mainMenuDesc[]
    SetNewMenuCheckFromPref(MENU_FULLSCREEN,              APP_FULLSCREEN);
    SetNewMenuCheckFromPref(MENU_LEDS,                    APP_LEDS_ENABLED);
    SetNewMenuCheckFromPref(MENU_TITLE_BAR,               APP_TITLE_BAR_ENABLED);
    SetNewMenuCheckFromPref(MENU_BACKSPACE_DEL_SWAP,      APP_BACKSPACE_DEL_SWAPPED);
    SetNewMenuCheckFromPref(MENU_SCROLLBACK,              APP_SCROLLBACK_ENABLED);
    SetNewMenuCheckFromPref(MENU_PACKET_WINDOW,           APP_PACKET_WINDOW_ENABLED);
    SetNewMenuCheckFromPref(MENU_TOOL_BAR,                APP_TOOL_BAR_ENABLED);
    SetNewMenuCheckFromPref(MENU_RETURN_SENDING_CRLF,     APP_RETURN_SENDING_CRLF);
    SetNewMenuCheckFromPref(MENU_LOCAL_ECHO,              APP_LOCAL_ECHO);
    SetNewMenuCheckFromPref(MENU_RAW_CONNECTION,          APP_RAW_CONNECTION);
    SetNewMenuCheckFromPref(MENU_FAST_SCROLL,             APP_FAST_SCROLL_ENABLED);
    SetNewMenuCheckFromPref(MENU_PETSCII_MODE,            APP_PETSCII_MODE);


    // Gadtools CreateMenuA() generates a list of Intuition Menu structs.
    // Menu item text in the screen's menu text pen: without it GadTools
    // uses pen 0, which on 16 colours is ANSI black but on 32+ colours the
    // UI's grey -- every item looked disabled.
    {
        static ULONG ctags[] = { GTMN_FrontPen, 0, TAG_END };

        ctags[1] = drawInfo ? Palette_MenuTextPen(drawInfo->dri_Version, drawInfo->dri_Pens)
                            : Palette_MenuTextPen(0, NULL);
        mainMenuStrip = CreateMenusA(mainMenuDesc, (struct TagItem *)ctags);
    }
    #ifdef _DEBUG
        Printf("   <-- CreateMenusA() => %s\n", (mainMenuStrip != NULL) ? "succeeded" : "failed");
    #endif

    if (mainMenuStrip == NULL) return;

    // Display the "Quit" menu item in red and highlight it with a box on hover.
    item = GetMenuItemFromID(MENU_QUIT);
    if (item != NULL)
    {
        if (prefs.DisplayDepth > 1)
            ((struct IntuiText *)item->ItemFill)->FrontPen = LegacyPen(15);

        item->Flags = (item->Flags & ~HIGHFLAGS) | HIGHBOX;
    }

    ltags[1] = STATE_IS_NOT(APP_FULLSCREEN);

    #ifdef _DEBUG
        PutStr("   --> LayoutMenusA()\n");
        res =
    #endif
    // Gadtools LayoutMenusA() calculates the sizes and locations of the menus and their items:
    LayoutMenusA(mainMenuStrip, visualInfos, (struct TagItem *)&ltags);
    #ifdef _DEBUG
        Printf("   <-- LayoutMenusA() => %s\n", res ? "succeeded" : "failed");
    #endif

    // Intuition SetMenuStrip() add the menu to the window:
    SetMenuStrip(win, mainMenuStrip);
    #ifdef _DEBUG
        Printf("   <-- SetMenuStrip()\n");
    #endif

    if (packetWin)  ResetMenuStrip(packetWin,  mainMenuStrip);
    if (toolBarWin) ResetMenuStrip(toolBarWin, mainMenuStrip);
}


 /**
 * @brief Open or reopen the application's display environment.
 *
 * Screen might already be open if the function is called to restart the UI without changing the
 * screen (Only recreate windows, menus and console bindings)
 *
 * When screen is completely (re)open, the function:
 * - Opens or locks the screen (custom screen or Workbench screen)
 * - Loads the screen font
 * - Allocates visual and drawing resources
 * - Used at program startup, after a screen mode change,
 *
 * @return TRUE on success, FALSE if the display could not be opened.
 */
BOOL OpenDisplay(void)
{
    #ifdef _DEBUG
        PutStr("--> OpenDisplay()\n");
    #endif

    if (scr == NULL)  // We need to (re)open completely the screen
        scr = OpenAppScreen();

    if (scr == NULL) {
        InfoReq(NULL,"Unable to open the screen. Please restart DCTelnet\n"
                     "and select an appropriate screen mode");
        prefs.DisplayID = (ULONG) INVALID_ID;
        SavePrefs();

        goto clean_and_return;
    }

    if (visualInfos == NULL) visualInfos = GetVisualInfoA(scr, NULL);
    if (drawInfo == NULL)    drawInfo    = GetScreenDrawInfo(scr);

    OpenAppWindow();
    if(win == NULL) { InfoReq(NULL,"Unable to open main window!"); goto clean_and_return; }

    CreateAppMenus();
    if (mainMenuStrip == NULL) { InfoReq(win, "Unable to create menus!");
                                 goto clean_and_return; }


    // Try to initialize the XEM library if the user enabled it.
    // If it fails fallback to builtin renderer.
    if(STATE_IS(APP_RENDERER_XEM_LIB))
        if (! InitializeXemLibrary(prefs.XemLibrary))
        {
            struct MenuItem *item = NULL;

            // Update Prefs State bits:
            STATE_UNSET(APP_RENDERER_ALL);
            STATE_SET(APP_RENDERER_BUILTIN);

            // Uncheck the "External XEM Library" option:
            //https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node024A.html
            // https://www.amiga-news.de/en/news/AN-2023-10-00017-EN.html
            ClearMenuStrip(win);

            item = GetMenuItemFromID(MENU_XEM_LIBRARY);
            if (item != NULL)
                item->Flags &= ~CHECKED;

            // Check the "Renderer > Built-in" option:
            item = GetMenuItemFromID(MENU_BUILTIN_RENDERER);
            if (item != NULL)
                item->Flags |= CHECKED;

            ResetMenuStrip(win, mainMenuStrip);
        }


    if (STATE_IS(APP_RENDERER_BUILTIN))
    {
        int res = term_init(scr, ansiFont);

        // If it fails fallback to console.device.
        if (res != RETURN_OK)
        {
            struct MenuItem *item = NULL;

            InfoReq(win,
                    "Failed to setup retro32-term, error # %ld.\nFallback to console.device", res);

            // Update Prefs State bits:
            STATE_UNSET(APP_RENDERER_ALL);
            STATE_SET(APP_RENDERER_CONSOLE_DEVICE);

            ClearMenuStrip(win);

            item = GetMenuItemFromID(MENU_BUILTIN_RENDERER);
            if (item != NULL)
                item->Flags &= ~CHECKED;

            // Check the "Renderer > Built-in" option:
            item = GetMenuItemFromID(MENU_CONSOLE_DEVICE);
            if (item != NULL)
                item->Flags |= CHECKED;

            ResetMenuStrip(win, mainMenuStrip);
        }
    }

    // The console device :
    // https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0080.html

    // Doc about OpenDevice() to open a console device :
    // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node029E.html
    // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0509.html
    if (STATE_IS(APP_RENDERER_IBMCON_DEVICE | APP_RENDERER_CONSOLE_DEVICE) && !OpenConsoleDevice())
        goto clean_and_return;

    STATE_UNSET(APP_ICONIFIED);

    /* After isAppIconified is cleared: ConWrite() drops writes while it is set. */
    if (isConDeviceOpened && displayIsPetscii)
        ConWrite(PETSCII_CONSOLE_SETUP, sizeof(PETSCII_CONSOLE_SETUP) - 1);

    LEDs();

    if(!isConnected)
    {
        STRPTR strRenderer = NULL;
        register ULONG flags = SysBase->AttnFlags;
        LONG cpu = '0';
        ULONG renderer = prefs.State & APP_RENDERER_ALL;

        if(flags & AFF_68010) cpu = '1';
        if(flags & AFF_68020) cpu = '2';
        if(flags & AFF_68030) cpu = '3';
        if(flags & AFF_68040) cpu = '4';
        if(flags & AFF_68060) cpu = '6';

        // One labelled line per fact, each shorter than 80 columns: the
        // renderer, the screen, and the grid the console measured.
        static char details[200];

        details[0] = 0;
        switch (renderer)
        {
            case APP_RENDERER_BUILTIN:
                strRenderer = "retro32-term (built-in)";
            break;

            case APP_RENDERER_XEM_LIB:
                strRenderer = prefs.XemLibrary;
            break;

            case APP_RENDERER_CONSOLE_DEVICE:
            case APP_RENDERER_IBMCON_DEVICE:
            if (isConDeviceOpened)
            {
                static char engine[80];
                char screenInfo[80];
                struct TextFont *cell = win->RPort->Font;
                UWORD cols, rows;
                struct Library *dev = (struct Library *)writeConsoleReq->io_Device;

                if (renderer == APP_RENDERER_IBMCON_DEVICE)
                    mysprintf(engine, "ibmcon.device %ld.%ld from %s",
                              (LONG)dev->lib_Version, (LONG)dev->lib_Revision, consoleFrom);
                else
                    mysprintf(engine, "console.device %ld.%ld",
                              (LONG)dev->lib_Version, (LONG)dev->lib_Revision);
                mysprintf(screenInfo, "%ld bit planes", (LONG)AppScreenDepth(scr));
                if (ansiOwnPens && penTableError)
                    strlcat(screenInfo, ", ANSI colours on pens 0-15 (old ibmcon)", sizeof(screenInfo));
                else if (ansiOwnPens)
                    strlcat(screenInfo, ", ANSI colours on their own pens", sizeof(screenInfo));
                TerminalGrid(&cols, &rows);
                mysprintf(details, "›0;1;36mScreen: ›37m%s\r\n\r\n"
                                   "›36mText area: ›37m%ld x %ld characters, font %ld x %ld\r\n\r\n",
                          screenInfo, (LONG)cols, (LONG)rows, (LONG)cell->tf_XSize, (LONG)cell->tf_YSize);
                if (fontMissing[0])
                {
                    char note[80];

                    mysprintf(note, "›31mFont %s could not be opened\r\n\r\n", fontMissing);
                    strlcat(details, note, sizeof(details));
                }
                strRenderer = engine;
            }
            else
                strRenderer = (renderer == APP_RENDERER_IBMCON_DEVICE) ? "ibmcon.device" : "console.device";
            break;
        }

        LocalFmt("›0;1;36m\f\r\n\r\n"
                "Processor: ›37m680%lc0\r\n\r\n›36m"
                "Kickstart: ›37m%ld.%ld\r\n\r\n›36m"
                "Renderer: ›37m%s\r\n\r\n"
                "%s›36m"
                "TCP Stack: ›37m",
                cpu,
                (LONG)((struct Library *)SysBase)->lib_Version,
                (LONG)SysBase->SoftVer,
                strRenderer, details);

        if(SocketBase)
        {
            register char *po;
            strlcpy(buf, SocketBase->lib_IdString, sizeof(buf));
            // truncate the string at the first line feed:
            po = strchr(buf, '\n');
            if(po) po[0] = '\0';

            LocalPrint(buf);
            LocalPrint("›m\r\n\r\n");
        }
        else
            LocalPrint("›31mNot active›m\r\n\r\n");
    }


    // everything alright:
    return TRUE;


clean_and_return:
    CloseDisplay(TRUE);

    return FALSE;
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
    #ifdef _DEBUG
        PutStr("--> CloseDisplay()\n");
    #endif

    // Unitilize XEM library if it was initialized (does nothing if it was not initialized)
    UninitializeXemLibrary();

    CloseConsoleDevice();

    if(packetWin)
    {
        ClearMenuStrip(packetWin);
        CloseWindow(packetWin);
        packetWin = NULL;
    }

    if(win)
    {
        if (STATE_IS_NOT(APP_FULLSCREEN))      // a reopen puts it back here (Snapshot Windows saves it)
        {
            wbWindowBox.Left   = win->LeftEdge;
            wbWindowBox.Top    = win->TopEdge;
            wbWindowBox.Width  = win->Width;
            wbWindowBox.Height = win->Height;
            wbWindowBoxValid = TRUE;
        }
        ClearMenuStrip(win);
        CloseWindow(win);
        win = NULL;
    }

    CloseScrollBack();
    CloseToolBarWindow();

    if(mainMenuStrip)    { FreeMenus(mainMenuStrip); mainMenuStrip = NULL; }

    if(manageScreen)
    {
        if(visualInfos)           { FreeVisualInfo(visualInfos);        visualInfos = NULL; }
        if(drawInfo)              { FreeScreenDrawInfo(scr, drawInfo);  drawInfo = NULL; }
        if(scr != NULL)
        {
            if (STATE_IS(APP_CUSTOM_SCREEN_OPENED))
            {
                #ifdef _DEBUG
                    BOOL result =
                #endif
                CloseScreen(scr);

                #ifdef _DEBUG
                    PutStr("   <-- CloseScreen()\n");
                    if (! result)  InfoReq(NULL, "ERROR: Failed to close screen!");
                #endif
            }

            scr = NULL;
        }
        if(ansiFont)              { CloseFont(ansiFont);                ansiFont = NULL; }
        ClosePetsciiFonts();
    }

    STATE_SET(APP_ICONIFIED);

    #ifdef _DEBUG
        PutStr("<-- CloseDisplay()\n");
        PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
    #endif
}

// inconify the application
// http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node024A.html
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
            STRPTR s = isConnected ? server : "Disconnected";
            appIconOnWB = AddAppIconA(0, 0, s, iconPort, NULL, diskObj, NULL);
            if (appIconOnWB == NULL)
            {
                FreeDiskObject(diskObj);  diskObj  = NULL;

                DeleteMsgPort(iconPort);  iconPort = NULL;
            }
        }
    }

    #ifdef _DEBUG
        PutStr("  <-- OpenIcon()\n");
        PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
    #endif
}

// shouldUniconifyify the application
// http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node024A.html
void CloseIcon(void)
{
    RemoveAppIcon(appIconOnWB);             appIconOnWB = NULL;
    FreeDiskObject(diskObj);                diskObj  = NULL;
    if (iconPort)
    {
        // Clear away any messages that arrived at the last moment and let Workbench know we're done
        // with the messages (cf. Amiga ROM Kernel Reference Manual v2.04 - Libraries)
        struct Message *msg = NULL;
        while (msg=GetMsg(iconPort))  ReplyMsg(msg);

        DeleteMsgPort(iconPort);
        iconPort = NULL;
    }
}
