/*
 * rtgprobe -- measures what DCTelnet's RTG/256-colour work depends on, on the
 * screen modes of a real machine (plan R2, 2026-09-27-rtg-and-256-colours.md).
 *
 * Pick a screen mode in the ASL requester; the probe opens that screen and
 * appends to PROGDIR:rtgprobe.txt:
 *   - the mode id and the depth as struct BitMap says it and as
 *     GetBitMapAttr(BMA_DEPTH) says it, plus BMF_STANDARD (clear = RTG bitmap)
 *   - what a COMPLEMENT RectFill does to pen A with and without a write mask
 *     of A ^ B (ibmcon's cursor), read back with ReadPixel
 * Run it once per mode to test (AGA 256 colours, RTG 8, 16 and 24 bit).
 *
 * Build: vc +aos68k -O2 -o rtgprobe rtgprobe.c -lamiga
 */
#include <exec/types.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/modeid.h>
#include <intuition/screens.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <graphics/gfxbase.h>
#include <intuition/intuitionbase.h>
#include <stdio.h>
#include <string.h>

struct Library *AslBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

#define PEN_A 20
#define PEN_B 37

static void logf_(BPTR fh, const char *text)
{
    FPuts(fh, (STRPTR)text);
    PutStr((STRPTR)text);
}

int main(void)
{
    struct ScreenModeRequester *req;
    struct Screen *scr;
    struct RastPort *rp;
    char line[160];
    BPTR fh;
    ULONG id, depth;
    LONG plain, masked, back;

    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
        { PutStr("needs graphics.library 39\n"); return 20; }
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
        { PutStr("needs intuition.library 39\n"); return 20; }
    if (!(AslBase = OpenLibrary("asl.library", 38))) { PutStr("needs asl.library 38\n"); return 20; }

    req = (struct ScreenModeRequester *)AllocAslRequestTags(ASL_ScreenModeRequest,
            ASLSM_TitleText, (ULONG)"rtgprobe: pick a screen mode to measure",
            ASLSM_DoDepth, TRUE, ASLSM_MinDepth, 6, ASLSM_MaxDepth, 32,
            ASLSM_PropertyMask, 0, ASLSM_PropertyFlags, 0,
            TAG_DONE);
    if (!req || !AslRequestTags(req, TAG_DONE)) { PutStr("cancelled\n"); return 5; }
    id = req->sm_DisplayID;
    depth = req->sm_DisplayDepth;

    scr = OpenScreenTags(NULL, SA_DisplayID, id, SA_Depth, depth,
                         SA_Width, req->sm_DisplayWidth, SA_Height, req->sm_DisplayHeight,
                         SA_Title, (ULONG)"rtgprobe", SA_ShowTitle, TRUE, TAG_DONE);
    FreeAslRequest(req);
    fh = Open((STRPTR)"PROGDIR:rtgprobe.txt", MODE_READWRITE);
    if (!fh) { if (scr) CloseScreen(scr); PutStr("cannot write PROGDIR:rtgprobe.txt\n"); return 20; }
    Seek(fh, 0, OFFSET_END);

    if (!scr)
    {
        sprintf(line, "mode $%08lx depth %lu: OpenScreen FAILED\n", id, depth);
        logf_(fh, line);
        Close(fh);
        return 10;
    }
    rp = &scr->RastPort;

    sprintf(line, "mode $%08lx requested depth %lu: bm_Depth %lu, BMA_DEPTH %lu, BMA_FLAGS $%lx (%s)\n",
            id, depth, (ULONG)rp->BitMap->Depth,
            GetBitMapAttr(rp->BitMap, BMA_DEPTH), GetBitMapAttr(rp->BitMap, BMA_FLAGS),
            (GetBitMapAttr(rp->BitMap, BMA_FLAGS) & BMF_STANDARD) ? "planar" : "non-standard/RTG");
    logf_(fh, line);

    /* The cursor: COMPLEMENT over pen A, without and with a write mask A^B. */
    SetRGB32(&scr->ViewPort, PEN_A, 0xFFFFFFFF, 0, 0);
    SetRGB32(&scr->ViewPort, PEN_B, 0, 0xFFFFFFFF, 0);
    SetDrMd(rp, JAM2);
    SetAPen(rp, PEN_A);
    RectFill(rp, 20, 40, 60, 60);
    SetDrMd(rp, COMPLEMENT);
    RectFill(rp, 20, 40, 60, 60);
    plain = ReadPixel(rp, 30, 50);
    RectFill(rp, 20, 40, 60, 60);                      /* back to A */
    SetWriteMask(rp, PEN_A ^ PEN_B);
    RectFill(rp, 20, 40, 60, 60);
    masked = ReadPixel(rp, 30, 50);
    RectFill(rp, 20, 40, 60, 60);
    back = ReadPixel(rp, 30, 50);
    SetWriteMask(rp, 0xFF);
    SetDrMd(rp, JAM2);

    sprintf(line, "  COMPLEMENT on pen %d: plain -> %ld, mask %d^%d -> %ld (want %d), again -> %ld (want %d)\n",
            PEN_A, plain, PEN_A, PEN_B, masked, PEN_B, back, PEN_A);
    logf_(fh, line);

    Close(fh);
    Delay(100);                                        /* leave the screen up for a moment */
    CloseScreen(scr);
    CloseLibrary(AslBase);
    CloseLibrary((struct Library *)IntuitionBase);
    CloseLibrary((struct Library *)GfxBase);
    return 0;
}
