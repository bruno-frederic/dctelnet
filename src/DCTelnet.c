/* ====================================================================== */
/* ============================= DC TELNET ============================== */
/* ====================================================================== */


#define DCTELNET_VERSION "1.9.1-dev"
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
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
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
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR0},
    {    NM_ITEM, "Scroll Back",                    "X",             0,               0, (APTR)MENU_SCROLLBACK},
    {    NM_ITEM, "Iconify",                        "&",             0,               0, (APTR)MENU_ICONIFY},
    {    NM_ITEM, "Display Speed Test",             "Y",             0,               0, (APTR)MENU_DISPLAY_SPEED_TEST},
    {    NM_ITEM, "Finger",                         "@",             0,               0, (APTR)MENU_FINGER},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR1},
    {    NM_ITEM, "Reset Screen",                   "C",             0,               0, (APTR)MENU_RESET_SCREEN},
    {    NM_ITEM, "Quit",                           "Q",             0,               0, (APTR)MENU_QUIT},

    { NM_TITLE, "Transfer",  0 , 0, 0, (APTR)MENU_TRANSFER},
    {    NM_ITEM, "Upload",                         "U",             0,               0, (APTR)MENU_UPLOAD},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR2},
    {    NM_ITEM, "Download",                       "D",             0,               0, (APTR)MENU_DOWNLOAD},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR3},
    {    NM_ITEM, "ASCII Send",                     "%",             0,               0, (APTR)MENU_ASCII_SEND},

    { NM_TITLE, "Connection",  0 , 0, 0, (APTR)MENU_CONNECTION},
    {    NM_ITEM, "Connect",                        "M",             0,               0, (APTR)MENU_CONNECT},
    {    NM_ITEM, "Connect (New instance)",         "G",             0,               0, (APTR)MENU_CONNECT_NEW_INSTANCE},
    {    NM_ITEM, "Disconnect",                     "H",             0,               0, (APTR)MENU_DISCONNECT},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR4},
    {    NM_ITEM, "Address Book",                   "B",             0,               0, (APTR)MENU_ADDRESS_BOOK},
    {    NM_ITEM, NM_BARLABEL,                       0 ,             0,               0, (APTR)MENU_BAR5},
    {    NM_ITEM, "Information",                    "^",             0,               0, (APTR)MENU_INFORMATION},

    { NM_TITLE, "Options",  0, 0, 0, (APTR)MENU_OPTIONS},
    {    NM_ITEM, "Use Workbench",                  "W", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_USE_WORKBENCH},
    {    NM_ITEM, "Disable LEDs",                   "I", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_DISABLE_LEDS},
    {    NM_ITEM, "Hide TitleBar",                  "R", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_HIDE_TITLEBAR},
    #ifdef _LEGACY_RECEIVE
        {    NM_ITEM, "CRLF Correction",            "L", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_UNUSED_CRLF},
    #endif
    {    NM_ITEM, "BS/DEL Swap",                    "/", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_BS_DEL_SWAP},
    {    NM_ITEM, "Disable Scroll-B",               "E", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_DISABLE_SCROLLBACK},
    #ifdef _LEGACY_RECEIVE
        {    NM_ITEM, "Strip Colour",               "J", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_STRIP_ANSI_CODES},
        {    NM_ITEM, "Simple Telnet",              "1", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_UNUSED_SIMPLE_TELNET},
    #endif
    {    NM_ITEM, "Packet Window",                  "2", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_PACKET_WINDOW},
    {    NM_ITEM, "Use XEM Library",                "3", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_USE_XEM_LIBRARY},
    {    NM_ITEM, "Tool Bar",                       "4", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_TOOLBAR},
    {    NM_ITEM, "Return = CR + LF",               "5", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_RETURN_CRLF},
    {    NM_ITEM, "Local Echoback",                 "6", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_LOCAL_ECHOBACK},
    {    NM_ITEM, "Raw Connection",                 "7", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_RAW_CONNECTION},
    {    NM_ITEM, "Jump Scroll",                    "8", HIGHCOMP|CHECKIT|MENUTOGGLE, 0, (APTR)MENU_JUMP_SCROLL},

    { NM_TITLE, "Settings",                          0 ,             0,               0, (APTR)MENU_SETTINGS},
    {    NM_ITEM, "Screen Mode..",                  "S",             0,               0, (APTR)MENU_SCREEN_MODE},
    {    NM_ITEM, "Screen Font..",                  "F",             0,               0, (APTR)MENU_SCREEN_FONT},
    {    NM_ITEM, "Screen Palette..",               "-",             0,               0, (APTR)MENU_SCREEN_PALETTE},
    {    NM_ITEM, "Download Path..",                "O",             0,               0, (APTR)MENU_DOWNLOAD_PATH},
    {    NM_ITEM, "Transfer Protocol..",            "T",             0,               0, (APTR)MENU_TRANSFER_PROTOCOL},
    {    NM_ITEM, "Protocol Options..",             "*",             0,               0, (APTR)MENU_PROTOCOL_OPTIONS},
    {    NM_ITEM, "Function Keys..",                "K",             0,               0, (APTR)MENU_FUNCTION_KEYS},
    {    NM_ITEM, "XEM Library..",                  "#",             0,               0, (APTR)MENU_XEM_LIBRARY},
    {    NM_ITEM, "XEM Lib Options..",              "+",             0,               0, (APTR)MENU_XEM_LIB_OPTIONS},
    {    NM_ITEM, "Telnet Display ID..",            "9",             0,               0, (APTR)MENU_TELNET_DISPLAY_ID},
    {    NM_ITEM, "ScrollBack Lines..",             "0",             0,               0, (APTR)MENU_SCROLLBACK_LINES},
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
static void ResetTelnetContext(void);
static void ResetZmodemContext(void);
static void SetLocalEchoBack(BOOL wantedState);

extern struct ExecBase *SysBase;
struct ReqToolsBase *ReqToolsBase = NULL;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *KeymapBase, *GadToolsBase, *AslBase, *SocketBase;
struct Library *DiskfontBase, *IconBase, *WorkbenchBase, *UtilityBase;

struct Window *win, *scrollbackWin, *toolBarWin;
static struct Window *packetWin;
struct List *scrollbackList;
struct Screen *scr;
struct DrawInfo *drawInfo;
static struct Gadget screenToBackGadget; // In top right corner when title bar is hidden in full screen
struct NewGadget newGadget;
static struct TextAttr fontAttr;    // describes the desired font
struct TextFont *ansiFont;          // actual font loaded via OpenFont(), ready to use
static BOOL isConDeviceOpened = FALSE;
static struct IOStdReq *writeConsoleReq = NULL;
static struct MsgPort  *writeConsoleMP  = NULL;
struct Menu *mainMenuStrip;
static struct DiskObject *diskObj;
struct MsgPort *iconPort;
static struct AppIcon *appIconOnWB;
static struct hostent *hostAddr;
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

