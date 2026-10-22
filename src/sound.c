/* src/sound.c -- tones through audio.device: the Bell as a sound, ANSI music. */
#ifdef __VBCC__
    #pragma dontwarn 306
#endif
#include <proto/exec.h>
#include <devices/audio.h>
#include <clib/alib_protos.h>          // BeginIO()
#include <exec/memory.h>
#ifdef __VBCC__
    #pragma popwarn
#endif
#include "sound.h"

#define PAULA_CLOCK   3546895UL     /* PAL; NTSC is 1% higher: close enough */
#define WAVE_LEN      16
#define REQUESTS      48            /* notes queued at a time */

static struct MsgPort *port;
static struct IOAudio *master;
static struct IOAudio *req[REQUESTS];
static BOOL   busy[REQUESTS];
static BYTE  *wave;
static BOOL   opened, failed;

static BOOL SoundOpen(void)
{
    static UBYTE channels[] = { 1, 2, 4, 8 };   /* any one channel */
    int i;

    if (opened) return TRUE;
    if (failed) return FALSE;
    failed = TRUE;                              /* until everything is there */
    if (!(wave = AllocMem(WAVE_LEN, MEMF_CHIP))) return FALSE;
    for (i = 0; i < WAVE_LEN; i++) wave[i] = (BYTE)(i < WAVE_LEN / 2 ? 100 : -100);
    if (!(port = CreateMsgPort())) return FALSE;
    if (!(master = (struct IOAudio *)CreateIORequest(port, sizeof(struct IOAudio)))) return FALSE;
    master->ioa_Request.io_Message.mn_Node.ln_Pri = -40;  /* any other program wins */
    master->ioa_Request.io_Flags = ADIOF_NOWAIT;
    master->ioa_Data = channels;
    master->ioa_Length = sizeof(channels);
    for (i = 0; i < REQUESTS; i++)
        if (!(req[i] = (struct IOAudio *)AllocMem(sizeof(struct IOAudio), MEMF_PUBLIC | MEMF_CLEAR)))
            return FALSE;
    if (OpenDevice(AUDIONAME, 0, (struct IORequest *)master, 0))
        return FALSE;                           /* no free channel: no sound */
    opened = TRUE;
    failed = FALSE;
    return TRUE;
}

/* The requests audio.device has finished with are free again. */
static void Reclaim(void)
{
    struct Message *msg;
    int i;

    while ((msg = GetMsg(port)))
        for (i = 0; i < REQUESTS; i++)
            if ((struct Message *)req[i] == msg) busy[i] = FALSE;
}

void Sound_Play(const struct Note *notes, int count)
{
    int n, i;

    if (!SoundOpen()) return;
    Reclaim();
    for (n = 0; n < count; n++)
    {
        UWORD hz = notes[n].hz ? notes[n].hz : 440;     /* a rest: silent cycles */
        ULONG period = PAULA_CLOCK / ((ULONG)hz * WAVE_LEN);
        ULONG cycles = (ULONG)hz * notes[n].ms / 1000;
        struct IOAudio *a;

        for (i = 0; i < REQUESTS && busy[i]; i++) ;
        if (i == REQUESTS) return;                      /* the queue is full: the rest is dropped */
        a = req[i];
        *a = *master;                                   /* device, unit, allocation key */
        a->ioa_Request.io_Message.mn_ReplyPort = port;
        a->ioa_Request.io_Command = CMD_WRITE;
        a->ioa_Request.io_Flags = ADIOF_PERVOL;
        a->ioa_Data = (UBYTE *)wave;
        a->ioa_Length = WAVE_LEN;
        a->ioa_Period = (UWORD)(period < 124 ? 124 : period > 65535 ? 65535 : period);
        a->ioa_Volume = notes[n].hz ? 48 : 0;
        a->ioa_Cycles = (UWORD)(cycles ? (cycles > 65535 ? 65535 : cycles) : 1);
        BeginIO((struct IORequest *)a);
        busy[i] = TRUE;
    }
}

void Sound_Beep(void)
{
    static const struct Note beep = { 880, 150 };

    Sound_Play(&beep, 1);
}

void Sound_Close(void)
{
    int i;

    if (opened)
    {
        for (i = 0; i < REQUESTS; i++)
            if (busy[i])
            {
                AbortIO((struct IORequest *)req[i]);
                WaitIO((struct IORequest *)req[i]);
            }
        CloseDevice((struct IORequest *)master);
    }
    for (i = 0; i < REQUESTS; i++)
        if (req[i]) { FreeMem(req[i], sizeof(struct IOAudio)); req[i] = NULL; }
    if (master) DeleteIORequest((struct IORequest *)master);
    if (port) DeleteMsgPort(port);
    if (wave) FreeMem(wave, WAVE_LEN);
    master = NULL; port = NULL; wave = NULL; opened = FALSE;
}
