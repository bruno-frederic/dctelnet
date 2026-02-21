#ifndef GUIS_H
#define GUIS_H

#include <exec/types.h>

// Types
struct MyNewGadget
{
    WORD ng_LeftEdge, ng_TopEdge;       // gadget position
    WORD ng_Width, ng_Height;           // gadget size
    UBYTE *ng_GadgetText;               // gadget label
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
void AddressBook(void);
long mytime(void);
void RefreshListView(UWORD top);
void OpenToolBarWindow(char setmenus);
void CloseToolBarWindow(void);
char MakeGadgets(struct MyNewGadget ProjectNGad[], struct Gadget *ProjectGadgets[], ULONG ProjectGTags[], struct Gadget *g, UBYTE ProjectGTypes[], UWORD Count);

#endif /* GUIS_H */
