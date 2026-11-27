/**
 * @file timeit.c
 * @brief Execute a command line and print its duration in milliseconds.
 *
 * Use ReadEClock() as "This is the most precise time measure available through the timer device."
 * see Amiga ROM Kernal Reference Manual - Devices, 3rd edition
 * https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node00C0.html
 *
 *
 * To measure DCTelnet's startup, rendering and shutdown duration:
 *
 * First build and install a DCTelnet release binary.
 *
 * Build TimeIt from tests/ folder:
    docker run --rm --volume ".:/opt/code" --workdir /opt/code "walkero/docker4amigavbcc:m68k-amigaos-2.0.0" \
        vc +aos68k -O3 -speed -I'$VBCC/targets/m68k-amigaos/include' -I'$NDK32_INC' -o TimeIt  timeit.c
 *
 * Install :
    cp -p TimeIt          ../build/installed/DCTelnet
    cp -p test-speed.ans  ../build/installed/DCTelnet/
 *
 * On Amiga, from DCTelnet installed folder:
    TimeIt DCTelnet --test-speed
 *
 *
 * @author Bruno FREDERIC
 * @date 2026
 */


#include <proto/exec.h>         // OpenDevice(), CloseDevice()
#include <proto/dos.h>          // Delay(), TICKS_PER_SECOND, Printf(), PutStr()
#include <proto/timer.h>        // ReadEClock(), struct EClockVal, TIMERNAME
#include <assert.h>
#include <string.h>             // strlen(), memcpy()

#define CMD_MAX_LEN 512

// For timer.device:
struct Device *TimerBase = NULL;
LONG GetElapsedEClockMillis(struct EClockVal* startTime, struct EClockVal* endTime, ULONG freq);

int main(int argc, char *argv[])
{
    char cmd[CMD_MAX_LEN];
    size_t cmdLen = 0;

    // argc is 0 when started from Workbench
    if (argc < 2)
    {
        PutStr("Usage: TimeIt <command> [arguments]\n");
        return RETURN_ERROR;
    }

    // Join all arguments with spaces to rebuild the command line
    for (int i = 1; i < argc; i++)
    {
        size_t argLen = strlen(argv[i]);
        if (cmdLen + argLen + 2 > CMD_MAX_LEN)
        {
            PutStr("Command line too long!\n");
            return RETURN_ERROR;
        }
        if (i > 1)
            cmd[cmdLen++] = ' ';
        memcpy(cmd + cmdLen, argv[i], argLen);
        cmdLen += argLen;
    }
    cmd[cmdLen] = '\0';

    /* No Message Port is needed when only calling timer.device functions,
       A zero-initialized struct timerequest is sufficient.
       See Amiga ROM Kernal Reference Manual - Devices, 3rd edition
       https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node00C1.html
     */
    struct timerequest timerIO = {0};

    /* The Amiga ROM Kernal Reference Manual - Devices, 3rd edition states:
       "These functions still require the device to be opened with the proper timer device unit"
       https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node00BF.html
     */
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, (struct IORequest *)&timerIO, 0) == RETURN_OK)
    {
        // Set up base pointer required for timer functions:
        TimerBase = timerIO.tr_node.io_Device;
    }
    else
    {
        PutStr("Can not open device!\n");
        return RETURN_FAIL;
    }

    struct EClockVal startTime, endTime;

    ReadEClock(&startTime);
    Execute(cmd, (BPTR) 0, (BPTR) 0);
    ULONG freq = ReadEClock(&endTime);

    LONG elapsedMs = GetElapsedEClockMillis(&startTime, &endTime, freq);

    Printf(", \"cmd_duration_ms\":%ld", elapsedMs);


    if (TimerBase != NULL)  CloseDevice((struct IORequest *) &timerIO);

    return RETURN_OK;
}

/**
 * @brief Calculate the elapsed time between two E-clock readings.
 *
 * The readings and frequency are obtained from ReadEClock().
 * The end reading must not precede the start reading.
 *
 * @param startTime Non-NULL starting E-clock reading.
 * @param endTime   Non-NULL   ending E-clock reading.
 * @param freq      E-Clock frequency in ticks per second.
 *
 * @return Elapsed milliseconds,
 *         or -1 if freq is zero or the calculated result exceeds 2,147,483,647 ms (~25 days).
 */
LONG GetElapsedEClockMillis(struct EClockVal* startTime, struct EClockVal* endTime, ULONG freq)
{
    if (freq == 0)
    {
        #ifdef _DEBUG
            assert(FALSE);
        #endif
        return -1;
    }

    unsigned long long start = ((unsigned long long)startTime->ev_hi << 32) | startTime->ev_lo;
    unsigned long long end   = ((unsigned long long)  endTime->ev_hi << 32) | endTime->ev_lo;

    assert(start < end);

    unsigned long long elapsedTicks = end - start;
    ULONG elapsedMs = (ULONG)((elapsedTicks * 1000) / freq);

    if (elapsedMs > LONG_MAX)
        return -1;

    return (LONG)elapsedMs;
}