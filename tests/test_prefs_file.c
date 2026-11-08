/* tests/test_prefs_file.c -- DCTelnet.Prefs files of every version. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "prefs_file.h"
#include "palette.h"

static UBYTE file[2048];

static size_t v2_file(const struct PrefsStruct *p, UWORD dataSize)
{
    struct DCTFileHeader hdr = { { 'D', 'C', 'T', 'P' }, 2, 0 };
    hdr.dataSize = dataSize;
    memcpy(file, &hdr, sizeof(hdr));
    memcpy(file + sizeof(hdr), p, dataSize <= sizeof(*p) ? dataSize : sizeof(*p));
    if (dataSize > sizeof(*p)) memset(file + sizeof(hdr) + sizeof(*p), 0x5A, dataSize - sizeof(*p));
    return sizeof(hdr) + dataSize;
}

static void test_a_v2_file_round_trips(void)
{
    struct PrefsStruct p, got;
    memset(&p, 0x11, sizeof(p));
    assert(Prefs_Decode(file, v2_file(&p, sizeof(p)), &got) == PREFS_FILE_V2);
    assert(memcmp(&p, &got, sizeof(p)) == 0);
}

/* A file from a 2.x without the newest fields loads quietly, the missing
 * ones 0 (defaulted later); it was "inconsistent data" + "truncated data". */
static void test_an_older_v2_file_loads_with_the_new_fields_zero(void)
{
    struct PrefsStruct p, got;
    UWORD older = (UWORD)offsetof(struct PrefsStruct, XferOptions);
    memset(&p, 0x11, sizeof(p));
    assert(Prefs_Decode(file, v2_file(&p, older), &got) == PREFS_FILE_V2);
    assert(memcmp(&p, &got, older) == 0);
    assert(got.XferOptions[0] == 0 && got.XferOptions[51] == 0);
}

/* A file from a newer 2.x: the fields this build knows, the rest ignored. */
static void test_a_newer_v2_file_loads_the_fields_it_knows(void)
{
    struct PrefsStruct p, got;
    memset(&p, 0x22, sizeof(p));
    assert(Prefs_Decode(file, v2_file(&p, (UWORD)(sizeof(p) + 40)), &got) == PREFS_FILE_V2);
    assert(memcmp(&p, &got, sizeof(p)) == 0);
}

static void test_damaged_files_are_refused(void)
{
    struct PrefsStruct p, got;
    struct DCTFileHeader hdr = { { 'D', 'C', 'T', 'P' }, 3, 10 };
    memset(&p, 0, sizeof(p));
    assert(Prefs_Decode(file, 5, &got) == PREFS_FILE_BAD);
    memcpy(file, &hdr, sizeof(hdr));
    assert(Prefs_Decode(file, sizeof(hdr) + 10, &got) == PREFS_FILE_BAD);   /* version 3 */
    memcpy(file, "XXXX", 4);
    assert(Prefs_Decode(file, 100, &got) == PREFS_FILE_BAD);                /* legacy, too short */
}

/* A DCTelnet 1.x file (big-endian, no header). */
static void put16(size_t at, unsigned v) { file[at] = (UBYTE)(v >> 8); file[at + 1] = (UBYTE)v; }
static void put32(size_t at, unsigned long v) { put16(at, (unsigned)(v >> 16)); put16(at + 2, (unsigned)v); }

static size_t legacy_file(unsigned long flags)
{
    memset(file, 0, 376);
    put32(0, 0x29004UL); put16(4, 640); put16(6, 256); put16(8, 4); put16(10, 8);
    strcpy((char *)file + 12, "IBM.font");
    strcpy((char *)file + 44, "Work:Downloads");
    strcpy((char *)file + 96, "xprzmodem.library");
    strcpy((char *)file + 148, "TN");
    put16(200, 0x000); put16(202, 0xFFF); put16(214, 0xF00);
    put32(232, flags);
    put16(236, 10); put16(238, 20); put16(240, 600); put16(242, 300);
    put16(244, 5); put16(246, 15); put16(248, 500); put16(250, 150);
    strcpy((char *)file + 252, "Work:Uploads");
    strcpy((char *)file + 304, "xemvt340.library");
    put32(336, 1000);
    strcpy((char *)file + 340, "ANSI");
    put16(372, 7); put16(374, 30);
    return 376;
}

