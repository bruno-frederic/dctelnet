/*
 * @file prefs_file.c
 * @brief The DCTelnet.Prefs file as bytes (see prefs_file.h).
 */
#include "prefs_file.h"

// DCTelnet 1.x PrefsStruct: byte offsets of its fields (68000 layout, no padding).
enum
{
    OLD_DISPLAY_ID = 0, OLD_WIDTH = 4, OLD_HEIGHT = 6, OLD_DEPTH = 8, OLD_FONTSIZE = 10,
    OLD_FONTNAME = 12, OLD_DOWNLOADPATH = 44, OLD_XFERLIBRARY = 96, OLD_XFERINIT = 148,
    OLD_COLOR = 200, OLD_FLAGS = 232, OLD_WIN = 236, OLD_SB = 244, OLD_UPLOADPATH = 252,
    OLD_DISPLAYDRIVER = 304, OLD_SB_LINES = 336, OLD_DISPLAYIDSTR = 340, OLD_TOOLBAR = 372,
    OLD_ESSENTIAL = 232,        // screen mode, font, paths and palette
    OLD_V1_SIZE = 376,
    // The builds before v2.0 appended an extension block at 444:
    OLD_REDIAL_TRIES = 444, OLD_REDIAL_DELAY = 445, OLD_ANTI_IDLE = 446, OLD_CONNECT_TIMEOUT = 447,
    OLD_CHARSET = 448, OLD_BELL = 449, OLD_ANSI_MUSIC = 450,
    OLD_EXT_END = 508
};

// DCTelnet 1.x option flags (PrefsStruct.flags).
#define OLD_HIDE_TITLEBAR       (1UL << 0)
#define OLD_CRLF_CORRECTION     (1UL << 1)
#define OLD_HIDE_LEDS           (1UL << 2)
#define OLD_USE_WORKBENCH       (1UL << 3)
#define OLD_BS_DEL_SWAP         (1UL << 4)
#define OLD_DISABLE_SCROLLBACK  (1UL << 5)
#define OLD_PACKET_WINDOW       (1UL << 8)
#define OLD_USE_XEM_LIBRARY     (1UL << 9)
#define OLD_TOOL_BAR            (1UL << 10)
#define OLD_RETURN_CRLF         (1UL << 11)
#define OLD_LOCAL_ECHO          (1UL << 12)
#define OLD_RAW_CONNECTION      (1UL << 13)
#define OLD_JUMP_SCROLL         (1UL << 14)
#define OLD_PETSCII_MODE        (1UL << 15)
#define OLD_WINDOW_SNAPSHOT     (1UL << 16)     // the pre-2.0 PETSCII/RTG builds
#define OLD_VT_KEYS             (1UL << 17)
#define OLD_RLOGIN              (1UL << 18)
#define OLD_132_COLUMNS         (1UL << 19)
#define OLD_SSH                 (1UL << 20)

static UWORD Word(const UBYTE *p) { return (UWORD)(p[0] << 8 | p[1]); }
static ULONG Long(const UBYTE *p) { return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | (ULONG)p[2] << 8 | p[3]; }

static void Text(TEXT *to, size_t size, const UBYTE *from, size_t fromSize)
{
    size_t n = fromSize < size - 1 ? fromSize : size - 1;
    memcpy(to, from, n);
    to[n] = 0;
    to[size - 1] = 0;
}

// The old flags in the current State bits: the "hide" and "disable" options
// became positive ones, and the console (ibmcon.device, the only one 1.x
// had besides XEM) became a renderer choice.
static ULONG StateFromOldFlags(ULONG f)
{
    ULONG s = 0;

    if (!(f & OLD_HIDE_TITLEBAR))       s |= APP_TITLE_BAR_ENABLED;
    if (f & OLD_CRLF_CORRECTION)        s |= APP_INCOMING_LF_TO_CRLF;
    if (!(f & OLD_HIDE_LEDS))           s |= APP_LEDS_ENABLED;
    if (!(f & OLD_USE_WORKBENCH))       s |= APP_FULLSCREEN;
    if (f & OLD_BS_DEL_SWAP)            s |= APP_BACKSPACE_DEL_SWAPPED;
    if (!(f & OLD_DISABLE_SCROLLBACK))  s |= APP_SCROLLBACK_ENABLED;
    if (f & OLD_PACKET_WINDOW)          s |= APP_PACKET_WINDOW_ENABLED;
    s |= (f & OLD_USE_XEM_LIBRARY) ? APP_RENDERER_XEM_LIB : APP_RENDERER_IBMCON_DEVICE;
    if (f & OLD_TOOL_BAR)               s |= APP_TOOL_BAR_ENABLED;
    if (f & OLD_RETURN_CRLF)            s |= APP_RETURN_SENDING_CRLF;
    if (f & OLD_LOCAL_ECHO)             s |= APP_LOCAL_ECHO;
    if (f & OLD_RAW_CONNECTION)         s |= APP_RAW_CONNECTION;
    if (f & OLD_JUMP_SCROLL)            s |= APP_FAST_SCROLL_ENABLED;
    if (f & OLD_PETSCII_MODE)           s |= APP_PETSCII_MODE;
    if (f & OLD_WINDOW_SNAPSHOT)        s |= APP_WINDOW_SNAPSHOT;
    if (f & OLD_VT_KEYS)                s |= APP_VT_KEYS;
    if (f & OLD_RLOGIN)                 s |= APP_RLOGIN;
    if (f & OLD_132_COLUMNS)            s |= APP_132_COLUMNS;
    if (f & OLD_SSH)                    s |= APP_SSH;
    return s;
}

