#ifndef DCTELNET_H
#define DCTELNET_H

#include <exec/types.h>
#include <string.h>     // size_t

// Types
struct PrefsStruct
{
    ULONG DisplayID;
    UWORD DisplayWidth,
          DisplayHeight,
          DisplayDepth,
          fontsize;
    char  fontname[32],
          downloadpath[52],
          xferlibrary[52],
          xferinit[52];
    UWORD color[16];
    ULONG flags;
    UWORD win_left,
          win_top,
          win_width,
          win_height,
          sb_left,
          sb_top,
          sb_width,
          sb_height;
    char  uploadpath[52],
          displaydriver[32];
    ULONG sb_lines;
    char  displayidstr[32];
    UWORD toolBarWin_left,
          toolBarWin_top;
};

/*
* Bit flags stored in PrefsStruct->flags.
* These defines describe the meaning of each individual bit.
* These flags map directly to the checkable items in the "Options" pull-down menu
*/
#define FLAG_HIDE_TITLEBAR       (1 << 0)   // BIT 0  = Hide Title Bar
#define FLAG_CRLF_CORRECTION     (1 << 1)   // BIT 1  = CRLF Correction
#define FLAG_HIDE_LEDS           (1 << 2)   // BIT 2  = Hide LEDS
#define FLAG_USE_WORKBENCH       (1 << 3)   // BIT 3  = Use Workbench
#define FLAG_BS_DEL_SWAP         (1 << 4)   // BIT 4  = BS/DEL Swap
#define FLAG_DISABLE_SCROLLBACK  (1 << 5)   // BIT 5  = Disable Scrollback
#define FLAG_STRIP_COLOUR        (1 << 6)   // BIT 6  = Strip Colour
#define FLAG_SIMPLE_TELNET       (1 << 7)   // BIT 7  = Very simple telnet negotiation.
#define FLAG_PACKET_WINDOW       (1 << 8)   // BIT 8  = Packet Window
#define FLAG_USE_XEM_LIBRARY     (1 << 9)   // BIT 9  = Use XEM Library
#define FLAG_TOOL_BAR            (1 << 10)  // BIT 10 = Tool bar
#define FLAG_RETURN_CRLF         (1 << 11)  // BIT 11 = Return = CR&LF
#define FLAG_LOCAL_ECHO          (1 << 12)  // BIT 12 = Local Echoback
#define FLAG_RAW_CONNECTION      (1 << 13)  // BIT 13 = Raw Connection (NO telnet negotiation data)
#define FLAG_JUMP_SCROLL         (1 << 14)  // BIT 14 = Jump Scroll

// ID of the gadget in top right corner when title bar is hidden in full screen
#define GADGET_SCREEN_TO_BACK  20


// Global variables exported
extern char server[64];
extern long nScrollbackLines;
extern long tcpSocket, nBytesReceived;
extern struct DrawInfo *drawInfo;
extern struct Menu *menuStrip;
extern struct MsgPort *iconPort;
extern struct NewWindow newWin;
extern struct PrefsStruct prefs;
extern struct List *scrollbackList;
extern struct Screen *scr;
extern struct TextFont *ansiFont;
extern struct Window *win, *scrollbackWin, *toolBarWin;

extern struct NewGadget newGadget;
extern BOOL shouldQuitApp;          // program finished
extern BOOL isRunningOnWB;          // running in wb
extern BOOL isAppIconified;         // iconified
extern BOOL shouldUniconify;        // must shouldUniconifyify
// Used in Receive(), xpr_sflush(). Beware: each call to this function destroy the content
extern UBYTE recvBuffer[4096];
extern unsigned char buf[2048];
#define F_KEY_COUNT 10
#define F_KEY_SIZE  152  // 151 chars + '\0'
extern TEXT fKeys[F_KEY_COUNT * F_KEY_SIZE];
extern UWORD winTop;                // WinTop topEdge (titlebar height)
extern struct Task *mainTask;       // An AmigaOS Task is roughly equivalent to a thread

// This flag is set by the "Connecting..." window task when the user cancels the operation:
extern BOOL isConnectionAborted;
extern UWORD connectMsgType;
extern char *connectString;

extern UWORD tcpPort;    // current tcp port

extern const char prefsFilename[];
extern const char keysFilename[];
extern const char bookFilename[];

extern void *visualInfos;
extern char username[42], password[42];


// Functions exported
BOOL OpenDisplay(void);
void CloseDisplay(BOOL manageScreen);
long TCPSend(const char *buf, long len);
void CloseIcon(void);                                   // Uniconify the application
void LEDs(void);
void LocalFmt(char *ctl, ...);
void TextFmt(struct RastPort *rP, char *ctl, ...);
void LocalPrint(char *data);
void OpenIcon(void);                                    // inconify the application
void SavePrefs(void);
void SimpleReq(char *str);
void SetWaitPointer(struct Window * window);
UWORD BeginServerConnection(char *servername, UWORD port);

#endif /* DCTELNET_H */
