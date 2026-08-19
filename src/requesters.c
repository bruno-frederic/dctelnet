/**
 * @file requesters.c
 * @brief Lightweight Intuition/ASL requesters for AmigaOS
 *
 * This module provides a set of lightweight requester wrappers built on top of native AmigaOS
 * Intuition, GadTools, and ASL libraries.
 *
 * It replaces the need for external requester libraries (such as ReqTools) while keeping
 * compatibility with AmigaOS 2.1+ (full ASL/GadTools support).
 *
 * The following requester types are implemented:
 *  - Informational and confirmation requesters (EasyRequestArgs-based, OS 2.0+)
 *  - String input requester (GadTools-based modal window, OS 2.0+)
 *  - File requester (ASL_FileRequest, OS 2.0+)
 *  - Directory requester (ASL_FileRequest with FIL1F_NOFILES mode, OS 2.0+)
 *  - Font requester (ASL_FontRequest, OS 2.0+)
 *  - Screen mode requester (ASL_ScreenModeRequest, OS 2.1+)
 *  - Recovery/fatal alert fallback system (DisplayAlert / Exec Alert, OS 1.0+)
 *
 * Compatibility strategy:
 *  - On Kickstart 1.x systems, only minimal functionality is available.
 *    ASL- and GadTools-based requesters are disabled at runtime, and calls fail safely without
 *    triggering crashes.
 *  - The only guaranteed UI fallback on very low system configurations is RecoveryAlert(), which
 *    uses Intuition DisplayAlert() or Exec Alert() when Intuition is unavailable.
 *  - All requester functions perform runtime library checks and return early on missing
 *    dependencies.
 *  - The caller is always notified of failures via InfoReq() (when possible) or RecoveryAlert()
 *    for critical situations.
 *
 * Design goals:
 *  - Minimize external dependencies
 *  - Ensure safe buffer handling and explicit size control
 *
 * Memory and safety rules:
 *  - All string buffers are caller-owned and explicitly size-bounded
 *
 * Library requirements:
 *  - intuition.library v36+ (AmigaOS 2.0+)
 *  - gadtools.library (AmigaOS 2.0+)
 *  - asl.library v38+ (AmigaOS 2.1+, for screen mode requester)
 *
 * @author Bruno FREDERIC
 * @date 2026
 */


#include <proto/exec.h>       // Alert()
#include <proto/intuition.h>  // OpenWindow(), CloseWindow(), (Un)lockPubScreen() ActivateGadget(),
                              // EasyRequestArgs()
#include <proto/graphics.h>   // TextLength()
#include <proto/gadtools.h>   // CreateGadget(), CreateContext(), GetVisualInfoA()
                              // GT_GetIMsg(), GT_ReplyIMsg, GT_RefreshWindow()
#include <proto/asl.h>        // FileRequester, FontRequester, ScreenModeRequester
#include <exec/alerts.h>      // AN_Unknown, AG_OpenLib, AO_DOSLib (values for Alert()
#include <stdarg.h>           // va_list, va_start(), va_end()
#include <string.h>           // strlen(), memset(), size_t
#include "requesters.h"


// Calling module must provide these:
// Safer string copy than strcpy() and strncpy(). Note: this is a POSIX function.
extern size_t strlcpy(char *dst, const char *src, size_t dstSize);


// Maximum number of characters that fit entirely on one DisplayAlert()
// line on a standard Amiga PAL/NTSC screen using the default alert font.
#define ALERT_MAX_DISPLAYABLE_CHARS  77

static const TEXT ALERT_SECOND_LINE[] = "Press mouse button to continue";

// DisplayAlert() format overhead:
//   3 bytes position per line,
//   1 terminating '\0' per line,
//   1 continuation byte per line (including the last one).
// sizeof(ALERT_SECOND_LINE) already includes its terminating '\0'.
#define ALERT_BUFFER_SIZE   (3 + ALERT_MAX_DISPLAYABLE_CHARS + 2 + \
                             3 + sizeof(ALERT_SECOND_LINE) + 1)

/**
 * @brief Display a critical error message using the most reliable mechanism available.
 *
 * This function is intended for fatal or near-fatal errors occurring during program initialization
 * or when normal requester functions cannot be used.
 *
 * It first attempts to display a standard Intuition recovery alert using DisplayAlert(). If
 * intuition.library is not currently open, the function temporarily opens it using OldOpenLibrary()
 * for maximum compatibility with older Kickstart versions.
 *
 * If intuition.library cannot be opened, the function falls back to the Exec Alert() mechanism,
 * which may display a Guru Meditation–style error on very old systems.
 *
 *
 * @param msg
 *        Short error message to display in the alert box (less than 100 characters).
 *
 * @note Intended for exceptional situations only. Do not use for normal informational or warning
 *       messages.
 *
 *
 * @todo test under old Kickstart 1.*
 */