BOOL Prefs_FromLegacy(const UBYTE *old, size_t len, struct PrefsStruct *out)
{
    int i;

    if (len < OLD_ESSENTIAL)
        return FALSE;
    memset(out, 0, sizeof(*out));
    out->DisplayID     = Long(old + OLD_DISPLAY_ID);
    out->DisplayWidth  = Word(old + OLD_WIDTH);
    out->DisplayHeight = Word(old + OLD_HEIGHT);
    out->DisplayDepth  = Word(old + OLD_DEPTH);
    out->FontSize      = Word(old + OLD_FONTSIZE);
    Text(out->FontName,     sizeof(out->FontName),     old + OLD_FONTNAME,     32);
    Text(out->DownloadPath, sizeof(out->DownloadPath), old + OLD_DOWNLOADPATH, 52);
    Text(out->XferLibrary,  sizeof(out->XferLibrary),  old + OLD_XFERLIBRARY,  52);
    Text(out->XferOptions,  sizeof(out->XferOptions),  old + OLD_XFERINIT,     52);
    for (i = 0; i < 16; i++)
        out->DeviceColors[i] = Word(old + OLD_COLOR + 2 * i);
    if (len < OLD_V1_SIZE)          // an older, shorter 1.x file: the rest stays default
        return TRUE;

    out->State = StateFromOldFlags(Long(old + OLD_FLAGS));
    out->MainWinLeftEdge       = (WORD)Word(old + OLD_WIN);
    out->MainWinTopEdge        = (WORD)Word(old + OLD_WIN + 2);
    out->MainWinWidth          = (WORD)Word(old + OLD_WIN + 4);
    out->MainWinHeight         = (WORD)Word(old + OLD_WIN + 6);
    out->ScrollbackWinLeftEdge = (WORD)Word(old + OLD_SB);
    out->ScrollbackWinTopEdge  = (WORD)Word(old + OLD_SB + 2);
    out->ScrollbackWinWidth    = (WORD)Word(old + OLD_SB + 4);
    out->ScrollbackWinHeight   = (WORD)Word(old + OLD_SB + 6);
    Text(out->UploadPath,     sizeof(out->UploadPath),     old + OLD_UPLOADPATH,    52);
    Text(out->XemLibrary,     sizeof(out->XemLibrary),     old + OLD_DISPLAYDRIVER, 32);
    out->nScrollbackLines = Long(old + OLD_SB_LINES);
    Text(out->TelnetTermType, sizeof(out->TelnetTermType), old + OLD_DISPLAYIDSTR,  32);
    out->ToolBarWinLeftEdge    = (WORD)Word(old + OLD_TOOLBAR);
    out->ToolBarWinTopEdge     = (WORD)Word(old + OLD_TOOLBAR + 2);
    if (len < OLD_EXT_END)      // no extension block: its settings stay default
        return TRUE;

    out->RedialTries     = old[OLD_REDIAL_TRIES];
    out->RedialDelay     = old[OLD_REDIAL_DELAY];
    out->AntiIdleMinutes = old[OLD_ANTI_IDLE];
    out->ConnectTimeout  = old[OLD_CONNECT_TIMEOUT];
    out->Charset         = old[OLD_CHARSET];
    out->Bell            = old[OLD_BELL];
    out->AnsiMusic       = old[OLD_ANSI_MUSIC];
    return TRUE;
}

int Prefs_Decode(const UBYTE *file, size_t len, struct PrefsStruct *out)
{
    struct DCTFileHeader hdr;
    size_t n;

    memset(out, 0, sizeof(*out));
    if (len < sizeof(hdr))
        return PREFS_FILE_BAD;
    memcpy(&hdr, file, sizeof(hdr));                            // (file may be unaligned)
    if (memcmp(hdr.magic, "DCTP", 4) != 0)
        return Prefs_FromLegacy(file, len, out) ? PREFS_FILE_LEGACY : PREFS_FILE_BAD;
    if (hdr.version != 2)
        return PREFS_FILE_BAD;
    n = hdr.dataSize;
    if (n > len - sizeof(hdr)) n = len - sizeof(hdr);           // (a file cut short)
    if (n > sizeof(*out))      n = sizeof(*out);
    memcpy(out, file + sizeof(hdr), n);
    return PREFS_FILE_V2;
}

UWORD *Prefs_Palette(struct PrefsStruct *p)
{
    return (p->State & (APP_RENDERER_BUILTIN | APP_RENDERER_XEM_LIB)) ? p->AnsiColors : p->DeviceColors;
}

BOOL Prefs_ScreenFits(const struct PrefsStruct *p)
{
    if (p->State & APP_RENDERER_BUILTIN)
        return p->DisplayWidth == 640 && p->DisplayHeight >= 200 && p->DisplayHeight <= 256
            && p->DisplayDepth == 4;
    if (p->State & APP_RENDERER_IBMCON_DEVICE)             // ibmcon.device crashes at 1920x1200
        return p->DisplayWidth >= WIN_MIN_WIDTH && p->DisplayWidth <= 1920
            && p->DisplayHeight >= WIN_MIN_HEIGHT && p->DisplayHeight <= 1080
            && p->DisplayDepth != 0 && p->DisplayDepth <= 32;
    return p->DisplayWidth >= WIN_MIN_WIDTH && p->DisplayWidth <= DISP_MAX_WIDTH
        && p->DisplayHeight >= WIN_MIN_HEIGHT && p->DisplayHeight <= DISP_MAX_HEIGHT
        && p->DisplayDepth != 0 && p->DisplayDepth <= 32;
}

UWORD Prefs_MaxDepth(const struct PrefsStruct *p)
{
    return (p->State & APP_RENDERER_BUILTIN) ? 4 : 32;
}
