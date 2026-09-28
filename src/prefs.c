/*
 * @file prefs.c
 * @brief Load, validate, migrate, and save DCTelnet preferences.
 *
 * This module owns the application preferences stored in PROGDIR:DCTelnet.Prefs, including screen,
 * font, window, terminal, colour, and file-transfer settings.
 * It validates data read from disk, converts legacy unversioned preference files, initializes
 * missing values with safe defaults, and loads the optional function-key macro file from
 * PROGDIR:DCTelnet.Keys.
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <graphics/modeid.h>            // PAL_MONITOR_ID, HIRES_KEY
#include "prefs.h"
#include "prefs_file.h"
#include "site_prefs.h"
#include "dctelnet.h"                   // ChooseScreen(), SimpleReq()
#include "utils.h"
#include "requesters.h"

// Global application preferences and runtime state.
// The structure is zero-initialized here; ValidateAndInitPrefs() fills in defaults on first run and
// sanitizes values loaded from the Prefs file.
struct PrefsStruct prefs = { 0 };

TEXT fKeys[F_KEY_COUNT * F_KEY_SIZE];

char const prefsFilename[] = "PROGDIR:DCTelnet.Prefs";
char const bookFilename[]  = "PROGDIR:DCTelnet.Book";
char const keysFilename[]  = "PROGDIR:DCTelnet.Keys";

// prefs.AnsiColors: 16-color CGA/VGA text palette in ANSI order. Convenient for the built-in
// renderer: ANSI SGR color numbers map directly (Black, Red, Green, Yellow, Blue, Magenta, Cyan,
// White), requiring only a subtraction.
// Values: 4 unused bits followed by 4 bits for each colour channel: Red, Green, Blue.
static const UWORD defaultAnsiColors[16] = {
    0x0000,  // 0 : #000 black
    0x0A00,  // 1 : #A00 red
    0x00A0,  // 2 : #0A0 green
    0x0A50,  // 3 : #A50 brown
    0x000A,  // 4 : #00A blue
    0x0A0A,  // 5 : #A0A magenta
    0x00AA,  // 6 : #0AA cyan
    0x0AAA,  // 7 : #AAA dark white = light gray (!= #FFF bright white)

    // Bright variants: the built-in renderer treats a color as bright when atr_bold or
    // atr_blink is set.
    // #555  #F55 #5F5  #FF5  #55F  #F5F  #5FF  #FFF
        0x0555, 0x0F55, 0x05F5, 0x0FF5, 0x055F, 0x0F5F, 0x05FF, 0x0FFF
};

// prefs.DeviceColors: original ibmcon/console.device palette, brighter than ANSI and with less
// contrast between regular and bright color variants
static const UWORD defaultDeviceColors[16] = {
    0x0000,  // 0 : #000 black
    0x0DDD,  // 1 : #DDD dark white = light gray (order differs from ANSI)
    0x00D0,  // 2 : #0D0 green
    0x0DD0,  // 3 : #DD0 yellow
    0x000D,  // 4 : #00D blue
    0x0D0D,  // 5 : #D0D magenta
    0x00DD,  // 6 : #0DD cyan
    0x0D00,  // 7 : #D00 red (order differs from ANSI)

    // brighter :
    // #555  #FFF #5F0  #FF0  #00F  #F0F  #0FF  #F00
        0x0555, 0x0FFF, 0x00F0, 0x0FF0, 0x000F, 0x0F0F, 0x00FF, 0x0F00
};


// Pens are used by Intuition to draw the user interface. Each pen corresponds
// to an entry in the screen's color palette.
// Doc: https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node00EC.html

// Pens in ANSI palette:
const UWORD ansiPens[] = {
    7,  // #AAA DETAILPEN compatible with V34. Used to render text in the screen's title bar.
    4,  // #00A  BLOCKPEN compatible with V34. Used to fill the screen's title bar

    7,  // #AAA TEXTPEN for regular text on BACKGROUNDPEN
    7,  // #AAA bright edge on 3D objects
    6,  // #0AA   dark edge on 3D objects
    4,  // #00A FILLPEN for filling the active window borders and selected gadgets.
    7,  // #A0A FILLTEXTPEN for text rendered over FILLPEN
    0,  // #000 BACKGROUNDPEN for the background color. (Used for requesters in DCTelnet)
    5,  // #A0A HIGHLIGHTTEXTPEN for "special color" or highlighted text on BACKGROUNDPEN

    // Pens used by windows with WFLG_NEWLOOKMENUS set (V39+):
    4,  // #00A Title bar text, menu titles and keyboard shortcut in menus
    7,  // #AAA screen-bar/menus fill
    6,  // #0AA line under screen-bar and menus

    // There's no way to select the text pen for a highlighted menu item; the OS binary-
    // complements the text color instead (HIGHCOMP).

    0xFFFF  // end of palette
};

// Original DCTelnet pens for ibmcon/console.device:
// 0 = #000 black ; 1 = #DDD dark white ; 4 = #00D blue ; 5 = #D0D magenta ;
// 6 = #0DD cyan  ; 7 = #D00 red
const UWORD devicePens[]  = { 1,4, 1,1,6,4,1,0,5, 4,1,6, 0xFFFF };

// Fallback pen table for screens limited to 2 or 4 colors (1 or 2 bitplanes).
//static const UWORD defaultPens[] = { 0xFFFF };
/* TODO palette more sympathic for other modes:
    2 colors : blanc, noir
    4 colors : blanc, noir, rouge, bleu    for my icons and general app them
*/
const UWORD defaultPens[] = {  1,4, 1,1,1,4,1,0,7, 4,1,1, 0xFFFF };