VOID RecoveryAlert(CONST_STRPTR msg)
{
    BOOL shouldCloseIntuitionLibrary = FALSE;

    if (IntuitionBase == NULL)
    {
        // Try to open the library to display a proper visual alert.
        // This is a best-effort fallback used in recovery context.
        // (OldOpenLibrary() to be KS 1.0 compatible)
        IntuitionBase = (struct IntuitionBase *) OldOpenLibrary("intuition.library");
        shouldCloseIntuitionLibrary = TRUE;
    }

    if (IntuitionBase == NULL)
    {
        // System in bad shape: issue an Exec Alert()
        //https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01E3.html
        Alert(AT_DeadEnd | AN_Unknown | AG_OpenLib | AO_Intuition);   // Error number: B503 8004
    }
    else
    {
        /*
        * Each line needs 2 bytes of X position, and 1 byte of Y position.
        *    In our 1st line: x = \x00\xF0 (2 bytes) and y = \x14 (1 byte)
        *    In our 2nd line: x = \x00\xA0 (2 bytes) and y = \x24 (1 byte)
        * Each line is null terminated plus a continuation character (0=done).
        */
        TEXT alertMsg[ALERT_BUFFER_SIZE];
        TEXT *p = alertMsg;
        TEXT *s;
        size_t len;
        size_t i;
        size_t max_copy;


        // First line header : 16 bit x-coordinate and an 8 bit y-coordinate
        *p++ = 0x00;  *p++ = 0x10;  *p++ = 0x14;

        // Safe strlen()  (avoid function calls to remain safe in low-memory / recovery context)
        len = 0;
        if (msg != NULL)
        {
            while (len < ALERT_MAX_DISPLAYABLE_CHARS  &&  msg[len] != '\0')
                len++;

            max_copy = (msg[len] == '\0') ? len : (ALERT_MAX_DISPLAYABLE_CHARS - 3);
            // -3 chars when we didn’t find the string NUL terminal char. The msg is too long and
            // 3 "." will truncate the message.

            // First line copy avoiding function calls to remain safe in low-memory / recovery context.
            // Manual copy ensures no dependency on external C or OS libraries
            for (i = 0; i < max_copy; i++)
                *p++ = msg[i];

            if (len > max_copy) // Truncate with "..."
            {
                *p++ = '.'; *p++ = '.'; *p++ = '.';
            }
        }

        *p++ = '\0';    // End of first string

        // Continuation byte: non-zero, there is another substring in this alert message.
        *p++ = 0x01;

        // Second line header : 16 bit x-coordinate and an 8 bit y-coordinate
        *p++ = 0x00;  *p++ = 0x80;  *p++ = 0x24;

        // Second line copy
        s = (TEXT *) ALERT_SECOND_LINE;
        while (*s != '\0')
            *p++ = *s++;
        *p++ = '\0';

        // Continuation byte: zero, this is the last substring in this alert message.
        *p++ = 0x00;

        DisplayAlert(RECOVERY_ALERT,  // RECOVERY_ALERT or DEADEND_ALERT
                     alertMsg,        // string that is made up of one or more substrings
                     52);             // the required display height
    }


    // IntuitionBase is a shared global; we only close it if it was opened locally in this function.
    if (shouldCloseIntuitionLibrary && IntuitionBase != NULL)
    {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
    }
}


/**
 * @brief Display a simple informational requester with an OK button.
 *
 * This is a lightweight wrapper around EasyRequestArgs() that shows a modal message requester using
 * only native AmigaOS Intuition functionality.
 *
 * The requester displays the supplied message text and a single "OK" button. It can be attached to
 * a parent window or, if @p parent is NULL, it will be opened on the default public screen
 * (typically the Workbench screen).
 *
 *
 * @param parent
 *        Parent window for the requester, or NULL to display it on the default public screen.
 *
 * @param str
 *        Message text to display. The string is interpreted as an EasyRequest() format string.
 */
