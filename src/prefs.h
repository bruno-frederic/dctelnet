#ifndef PREFS_H
#define PREFS_H

/*
 * @file prefs.h
 * @brief Public interface for DCTelnet preferences and application state.
 *
 * Defines the persistent preferences structure, file-format header, legacy preferences layout,
 * application state flags, geometry limits, function-key macro storage, and the functions used to
 * load and save preferences.
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <exec/types.h>

// Types

/**
 * @brief Header identifying and describing a configuration file.
 *
 * The magic field identifies the file type:
 * - "DCTP" for application preferences
 * - "DCTB" for the Address Book
 * - "DCTK" for key macros
 *
 * The header is followed by the corresponding data structure.
 */
struct DCTFileHeader
{
    UBYTE magic[4];
    UWORD version;      // Format version
    UWORD dataSize;     // Size of the data structure following the header
};

/**
 * @brief Global application preferences and runtime state.
 *
 * Contains the application's persistent settings together with runtime status flags.
 *
 * Most settings are persisted in the Prefs file, while some flags represent transient runtime state
 * and are not saved.
 *
 * TODO: Identify which settings should be stored in the Address Book for each session.
 *
 *       Initially, instead of modifying the Address Book profile window, consider adding a
 *       "Save options to current session profile" action.
 *
 *       Global settings should have default values.
 */
struct PrefsStruct
{
    // Bit flags defining the application state (see definitions below).
    // Keep in quickly accessible memory as this field is heavily used.
    ULONG State;

    // Color palettes (16 RGB4 entries)
    UWORD AnsiColors[16];
   // ibmcon/console.device palette (brighter than the ANSI palette, with red and white swapped)
    UWORD DeviceColors[16];

    // Screen configuration
    // long, unsigned value when tag attributes SA_DisplayID, SA_Width, SA_Height, SA_Depth
    ULONG DisplayID;
    UWORD DisplayWidth;
    UWORD DisplayHeight;
    UWORD DisplayDepth;

    // Font settings
    UWORD FontSize;     // UWORD in struct TextAttr
    TEXT  FontName[32];

    // Windows geometry (WORD in struct NewWin)
    WORD MainWinLeftEdge;
    WORD MainWinTopEdge;
    WORD MainWinWidth;
    WORD MainWinHeight;
    WORD ScrollbackWinLeftEdge;
    WORD ScrollbackWinTopEdge;
    WORD ScrollbackWinWidth;
    WORD ScrollbackWinHeight;
    WORD ToolBarWinLeftEdge;
    WORD ToolBarWinTopEdge;

    // Number of scrollback lines
    ULONG nScrollbackLines;     // Used when creating the BOOPSI prop gadget "GAD_SCROLLER"

    TEXT  TelnetTermType[32];   // "VT102" ...

    TEXT  XemLibrary[52];       // PROGDIR:Libs/xemvt340.library is 29 chars + \'0'

    // Paths and transfer settings
    TEXT  XferLibrary[52];
    TEXT  DownloadPath[52];
    TEXT  UploadPath[52];

    // New fields for the next file format version.
    // ...
    // Fields are only ever appended (below): an older file loads with the new
    // ones 0, which ValidateAndInitPrefs() turns into their defaults
    // (Prefs_Decode in prefs_file.c).

    // TODO: Consider storing the transfer options in separate fields, as done by XprOptions(),
    //       instead of adding XferOptions here.
    TEXT  XferOptions[52];

    // Connection Options. 0 is each one's default.
    UBYTE RedialTries;          // failed connects tried again; 0: none
    UBYTE RedialDelay;          // seconds between tries; 0: 10
    UBYTE AntiIdleMinutes;      // a keep-alive after this long without a key sent; 0: never
    UBYTE ConnectTimeout;       // seconds a connect may take; 0: the TCP stack's own limit

    UBYTE Charset;              // what the BBS sends: CHARSET_CP437 (0), _LATIN1, _UTF8 (charset.h)
    UBYTE Bell;                 // BEL: BELL_FLASH (0), BELL_SOUND, BELL_OFF
    UBYTE AnsiMusic;            // <>0: play ANSI music (ansimusic.h)

    // Keeps the struct a multiple of 4 bytes with no padding a compiler adds
    // on its own (every byte is a field: site_prefs.c carries them all). A
    // new UBYTE field takes one of these.
    UBYTE Reserved[1];
};

/*
 * Bit flags defining the application state.
 *
 * Flags are stored in PrefsStruct->State.
 * FLAGS_PERSISTENT_MASK defines which flags are saved to disk.
 *
 * The goal is to pack frequently accessed application state into a single 32-bit value, allowing
 * the compiler to keep it in a register and minimize memory accesses to PrefsStruct, especially in
 * performance-critical paths.
 */
#define APP_PACKET_WINDOW_ENABLED    (1UL << 0)  // Rarely accessed (during display initalization)
#define APP_TOOL_BAR_ENABLED         (1UL << 1)  // Rarely accessed (during display initalization)
#define APP_FAST_SCROLL_ENABLED      (1UL << 2)  // Rarely accessed (during ibmcon.device init)
#define APP_FULLSCREEN               (1UL << 3)  // Not used in hot-path

