/*
 * readtext_probe -- checks ibmcon.device's screen buffer (1.11) on a real
 * or emulated Amiga: every drawing path must leave in the buffer what it
 * drew. Opens the device given as argument (default DEVS:ibmcon.device)
 * on a Workbench window, sends each case, reads the rows back with
 * IBMCMD_READTEXT and prints PASS/FAIL per case (also to the file given
 * as second argument).
 *
 * Build: vc +aos68k -O1 -o readtext_probe readtext_probe.c -lamiga
 */
#include <exec/types.h>
#include <exec/io.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>

#define IBMCMD_READTEXT 0x7FE3

struct IntuitionBase *IntuitionBase;
static struct IOStdReq *io;
static BPTR logfh;
static int failures;

static void say(const char *text)
{
    PutStr((STRPTR)text);
    if (logfh) FPuts(logfh, (STRPTR)text);
}

static void out(const char *s)
{
    io->io_Command = CMD_WRITE;
    io->io_Data = (APTR)s;
    io->io_Length = strlen(s);
    DoIO((struct IORequest *)io);
}

/* Row (1-based) as text, trailing blanks cut; attrs of column 1 in *cell. */
static int row(int r, char *text, UBYTE cell[4])
{
    static UBYTE buf[4 * 200];
    int n, i;

    io->io_Command = IBMCMD_READTEXT;
    io->io_Data = buf;
    io->io_Length = sizeof(buf);
    io->io_Offset = r;
    DoIO((struct IORequest *)io);
    if (io->io_Error || io->io_Actual == 0) return -1;     /* no such row */
    n = (int)io->io_Actual / 4;
    for (i = 0; i < n; i++) text[i] = (char)buf[i * 4];
    while (n > 0 && text[n - 1] == ' ') n--;
    text[n] = 0;
    if (cell) memcpy(cell, buf, 4);
    return n;
}

static void expect(const char *name, int r, const char *want)
{
    char got[256], line[400];

    if (row(r, got, NULL) < 0)
    {
        sprintf(line, "FAIL %s: READTEXT error %d\n", name, io->io_Error);
        failures++;
    }
    else if (strcmp(got, want) != 0)
    {
        sprintf(line, "FAIL %s: row %d is \"%s\", want \"%s\"\n", name, r, got, want);
        failures++;
    }
    else
        sprintf(line, "PASS %s\n", name);
    say(line);
}

int main(int argc, char **argv)
{
    struct MsgPort *port;
    struct Window *win;
    const char *dev = argc > 1 ? argv[1] : "DEVS:ibmcon.device";
    char line[128], text[256];
    UBYTE cell[4];
    int rows;

    if (argc > 2) logfh = Open((STRPTR)argv[2], MODE_NEWFILE);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    win = OpenWindowTags(NULL, WA_Left, 0, WA_Top, 20, WA_InnerWidth, 640, WA_InnerHeight, 200,
                         WA_Title, (ULONG)"readtext_probe", WA_GimmeZeroZero, TRUE,
                         WA_DragBar, TRUE, TAG_DONE);
    port = CreateMsgPort();
    io = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOStdReq));
    if (!win || !io) { say("FAIL setup\n"); return 20; }
    io->io_Data = win;
    io->io_Length = sizeof(struct Window);
    if (OpenDevice((STRPTR)dev, 0, (struct IORequest *)io, 0))
    {
        sprintf(line, "FAIL cannot open %s\n", dev); say(line); return 20;
    }

    out("\033[2J\033[HABCDEFGHIJ\033[1;3H\033[4X");
    expect("CSI X erases 4 cells", 1, "AB    GHIJ");
    out("\033[2;1H0123456789\033[2;3H\033[2P");
    expect("CSI P deletes 2 cells", 2, "01456789");
    out("\033[2;3H\033[2@");
    expect("CSI @ inserts 2 cells", 2, "01  456789");
    out("\033[3;1Hline3\033[4;1Hline4\033[3;1H\033[M");
    expect("CSI M: line4 moves up", 3, "line4");
    expect("CSI M: blank below", 4, "");
    out("\033[3;1H\033[L");
    expect("CSI L: blank inserted", 3, "");
    expect("CSI L: line4 moves down", 4, "line4");
    out("\033[5;1Hkeep\033[5;3H\033[K");
    expect("CSI K erases to the end", 5, "ke");
    out("\033[6;1Hstart\033[6;3H\033[1K");
    expect("CSI 1K erases to the cursor", 6, "   rt");
    rows = 0;
    while (row(rows + 1, text, NULL) >= 0 && rows < 200) rows++;
    sprintf(line, "INFO %d rows\n", rows); say(line);
    sprintf(line, "\033[%d;1Hlast\n", rows); out(line);
    expect("LF at the bottom scrolls up", rows - 1, "last");
    out("\033[1;1H\033[1;31;44mX\033[0m");
    row(1, text, cell);
    sprintf(line, "%s attributes of a bold red on blue X: char %c fg %d bg %d flags $%02x\n",
            (cell[0] == 'X' && cell[2] == 4 && (cell[3] & 0x10)) ? "PASS" : "FAIL",
            (int)cell[0], (int)cell[1], (int)cell[2], (int)cell[3]);
    if (!(cell[0] == 'X' && cell[2] == 4 && (cell[3] & 0x10))) failures++;
    say(line);
    out("\033[2J\033[1;1Hrow1\033[2;1Hrow2\033[S");
    expect("CSI S scrolls up", 1, "row2");
    out("\033[T");
    expect("CSI T scrolls down", 2, "row2");
    expect("CSI T: blank on top", 1, "");
    out("\033[1;1Htop\033[1;1H\033[A");
    expect("cursor up past the top scrolls down", 2, "top");
    out("\033[2J");
    expect("CSI 2J clears", 3, "");
    sprintf(line, "DONE %d failures\n", failures); say(line);

    CloseDevice((struct IORequest *)io);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    CloseWindow(win);
    if (logfh) Close(logfh);
    CloseLibrary((struct Library *)IntuitionBase);
    return failures ? 5 : 0;
}