VOID InfoReq(struct Window *parent, CONST_STRPTR str, ...) // varargs parameters
{
    va_list args;

    struct EasyStruct es = {
        sizeof(struct EasyStruct),  // es_StructSize
        0,                          // es_Flags
        "Information",              // es_Title
        NULL,                       // es_TextFormat
        "OK"                        // es_GadgetFormat
    };
    es.es_TextFormat = str;     // SAS/C won’t accept a non-literal when intializing a struct


    if (IntuitionBase == NULL || IntuitionBase->LibNode.lib_Version < 36)  // we check at runtime
    {
        RecoveryAlert("InfoReq() requires Intuition library v36 or newer (AmigaOS 2.0+).");
        return;
    }


    #ifdef __VBCC__
    #pragma dontwarn 79 // warning 79: offset equals size of object
    #endif
    va_start(args, str);
    #ifdef __VBCC__
    #pragma popwarn
    #endif

    EasyRequestArgs(parent, // This can be NULL; requester will appear on the Workbench screen
                    &es,
                    NULL,   // idcmpPtr
                    (APTR)args);

    va_end(args);
}



/**
 * @brief Display a confirmation requester and return the selected button.
 *
 * The message text may contain format specifiers understood by EasyRequest().
 * Additional arguments are passed through to EasyRequestArgs().
 *
 *
 *
 * @param parent
 *        Parent window, or NULL to open on the Workbench screen.
 *
 * @param str
 *        Message format string.
 *        It can be specified using a printf()-style format string that also accepts variables as
 *        part of the text. If variables are specified in the text, their value is taken from the
 *        varargs parameters.
 *
 * @param gadgetFormat
 *        Gadget labels separated by '|'.
 *        Example: "Yes|No" or "Save|Cancel".
 *
 * @return
 *        Button number selected by the user.
 *        Button numbering starts at 1 from left to right.
 *        The rightmost button is conventionally the negative reply.
 */
LONG ConfirmRequester(struct Window *parent, CONST_STRPTR gadgetFormat, CONST_STRPTR str,
                      ...) // varargs parameters
{
    va_list args;
    LONG answer = FALSE;

    struct EasyStruct es =
    {
        sizeof(struct EasyStruct),  // es_StructSize
        0,                          // es_Flags
        "Confirmation",             // es_Title
        NULL,                       // es_TextFormat
        NULL                        // es_GadgetFormat
    };
    es.es_TextFormat = str;     // SAS/C won’t accept a non-literal when intializing a struct
    es.es_GadgetFormat = gadgetFormat;


    if (IntuitionBase == NULL || IntuitionBase->LibNode.lib_Version < 36)  // we check at runtime
    {
        RecoveryAlert("ConfirmRequester() requires Intuition library v36 or newer (AmigaOS 2.0+).");
        return answer;
    }

    #ifdef __VBCC__
    #pragma dontwarn 79 // warning 79: offset equals size of object
    #endif
    va_start(args, str);
    #ifdef __VBCC__
    #pragma popwarn
    #endif

    // EasyRequest() provides a simple way to make a requester that allows the user to select one of
    // a limited number of choices.
    answer =  EasyRequestArgs(parent, // This can be NULL; requester will appear on the WB screen
                              &es,
                              NULL,   // idcmpPtr
                              (APTR)args);

    va_end(args);

    return answer;
}


enum
{
    GID_STRING = 1,
    GID_OK,
    GID_CANCEL
};

/**
 * @brief Displays a modal string requester using GadTools.
 *
 * This function is intended as a replacement for ReqTools' rtGetStringA() function.
 *
 * @param parent
 *        Parent window. May be NULL.
 *
 * @param title
 *        Requester window title.
 *
 * @param prompt
 *        Text displayed next to the string gadget.
 *
 * @param buffer
 *        Editable string buffer.
 *
 * @param maxLen
 *        Maximum buffer length, including the terminating NUL character.
 *        The user cannot enter more than maxLen - 1 characters.
 *
 * @return TRUE if the user validated the requester using the OK button or the Return key.
 *
 * @return FALSE if the requester was cancelled using the Cancel button, the Escape key, or the
 *         window close gadget.
 *
 * @note
 * Another possible approach, not explored due to the lack of examples, would be to create a
 * Intuition requester containing gadgets rather than creating a dedicated window with gadgets:
 *
 * https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node01B4.html
 *
 * However, requester gadgets send Intuition messages to the parent window's message port, which may
 * complicate event handling.
 *
 * @see
 * Amiga ROM Kernel Reference Manual: Libraries, 2nd Edition,
 * Chapter 7 "Intuition Requesters and Alerts".
 *
 * Additional information for AmigaOS Release 2 can be found in:
 * Chapter 16 "ASL Library".
 */