// Default dimensions
#define DISP_DEFAULT_WIDTH   640   // PAL High Res (640x256), no interlaced
#define DISP_DEFAULT_HEIGHT  256
#define DISP_DEFAULT_DEPTH     4   // 4 -> 16 colors
#define WIN_DEFAULT_WIDTH    640
#define WIN_DEFAULT_HEIGHT   200   // 200 = display height in HiRes NTSC

/**
 * @brief Validate the format of a 16-entry RGB4 palette.
 *
 * Amiga RGB4 palette entries use only the low twelve bits: 0x0RGB. The four most significant bits
 * must remain clear, and a palette must contain at least one visible colour. This also rejects the
 * all-zero palette produced by the zero-initialized prefs structure before defaults are applied.
 * The palette is not modified.
 *
 * @return TRUE if every entry uses the RGB4 format, FALSE otherwise.
 */
static BOOL ValidatePalette(const UWORD palette[16])
{
    UWORD i;
    BOOL hasNonZeroColor = FALSE;

    if (palette == NULL)
        return FALSE;

    for (i = 0; i < 16; i++)
    {
        if ((palette[i] & 0xF000) != 0)
            return FALSE;

        if (palette[i] != 0)
            hasNonZeroColor = TRUE;
    }

    return hasNonZeroColor;
}

/**
 * @brief Initialize the prefs structure, replacing any aberrant field with a sensible default.
 *
 * Used both to build the initial default configuration and to sanitize preferences just loaded
 * from disk, since the file is untrusted and may be missing, corrupted, or truncated: every field
 * is checked against its valid range and reset individually rather than discarding the whole
 * structure.
 */
