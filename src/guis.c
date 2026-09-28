
// DCTelnet - ADDRESS BOOK, EDITPROFILE, FUNCTION KEYS AND SCROLLBACK GUI

#define __USE_SYSBASE

#ifdef __VBCC__
    #pragma dontwarn 306
#endif
#include <proto/exec.h>               // AllocMem(), AddTail(), FreeMem(), WaitPort(), Remove()
#include <proto/dos.h>                // Open(), Close(), FRead(), FWrite()
#include <proto/intuition.h>          // OpenWindow(),CloseWindow(), NewObjectA() but no NewObject()
#include <proto/graphics.h>           // Move(), SetAPen(), Text(), SetFont(), Draw()
#include <proto/gadtools.h>           // LISTVIEW_KIND, BUTTON_KIND, GTLV_Labels...
#include <proto/icon.h>               // GetDiskObjectNew(), FreeDiskObject()
#ifdef __VBCC__
    #pragma popwarn
#endif
#include <string.h>                   // memcpy(), strlen()
#include <ctype.h>                    // tolower(), toupper()
#include "abook.h"                    // required
#include "edit.h"                     // required
#include "guis.h"
#include "DCTelnet.h"
#include "requesters.h"
#include "utils.h"
#include "prefs.h"
#include "site_prefs.h"
#include "charset.h"               // CHARSET_*
#include "prefs_file.h"             // Prefs_Palette()
#include "Xem_wrapper.h"                 // SaveXemOptions()
#include "listsel.h"
#include "textedit.h"
#include "iconpens.h"
#include "screenfont.h"
#include "ansiscan.h"
#include <intuition/sghooks.h>

struct BookStruct
{
    char    name[32];
    char    host[52];
    UWORD    port;
    ULONG   lastConnect;
    char    username[42];
    char    password[42];
    ULONG   settingsId;     // PROGDIR:Sites/<id>.prefs holds this entry's own settings, 0 = global
    char    comment[64];    // free-text note (was reserved space: old versions keep it)
    char    res[14];        // reserved; old versions write it back byte for byte
};

// DCTelnet.Book is a headerless run of 256-byte records: a different size
// would misread every existing book. Fails to compile if a field changes it.
typedef char BookStructMustBe256Bytes[sizeof(struct BookStruct) == 256 ? 1 : -1];

static BOOL EditProfile(struct BookStruct *book, struct List *list);

/*
 * Per-entry settings files (issue #10). An entry that has settings of its own
 * stores an id in BookStruct.settingsId; the settings live in
 * PROGDIR:Sites/<id>.prefs, so DCTelnet.Book keeps its 256-byte record and
 * still loads in versions that know nothing about entry settings.
 */
#define SITES_DIR "PROGDIR:Sites"

static void EntrySettingsPath(ULONG id, char *path)
{
    mysprintf(path, SITES_DIR "/%lu.prefs", id);
}

// An entry's XEM options (Terminal group), in the XEM library's own format.
void EntryXemOptionsPath(ULONG id, char *path)
{
    mysprintf(path, SITES_DIR "/%lu.xem", id);
}

BOOL LoadEntrySettings(ULONG id, struct SiteSettings *out)
{
    char path[40];
    UBYTE *file;
    LONG len;
    BOOL ok;

    if (!id) return FALSE;
    EntrySettingsPath(id, path);
    // The whole file: an old build's (DCS2-4) is longer than this build's
    // own, and a newer build's has fields appended.
    file = ReadWholeFile(path, &len, SITE_FILE_READ_MAX);
    if (!file) return FALSE;
    // A DCTelnet 'DCS1' snapshot has no function keys: it gets the global ones.
    ok = SitePrefs_Decode(out, file, (size_t)len, fKeys);
    FreeVec(file);
    return ok;
}

BOOL SaveEntrySettings(ULONG id, const struct SiteSettings *settings)
{
    static UBYTE fileBuf[SITE_FILE_SIZE_MAX];
    char path[40];
    BPTR fh, lock;
    size_t len;
    BOOL ok = FALSE;

    if (!id) return FALSE;
    lock = Lock(SITES_DIR, SHARED_LOCK);
    if (!lock) lock = CreateDir(SITES_DIR);
    if (!lock) return FALSE;
    UnLock(lock);

    len = SitePrefs_Encode(settings, fileBuf, sizeof(fileBuf));
    EntrySettingsPath(id, path);
    fh = Open(path, MODE_NEWFILE);
    if (fh)
    {
        ok = Write(fh, fileBuf, (LONG)len) == (LONG)len;
        Close(fh);
    }
    return ok;
}

static void DeleteEntrySettings(ULONG id)
{
    char path[40];

    if (!id) return;
    EntrySettingsPath(id, path);
    DeleteFile(path);
    EntryXemOptionsPath(id, path);
    DeleteFile(path);
}

/*
 * The Address Book entry the current connection came from, so
 * Settings > Save Settings to Address Book Entry knows which record to
 * update. Matched in DCTelnet.Book by name, host and port.
 */
static struct
{
    BOOL  valid;
    char  name[32];
    char  host[52];
    UWORD port;
} connectedEntry;

void RememberConnectedEntry(const char *name, const char *host, UWORD port)
{
    strlcpy(connectedEntry.name, name, sizeof(connectedEntry.name));
    strlcpy(connectedEntry.host, host, sizeof(connectedEntry.host));
    connectedEntry.port = port;
    connectedEntry.valid = TRUE;
}

void ForgetConnectedEntry(void)
{
    connectedEntry.valid = FALSE;
}

// Offset of the connected entry's record in DCTelnet.Book (fh positioned at
// the start), -1 if absent. *maxId receives the highest settings id seen.
static LONG FindConnectedRecord(BPTR fh, struct BookStruct *record, ULONG *maxId)
{
    LONG offset = -1, pos = 0;

    *maxId = 0;
    while (Read(fh, record, sizeof(*record)) == sizeof(*record))
    {
        if (record->settingsId > *maxId) *maxId = record->settingsId;
        if (offset < 0 && record->port == connectedEntry.port
            && !strcmp(record->name, connectedEntry.name) && !strcmp(record->host, connectedEntry.host))
            offset = pos;
        pos += sizeof(*record);
    }
    return offset;
}

/*
 * Stamp lastConnect on the connected entry's DCTelnet.Book record. Used when
 * the connect ran after the Address Book window had closed (it waited for the
 * display to reopen), so the in-memory list the window normally updates is gone.
 */
void StampConnectedEntry(void)
{
    static struct BookStruct record;
    ULONG maxId;
    LONG offset;
    BPTR fh;

    if (!connectedEntry.valid) return;
    fh = Open(bookFilename, MODE_OLDFILE);
    if (!fh) return;
    offset = FindConnectedRecord(fh, &record, &maxId);
    if (offset >= 0)
    {
        Seek(fh, offset, OFFSET_BEGINNING);
        if (Read(fh, &record, sizeof(record)) == sizeof(record))
        {
            record.lastConnect = mytime();
            Seek(fh, offset, OFFSET_BEGINNING);
            Write(fh, &record, sizeof(record));
        }
    }
    Close(fh);
}

/*
 * Settings > Save Settings to Address Book Entry: the live settings become
 * the connected entry's own (creating its settings file and id if it had
 * none), and from here on menu changes in this connection are session-only.
 * Returns FALSE when not connected through the Address Book or on a write
 * error.
 */
enum SaveEntryResult SaveSettingsToConnectedEntry(void)
{
    static struct BookStruct record;
    static struct SiteSettings settings;
    enum SaveEntryResult result = SAVE_ENTRY_WRITE_ERROR;
    BPTR fh;
    LONG offset;
    ULONG maxId, id = 0;

    if (!connectedEntry.valid) return SAVE_ENTRY_NOT_CONNECTED;
    fh = Open(bookFilename, MODE_OLDFILE);
    if (!fh) return SAVE_ENTRY_WRITE_ERROR;
    offset = FindConnectedRecord(fh, &record, &maxId);
    if (offset < 0)
    {
        Close(fh);
        return SAVE_ENTRY_NOT_CONNECTED;       // the entry was deleted meanwhile
    }
    Seek(fh, offset, OFFSET_BEGINNING);
    if (Read(fh, &record, sizeof(record)) == sizeof(record))
        id = record.settingsId;

    // The groups changed since connecting, plus the ones the entry already
    // overrides; the entry's login macro is kept.
    memset(&settings, 0, sizeof(settings));
    LoadEntrySettings(id, &settings);
    settings.groups |= SitePrefs_DifferingGroups(ConnectBaseSettings(), &prefs,
                                                 ConnectBaseFKeys(), fKeys);
    if (!settings.groups)
    {
        Close(fh);
        return SAVE_ENTRY_NOTHING_CHANGED;
    }
    settings.prefs = prefs;
    memcpy(settings.fKeys, fKeys, sizeof(settings.fKeys));

    if (!id) id = maxId + 1;
    if (SaveEntrySettings(id, &settings))
    {
        if (settings.groups & SITE_GROUP_TERMINAL)
        {
            char xemPath[40];

            EntryXemOptionsPath(id, xemPath);
            SaveXemOptions(xemPath);            // no-op when XEM is not in use
        }
        if (record.settingsId != id)
        {
            record.settingsId = id;
            Seek(fh, offset, OFFSET_BEGINNING);
            Write(fh, &record, sizeof(record));
        }
        result = SAVE_ENTRY_SAVED;
    }
    Close(fh);

    // From here on this connection is the entry's session: later menu changes
    // stay out of the global settings, which go back to how they were.
    if (result == SAVE_ENTRY_SAVED) AdoptEntrySession(id, settings.groups);
    return result;
}



static struct Window         *aBookWnd;           // "Address Book" window
static struct Gadget         *aBookGList;         // "Address Book" window GList
static struct Gadget         *aBookGadgets[6];    // "Address Book" window Gadgets
#define aBookWidth 420
#define aBookHeight 132
static struct TextAttr        Attr;
static UWORD                  FontX, FontY;
UWORD                  OffX, OffY;

static ULONG lastsec, lasttic;

static UBYTE *SORT0Labels[] = {
    (UBYTE *)"Recent Connect (first)",
    (UBYTE *)"Oldest Connect (first)",
    (UBYTE *)"Name (A-Z)",
    (UBYTE *)"Name (Z-A)",
    NULL
};


static UBYTE aBookGTypes[] = {
    LISTVIEW_KIND,
    BUTTON_KIND,
    BUTTON_KIND,
    BUTTON_KIND,
    CYCLE_KIND,
    BUTTON_KIND
};


static struct MyNewGadget aBookNGad[] = {
    10, 5, 401, 72, NULL,
    19, 84, 93, 13, (UBYTE *)"_Connect",
    115, 84, 93, 13, (UBYTE *)"_Edit",
    212, 84, 93, 13, (UBYTE *)"_Add",
    160, 103, 203, 13, (UBYTE *)"List Sorted By:",
    308, 84, 93, 13, (UBYTE *)"_Delete",
};

