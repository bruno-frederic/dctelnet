#ifndef DCTELNET_H
#define DCTELNET_H

#include <exec/types.h>

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
    UWORD twin_left,
          twin_top;
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

// Global variables exported
extern char server[64];
extern long lines;
extern long sok, bytes;
extern struct DrawInfo *DrawInfo;
extern struct Menu *menuStrip;
extern struct MsgPort *iconport;
extern struct NewWindow nwin;
extern struct PrefsStruct prefs;
extern struct List *slist;
extern struct Screen *scr;
extern struct TextFont *ansifont;
extern struct Window *win, *sbwin, *twin;
extern ULONG tags[5];

extern struct NewGadget ng;
extern UWORD WinTop;                // WinTop topEdge (titlebar height)
extern UBYTE done;                  // program finished
extern UBYTE wb;                    // running in wb
extern UBYTE icon;                  // iconified
extern UBYTE unicon;                // must uniconify
extern unsigned char buf[2048];
extern unsigned char keys[1520];
extern UWORD WinTop;                // WinTop topEdge (titlebar height)
extern struct Task *parent;

extern UWORD abort_flag;
extern UWORD connect_msg_type;
extern char *connect_string;

extern UWORD cport;	// current tcp port

extern char prefsfile[];
extern char keysfile[];
extern char bookfile[];

extern void *vi;
extern char username[42], password[42];


// Functions exported
BOOL OpenDisplay(BOOL manageScreen);
void CloseDisplay(BOOL manageScreen);
long TCPSend(char *buf, long len);
void CloseIcon(void);                                   // Uniconify the application
void LEDs(void);
void mysprintf(char *Buffer,char *ctl, ...);
void LocalFmt(char *ctl, ...);
void TextFmt(struct RastPort *rP, char *ctl, ...);
void LocalPrint(char *data);
void OpenIcon(void);                                    // inconify the application
void SavePrefs(void);
void SimpleReq(char *str);
UWORD Connect_To_Server(char *servername, UWORD port);

#endif /* DCTELNET_H */