BOOL GetStringRequester(struct Window *parent, STRPTR title, STRPTR prompt,
                        STRPTR buffer, UWORD maxLen)
{
    struct Screen     *screen;

    struct RastPort   *rp = NULL;
    struct VisualInfo *vi = NULL;
    struct Window     *win = NULL;

    struct Gadget *gadList   = NULL;
    struct Gadget *gad       = NULL;
    struct Gadget *stringGad = NULL;

    struct NewGadget ng;

    UWORD winWidth;
    UWORD winHeight;
    LONG  winLeft;
    LONG  winTop;
    UWORD titlebarHeight;

    UWORD fontYsize;
    WORD  yMargin;
    WORD  xMargin;

    WORD  gadgetHeight;
    WORD  promptWidth;
    WORD  bufferWidth;
    WORD  fieldWidth;
    WORD  buttonWidth;

    BOOL  result = FALSE;
    BOOL  done;


    #ifdef _DEBUG
        if (GfxBase == NULL)
            { RecoveryAlert("Graphics library is not opened!"); return result; }
        if (IntuitionBase == NULL)
            { RecoveryAlert("Intuition library is not opened!"); return result; }
    #endif

    if (GadToolsBase == NULL)  // absent on OS 1.* so we check at runtime:
    {
        RecoveryAlert("GetStringRequester() requires GadTools library (AmigaOS 2.0+).");
        return result;
    }

    if (maxLen < 1)
    {
        InfoReq(parent, "GetStringRequester(): maxLen must be >= 1");
        return result;
    }

    if (prompt == NULL || buffer == NULL)
    {
        InfoReq(parent, "GetStringRequester(): parameters must not be NULL");
        return result;
    }


    // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node03C1.html
    // Use the default public screen when no parent window is provided.
    screen = parent != NULL ? parent->WScreen : LockPubScreen(NULL);
    if (screen == NULL) { InfoReq(parent, "LockPubScreen() => NULL (failed)"); goto clean_and_return; }

    vi = GetVisualInfoA(screen, NULL);
    if (vi  == NULL) { InfoReq(parent, "GetVisualInfoA() => NULL (failed)"); goto clean_and_return; }

    rp = &screen->RastPort;

    // If the font height cannot be determined, assume 20 pixels.
    fontYsize = (screen->Font == NULL) ? 20 : screen->Font->ta_YSize;

    yMargin = fontYsize;
    xMargin = TextLength(rp, "W", strlen("W"));  // "W" is widest character in ISO-8859-1

    // Dynamically calculate gadget dimensions.
    gadgetHeight = fontYsize + yMargin;

    // Measure the pixel width of the prompt and the current buffer content using the current screen
    // font. This ensures the requester is wide enough to display the longest of the two without
    // clipping.
    promptWidth = TextLength(rp, prompt, strlen(prompt));
    bufferWidth = TextLength(rp, buffer, strlen(buffer));

    //  We’ll use the larger of the two widths to size the gadget:
    fieldWidth = (promptWidth > bufferWidth) ? promptWidth : bufferWidth;
    fieldWidth += xMargin;

    buttonWidth  = TextLength(rp, "Cancel", strlen("Cancel")) + xMargin;

    // Predict window’s titlebar height before calling OpenWindow():
    titlebarHeight = screen->WBorTop + fontYsize + 1;

    // Calculate the required window size:
    winWidth = xMargin + fieldWidth + xMargin + fieldWidth + xMargin;
    winHeight = titlebarHeight
              + yMargin
              + gadgetHeight // String gadget
              + yMargin
              + gadgetHeight // [OK] [Cancel] button gadgets
              + yMargin;

    // Impose a minimal window size:
    if (winWidth  < 200)  winWidth = 200;
    if (winHeight <  70)  winHeight = 70;

    // Reduce the requester size if it exceeds the available space,
    // then center it within the parent window or screen.
    if (parent == NULL)
    {
        // Oversized:
        if (winWidth  > screen->Width)
        {
            winWidth    = screen->Width;
            fieldWidth = (winWidth - 3 * xMargin) / 2;
        }
        if (winHeight > screen->Height)  winHeight = screen->Height - titlebarHeight;

        // Centering:
        winLeft = (screen->Width  - winWidth) / 2;
        winTop  = (screen->Height - winHeight) / 2;
    }
    else
    {
        if (winWidth  > parent->Width)
        {
            winWidth = parent->Width;
            fieldWidth = (winWidth - 3 * xMargin) / 2;
        }
        if (winHeight > parent->Height)  winHeight = parent->Height - titlebarHeight;

        winLeft = parent->LeftEdge + (parent->Width  - winWidth) / 2;
        winTop  = parent->TopEdge  + (parent->Height - winHeight) / 2;
    }

    gad = CreateContext(&gadList);
    if (gad == NULL || gadList == NULL)
    { InfoReq(parent, "CreateContext() => NULL (failed)"); goto clean_and_return; }

    memset(&ng, 0, sizeof(ng));

    // String gadget
    ng.ng_GadgetText = prompt;
    ng.ng_GadgetID   = GID_STRING;
    ng.ng_TextAttr   = screen->Font;
    ng.ng_VisualInfo = vi;

    ng.ng_TopEdge  = titlebarHeight + yMargin;

    // Position the input field at the horizontal midpoint of the window.
    // The left half of the requester is reserved for the prompt text,
    // while the right half is used for user input.
    ng.ng_LeftEdge = winWidth / 2;

    // Use the right half of the requester for text input while keeping a small right margin:
    ng.ng_Width      = winWidth - ng.ng_LeftEdge - xMargin;

    // Must be large enough to accommodate the selected font; otherwise CreateGadget() will fail:
    ng.ng_Height     = gadgetHeight;


    // https://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node025A.html
    // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0275.html
    // Set the prevgad argument to the gadget address returned by CreateContext() if this is the
    // first (or only) gadget in the list.
    stringGad = gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_String,   buffer,
        GTST_MaxChars, maxLen - 1,  // UWORD
        TAG_DONE);

    if (gad == NULL) { InfoReq(parent, "CreateGadget() => NULL (failed)"); goto clean_and_return; }

    // [OK] button
    ng.ng_GadgetText = "OK";
    ng.ng_GadgetID   = GID_OK;
    ng.ng_LeftEdge   = xMargin;
    ng.ng_TopEdge    += gadgetHeight + yMargin;
    ng.ng_Width      = buttonWidth;
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (gad == NULL) { InfoReq(parent, "CreateGadget() => NULL (failed)"); goto clean_and_return; }

    // [Cancel] button
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID   = GID_CANCEL;
    ng.ng_LeftEdge   = winWidth - xMargin - buttonWidth;
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (gad == NULL) { InfoReq(parent, "CreateGadget() => NULL (failed)"); goto clean_and_return; }


    win = OpenWindowTags(NULL,
        WA_Title,       (ULONG)title,
        WA_Left,        winLeft,
        WA_Top,         winTop,
        WA_Width,       winWidth,
        WA_Height,      winHeight,

        WA_CloseGadget, TRUE,
        WA_DragBar,     TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate,    TRUE,

        parent ? WA_CustomScreen : WA_PubScreen, screen,

        WA_IDCMP,
            IDCMP_CLOSEWINDOW |
            IDCMP_GADGETUP |
            IDCMP_VANILLAKEY,

        WA_Gadgets, (ULONG)gadList,

        TAG_DONE);

    if (win == NULL)  { InfoReq(parent, "OpenWindowTags() => NULL (failed)"); goto clean_and_return; }

    // Note that once a window is open on the screen the program does not need to hold the screen
    // lock, as the window acts as a lock on the screen.
    if (parent == NULL && screen != NULL)
    {
        UnlockPubScreen(NULL, screen);
        screen = NULL;
    }


    GT_RefreshWindow(win, NULL);

    ActivateGadget(stringGad, win, NULL);

    done = FALSE;
    while (! done)
    {
        struct IntuiMessage *msg;

        Wait(1L << win->UserPort->mp_SigBit);

        while ((msg = GT_GetIMsg(win->UserPort)))
        {
            ULONG class = msg->Class;
            UWORD code  = msg->Code;
            UWORD gid   = 0;

            if (class == IDCMP_GADGETUP)
                gid = ((struct Gadget *)msg->IAddress)->GadgetID;

            GT_ReplyIMsg(msg);

            switch(class)
            {
                case IDCMP_CLOSEWINDOW:
                    done = TRUE;
                break;

                case IDCMP_VANILLAKEY:

                    switch(code)
                    {
                        case 27:    // ESC
                            done = TRUE;
                        break;

                        case '\r':  // RETURN
                            result = TRUE;
                            done = TRUE;
                        break;
                    }
                break;

                case IDCMP_GADGETUP:

                    switch(gid)
                    {
                        case GID_OK:
                            result = TRUE;
                            done = TRUE;
                        break;

                        case GID_CANCEL:
                            done = TRUE;
                        break;
                    }
                break;
            }
        }
    }

    if (result == TRUE)
    {
        // The gadget buffer is expected to be limited by "GTST_MaxChars, maxLen - 1",
        // but this provides additional protection against unexpected gadget behavior.
        strlcpy(buffer,
                ((struct StringInfo *)stringGad->SpecialInfo)->Buffer,
                maxLen);
    }

