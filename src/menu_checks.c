/* src/menu_checks.c -- see menu_checks.h. */
#include "menu_checks.h"
#include "charset.h"

static const struct { enum MenuItemID id; ULONG flag; } stateChecks[] = {
    { MENU_FULLSCREEN,          APP_FULLSCREEN },
    { MENU_LEDS,                APP_LEDS_ENABLED },
    { MENU_TITLE_BAR,           APP_TITLE_BAR_ENABLED },
    { MENU_BACKSPACE_DEL_SWAP,  APP_BACKSPACE_DEL_SWAPPED },
    { MENU_SCROLLBACK,          APP_SCROLLBACK_ENABLED },
    { MENU_PACKET_WINDOW,       APP_PACKET_WINDOW_ENABLED },
    { MENU_TOOL_BAR,            APP_TOOL_BAR_ENABLED },
    { MENU_RETURN_SENDING_CRLF, APP_RETURN_SENDING_CRLF },
    { MENU_VT_KEYS,             APP_VT_KEYS },
    { MENU_LOCAL_ECHO,          APP_LOCAL_ECHO },
    { MENU_RAW_CONNECTION,      APP_RAW_CONNECTION },
    { MENU_RLOGIN,              APP_RLOGIN },
    { MENU_FAST_SCROLL,         APP_FAST_SCROLL_ENABLED },
    { MENU_PETSCII_MODE,        APP_PETSCII_MODE },
};

BOOL MenuChecks_Checked(const struct PrefsStruct *p, enum MenuItemID id, BOOL *checked)
{
    size_t i;

    for (i = 0; i < sizeof(stateChecks) / sizeof(stateChecks[0]); i++)
    {
        if (stateChecks[i].id == id)
        {
            *checked = (p->State & stateChecks[i].flag) ? TRUE : FALSE;
            return TRUE;
        }
    }

    switch (id)
    {
    /* One renderer is checked; the first set bit wins, ibmcon is the default. */
    case MENU_BUILTIN_RENDERER:
        *checked = (p->State & APP_RENDERER_BUILTIN) ? TRUE : FALSE;
        return TRUE;
    case MENU_CONSOLE_DEVICE:
        *checked = !(p->State & APP_RENDERER_BUILTIN) && (p->State & APP_RENDERER_CONSOLE_DEVICE);
        return TRUE;
    case MENU_XEM_LIBRARY:
        *checked = !(p->State & (APP_RENDERER_BUILTIN | APP_RENDERER_CONSOLE_DEVICE))
                   && (p->State & APP_RENDERER_XEM_LIB);
        return TRUE;
    case MENU_IBMCON_DEVICE:
        *checked = !(p->State & (APP_RENDERER_BUILTIN | APP_RENDERER_CONSOLE_DEVICE
                                 | APP_RENDERER_XEM_LIB));
        return TRUE;

    /* 132 columns need ibmcon.device: other renderers never show the mark. */
    case MENU_132_COLUMNS:
        *checked = !(p->State & (APP_RENDERER_BUILTIN | APP_RENDERER_CONSOLE_DEVICE
                                 | APP_RENDERER_XEM_LIB))
                   && (p->State & APP_132_COLUMNS);
        return TRUE;

    case MENU_CHARSET_CP437:  *checked = (p->Charset == CHARSET_CP437);  return TRUE;
    case MENU_CHARSET_LATIN1: *checked = (p->Charset == CHARSET_LATIN1); return TRUE;
    case MENU_CHARSET_UTF8:   *checked = (p->Charset == CHARSET_UTF8);   return TRUE;

    case MENU_BELL_FLASH: *checked = (p->Bell == BELL_FLASH); return TRUE;
    case MENU_BELL_SOUND: *checked = (p->Bell == BELL_SOUND); return TRUE;
    case MENU_BELL_OFF:   *checked = (p->Bell == BELL_OFF);   return TRUE;

    case MENU_ANSI_MUSIC: *checked = p->AnsiMusic ? TRUE : FALSE; return TRUE;

    default:
        return FALSE;
    }
}
