#ifdef _DEBUG
// Helpers for debugging purpose:

UBYTE socketLibSigBit = 0;

void Pause(void)
{
    char c;

    static const char msg[] = "Press Return to continue...\n";
    Write(Output(), msg, sizeof(msg)/sizeof(msg[0]));

    while (Read(Input(), &c, 1) > 0 && c != '\n')
    {
        // looping empties the buffer before getting back to shell
    }
}

// Wrapper around VPrintf() to simplify printing a single byte. This function hides the complexity
// of VPrintf() and protects against VPrintf's sensitivity to types and format specifiers.
void LogByte(unsigned char b)
{
    static LONG array[1] = { 0 };

    array[0] = b;
    VPrintf(" %ld", array);
}

void LocalPrintByte(unsigned char b)
{
    LONG l = b;
    static char s[5] = { '\0' }; // 1 space, 3 digits, '\0'
    mysprintf(s, " %ld", l);
    LocalPrint(s);
}


void LogWindowsSigBit(void)
{
    ULONG array[1];

    array[0] = dontUseSig31;
    VPrintf("                      dontUseSig31 = %lu\n", array);

    if (SocketBase)
    {
        array[0] = socketLibSigBit;
        if (array[0] >= 31)
        {
            PutStr("ERROR: anormal mp_SigBit!!!\n");
        }
        if (! (mainTask->tc_SigAlloc & (1L << array[0])))
        {
            PutStr("ERROR: mp_SigBit disapeared from mainTask->tc_SigAlloc!!!\n");
        }
        VPrintf("                  socketLib_SigBit = %lu\n", array);
    }

    if (win)
    {
        array[0] = win->UserPort->mp_SigBit;    // Promotes UBYTE to ULONG
        if (array[0] >= 31)
        {
            PutStr("ERROR: anormal mp_SigBit!!!\n");
        }
        if (! (mainTask->tc_SigAlloc & (1L << array[0])))
        {
            PutStr("ERROR: mp_SigBit disapeared from mainTask->tc_SigAlloc!!!\n");
        }
        VPrintf("          win->UserPort->mp_SigBit = %lu\n", array);
    }

    if (scrollbackWin)
    {
        array[0] = scrollbackWin->UserPort->mp_SigBit;
        if (array[0] >= 31)
        {
            PutStr("ERROR: anormal mp_SigBit!!!\n");
        }
        if (! (mainTask->tc_SigAlloc & (1L << array[0])))
        {
            PutStr("ERROR: mp_SigBit disapeared from mainTask->tc_SigAlloc!!!\n");
        }
        VPrintf("scrollbackWin->UserPort->mp_SigBit = %lu\n", array);
    }

    if (packetWin)
    {
        array[0] = packetWin->UserPort->mp_SigBit;
        if (array[0] >= 31)
        {
            PutStr("ERROR: anormal mp_SigBit!!!\n");
        }
        if (! (mainTask->tc_SigAlloc & (1L << array[0])))
        {
            PutStr("ERROR: mp_SigBit disapeared from mainTask->tc_SigAlloc!!!\n");
        }
        VPrintf("         packetWin->UserPort->mp_SigBit = %lu\n", array);
    }

    if (toolBarWin)
    {
        array[0] = toolBarWin->UserPort->mp_SigBit;
        if (array[0] >= 31)
        {
            PutStr("ERROR: anormal mp_SigBit!!!\n");
        }
        if (! (mainTask->tc_SigAlloc & (1L << array[0])))
        {
            PutStr("ERROR: mp_SigBit disapeared from mainTask->tc_SigAlloc!!!\n");
        }
        VPrintf("   toolBarWin->UserPort->mp_SigBit = %lu\n", array);
    }

    if (writeConPort)
    {
        array[0] = writeConPort->mp_SigBit;
        if (array[0] >= 31)
        {
            PutStr("ERROR: anormal mp_SigBit!!!\n");
        }
        if (! (mainTask->tc_SigAlloc & (1L << array[0])))
        {
            PutStr("ERROR: mp_SigBit disapeared from mainTask->tc_SigAlloc!!!\n");
        }
        VPrintf("           writeConPort->mp_SigBit = %lu\n", array);
    }

    if (iconPort)
    {
        array[0] = iconPort->mp_SigBit;
        if (array[0] >= 31)
        {
            PutStr("ERROR: anormal mp_SigBit!!!\n");
        }
        if (! (mainTask->tc_SigAlloc & (1L << array[0])))
        {
            PutStr("ERROR: mp_SigBit disapeared from mainTask->tc_SigAlloc!!!\n");
        }
        VPrintf("           iconPort->mp_SigBit = %lu\n", array);
    }
}


UBYTE BitPosition(ULONG x)
{
    UBYTE pos = 0;

    while (x >>= 1)
        pos++;

    return pos;
}

#ifdef __SASC
typedef unsigned char uint8_t;
typedef unsigned long uint32_t;
#endif

// Prints a 32-bit unsigned value in hexadecimal and binary.
// Uses VPrintf(), therefore requires Kickstart 2.0 or newer.
void PrintBitsULONG(uint32_t val)
{
    char bin[36];   // 32 bits + 3 spaces + '\0'
    char hex[12];   // "FF FF FF FF" + '\0'
    APTR argArray[2];
    int bit, nByte;

    int bp = 0, hp = 0;

    for (nByte = 3; nByte >= 0; nByte--)
    {
        uint8_t b = (val >> (nByte * 8)) & 0xFF;

        // hex
        hex[hp++] = "0123456789ABCDEF"[b >> 4];
        hex[hp++] = "0123456789ABCDEF"[b & 0x0F];
        if (nByte) hex[hp++] = ' ';

        // binary
        for (bit = 7; bit >= 0; --bit)
            bin[bp++] = (b & (1 << bit)) ? '1' : '0';

        if (nByte) bin[bp++] = ' ';
    }

    hex[hp] = '\0';
    bin[bp] = '\0';

    argArray[0] = hex;
    argArray[1] = bin;
    VPrintf("0x%s = %s\n", argArray);
}

void LogWaitSelectResult(LONG l, ULONG sigmask)
{
    PrintBitsULONG(sigmask);

    if (win)
        if (sigmask & (1L << win->UserPort->mp_SigBit))
            PutStr("          win signal bit set\n");

    if (scrollbackWin)
        if (sigmask & (1L << scrollbackWin->UserPort->mp_SigBit))
            PutStr("scrollbackWin signal bit set\n");

    if (packetWin)
        if (sigmask & (1L << packetWin->UserPort->mp_SigBit))
            PutStr("    packetWin signal bit set\n");

    if (toolBarWin)
        if (sigmask & (1L << toolBarWin->UserPort->mp_SigBit))
            PutStr("   toolBarWin signal bit set\n");
}
#endif