clean_and_return:

    if (win     != NULL)  CloseWindow(win);
    if (gadList != NULL)  FreeGadgets(gadList);
    if (vi      != NULL)  FreeVisualInfo(vi);

    if (parent == NULL && screen != NULL)
        UnlockPubScreen(NULL, screen);

    return result;
}


/**
 * @brief Display an ASL file requester and return the selected file.
 *
 * Opens a standard AmigaOS ASL file requester initialized with the directory, file name, and
 * pattern supplied by the caller.
 *
 * Depending on @p mode, the requester operates either in file selection mode or in save mode.
 *
 * If the user confirms the requester, the selected directory and file name are copied back into the
 * buffers provided by the caller.
 *
 *
 * @param parent
 *        Parent window for the requester, or NULL to open it on the default public screen.
 *
 * @param dirName
 *        Input/output directory buffer.
 *        On entry, provides the initial directory displayed by the requester.
 *        On successful return, receives the selected directory.
 *        May be NULL only if @p dirMaxLen is zero.
 *
 * @param dirMaxLen
 *        Size of the @p dirName buffer, including the terminating NUL character.
 *        If zero, the selected directory is not returned and @p dirName may be NULL.
 *
 * @param fileName
 *        Input/output file name buffer.
 *        On entry, provides the initial file name displayed by the requester.
 *        On successful return, receives the selected file name.
 *        May be NULL only if @p fileMaxLen is zero.
 *
 * @param fileMaxLen
 *        Size of the @p fileName buffer, including the terminating NUL character.
 *        If zero, the selected file name is not returned and @p fileName may be NULL.
 *
 * @param pattern
 *        Optional ASL file pattern used to filter displayed files.
 *        May be NULL to disable pattern filtering.
 *
 * @param mode
 *        Requester operating mode.
 *        Use FILEREQ_LOAD to select an existing file or FILEREQ_SAVE to create
 *        or overwrite a file.
 *
 * @return TRUE if the user selected a file and confirmed the requester.
 * @return FALSE if the requester was cancelled or an error occurred.
 *
 * @note The caller must have opened asl.library before calling this function.
 *
 * @note The @p dirName and @p fileName parameters are both input and output.
 *
 * @note Passing a buffer size of zero disables the corresponding output value.
 *       In that case, the associated pointer may be NULL.
 */