struct PrefsStruct prefs;

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
TEXT fKeys[F_KEY_COUNT * F_KEY_SIZE];
static unsigned char conbuf[16], scrollbuf[402];
char server[64];
static ULONG lasttop;        // last topline of scrollback
UWORD tcpPort = 23;    // current tcp port
UWORD winTop;        // WinTop topEdge (titlebar height)
BOOL shouldQuitApp;    // program finished
static BOOL isConnected;    // tcp connected
#ifdef _LEGACY_RECEIVE
    // Useless with new Telnet state machine:
    static UBYTE passAll;       // passall telnet negotiation
    static UBYTE passFlag;      // already sent 8bit info
#endif
static BOOL shouldRestart;    // prefs changed, restart
static BOOL shouldReopenScreen;    // flag
BOOL isRunningOnWB; // running in wb
BOOL isAppIconified;    // iconified
static BOOL shouldIconify;        // must iconify
BOOL shouldUniconify;        // must uniconify
static UBYTE drivertype;    // drivertype 0 - normal    1 - xem library
#define DRIVER_NORMAL  0
#define DRIVER_XEM_LIB 1
static BOOL isFingerRequest;        // isFingerRequest?

static UWORD colorPens[]  = { 1,4,1,1,6,4,1,0,5,4,1,6,65535 };
static UWORD color[] = { 0x0000, 0x0DDD, 0x00D0, 0x0DD0, 0x000D, 0x0D0D, 0x00DD, 0x0D00,
          0x0555, 0x0FFF, 0x00F0, 0x0FF0, 0x000F, 0x0F0F, 0x00FF, 0x0F00, 65535 };
/*                 black,    white,  green, yellow, blue, purple, aqua,   red */

char const prefsFilename[] = "PROGDIR:DCTelnet.Prefs";
char const bookFilename[]  = "PROGDIR:DCTelnet.Book";
char const keysFilename[]  = "PROGDIR:DCTelnet.Keys";
static char *programName = NULL;   // Name provided by argv[0] or task::tc_Node.ln_Name
struct Task *mainTask = NULL;
BYTE dontUseSig31 = -1; // don't use it, ibmcon.device will destroy it.

#include "DCTelnet-debug.h"


static void ConWrite(char *data, long len)
{
    if(!isAppIconified)
    {
        if(drivertype)
            XemWrite(data, len);
        else {
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
        }
    }
}

