/* src/site_prefs.c -- per-Address-Book-entry settings (issue #10). */
#include "site_prefs.h"
#include "prefs_file.h"
#include "palette.h"
#include "charset.h"

/* Flags whose change needs the screen reopened (OpenAppScreen reads them).
 * APP_PETSCII_MODE is not one: the C64 display is up only during a
 * connection, and DCTelnet.c switches it with the connection itself. */
#define SCREEN_FLAGS  (APP_FULLSCREEN | APP_TITLE_BAR_ENABLED)

/* Flags whose change needs the windows reopened (OpenDisplay reads them). */
#define WINDOW_FLAGS  (APP_PACKET_WINDOW_ENABLED | APP_RENDERER_ALL | APP_TOOL_BAR_ENABLED | APP_FAST_SCROLL_ENABLED)

static void copy_geometry(struct PrefsStruct *to, const struct PrefsStruct *from)
{
    to->MainWinLeftEdge       = from->MainWinLeftEdge;
    to->MainWinTopEdge        = from->MainWinTopEdge;
    to->MainWinWidth          = from->MainWinWidth;
    to->MainWinHeight         = from->MainWinHeight;
    to->ScrollbackWinLeftEdge = from->ScrollbackWinLeftEdge;
    to->ScrollbackWinTopEdge  = from->ScrollbackWinTopEdge;
    to->ScrollbackWinWidth    = from->ScrollbackWinWidth;
    to->ScrollbackWinHeight   = from->ScrollbackWinHeight;
    to->ToolBarWinLeftEdge    = from->ToolBarWinLeftEdge;
    to->ToolBarWinTopEdge     = from->ToolBarWinTopEdge;
}

static void replace_keeping_geometry(struct PrefsStruct *live, const struct PrefsStruct *with)
{
    struct PrefsStruct geometry = *live;

    *live = *with;
    copy_geometry(live, &geometry);
}

#define TERMINAL_GROUP_FLAGS  (APP_PETSCII_MODE | APP_RENDERER_ALL | APP_RAW_CONNECTION | APP_LOCAL_ECHO \
                               | APP_RLOGIN | APP_SSH)
#define KEYBOARD_GROUP_FLAGS  (APP_BACKSPACE_DEL_SWAPPED | APP_RETURN_SENDING_CRLF | APP_VT_KEYS)

static void copy_flags(struct PrefsStruct *to, const struct PrefsStruct *from, ULONG mask)
{
    to->State = (to->State & ~mask) | (from->State & mask);
}

void SitePrefs_ApplyEntry(struct PrefsStruct *live, const struct PrefsStruct *global,
                          const struct SiteSettings *entry)
{
    struct PrefsStruct result = *global;
    const struct PrefsStruct *e = &entry->prefs;

    if (entry->groups & SITE_GROUP_SCREEN)
    {
        result.FontSize      = e->FontSize;
        memcpy(result.FontName, e->FontName, sizeof(result.FontName));
        memcpy(result.AnsiColors, e->AnsiColors, sizeof(result.AnsiColors));
        memcpy(result.DeviceColors, e->DeviceColors, sizeof(result.DeviceColors));
    }
    if (entry->groups & SITE_GROUP_TERMINAL)
    {
        memcpy(result.XemLibrary, e->XemLibrary, sizeof(result.XemLibrary));
        memcpy(result.TelnetTermType, e->TelnetTermType, sizeof(result.TelnetTermType));
        result.Charset = e->Charset;
        copy_flags(&result, e, TERMINAL_GROUP_FLAGS);
    }
    if (entry->groups & SITE_GROUP_KEYBOARD)
        copy_flags(&result, e, KEYBOARD_GROUP_FLAGS);
    if (entry->groups & SITE_GROUP_TRANSFER)
    {
        memcpy(result.XferLibrary, e->XferLibrary, sizeof(result.XferLibrary));
        memcpy(result.XferOptions, e->XferOptions, sizeof(result.XferOptions));
    }
    replace_keeping_geometry(live, &result);
}

void SitePrefs_Restore(struct PrefsStruct *live, const struct PrefsStruct *global)
{
    replace_keeping_geometry(live, global);
}

