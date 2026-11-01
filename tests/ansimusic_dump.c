/* tests/ansimusic_dump.c -- runs AnsiMusic_Filter over each input line (a
 * hex string) and prints what comes out, for check_ansimusic_vectors.py.
 * Usage: ansimusic_dump [-1] < lines; -1 feeds one byte per call, as if
 * every byte came in its own network read. Output per line:
 *   <text hex> <hz>:<ms> <hz>:<ms> ...   (text hex "-" when empty) */
#include <stdio.h>
#include <string.h>
#include "ansimusic.h"

static UBYTE in[8192], out[8192 + ANSIMUSIC_HELD], text[16384];
static struct Note notes[512], all[4096];
static char line[16384 + 2];

static int hexval(char c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}

int main(int argc, char **argv)
{
    int bytewise = argc > 1 && strcmp(argv[1], "-1") == 0;

    while (fgets(line, sizeof(line), stdin))
    {
        struct AnsiMusic m;
        size_t len = 0, t = 0, i, step, k;
        int count = 0, n, j;

        for (i = 0; hexval(line[i]) >= 0 && hexval(line[i + 1]) >= 0; i += 2)
            in[len++] = (UBYTE)(hexval(line[i]) * 16 + hexval(line[i + 1]));
        AnsiMusic_Init(&m);
        step = bytewise ? 1 : len;
        for (i = 0; i < len; i += step)
        {
            size_t o = AnsiMusic_Filter(&m, in + i, step, out, notes, 512, &n);

            for (k = 0; k < o; k++) text[t++] = out[k];
            for (j = 0; j < n; j++) all[count++] = notes[j];
        }
        if (t == 0) printf("-");
        for (k = 0; k < t; k++) printf("%02x", text[k]);
        for (j = 0; j < count; j++) printf(" %u:%u", all[j].hz, all[j].ms);
        printf("\n");
    }
    return 0;
}