void LocalPrint(char *data)
{
    ConWrite(data, strlen(data));
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
    ConWrite(buf, strlen(buf));
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
long TCPSend(const char *buf, long len)
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


/**
 * @brief Display a busy/wait mouse pointer in the specified window.
 *
 * Replaces the window's current pointer with a custom 16×16 "wait" pointer to indicate that a modal
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
    //rtEZRequestA(str, "OK", NULL, NULL, (struct TagItem *)&tags);
    InfoReq(isRunningOnWB ? NULL : win, str);
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


static void DisConnect(char remote, char quiet)
{
    if(isConnected)
    {
        if(!quiet && !isAppIconified)
        {
            register ULONG spent;
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
        #ifdef _LEGACY_RECEIVE
            passAll = FALSE;
            passFlag = FALSE;
        #else
        ResetTelnetContext();
        ResetZmodemContext();
        #endif
        nBytesReceived = 0;
        nBytesSent = 0;

        if(isFingerRequest)
        {
            WORD optionsMenuNumber = GetMenuNumberFromID(MENU_OPTIONS);

            if (optionsMenuNumber >= 0)
                OnMenu(win, FULLMENUNUM(optionsMenuNumber, NOITEM, NOSUB));

            isFingerRequest = FALSE;
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
        ReqToolsBase = (struct ReqToolsBase *)OpenLibrary (REQTOOLSNAME, 0);

        if (ReqToolsBase)
        {
            result=TRUE;
            #ifdef _DEBUG
                PutStr("InitializeReqToolsLib() : ReqTools library loaded\n");
            #endif
        }
        else
        {
            InfoReq(isRunningOnWB ? NULL : win,
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
            InfoReq(isRunningOnWB ? NULL : win,
                    "InitializeReqToolsLib() called with a NULL argument!");
        #endif
    }

    return result;
}

static BOOL ChooseScreen(char firsttime)
{
    BOOL result = FALSE;

    if(firsttime)
    {
        prefs.DisplayID     = (PAL_MONITOR_ID | HIRES_KEY); // PAL High Res (640×256), no interlaced
        prefs.DisplayWidth  = 640;
        prefs.DisplayHeight = 256;
        prefs.DisplayDepth  = 4;
    }

    if (AslBase && AslBase->lib_Version >= 38) // ASL screen mode requester introduced with AmigaOS 2.1
    {
        result = ScreenModeRequester(isRunningOnWB ? NULL : win, &prefs.DisplayID,
                                    &prefs.DisplayWidth, &prefs.DisplayHeight, &prefs.DisplayDepth);
    }
    else    // fallback to legacy ReqTools library
    {
        struct rtScreenModeRequester *scrmodereq;
        ULONG reqtoolsTags[5];

        InitializeReqToolsLib(reqtoolsTags);

        if(scrmodereq = rtAllocRequestA (RT_SCREENMODEREQ, NULL))
        {
            //if(firsttime)
            //{
                // scrmodereq->DisplayID = HIRES_KEY;//PAL_MONITOR_ID
                // (...)
            //} else {
                //rtChangeReqAttr(scrmodereq, RTSC_ModeFromScreen, scr, TAG_END);
            scrmodereq->DisplayID     = prefs.DisplayID;
            scrmodereq->DisplayWidth  = prefs.DisplayWidth;
            scrmodereq->DisplayHeight = prefs.DisplayHeight;
            scrmodereq->DisplayDepth  = prefs.DisplayDepth;
            //}

            if (rtScreenModeRequest (scrmodereq, "Screen Mode..",
                                     RT_Window,    win,
                                     RTSC_Flags,    SCREQF_DEPTHGAD|SCREQF_SIZEGADS|SCREQF_GUIMODES,
                                     RTSC_MaxDepth,    4,
                                     TAG_END))
            {
                prefs.DisplayID     = scrmodereq->DisplayID;
                prefs.DisplayWidth  = scrmodereq->DisplayWidth;
                prefs.DisplayHeight = scrmodereq->DisplayHeight;
                prefs.DisplayDepth  = scrmodereq->DisplayDepth;
                result = TRUE;
            }
            rtFreeRequest (scrmodereq);
        }
    }

    // On first time init, returning FALSE prevents the preferences from being written to disk.
    return result;
}

static void ChoosePalette(void)
{
    APTR reqinfo;
    ULONG reqtoolsTags[5];

    InitializeReqToolsLib(reqtoolsTags);

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
 * redirect execution to the common “add” block. This structure triggers VBCC warning 175 (“this
 * code is weird”) when optimization is enabled, hence the conditional suppression pragma.
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

    if (prefs.flags & FLAG_RAW_CONNECTION)
    {
        ConWrite(recvBuffer, len);
        if(!(prefs.flags & FLAG_DISABLE_SCROLLBACK))   AddBuf(recvBuffer, len);

        for (i = 0; i < len; i++)
        {
            if (ZmodemDetect(recvBuffer[i]))
                break;
        }
    }
    else // Telnet connection
    {
        for(i = 0; i < len; i++)
        {
            #ifdef _DEBUG
                if (outLen >= sizeof(outBuffer))
                {
                    InfoReq(isRunningOnWB ? NULL : win, "outBuffer overflow in Receive()!");
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
            ConWrite(outBuffer, outLen);

            if(!(prefs.flags & FLAG_DISABLE_SCROLLBACK))
                AddBuf(outBuffer, outLen);
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
        if (! (prefs.flags & FLAG_RAW_CONNECTION))
            IsTelnetSessionReadyForXfer();

        if (zmodemCtx.state == ZMODEM_DOWNLOAD) Download(prefs.xferlibrary);
        if (zmodemCtx.state == ZMODEM_UPLOAD)     Upload(prefs.xferlibrary);

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
norm:                buf[j] = str[i];
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


static void SpeedTest(void)
{
    ULONG before_s, before_micros;
    ULONG after_s, after_micros;
    ULONG elapsed_micros;
    ULONG lines_per_second;
    register UWORD i;
    char *rating;

    // "›0 p" = 9B 30 20 70 : Set Cursor Rendition -> make cursor invisible
    //                        (disabling the cursor slightly improves output speed)
    // "›m"   = 9B 6D        : Select Graphic Rendition -> reset attributes (white on black)
    // "\f"   = 0x0C (FF)    : Form Feed -> clear the window
    // Reference: Amiga ROM Kernel Reference Manual v2.04 - Devices (1991),
    //            section "Control Sequences for Window Output"
    ConWrite("›0 p›m\f", 7);

    if(drivertype && isRunningOnWB)  // if XEM Enabled running on Workbench
    {
        LocalPrint("WARNING: The Xem library may hang the terminal window during this test!\r\n"
                   "If this happens, use \"Reset Screen\" from the DCTelnet menu.\r\n");
        Delay(3*TICKS_PER_SECOND);
    }

    CurrentTime(&before_s, &before_micros);

    for (i = 1; i < 201; i++)
        LocalFmt("Line %ld.\r\n", i);

    CurrentTime(&after_s, &after_micros);

    // set cursor visible
    ConWrite("›1 p", 4);

    if (after_micros >= before_micros)
    {
        elapsed_micros = (after_s - before_s) * 1000000
                       + (after_micros - before_micros);
    }
    else
    {
        elapsed_micros = (after_s - before_s - 1) * 1000000
                       + (1000000 - before_micros + after_micros);
    }

    if (elapsed_micros == 0)
    {
        LocalPrint("\r\nSpeed: too fast to measure\r\n"
                   "Rating: Incredible\r\n");
    }
    else
    {
        lines_per_second = 200000000 / elapsed_micros;

        if (lines_per_second < 20)
            rating = "Poor";
        else if (lines_per_second < 30)
            rating = "Average";
        else if (lines_per_second < 50)
            rating = "Good";
        else
            rating = "Excellent";

        LocalFmt("\r\nSpeed: %ld lines/second\r\n"
                 "Rating: %s\r\n",
                 lines_per_second, rating);
    }
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

static void Finger(void)
{
    char tbuf[64] = "reiver@plan.cat";

    if (GetStringRequester(isRunningOnWB ? NULL : win,
                              "Finger",
                              "Enter EMail Address:",
                              tbuf, sizeof(tbuf))
       )
    {
        char * host = strchr(tbuf, '@');
        if(host)
        {
            ULONG originalState = prefs.flags & FLAG_RAW_CONNECTION;

            host[0] = 0;
            *host++;

            prefs.flags |= FLAG_RAW_CONNECTION;     // Enable flag (NO telnet negotiation)
            if(BeginServerConnection(host, 79) == RETURN_OK)
            {
                WORD optionsMenuNumber = GetMenuNumberFromID(MENU_OPTIONS);

                mysprintf(buf, "/W %s\r\n", tbuf);
                send(tcpSocket, buf, strlen(buf), 0);

                if (optionsMenuNumber >= 0)
                    OffMenu(win, FULLMENUNUM(optionsMenuNumber, NOITEM, NOSUB));

                isFingerRequest = TRUE;
            }
            prefs.flags = (prefs.flags & ~FLAG_RAW_CONNECTION) | originalState;  // Restore state
        }
    }
}

/**
@brief Load application preferences from the prefs file.

Loads preferences from the configured prefs file. If the file does not exist, it is created and
initialized with default values.

Calling code must ensure that ReqTools.library is opened before invoking this function.

@return TRUE on success, non-zero error code on failure.
*/
BOOL LoadPrefs(void)
{
    BPTR fh = Open(prefsFilename, MODE_OLDFILE);
    BOOL firsttime = FALSE;

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
        firsttime = TRUE;
        isAppIconified = TRUE;
    }

    // Prefs loaded but use must choose another Screen Mode. OpenDisplay() could not OpenScreen()
    // last time DCTelnet was launched.
    if (prefs.DisplayID == DEFAULT_MONITOR_ID)
    {
        firsttime = TRUE;
        isAppIconified = TRUE;
    }

    if (firsttime)
    {
        InfoReq(NULL,
                "This is the first time you've run DCTelnet."    "\n"
                                        "\n"
                "You will now have to select a screen mode"    "\n"
                "for DCTelnet to open on. The recommended"    "\n"
                "mode is 640*256*16 for good ANSI emulation."    "\n"
                                        "\n"
                "Once DCTelnet has started, you can make"    "\n"
                "the program open a window on the Workbench"    "\n"
                "screen, instead of opening its own screen."
            );

        if(ChooseScreen(TRUE))
        {
            prefs.fontsize = 8;
            strlcpy(prefs.fontname,    "topaz.font",                  sizeof(prefs.fontname));
            strlcpy(prefs.xferlibrary, "xprzmodem.library",           sizeof(prefs.xferlibrary));
            strlcpy(prefs.xferinit,    "TC,OR,B32,FO,AN,DN,KY,SN,RN", sizeof(prefs.xferinit));
            memcpy(prefs.color, color, sizeof(prefs.color));
            //CopyMem(&color[0], &prefs.color[0], 32);
            prefs.flags = FLAG_TOOL_BAR;
fixprefs:        //prefs.win_left = 0;
            prefs.win_top = 11;
            prefs.win_width = 640;
            prefs.win_height = 200;
            //prefs.sb_left = 0;
            prefs.sb_top = 12;
            prefs.sb_width = 640;
            prefs.sb_height = (prefs.DisplayHeight / 2) - 4;
            SavePrefs();
        }
        else
        {
            return FALSE;
        }
    }

    if(prefs.sb_lines == 0) prefs.sb_lines = 300;

    if(prefs.displayidstr[0] == 0) strlcpy(prefs.displayidstr, "VT102", sizeof(prefs.displayidstr));

    // Loads the macro function keys config file if present:
    fh = Open(keysFilename, MODE_OLDFILE);
    if(fh)
    {
        Read(fh, fKeys, F_KEY_COUNT * F_KEY_SIZE);
        Close(fh);
    }

    return TRUE;
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


    // Workaround for connection freeze after changing display settings: ibmcon.device improperly
    // frees signal bit 31 when being closed. We explicitly allocate signal 31 here to prevent it
    // from being assigned elsewhere and accidentally released.
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

    UtilityBase = OpenLibrary("utility.library", 0);
    if (UtilityBase == NULL)
    {
        InfoReq(NULL, "utility library could not be opened.\n"
                "DCTelnet requires utility.library.");
        goto clean_exit;
    }


    if (! LoadPrefs()) goto clean_exit;

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

    // Connect to server if it was specified in the command line. It needs an opened display.
    if (server[0] != '\0')
        BeginServerConnection(server, tcpPort);

    shouldRestart = FALSE;
    shouldReopenScreen = FALSE;

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

        if(isAppIconified)
        {
            if(iconPort) iconsig = 1L<<iconPort->mp_SigBit;

            if(isConnected)
            {
                FD_ZERO(&rd);
                FD_SET(tcpSocket, &rd);
                sigmask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | iconsig;

                // https://wiki.amigaos.net/amiga/autodocs/bsdsocket.doc.txt (tout à la fin)
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
                    if (i <  0)  InfoReq(isRunningOnWB ? NULL : win,
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
                        InfoReq(isRunningOnWB ? NULL : win,
                                "WaitSelect() returns < 0 (error) ! Why???");
                    }
                    else if (i == 0)
                    {
                        PutStr("<-- WaitSelect() => 0 (= timeout or signal received)\n    sigs=");
                        LogWaitSelectResult(sigmask);

                        if (FD_ISSET(tcpSocket, &rd))
                            InfoReq(isRunningOnWB ? NULL : win,
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
                    if((prefs.flags & (FLAG_HIDE_TITLEBAR | FLAG_HIDE_LEDS)) == 0)
                    {
                        SetAPen(&scr->RastPort, 10);
                        RectFill(&scr->RastPort, scr->Width-70, 3, scr->Width-62, prefs.fontsize-2);
                    }
                    Receive();
                    if((prefs.flags & (FLAG_HIDE_TITLEBAR | FLAG_HIDE_LEDS)) == 0)
                        EraseRect(&scr->RastPort, scr->Width-72, 2, scr->Width-60, prefs.fontsize-1);
                }

            } else {  // not connected
                ULONG sig;

                if(scrollbackWin)  sig = 1L << scrollbackWin->UserPort->mp_SigBit; else sig = 0;
                if(packetWin) sig |= 1L << packetWin->UserPort->mp_SigBit;
                if (toolBarWin) sig |= 1L << toolBarWin->UserPort->mp_SigBit;

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
            }
        }
    } /* -- end of main loop -- */

    SavePrefs();

    prefs.flags = FLAG_HIDE_TITLEBAR;

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
        if (! ConfirmRequester(isRunningOnWB ? NULL : win, "OverWrite|Cancel",
                                       "File Already Exists."))
        //if(!rtEZRequestA("File Already Exists.","OverWrite|Cancel", NULL, NULL, (struct TagItem *)&reqtoolsTags))
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


    if (GetStringRequester(isRunningOnWB ? NULL : win,
                              "Connect",
                              "Enter host:port",
                              tbuf, sizeof(tbuf))
       )
    //if(rtGetStringA(tbuf, 63, "Enter host,port:", 0, (struct TagItem *)&reqtoolsTags))
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

        InfoReq(isRunningOnWB ? NULL : win,
                "     Host Name ... : %s\n"
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

        // mysprintf(buf,    "     Host Name ... : %s\n"
        //         "    IP Address ... : %s\n"
        //         "      TCP Port ... : %ld\n\n"
        //         "   Online Time ... : %02ld:%02ld:%02ld\n"
        //         "    Bytes Sent ... : %ld\n"
        //         "Bytes Received ... : %ld",
        //     hostAddr->h_name,
        //     Inet_NtoA(inetSocketAddr.sin_addr.s_addr),
        //     tcpPort,
        //     spent/3600, (spent/60)%60, spent%60,
        //     nBytesSent,
        //     nBytesReceived);

        // rtEZRequestTags(buf, "OK", NULL, NULL,
        //         RT_Window,    win,
        //         RT_ReqPos,    REQPOS_CENTERSCR,
        //         TAG_DONE);
    } else
        SimpleReq("Not isConnected");
}