void SitePrefs_ForSave(struct PrefsStruct *out, const struct PrefsStruct *live,
                       const struct PrefsStruct *global, BOOL sessionActive)
{
    if (sessionActive)
    {
        *out = *global;
        copy_geometry(out, live);
    }
    else
        *out = *live;
}

BOOL SitePrefs_DisplayDiffers(const struct PrefsStruct *a, const struct PrefsStruct *b,
                              BOOL *reopenScreen)
{
    BOOL windows;

    *reopenScreen = a->DisplayID != b->DisplayID
                 || a->DisplayWidth != b->DisplayWidth
                 || a->DisplayHeight != b->DisplayHeight
                 || a->DisplayDepth != b->DisplayDepth
                 || a->FontSize != b->FontSize
                 || strcmp((const char *)a->FontName, (const char *)b->FontName) != 0
                 || ((a->State ^ b->State) & SCREEN_FLAGS) != 0;

    windows = ((a->State ^ b->State) & WINDOW_FLAGS) != 0
           || strcmp((const char *)a->XemLibrary, (const char *)b->XemLibrary) != 0
           || memcmp(a->AnsiColors, b->AnsiColors, sizeof(a->AnsiColors)) != 0
           || memcmp(a->DeviceColors, b->DeviceColors, sizeof(a->DeviceColors)) != 0;

    return *reopenScreen || windows;
}

#include <stddef.h>

/* Every field of struct PrefsStruct but State (carried bit by bit). A field
 * added to the struct is added here, or a change to it is not carried
 * (test_site_prefs covers every byte). */
static const struct { size_t offset, size; } prefsFields[] =
{
#define F(m) { offsetof(struct PrefsStruct, m), sizeof(((struct PrefsStruct *)0)->m) }
    F(AnsiColors), F(DeviceColors),
    F(DisplayID), F(DisplayWidth), F(DisplayHeight), F(DisplayDepth),
    F(FontSize), F(FontName),
    F(MainWinLeftEdge), F(MainWinTopEdge), F(MainWinWidth), F(MainWinHeight),
    F(ScrollbackWinLeftEdge), F(ScrollbackWinTopEdge), F(ScrollbackWinWidth), F(ScrollbackWinHeight),
    F(ToolBarWinLeftEdge), F(ToolBarWinTopEdge),
    F(nScrollbackLines), F(TelnetTermType), F(XemLibrary),
    F(XferLibrary), F(DownloadPath), F(UploadPath), F(XferOptions),
    F(RedialTries), F(RedialDelay), F(AntiIdleMinutes), F(ConnectTimeout), F(Charset),
    F(Bell), F(AnsiMusic), F(Reserved)
#undef F
};

void SitePrefs_CarryChange(struct PrefsStruct *global, const struct PrefsStruct *before,
                           const struct PrefsStruct *after)
{
    const UBYTE *b = (const UBYTE *)before, *a = (const UBYTE *)after;
    UBYTE *g = (UBYTE *)global;
    ULONG changed = before->State ^ after->State;
    size_t i;

    for (i = 0; i < sizeof(prefsFields) / sizeof(prefsFields[0]); i++)
        if (memcmp(b + prefsFields[i].offset, a + prefsFields[i].offset, prefsFields[i].size) != 0)
            memcpy(g + prefsFields[i].offset, a + prefsFields[i].offset, prefsFields[i].size);
    global->State = (global->State & ~changed) | (after->State & changed);
}

void SitePrefs_HandInit(struct SiteHandChanges *h, const struct PrefsStruct *loaded)
{
    h->before = *loaded;
    h->after  = *loaded;
}

void SitePrefs_HandChange(struct SiteHandChanges *h, struct PrefsStruct *global,
                          const struct PrefsStruct *before, const struct PrefsStruct *after,
                          BOOL inSession)
{
    if (inSession)
        SitePrefs_CarryChange(global, before, after);   // the run keeps it
    else
        SitePrefs_CarryChange(&h->before, before, after);   // a real change: saved
    SitePrefs_CarryChange(&h->after, before, after);
}

