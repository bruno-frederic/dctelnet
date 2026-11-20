/* tests/test_petscii_font.c -- the C64 fonts DCTelnet builds from its
 * embedded glyph data (src/petscii_font.c) are the shipped Petscii fonts.
 *
 * A PETSCII session without Petscii.font on disk showed CP437 lookalikes
 * instead of the C64 glyphs. DCTelnet now carries the glyphs itself
 * (src/petscii_glyphs.c, made from assets/glyphs by tools/gen_petscii_glyphs.sh) and
 * builds the fonts in memory. This test holds that build to the disk fonts
 * tools/gen_petscii_font.py makes (build/package/DCTelnet/Fonts): the strike
 * bitmap and the CharLoc table in memory must be the bytes in the font file. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "petscii_font.h"

static int failures = 0;

#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
                                              printf(__VA_ARGS__); printf("\n"); } } while (0)

static unsigned char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long n;

    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc((size_t)n);
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n)
    {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    *len = (size_t)n;
    return buf;
}

static int contains(const unsigned char *hay, size_t haylen, const unsigned char *needle, size_t len)
{
    size_t i;

    for (i = 0; i + len <= haylen; i++)
        if (memcmp(hay + i, needle, len) == 0)
            return 1;
    return 0;
}

/* The embedded glyphs are the bytes of the asset the disk fonts come from. */
static void test_embedded_glyphs_are_the_assets(const char *asset, const unsigned char *embedded, size_t embeddedLen)
{
    size_t len;
    unsigned char *blob = read_file(asset, &len);

    CHECK(blob != NULL, "cannot read %s", asset);
    if (!blob)
        return;
    CHECK(len == embeddedLen && memcmp(blob, embedded, len) == 0, "%s: embedded glyphs differ from the asset", asset);
    free(blob);
}

/* A PETSCII session without the font files draws the C64 glyphs: the font
 * built in memory carries the strike bitmap and CharLoc of the shipped font. */
static void test_font_built_in_memory_is_the_shipped_font(const char *fontfile, const unsigned char *glyphs)
{
    static UBYTE chardata[PETSCII_FONT_DATA_SIZE];
    ULONG charloc[PETSCII_FONT_LOC_COUNT];
    unsigned char locBytes[PETSCII_FONT_LOC_COUNT * 4];
    size_t len, i;
    unsigned char *file = read_file(fontfile, &len);

    CHECK(file != NULL, "cannot read %s", fontfile);
    if (!file)
        return;
    petscii_font_strike(glyphs, chardata);
    petscii_font_charloc(charloc);
    for (i = 0; i < PETSCII_FONT_LOC_COUNT; i++)    /* the font file is big-endian */
    {
        locBytes[i * 4]     = (unsigned char)(charloc[i] >> 24);
        locBytes[i * 4 + 1] = (unsigned char)(charloc[i] >> 16);
        locBytes[i * 4 + 2] = (unsigned char)(charloc[i] >> 8);
        locBytes[i * 4 + 3] = (unsigned char)charloc[i];
    }
    CHECK(contains(file, len, chardata, sizeof chardata), "%s: strike bitmap built in memory is not the font's", fontfile);
    CHECK(contains(file, len, locBytes, sizeof locBytes), "%s: CharLoc built in memory is not the font's", fontfile);
    free(file);
}

static void test_double_row(void)
{
    CHECK(petscii_font_double_row(0x00) == 0x0000, "0x00 -> %04x", petscii_font_double_row(0x00));
    CHECK(petscii_font_double_row(0xFF) == 0xFFFF, "0xFF -> %04x", petscii_font_double_row(0xFF));
    CHECK(petscii_font_double_row(0x80) == 0xC000, "0x80 -> %04x", petscii_font_double_row(0x80));
    CHECK(petscii_font_double_row(0x01) == 0x0003, "0x01 -> %04x", petscii_font_double_row(0x01));
    CHECK(petscii_font_double_row(0x5A) == 0x33CC, "0x5A -> %04x", petscii_font_double_row(0x5A));
}

int main(void)
{
    test_double_row();
    test_embedded_glyphs_are_the_assets("../assets/glyphs/c64_upper_8x8.bin", petscii_glyphs_upper, PETSCII_FONT_BLOB_SIZE);
    test_embedded_glyphs_are_the_assets("../assets/glyphs/c64_lower_8x8.bin", petscii_glyphs_lower, PETSCII_FONT_BLOB_SIZE);
    test_font_built_in_memory_is_the_shipped_font("../build/package/DCTelnet/Fonts/Petscii/8", petscii_glyphs_upper);
    test_font_built_in_memory_is_the_shipped_font("../build/package/DCTelnet/Fonts/PetsciiLower/8", petscii_glyphs_lower);
    if (failures)
    {
        printf("test_petscii_font: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_petscii_font: all passed\n");
    return 0;
}
