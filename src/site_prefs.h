/* src/site_prefs.h -- per-Address-Book-entry settings (issue #10).
 *
 * `prefs` is the live settings every part of DCTelnet reads. Outside an
 * entry session it is also what DCTelnet.Prefs holds. When the user
 * connects to an Address Book entry that has settings of its own, the live
 * settings become the entry's and the global ones are kept aside; they come
 * back on disconnect, and only they are ever saved. Window geometry is
 * global: where the user put the windows is not part of a board's settings.
 *
 * Pure functions on struct PrefsStruct, unit-tested on the host.
 */
#ifndef SITE_PREFS_H
#define SITE_PREFS_H

#include <string.h>     // size_t
#include "prefs.h"

/*
 * An entry overrides settings by GROUP (Term 4.x style): an overridden group's
 * members come from the entry, every other setting from the global ones.
 *   Screen:   FontName, FontSize, AnsiColors, DeviceColors (the screen mode
 *             and Full-screen are app-wide: an entry never changes where or
 *             in which mode DCTelnet runs)
 *   Terminal: APP_PETSCII_MODE, the renderer (APP_RENDERER_*), XemLibrary,
 *             TelnetTermType, APP_RAW_CONNECTION, APP_LOCAL_ECHO (+ XEM options)
 *   Keyboard: APP_BACKSPACE_DEL_SWAPPED, APP_RETURN_SENDING_CRLF, APP_VT_KEYS (+ function keys)
 *   Transfer: XferLibrary, XferOptions
 * Everything else is app-wide and never comes from an entry.
 */
#define SITE_GROUP_SCREEN    (1UL << 0)
#define SITE_GROUP_TERMINAL  (1UL << 1)
#define SITE_GROUP_KEYBOARD  (1UL << 2)
#define SITE_GROUP_TRANSFER  (1UL << 3)
#define SITE_GROUPS_ALL      (SITE_GROUP_SCREEN | SITE_GROUP_TERMINAL | \
                              SITE_GROUP_KEYBOARD | SITE_GROUP_TRANSFER)

#define SITE_FKEY_BYTES        (10 * 152)   /* F_KEY_COUNT * F_KEY_SIZE (checked in DCTelnet.c) */
#define SITE_LOGIN_MACRO_SIZE  128

/* What one Address Book entry stores besides its BookStruct record. */
struct SiteSettings
{
    ULONG              groups;                         /* SITE_GROUP_* overridden */
    struct PrefsStruct prefs;                          /* values for those groups */
    TEXT               fKeys[SITE_FKEY_BYTES];         /* Keyboard group */
    char               loginMacro[SITE_LOGIN_MACRO_SIZE]; /* entry field, sent after connect */
};

/* live = global with the entry's overridden groups, keeping live's window geometry. */
void SitePrefs_ApplyEntry(struct PrefsStruct *live, const struct PrefsStruct *global,
                          const struct SiteSettings *entry);

/* live = global settings, keeping live's window geometry. */
void SitePrefs_Restore(struct PrefsStruct *live, const struct PrefsStruct *global);

/* What DCTelnet.Prefs must hold: during an entry session the global
 * settings with the live window geometry, otherwise the live settings. */
void SitePrefs_ForSave(struct PrefsStruct *out, const struct PrefsStruct *live,
                       const struct PrefsStruct *global, BOOL sessionActive);

/* A setting the user changed by hand during an entry session (before ->
 * after) is theirs everywhere: every field that changed is carried into
 * global too, whole (a font name is never half copied), and the flags bit
 * by bit. Fields that did not change keep global's value. */
void SitePrefs_CarryChange(struct PrefsStruct *global, const struct PrefsStruct *before,
                           const struct PrefsStruct *after);

/* Settings the user changed by hand while connected to an entry apply for
 * the rest of the run -- a disconnect does not take them back -- but are
 * never saved: DCTelnet.Prefs keeps what they replaced. A later change of
 * the same setting while not connected is a real one and is saved.
 * before: the saved value of every field; after: its value now. */
struct SiteHandChanges
{
    struct PrefsStruct before, after;
};

/* Start the record from the settings as loaded. */
void SitePrefs_HandInit(struct SiteHandChanges *h, const struct PrefsStruct *loaded);