void SitePrefs_HandForSave(const struct SiteHandChanges *h, struct PrefsStruct *toSave)
{
    SitePrefs_CarryChange(toSave, &h->after, &h->before);
}

BOOL SitePrefs_OnlyLookDiffers(const struct PrefsStruct *a, const struct PrefsStruct *b)
{
    static struct PrefsStruct look;
    BOOL reopenScreen;

    look = *a;
    look.FontSize = b->FontSize;
    memcpy(look.FontName, b->FontName, sizeof(look.FontName));
    memcpy(look.AnsiColors, b->AnsiColors, sizeof(look.AnsiColors));
    memcpy(look.DeviceColors, b->DeviceColors, sizeof(look.DeviceColors));
    return !SitePrefs_DisplayDiffers(&look, b, &reopenScreen);
}

/*
 * Settings files: 'DCTS' (DCTFileHeader, version 1, dataSize = the
 * PrefsStruct size), then groups, PrefsStruct, fKeys, loginMacro. Before
 * v2.0 they held the DCTelnet 1.x PrefsStruct: 'DCS1' (only it), 'DCS2'
 * (+ groups, keys, macro), 'DCS3' and 'DCS4' (the same with the struct 444
 * and 508 bytes long) -- converted by Prefs_FromLegacy.
 */
static const UBYTE MAGIC_OLD[3] = { 'D', 'C', 'S' };
#define OLD_V1_PREFS 376

size_t SitePrefs_Encode(const struct SiteSettings *s, UBYTE *buf, size_t max)
{
    struct DCTFileHeader hdr = { { 'D', 'C', 'T', 'S' }, SITE_FILE_VERSION, sizeof(struct PrefsStruct) };
    UBYTE *p = buf;

    if (max < SITE_FILE_SIZE_MAX)
        return 0;
    memcpy(p, &hdr, sizeof(hdr));                    p += sizeof(hdr);
    memcpy(p, &s->groups, sizeof(s->groups));        p += sizeof(s->groups);
    memcpy(p, &s->prefs, sizeof(s->prefs));          p += sizeof(s->prefs);
    memcpy(p, s->fKeys, sizeof(s->fKeys));           p += sizeof(s->fKeys);
    memcpy(p, s->loginMacro, sizeof(s->loginMacro)); p += sizeof(s->loginMacro);
    return (size_t)(p - buf);
}

/* groups, prefs (prefsLen bytes, converted when legacy), keys, macro from p. */
static BOOL decode_body(struct SiteSettings *out, const UBYTE *p, size_t left, size_t prefsLen,
                        BOOL legacy)
{
    if (left != sizeof(out->groups) + prefsLen + SITE_FKEY_BYTES + SITE_LOGIN_MACRO_SIZE)
        return FALSE;
    memcpy(&out->groups, p, sizeof(out->groups));    p += sizeof(out->groups);
    if (legacy)
    {
        if (!Prefs_FromLegacy(p, prefsLen, &out->prefs))
            return FALSE;
    }
    else
    {
        memset(&out->prefs, 0, sizeof(out->prefs));
        memcpy(&out->prefs, p, prefsLen < sizeof(out->prefs) ? prefsLen : sizeof(out->prefs));
    }
    p += prefsLen;
    memcpy(out->fKeys, p, sizeof(out->fKeys));       p += sizeof(out->fKeys);
    memcpy(out->loginMacro, p, sizeof(out->loginMacro));
    out->loginMacro[sizeof(out->loginMacro) - 1] = 0;
    return TRUE;
}

