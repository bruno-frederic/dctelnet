#ifndef GUIS_H
#define GUIS_H

#include "prefs.h"   // struct PrefsStruct

#include <exec/types.h>

// Types
struct MyNewGadget
{
    WORD ng_LeftEdge, ng_TopEdge;       // gadget position
    WORD ng_Width, ng_Height;           // gadget size
    UBYTE *ng_GadgetText;               // gadget label
};

/**
 * @brief Stable identifiers for toolbar buttons.
 *
 * Each value corresponds to the matching entry in the icons[] array.
 */
enum ToolbarButtonID
{
    BUTTON_CONNECT,
    BUTTON_DISCONNECT,
    BUTTON_ADDRESS_BOOK,
    BUTTON_INFORMATION,
    BUTTON_UPLOAD,
    BUTTON_DOWNLOAD,
    BUTTON_QUIT,

    BUTTON_COUNT
};

// Global variables exported
extern UWORD                 OffX, OffY;
extern APTR Scroller;

// Functions exported
UWORD ComputeX( UWORD value );
UWORD ComputeY( UWORD value );
void CheckDimensions(struct NewWindow *newwin);
void ComputeFont( UWORD width, UWORD height );
void CloseScrollBack(void);
void OpenScrollBack(UWORD sel);
void FunctionKeys(void);
BOOL EditFunctionKeys(TEXT *keys, const char *title);
void AddressBook(void);
struct SiteSettings;
BOOL LoadEntrySettings(ULONG id, struct SiteSettings *out);
BOOL SaveEntrySettings(ULONG id, const struct SiteSettings *settings);
void StampConnectedEntry(void);
void EntryXemOptionsPath(ULONG id, char *path);
void RememberConnectedEntry(const char *name, const char *host, UWORD port);
void ForgetConnectedEntry(void);
enum SaveEntryResult
{
    SAVE_ENTRY_SAVED,
    SAVE_ENTRY_NOT_CONNECTED,       // not connected through the Address Book
    SAVE_ENTRY_NOTHING_CHANGED,     // no group differs and the entry overrides none
    SAVE_ENTRY_WRITE_ERROR
};
enum SaveEntryResult SaveSettingsToConnectedEntry(void);
void RefreshListView(UWORD top);
BOOL FindInScrollBack(ULONG *top);
BOOL EditConnectionOptions(struct PrefsStruct *p);
void OpenToolBarWindow(char setmenus);
void CloseToolBarWindow(void);
char MakeGadgets(struct MyNewGadget ProjectNGad[], struct Gadget *ProjectGadgets[], ULONG ProjectGTags[], struct Gadget *g, UBYTE ProjectGTypes[], UWORD Count);

#endif /* GUIS_H */