BOOL FileRequester(struct Window *parent, STRPTR dirName, UWORD dirMaxLen,
                   STRPTR fileName, UWORD fileMaxLen,
                   STRPTR pattern,
                   FileRequesterMode mode)
{
    struct FileRequester *fr = NULL;
    BOOL  result = FALSE;

    ULONG funcFlags = FILF_PATGAD;  // Enable pattern match gadget

    if (mode == FILEREQ_SAVE) funcFlags |= FILF_SAVE; // To create a save requester

    if (AslBase == NULL)  // absent on OS 1.* so we check at runtime:
    {
        RecoveryAlert("FileRequester() requires ASL.library (AmigaOS 2.0+).");
        return result;
    }

    if (dirName == NULL && dirMaxLen != 0)
    {
        InfoReq(parent, "FileRequester() called with a NULL dirName and non-zero dirMaxLen!");
        return result;
    }

    if (fileName == NULL && fileMaxLen != 0)
    {
        InfoReq(parent, "FileRequester() called with a NULL fileName and non-zero fileMaxLen!");
        return result;
    }

    fr = AllocAslRequestTags(ASL_FileRequest,    // type of requester
                            ASL_Window, parent,

                            // Supply initial values for requester:
                            ASL_Dir,  dirName,
                            ASL_File, fileName,
                            ASL_Pattern, pattern,

                            ASL_FuncFlags, funcFlags,

                            TAG_DONE);

    if (fr == NULL)  { InfoReq(parent, "AllocAslRequestTags() => NULL (failed)");
                               goto clean_and_return; }

    // https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node005B.html
    result = AslRequest(fr, NULL);

    if (result) // The user selected something
    {
        strlcpy(dirName, fr->rf_Dir, dirMaxLen);
        strlcpy(fileName, fr->rf_File , fileMaxLen);
    }

clean_and_return:
    if (fr != NULL)  FreeAslRequest(fr);

    return result;
}