BOOL SitePrefs_Decode(struct SiteSettings *out, const UBYTE *buf, size_t len,
                      const TEXT *defaultFKeys)
{
    static struct SiteSettings s;
    struct DCTFileHeader hdr;

    if (len < SITE_MAGIC_SIZE)
        return FALSE;
    memset(&s, 0, sizeof(s));
    if (len >= sizeof(hdr) && memcmp(buf, "DCTS", 4) == 0)
    {
        memcpy(&hdr, buf, sizeof(hdr));
        if (hdr.version != SITE_FILE_VERSION
            || !decode_body(&s, buf + sizeof(hdr), len - sizeof(hdr), hdr.dataSize, FALSE))
            return FALSE;
    }
    else if (memcmp(buf, MAGIC_OLD, 3) == 0 && buf[3] == '1')
    {
        /* The first per-entry build's snapshot: every group overridden. */
        if (len != SITE_MAGIC_SIZE + OLD_V1_PREFS
            || !Prefs_FromLegacy(buf + SITE_MAGIC_SIZE, OLD_V1_PREFS, &s.prefs))
            return FALSE;
        s.groups = SITE_GROUPS_ALL;
        memcpy(s.fKeys, defaultFKeys, sizeof(s.fKeys));
    }
    else if (memcmp(buf, MAGIC_OLD, 3) == 0 && buf[3] >= '2' && buf[3] <= '4')
    {
        static const size_t oldPrefs[3] = { 376, 444, 508 };     /* DCS2, DCS3, DCS4 */
        if (!decode_body(&s, buf + SITE_MAGIC_SIZE, len - SITE_MAGIC_SIZE, oldPrefs[buf[3] - '2'], TRUE))
            return FALSE;
    }
    else
        return FALSE;
    Palette_Repair(&s.prefs);      // a 1.x (DCS1-4) entry has only the console palette
    *out = s;
    return TRUE;
}

static size_t append_str(char *out, size_t len, size_t max, const char *str)
{
    while (*str && len < max) out[len++] = *str++;
    return len;
}

size_t SitePrefs_NextMacroSegment(const char **cursor, const char *user, const char *pass,
                                  char *out, size_t max, BOOL *wait,
                                  char *waitText, size_t waitMax)
{
    const char *c = *cursor;
    size_t len = 0;

    *wait = FALSE;
    waitText[0] = 0;
    while (*c && len < max)
    {
        if (c[0] == '\\' && c[1] == 'w' && c[2] == '"')
        {
            size_t n = 0;

            for (c += 3; *c && *c != '"'; c++)          /* \w"text": wait for it */
                if (n + 1 < waitMax) waitText[n++] = *c;
            waitText[n] = 0;
            if (*c == '"') c++;
            break;
        }
        if (c[0] == '\\' && c[1])
        {
            switch (c[1])
            {
                case 'u':  len = append_str(out, len, max, user); break;
                case 'p':  len = append_str(out, len, max, pass); break;
                case 'r':  out[len++] = '\r'; break;
                case '\\': out[len++] = '\\'; break;
                case 'd':  *wait = TRUE; break;
                default:   out[len++] = c[0];
                           if (len < max) out[len++] = c[1];
                           break;
            }
            c += 2;
            if (*wait) break;
        }
        else
            out[len++] = *c++;
    }
    *cursor = c;
    return len;
}

BOOL SitePrefs_SwapInKeys(TEXT *live, TEXT *aside, const struct SiteSettings *entry)
{
    if (!(entry->groups & SITE_GROUP_KEYBOARD))
        return FALSE;
    memcpy(aside, live, SITE_FKEY_BYTES);
    memcpy(live, entry->fKeys, SITE_FKEY_BYTES);
    return TRUE;
}

void SitePrefs_SwapOutKeys(TEXT *live, const TEXT *aside)
{
    memcpy(live, aside, SITE_FKEY_BYTES);
}

static BOOL group_differs(ULONG group, const struct PrefsStruct *a, const struct PrefsStruct *b)
{
    /* Apply b's group onto a copy of a: the group differs iff the result
     * is not a. Uses the one definition of the group members above. */
    static struct PrefsStruct result;
    static struct SiteSettings entry;

    entry.groups = group;
    entry.prefs = *b;
    result = *a;
    SitePrefs_ApplyEntry(&result, a, &entry);
    return memcmp(&result, a, sizeof(result)) != 0;
}

