/* src/keys.c -- keys typed in the terminal, as the BBS expects them. */
#include "keys.h"

#define CSI_BYTE 0x9B

size_t Keys_Next(const UBYTE *buf, size_t len, int *id, UBYTE *text)
{
    size_t i;
    UWORD num = 0;
    BOOL digits = FALSE, space = FALSE, question = FALSE, several = FALSE;

    *id = KEY_NONE;
    if (len == 0)
        return 0;
    if (buf[0] != CSI_BYTE)
    {
        *id = KEY_TEXT;
        *text = buf[0];
        return 1;
    }
    /* Parameters and intermediates, then a final byte ('@'..'~'). */
    for (i = 1; i < len; i++)
    {
        UBYTE c = buf[i];

        if (c >= '0' && c <= '9')
        {
            if (num < 1000) num = (UWORD)(num * 10 + (c - '0'));
            digits = TRUE;
        }
        else if (c == ';')
            several = TRUE;                 /* two parameters: no key of ours */
        else if (c == ' ')
            space = TRUE;
        else if (c == '?')
            question = TRUE;
        else if (c >= '@' && c <= '~')
        {
            if (several)
                ;                               /* KEY_NONE */
            else if (c == '~' && question && !digits)
                *id = KEY_HELP;
            else if (c == '~' && digits && !space && !question)
            {
                if (num <= 9)                    *id = KEY_F1 + num;
                else if (num <= 19)              *id = KEY_SHIFT_F1 + (num - 10);
                else if (num == 40)              *id = KEY_INSERT;
                else if (num == 41)              *id = KEY_PAGE_UP;
                else if (num == 42)              *id = KEY_PAGE_DOWN;
                else if (num == 44)              *id = KEY_HOME;
                else if (num == 45)              *id = KEY_END;
            }
            else if (!digits && !question)
            {
                if (space)
                {
                    if (c == 'A')      *id = KEY_HOME;        /* Shift-Left */
                    else if (c == '@') *id = KEY_END;         /* Shift-Right */
                }
                else switch (c)
                {
                case 'A': *id = KEY_UP;        break;
                case 'B': *id = KEY_DOWN;      break;
                case 'C': *id = KEY_RIGHT;     break;
                case 'D': *id = KEY_LEFT;      break;
                case 'T': *id = KEY_PAGE_UP;   break;   /* Shift-Up */
                case 'S': *id = KEY_PAGE_DOWN; break;   /* Shift-Down */
                }
            }
            return i + 1;
        }
        else
            break;                          /* not a key sequence: stop here */
    }
    return i;
}

int Keys_FromRawCode(UWORD code)
{
    switch (code)
    {
    case 0x47: return KEY_INSERT;
    case 0x48: return KEY_PAGE_UP;
    case 0x49: return KEY_PAGE_DOWN;
    case 0x70: return KEY_HOME;
    case 0x71: return KEY_END;
    }
    return KEY_NONE;
}

size_t Keys_Bytes(int id, BOOL vtKeys, BOOL cursorKeyMode, char out[KEYS_MAX_BYTES])
{
    static const char *const bbs[] = { "\033[H", "\033[K", "\033[V", "\033[U", "\033[@" };
    static const char *const vt[]  = { "\033[1~", "\033[4~", "\033[5~", "\033[6~", "\033[2~" };
    const char *s;

    if (id >= KEY_UP && id <= KEY_LEFT)
    {
        out[0] = '\033';
        out[1] = cursorKeyMode ? 'O' : '[';
        out[2] = "ABCD"[id - KEY_UP];
        return 3;
    }
    if (id < KEY_HOME || id > KEY_INSERT)
        return 0;
    s = (vtKeys ? vt : bbs)[id - KEY_HOME];
    memcpy(out, s, strlen(s));
    return strlen(s);
}