/**
 * @brief Updates a NewMenu item's checked state based on a preference flag.
 *
 * If the specified flag is set in prefs.flags, the menu item is checked.
 * Otherwise, the menu item is unchecked.
 *
 * @param id Identifier of the NewMenu item whose CHECKED state is to be set.
 * @param flag The flag in prefs.flags controlling the menu item's checked state.
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

    if (prefs.flags & flag)
        newMenuItem->nm_Flags |= CHECKED;
    else
        newMenuItem->nm_Flags &= ~CHECKED;
}


/**
 * @brief Updates prefs.flags based on a menu item's checked state.
 *
 * If the menu item is checked, the prefs.flags bit is set.
 * If it is unchecked, the flag bit is cleared.
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

/*
 Uncheck a menu item and clear the corresponding flag in prefs.flags,
 or check the menu item and set the flag, depending on the "wantedState" parameter.
 https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node024A.html
 https://www.amiga-news.de/en/news/AN-2023-10-00017-EN.html
*/
static void SetLocalEchoBack(BOOL wantedState)
{
    struct MenuItem *item = NULL;

    BOOL currentState = prefs.flags & FLAG_LOCAL_ECHO;

    #ifdef _DEBUG
        PutStr("›34m--> SetLocalEchoBack()›m\n");
    #endif

    if (currentState != wantedState)
    {
        #ifdef _DEBUG
            PutStr("›34mcurrentState != wantedState›m\n");
        #endif
        ClearMenuStrip(win);

        item = GetMenuItemFromID(MENU_LOCAL_ECHOBACK);
        if (item != NULL)
        {
            if (wantedState)
            {
                item->Flags |= CHECKED;
                prefs.flags |= FLAG_LOCAL_ECHO;
            }
            else
            {
                item->Flags &= ~CHECKED;
                prefs.flags &= ~FLAG_LOCAL_ECHO;
            }
        }
        ResetMenuStrip(win, mainMenuStrip);
    }
}