ULONG SitePrefs_DifferingGroups(const struct PrefsStruct *a, const struct PrefsStruct *b,
                                const TEXT *keysA, const TEXT *keysB)
{
    ULONG groups = 0;

    if (group_differs(SITE_GROUP_SCREEN, a, b))   groups |= SITE_GROUP_SCREEN;
    if (group_differs(SITE_GROUP_TERMINAL, a, b)) groups |= SITE_GROUP_TERMINAL;
    if (group_differs(SITE_GROUP_TRANSFER, a, b)) groups |= SITE_GROUP_TRANSFER;
    if (group_differs(SITE_GROUP_KEYBOARD, a, b) || memcmp(keysA, keysB, SITE_FKEY_BYTES) != 0)
        groups |= SITE_GROUP_KEYBOARD;
    return groups;
}

/* Bounded string building for the summaries (no printf: this file also runs
 * on the host and in a 68000 build without a C library formatter). */
static void put(char *out, size_t *len, size_t max, const char *str)
{
    while (*str && *len + 1 < max) out[(*len)++] = *str++;
    out[*len] = 0;
}

static void put_num(char *out, size_t *len, size_t max, ULONG n)
{
    char digits[12];
    int i = sizeof(digits) - 1;

    digits[i] = 0;
    do { digits[--i] = (char)('0' + n % 10); n /= 10; } while (n && i > 0);
    put(out, len, max, &digits[i]);
}

static void put_font(char *out, size_t *len, size_t max, const struct PrefsStruct *p)
{
    char name[sizeof(p->FontName)];
    char *dot;

    memcpy(name, p->FontName, sizeof(name));
    name[sizeof(name) - 1] = 0;
    dot = strrchr(name, '.');
    if (dot && strcmp(dot, ".font") == 0) *dot = 0;
    put(out, len, max, name);
    put(out, len, max, " ");
    put_num(out, len, max, p->FontSize);
}

void SitePrefs_GroupSummary(ULONG group, const struct PrefsStruct *p, char *out, size_t max)
{
    size_t len = 0;

    if (max == 0) return;
    out[0] = 0;
    if (group == SITE_GROUP_SCREEN)
    {
        put_font(out, &len, max, p);
    }
    else if (group == SITE_GROUP_TERMINAL)
    {
        if (p->State & APP_PETSCII_MODE)
            put(out, &len, max, "PETSCII Mode");
        else if (p->State & APP_RENDERER_XEM_LIB)
        {
            put(out, &len, max, "XEM ");
            put(out, &len, max, (const char *)p->XemLibrary);
        }
        else
            put(out, &len, max, "ANSI");
        // The terminal type the board is told (TelnetSendTType): PETSCII Mode
        // always says "PETSCII", whatever Display ID holds.
        put(out, &len, max, ", type ");
        put(out, &len, max, (p->State & APP_PETSCII_MODE) ? "PETSCII" : (const char *)p->TelnetTermType);
        if (p->Charset == CHARSET_UTF8)        put(out, &len, max, ", UTF-8");
        else if (p->Charset == CHARSET_LATIN1) put(out, &len, max, ", Amiga characters");
        if (p->State & APP_RAW_CONNECTION) put(out, &len, max, ", Raw Connection");
        if (p->State & APP_RLOGIN)         put(out, &len, max, ", Rlogin");
        if (p->State & APP_SSH)            put(out, &len, max, ", SSH");
        if (p->State & APP_LOCAL_ECHO)     put(out, &len, max, ", Local Echo");
    }
    else if (group == SITE_GROUP_KEYBOARD)
    {
        if (p->State & APP_BACKSPACE_DEL_SWAPPED) put(out, &len, max, "BS/DEL Swap");
        if (p->State & APP_RETURN_SENDING_CRLF)
        {
            if (len) put(out, &len, max, ", ");
            put(out, &len, max, "Return = CR + LF");
        }
        if (p->State & APP_VT_KEYS)
        {
            if (len) put(out, &len, max, ", ");
            put(out, &len, max, "VT Keys");
        }
        if (!len) put(out, &len, max, "Standard keys");
    }
    else if (group == SITE_GROUP_TRANSFER)
    {
        put(out, &len, max, p->XferLibrary[0] ? (const char *)p->XferLibrary : "No protocol");
        if (p->XferOptions[0])
        {
            put(out, &len, max, ", ");
            put(out, &len, max, (const char *)p->XferOptions);
        }
    }
}