/* An old DCTelnet.Prefs kept only the screen mode, font and palette: every
 * option, path, window and the terminal type went back to the defaults. */
static void test_an_old_prefs_file_keeps_all_its_settings(void)
{
    struct PrefsStruct got;
    size_t n = legacy_file((1UL << 3) | (1UL << 4) | (1UL << 10) | (1UL << 12) | (1UL << 15));

    assert(Prefs_Decode(file, n, &got) == PREFS_FILE_LEGACY);
    assert(got.DisplayID == 0x29004UL && got.DisplayWidth == 640 && got.DisplayHeight == 256);
    assert(got.DisplayDepth == 4 && got.FontSize == 8 && !strcmp((char *)got.FontName, "IBM.font"));
    assert(got.DeviceColors[1] == 0xFFF && got.DeviceColors[7] == 0xF00);
    assert(!strcmp((char *)got.DownloadPath, "Work:Downloads") && !strcmp((char *)got.UploadPath, "Work:Uploads"));
    assert(!strcmp((char *)got.XferLibrary, "xprzmodem.library") && !strcmp((char *)got.XferOptions, "TN"));
    assert(!strcmp((char *)got.XemLibrary, "xemvt340.library") && !strcmp((char *)got.TelnetTermType, "ANSI"));
    assert(got.MainWinLeftEdge == 10 && got.MainWinTopEdge == 20 && got.MainWinWidth == 600 && got.MainWinHeight == 300);
    assert(got.ScrollbackWinWidth == 500 && got.ScrollbackWinHeight == 150);
    assert(got.ToolBarWinLeftEdge == 7 && got.ToolBarWinTopEdge == 30);
    assert(got.nScrollbackLines == 1000);
    /* Use Workbench, BS/DEL swap, tool bar, local echo, PETSCII; title bar,
     * LEDs and scrollback were not hidden; the console was ibmcon.device. */
    assert(got.State == (APP_BACKSPACE_DEL_SWAPPED | APP_TOOL_BAR_ENABLED | APP_LOCAL_ECHO | APP_PETSCII_MODE
                         | APP_TITLE_BAR_ENABLED | APP_LEDS_ENABLED | APP_SCROLLBACK_ENABLED
                         | APP_RENDERER_IBMCON_DEVICE));
}

/* The builds before v2.0 appended settings at 444 (a 508-byte struct). */
static void test_the_extension_block_of_the_builds_before_v2_carries(void)
{
    struct PrefsStruct got;
    legacy_file(0);
    memset(file + 376, 0, 508 - 376);
    file[444] = 3; file[445] = 20; file[446] = 5; file[447] = 30; file[448] = 2; file[449] = 1; file[450] = 1;
    assert(Prefs_Decode(file, 508, &got) == PREFS_FILE_LEGACY);
    assert(got.RedialTries == 3 && got.RedialDelay == 20 && got.AntiIdleMinutes == 5 && got.ConnectTimeout == 30);
    assert(got.Charset == 2 && got.Bell == 1 && got.AnsiMusic == 1);   /* UTF-8, Sound, music on */
    assert(Prefs_Decode(file, 444, &got) == PREFS_FILE_LEGACY && got.RedialTries == 0);
}

/* The builds before v2.0 added their own flags after bit 15. */
static void test_the_flags_of_the_builds_before_v2_carry(void)
{
    struct PrefsStruct got;
    size_t n = legacy_file((1UL << 3) | (1UL << 16) | (1UL << 17) | (1UL << 18) | (1UL << 19) | (1UL << 20));  /* + VT keys, Rlogin, 132 columns, SSH */
    assert(Prefs_Decode(file, n, &got) == PREFS_FILE_LEGACY);
    assert(got.State & APP_WINDOW_SNAPSHOT);
    assert(got.State & APP_VT_KEYS);
    assert(got.State & APP_RLOGIN);
    assert(got.State & APP_132_COLUMNS);
    assert(got.State & APP_SSH);
}