/* A menu pick changed the live settings from before to after. In an entry
 * session the change is carried into global (the run keeps it) and noted
 * as temporary; outside one it is a real change. */
void SitePrefs_HandChange(struct SiteHandChanges *h, struct PrefsStruct *global,
                          const struct PrefsStruct *before, const struct PrefsStruct *after,
                          BOOL inSession);

/* toSave (the global settings) with every temporary change taken back. */
void SitePrefs_HandForSave(const struct SiteHandChanges *h, struct PrefsStruct *toSave);

/* TRUE when a and b differ only in the terminal's look -- the font and the
 * palette. On the Workbench only the console reopens for that; the window
 * stays where it is. */
BOOL SitePrefs_OnlyLookDiffers(const struct PrefsStruct *a, const struct PrefsStruct *b);

/* TRUE when switching between a and b needs the display reopened;
 * *reopenScreen is set when the screen itself must be reopened too, not
 * only the windows (the same split as MENU_SCREEN_FONT vs MENU_TOOL_BAR). */
BOOL SitePrefs_DisplayDiffers(const struct PrefsStruct *a, const struct PrefsStruct *b,
                              BOOL *reopenScreen);

/* An entry's settings file, PROGDIR:Sites/<id>.prefs (site_prefs.c):
 *   DCTFileHeader 'DCTS' version 1, ULONG groups, PrefsStruct, fKeys,
 *   loginMacro (written); a PrefsStruct of another size loads as
 *   DCTelnet.Prefs does (fields are only appended).
 *   'DCS1'..'DCS4': files of the builds before v2.0 (converted).
 */
#define SITE_FILE_VERSION  1
#define SITE_MAGIC_SIZE    4
#define SITE_FILE_SIZE_MAX (sizeof(struct DCTFileHeader) + sizeof(ULONG) + sizeof(struct PrefsStruct) \
                            + SITE_FKEY_BYTES + SITE_LOGIN_MACRO_SIZE)
/* The longest settings file there can be (a 16-bit dataSize): what a
 * reader must accept. SITE_FILE_SIZE_MAX is this build's own, which an
 * old DCS3/DCS4 file (444/508-byte struct) and a newer build's exceed. */
#define SITE_FILE_READ_MAX (sizeof(struct DCTFileHeader) + sizeof(ULONG) + 65535UL \
                            + SITE_FKEY_BYTES + SITE_LOGIN_MACRO_SIZE)

/* Returns the bytes written, 0 when max is too small. */
size_t SitePrefs_Encode(const struct SiteSettings *s, UBYTE *buf, size_t max);

/* FALSE (out untouched) unless buf is one whole valid settings file. A DCS1
 * snapshot decodes as every group overridden, fKeys = defaultFKeys, no macro. */
BOOL SitePrefs_Decode(struct SiteSettings *out, const UBYTE *buf, size_t len,
                      const TEXT *defaultFKeys);

/*
 * Login macro, sent after connecting: \u username, \p password, \r Return,
 * \d wait one second, \\ a backslash; any other \x goes out as written.
 * Returns the next segment to send (in out, NOT NUL-terminated, at most max
 * bytes) and advances *cursor. *wait is TRUE when the segment ended at a \d:
 * the caller waits a second before asking for the next one.
 */
size_t SitePrefs_NextMacroSegment(const char **cursor, const char *user, const char *pass,
                                  char *out, size_t max, BOOL *wait);

/* An entry overriding the Keyboard group: its F1-F10 become live, the live
 * (global) ones are kept in aside. Returns FALSE (nothing moved) otherwise. */
BOOL SitePrefs_SwapInKeys(TEXT *live, TEXT *aside, const struct SiteSettings *entry);
void SitePrefs_SwapOutKeys(TEXT *live, const TEXT *aside);

/* The groups whose members differ between a and b (keysA/keysB are their
 * F1-F10). App-wide settings and window geometry never count. */
ULONG SitePrefs_DifferingGroups(const struct PrefsStruct *a, const struct PrefsStruct *b,
                                const TEXT *keysA, const TEXT *keysB);

/* One-line summary of a group's settings in p, for the settings window.
 * Always NUL-terminated within max. */
void SitePrefs_GroupSummary(ULONG group, const struct PrefsStruct *p, char *out, size_t max);

#endif /* SITE_PREFS_H */