static ULONG aBookGTags[] = {
    GTLV_Labels, 0, (GTLV_ShowSelected), (ULONG)NULL, GTLV_Selected, 0, (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    (GTCY_Labels), (ULONG)&SORT0Labels[ 0 ], (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE)
};

UWORD ComputeX( UWORD value )
{
    return(( UWORD )((( FontX * value ) + 4 ) / 8 ));
}

UWORD ComputeY( UWORD value )
{
    return(( UWORD )((( FontY * value ) + 4 ) / 8 ));
}

char MakeGadgets(struct MyNewGadget ProjectNGad[], struct Gadget *ProjectGadgets[], ULONG ProjectGTags[], struct Gadget *g, UBYTE ProjectGTypes[], UWORD Count)
{
    UWORD lc, tc;

    newGadget.ng_VisualInfo = visualInfos;
    newGadget.ng_TextAttr   = &Attr;

    for( lc = 0, tc = 0; lc < Count; lc++ )
    {
        //CopyMem((char * )&ProjectNGad[ lc ], (char * )&ng, (long)sizeof( struct MyNewGadget ));

        memcpy((char * )&newGadget, (char * )&ProjectNGad[ lc ], sizeof( struct MyNewGadget ));

        newGadget.ng_GadgetID     = lc;
        newGadget.ng_LeftEdge   = OffX + ComputeX( newGadget.ng_LeftEdge );
        newGadget.ng_TopEdge    = OffY + ComputeY( newGadget.ng_TopEdge );
        newGadget.ng_Width      = ComputeX( newGadget.ng_Width );
        newGadget.ng_Height     = ComputeY( newGadget.ng_Height);

        ProjectGadgets[ lc ] = g = CreateGadgetA((ULONG)ProjectGTypes[ lc ], g, &newGadget, ( struct TagItem * )&ProjectGTags[ tc ] );

        while( ProjectGTags[ tc ] ) tc += 2;
        tc++;

        if ( NOT g ) return( 2 );
    }
    return(0);
}


void ComputeFont( UWORD width, UWORD height )
{
    //Font = &Attr;
    Attr.ta_Name = (STRPTR)scr->RastPort.Font->tf_Message.mn_Node.ln_Name;
    Attr.ta_YSize = FontY = scr->RastPort.Font->tf_YSize;
    FontX = scr->RastPort.Font->tf_XSize;

    OffX = scr->WBorLeft;
    OffY = scr->RastPort.TxHeight + scr->WBorTop + 1;

    if (( ComputeX( width ) + OffX + scr->WBorRight ) > scr->Width )
        goto UseTopaz;
    if (( ComputeY( height ) + OffY + scr->WBorBottom ) > scr->Height )
        goto UseTopaz;
    return;

UseTopaz:
    Attr.ta_Name = "topaz.font";
    FontX = FontY = Attr.ta_YSize = 8;
}

static int OpenABookWindow( void )
{
    struct Gadget    *g;
    UWORD        ww, wh;
    long    x,y;

    ComputeFont( aBookWidth, aBookHeight );

    ww = ComputeX( aBookWidth );
    wh = ComputeY( aBookHeight );

    if ( ! ( g = CreateContext( &aBookGList )))
        return( 1L );

    if(MakeGadgets(aBookNGad, aBookGadgets, aBookGTags, g, aBookGTypes, aBook_CNT) != 0) return( 2L );

    x = ww + OffX + scr->WBorRight;
    y = wh + OffY + scr->WBorBottom;

    newWin.LeftEdge = (scr->Width - x) / 2;
    newWin.TopEdge = (scr->Height - y) / 2;
    newWin.Width = x;
    newWin.Height = y;
    newWin.IDCMPFlags = LISTVIEWIDCMP|BUTTONIDCMP|CYCLEIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY;
    newWin.Flags = WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP;
    newWin.FirstGadget = aBookGList;
    newWin.Title = "DCTelnet: Address Book";

    aBookWnd = OpenWindow(&newWin);
    if(!aBookWnd) return( 4L );

/*    if ( ! ( aBookWnd = OpenWindowTags( NULL,
                WA_Left,    (scr->Width - x) / 2,
                WA_Top,        (scr->Height - y) / 2,
                WA_Width,    x,
                WA_Height,    y,
                WA_IDCMP,    LISTVIEWIDCMP|BUTTONIDCMP|CYCLEIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY,
                WA_Flags,    WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP,
                WA_Gadgets,    aBookGList,
                WA_Title,    "DCTelnet: Address Book",
                WA_CustomScreen,scr,
                TAG_DONE )))
    return( 4L );*/

    GT_RefreshWindow( aBookWnd, NULL );

    ComputeFont( aBookWidth, aBookHeight );

    DrawBevelBox( aBookWnd->RPort, OffX + ComputeX( 11 ),
                    OffY + ComputeY( 80 ),
                    ComputeX( 399 ),
                    ComputeY( 43 ),
                    GT_VisualInfo, visualInfos, GTBB_Recessed, TRUE, TAG_DONE );
    DrawBevelBox( aBookWnd->RPort, OffX + ComputeX( 3 ),
                    OffY + ComputeY( 2 ),
                    ComputeX( 415 ),
                    ComputeY( 128 ),
                    GT_VisualInfo, visualInfos, TAG_DONE );

    return( 0L );
}


static struct Node *FindNode(struct List *listviewlist, UWORD lastcode)
{
    struct Node *worknode;
    UWORD i = 0;

    // BF: Why this test? lastcode is UWORD, how could it be negative?
    if(/*lastcode != -1  &&*/  listviewlist->lh_TailPred != (struct Node *)listviewlist)
    {
        worknode = listviewlist->lh_Head;
        while(i < lastcode  &&  worknode)
        {
            i++;
            worknode = worknode->ln_Succ;
        }
        return(worknode);
    }
    return(0);
}


/**
 * @brief Sorts the bookmark list.
 *
 * Sorts the specified bookmark list according to the selected sort mode.
 *
 * @param list      Pointer to the bookmark list to sort.
 * @param sortMode  Sort mode (0..3) :  index in the SORT0Labels[] array
 */
static void SortABook(struct List *list, UWORD sortMode)
{
    register char *temp;
    struct Node *worknode, *nextnode;
    struct Node *inworknode, *innextnode;
    BOOL swap;
    const BOOL sortByLastConnect = (sortMode <= 1);
    const BOOL reverseSort = (sortMode == 1 || sortMode == 3);

    worknode = list->lh_Head;
    while (worknode)
    {
        nextnode = worknode->ln_Succ;
        if (! nextnode)
            break;

        inworknode = list->lh_Head;
        while (inworknode)
        {
            innextnode = inworknode->ln_Succ;
            if (! innextnode)
                break;

            swap = FALSE;

            if (sortByLastConnect)
            {
                ULONG a = ((struct BookStruct *)inworknode->ln_Name)->lastConnect;
                ULONG b = ((struct BookStruct *)worknode->ln_Name)->lastConnect;

                if (reverseSort)  // Oldest first
                {
                    if(a > b)  swap = TRUE;
                }
                else              // Most recent first
                {
                    if(a < b)  swap = TRUE;
                }
            }
            else
            {
                int cmp = stricmp(inworknode->ln_Name, worknode->ln_Name);

                if (reverseSort)  // Z -> A
                {
                    if (cmp < 0)  swap = TRUE;
                }
                else              // A -> Z
                {
                    if (cmp > 0)  swap = TRUE;
                }
            }

            if (swap)
            {
                temp = inworknode->ln_Name;
                inworknode->ln_Name = worknode->ln_Name;
                worknode->ln_Name = temp;
            }

            inworknode = innextnode;
        }

        worknode = nextnode;
    }
}

/**
 * Opens and manages the Address Book window.
 *
 * This function:
 * - Loads the address book entries from disk
 * - Displays them in a listview
 * - Allows the user to connect, add, edit, delete and sort entries
 * - Optionally initiates a connection to the selected host
 * - Saves modifications back to disk before exiting
 *
 * The function is modal and returns only when the user
 * closes the Address Book window or initiates a connection.
 */
void AddressBook(void)
{
    struct IntuiMessage *message;
    struct Gadget *gad;
    struct BookStruct *book, *conbook=NULL;
    struct List *listviewlist;
    struct Node *worknode, *nextnode;
    UWORD lastcode = 0;
    UWORD code;
    ULONG class;
    ULONG sec, tic;
    BPTR fh;
    char readfin = FALSE;
    char ret = FALSE;
    char save = FALSE;
    char subdone = FALSE;

    listviewlist = AllocMem(sizeof(struct List), MEMF_PUBLIC|MEMF_CLEAR);
    if(!listviewlist) return;
    listviewlist->lh_TailPred = (struct Node *)listviewlist;
    listviewlist->lh_Head = (struct Node *)&listviewlist->lh_Tail;

    // Load existing address book entries from disk
    fh = Open(bookFilename, MODE_OLDFILE);
    if(fh)
    {
        while(!readfin)
        {
            book = AllocMem(sizeof(struct BookStruct), MEMF_PUBLIC);
            if(book)
            {
                // Read one address book entry
                if(FRead(fh, book, sizeof(struct BookStruct), 1))
                {
                    // Create a list node pointing to this entry
                    worknode = AllocMem(sizeof(struct Node), MEMF_PUBLIC|MEMF_CLEAR);
                    if(worknode)
                    {
                        worknode->ln_Name = book->name;
                        AddTail(listviewlist, worknode);
                    } else {
                        FreeMem(book, sizeof(struct BookStruct));
                        readfin = TRUE;
                    }
                } else {
                    FreeMem(book, sizeof(struct BookStruct));
                    readfin = TRUE;
                }
            } else {
                readfin = TRUE;
            }
        }
        Close(fh);
    }

    // Sort using the default Address Book sort order
    SortABook(listviewlist, 0);

    // Attach the list to the listview gadget
    aBookGTags[1] = (unsigned long)listviewlist;

    // Open the Address Book window
    if(OpenABookWindow() == RETURN_OK)
    {
        //GT_SetGadgetAttrs(aBookGadgets[GD_LIST],aBookWnd,0,GTLV_Labels,listviewlist,TAG_DONE);
        // Main event loop
        while(!subdone)
        {
            WaitPort(aBookWnd->UserPort);
            while (message = GT_GetIMsg(aBookWnd->UserPort))
            {
                    gad   = (struct Gadget *)message->IAddress;
                class = message->Class;
                code  = message->Code;
                GT_ReplyIMsg(message);
                switch (class)
                {
                case IDCMP_VANILLAKEY:  // Keyboard shortcuts
                    switch(toupper(code))
                    {
                        // Directly jump to the part that manage the required button
                        case 'C':
                            goto connect;
                        case 'E':
                            goto edit;
                        case 'A':
                            goto add;
                        case 'D':
                            goto delete;
                    }
                    break;

                case IDCMP_CLOSEWINDOW:     // User clicked the close gadget
                    subdone = TRUE;
                    break;

                // A action button has been pressed in the Address Book:
                case IDCMP_GADGETUP:
                    switch(gad->GadgetID)
                    {
                    case GD_SORT:       //  Sorting mode changed
                        SortABook(listviewlist, code);

                        // Refresh the ListView to reflect the new sort order while preserving the
                        // current selection index.
                        GT_SetGadgetAttrs(aBookGadgets[GD_LIST], aBookWnd, NULL,
                                          GTLV_Labels,   listviewlist,
                                          GTLV_Selected, lastcode,
                                          TAG_DONE);
                        break;

                    case GD_LIST:       // Item selected in listview or double-clicked
                        CurrentTime(&sec, &tic);
                        if(DoubleClick(lastsec, lasttic, sec, tic) && lastcode == code)
                        {
                            lastcode = code;
                            goto connect;
                        }
                        lastsec = sec;
                        lasttic = tic;
                        lastcode = code;
                        break;

                    case GD_CONNECT:     // Connect to selected entry
connect:
                        if(worknode = FindNode(listviewlist, lastcode))
                        {
                            conbook = (struct BookStruct *)worknode->ln_Name;
                            subdone = TRUE;
                            ret = TRUE;
                            save = TRUE;
                        }
                        break;

                    case GD_DELETE:      // Delete selected entry
delete:
                        if(worknode = FindNode(listviewlist, lastcode))
                        {
                            if (ConfirmRequester(win,"Delete|Cancel",
                                                         "Delete \"%s\"?", worknode->ln_Name))
                            //mysprintf(buf, "Delete \042%s\042?", worknode->ln_Name);
                            //if(rtEZRequestA(buf, "Delete|Cancel", NULL, NULL, (struct TagItem *)&reqtoolsTags))
                            {
                                struct Node *n;
                                UWORD remaining = 0;

                                // GadTools must not see the list while it changes.
                                GT_SetGadgetAttrs(aBookGadgets[GD_LIST], aBookWnd, NULL,
                                                  GTLV_Labels, ~0UL, TAG_DONE);
                                Remove(worknode);
                                DeleteEntrySettings(((struct BookStruct *)worknode->ln_Name)->settingsId);
                                FreeMem(worknode->ln_Name, sizeof(struct BookStruct));
                                FreeMem(worknode, sizeof(struct Node));
                                for (n = listviewlist->lh_Head; n->ln_Succ; n = n->ln_Succ)
                                    remaining++;
                                // lastcode-- turned row 0 into 65535 (UWORD): nothing
                                // selected, and the next Delete did nothing.
                                lastcode = ListSel_AfterDelete(lastcode, remaining);
                                GT_SetGadgetAttrs(aBookGadgets[GD_LIST],aBookWnd,0,GTLV_Labels,listviewlist,GTLV_Selected,lastcode,TAG_DONE);
                                save = TRUE;
                            }
                        }
                        break;

                    case GD_EDIT:        // Edit selected entry
edit:
                        if(worknode = FindNode(listviewlist, lastcode))
                        {
                            if(EditProfile((struct BookStruct *)worknode->ln_Name, listviewlist))
                            {
                                GT_SetGadgetAttrs(aBookGadgets[GD_LIST],aBookWnd,0,GTLV_Labels,listviewlist,GTLV_Selected,lastcode,TAG_DONE);
                                save = TRUE;
                            }
                        }
                        break;

                    case GD_NEW:        // Add a new entry
add:
                        book = AllocMem(sizeof(struct BookStruct), MEMF_PUBLIC|MEMF_CLEAR);
                        if(book)
                        {
                            strlcpy(book->name, "*new site*", sizeof(book->name));
                            strlcpy(book->host, "*ip/host here*", sizeof(book->host));
                            book->port = 23;
                            if(EditProfile(book, listviewlist))
                            {
                                worknode = AllocMem(sizeof(struct Node), MEMF_PUBLIC|MEMF_CLEAR);
                                if(worknode)
                                {
                                    worknode->ln_Name = book->name;
                                    AddTail(listviewlist, worknode);
                                    GT_SetGadgetAttrs(aBookGadgets[GD_LIST],aBookWnd,0,GTLV_Labels,listviewlist,GTLV_Selected,lastcode,TAG_DONE);
                                    save = TRUE;
                                } else
                                    FreeMem(book, sizeof(struct BookStruct));
                            } else
                                FreeMem(book, sizeof(struct BookStruct));
                        }
                        break;
                    }
                }
            }
        }
    }

    if ( aBookWnd ) CloseWindow( aBookWnd );

    if ( aBookGList ) FreeGadgets( aBookGList );

    // Initiate connection if requested
    if(ret)
    {
        static struct SiteSettings entrySettings;
        BOOL hasFile = LoadEntrySettings(conbook->settingsId, &entrySettings);
        BOOL hasSettings = hasFile && entrySettings.groups != 0;
        const char *loginMacro = hasFile ? entrySettings.loginMacro : "";

        // When the settings for this connect change the display (the entry's own,
        // or the global ones coming back after another entry's session), the
        // display must be reopened BEFORE connecting, which only the main loop
        // can do -- this runs inside GetWindowMsg(win). The connect is queued and
        // runs right after the reopen.
        // Every connect runs from the main loop (RunPendingConnect): after
        // the display reopen, and again on a redial.
        BeginEntrySession(hasSettings ? conbook->settingsId : 0,
                          hasSettings ? &entrySettings : NULL);
        DeferConnect(conbook->name, conbook->host, conbook->port, conbook->settingsId,
                     conbook->username, conbook->password, loginMacro);
    }

    // Save address book back to disk if modified
    if(save) fh = Open(bookFilename, MODE_NEWFILE); else fh = 0;

    worknode = listviewlist -> lh_Head;
    while(1)
    {
        nextnode = worknode -> ln_Succ;
        if(!nextnode) break;

        if(fh) FWrite(fh, worknode->ln_Name, sizeof(struct BookStruct), 1);

        FreeMem(worknode->ln_Name, sizeof(struct BookStruct));
        FreeMem(worknode, sizeof(struct Node));
        worknode = nextnode;
    }
    FreeMem(listviewlist, sizeof(struct List));
    if(fh) Close(fh);
}


static struct Window         *editProfileWnd;           // "Edit Address Book Profile" window
static struct Gadget         *editProfileGList;         // "Edit Address Book Profile" window GList
static struct Gadget         *editProfileGadgets[editProfile_CNT]; // "Edit Address Book Profile" window gadgets
#define editProfileWidth 450
#define editProfileHeight 185

static UBYTE editProfileGTypes[] = {
    STRING_KIND,
    STRING_KIND,
    TEXT_KIND,
    BUTTON_KIND,
    BUTTON_KIND,
    INTEGER_KIND,
    STRING_KIND,
    STRING_KIND,
    TEXT_KIND,
    BUTTON_KIND,
    BUTTON_KIND,
    STRING_KIND,
    STRING_KIND,
    TEXT_KIND
};

static struct MyNewGadget editProfileNGad[] = {
    120, 5, 317, 13, (UBYTE *)"_Site Name:",
    120, 21, 317, 13, (UBYTE *)"_Address:",
    121, 37, 177, 13, (UBYTE *)"Last Called:",
    3, 170, 101, 13, (UBYTE *)"_Ok",
    345, 170, 101, 13, (UBYTE *)"_Cancel",
    365, 37, 72, 13, (UBYTE *)"_Port:",
    120, 53, 317, 13, (UBYTE *)"_Username:",
    120, 68, 317, 13, (UBYTE *)"Pass_word:",
    120, 134, 317, 13, (UBYTE *)"Settings:",
    3, 150, 218, 13, (UBYTE *)"Se_ttings...",
    228, 150, 218, 13, (UBYTE *)"Use _Global Settings",
    120, 84, 317, 13, (UBYTE *)"Co_mment:",
    120, 100, 317, 13, (UBYTE *)"_Login Macro:",
    3, 114, 443, 12, NULL,
};

static ULONG editProfileGTags[] = {
    GTST_String, 0, (GTST_MaxChars), 31, (GT_Underscore), '_', (TAG_DONE),
    GTST_String, 0, (GTST_MaxChars), 51, (GT_Underscore), '_', (TAG_DONE),
    GTTX_Text, 0, (GTTX_Border), TRUE, (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    (GTIN_Number), 0, (GTIN_MaxChars), 9, (GT_Underscore), '_', (TAG_DONE),
    GTST_String, 0, (GTST_MaxChars), 41, (GT_Underscore), '_', (TAG_DONE),
    GTST_String, 0, (GTST_MaxChars), 41, (GT_Underscore), '_', (TAG_DONE),
    GTTX_Text, 0, (GTTX_Border), TRUE, (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    GTST_String, 0, (GTST_MaxChars), 63, (GT_Underscore), '_', (TAG_DONE),
    GTST_String, 0, (GTST_MaxChars), 127, (GT_Underscore), '_', (TAG_DONE),
    GTTX_Text, 0, (TAG_DONE)
};

// Where each gadget's initial value sits in editProfileGTags[] above (the
// value slot after GTST_String / GTTX_Text / GTIN_Number). Keep in step with
// the array when gadgets are added.
#define EP_TAG_SITE      1
#define EP_TAG_ADDRESS   8
#define EP_TAG_LAST     15
#define EP_TAG_PORT     26
#define EP_TAG_USERNAME 33
#define EP_TAG_PASSWORD 40
#define EP_TAG_SETTINGS 47
#define EP_TAG_COMMENT  58
#define EP_TAG_LOGIN_MACRO 65
#define EP_TAG_MACRO_HELP  72

/*
 * A field entered with Tab is replaced by what is typed (string edit hook):
 * GadTools string gadgets have no select-all, and typing into a prefilled
 * field appended to the old value. The first key in a field other than the
 * one last typed or clicked in replaces its value; tabbing through keeps
 * it, and a click positions the cursor for editing as before.
 */
static struct Gadget *lastEditedField;

static ULONG __SAVE_DS__ __ASM__ ReplaceOnFirstKey(__REG__(a0, struct Hook *hook),
                                                   __REG__(a2, struct SGWork *sgw),
                                                   __REG__(a1, ULONG *msg))
{
    BOOL fresh;

    if (*msg == SGH_CLICK)
    {
        lastEditedField = sgw->Gadget;      // clicked in: edit where the cursor is
        return ~0UL;
    }
    if (*msg != SGH_KEY)
        return 0;
    fresh = sgw->Gadget != lastEditedField;
    lastEditedField = sgw->Gadget;
    if (fresh && (sgw->EditOp == EO_INSERTCHAR || sgw->EditOp == EO_REPLACECHAR))
    {
        TextEdit_FirstKey((char *)sgw->WorkBuffer, &sgw->BufferPos, &sgw->NumChars, TRUE);
        if (sgw->StringInfo && (sgw->Gadget->Activation & GACT_LONGINT))
            sgw->LongInt = (sgw->WorkBuffer[0] >= '0' && sgw->WorkBuffer[0] <= '9')
                         ? sgw->WorkBuffer[0] - '0' : 0;
        sgw->Actions |= SGA_REDISPLAY;
    }
    return ~0UL;
}

static struct Hook replaceOnFirstKey = { { NULL, NULL }, (HOOKFUNC)ReplaceOnFirstKey, NULL, NULL };

// Put the hook on every string and integer gadget of a window's gadgets.
static void InstallReplaceOnFirstKey(struct Gadget **gads, UWORD count)
{
    UWORD i;

    lastEditedField = NULL;
    for (i = 0; i < count; i++)
    {
        struct Gadget *g = gads[i];

        if (g && (g->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET)
        {
            struct StringInfo *si = (struct StringInfo *)g->SpecialInfo;

            if (si && si->Extension)
                si->Extension->EditHook = &replaceOnFirstKey;
        }
    }
}

// Draw the Edit Address Book Profile window
static int OpenEditProfileWindow( void )
{
    struct Gadget    *g;
    UWORD        ww, wh;
    long x,y;

    ComputeFont( editProfileWidth, editProfileHeight );

    ww = ComputeX( editProfileWidth );
    wh = ComputeY( editProfileHeight );

    if ( ! ( g = CreateContext( &editProfileGList )))
        return( 1L );

    if(MakeGadgets(editProfileNGad, editProfileGadgets, editProfileGTags, g, editProfileGTypes, editProfile_CNT) != 0) return( 2L );
    InstallReplaceOnFirstKey(editProfileGadgets, editProfile_CNT);

    x = ww + OffX + scr->WBorRight;
    y = wh + OffY + scr->WBorBottom;

    newWin.LeftEdge = (scr->Width - x) / 2;
    newWin.TopEdge = (scr->Height - y) / 2;
    newWin.Width = x;
    newWin.Height = y;
    newWin.IDCMPFlags = STRINGIDCMP|TEXTIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY;
    newWin.Flags = WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP;
    newWin.FirstGadget = editProfileGList;
    newWin.Title = "Edit Address Book Profile";

    editProfileWnd = OpenWindow(&newWin);
    if(!editProfileWnd) return( 4L );

    /*if ( ! ( editProfileWnd = OpenWindowTags( NULL,
                WA_Left,    (scr->Width - x) / 2,
                WA_Top,        (scr->Height - y) / 2,
                WA_Width,    x,
                WA_Height,    y,
                WA_IDCMP,    STRINGIDCMP|TEXTIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY,
                WA_Flags,    WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP,
                WA_Gadgets,    editProfileGList,
                WA_Title,    "Edit Address Book Profile",
                WA_CustomScreen,    scr,
                TAG_DONE )))
    return( 4L );*/

    GT_RefreshWindow( editProfileWnd, NULL );

    ComputeFont( editProfileWidth, editProfileHeight );

    DrawBevelBox( editProfileWnd->RPort, OffX + ComputeX( 3 ),
                    OffY + ComputeY( 1 ),
                    ComputeX( 444 ),
                    ComputeY( 128 ),   /* every field down to the macro help line */
                    GT_VisualInfo, visualInfos, TAG_DONE );
    return( 0L );
}


/*
Opens the Edit Address Book Profile dialog.

Updates the book structure only if the user validates the changes.

return TRUE  if the user validated the changes (OK)
       FALSE if the user cancelled or closed the window
 */
// Next free settings id: one above the highest id in the Address Book list.
static ULONG NextSettingsId(struct List *list)
{
    struct Node *node;
    ULONG maxId = 0;

    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        ULONG id = ((struct BookStruct *)node->ln_Name)->settingsId;
        if (id > maxId) maxId = id;
    }
    return maxId + 1;
}

/*
 * Per-entry Settings window (Term 4.x style groups). One block per group: an
 * Override checkbox, a one-line summary of what the entry will use, a Use
 * Current button (copy the group from the settings in use now) and the
 * group's own fields. Changing a field overrides its group. Works on a copy
 * of the entry's settings; Ok hands it back to the edit window, which saves
 * it with the entry.
 */
enum
{
    SG_SCREEN_OVR, SG_SCREEN_SUM, SG_SCREEN_CUR, SG_SCREEN_FONT, SG_SCREEN_PALETTE,
    SG_TERM_OVR, SG_TERM_SUM, SG_TERM_CUR, SG_TERM_PETSCII, SG_TERM_XEMLIB, SG_TERM_DISPID,
    SG_TERM_RAW, SG_TERM_ECHO, SG_TERM_RENDERER, SG_TERM_RLOGIN, SG_TERM_CHARSET,
    SG_KEY_OVR, SG_KEY_SUM, SG_KEY_CUR, SG_KEY_BSDEL, SG_KEY_CRLF, SG_KEY_FKEYS, SG_KEY_VT,
    SG_XFER_OVR, SG_XFER_SUM, SG_XFER_CUR, SG_XFER_PROTO, SG_XFER_OPTS,
    SG_OK, SG_CANCEL,
    SG_COUNT
};

struct SettingsGadgetDef
{
    WORD  x, y, w, h;
    char  *label;
    ULONG placeText;
    UWORD kind;
};

/* Design grid of 8-pixel characters (topaz 8), scaled by ComputeX/Y: a
 * button is its label's length * 8 + 16, a checkbox 26 plus its label. */
#define settingsWidth  616
#define settingsHeight 218

static const struct SettingsGadgetDef settingsDefs[SG_COUNT] =
{
    {  80,   4, 112, 13, "Screen",               PLACETEXT_LEFT,  CYCLE_KIND    },
    { 196,   4, 300, 13, NULL,                   0,               TEXT_KIND     },
    { 500,   4, 112, 13, "Use Current",          PLACETEXT_IN,    BUTTON_KIND   },
    {  80,  20,  72, 13, "Font...",              PLACETEXT_IN,    BUTTON_KIND   },
    { 156,  20,  96, 13, "Palette...",           PLACETEXT_IN,    BUTTON_KIND   },

    {  80,  42, 112, 13, "Terminal",             PLACETEXT_LEFT,  CYCLE_KIND    },
    { 196,  42, 300, 13, NULL,                   0,               TEXT_KIND     },
    { 500,  42, 112, 13, "Use Current",          PLACETEXT_IN,    BUTTON_KIND   },
    {  80,  59,  26, 11, "PETSCII Mode",         PLACETEXT_RIGHT, CHECKBOX_KIND },
    { 316,  58, 136, 13, "XEM Library...",       PLACETEXT_IN,    BUTTON_KIND   },
    { 456,  58, 120, 13, "Display ID...",        PLACETEXT_IN,    BUTTON_KIND   },
    {  80,  75,  26, 11, "Raw Connection",       PLACETEXT_RIGHT, CHECKBOX_KIND },
    { 256,  75,  26, 11, "Local Echoback",       PLACETEXT_RIGHT, CHECKBOX_KIND },
    {  80,  91, 176, 13, "Renderer",             PLACETEXT_LEFT,  CYCLE_KIND    },
    { 432,  75,  26, 11, "Rlogin",               PLACETEXT_RIGHT, CHECKBOX_KIND },
    { 344,  91, 176, 13, "Characters",           PLACETEXT_LEFT,  CYCLE_KIND    },

    {  80, 113, 112, 13, "Keyboard",             PLACETEXT_LEFT,  CYCLE_KIND    },
    { 196, 113, 300, 13, NULL,                   0,               TEXT_KIND     },
    { 500, 113, 112, 13, "Use Current",          PLACETEXT_IN,    BUTTON_KIND   },
    {  80, 130,  26, 11, "BS/DEL Swap",          PLACETEXT_RIGHT, CHECKBOX_KIND },
    { 216, 130,  26, 11, "Return = CR + LF",     PLACETEXT_RIGHT, CHECKBOX_KIND },
    { 396, 129, 144, 13, "Function Keys...",     PLACETEXT_IN,    BUTTON_KIND   },
    {  80, 146,  26, 11, "VT Keys",              PLACETEXT_RIGHT, CHECKBOX_KIND },

    {  80, 167, 112, 13, "Transfer",             PLACETEXT_LEFT,  CYCLE_KIND    },
    { 196, 167, 300, 13, NULL,                   0,               TEXT_KIND     },
    { 500, 167, 112, 13, "Use Current",          PLACETEXT_IN,    BUTTON_KIND   },
    {  80, 183, 104, 13, "Protocol...",          PLACETEXT_IN,    BUTTON_KIND   },
    { 188, 183, 168, 13, "Protocol Options...",  PLACETEXT_IN,    BUTTON_KIND   },

    {   4, 202, 100, 13, "Ok",                   PLACETEXT_IN,    BUTTON_KIND   },
    { 512, 202, 100, 13, "Cancel",               PLACETEXT_IN,    BUTTON_KIND   },
};

// The Override cycle gadget of each group: which settings the entry uses.
static STRPTR overrideLabels[] = { (STRPTR)"Global", (STRPTR)"This Entry", NULL };
// The Character Set (charset.h: CHARSET_CP437, _LATIN1, _UTF8).
static STRPTR charsetLabels[] = { (STRPTR)"IBM PC", (STRPTR)"Amiga", (STRPTR)"UTF-8", NULL };
// The Renderer cycle: APP_RENDERER_BUILTIN .. APP_RENDERER_IBMCON_DEVICE, in bit order.
static STRPTR rendererLabels[] = { (STRPTR)"Built-in", (STRPTR)"console.device", (STRPTR)"XEM Library",
                                   (STRPTR)"ibmcon.device", NULL };
#define RENDERER_FIRST_BIT 9    // APP_RENDERER_BUILTIN (prefs.h)
typedef char RendererBitsInOrder[(APP_RENDERER_BUILTIN == 1UL << RENDERER_FIRST_BIT
                                  && APP_RENDERER_IBMCON_DEVICE == 1UL << (RENDERER_FIRST_BIT + 3)) ? 1 : -1];

// The Renderer cycle position for a State (its renderer bit).
static UWORD RendererIndex(ULONG state)
{
    UWORD i;
    for (i = 0; i < 4; i++)
        if (state & (1UL << (RENDERER_FIRST_BIT + i))) return i;
    return 0;
}

static const ULONG groupOfOverride[4]  = { SITE_GROUP_SCREEN, SITE_GROUP_TERMINAL, SITE_GROUP_KEYBOARD, SITE_GROUP_TRANSFER };
static const UWORD overrideGadget[4]   = { SG_SCREEN_OVR, SG_TERM_OVR, SG_KEY_OVR, SG_XFER_OVR };
static const UWORD summaryGadget[4]    = { SG_SCREEN_SUM, SG_TERM_SUM, SG_KEY_SUM, SG_XFER_SUM };

static struct Window *settingsWnd;
static struct Gadget *settingsGList;
static struct Gadget *settingsGadgets[SG_COUNT];

static BOOL IsChecked(UWORD id)
{
    return (settingsGadgets[id]->Flags & GFLG_SELECTED) != 0;
}

static void SetChecked(UWORD id, BOOL on)
{
    GT_SetGadgetAttrs(settingsGadgets[id], settingsWnd, NULL, GTCB_Checked, on, TAG_DONE);
}

// Show the values the entry would use: its own for overridden groups, the
// global ones otherwise.
static void RefreshSettingsWindow(const struct SiteSettings *work)
{
    static char summary[4][80];
    static struct PrefsStruct shown;
    UWORD g;

    shown = *GlobalSettings();
    SitePrefs_ApplyEntry(&shown, GlobalSettings(), work);
    for (g = 0; g < 4; g++)
    {
        GT_SetGadgetAttrs(settingsGadgets[overrideGadget[g]], settingsWnd, NULL,
                          GTCY_Active, (work->groups & groupOfOverride[g]) ? 1 : 0, TAG_DONE);
        SitePrefs_GroupSummary(groupOfOverride[g], &shown, summary[g], sizeof(summary[g]));
        GT_SetGadgetAttrs(settingsGadgets[summaryGadget[g]], settingsWnd, NULL,
                          GTTX_Text, summary[g], TAG_DONE);
    }
    SetChecked(SG_TERM_PETSCII, (shown.State & APP_PETSCII_MODE) != 0);
    SetChecked(SG_TERM_RAW,     (shown.State & APP_RAW_CONNECTION) != 0);
    SetChecked(SG_TERM_ECHO,    (shown.State & APP_LOCAL_ECHO) != 0);
    SetChecked(SG_TERM_RLOGIN,  (shown.State & APP_RLOGIN) != 0);
    GT_SetGadgetAttrs(settingsGadgets[SG_TERM_CHARSET], settingsWnd, NULL,
                      GTCY_Active, (ULONG)shown.Charset, TAG_DONE);
    SetChecked(SG_KEY_BSDEL,    (shown.State & APP_BACKSPACE_DEL_SWAPPED) != 0);
    SetChecked(SG_KEY_CRLF,     (shown.State & APP_RETURN_SENDING_CRLF) != 0);
    SetChecked(SG_KEY_VT,       (shown.State & APP_VT_KEYS) != 0);
    GT_SetGadgetAttrs(settingsGadgets[SG_TERM_RENDERER], settingsWnd, NULL,
                      GTCY_Active, (ULONG)RendererIndex(shown.State), TAG_DONE);
}

// A field of a group is about to change: the group becomes overridden, with
// the values it showed so far (so editing one field keeps the others).
static void OverrideGroup(struct SiteSettings *work, ULONG group)
{
    if (!(work->groups & group))
    {
        static struct PrefsStruct shown;
        static struct SiteSettings one;

        shown = *GlobalSettings();
        SitePrefs_ApplyEntry(&shown, GlobalSettings(), work);
        one.groups = group;
        one.prefs = shown;
        SitePrefs_ApplyEntry(&work->prefs, &work->prefs, &one);
        if (group == SITE_GROUP_KEYBOARD)
            memcpy(work->fKeys, GlobalFKeys(), sizeof(work->fKeys));
        work->groups |= group;
    }
}

// A checkbox into a State bit; inverted: the box shows the bit clear (Use Workbench).
static void SetFlagFromGadget(struct SiteSettings *work, ULONG group, UWORD id, ULONG flag, BOOL inverted)
{
    OverrideGroup(work, group);
    if (IsChecked(id) != inverted) work->prefs.State |= flag;
    else                           work->prefs.State &= ~flag;
}

static void UseCurrentForGroup(struct SiteSettings *work, ULONG group, BOOL *captureXem)
{
    static struct SiteSettings one;

    one.groups = group;
    one.prefs = prefs;
    SitePrefs_ApplyEntry(&work->prefs, &work->prefs, &one);
    if (group == SITE_GROUP_KEYBOARD) memcpy(work->fKeys, fKeys, sizeof(work->fKeys));
    if (group == SITE_GROUP_TERMINAL) *captureXem = TRUE;
    work->groups |= group;
}

/*
 * Settings > Connection Options...: redial after a failed connect, the
 * anti-idle NOP, the connect timeout. 0 turns a setting off.
 */
enum { CO_TRIES, CO_DELAY, CO_IDLE, CO_TIMEOUT, CO_NOTE, CO_OK, CO_CANCEL, CO_COUNT };
#define connOptionsWidth  300
#define connOptionsHeight 104

static const struct SettingsGadgetDef connOptionsDefs[CO_COUNT] =
{
    { 240,   4,  52, 13, "Redial Attempts",          PLACETEXT_LEFT, INTEGER_KIND },
    { 240,  20,  52, 13, "Seconds Between Attempts", PLACETEXT_LEFT, INTEGER_KIND },
    { 240,  36,  52, 13, "Keep Alive After Minutes", PLACETEXT_LEFT, INTEGER_KIND },
    { 240,  52,  52, 13, "Connect Timeout, Seconds", PLACETEXT_LEFT, INTEGER_KIND },
    {   8,  70, 284, 12, NULL,                       0,              TEXT_KIND    },
    {   4,  87, 100, 13, "Ok",                       PLACETEXT_IN,   BUTTON_KIND  },
    { 196,  87, 100, 13, "Cancel",                   PLACETEXT_IN,   BUTTON_KIND  },
};

static UBYTE IntegerGadgetValue(struct Gadget *g)
{
    LONG v = ((struct StringInfo *)g->SpecialInfo)->LongInt;

    return (UBYTE)(v < 0 ? 0 : v > 255 ? 255 : v);
}

/* TRUE when p was changed (Ok). */
BOOL EditConnectionOptions(struct PrefsStruct *p)
{
    struct Gadget *glist = NULL, *g, *gads[CO_COUNT];
    struct Window *w = NULL;
    struct NewGadget ng;
    UBYTE values[4];
    UWORD i, ww, wh;
    BOOL done = FALSE, ok = FALSE;

    values[0] = p->RedialTries;
    values[1] = p->RedialDelay ? p->RedialDelay : 10;
    values[2] = p->AntiIdleMinutes;
    values[3] = p->ConnectTimeout;
    ComputeFont(connOptionsWidth, connOptionsHeight);
    ww = ComputeX(connOptionsWidth);
    wh = ComputeY(connOptionsHeight);
    if (!(g = CreateContext(&glist))) return FALSE;
    for (i = 0; i < CO_COUNT; i++)
    {
        const struct SettingsGadgetDef *d = &connOptionsDefs[i];
        ULONG tags[5];

        memset(&ng, 0, sizeof(ng));
        ng.ng_LeftEdge   = OffX + ComputeX(d->x);
        ng.ng_TopEdge    = OffY + ComputeY(d->y);
        ng.ng_Width      = ComputeX(d->w);
        ng.ng_Height     = ComputeY(d->h);
        ng.ng_GadgetText = (UBYTE *)d->label;
        ng.ng_TextAttr   = &Attr;
        ng.ng_GadgetID   = i;
        ng.ng_Flags      = d->placeText;
        ng.ng_VisualInfo = visualInfos;
        tags[0] = TAG_DONE;
        if (d->kind == INTEGER_KIND)
        {
            tags[0] = GTIN_Number; tags[1] = values[i];
            tags[2] = GTIN_MaxChars; tags[3] = 3; tags[4] = TAG_DONE;
        }
        if (d->kind == TEXT_KIND)
        {
            tags[0] = GTTX_Text; tags[1] = (ULONG)"0 turns a setting off."; tags[2] = TAG_DONE;
        }
        gads[i] = g = CreateGadgetA(d->kind, g, &ng, (struct TagItem *)tags);
        if (!g) break;
    }
    if (g)
        w = OpenWindowTags(NULL,
                           WA_Left, (scr->Width - (ww + OffX + scr->WBorRight)) / 2,
                           WA_Top, (scr->Height - (wh + OffY + scr->WBorBottom)) / 2,
                           WA_Width, ww + OffX + scr->WBorRight,
                           WA_Height, wh + OffY + scr->WBorBottom,
                           WA_Title, (ULONG)"Connection Options",
                           WA_Gadgets, (ULONG)glist,
                           WA_IDCMP, INTEGERIDCMP | BUTTONIDCMP | IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW,
                           WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE,
                           WA_Activate, TRUE, WA_RMBTrap, TRUE, WA_SmartRefresh, TRUE,
                           WA_CustomScreen, (ULONG)scr,
                           TAG_DONE);
    if (w)
    {
        struct IntuiMessage *m;

        GT_RefreshWindow(w, NULL);
        while (!done)
        {
            WaitPort(w->UserPort);
            while (!done && (m = GT_GetIMsg(w->UserPort)))
            {
                ULONG class = m->Class;
                UWORD id = class == IDCMP_GADGETUP ? ((struct Gadget *)m->IAddress)->GadgetID : 0;

                GT_ReplyIMsg(m);
                if (class == IDCMP_CLOSEWINDOW) done = TRUE;
                else if (class == IDCMP_REFRESHWINDOW) { GT_BeginRefresh(w); GT_EndRefresh(w, TRUE); }
                else if (class == IDCMP_GADGETUP && id == CO_OK) ok = done = TRUE;
                else if (class == IDCMP_GADGETUP && id == CO_CANCEL) done = TRUE;
            }
        }
        if (ok)
        {
            p->RedialTries     = IntegerGadgetValue(gads[CO_TRIES]);
            p->RedialDelay     = IntegerGadgetValue(gads[CO_DELAY]);
            p->AntiIdleMinutes = IntegerGadgetValue(gads[CO_IDLE]);
            p->ConnectTimeout  = IntegerGadgetValue(gads[CO_TIMEOUT]);
        }
        CloseWindow(w);
    }
    FreeGadgets(glist);
    return ok;
}

/**
 * @brief Edit an Address Book entry's settings groups.
 * @param entry      in: the entry's settings; out (on TRUE): the edited ones
 * @param captureXem set TRUE when the Terminal group took the current settings:
 *                   the caller then saves the running XEM options with the entry
 */
static BOOL EditEntrySettings(const char *entryName, struct SiteSettings *entry, BOOL *captureXem)
{
    static struct SiteSettings work, undo;
    static char title[64];
    struct NewGadget ng;
    struct Gadget *g;
    struct IntuiMessage *message;
    struct Gadget *gad;
    ULONG class;
    UWORD code, i, ww, wh;
    BOOL done = FALSE, ok = FALSE;

    work = *entry;
    *captureXem = FALSE;
    mysprintf(title, "Settings for \"%s\"", (char *)entryName);

    ComputeFont(settingsWidth, settingsHeight);
    ww = ComputeX(settingsWidth);
    wh = ComputeY(settingsHeight);

    if (!(g = CreateContext(&settingsGList))) return FALSE;
    for (i = 0; i < SG_COUNT; i++)
    {
        const struct SettingsGadgetDef *d = &settingsDefs[i];
        ULONG tags[5];

        memset(&ng, 0, sizeof(ng));
        ng.ng_LeftEdge   = OffX + ComputeX(d->x);
        ng.ng_TopEdge    = OffY + ComputeY(d->y);
        ng.ng_Width      = ComputeX(d->w);
        ng.ng_Height     = ComputeY(d->h);
        ng.ng_GadgetText = (UBYTE *)d->label;
        ng.ng_TextAttr   = &Attr;
        ng.ng_GadgetID   = i;
        ng.ng_Flags      = d->placeText;
        ng.ng_VisualInfo = visualInfos;

        tags[0] = TAG_DONE;
        if (d->kind == TEXT_KIND)  { tags[0] = GTTX_Border; tags[1] = TRUE; tags[2] = TAG_DONE; }
        if (d->kind == CYCLE_KIND) { tags[0] = GTCY_Labels;
                                     tags[1] = (ULONG)(i == SG_TERM_RENDERER ? rendererLabels
                                                     : i == SG_TERM_CHARSET ? charsetLabels : overrideLabels);
                                     tags[2] = TAG_DONE; }
        settingsGadgets[i] = g = CreateGadgetA(d->kind, g, &ng, (struct TagItem *)tags);
        if (!g) break;
    }
    if (g)
    {
        newWin.LeftEdge    = (scr->Width - (ww + OffX + scr->WBorRight)) / 2;
        newWin.TopEdge     = (scr->Height - (wh + OffY + scr->WBorBottom)) / 2;
        newWin.Width       = ww + OffX + scr->WBorRight;
        newWin.Height      = wh + OffY + scr->WBorBottom;
        newWin.IDCMPFlags  = BUTTONIDCMP|CHECKBOXIDCMP|CYCLEIDCMP|TEXTIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW;
        newWin.Flags       = WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP;
        newWin.FirstGadget = settingsGList;
        newWin.Title       = (UBYTE *)title;
        settingsWnd = OpenWindow(&newWin);
    }

    if (settingsWnd)
    {
        GT_RefreshWindow(settingsWnd, NULL);
        RefreshSettingsWindow(&work);
        SetWaitPointer(editProfileWnd);

        while (!done)
        {
            WaitPort(settingsWnd->UserPort);
            while (!done && (message = GT_GetIMsg(settingsWnd->UserPort)))
            {
                gad   = (struct Gadget *)message->IAddress;
                class = message->Class;
                code  = message->Code;
                GT_ReplyIMsg(message);

                if (class == IDCMP_CLOSEWINDOW) { done = TRUE; break; }
                if (class == IDCMP_REFRESHWINDOW)
                {
                    GT_BeginRefresh(settingsWnd);
                    GT_EndRefresh(settingsWnd, TRUE);
                    continue;
                }
                if (class != IDCMP_GADGETUP) continue;

                switch (gad->GadgetID)
                {
                case SG_SCREEN_OVR: case SG_TERM_OVR: case SG_KEY_OVR: case SG_XFER_OVR:
                {
                    ULONG group = groupOfOverride[gad->GadgetID == SG_SCREEN_OVR ? 0 :
                                                  gad->GadgetID == SG_TERM_OVR ? 1 :
                                                  gad->GadgetID == SG_KEY_OVR ? 2 : 3];
                    if (code == 1) OverrideGroup(&work, group);     // "This Entry"
                    else           work.groups &= ~group;           // "Global"
                    break;
                }
                case SG_SCREEN_CUR: UseCurrentForGroup(&work, SITE_GROUP_SCREEN, captureXem);   break;
                case SG_TERM_CUR:   UseCurrentForGroup(&work, SITE_GROUP_TERMINAL, captureXem); break;
                case SG_KEY_CUR:    UseCurrentForGroup(&work, SITE_GROUP_KEYBOARD, captureXem); break;
                case SG_XFER_CUR:   UseCurrentForGroup(&work, SITE_GROUP_TRANSFER, captureXem); break;

                // A requester button seeds the group (so the requester starts from
                // the values shown) and keeps the change only when not cancelled.
                case SG_SCREEN_FONT:
                    undo = work;
                    OverrideGroup(&work, SITE_GROUP_SCREEN);
                    if (!FontRequester(settingsWnd, (STRPTR)work.prefs.FontName, sizeof(work.prefs.FontName),
                                       &work.prefs.FontSize)) work = undo;
                    break;
                case SG_SCREEN_PALETTE:
                    undo = work;
                    OverrideGroup(&work, SITE_GROUP_SCREEN);
                    if (!EditPalette(&work.prefs)) work = undo;
                    break;

                case SG_TERM_PETSCII:
                    SetFlagFromGadget(&work, SITE_GROUP_TERMINAL, SG_TERM_PETSCII, APP_PETSCII_MODE, FALSE);
                    break;
                case SG_TERM_RENDERER:
                    OverrideGroup(&work, SITE_GROUP_TERMINAL);
                    work.prefs.State = (work.prefs.State & ~APP_RENDERER_ALL)
                                     | (1UL << (RENDERER_FIRST_BIT + (code & 3)));
                    break;
                case SG_TERM_RAW:
                    SetFlagFromGadget(&work, SITE_GROUP_TERMINAL, SG_TERM_RAW, APP_RAW_CONNECTION, FALSE);
                    break;
                case SG_TERM_RLOGIN:
                    SetFlagFromGadget(&work, SITE_GROUP_TERMINAL, SG_TERM_RLOGIN, APP_RLOGIN, FALSE);
                    break;
                case SG_TERM_CHARSET:
                    OverrideGroup(&work, SITE_GROUP_TERMINAL);
                    work.prefs.Charset = (UBYTE)code;
                    break;
                case SG_TERM_ECHO:
                    SetFlagFromGadget(&work, SITE_GROUP_TERMINAL, SG_TERM_ECHO, APP_LOCAL_ECHO, FALSE);
                    break;
                case SG_TERM_XEMLIB:
                    undo = work;
                    OverrideGroup(&work, SITE_GROUP_TERMINAL);
                    if (!FileRequester(settingsWnd, "LIBS:", 0, (STRPTR)work.prefs.XemLibrary,
                                       sizeof(work.prefs.XemLibrary), "xem#?.library", FILEREQ_LOAD))
                        work = undo;
                    break;
                case SG_TERM_DISPID:
                    undo = work;
                    OverrideGroup(&work, SITE_GROUP_TERMINAL);
                    if (!GetStringRequester(settingsWnd, "Telnet Display ID...", "Term type:",
                                            (STRPTR)work.prefs.TelnetTermType, sizeof(work.prefs.TelnetTermType)))
                        work = undo;
                    break;

                case SG_KEY_BSDEL:
                    SetFlagFromGadget(&work, SITE_GROUP_KEYBOARD, SG_KEY_BSDEL, APP_BACKSPACE_DEL_SWAPPED, FALSE);
                    break;
                case SG_KEY_CRLF:
                    SetFlagFromGadget(&work, SITE_GROUP_KEYBOARD, SG_KEY_CRLF, APP_RETURN_SENDING_CRLF, FALSE);
                    break;
                case SG_KEY_VT:
                    SetFlagFromGadget(&work, SITE_GROUP_KEYBOARD, SG_KEY_VT, APP_VT_KEYS, FALSE);
                    break;
                case SG_KEY_FKEYS:
                    undo = work;
                    OverrideGroup(&work, SITE_GROUP_KEYBOARD);
                    if (!EditFunctionKeys(work.fKeys, title)) work = undo;
                    break;

                case SG_XFER_PROTO:
                    undo = work;
                    OverrideGroup(&work, SITE_GROUP_TRANSFER);
                    if (!FileRequester(settingsWnd, "LIBS:", 0, (STRPTR)work.prefs.XferLibrary,
                                       sizeof(work.prefs.XferLibrary), "xpr#?.library", FILEREQ_LOAD))
                        work = undo;
                    break;
                case SG_XFER_OPTS:
                    undo = work;
                    OverrideGroup(&work, SITE_GROUP_TRANSFER);
                    if (!GetStringRequester(settingsWnd, "XPR Protocol Options..", "Options string:",
                                            (STRPTR)work.prefs.XferOptions, sizeof(work.prefs.XferOptions)))
                        work = undo;
                    break;

                case SG_OK:     done = TRUE; ok = TRUE; break;
                case SG_CANCEL: done = TRUE;            break;
                }
                if (!done) RefreshSettingsWindow(&work);
            }
        }
        ClearPointer(editProfileWnd);
        CloseWindow(settingsWnd);
        settingsWnd = NULL;
    }
    FreeGadgets(settingsGList);
    settingsGList = NULL;

    if (ok) *entry = work;
    return ok;
}

static BOOL EditProfile(struct BookStruct *book, struct List *list)
{
    char strLastTime[2 * LEN_DATSTRING];
    // The settings buttons only stage a change; Ok applies it, Cancel drops it.
    enum { SETTINGS_KEEP, SETTINGS_EDITED, SETTINGS_USE_GLOBAL } settingsAction = SETTINGS_KEEP;
    BOOL captureXem = FALSE;
    static struct SiteSettings snapshot;
    struct IntuiMessage *message;
    struct Gadget *gad;
    ULONG class;
    UWORD code;
    char subdone = FALSE;
    BOOL ret = FALSE;

    // Initialize gadget fields with current book data
    editProfileGTags[EP_TAG_SITE] = (unsigned long)book->name;
    editProfileGTags[EP_TAG_ADDRESS] = (unsigned long)book->host;
    myctime(book->lastConnect, strLastTime, sizeof(strLastTime));
    editProfileGTags[EP_TAG_LAST] = (unsigned long)strLastTime;
    editProfileGTags[EP_TAG_PORT] = (unsigned long)book->port;
    editProfileGTags[EP_TAG_USERNAME] = (unsigned long)book->username;
    editProfileGTags[EP_TAG_PASSWORD] = (unsigned long)book->password;
    // "Own" only when the entry's settings file really loads: a missing,
    // truncated or foreign file means the entry connects with global settings.
    memset(&snapshot, 0, sizeof(snapshot));   // no stale macro from another entry
    editProfileGTags[EP_TAG_SETTINGS] = (unsigned long)(LoadEntrySettings(book->settingsId, &snapshot)
                                         && snapshot.groups ? "This Entry (some groups)" : "Global");
    editProfileGTags[EP_TAG_COMMENT] = (unsigned long)book->comment;
    editProfileGTags[EP_TAG_LOGIN_MACRO] = (unsigned long)snapshot.loginMacro;
    // Sent after connecting (SendLoginMacro); the codes it understands:
    editProfileGTags[EP_TAG_MACRO_HELP] =
        (unsigned long)"\\u Username \\p Password \\r Return \\d Wait 1 s";

    // Open the Edit Profile window
    if(OpenEditProfileWindow() == RETURN_OK)
    {
        SetWaitPointer(aBookWnd);  // Set "wait" mouse pointer to indicate modal operation

        // Activate the first gadget (Site Name) to receive keyboard input
        ActivateGadget(editProfileGadgets[GD_SITE], editProfileWnd, 0);

        while(!subdone)
        {
            register struct Gadget *vgad = NULL;

            WaitPort(editProfileWnd->UserPort); // Wait for input events

            while (message = GT_GetIMsg(editProfileWnd->UserPort))
            {
                gad   = (struct Gadget *)message->IAddress; // Gadget associated with the message
                class = message->Class;
                code  = message->Code;                      // Key code for keyboard events
                GT_ReplyIMsg(message);                      // Acknowledge message

                switch (class)
                {
                case IDCMP_CLOSEWINDOW:      // User clicked the close gadget
                    subdone = TRUE;
                    ret = FALSE;
                    break;

                case IDCMP_VANILLAKEY:       // Keyboard input (letters, Enter, etc.)
                    switch(toupper(code))
                    {
                        case 'O':   // OK
                            subdone = TRUE;
                            ret = TRUE;
                            break;
                        case 'C':   // Cancel
                            subdone = TRUE;
                            ret = FALSE;
                            break;
                        // Keyboard shortcuts to jump to a specific gadget:
                        case 'S':  vgad = editProfileGadgets[GD_SITE];        break;
                        case 'A':  vgad = editProfileGadgets[GD_ADDRESS];     break;
                        case 'P':  vgad = editProfileGadgets[GD_PORT];        break;
                        case 'U':  vgad = editProfileGadgets[GD_USERNAME];    break;
                        case 'W':  vgad = editProfileGadgets[GD_PASSWORD];    break;
                        case 'T':  goto editSettings;
                        case 'G':  goto useGlobal;
                        case 'M':  vgad = editProfileGadgets[GD_COMMENT];     break;
                        case 'L':  vgad = editProfileGadgets[GD_LOGIN_MACRO]; break;
                    }
                    if(vgad) ActivateGadget(vgad, editProfileWnd, 0); // Focus gadget
                    break;

                case IDCMP_GADGETUP:         // Mouse released over a gadget
                    switch(gad->GadgetID)
                    {
                    case GD_OK:
                        subdone = TRUE;
                        ret = TRUE;
                        break;
                    case GD_CANCEL:
                        subdone = TRUE;
                        ret = FALSE;
                        break;
                    case GD_USE_CURRENT:        // the "Settings..." button
editSettings:
                        if (EditEntrySettings(book->name, &snapshot, &captureXem))
                        {
                            settingsAction = SETTINGS_EDITED;
                            GT_SetGadgetAttrs(editProfileGadgets[GD_SETTINGS], editProfileWnd, NULL,
                                              GTTX_Text, snapshot.groups ? "This Entry (saved on Ok)" : "Global",
                                              TAG_DONE);
                        }
                        break;
                    case GD_USE_GLOBAL:
useGlobal:
                        settingsAction = SETTINGS_USE_GLOBAL;
                        GT_SetGadgetAttrs(editProfileGadgets[GD_SETTINGS], editProfileWnd, NULL,
                                          GTTX_Text, "Global (on Ok)", TAG_DONE);
                        break;
                    }
                    break;
                }
            }
        }

        // Copy gadget values to the BookStruct if user pressed OK:
        if (ret)
        {
            strlcpy(book->name,
                    ((struct StringInfo *)editProfileGadgets[GD_SITE]->SpecialInfo)->Buffer,
                    sizeof(book->name));
            strlcpy(book->host,
                    ((struct StringInfo *)editProfileGadgets[GD_ADDRESS]->SpecialInfo)->Buffer,
                    sizeof(book->host));
            book->port = ((struct StringInfo *)editProfileGadgets[GD_PORT]->SpecialInfo)->LongInt;
            strlcpy(book->username,
                    ((struct StringInfo *)editProfileGadgets[GD_USERNAME]->SpecialInfo)->Buffer,
                    sizeof(book->username));
            strlcpy(book->password,
                    ((struct StringInfo *)editProfileGadgets[GD_PASSWORD]->SpecialInfo)->Buffer,
                    sizeof(book->password));

            strlcpy(book->comment,
                    ((struct StringInfo *)editProfileGadgets[GD_COMMENT]->SpecialInfo)->Buffer,
                    sizeof(book->comment));

            // The settings file holds the overridden groups AND the login macro:
            // it exists while either is there.
            if (settingsAction == SETTINGS_USE_GLOBAL)
                snapshot.groups = 0;
            strlcpy(snapshot.loginMacro,
                    ((struct StringInfo *)editProfileGadgets[GD_LOGIN_MACRO]->SpecialInfo)->Buffer,
                    sizeof(snapshot.loginMacro));

            if (!snapshot.groups && !snapshot.loginMacro[0])
            {
                DeleteEntrySettings(book->settingsId);
                book->settingsId = 0;
            }
            else
            {
                ULONG id = book->settingsId ? book->settingsId : NextSettingsId(list);

                if (SaveEntrySettings(id, &snapshot))
                {
                    book->settingsId = id;
                    if (settingsAction == SETTINGS_EDITED && captureXem && (snapshot.groups & SITE_GROUP_TERMINAL))
                    {
                        char xemPath[40];

                        EntryXemOptionsPath(id, xemPath);
                        SaveXemOptions(xemPath);    // no-op when XEM is not in use
                    }
                }
                else
                    InfoReq(editProfileWnd, "Could not save the settings to " SITES_DIR ".");
            }
        }

        ClearPointer(aBookWnd); // Restore normal pointer
    }

    // Close window and free gadgets
    if ( editProfileWnd ) CloseWindow( editProfileWnd );
    if ( editProfileGList ) FreeGadgets( editProfileGList );

    return ret;
}


#include <intuition/imageclass.h>
#include <intuition/icclass.h>


enum    {    GAD_SCROLLER,
        GAD_UP,
        GAD_DOWN
    };

static APTR UpImage, DownImage;
static APTR UpArrow, DownArrow;
APTR Scroller;

void CloseScrollBack(void)
{
    if(scrollbackWin)
    {
        ClearMenuStrip(scrollbackWin);
        CloseWindow(scrollbackWin);    scrollbackWin = NULL;
        DisposeObject(Scroller);       Scroller=NULL;
        DisposeObject(UpArrow);        UpArrow = NULL;
        DisposeObject(DownArrow);      DownArrow = NULL;
        DisposeObject(UpImage);        UpImage = NULL;
        DisposeObject(DownImage);      DownImage = NULL;
    }
}

// F2 in the scroll back: the next line below the top one holding the text
// asked for (ignoring case), wrapping round to the start, scrolled to the
// top. Asked each time, with the last text: Return searches on. TRUE when
// *top moved.
BOOL FindInScrollBack(ULONG *top)
{
    static char findText[64];
    struct Node *node;
    ULONG line, tried;

    if (!GetStringRequester(scrollbackWin, "Find in Scroll Back", "Text:", findText, sizeof(findText))
        || !findText[0] || IsListEmpty(scrollbackList))
        return FALSE;
    line = *top;
    node = FindNode(scrollbackList, (UWORD)line);
    for (tried = 0; node && tried < nScrollbackLines; tried++)
    {
        node = node->ln_Succ;                       // the next line ...
        line++;
        if (!node || !node->ln_Succ)                // ... or the first again
        {
            node = scrollbackList->lh_Head;
            line = 0;
        }
        if (Ansi_FindText(node->ln_Name, strlen(node->ln_Name), findText) >= 0)
        {
            *top = line;
            RefreshListView((UWORD)line);
            return TRUE;
        }
    }
    InfoReq(scrollbackWin, "\"%s\" is not in the scroll back.", findText);
    return FALSE;
}

void RefreshListView(UWORD top)
{
    register struct RastPort *rp = scrollbackWin->RPort;
    struct Node *node = FindNode(scrollbackList, top);
    UWORD WWinTop = rp->Font->tf_YSize + scr->WBorTop + 2;
    UWORD y, i = 0;
    char print = TRUE;

    Move(rp, 5, WWinTop);
    WWinTop += rp->Font->tf_YSize;
    SetAPen(rp, drawInfo->dri_Pens[TEXTPEN]);
    while(1)
    {
        UWORD chars = (scrollbackWin->Width - 28) / rp->Font->tf_XSize;
        UWORD len;

        if(print)
        {
            if(!node->ln_Succ)
                print = FALSE;
            else {
                len = strlen(node->ln_Name);
                if(chars > len) chars = len;
            }
        }

        y = WWinTop + (i * rp->Font->tf_YSize);

        if(y > (scrollbackWin->Height-5)) break;

        Move(rp, 5, y);
        if(print) Text(rp, node->ln_Name, chars);
        EraseRect(rp, rp->cp_x, (rp->cp_y-rp->Font->tf_YSize)+2, scrollbackWin->Width - 24, rp->cp_y+1);

        if(print) node = node->ln_Succ;
        i++;
    }
}

void OpenScrollBack(UWORD sel)
{
    STATIC struct TagItem ArrowMappings[] =
    {
        GA_ID,    GA_ID,
        TAG_END
    };

    ULONG ArrowHeight;
    LONG SizeType;
    Object *SizeImage;

    if(prefs.ScrollbackWinWidth > scr->Width) prefs.ScrollbackWinWidth = scr->Width;
    if(prefs.ScrollbackWinHeight > scr->Height) prefs.ScrollbackWinHeight = scr->Height;

    if(scr->Flags & SCREENHIRES)
        SizeType = SYSISIZE_MEDRES;
    else
        SizeType = SYSISIZE_LOWRES;

    /*
     NewObject() allows an arbitrary number of tags. It is a varargs stub for NewObjectA().
     You specify a class either as a pointer (for a private class) or by its ID string (for public
     classes).  If the class pointer is NULL, then the classID is used.
    */
    if(SizeImage = NewObject(NULL,SYSICLASS,  // class
        SYSIA_Size,    SizeType,                 // 1st  tag (= key/value pair = property)
        SYSIA_Which,    SIZEIMAGE,            // 2nd  tag
        SYSIA_DrawInfo,    drawInfo,             // ...
    TAG_DONE))                                // terminator tag
    {
        ULONG SizeWidth, SizeHeight;

        GetAttr(IA_Width, SizeImage, &SizeWidth);
        GetAttr(IA_Height, SizeImage, &SizeHeight);

        DisposeObject(SizeImage);

        if(UpImage = NewObject(NULL, SYSICLASS,
            SYSIA_Size,    SizeType,
            SYSIA_Which,    UPIMAGE,
            SYSIA_DrawInfo,    drawInfo,
        TAG_DONE))
        {
            GetAttr(IA_Height, UpImage, &ArrowHeight);

            if(DownImage = NewObject(NULL, SYSICLASS,
                SYSIA_Size,    SizeType,
                SYSIA_Which,    DOWNIMAGE,
                SYSIA_DrawInfo,    drawInfo,
            TAG_DONE))
            {
                if(Scroller = NewObject(NULL, PROPGCLASS,
                    GA_ID,        GAD_SCROLLER,
                    GA_Top,        scr->WBorTop + scr->Font->ta_YSize + 2,
                    GA_RelHeight,    -(scr->WBorTop + scr->Font->ta_YSize + 2 + SizeHeight + 1 + 2 * ArrowHeight),
                    GA_Width,    SizeWidth - 8,
                    GA_RelRight,    -(SizeWidth - 5),
                    GA_Immediate,    TRUE,
                    GA_FollowMouse,    TRUE,
                    GA_RelVerify,    TRUE,
                    GA_RightBorder,    TRUE,
                    PGA_Freedom,    FREEVERT,
                    PGA_NewLook,    TRUE,
                    PGA_Borderless,    TRUE,
                    PGA_Top,    sel,
                    PGA_Visible,    (prefs.ScrollbackWinHeight - (scr->Font->ta_YSize + scr->WBorTop + 2)) / scr->Font->ta_YSize,
                    PGA_Total,    nScrollbackLines,
                TAG_DONE))
                {
                    if(UpArrow = NewObject(NULL, BUTTONGCLASS,
                        GA_ID,        GAD_UP,
                        GA_Image,    UpImage,
                        GA_RelRight,    -(SizeWidth - 1),
                        GA_RelBottom,    -(SizeHeight - 1 + 2 * ArrowHeight),
                        GA_Height,    ArrowHeight,
                        GA_Width,    SizeWidth,
                        GA_Previous,    Scroller,
                        GA_RightBorder,    TRUE,
                        ICA_TARGET,    ICTARGET_IDCMP,
                        ICA_MAP,    ArrowMappings,
                    TAG_DONE))
                    {
                        if(DownArrow = NewObject(NULL, BUTTONGCLASS,
                            GA_ID,        GAD_DOWN,
                            GA_Image,    DownImage,
                            GA_RelRight,    -(SizeWidth - 1),
                            GA_RelBottom,    -(SizeHeight - 1 + ArrowHeight),
                            GA_Height,    ArrowHeight,
                            GA_Width,    SizeWidth,
                            GA_Previous,    UpArrow,
                            GA_RightBorder,    TRUE,
                            ICA_TARGET,    ICTARGET_IDCMP,
                            ICA_MAP,    ArrowMappings,
                        TAG_DONE))
                        {
                            newWin.LeftEdge   = prefs.ScrollbackWinLeftEdge;
                            newWin.TopEdge    = prefs.ScrollbackWinTopEdge;
                            newWin.Width      = prefs.ScrollbackWinWidth;
                            newWin.Height     = prefs.ScrollbackWinHeight;
                            newWin.IDCMPFlags = IDCMP_IDCMPUPDATE | LISTVIEWIDCMP | IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_CLOSEWINDOW | BUTTONIDCMP | IDCMP_RAWKEY;
                            newWin.Flags = WFLG_NOCAREREFRESH | WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET;
                            newWin.FirstGadget = Scroller;
                            newWin.Title = "Scroll Back:  F1 - Clear  F2 - Find  F3 - Print  F5 - Save";
                            newWin.MinWidth   = WIN_MIN_WIDTH;
                            newWin.MinHeight  = WIN_MIN_HEIGHT;
                            newWin.MaxWidth   = DISP_MAX_WIDTH;
                            newWin.MaxHeight  = DISP_MAX_HEIGHT;
                            CheckDimensions(&newWin);
                            scrollbackWin = OpenWindow(&newWin);
                            /*scrollbackWin = OpenWindowTags(NULL,
                                WA_Title,        "Scroll Back:  F1 - Clear  F2 - Find  F3 - Print  F5 - Save",
                                WA_Left,        prefs.ScrollbackWinLeftEdge,
                                WA_Top,            prefs.ScrollbackWinTopEdge,
                                WA_Width,        prefs.ScrollbackWinWidth,
                                WA_Height,        prefs.ScrollbackWinHeight,
                                WA_MinHeight,        50,
                                WA_MinWidth,        200,
                                WA_MaxHeight,        1200,
                                WA_MaxWidth,        1600,
                                WA_CustomScreen,    scr,
                                WA_Gadgets,        Scroller,
                                WA_IDCMP,        IDCMP_IDCMPUPDATE | LISTVIEWIDCMP | IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_CLOSEWINDOW | BUTTONIDCMP | IDCMP_RAWKEY,
                                WA_Flags,        WFLG_NOCAREREFRESH | WFLG_ACTIVATE|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET,
                                TAG_END);*/
                            if(scrollbackWin)
                            {
                                //GT_RefreshWindow(scrollbackWin, NULL);
                                RefreshListView(sel);
                                ResetMenuStrip(scrollbackWin, mainMenuStrip);
                            }
                        }
                    }
                }
            }
        }
    }
}


#include "fkey.h"

static struct Window         *fKeysWnd;           // "Function Keys" settings window
static struct Gadget         *fKeysGList;         // "Function Keys" settings window GList
static struct Gadget         *fKeysGadgets[13];   // "Function Keys" settings window gadgets
#define fKeysWidth 503
#define fKeysHeight 199


static UBYTE *MOD0Labels[] = {
    (UBYTE *)"None",
    (UBYTE *)"Shift",
    NULL };

static UBYTE fKeysGTypes[] = {
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    STRING_KIND,
    CYCLE_KIND,
    BUTTON_KIND,
    BUTTON_KIND
};


static struct MyNewGadget fKeysNGad[] = {
    43, 21, 443, 15, (UBYTE *)"F1:",
    43, 37, 443, 15, (UBYTE *)"F2:",
    43, 53, 443, 15, (UBYTE *)"F3:",
    43, 69, 443, 15, (UBYTE *)"F4:",
    43, 85, 443, 15, (UBYTE *)"F5:",
    43, 101, 443, 15, (UBYTE *)"F6:",
    43, 117, 443, 15, (UBYTE *)"F7:",
    43, 133, 443, 15, (UBYTE *)"F8:",
    43, 149, 443, 15, (UBYTE *)"F9:",
    43, 165, 443, 15, (UBYTE *)"F0:",
    171, 4, 187, 14, (UBYTE *)"Modifier:",
    7, 183, 101, 14, (UBYTE *)"_Save",
    394, 183, 101, 14, (UBYTE *)"_Cancel",
};

static ULONG fKeysGTags[] = {
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    GTST_String, (ULONG) NULL, (GTST_MaxChars), F_KEY_SIZE-1, (TAG_DONE),
    (GTCY_Labels), (ULONG)&MOD0Labels[ 0 ], (GA_Disabled), TRUE, (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE),
    (GT_Underscore), '_', (TAG_DONE)
};

static int OpenFKeysWindow( const char *title )
{
    struct Gadget    *g;
    UWORD        ww, wh;
    long x, y;

    ComputeFont( fKeysWidth, fKeysHeight );

    ww = ComputeX( fKeysWidth );
    wh = ComputeY( fKeysHeight );

    if ( ! ( g = CreateContext( &fKeysGList )))
        return( 1L );

    if(MakeGadgets(fKeysNGad, fKeysGadgets, fKeysGTags, g, fKeysGTypes, fKeys_CNT) != 0) return( 2L );

    x = ww + OffX + scr->WBorRight;
    y = wh + OffY + scr->WBorBottom;

    /*newWin.LeftEdge = (scr->Width - x) / 2;
    newWin.TopEdge = (scr->Height - y) / 2;
    newWin.Width = x;
    newWin.Height = y;
    newWin.IDCMPFlags = CYCLEIDCMP|STRINGIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY;
    newWin.Flags = WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP;
    newWin.FirstGadget = fKeysGList;
    newWin.Title = "Function Keys";

    CheckDimensions(&newWin);

    fKeysWnd = OpenWindow(&newWin);
    if(!fKeysWnd) return( 4L );*/

    if ( ! ( fKeysWnd = OpenWindowTags( NULL,
                WA_Left,    (scr->Width - x) / 2,
                WA_Top,        (scr->Height - y) / 2,
                WA_Width,    x,
                WA_Height,    y,
                WA_IDCMP,    CYCLEIDCMP|STRINGIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY,
                WA_Flags,    WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_RMBTRAP,
                WA_Gadgets,    fKeysGList,
                WA_Title,    title,
                WA_CustomScreen,scr,
                TAG_DONE )))
    return( 4L );

    GT_RefreshWindow( fKeysWnd, NULL );

    return( 0L );
}


/**
 * @brief Display the Function Keys configuration dialog.
 *
 * Opens a modal GadTools-based dialog that allows the user to edit the strings associated with the
 * function keys (F1-F10). Each string defines the text that will be sent to the remote Telnet
 * server when the corresponding function key is pressed.
 *
 * If the user confirms the changes, the updated strings are copied to the global function key table
 * and written to the function key preferences file. If the dialog is cancelled, all modifications
 * are discarded.
 *
 * @note The dialog is implemented entirely with GadTools gadgets and processes Intuition messages
 *       until the user closes it.
 *
 * @note The function updates the in-memory function key table before writing it to disk.
 */
/**
 * @brief Edit a set of F1-F10 macros (the global ones, the live session's, or
 *        an Address Book entry's) in the Function Keys window.
 *
 * @return TRUE when the user saved: `keys` then holds the edited macros.
 */
BOOL EditFunctionKeys(TEXT *keys, const char *title)
{
    struct IntuiMessage *message;
    struct Gadget *gad;
    ULONG class;
    UWORD code;
    BOOL subdone = FALSE;
    BOOL save = FALSE;
    int i;

    // Initialize gadget fields with current settings:
    for (i = 0; i < F_KEY_COUNT; i++)
        fKeysGTags[1 + i * 5] = (ULONG)&keys[i * F_KEY_SIZE];

    // Open the Functions Keys window
    if(OpenFKeysWindow(title) == RETURN_OK)
    {
        while(!subdone)
        {
            WaitPort(fKeysWnd->UserPort);
            while (message = GT_GetIMsg(fKeysWnd->UserPort))
            {
                gad   = (struct Gadget *)message->IAddress;
                class = message->Class;
                code  = message->Code;
                GT_ReplyIMsg(message);

                switch (class)
                {
                case IDCMP_CLOSEWINDOW:
                    subdone = TRUE;
                    break;

                case IDCMP_VANILLAKEY:
                    switch(toupper(code))
                    {
                        case 'S':            // Save
                            save = TRUE;
                            /* fall through */
                        case 'C':
                            subdone = TRUE;  // Cancel
                    }
                    break;

                case IDCMP_GADGETUP:
                    switch(gad->GadgetID)
                    {
                    case GD_SAVEE:
                        save = TRUE;
                        /* fall through */
                    case GD_CANCELL:
                        subdone = TRUE;
                        break;
                    }
                }
            }
        }
    }

    // Copy the gadget values into keys if the user pressed Save:
    if(save)
    {
        for (i=0 ; i < F_KEY_COUNT ; i++)
        {
            // The gadget buffer is expected to be limited by "(GTST_MaxChars), F_KEY_SIZE-1",
            // but this provides additional protection against unexpected gadget behavior.
            strlcpy(&keys[i * F_KEY_SIZE],
                    ((struct StringInfo *)fKeysGadgets[i]->SpecialInfo)->Buffer,
                    F_KEY_SIZE);
        }
    }

    if (fKeysWnd)   { CloseWindow(fKeysWnd);   fKeysWnd   = NULL; }
    if (fKeysGList) { FreeGadgets(fKeysGList); fKeysGList = NULL; }
    return save;
}

/**
 * @brief Settings > Function Keys. Edits the keys in use; they are written to
 *        DCTelnet.Keys only when they are the global ones. While connected to an
 *        entry that overrides the Keyboard group the edit is session-only, like
 *        every other settings change in an entry session.
 */
void FunctionKeys(void)
{
    if (SessionOverridesKeyboard())
    {
        EditFunctionKeys(fKeys, "Function Keys (this connection only)");
        return;
    }

    if (EditFunctionKeys(fKeys, "Function Keys"))
    {
        register BPTR fh;
        LONG l;

        fh = Open(keysFilename, MODE_NEWFILE);
        l = 0;
        if(fh)
        {
            l = Write(fh, fKeys, F_KEY_COUNT * F_KEY_SIZE);
            Close(fh);
        }

        if (!fh || l != (F_KEY_COUNT * F_KEY_SIZE))
        {
            SimpleReq("ERROR: Failed to save Function keys settings!");
        }
    }
}


static char *icons[BUTTON_COUNT] =
{
    "Connect",
    "Disconnect",
    "AddressBook",
    "Information",
    "Upload",
    "Download",
    "Quit"
};

static struct DiskObject *dob[BUTTON_COUNT];


void CheckDimensions(struct NewWindow *newwin)
{
    if(newwin->Width > scr->Width) newwin->Width = scr->Width;
    if(newwin->Height > scr->Height) newwin->Height = scr->Height;

    if(newwin->LeftEdge + newwin->Width > scr->Width) newwin->LeftEdge = 0;
    if(newwin->TopEdge + newwin->Height > scr->Height) newwin->TopEdge = 0;
}

// Tool bar icons that carry their colours (DCTELNET_PALETTE, iconpens.h) are
// redrawn with the screen's closest pens: images drawn for fixed pens showed
// whatever colours a screen had there (MagicWB icons on a standard Workbench,
// on DCTelnet's own 256-colour screen). An icon without the tool type, such as
// one a user drew, is shown as it is.
static struct Image *toolImage[BUTTON_COUNT][2];
static UBYTE toolPen[BUTTON_COUNT][ICONPENS_MAX];
static BOOL  toolPenOwned[BUTTON_COUNT][ICONPENS_MAX];

// The standard Workbench colours: the match without ObtainBestPen (OS 2).
static const ULONG standardPens[4] = { 0xAAAAAA, 0x000000, 0xFFFFFF, 0x6688BB };

static struct Image *RemapToolImage(const struct Image *img, const UBYTE *map, int n)
{
    struct Image *out;
    UBYTE maxPen = 0;
    UWORD depth;
    ULONG plane;
    int i;

    for (i = 0; i < n; i++)
        if (map[i] > maxPen) maxPen = map[i];
    depth = IconPens_Depth(maxPen);
    plane = IconPens_PlaneSize(img->Width, img->Height);
    if (!(out = AllocVec(sizeof(struct Image), MEMF_CLEAR)))
        return NULL;
    if (!(out->ImageData = AllocVec(plane * depth, MEMF_CHIP)))
    {
        FreeVec(out);
        return NULL;
    }
    out->LeftEdge   = img->LeftEdge;
    out->TopEdge    = img->TopEdge;
    out->Width      = img->Width;
    out->Height     = img->Height;
    out->Depth      = depth;
    out->PlanePick  = (UBYTE)((1 << depth) - 1);
    IconPens_Remap(img->ImageData, img->Width, img->Height, img->Depth, img->PlanePick,
                   img->PlaneOnOff, map, n, out->ImageData, depth);
    return out;
}

static void RemapToolIcon(UWORD i)
{
    struct Gadget *gad = &dob[i]->do_Gadget;
    struct Image **render[2];
    ULONG rgb[ICONPENS_MAX];
    char *value;
    int n, k;

    value = (char *)FindToolType((CONST_STRPTR *)dob[i]->do_ToolTypes, ICONPENS_TOOLTYPE);
    if (!value || !(n = IconPens_Parse(value, rgb)))
        return;
    for (k = 0; k < n; k++)
    {
        toolPenOwned[i][k] = FALSE;
        if (k == 0)                                 // the icon's background
            toolPen[i][k] = (UBYTE)drawInfo->dri_Pens[BACKGROUNDPEN];
        else if (GfxBase->LibNode.lib_Version >= 39)
            toolPen[i][k] = ObtainNearestPen(scr->ViewPort.ColorMap, rgb[k], PRECISION_IMAGE,
                                             &toolPenOwned[i][k]);
        else
            toolPen[i][k] = (UBYTE)IconPens_Nearest(rgb[k], standardPens, 4);
    }
    render[0] = (struct Image **)&gad->GadgetRender;
    render[1] = (struct Image **)&gad->SelectRender;
    for (k = 0; k < 2; k++)
        if (*render[k] && (toolImage[i][k] = RemapToolImage(*render[k], toolPen[i], n)))
            *render[k] = toolImage[i][k];
}

static void FreeToolIcons(void)
{
    UWORD i, k;

    for (i = 0; i < BUTTON_COUNT; i++)
    {
        for (k = 0; k < 2; k++)
            if (toolImage[i][k])
            {
                FreeVec(toolImage[i][k]->ImageData);
                FreeVec(toolImage[i][k]);
                toolImage[i][k] = NULL;
            }
        for (k = 0; k < ICONPENS_MAX; k++)
            if (toolPenOwned[i][k])
            {
                ReleasePen(scr->ViewPort.ColorMap, toolPen[i][k]);
                toolPenOwned[i][k] = FALSE;
            }
        if (dob[i]) { FreeDiskObject(dob[i]);  dob[i] = NULL; }
    }
}

void CloseToolBarWindow(void)
{
    if (toolBarWin)
    {
        register struct MenuItem *item;

        ClearMenuStrip(toolBarWin);
        CloseWindow(toolBarWin);
        toolBarWin = NULL;
        FreeToolIcons();

        item = GetMenuItemFromID(MENU_TOOL_BAR);
        if (item != NULL)
            item->Flags &= ~CHECKED;
    }
}

void OpenToolBarWindow(char setmenus)
{
    if(!toolBarWin)
    {
        register struct Gadget *firstgad = 0;
        register struct Gadget *gad = 0;
        UWORD nextleft = scr->WBorLeft + 1, maxheight = 0, i = 0;
        WORD wintop, spacing = 5;

        wintop = STATE_IS(APP_FULLSCREEN) ? 0 : winTop;

        do
        {
            // The icons carry their colours: one set for any screen, drawn
            // for the shape of its pixels.
            strlcpy(buf, ScreenFont_TallPixels(modeResX, modeResY) ? "PROGDIR:ToolBar/Wide/"
                                                                   : "PROGDIR:ToolBar/Square/", sizeof(buf));
            strlcat(buf, icons[i], sizeof(buf));
            dob[i] = GetDiskObjectNew(buf);
            if(dob[i])
            {
                RemapToolIcon(i);
                if(gad) gad->NextGadget = &dob[i]->do_Gadget;
                gad = &dob[i]->do_Gadget;
                gad->NextGadget = 0;
                gad->LeftEdge = nextleft;
                gad->TopEdge = wintop + 1;
                if(gad->SelectRender == 0)
                    gad->Flags = GFLG_GADGIMAGE | GFLG_GADGHCOMP;
                else
                    gad->Flags = GFLG_GADGIMAGE | GFLG_GADGHIMAGE;
                gad->Activation = GACT_RELVERIFY;
                gad->GadgetType = GTYP_BOOLGADGET;
                //gad->GadgetText = 0;
                gad->GadgetID = i;
                gad->UserData = (APTR)dob[i]->do_ToolTypes[0];
                nextleft = gad->LeftEdge + gad->Width + spacing;
                if(!firstgad) firstgad = gad;
                if(gad->Height > maxheight) maxheight = gad->Height;
            }
            i++;
        }
        while(i != BUTTON_COUNT);

        if(!firstgad)
        {
            SimpleReq("No icons available.");
            STATE_UNSET(APP_TOOL_BAR_ENABLED);
            return;
        }

        if (STATE_IS_NOT(APP_FULLSCREEN))
        {
            newWin.LeftEdge = prefs.ToolBarWinLeftEdge;
            newWin.TopEdge  = prefs.ToolBarWinTopEdge;

            newWin.Width = gad->LeftEdge + gad->Width + scr->WBorRight + 1;
            newWin.Height = maxheight + wintop + scr->WBorBottom + 3 + scr->RastPort.Font->tf_YSize;
            newWin.Flags = WFLG_NOCAREREFRESH|WFLG_NEWLOOKMENUS|WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET;
            newWin.IDCMPFlags = IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_GADGETUP;
            newWin.Title = "Tool Bar";
        } else {

            spacing = scr->Width / i;
            nextleft = (scr->Width - ((scr->Width - spacing) + gad->Width)) / 2;
            i = 0;
            gad = firstgad;
            while(gad)
            {
                gad->LeftEdge = nextleft + (spacing * i);
                gad = gad->NextGadget;
                i++;
            }

            newWin.LeftEdge = 0;
            if STATE_IS(APP_TITLE_BAR_ENABLED)
                newWin.TopEdge = scr->BarHeight + 1;    // below the title bar as drawn
            else
                newWin.TopEdge = 0;

            newWin.Width = scr->Width;
            newWin.Height = maxheight + scr->RastPort.Font->tf_YSize + 4;
            newWin.Flags = WFLG_NOCAREREFRESH|WFLG_NEWLOOKMENUS|WFLG_BACKDROP|WFLG_BORDERLESS;
            newWin.IDCMPFlags = IDCMP_MENUPICK | IDCMP_GADGETUP | IDCMP_RAWKEY;
            newWin.Title = 0;
        }
        newWin.FirstGadget = firstgad;

        CheckDimensions(&newWin);

        toolBarWin = OpenWindow(&newWin);
        if (!toolBarWin)
            FreeToolIcons();
        if (toolBarWin)
        {
            if(setmenus) ResetMenuStrip(toolBarWin, mainMenuStrip);
            SetFont(toolBarWin->RPort, scr->RastPort.Font);
            SetAPen(toolBarWin->RPort, drawInfo->dri_Pens[TEXTPEN]);
            gad = firstgad;
            while(gad)
            {
                register UWORD len = strlen((char *)gad->UserData);

                Move(toolBarWin->RPort, gad->LeftEdge + ((gad->Width - (len*scr->RastPort.Font->tf_XSize)) / 2), wintop + maxheight + scr->RastPort.Font->tf_YSize - 1);
                Text(toolBarWin->RPort, (char *)gad->UserData, len);

                gad = gad->NextGadget;
            }
            if (STATE_IS(APP_FULLSCREEN))
            {
                SetAPen(toolBarWin->RPort, drawInfo->dri_Pens[SHINEPEN]);
                Move(toolBarWin->RPort, 0, toolBarWin->Height-2);
                Draw(toolBarWin->RPort, toolBarWin->Width, toolBarWin->Height-2);
                SetAPen(toolBarWin->RPort, drawInfo->dri_Pens[FILLPEN]);
                Move(toolBarWin->RPort, 0, toolBarWin->Height-1);
                Draw(toolBarWin->RPort, toolBarWin->Width, toolBarWin->Height-1);
            }
        }
    }
}