static void ValidateAndInitPrefs(BOOL *userMustChooseAScreenMode)
{
    // prefs.State is initialized here for the first run and is replaced with the default state when
    // an invalid value is loaded from the Prefs file (see the #defines in prefs.h).
    if (prefs.State == 0)
        prefs.State = APP_FULLSCREEN | APP_RENDERER_BUILTIN;

    if (!ValidatePalette(prefs.AnsiColors))
        memcpy(prefs.AnsiColors,   defaultAnsiColors,   sizeof(defaultAnsiColors));

    if (!ValidatePalette(prefs.DeviceColors))
        memcpy(prefs.DeviceColors, defaultDeviceColors, sizeof(defaultDeviceColors));

    // Full-screen geometry / font: bounds depend on the active renderer.
    if (STATE_IS(APP_RENDERER_BUILTIN))
    {
        // built-in renderer is, for now, very strict:
        if (prefs.DisplayWidth  != 640
         || prefs.DisplayHeight <  200 || prefs.DisplayHeight > 256
         || prefs.DisplayDepth  != 4)
        {
            *userMustChooseAScreenMode = TRUE;
        }

        if (prefs.FontSize      != 8)                                 prefs.FontSize      = 8;
    }
    else if (STATE_IS(APP_RENDERER_IBMCON_DEVICE))
    {
        // ibmcon.device crashes at 1920x1200 resolution
        if (prefs.DisplayWidth  < WIN_MIN_WIDTH  || prefs.DisplayWidth  > 1920
         || prefs.DisplayHeight < WIN_MIN_HEIGHT || prefs.DisplayHeight > 1080
         || prefs.DisplayDepth  == 0             || prefs.DisplayDepth  > 32)
        {
            *userMustChooseAScreenMode = TRUE;
        }
    }
    else
    {
        if (prefs.DisplayWidth  < WIN_MIN_WIDTH  || prefs.DisplayWidth  > DISP_MAX_WIDTH
         || prefs.DisplayHeight < WIN_MIN_HEIGHT || prefs.DisplayHeight > DISP_MAX_HEIGHT
         || prefs.DisplayDepth  == 0             || prefs.DisplayDepth  > 32)
        {
            *userMustChooseAScreenMode = TRUE;
        }
    }

    if (prefs.FontSize < 6 || prefs.FontSize > 72)  prefs.FontSize  = 8;

    // Geometry for windows on Workbench
    if (prefs.MainWinLeftEdge < 0              || prefs.MainWinLeftEdge > DISP_MAX_WIDTH)   prefs.MainWinLeftEdge = 0;
    if (prefs.MainWinTopEdge  < 11             || prefs.MainWinTopEdge  > DISP_MAX_HEIGHT)  prefs.MainWinTopEdge  = 11;
    if (prefs.MainWinWidth    < WIN_MIN_WIDTH  || prefs.MainWinWidth    > DISP_MAX_WIDTH)   prefs.MainWinWidth    = WIN_DEFAULT_WIDTH;
    if (prefs.MainWinHeight   < WIN_MIN_HEIGHT || prefs.MainWinHeight   > DISP_MAX_HEIGHT)  prefs.MainWinHeight   = WIN_DEFAULT_HEIGHT;

    if (prefs.ToolBarWinLeftEdge < 0         || prefs.ToolBarWinLeftEdge > DISP_MAX_WIDTH)  prefs.ToolBarWinLeftEdge = 0;
    if (prefs.ToolBarWinTopEdge  < 20        || prefs.ToolBarWinTopEdge  > DISP_MAX_HEIGHT) prefs.ToolBarWinTopEdge  = 20;

    // Geometry for Scrollback window on Workbench AND on custom full-screen:
    if (prefs.ScrollbackWinLeftEdge < 0              || prefs.ScrollbackWinLeftEdge > DISP_MAX_WIDTH)   prefs.ScrollbackWinLeftEdge = 0;
    if (prefs.ScrollbackWinTopEdge  < 12             || prefs.ScrollbackWinTopEdge  > DISP_MAX_HEIGHT)  prefs.ScrollbackWinTopEdge  = 12;
    if (prefs.ScrollbackWinWidth    < WIN_MIN_WIDTH  || prefs.ScrollbackWinWidth    > DISP_MAX_WIDTH)   prefs.ScrollbackWinWidth    = WIN_DEFAULT_WIDTH;
    if (prefs.ScrollbackWinHeight   < WIN_MIN_HEIGHT || prefs.ScrollbackWinHeight   > DISP_MAX_HEIGHT)  prefs.ScrollbackWinHeight   = (prefs.DisplayHeight / 2) - 4;

    // Scrollback line count
    if (prefs.nScrollbackLines < 10 || prefs.nScrollbackLines > 32000)
        prefs.nScrollbackLines = 300;

    // Fixed-size strings: guarantee nul-termination before any further use, then fall back to
    // defaults when empty.
    prefs.FontName[sizeof(prefs.FontName) - 1] = '\0';
    prefs.TelnetTermType[sizeof(prefs.TelnetTermType) - 1] = '\0';
    prefs.XemLibrary[sizeof(prefs.XemLibrary) - 1] = '\0';
    prefs.XferLibrary[sizeof(prefs.XferLibrary) - 1] = '\0';
    prefs.DownloadPath[sizeof(prefs.DownloadPath) - 1] = '\0';
    prefs.UploadPath[sizeof(prefs.UploadPath) - 1] = '\0';
    prefs.XferOptions[sizeof(prefs.XferOptions) - 1] = '\0';

    if (prefs.FontName[0]       == '\0') strlcpy(prefs.FontName,       "topaz.font",                  sizeof(prefs.FontName));
    if (prefs.TelnetTermType[0] == '\0') strlcpy(prefs.TelnetTermType, "VT102",                       sizeof(prefs.TelnetTermType));
    if (prefs.XemLibrary[0]     == '\0') strlcpy(prefs.XemLibrary,     "xemvt340.library",            sizeof(prefs.XemLibrary));
    if (prefs.XferLibrary[0]    == '\0') strlcpy(prefs.XferLibrary,    "xprzmodem.library",           sizeof(prefs.XferLibrary));
    if (prefs.XferOptions[0]    == '\0') strlcpy(prefs.XferOptions,    "TC,OR,B32,FO,AN,DN,KY,SN,RN", sizeof(prefs.XferOptions));
    // DownloadPath / UploadPath may legitimately be empty: no default beyond nul-termination.
}