/**
 * @brief Display a directory requester and return the selected directory.
 *
 * Opens a standard AmigaOS ASL directory requester initialized with the directory name supplied by
 * the caller.
 *
 * If the user selects a directory and confirms the requester, the selected directory name is copied
 * back into the provided buffer.
 *
 * This requester is a wrapper around the ASL FileRequest class configured in directory-only mode
 * (FIL1F_NOFILES).
 *
 * @param parent
 *        Parent window for the requester, or NULL to open it on the default public screen.
 *
 * @param dirName
 *        Input/output directory buffer.
 *        On entry, contains the initial directory displayed by the requester.
 *        On successful return, contains the selected directory.
 *        Must not be NULL.
 *
 * @param dirMaxLen
 *        Size of the @p dirName buffer, including the terminating NUL character.
 *        Must be greater than zero.
 *
 * @return TRUE if the user selected a directory and confirmed the requester.
 * @return FALSE if the requester was cancelled or an error occurred.
 *
 * @note The caller must have opened asl.library before calling this function.
 *
 * @note This function uses FIL1F_NOFILES to restrict the requester to
 *       directory selection only.
 *
 * @note The @p dirName parameter is both input and output.
 */
BOOL DirectoryRequester(struct Window *parent, STRPTR dirName, UWORD dirMaxLen)
{
    struct FileRequester *fr = NULL;
    BOOL  result = FALSE;

    if (AslBase == NULL)  // absent on OS 1.* so we check at runtime:
    {
        RecoveryAlert("DirectoryRequester() requires ASL.library (AmigaOS 2.0+).");
        return result;
    }

    if (dirName == NULL || dirMaxLen == 0)
    {
        InfoReq(parent, "DirectoryRequester() called with a NULL dirName or a zero maxLen!");
        return result;
    }


    fr = AllocAslRequestTags(ASL_FileRequest,    // type of requester
                             ASL_Window, parent,
                             ASL_Hail,   "Select Drawer",
                             // Supply initial values for requester:
                             ASL_Dir,    dirName,

                             // If the FIL1F_NOFILES flag is set, the requester will appear without
                             // a string gadget for file names and will display only directory names
                             // in the scrolling list gadget.
                             ASL_ExtFlags1, FIL1F_NOFILES,

                             TAG_DONE);

    if (fr == NULL)  { InfoReq(parent, "AllocAslRequestTags() => NULL (failed)");
                               goto clean_and_return; }
    result = AslRequest(fr, NULL);

    if (result) // The user selected something
        strlcpy(dirName, fr->rf_Dir, dirMaxLen);

clean_and_return:
    if (fr != NULL)  FreeAslRequest(fr);

    return result;
}


/**
 * @brief Display a font requester and return the selected font.
 *
 * Opens a standard AmigaOS ASL font requester initialized with the font name and height supplied by
 * the caller.
 *
 * Only fixed-width fonts are displayed, making this requester suitable for terminal, console, and
 * text-oriented applications.
 *
 * If the user selects a font and confirms the requester, both the font name and font height are
 * updated with the selected values.
 *
 * @param parent
 *        Parent window for the requester, or NULL to open it on the default public screen.
 *
 * @param fontName
 *        Input/output font name buffer.
 *        On entry, contains the initial font name displayed by the requester.
 *        On successful return, receives the selected font name.
 *        Must not be NULL.
 *
 * @param maxLen
 *        Size of the @p fontName buffer, including the terminating NUL character. Must be greater
 *        than zero.
 *
 * @param fontYSize
 *        Input/output font height.
 *        On entry, specifies the initial font height displayed by the requester. On successful
 *        return, receives the selected font height.
 *        Must not be NULL.
 *
 * @return TRUE if the user selected a font and confirmed the requester.
 * @return FALSE if the requester was cancelled or an error occurred.
 *
 * @note The caller must have opened asl.library before calling this function.
 *
 * @note The @p fontName and @p fontYSize parameters are both input and output.
 *
 * @note Only fixed-width fonts are displayed.
 */
BOOL FontRequester(struct Window *parent, STRPTR fontName, UWORD maxLen, UWORD *fontYSize)
{
    struct FontRequester *fr = NULL;
    BOOL  result = FALSE;

    if (AslBase == NULL)  // absent on OS 1.* so we check at runtime:
    {
        RecoveryAlert("FontRequester() requires ASL.library (AmigaOS 2.0+).");
        return result;
    }

    if (fontName == NULL || maxLen == 0)
    {
        InfoReq(parent, "FontRequester() called with a NULL fontName or a zero maxLen!");
        return result;
    }

    if (fontYSize == NULL)
    {
        InfoReq(parent, "FontRequester() called with a NULL fontYSize!");
        return result;
    }

    fr = AllocAslRequestTags(ASL_FontRequest,    // type of requester
                             ASL_Window,      parent,
                             // Supply initial values for requester:
                             ASL_FontName,    fontName,
                             ASL_FontHeight, *fontYSize,
                             ASL_FrontPen,    0x01L,
                             ASL_BackPen,     0x00L,

                             ASL_FuncFlags, FONF_FIXEDWIDTH | // Only show fixed width fonts
                                            FONF_DRAWMODE,
                             TAG_DONE);

    if (fr == NULL)  { InfoReq(parent, "AllocAslRequestTags() => NULL (failed)");
                               goto clean_and_return; }

    result = AslRequest(fr, NULL);

    if (result) // The user selected something
    {
        strlcpy(fontName, fr->fo_Attr.ta_Name, maxLen);

        *fontYSize = fr->fo_Attr.ta_YSize;
    }

clean_and_return:
    if (fr != NULL)  FreeAslRequest(fr);

    return result;
}