#define APP_BACKSPACE_DEL_SWAPPED    (1UL << 4)  // 1x for each pressed key
#define APP_RETURN_SENDING_CRLF      (1UL << 5)  // 1x for each pressed key
#define APP_LOCAL_ECHO               (1UL << 6)  // 1x for each pressed key
#define APP_RAW_CONNECTION           (1UL << 7)  // Tested 1 x in Receive()
#define APP_SCROLLBACK_ENABLED       (1UL << 8)  // Tested 1 x in Receive()

// Renderer is tested 1 x in ConWrite() which is called 1x for each Receive()
#define APP_RENDERER_BUILTIN         (1UL << 9)
#define APP_RENDERER_CONSOLE_DEVICE  (1UL << 10)
#define APP_RENDERER_XEM_LIB         (1UL << 11)
#define APP_RENDERER_IBMCON_DEVICE   (1UL << 12)

#define APP_RLOGIN                   (1UL << 13)  // Rlogin (RFC 1282): a raw byte stream after the login
                                                  // message, no telnet codes
#define APP_PETSCII_MODE             (1UL << 15)
#define APP_WINDOW_SNAPSHOT          (1UL << 16)  // MainWin* hold a Snapshot Windows size;
                                                  // clear: the Workbench window opens 80x25
#define APP_VT_KEYS                  (1UL << 17)  // Home/End/Page/Insert send VT codes (ESC[1~ ...);
                                                  // clear: ANSI-BBS (ESC[H ...)

#define APP_CUSTOM_SCREEN_OPENED     (1UL << 23)  // Rarely accessed (during display initalization)

// Keep hot flags in bits 24-31 for potentially faster 68000 memory BTST access (to be measured).

// Not implemented yet but would be tested a lot in Receive() : 1x for each line of text received
#define APP_INCOMING_LF_TO_CRLF      (1UL << 24)  // Reserved for future use

#define APP_TITLE_BAR_ENABLED        (1UL << 25)  //  2x in main loop for each Receive() (and 1x in LEDs(), LEDs() is not used in hot-path)
#define APP_LEDS_ENABLED             (1UL << 26)  //  2x in main loop for each Receive)) (and 1x in LEDs(), LEDs() is not used in hot-path)

// prefs.Bell
#define BELL_FLASH 0     // the screen flashes (DisplayBeep)
#define BELL_SOUND 1     // a short tone
#define BELL_OFF   2

// Persist bits 0 through APP_LEDS_ENABLED
#define STATE_PERSISTENT_MASK    ((APP_LEDS_ENABLED << 1) - 1)

// The following flags are runtime-only status flags:

#define APP_ICONIFIED                (1UL << 27)  // Tested 1x in ConWrite(), 1x in main loop (and 1x in LEDs(), LEDs() is not used in hot-path)

// Tested once per received character in the high-frequency Receive() loop.
// Additional detailed states are implemented in DCTelnet-protocol.h.
// Keeping these bits here provides easy access alongside the other application status flags.
#define APP_TELNET_STATE_DATA        (1UL << 28)  // Reserved for future use
#define APP_ZMODEM_IDLE              (1UL << 29)  // Reserved for future use

// Tested once for each character received in term_feed(). Four parser states require two bits.
// term_feed() is inlined into ConWrite(), so the compiler may optimize these tests together with
// the renderer state checks.
#define APP_CSI_PARSER_STATE_1       (1UL << 30)  // Reserved for future use
#define APP_CSI_PARSER_STATE_2       (1UL << 31)  // Reserved for future use


#define APP_RENDERER_ALL \
        ( APP_RENDERER_BUILTIN \
        | APP_RENDERER_CONSOLE_DEVICE \
        | APP_RENDERER_XEM_LIB \
        | APP_RENDERER_IBMCON_DEVICE )


// Set one or more state bits.
#define STATE_SET(bits) \
    (prefs.State |= (bits))

// Clear one or more state bits.
#define STATE_UNSET(bits) \
    (prefs.State &= ~(bits))

// Toggle one or more state bits.
#define STATE_TOGGLE(bits) \
    (prefs.State ^= (bits))

// Test whether one or more state bits are set.
#define STATE_IS(bits) \
    ((prefs.State & (bits)) != 0)

// Test whether all specified state bits are set.
#define STATE_ARE_ALL(bits) \
    ((prefs.State & (bits)) == (bits))

// Test whether none of the specified state bits are set.
#define STATE_IS_NOT(bits) \
    ((prefs.State & (bits)) == 0)


// Sane bounds for windows geometry
#define WIN_MIN_WIDTH     200
#define WIN_MIN_HEIGHT     50
#define DISP_MAX_WIDTH   7680   // UHD 8K
#define DISP_MAX_HEIGHT  4320   // UHD 8K

#define F_KEY_COUNT 10
#define F_KEY_SIZE  152  // 151 chars + '\0'


// Global variables exported
extern struct PrefsStruct prefs;
extern TEXT fKeys[F_KEY_COUNT * F_KEY_SIZE];
extern const char prefsFilename[];
extern const char keysFilename[];
extern const char bookFilename[];
extern const UWORD ansiPens[];
extern const UWORD devicePens[];
extern const UWORD defaultPens[];


// Functions exported
BOOL LoadPrefs(void);
void SavePrefs(void);

#endif /* PREFS_H */