/**
 * @brief Save the prefs structure to disk.
 *
 * The Prefs file MUST NOT be open before calling this function.
 */
void SavePrefs(void)
{
    LONG lenHeaderWritten = 0, lenPrefsWritten = 0;
    BPTR fileHandle = Open(prefsFilename, MODE_NEWFILE);

    if(fileHandle)
    {
        struct DCTFileHeader header = {
            { 'D','C','T','P' },
            2,            // 2 : DCTelnet v2.0
            sizeof(struct PrefsStruct)
        };

        // Only ever the global settings: during an Address Book entry session
        // the live settings are the entry's, and menu changes made then are
        // session-only (site_prefs.c).
        static struct PrefsStruct toSave;

        SitePrefs_ForSave(&toSave, &prefs, &globalPrefs, sessionSettingsId != 0);
        lenHeaderWritten = Write(fileHandle, &header, sizeof(header));
        lenPrefsWritten  = Write(fileHandle, &toSave, sizeof(toSave));

        Close(fileHandle);
    }

    if(lenHeaderWritten != sizeof(struct DCTFileHeader) || lenPrefsWritten != sizeof(prefs))
        SimpleReq("Error: unable to save the prefs file to disk!");
}

/**
@brief Load application preferences from the prefs file.

Loads preferences from the configured prefs file. If the file does not exist it is created and
initialized with default values.

Calling code must ensure that ReqTools.library is opened before invoking this function.

@return TRUE on success, FALSE on fatal error
*/
BOOL LoadPrefs(void)
{
    LONG len;
    BPTR fileHandle = 0;
    BOOL userMustChooseAScreenMode = FALSE;
    // The whole file: a v2 file of any size (fields are only appended),
    // or a DCTelnet 1.x file converted setting by setting.
    UBYTE *file = ReadWholeFile(prefsFilename, &len, PREFS_FILE_MAX);

    if (! file)
    {
        userMustChooseAScreenMode = TRUE;
    }
    else
    {
        int kind = Prefs_Decode(file, (size_t)len, &prefs);

        FreeVec(file);
        switch (kind)
        {
            case PREFS_FILE_V2:
            case PREFS_FILE_LEGACY:
                break;
            default:
                SimpleReq("Error: DCTelnet.Prefs is damaged or from an unknown version.\n"
                          "Default preferences will be used.");
                userMustChooseAScreenMode = TRUE;
                break;
        }
    }


    // Clear all runtime-only flags loaded from file.
    prefs.State &= STATE_PERSISTENT_MASK;

    // Data comes from disk: reset every aberrant value to a sensible default before it is used.
    ValidateAndInitPrefs(&userMustChooseAScreenMode);

    // A saved INVALID_ID means the previous OpenScreen() attempt failed.
    if (userMustChooseAScreenMode || prefs.DisplayID == (ULONG) INVALID_ID)
    {
        STATE_SET(APP_ICONIFIED);

        InfoReq(NULL,
                "Before DCTelnet can start, you will need to select"    "\n"
                "the screen mode to use. The recommended mode is"    "\n"
                "640x256 with 16 colours, which provides good ANSI"    "\n"
                "terminal emulation."    "\n"
                                        "\n"
                "Once DCTelnet has started, you can configure it to"    "\n"
                "open its window on the Workbench screen instead of"    "\n"
                "using a full-screen display."
            );

        // DisplayID: PAL High Res (640x256), no interlaced
        prefs.DisplayID     = PAL_MONITOR_ID | HIRES_KEY;
        prefs.DisplayWidth  = DISP_DEFAULT_WIDTH;
        prefs.DisplayHeight = DISP_DEFAULT_HEIGHT;
        prefs.DisplayDepth  = DISP_DEFAULT_DEPTH;

        if (! ChooseScreen())
            goto clean_and_fail;
    }


    #ifdef _DEBUG
        if (fileHandle != 0)  SimpleReq("DEBUG ASSERTION: LoadPrefs() left the prefs file open!");
    #endif

    // Loads the macro function keys config file if present:
    fileHandle = Open(keysFilename, MODE_OLDFILE);
    if(fileHandle)
    {
        len = Read(fileHandle, fKeys, sizeof(fKeys));
        Close(fileHandle);  fileHandle = 0;

        if (len != sizeof(fKeys))
        {
            SimpleReq("Error reading the DCTelnet.Keys file");
        }
    }


    // no fatal error:
    if (fileHandle)  Close(fileHandle);
    return TRUE;


clean_and_fail:
    if (fileHandle)  Close(fileHandle);
    return FALSE;
}
