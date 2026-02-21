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
    ULONG flags;  // ------------>    BIT 0 = Hide Title Bar
    UWORD win_left,                // BIT 1 = CRLF Correction
          win_top,                 // BIT 2 = Hide LEDS
          win_width,               // BIT 3 = Use Workbench
          win_height,              // BIT 4 = BS/DEL Swap
          sb_left,                 // BIT 5 = Disable Logging
          sb_top,                  // BIT 6 = Strip Colour
          sb_width,                // BIT 7 = Simple Negotiation
          sb_height;               // BIT 8 = Packet Window
    char  uploadpath[52],          // BIT 9 = Display Driver
          displaydriver[32];       // BIT 10 = Tool Window
    ULONG sb_lines;                // BIT 11 = Return = CR&LF
    char  displayidstr[32];        // BIT 12 = Local Echoback
    UWORD twin_left,               // BIT 13 = Raw Connection
          twin_top;                // BIT 14 = Jump Scroll
};

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
