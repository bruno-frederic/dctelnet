#ifndef DCTELNET_H
#define DCTELNET_H

#include <exec/types.h>
#include <string.h>     // size_t

// Types

// ID of the gadget in top right corner when title bar is hidden in full screen
#define GADGET_SCREEN_TO_BACK  20

/**
 * @brief Stable identifiers for entries in the main menu.
 *
 * Each identifier is stored in the corresponding NewMenu structure's nm_UserData field.
 * Values must remain unique.
 */
enum MenuItemID
{
    MENU_DCTELNET,
        MENU_ABOUT,

        MENU_SCROLLBACK_WIN,
        MENU_ICONIFY,
        MENU_DISPLAY_SPEED_TEST,
        MENU_FINGER,

        MENU_RESET_SCREEN,
        MENU_QUIT,

    MENU_TRANSFER,
        MENU_UPLOAD,

        MENU_DOWNLOAD,

        MENU_ASCII_SEND,

        MENU_DOWNLOAD_PATH,
        MENU_TRANSFER_PROTOCOL,
        MENU_PROTOCOL_OPTIONS,

    MENU_CONNECTION,
        MENU_CONNECT,
        MENU_CONNECT_NEW_INSTANCE,
        MENU_DISCONNECT,

        MENU_ADDRESS_BOOK,

        MENU_INFORMATION,

    MENU_TERMINAL,
        MENU_TELNET_TERM_TYPE,
        MENU_RAW_CONNECTION,
//      MENU_INCOMING_LF_TO_CRLF,
        MENU_PETSCII_MODE,

        MENU_LOCAL_ECHO,
        MENU_BACKSPACE_DEL_SWAP,
        MENU_RETURN_SENDING_CRLF,

        MENU_SCROLLBACK,
        MENU_SCROLLBACK_LINES,
        MENU_FUNCTION_KEYS,

    MENU_DISPLAY,
        MENU_RENDERER,
            MENU_BUILTIN_RENDERER,
            MENU_XEM_LIBRARY,
            MENU_CONSOLE_DEVICE,
            MENU_IBMCON_DEVICE,

            MENU_FAST_SCROLL,
            MENU_XEM_LIBRARY_PATH,
            MENU_XEM_LIB_OPTIONS,

        MENU_FULLSCREEN,
        MENU_LEDS,
        MENU_TITLE_BAR,
        MENU_PACKET_WINDOW,
        MENU_TOOL_BAR,

        MENU_SCREEN_MODE,
        MENU_SCREEN_FONT,
        MENU_SCREEN_PALETTE,
        MENU_SNAPSHOT_WINDOWS,

    MENU_LOGIN,
        MENU_SEND_USERNAME,
        MENU_SEND_PASSWORD,

    MENU_BAR,   // Used several times

    MENU_END
};

// Global variables exported
extern char server[64];
extern long nScrollbackLines;
extern long tcpSocket, nBytesReceived;
extern struct DrawInfo *drawInfo;
extern struct Menu *mainMenuStrip;
extern struct MsgPort *iconPort;
extern struct NewWindow newWin;
extern struct List *scrollbackList;
extern struct Screen *scr;
extern struct TextFont *ansiFont;
extern struct Window *win, *scrollbackWin, *toolBarWin;

extern struct NewGadget newGadget;
extern BOOL shouldQuitApp;          // program finished
extern BOOL shouldUniconify;        // must shouldUniconifyify
// Used in Receive(), xpr_sflush(). Beware: each call to this function destroy the content
extern UBYTE recvBuffer[4096];
extern unsigned char buf[2048];
extern UWORD winTop;                // WinTop topEdge (titlebar height)
extern struct Task *mainTask;       // An AmigaOS Task is roughly equivalent to a thread

// This flag is set by the "Connecting..." window task when the user cancels the operation:
extern BOOL isConnectionAborted;
extern UWORD connectMsgType;
extern char *connectString;

extern UWORD tcpPort;    // current tcp port

extern void *visualInfos;
extern char username[42], password[42];


// Functions exported
BOOL OpenDisplay(void);
void CloseDisplay(BOOL manageScreen);
long TCPSend(const UBYTE *buf, long len);
void CloseIcon(void);                                   // Uniconify the application
void LEDs(void);
void LocalFmt(char *ctl, ...);
void TextFmt(struct RastPort *rP, char *ctl, ...);
void LocalPrint(char *data);
void OpenIcon(void);                                    // inconify the application
void SimpleReq(char *str);
BOOL ChooseScreen(void);
void SetWaitPointer(struct Window * window);
UWORD BeginServerConnection(char *servername, UWORD port);
struct MenuItem *GetMenuItemFromID(enum MenuItemID id);
WORD GetMenuNumberFromID(enum MenuItemID id);

#endif /* DCTELNET_H */