static void OutKey(unsigned char key)
{
    if(prefs.flags & FLAG_BS_DEL_SWAP)
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
            if (! (prefs.flags & FLAG_RAW_CONNECTION))
                TCPSend((void *)&key, 1);

        #ifdef _LEGACY_RECEIVE
            // Useless with new Telnet state machine:
            if(!passFlag)
            {
                // IAC DO BINARY   IAC WILL BINARY
                TCPSend("\377\375\000\377\373\000", 6); // 8-bit data path
                passFlag = TRUE;
            }
        #endif
        if(prefs.flags & FLAG_LOCAL_ECHO) goto cwrite;
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
                strlcat(strBuffer, (prefs.flags & FLAG_RETURN_CRLF) ? "\r\n" : "\r",
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
                                Upload(prefs.xferlibrary);
                            else
                                Download(prefs.xferlibrary);
                        } else
                            SimpleReq("You better connect first.");
                        break;
                    case BUTTON_QUIT:
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
                case RAWKEY_CRSRUP:
                    goto up;
                case RAWKEY_CRSRDOWN:
                    goto down;
                case RAWKEY_F5:
                    buf[0] = '\0';
                    strlcpy(fbuf, "DCTelnet.Cap", sizeof(fbuf));
                    if (FileRequester(isRunningOnWB ? NULL : win,
                                      buf,  sizeof(buf),
                                      fbuf, sizeof(fbuf),
                                      "#?",
                                      FILEREQ_SAVE))
                    //if(FileReq(buf, "#?", fbuf, "Save Scroll Back", TRUE, 0))
                    {
                        AddPart(buf, fbuf, sizeof(buf));
                        //strcat(buf, fbuf);
                        SaveScrollBack(buf);
                    }
                    break;
                case RAWKEY_F3:
                    if (ConfirmRequester(isRunningOnWB ? NULL : win, "Print|Cancel",
                                                 "Print Scrollback?"))
                    //if(rtEZRequestA("Print Scrollback?", "Print|Cancel", NULL, NULL, (struct TagItem *)&reqtoolsTags))
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
                                if (conbuf[i] >= '0' && conbuf[i] <= '9')
                                {
                                    key_macro = TRUE;
                                    SendMacro(&fKeys[(conbuf[i] - '0') * F_KEY_SIZE]);
                                }

                                switch(conbuf[i])
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
        {
            UWORD menuNumber = code;
            struct MenuItem *item = NULL;
            UWORD nextMenuNumber = MENUNULL;
            enum MenuItemID menuID;

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
                        InfoReq(isRunningOnWB ? NULL : win,
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

                    case MENU_SCROLLBACK:
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
                        if(isConnected)  Upload(prefs.xferlibrary);
                        else             SimpleReq("You better connect first.");
                        break;

                    case MENU_DOWNLOAD:
                        if(isConnected)  Download(prefs.xferlibrary);
                        else             SimpleReq("You better connect first.");
                        break;

                    case MENU_ASCII_SEND:
                        if(isConnected)
                        {
                            fbuf[0] = '\0';
                            if (FileRequester(isRunningOnWB ? NULL : win,
                                              prefs.uploadpath, sizeof(prefs.uploadpath),
                                              fbuf, sizeof(fbuf),
                                              "#?",
                                              FILEREQ_LOAD))
                            {
                                register long r;
                                strlcpy(buf, prefs.uploadpath, sizeof(buf));
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

                    case MENU_USE_WORKBENCH:
                        UpdatePrefsFlagFromMenu(item, FLAG_USE_WORKBENCH);
                        shouldRestart = TRUE;
                        shouldReopenScreen = TRUE;
                        break;

                    case MENU_DISABLE_LEDS:
                        UpdatePrefsFlagFromMenu(item, FLAG_HIDE_LEDS);
                        if(item->Flags & CHECKED)
                        {
                            if(!(prefs.flags & FLAG_HIDE_TITLEBAR))
                            {
                                SetAPen(&scr->RastPort, 1);
                                RectFill(&scr->RastPort, scr->Width-86, 2, scr->Width-60, prefs.fontsize-1);
                            }
                        }
                        else
                            LEDs();
                        break;

                    case MENU_HIDE_TITLEBAR:
                        UpdatePrefsFlagFromMenu(item, FLAG_HIDE_TITLEBAR);
                        shouldRestart = TRUE;
                        shouldReopenScreen = TRUE;
                        break;

                    case MENU_UNUSED_CRLF:
                        UpdatePrefsFlagFromMenu(item, FLAG_CRLF_CORRECTION);
                        break;

                    case MENU_BS_DEL_SWAP:
                        UpdatePrefsFlagFromMenu(item, FLAG_BS_DEL_SWAP);
                        break;

                    case MENU_DISABLE_SCROLLBACK:
                        UpdatePrefsFlagFromMenu(item, FLAG_DISABLE_SCROLLBACK);
                        break;

                    case MENU_STRIP_ANSI_CODES:
                        UpdatePrefsFlagFromMenu(item, FLAG_STRIP_COLOUR);
                        #ifndef _LEGACY_RECEIVE
                            if(item->Flags & CHECKED) LocalPrint("›m");
                        #endif
                        break;

                    case MENU_UNUSED_SIMPLE_TELNET:
                        UpdatePrefsFlagFromMenu(item, FLAG_SIMPLE_TELNET);
                        break;

                    case MENU_PACKET_WINDOW:
                        UpdatePrefsFlagFromMenu(item, FLAG_PACKET_WINDOW);
                        if (isRunningOnWB)
                            SimpleReq("Packet Window cannot work in Workbench mode.");
                        else
                            shouldRestart = TRUE;
                        break;

                    case MENU_USE_XEM_LIBRARY:
                        if (prefs.displaydriver[0] == '\0')
                        {
                            SimpleReq("No XEM library has been selected yet.\n"
                                      "Please choose one first from the Settings menu.");
                        }
                        else
                            UpdatePrefsFlagFromMenu(item, FLAG_USE_XEM_LIBRARY);

                        // Restart even when prefs.displaydriver[0] == '\0', this forces a menu
                        // refresh so the CHECKED state of the item is properly reverted :
                        shouldRestart = TRUE;
                        break;

                    case MENU_TOOLBAR:
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

                    case MENU_RETURN_CRLF:
                        UpdatePrefsFlagFromMenu(item, FLAG_RETURN_CRLF);
                        break;

                    case MENU_LOCAL_ECHOBACK:
                        UpdatePrefsFlagFromMenu(item, FLAG_LOCAL_ECHO);
                        break;

                    case MENU_RAW_CONNECTION:
                        UpdatePrefsFlagFromMenu(item, FLAG_RAW_CONNECTION);
                        break;
                    case MENU_JUMP_SCROLL:
                        UpdatePrefsFlagFromMenu(item, FLAG_JUMP_SCROLL);
                        if(!isRunningOnWB && !(prefs.flags & FLAG_USE_XEM_LIBRARY))
                            shouldRestart = TRUE;
                        break;

                    case MENU_SCREEN_MODE:
                        oldDispID = prefs.DisplayID;
                        oldDepth  = prefs.DisplayDepth;
                        if (ChooseScreen(FALSE)
                            && ((oldDispID  != prefs.DisplayID) || (oldDepth != prefs.DisplayDepth))
                           )
                        {
                            shouldRestart = TRUE;
                            shouldReopenScreen = TRUE;
                        }
                        break;

                    case MENU_SCREEN_FONT:
                        if (FontRequester(isRunningOnWB ? NULL : win,
                                          prefs.fontname, sizeof(prefs.fontname),
                                          &prefs.fontsize))
                        {
                            shouldRestart = TRUE;
                            shouldReopenScreen = TRUE;
                        }
                        break;

                    case MENU_SCREEN_PALETTE:
                        ChoosePalette();
                        break;

                    case MENU_DOWNLOAD_PATH:
                        DirectoryRequester(isRunningOnWB ? NULL : win,
                                           prefs.downloadpath, sizeof(prefs.downloadpath));
                        break;

                    case MENU_TRANSFER_PROTOCOL:
                        FileRequester(isRunningOnWB ? NULL : win,
                                      "LIBS:", 0, // 0 because we don't want to get the dirname
                                      prefs.xferlibrary, sizeof(prefs.xferlibrary),
                                      "xpr#?.library",
                                      FILEREQ_LOAD);
                        break;

                    case MENU_PROTOCOL_OPTIONS:
                        GetStringRequester(isRunningOnWB ? NULL : win,
                                                    "XPR Protocol Options..",
                                                    "Options string:",
                                                    prefs.xferinit, sizeof(prefs.xferinit));
                        // TODO Open XPR options Dialog : XferOptions(prefs.xferlibrary);
                        break;

                    case MENU_FUNCTION_KEYS:
                        WindowSub(FunctionKeys);
                        break;

                    case MENU_XEM_LIBRARY:
                        if (FileRequester(isRunningOnWB ? NULL : win,
                                          "LIBS:", 0, // 0 because we don't want to get the dirname
                                          prefs.displaydriver, sizeof(prefs.displaydriver),
                                          "xem#?.library",
                                          FILEREQ_LOAD))
                        {
                            if(prefs.flags & FLAG_USE_XEM_LIBRARY) shouldRestart = TRUE;
                        }
                        break;

                    case MENU_XEM_LIB_OPTIONS:
                        if (xemIO)
                            XEmulatorOptions(xemIO);
                        else
                            InfoReq(isRunningOnWB ? NULL : win, "The XEM library is currently "
                                           "disabled, so related functionality is unavailable.");
                        break;

                    case MENU_TELNET_DISPLAY_ID:
                        GetStringRequester(isRunningOnWB ? NULL : win,
                                                    "Telnet Display ID...",
                                                    "Term type:",
                                                    prefs.displayidstr, sizeof(prefs.displayidstr));
                        break;

                    case MENU_SCROLLBACK_LINES:
                        InitializeReqToolsLib(reqtoolsTags);
                        rtGetLongA(&prefs.sb_lines, "ScrollBack Lines..", NULL, (struct TagItem *)&reqtoolsTags);
                        break;

                    case MENU_SNAPSHOT_WINDOWS:
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
down:                if(lasttop+((scrollbackWin->Height - (prefs.fontsize + scr->WBorTop + 2)) / prefs.fontsize) < nScrollbackLines) lasttop++;
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
            PGA_Visible,    (scrollbackWin->Height - (prefs.fontsize + scr->WBorTop + 2)) / prefs.fontsize,
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

    UpdateConnectingWindowMessage("Looking up...", 4);
    UpdateConnectingWindowMessage(servername, 0);

    LocalFmt("\r\nLooking up ›32m%s›m...\r\n", servername);

    strlcpy(server, servername, sizeof(server));

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

    UpdateConnectingWindowMessage(Inet_NtoA(inetSocketAddr.sin_addr.s_addr), 1);
    UpdateConnectingWindowMessage(hostAddr->h_name, 2);

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

    #ifdef _LEGACY_RECEIVE
        if(!(prefs.flags & FLAG_RAW_CONNECTION))
            TCPSend("\377\375\003", 3);    // IAC DO SGA
        else {
            passAll = TRUE;
            passFlag = TRUE;
        }
    #else
    ResetTelnetContext();
    ResetZmodemContext();
    #endif

    if (isRunningOnWB) WindowToFront(win); else ScreenToFront(scr);

    conectionTime = mytime();

    isConnected = TRUE;

    LEDs();

    return(0);
}


struct Screen* OpenAppScreen(void)
{
    struct Screen *scr;

    if(prefs.flags & FLAG_USE_WORKBENCH) isRunningOnWB = TRUE;
    else isRunningOnWB = FALSE;

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
        prefs.flags &= ~FLAG_PACKET_WINDOW;           // Not Packet Window
        scr = LockPubScreen(0L);
    }
    else
    {
        register UWORD *pens;
        static struct NewScreen newscr;

        if(prefs.DisplayDepth < 3) pens = &colorPens[12]; else pens = colorPens;

        newscr.Width  = prefs.DisplayWidth;
        newscr.Height = prefs.DisplayHeight;
        newscr.Depth  = prefs.DisplayDepth;
        newscr.BlockPen = 1;
        newscr.Type = CUSTOMSCREEN;
        newscr.Font = &fontAttr;
        // Main window title in full screen mode:
        newscr.DefaultTitle = MainWindowTitle;

        scr = OpenScreenTags(&newscr,
            SA_DisplayID,    prefs.DisplayID,
            SA_Pens,        (ULONG)pens,
            SA_ShowTitle,    !(prefs.flags & FLAG_HIDE_TITLEBAR),
            SA_AutoScroll,    TRUE,
            SA_Interleaved,    TRUE,
            TAG_END);
        /*
        scr = OpenScreenTags(NULL,
            SA_Title,    "DCTelnet 1.5 © "__DATE__" By ZED^DC",
            SA_Width,    prefs.DisplayWidth,
            SA_Height,    prefs.DisplayHeight,
            SA_DisplayID,    prefs.DisplayID,
                        SA_Depth,    prefs.DisplayDepth,
            SA_ShowTitle,    !prefs.flags&1,
            SA_Type,    CUSTOMSCREEN,
                        SA_Pens,    (ULONG)pens,
            SA_Font,    &fontAttr,
            SA_AutoScroll,    TRUE,
            SA_Interleaved,    TRUE,
                        TAG_END);*/
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

    if (isRunningOnWB)
    {
        GetNewMenuItemFromID(MENU_USE_WORKBENCH)->nm_Flags |= CHECKED;
        //GetNewMenuItemFromID(MENU_JUMPSCROLL)->nm_Flags = NM_ITEMDISABLED;
        GetNewMenuItemFromID(MENU_SCREEN_MODE)->nm_Flags = NM_ITEMDISABLED;
        GetNewMenuItemFromID(MENU_SCREEN_PALETTE)->nm_Flags = NM_ITEMDISABLED;

        newWin.LeftEdge   = prefs.win_left;
        newWin.TopEdge    = prefs.win_top;
        newWin.Width      = prefs.win_width;
        newWin.Height     = prefs.win_height;
        newWin.MinWidth   = 200;
        newWin.MinHeight  = 50;
        newWin.MaxWidth   = 1600;
        newWin.MaxHeight  = 1200;
        newWin.IDCMPFlags = IDCMP_RAWKEY | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK;
        //newWin.Flags = WFLG_GIMMEZEROZERO|WFLG_NEWLOOKMENUS|WFLG_SIMPLE_REFRESH|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
        newWin.Flags = WFLG_GIMMEZEROZERO|WFLG_NEWLOOKMENUS|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
        // Main window title in windowed workbench mode:
        newWin.Title = MainWindowTitle;
        newWin.FirstGadget = 0;

        CheckDimensions(&newWin);

        win = OpenWindow(&newWin);

        // Be sure to unlock the public screen when done with it.  Note that once a window is open
        // on the screen the program does not need to hold the screen lock, as the window acts as a
        // lock on the screen.  The pointer to the screen structure is valid as long as a lock on
        // the screen is held by the application, or the application has a window open on the
        // screen (Amiga ROM Kernel Reference Manual, § Accessing a public screen by name)
        UnlockPubScreen(0L, scr);

        if(prefs.flags & FLAG_TOOL_BAR) OpenToolBarWindow(FALSE);
    } else { // running in full screen
        struct Gadget *backgad;
        UWORD top, height;

        GetNewMenuItemFromID(MENU_USE_WORKBENCH)->nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
        //GetNewMenuItemFromID(MENU_JUMPSCROLL)->nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
        GetNewMenuItemFromID(MENU_SCREEN_MODE)->nm_Flags = 0;
        GetNewMenuItemFromID(MENU_SCREEN_PALETTE)->nm_Flags = 0;

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
            screenToBackGadget.GadgetID = GADGET_SCREEN_TO_BACK;
        } else {
            top = prefs.fontsize + 3;
            height = scr->Height - (prefs.fontsize + 3);
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

        if(prefs.flags & FLAG_PACKET_WINDOW)    // Packet
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
            newWin.IDCMPFlags =    IDCMP_MENUPICK |
                        IDCMP_GADGETUP;
            newWin.Flags =    WFLG_NEWLOOKMENUS |
                    WFLG_BORDERLESS |
                    WFLG_BACKDROP;

            packetWin = OpenWindow(&newWin);

            SetAPen(packetWin->RPort, 1);
            Draw(packetWin->RPort, packetWin->Width, 0);
        }

        newWin.TopEdge = top;
        newWin.Height = height;
        newWin.FirstGadget = backgad;
        newWin.IDCMPFlags =    IDCMP_GADGETUP |
                    IDCMP_RAWKEY |
                    IDCMP_CLOSEWINDOW |
                    IDCMP_MENUPICK;
        newWin.Flags =    WFLG_SMART_REFRESH |
                WFLG_NEWLOOKMENUS |
                WFLG_BORDERLESS |
                WFLG_ACTIVATE |
                WFLG_BACKDROP;

        win = OpenWindow(&newWin);
    }

    SetFont(win->RPort, ansiFont);
}

void CreateAppMenus(void)
{
    register struct MenuItem *item;
    static ULONG ltags[] = { GTMN_NewLookMenus, TRUE, TAG_END };
    #ifdef _DEBUG
        BOOL res;
        PutStr("   --> CreateAppMenus()\n");
    #endif

    // Check options in menu as set in DCTelnet.prefs file:
    if((prefs.flags & FLAG_USE_XEM_LIBRARY) && prefs.displaydriver[0])
    {
        drivertype = DRIVER_XEM_LIB;
        GetNewMenuItemFromID(MENU_XEM_LIB_OPTIONS)->nm_Flags = 0;  // Enable "XEM Lib Options" (state not saved in prefs)
        GetNewMenuItemFromID(MENU_JUMP_SCROLL)->nm_Flags = NM_ITEMDISABLED;
    } else {
        drivertype = DRIVER_NORMAL;
        GetNewMenuItemFromID(MENU_JUMP_SCROLL)->nm_Flags = HIGHCOMP|CHECKIT|MENUTOGGLE;
        GetNewMenuItemFromID(MENU_XEM_LIB_OPTIONS)->nm_Flags = NM_ITEMDISABLED;
        prefs.flags &= ~FLAG_USE_XEM_LIBRARY;
    }

    // The NewMenu item CHECKED flag will be set according to saved Prefs flags. Note: these flags
    // are already set: HIGHCOMP|CHECKIT|MENUTOGGLE for every item in Options menu in mainMenuDesc[]
    SetNewMenuCheckFromPref(MENU_USE_WORKBENCH,        FLAG_USE_WORKBENCH);
    SetNewMenuCheckFromPref(MENU_DISABLE_LEDS,         FLAG_HIDE_LEDS);
    SetNewMenuCheckFromPref(MENU_HIDE_TITLEBAR,        FLAG_HIDE_TITLEBAR);
    #ifdef _LEGACY_RECEIVE
        SetNewMenuCheckFromPref(MENU_UNUSED_CRLF,           FLAG_CRLF_CORRECTION);
    #endif
    SetNewMenuCheckFromPref(MENU_BS_DEL_SWAP,          FLAG_BS_DEL_SWAP);
    SetNewMenuCheckFromPref(MENU_DISABLE_SCROLLBACK,   FLAG_DISABLE_SCROLLBACK);
    #ifdef _LEGACY_RECEIVE
        SetNewMenuCheckFromPref(MENU_STRIP_ANSI_CODES,      FLAG_STRIP_COLOUR);
        SetNewMenuCheckFromPref(MENU_UNUSED_SIMPLE_TELNET,  FLAG_SIMPLE_TELNET);
    #endif
    SetNewMenuCheckFromPref(MENU_PACKET_WINDOW,        FLAG_PACKET_WINDOW);
    SetNewMenuCheckFromPref(MENU_USE_XEM_LIBRARY,      FLAG_USE_XEM_LIBRARY);
    SetNewMenuCheckFromPref(MENU_TOOLBAR,              FLAG_TOOL_BAR);
    SetNewMenuCheckFromPref(MENU_RETURN_CRLF,          FLAG_RETURN_CRLF);
    SetNewMenuCheckFromPref(MENU_LOCAL_ECHOBACK,       FLAG_LOCAL_ECHO);
    SetNewMenuCheckFromPref(MENU_RAW_CONNECTION,       FLAG_RAW_CONNECTION);
    SetNewMenuCheckFromPref(MENU_JUMP_SCROLL,          FLAG_JUMP_SCROLL);


    // Gadtools CreateMenuA() generates a list of Intuition Menu structs.
    mainMenuStrip = CreateMenusA(mainMenuDesc, 0);
    #ifdef _DEBUG
        Printf("   <-- CreateMenusA() => %s\n", (mainMenuStrip != NULL) ? "succeeded" : "failed");
    #endif

    if (mainMenuStrip == NULL) return;

    // Display the "Quit" menu item in red and highlight it with a box on hover.
    item = GetMenuItemFromID(MENU_QUIT);
    if (item != NULL)
    {
        if (prefs.DisplayDepth > 1)
            ((struct IntuiText *)item->ItemFill)->FrontPen = 15;

        item->Flags = (item->Flags & ~HIGHFLAGS) | HIGHBOX;
    }

    ltags[1] = isRunningOnWB;

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
        ULONG beforeSigAlloc;
        ULONG afterSigAlloc;
        UBYTE conDeviceSigBit;
        PutStr("--> OpenDisplay()\n");
    #endif

    if (scr == NULL)  // We need to (re)open completely the screen
        scr = OpenAppScreen();

    if (scr == NULL) {
        InfoReq(NULL,"Unable to open the screen. Please restart DCTelnet\n"
                     "and select an appropriate screen mode");
        prefs.DisplayID = DEFAULT_MONITOR_ID;
        SavePrefs();

        goto clean_and_return;
    }

    if (visualInfos == NULL) visualInfos = GetVisualInfoA(scr, NULL);
    if (drawInfo == NULL)    drawInfo    = GetScreenDrawInfo(scr);

    OpenAppWindow();
    if(win == NULL) { InfoReq(NULL,"Unable to open main window!"); goto clean_and_return; }

    CreateAppMenus();
    if (mainMenuStrip == NULL) { InfoReq(isRunningOnWB ? NULL : win, "Unable to create menus!");
                                 goto clean_and_return; }


    // Try to initialize the XEM library if the user enabled it.
    // If it fails fallback to ibmcon/console device.
    if(drivertype == DRIVER_XEM_LIB)
        if (! InitializeXemLibrary())
        {
            struct MenuItem *item = NULL;

            drivertype = DRIVER_NORMAL; // Xem lib failed to load so we'll try with ibmcon.device

            prefs.flags &= ~FLAG_USE_XEM_LIBRARY;

            // Uncheck the "Use XEM Library" option:
            //https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node024A.html
            // https://www.amiga-news.de/en/news/AN-2023-10-00017-EN.html
            ClearMenuStrip(win);

            item = GetMenuItemFromID(MENU_USE_XEM_LIBRARY);
            if (item != NULL)
                item->Flags &= ~CHECKED;

            ResetMenuStrip(win, mainMenuStrip);
        }


    // The console device :
    // https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0080.html

    // Doc about OpenDevice() to open a console device :
    // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node029E.html
    // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0509.html
    if(drivertype == DRIVER_NORMAL)
    {
        UWORD unitNumber;
        char *devName = isRunningOnWB ? "console.device" : "ibmcon.device";
        BOOL b;

        // Exec Device I/O Functions docs:
        // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node02A5.html

        // CreateIORequest() requires a message port.
        writeConsoleMP = CreateMsgPort();
        if (!writeConsoleMP) { InfoReq(isRunningOnWB ? NULL : win,
                                     "Unable to create message port for console device!");
                             goto clean_and_return; }

        // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0344.html
        writeConsoleReq = CreateIORequest(writeConsoleMP, sizeof(struct IOStdReq));

        // The unit number that is a standard parameter for an open call is used
        // specially by this device.
        if (isRunningOnWB)
        {
            unitNumber = CONU_SNIPMAP;
        } else {
            if(prefs.flags & FLAG_JUMP_SCROLL)
                unitNumber = 2; // unit 2 is undocumented in the NDK and AmigaOS docs
            else
                unitNumber = CONU_CHARMAP;
        }

        //the window that is used by the console device for output:
        writeConsoleReq->io_Data = win;
        writeConsoleReq->io_Length = sizeof(struct Window);

        #ifdef _DEBUG
            PutStr("   --> OpenDevice()\n");
            beforeSigAlloc = mainTask->tc_SigAlloc;
            PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
            LogWindowsSigBit();
        #endif

        b = OpenDevice(devName, unitNumber, (struct IORequest *)writeConsoleReq, CONFLAG_DEFAULT);

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
        }
        else
        {
            isConDeviceOpened = FALSE;
            InfoReq(isRunningOnWB ? NULL : win, "Failed to open device: %s", devName);
            // mysprintf(buf,    "Failed to open device: %s", devName);
            // EZReq(NULL, buf);
            goto clean_and_return;
        }
    }

    isAppIconified = FALSE;

    LEDs();

    if(!isConnected)
    {
        STRPTR dispEngine;
        register ULONG flags = SysBase->AttnFlags;
        LONG cpu = '0';

        if(flags & AFF_68010) cpu = '1';
        if(flags & AFF_68020) cpu = '2';
        if(flags & AFF_68030) cpu = '3';
        if(flags & AFF_68040) cpu = '4';
        if(flags & AFF_68060) cpu = '6';


        if(drivertype == DRIVER_XEM_LIB)
            dispEngine = prefs.displaydriver;
        else if (isRunningOnWB)
            dispEngine = "console.device";
        else
            dispEngine = "ibmcon.device";

        LocalFmt("›0;1;36m\f\r\n\r\n"
                "Processor: ›37m680%lc0\r\n\r\n›36m"
                "Kickstart: ›37m%ld.%ld\r\n\r\n›36m"
                "Display engine: ›37m%s\r\n\r\n›36m"
                "TCP Stack: ›37m",
                cpu,
                (LONG)((struct Library *)SysBase)->lib_Version,
                (LONG)SysBase->SoftVer,
                dispEngine);

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

    // https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0190.html
    if (isConDeviceOpened)
    {
        #ifdef _DEBUG
            PutStr("   --> CloseDevice(&writeConsoleReq)\n");
            PutStr("SigAlloc:"); PrintBitsULONG(mainTask->tc_SigAlloc);
            LogWindowsSigBit();

            if (! (mainTask->tc_SigAlloc & (1L << 31)))
            {
                InfoReq(isRunningOnWB ? NULL : win,
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
                InfoReq(isRunningOnWB ? NULL : win, "ERROR: cannot allocate sigbit 31!");

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

    if(packetWin)
    {
        ClearMenuStrip(packetWin);
        CloseWindow(packetWin);
        packetWin = NULL;
    }

    if(win)
    {
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
            if (! isRunningOnWB)
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
    }

    isAppIconified = TRUE;

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
