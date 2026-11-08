/* tests/test_mainstack.c -- the main task needs a big stack (kitty PNG decode in VtWrite).
 *
 * Run from tests/. Checks that
 *  - DCT_MAIN_STACK is at least 32768 and at least twice the worst stack use
 *    measured on rig 3 (5088 bytes, see src/mainstack.h),
 *  - src/DCTelnet.c defines the global `__stack` that vbcc's startup honours,
 *    with exactly DCT_MAIN_STACK,
 *  - both shipped program icons carry do_StackSize = DCT_MAIN_STACK (Workbench
 *    starts the program with the icon's value; the startup raises it anyway).
 * On 31023d2 (no __stack, icons 4096) every check fails.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mainstack.h"

#define MEASURED_WORST 5088L
#define DO_STACKSIZE_OFFSET 74   /* DiskObject.do_StackSize, big endian ULONG */

static int fails = 0;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok)
        fails++;
}

static char *slurp(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    char *b;

    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    rewind(f);
    b = malloc((size_t)*len + 1);
    if (b && fread(b, 1, (size_t)*len, f) != (size_t)*len) {
        free(b);
        b = NULL;
    }
    fclose(f);
    if (b)
        b[*len] = 0;
    return b;
}

static void icon(const char *path)
{
    long len = 0;
    unsigned char *d = (unsigned char *)slurp(path, &len);
    unsigned long v = 0;
    char msg[160];

    if (d && len > DO_STACKSIZE_OFFSET + 4)
        v = ((unsigned long)d[74] << 24) | ((unsigned long)d[75] << 16) | ((unsigned long)d[76] << 8) | d[77];
    snprintf(msg, sizeof msg, "%s do_StackSize %lu == %ld", path, v, (long)DCT_MAIN_STACK);
    check(d && v == (unsigned long)DCT_MAIN_STACK, msg);
    free(d);
}

int main(void)
{
    long len = 0;
    char *src = slurp("../src/DCTelnet.c", &len);
    char *p;
    int declared = 0;

    check(DCT_MAIN_STACK >= 32768L, "DCT_MAIN_STACK >= 32768");
    check(DCT_MAIN_STACK >= 2 * MEASURED_WORST, "DCT_MAIN_STACK >= 2 x measured worst case");
    if (src) {
        for (p = strstr(src, "__stack"); p; p = strstr(p + 1, "__stack")) {
            if ((p == src || p[-1] == '\n' || p[-1] == ' ') && !strncmp(p, "__stack = DCT_MAIN_STACK;", 25)
                && p >= src + 5 && !strncmp(p - 5, "long ", 5))
                declared = 1;
        }
    }
    check(declared, "src/DCTelnet.c declares: long __stack = DCT_MAIN_STACK;");
    free(src);
    icon("../build/package/DCTelnet/DCTelnet.info");
    icon("../build/package/DCTelnet/DCTelnet.020.info");
    return fails ? 1 : 0;
}
