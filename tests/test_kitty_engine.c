/* KG-7: the kitty graphics engine suite (parser, base64 chunks, inflate,
 * PNG, placement, deletion, replies) from vtcon, run against the engine
 * DCTelnet pins (src/third_party/vtcon). The suite and its harness are
 * vtcon's own; this file only runs them, so the pinned engine and the
 * tests that cover it move together. */
#include <stdio.h>

extern int h_failures, h_checks;
void suite_kitty(void);

int main(void)
{
    suite_kitty();
    if (h_failures) {
        printf("kitty_engine: %d of %d checks failed\n", h_failures, h_checks);
        return 1;
    }
    printf("kitty_engine: all %d checks passed\n", h_checks);
    return 0;
}
