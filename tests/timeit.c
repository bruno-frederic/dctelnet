/**
 * @file test-speed.c
 * @brief Measure the total duration of DCTelnet --test-speed
 *
 * First build and install a DCTelnet release binary:
 *  make clean ci install
 *  - Built with VBCC
 * vc +aos68k -O3 -speed -o ../build/vbcc-68000-release/test-speed test-speed.c
 * cp ../build/vbcc-68000-release/test-speed ../build/installed/DCTelnet
 * cp test-speed.ans ../build/installed/DCTelnet/speedtest.ans
 *
 * On Amiga:
 * CD COD:DCTelnet/build/installed/DCTelnet
 * test-speed
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

struct IntuitionBase *IntuitionBase;

int main(void)
{
    CONST_STRPTR cmd = "DCTelnet.68000 --test-speed";
    ULONG before_s, before_micros;
    ULONG after_s, after_micros;
    ULONG elapsed_tenths;

    IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 36);
    if (IntuitionBase == NULL)  return RETURN_FAIL;

    CurrentTime(&before_s, &before_micros);

    Execute(cmd, (BPTR) 0, (BPTR) 0);

    CurrentTime(&after_s, &after_micros);

    // Convert the elapsed time to tenths of a second, rounding to the nearest tenth using integer
    // arithmetic.
    // CurrentTime() updates the time at most 60 times per second, so the sub-second value has a
    // maximum resolution of about 0.01667 s, which is sufficient for a tenth-of-a-second
    // measurement.
    if (after_micros >= before_micros)
    {
        elapsed_tenths = (after_s - before_s) * 10
                       + (after_micros - before_micros + 50000) / 100000;
    }
    else
    {
        elapsed_tenths = (after_s - before_s - 1) * 10
                       + (1000000 - before_micros + after_micros + 50000) / 100000;
    }

    Printf("{\"total_duration_seconds\":%ld.%ld}\r\n", elapsed_tenths / 10, elapsed_tenths % 10);

    if (IntuitionBase)  CloseLibrary((struct Library *) IntuitionBase);

    return RETURN_OK;
}