/**
 * @brief Display an ASL screen mode requester and return the selected mode.
 *
 * Opens the AmigaOS Screen Mode requester, allowing the user to select a display mode, resolution,
 * and color depth. The requester is initialized with the values provided by the caller.
 *
 * On successful confirmation, all output parameters are updated with the selected values.
 *
 * This requester requires:
 * - asl.library v38+ (AmigaOS 2.1+)
 * - graphics.library v36+ (for ModeNotAvailable() validation)
 *
 * @param parent
 *        Parent window for the requester, or NULL to open it on the default public screen.
 *
 * @param displayID
 *        Pointer to the initial display mode ID. On success, receives the selected display ID.
 *        Must not be NULL.
 *
 * @param displayWidth
 *        Pointer to the initial display width. On success, receives the selected width.
 *        Must not be NULL.
 *
 * @param displayHeight
 *        Pointer to the initial display height. On success, receives the selected height.
 *        Must not be NULL.
 *
 * @param displayDepth
 *        Pointer to the initial bitplane depth. On success, receives the selected depth.
 *        Must not be NULL.
 *
 * @return TRUE if the user selected a valid screen mode and confirmed the requester.
 * @return FALSE if the requester was cancelled, an error occurred, or the selected mode is not
 *         available on the system.
 *
 * @note The requester is currently limited to a maximum depth of 4 bitplanes (16 colors).
 *
 * @note All pointer parameters are mandatory and must be non-NULL.
 */
BOOL ScreenModeRequester(struct Window *parent, ULONG* displayID,
                         UWORD* displayWidth, UWORD* displayHeight, UWORD* displayDepth)
{
    struct ScreenModeRequester *sr = NULL;
    BOOL  result = FALSE;

    if (AslBase == NULL || AslBase->lib_Version < 38)  // we check at runtime:
    {
        RecoveryAlert("Screen mode selection requires ASL.library v38 or newer (AmigaOS 2.1+).");
        return result;
    }

    if (GfxBase == NULL || GfxBase->LibNode.lib_Version < 36)  // we check at runtime:
    {   // needed for ModeNotAvailable()
        RecoveryAlert(
                    "Screen mode selection requires graphics.library v36 or newer (AmigaOS 2.0+).");
        return result;
    }

    if (displayID == NULL || displayWidth == NULL || displayHeight == NULL || displayDepth == NULL)
    {
        InfoReq(parent, "ScreenModeRequester() called with a NULL argument!");
        return result;
    }

    sr = AllocAslRequestTags(ASL_ScreenModeRequest, // requester type
                        ASL_Window, parent,

                        // Supply initial values for requester:
                        ASLSM_InitialDisplayID,     *displayID,
                        ASLSM_InitialDisplayWidth,  *displayWidth,
                        ASLSM_InitialDisplayHeight, *displayHeight,
                        ASLSM_InitialDisplayDepth,  *displayDepth,   // # of bitlplanes


                        ASLSM_DoWidth,        TRUE,
                        ASLSM_DoHeight,       TRUE,
                        ASLSM_DoDepth,        TRUE,
                        ASLSM_MinWidth,       640,
                        ASLSM_MaxDepth,       4,    // Limit to 4 bitplanes (16 colors) sreen modes

                        TAG_DONE);

    if (sr == NULL)  { InfoReq(parent, "AllocAslRequestTags() => NULL (failed)");
                               goto clean_and_return; }

    result = AslRequest(sr, NULL);

    if (result) // The user selected something
    {
        if ( ModeNotAvailable(sr->sm_DisplayID))    // Check if the user selected something viable
        {
            InfoReq(parent, "Screen mode is not usable on your system!");
            result = FALSE;
        }
        else
        {
            *displayID     = sr->sm_DisplayID;
            *displayWidth  = sr->sm_DisplayWidth;
            *displayHeight = sr->sm_DisplayHeight;
            *displayDepth  = sr->sm_DisplayDepth;
        }
    }

clean_and_return:
    if (sr != NULL)  FreeAslRequest(sr);

    return result;
}