static void test_old_hide_options_and_xem_map_to_the_new_ones(void)
{
    struct PrefsStruct got;
    /* hide title bar, hide LEDs, disable scrollback, XEM, raw, jump scroll, fullscreen (no USE_WORKBENCH) */
    size_t n = legacy_file((1UL << 0) | (1UL << 2) | (1UL << 5) | (1UL << 9) | (1UL << 13) | (1UL << 14));
    assert(Prefs_Decode(file, n, &got) == PREFS_FILE_LEGACY);
    assert(got.State == (APP_FULLSCREEN | APP_RENDERER_XEM_LIB | APP_RAW_CONNECTION | APP_FAST_SCROLL_ENABLED));
}

/* A 1.x file shorter than 376 bytes: its screen, font, paths and palette. */
static void test_a_short_old_file_keeps_the_essentials(void)
{
    struct PrefsStruct got;
    legacy_file(0);
    assert(Prefs_Decode(file, 232, &got) == PREFS_FILE_LEGACY);
    assert(got.DisplayWidth == 640 && got.DeviceColors[1] == 0xFFF && got.State == 0);
    assert(got.MainWinWidth == 0);
}

/* The built-in renderer needs 80x25 cells of 8x8; any depth (on a screen
 * other than 4 planes it draws through the RastPort), RTG too. */
static void test_the_built_in_renderer_fits_any_screen_of_80x25_cells(void)
{
    struct PrefsStruct p;

    memset(&p, 0, sizeof(p));
    p.State = APP_RENDERER_BUILTIN;
    p.DisplayWidth = 640; p.DisplayHeight = 256; p.DisplayDepth = 4;
    assert(Prefs_ScreenFits(&p));
    p.DisplayDepth = 8;                         /* AGA 256 colours */
    assert(Prefs_ScreenFits(&p));
    p.DisplayWidth = 800; p.DisplayHeight = 600; p.DisplayDepth = 24;   /* RTG */
    assert(Prefs_ScreenFits(&p));
    p.DisplayWidth = 320; p.DisplayHeight = 256; p.DisplayDepth = 4;    /* 40 columns */
    assert(!Prefs_ScreenFits(&p));
    p.DisplayWidth = 640; p.DisplayHeight = 199;
    assert(!Prefs_ScreenFits(&p));
    p.DisplayHeight = 256; p.DisplayDepth = 3;
    assert(!Prefs_ScreenFits(&p));
    p.State = APP_RENDERER_IBMCON_DEVICE; p.DisplayWidth = 1920; p.DisplayHeight = 1200; p.DisplayDepth = 8;
    assert(!Prefs_ScreenFits(&p));             /* ibmcon.device crashes there */
}

/* A 1.x file has one palette, the console's (ibmcon order): the built-in
 * and XEM renderers' ANSI palette is made from it, not left black -- an
 * entry converted from 1.x drew black text on black with the built-in one. */
static void test_a_1x_file_gives_the_built_in_renderer_its_colours(void)
{
    struct PrefsStruct got;
    int i;

    legacy_file(0);
    assert(Prefs_FromLegacy(file, 376, &got));
    assert(got.DeviceColors[1] == 0xFFF && got.DeviceColors[7] == 0xF00);
    for (i = 0; i < 16; i++)
        assert(got.AnsiColors[i] == got.DeviceColors[Palette_IbmconToAnsi(i)]);
    assert(got.AnsiColors[7] == 0xFFF && got.AnsiColors[1] == 0xF00);   /* white, red: ANSI order */
}

int main(void)
{
    test_a_v2_file_round_trips();
    test_an_older_v2_file_loads_with_the_new_fields_zero();
    test_a_newer_v2_file_loads_the_fields_it_knows();
    test_damaged_files_are_refused();
    test_an_old_prefs_file_keeps_all_its_settings();
    test_old_hide_options_and_xem_map_to_the_new_ones();
    test_the_flags_of_the_builds_before_v2_carry();
    test_the_extension_block_of_the_builds_before_v2_carries();
    test_a_short_old_file_keeps_the_essentials();
    test_the_built_in_renderer_fits_any_screen_of_80x25_cells();
    test_a_1x_file_gives_the_built_in_renderer_its_colours();
    printf("prefs_file: all assertions passed\n");
    return 0;
}
