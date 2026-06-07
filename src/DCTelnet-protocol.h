
/*
 * @file DCTelnet-protocol.h
 * @brief Telnet protocol (RFC 854) handling and ZModem auto-detection for DCTelnet.
 *
 * The implementation uses:
 *   - an incremental byte-oriented parser state machine
 *   - RFC1143-compatible option negotiation tracking to prevent loop
 *   - subnegotiation buffering
 *   - automatic ZModem transfer sequence detection (small state machine)
 *
 * The Telnet parser operates correctly even when protocol commands are split across multiple TCP
 * recv() calls.
 *
 * Telnet negotiation is started lazily: the client waits until the remote host sends a Telnet
 * command before enabling Telnet protocol handling.
 *
 * Supported Telnet options:
 *   - BINARY  (RFC 856)  Enable 8-bit transparent data transmission
 *   - SGA     (RFC 858)  Suppress GO AHEAD for full-duplex communication
 *   - TTYPE   (RFC 1091) Terminal type negotiation
 *   - NAWS    (RFC 1073) Window size negotiation (client sends terminal dimensions)
 *   - ECHO    (RFC 857)  Server-side character echo negotiation
 *
 * This code is intentionally kept in the same compilation unit as DCTelnet.o to improve compiler
 * optimization opportunities, as byte-oriented parsing involves a large number of function calls.
 *
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

 #include "limits.h" // for UCHAR_MAX

/**
 * @brief Telnet protocol parser states.
 */
typedef enum
{
    TELNET_STATE_DATA = 0,   /* Normal data stream */
    TELNET_STATE_IAC,        /* Got IAC (0xFF), waiting command */
    TELNET_STATE_WILL,       /* Got WILL, waiting option */
    TELNET_STATE_WONT,       /* Got WONT, waiting option */
    TELNET_STATE_DO,         /* Got DO, waiting option */
    TELNET_STATE_DONT,       /* Got DONT, waiting option */
    TELNET_STATE_SB,         /* Subnegotiation */
    TELNET_STATE_SB_DATA,    /* Inside SB data */
    TELNET_STATE_SB_IAC      /* IAC inside SB */
} TelnetProtocolState;


/**
 * @brief RFC1143 Telnet option negotiation states.
 *
 * These states track the negotiation status of a Telnet option for one side of the connection.
 *
 * RFC1143 defines a finite state machine that prevents negotiation loops during Telnet option
 * exchanges.
 *
 * The same state model is used independently for:
 *   - local/client-side options ("us")
 *   - remote/server-side options ("him")
 */
typedef enum
{
    NO      = 0,
    YES     = 1,
    WANTNO  = 2,
    WANTYES = 3
} OptionNegotiationState;


/**
 * @brief RFC1143 negotiation state tracking both sides of one Telnet option.
 *
 * RFC 1143 drescribes the state information that you must keep about each side of each option and
 * describe a telnet state machine in section 7.
 *
 *   - us   : local/client-side state
 *   - him  : remote/server-side state
 *
 * The queue flags (usQ/himQ) are the additional state bits required by RFC1143 to handle
 * simultaneous or conflicting negotiation requests.
 *
 *
 * Bitfields are used to minimize memory usage on Amiga systems.
 */
#ifdef __VBCC__
#pragma dontwarn 51 // warning: bitfield type non-portable
#endif
typedef struct
{
    UBYTE  us   : 2;
    UBYTE  him  : 2;
    UBYTE  usQ  : 1;
    UBYTE  himQ : 1;
} TelnetOptionState;
#ifdef __VBCC__
#pragma popwarn
#endif
// Macros to mimic queue bit value from RFC 1143
#define EMPTY     FALSE   // 0 on Amiga
#define OPPOSITE  TRUE


/**
 * @brief Global Telnet runtime context.
 *
 * Holds all parser state and negotiation state required by the Telnet protocol implementation.
 *
 * A single static instance is used by the Telnet protocol module.
 */
typedef struct
{
    TelnetProtocolState state;

    UBYTE sbOption;         // Last option received
    UBYTE sbBuffer[256];
    UWORD sbLength;
    TelnetOptionState optState[UCHAR_MAX+1];

    // Negotiation detection helpers:
    // - isServerNegotiationSeen: set when server sends WILL/DO (initial sequence started by server)
    // - isNegotiationTriggered: ensure we only trigger once the client-side negotiation
    BOOL isServerNegotiationSeen;
    BOOL isClientNegotiationTriggered;
} TelnetContext;

static TelnetContext telnetCtx = { 0 }; // 520 bytes long


/**
 * @brief Reset the global Telnet parser and negotiation context.
 *
 * Clears the entire Telnet runtime context structure, including:
 *   - protocol parser state
 *   - subnegotiation buffer state
 *   - all RFC1143 negotiation states
 *
 * This function should typically be called:
 *   - before opening a new Telnet session
 *   - after a disconnect
 *   - when recovering from protocol desynchronization
 */
static void ResetTelnetContext(void)
{
    memset(&telnetCtx, 0, sizeof(telnetCtx));
}


/**
 * @brief Send a Telnet option negotiation command.
 *
 * Sends a standard 3-byte Telnet negotiation sequence:
 *
 *   IAC <command> <option>
 *
 * Typical commands are: WILL, WONT, DO, DONT
 *
 * Example:
 *
 *   IAC WILL BINARY
 *
 * This function is used only for Telnet option negotiation, not for subnegotiation blocks (SB/SE).
 *
 * @param cmd
 *        Telnet negotiation command.
 *
 * @param option
 *        Telnet option identifier.
 */
static void TelnetSendOptionCommand(UBYTE cmd, UBYTE option)
{
    UBYTE buf[3];

    #ifdef _DEBUG
        PutStr("›35m»IAC ");
        if   (TELCMD_OK(cmd))     PutStr(TELCMD(cmd));
        else                      LogByte(cmd);
        PutStr(" ");
        if   (TELOPT_OK(option))  PutStr(TELOPT(option));
        else                      LogByte(option);
        PutStr("›m\n");
    #endif

    buf[0] = IAC;
    buf[1] = cmd;
    buf[2] = option;

    TCPSend(buf, sizeof(buf));
}


/**
 * @brief Initiate negotiation to enable a Telnet option on the client side.
 *
 * Sends a WILL negotiation request for the specified Telnet option and updates the local RFC1143
 * negotiation state machine.
 *
 * This function manages the "us" side of the negotiation state, meaning options implemented locally
 * by the client.
 *
 * It also handles queued state transitions as defined by RFC1143 in order to prevent negotiation
 * loops and invalid simultaneous requests.
 *
 * @param option
 *        Telnet option identifier to enable locally.
 */
static void TelnetAskToEnableClientOption(UBYTE option)
{
        switch(telnetCtx.optState[option].us)
    {
        case NO:
            telnetCtx.optState[option].us = WANTYES;
            TelnetSendOptionCommand(WILL, option);
        break;

        case YES:
            /*
                RFC 854, section 3 states:
                "a. Parties may only request a change in option status; i.e., a party may not send
                out a "request" merely to announce what mode it is in.
            */
            #ifdef _DEBUG
                PutStr("\n›34mError: Telnet option already enabled on client!›m\n");
            #endif
        break;

        // The states WANTNO and WANTYES are used when we have sent a WILL/WONT to the server:

        case WANTNO:
            if (telnetCtx.optState[option].usQ == EMPTY)
            {
                #ifdef _DEBUG
                    PutStr(
                      "\n›34mError: Cannot initiate new request in the middle of negotiation!›m\n");
                #endif
            }
            else // OPPOSITE
            {
                #ifdef _DEBUG
                    PutStr("\n›34mError: Already queued an enable request!›m\n");
                #endif
            }
        break;

        case WANTYES:
            if (telnetCtx.optState[option].usQ == EMPTY)
            {
                #ifdef _DEBUG
                    PutStr("\n›34mError: Already negotiating for enable!›m\n");
                #endif
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].usQ = EMPTY;
            }
        break;
    }
}


/**
 * @brief Initiate negotiation to enable a Telnet option on the server side.
 *
 * Sends a DO negotiation request asking the remote Telnet server to enable the specified option and
 * updates the RFC1143 negotiation state machine.
 *
 * This function manages the "him" side of the negotiation state, meaning options implemented
 * remotely by the server.
 *
 * It also handles queued state transitions as defined by RFC1143 in order to prevent negotiation
 * loops and invalid simultaneous requests.
 *
 * @param option
 *        Telnet option identifier requested from the remote server.
 */
static void TelnetAskToEnableServerOption(UBYTE option)
{
    switch(telnetCtx.optState[option].him)
    {
        case NO:
            telnetCtx.optState[option].him = WANTYES;
            TelnetSendOptionCommand(DO, option);
        break;

        case YES:
            #ifdef _DEBUG
                PutStr("\n›34mError: Telnet option already enabled on server!›m\n");
            #endif
        break;

        // The states WANTNO and WANTYES are used when we have sent a DO/DONT to the server:

        case WANTNO:
            if (telnetCtx.optState[option].himQ == EMPTY)
            {
                #ifdef _DEBUG
                    PutStr(
                      "\n›34mError: Cannot initiate new request in the middle of negotiation!›m\n");
                #endif
            }
            else // OPPOSITE
            {
                #ifdef _DEBUG
                    PutStr("\n›34mError: Already queued an enable request!›m\n");
                #endif
            }
        break;

        case WANTYES:
            if (telnetCtx.optState[option].himQ == EMPTY)
            {
                #ifdef _DEBUG
                    PutStr("\n›34mError: Already negotiating for enable!›m\n");
                #endif
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].himQ = EMPTY;
            }
        break;
    }
}


/**
 * @brief Sends the Telnet terminal type (TTYPE) to the remote host.
 *
 * This function sends a Telnet TTYPE subnegotiation sequence as defined in RFC 1091:
 *
 *     IAC SB TELOPT_TTYPE TELQUAL_IS <terminal-type> IAC SE
 *
 * The <terminal-type> field is taken from prefs.displayidstr (for example: "vt100", "xterm", etc.).
 *
 * This function should only be called after the remote host has negotiated the TTYPE option with:
 *
 *     IAC DO TELOPT_TTYPE
 *
 * and requested the terminal type using:
 *
 *     IAC SB TELOPT_TTYPE TELQUAL_SEND IAC SE
 *
 * @note The terminal type string is sent as-is without additional escaping of IAC (0xFF) bytes.
 *       This is acceptable as long as the string does not contain 0xFF.
 */
static void TelnetSendTType(void)
{
    UBYTE start[] = { IAC, SB, TELOPT_TTYPE, TELQUAL_IS };
    UBYTE   end[] = { IAC, SE };

    // Only send terminal type if the option was successfully negotiated with the telnet server
    if (telnetCtx.optState[TELOPT_TTYPE].us != YES)
        return;

    #ifdef _DEBUG
            PutStr("›35m»IAC SB TELOPT_TTYPE TELQUAL_IS ");
            PutStr(prefs.displayidstr);
            PutStr(" IAC SE›m\n");
    #endif

    TCPSend(start, sizeof(start));
    TCPSend(prefs.displayidstr, strlen(prefs.displayidstr));
    TCPSend(end, sizeof(end));
}


/**
 * @brief Send the current terminal window size using the Telnet NAWS option.
 *
 * This function sends a Telnet subnegotiation sequence for TELOPT_NAWS (Negotiate About Window
 * Size, RFC 1073):
 *
 *   IAC SB NAWS <cols-hi> <cols-lo> <rows-hi> <rows-lo> IAC SE
 *
 * Any data byte equal to IAC (0xFF) is escaped as required by the Telnet protocol.
 *
 * The terminal dimensions are retrieved from:
 *   - XEmulator.library when available
 *   - otherwise the current Intuition window/font metrics
 *
 * The maximum generated packet size is 13 bytes:
 *   3-byte header
 *   + 4 data bytes
 *   + optional escaping of all 4 bytes
 *   + 2-byte trailer
 */
static void TelnetSendWindowSize(void)
{
    UBYTE buf[16];
    UBYTE vals[4];
    LONG Columns = 80, Lines = 25;
    int i = 0;
    int j = 0;

    // Only send window size if the option was successfully negotiated with the telnet server
    if (telnetCtx.optState[TELOPT_NAWS].us != YES)
        return;

    if(XEmulatorBase && xemIO && XEmulatorBase->lib_Version >= 4)
    {
        ULONG Result = XEmulatorInfo(xemIO,XEMI_CONSOLE_DIMENSIONS);

        Columns = XEMI_EXTRACT_COLUMNS(Result);
        Lines   = XEMI_EXTRACT_LINES(Result);
    }
    else if (win && win->RPort && win->RPort->Font)
    {
         // Get terminal dimensions using current font metrics when using ibmcon/console device
        Columns = win->Width / win->RPort->Font->tf_XSize;
        Lines   = win->Height / win->RPort->Font->tf_YSize;
    }

    buf[i++] = IAC;     buf[i++] = SB;    buf[i++] = TELOPT_NAWS;

    vals[0] = (Columns >> 8) & 0xFF;    // high byte of Columns
    vals[1] =        Columns & 0xFF;    // low  byte of Columns
    vals[2] = (Lines >> 8) & 0xFF;      // high byte of Lines
    vals[3] =        Lines & 0xFF;      // low  byte of Lines

    for (j = 0; j < 4; j++)
    {
        // Escape IAC bytes as required by Telnet.
        if (vals[j] == IAC)
            buf[i++] = IAC;

        buf[i++] = vals[j];
    }

    buf[i++] = IAC;    buf[i++] = SE;

    TCPSend(buf, i);

    #ifdef _DEBUG
        PutStr("›35m»IAC SB TELOPT_NAWS ");
        for (j = 3; j < i-2; j++)
            LogByte(buf[j]);
        PutStr(" IAC SE›m\n");
    #endif
}


/**
 * @brief Process a completed Telnet subnegotiation sequence.
 *
 * This function is called after reception of a complete:
 *
 *   IAC SB ... IAC SE
 *
 * The received payload is stored in:
 *   - telnetCtx.sbOption
 *   - telnetCtx.sbBuffer
 *   - telnetCtx.sbLength
 *
 * Currently supported subnegotiations:
 *   - TELOPT_TTYPE
 *
 * Unsupported subnegotiations are ignored.
 */
static void TelnetHandleSubnegotiation(void)
{
    #ifdef _DEBUG
        int i;

        PutStr(" ");
        if   (TELOPT_OK(telnetCtx.sbOption))    PutStr(TELOPT(telnetCtx.sbOption));
        else                                    LogByte(telnetCtx.sbOption);

        // First byte is sub-option qualifier, cf. "third_party\netinclude\arpa\telnet.h"
        for (i = 0; i < telnetCtx.sbLength; i++)
            LogByte(telnetCtx.sbBuffer[i]);
    #endif


    switch(telnetCtx.sbOption)
    {
        case TELOPT_TTYPE: /* Terminal type  */
            TelnetSendTType();
        break;

        default:
            #ifdef _DEBUG
                PutStr(" (Unhandled!)");
            #endif
        break;
    }

    #ifdef _DEBUG
        PutStr("›m\n");
    #endif
}


/**
 * @brief Handles the server's WILL announcement for a Telnet option on server-side.
 *
 * This function is called when the server announces that it is willing to enable a Telnet option
 *  (WILL).
 *
 * It replies with DO or DONT depending on whether the option is supported by DCTelnet.
 *
 * @param option
 *        Telnet option code announced by the server.
 */
static void TelnetHandleWill(UBYTE option)
{
    switch(telnetCtx.optState[option].him)
    {
        case NO:
            switch(option)
            {
                // We agree that he sould enable for these options:
                case TELOPT_ECHO:      // RFC 857 : echoing data characters it receives over the TELNET connection back to the sender of the data characters
                case TELOPT_BINARY:    // enable 8-bit data transmission
                case TELOPT_SGA:       // suppress GO AHEAD signals (Full-duplex communication)

                    telnetCtx.optState[option].him = YES;
                    TelnetSendOptionCommand(DO, option);
                break;


                /*  RFC 854 section 3 :
                    "the DON'T and WON'T responses are guaranteed to leave the connection in a state
                    which both ends can handle. Thus, all hosts may implement their TELNET processes
                    to be totally unaware of options that are not supported, simply returning a
                    rejection to (i.e., refusing) any option request that cannot be understood."
                */
                default:
                    TelnetSendOptionCommand(DONT, option);
                break;
            }
        break;

        case YES:
            // No state change, ignored.
        break;


        // The states WANTNO and WANTYES are used when we have sent a DO/DONT to the server:

        case WANTNO:
            #ifdef _DEBUG
                PutStr("›36m Error: DONT answered by WILL.!›m\n");
            #endif
            if (telnetCtx.optState[option].himQ == EMPTY)
            {
                telnetCtx.optState[option].him = NO;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].him = YES;
                telnetCtx.optState[option].himQ = EMPTY;
            }
        break;

        case WANTYES:
            if (telnetCtx.optState[option].himQ == EMPTY)
            {
                telnetCtx.optState[option].him  = YES;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].him  = WANTNO;
                telnetCtx.optState[option].himQ = EMPTY;
                TelnetSendOptionCommand(DONT, option);
            }
        break;
    }

    if (option == TELOPT_ECHO)
    {
        switch(telnetCtx.optState[option].him)
        {
            case NO:
                // Server doesn't want to echo characters back to the client
                // so DCtelnet must enable "Local echoback" option
                SetLocalEchoBack(TRUE);
            break;

            case YES:
                SetLocalEchoBack(FALSE);
            break;
        }
    }
}

/**
 * @brief Negotiates and enables essential Telnet options required for Zmodem transfers.
 *
 * This function sequentially requests the negotiation of BINARY and SGA (Suppress Go Ahead) options
 * for both the server and client sides. These options are necessary for proper Zmodem file transfer
 * functionality.
 *
 */
static void TelnetNegotiateRequiredOptions(void)
{
    #ifdef _DEBUG
        PutStr("--> TelnetNegotiateRequiredOptions(): ");
    #endif
    if (telnetCtx.optState[TELOPT_BINARY].him == NO)
        TelnetAskToEnableServerOption(TELOPT_BINARY);
    if (telnetCtx.optState[TELOPT_SGA].him == NO)
        TelnetAskToEnableServerOption(TELOPT_SGA);
    if (telnetCtx.optState[TELOPT_BINARY].us == NO)
        TelnetAskToEnableClientOption(TELOPT_BINARY);
    if (telnetCtx.optState[TELOPT_SGA].us == NO)
        TelnetAskToEnableClientOption(TELOPT_SGA);
}


/**
 * @brief Handles the server's WONT announcement for a Telnet option.
 *
 * This function is called when the remote Telnet server announces that it refuses or disables a
 * Telnet option using the WONT command.
 *
 * The RFC1143 negotiation state machine is updated accordingly and any required response is
 * transmitted.
 *
 * @note RFC 854 section 2 states that a Telnet implementation must never refuse a request to
 *       disable an option.
 *
 * @param option
 *        Telnet option identifier rejected or disabled by the server.
 */
static void TelnetHandleWont(UBYTE option)
{
    switch(telnetCtx.optState[option].him)
    {
        case NO:
            // No state change, ignored.
        break;

        case YES:
            telnetCtx.optState[option].him = NO;
            TelnetSendOptionCommand(DONT, option);
        break;

        case WANTNO:
            if (telnetCtx.optState[option].himQ == EMPTY)
            {
                telnetCtx.optState[option].him = NO;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].him  = WANTYES;
                telnetCtx.optState[option].himQ = EMPTY;
                TelnetSendOptionCommand(DO, option);
            }
        break;

        case WANTYES:
            if (telnetCtx.optState[option].himQ == EMPTY)
            {
                telnetCtx.optState[option].him = NO;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].him = NO;
                telnetCtx.optState[option].himQ = EMPTY;
            }
        break;
    }


    if (option == TELOPT_ECHO && telnetCtx.optState[option].him == NO)
    {
        // Server doesn't want to echo characters back to the client
        // so DCtelnet must enable "Local echoback" option
        SetLocalEchoBack(TRUE);
    }
}


/**
 * @brief Handles the server's DO request for a Telnet option on client-side.
 *
 * This function is called when the server asks the client to enable a Telnet option (DO).
 *
 * It replies with WILL or WONT depending on whether the option is supported by DCTelnet.
 *
 * @param option The Telnet option code requested by the server.
 */
static void TelnetHandleDo(UBYTE option)
{
    switch(telnetCtx.optState[option].us)
    {
        case NO:
            switch(option)
            {
                // We agree to enable these options on our side:
                case TELOPT_BINARY:    // enable 8-bit data transmission
                case TELOPT_SGA:       // suppress GO AHEAD signals (Full-duplex communication)
                case TELOPT_TTYPE:     // Terminal type
                case TELOPT_NAWS:      // (Negotiate About Window Size, RFC 1073)
                    telnetCtx.optState[option].us = YES;
                    TelnetSendOptionCommand(WILL, option);
                break;


                /*  RFC 854 section 3 :
                    "the DON'T and WON'T responses are guaranteed to leave the connection in a state
                    which both ends can handle. Thus, all hosts may implement their TELNET processes
                    to be totally unaware of options that are not supported, simply returning a
                    rejection to (i.e., refusing) any option request that cannot be understood."

                    RFC 854, section 3.b states:
                    "It is required that a response be sent to requests for a change of mode -- even
                    if the mode is not changed."
                */
                default:
                    // also WONT TELOPT_ECHO of client, because client will never echo data chars
                    // it receives over the TELNET connection back to the sender of the data chars.
                    TelnetSendOptionCommand(WONT, option);
                break;
            }
        break;

        case YES:
            /* No state change, ignored.
               RFC 854, section 3.b states:
               "If a party receives what appears to be a request to enter some mode it is already
               in, the request should not be acknowledged. This non-response is essential to prevent
               endless loops in the negotiation."
            */
        break;


        // The states WANTNO and WANTYES are used when we have sent a WILL/WONT to the server:

        case WANTNO:
            #ifdef _DEBUG
                PutStr("›36m Error: WONT answered by DO.!›m\n");
            #endif
            if (telnetCtx.optState[option].usQ == EMPTY)
            {
                telnetCtx.optState[option].us = NO;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].us = YES;
                telnetCtx.optState[option].usQ = EMPTY;
            }
        break;

        case WANTYES:
            if (telnetCtx.optState[option].usQ == EMPTY)
            {
                telnetCtx.optState[option].us  = YES;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].us  = WANTNO;
                telnetCtx.optState[option].usQ = EMPTY;
                TelnetSendOptionCommand(WONT, option);
            }
        break;
    }

    if (option == TELOPT_NAWS && telnetCtx.optState[option].us == YES)
    {
        // After WILL NAWS, immediately send the window size as required by RFC.
        TelnetSendWindowSize();
    }
}


/**
 * @brief Handles the server's DONT request for a Telnet option.
 *
 * This function is called when the remote Telnet server requests or acknowledges that the client
 * disables a Telnet option using the DONT command.
 *
 * The local RFC1143 negotiation state machine is updated accordingly and any required response is
 * transmitted.
 *
 * @note RFC 854 section 2 states that a Telnet implementation must never refuse a request to
 *       disable an option.
 *
 * @param option
 *        Telnet option identifier the server wants disabled locally.
 */
static void TelnetHandleDont(UBYTE option)
{
    switch(telnetCtx.optState[option].us)
    {
        case NO:
            // No state change, ignored.
        break;

        case YES:
            telnetCtx.optState[option].us = NO;
            TelnetSendOptionCommand(WONT, option);
        break;

        case WANTNO:
            if (telnetCtx.optState[option].usQ == EMPTY)
            {
                telnetCtx.optState[option].us = NO;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].us  = WANTYES;
                telnetCtx.optState[option].usQ = EMPTY;
                TelnetSendOptionCommand(WILL, option);
            }
        break;

        case WANTYES:
            if (telnetCtx.optState[option].usQ == EMPTY)
            {
                telnetCtx.optState[option].us = NO;
            }
            else // OPPOSITE
            {
                telnetCtx.optState[option].us = NO;
                telnetCtx.optState[option].usQ = EMPTY;
            }
        break;
    }
}


/**
 * @brief Parse a single byte from the Telnet stream.
 *
 * This function implements the main Telnet protocol parser state machine.
 *
 * It processes:
 *   - normal application data
 *   - Telnet IAC command sequences
 *   - option negotiation (WILL/WONT/DO/DONT)
 *   - subnegotiation blocks (SB/SE)
 *   - escaped IAC bytes
 *
 * The parser is designed to operate incrementally on a byte stream so that Telnet commands remain
 * correctly decoded even when split across multiple TCP recv() calls.
 *
 * When a normal application data byte is processed, the function returns TRUE.
 *
 * Telnet protocol bytes are consumed internally and the function return FALSE.
 *
 * @param c
 *        Input byte received from the TCP stream.
 *
 *
 * @return TRUE if the byte is normal application data,
 *         FALSE if it belongs to the Telnet protocol layer.
 */
static BOOL TelnetParseByte(UBYTE c)
{
    switch(telnetCtx.state)
    {
        case TELNET_STATE_DATA:

            if(c != IAC)
            {
                // Normal application data
                return TRUE;
            }
            else
            {
                telnetCtx.state = TELNET_STATE_IAC;
            }

        break;


        case TELNET_STATE_IAC:

            #ifdef _DEBUG
                if (c != IAC)
                {
                    PutStr("›36m«IAC ");
                    if   (TELCMD_OK(c))  PutStr(TELCMD(c));
                    else                 LogByte(c);
                }
            #endif

            switch(c)
            {
                case IAC:
                    /* Escaped 0xFF */
                    telnetCtx.state = TELNET_STATE_DATA;
                    return TRUE;
                break;

                case WILL:
                    telnetCtx.state = TELNET_STATE_WILL;
                break;

                case WONT:
                    telnetCtx.state = TELNET_STATE_WONT;
                break;

                case DO:
                    telnetCtx.state = TELNET_STATE_DO;
                break;

                case DONT:
                    telnetCtx.state = TELNET_STATE_DONT;
                break;

                case SB:
                    telnetCtx.sbLength = 0;
                    telnetCtx.state = TELNET_STATE_SB;
                break;

                case SE:
                    telnetCtx.state = TELNET_STATE_DATA;
                break;


                default:
                    #ifdef _DEBUG
                        PutStr(" Ingored unsupported command!\n");
                    #endif
                    telnetCtx.state = TELNET_STATE_DATA;
                break;
            }

            #ifdef _DEBUG
                if (telnetCtx.state == TELNET_STATE_DATA) // Telnet IAC sequence ended so switch
                    PutStr("›m\n");                         // back to normal text attributes.
            #endif

        break; // case TELNET_STATE_IAC


        case TELNET_STATE_WILL:
            #ifdef _DEBUG
                PutStr(" ");
                if   (TELOPT_OK(c))  PutStr(TELOPT(c));
                else                 LogByte(c);
                PutStr("›m"); Flush(Output());
            #endif

            // Mark that we saw server-initiated negotiation. This is used to delay client-initiated
            // negotiation until the server's initial sequence has finished.
            telnetCtx.isServerNegotiationSeen = TRUE;

            TelnetHandleWill(c);

            telnetCtx.state = TELNET_STATE_DATA;
        break;


        case TELNET_STATE_WONT:
            #ifdef _DEBUG
                PutStr(" ");
                if   (TELOPT_OK(c))  PutStr(TELOPT(c));
                else                 LogByte(c);
                PutStr("›m"); Flush(Output());
            #endif

            TelnetHandleWont(c);
            telnetCtx.state = TELNET_STATE_DATA;
        break;


        case TELNET_STATE_DO:
            #ifdef _DEBUG
                PutStr(" ");
                if   (TELOPT_OK(c))  PutStr(TELOPT(c));
                else                 LogByte(c);
                PutStr("›m"); Flush(Output());
            #endif

            // Mark that we saw server-initiated negotiation. This is used to delay client-initiated
            // negotiation until the server's initial sequence has finished.
            telnetCtx.isServerNegotiationSeen = TRUE;

            TelnetHandleDo(c);

            telnetCtx.state = TELNET_STATE_DATA;
        break;


        case TELNET_STATE_DONT:
            #ifdef _DEBUG
                PutStr(" ");
                if   (TELOPT_OK(c))  PutStr(TELOPT(c));
                else                 LogByte(c);
                PutStr("›m"); Flush(Output());
            #endif

            TelnetHandleDont(c);
            telnetCtx.state = TELNET_STATE_DATA;
        break;


        case TELNET_STATE_SB:

            telnetCtx.sbOption = c;
            telnetCtx.sbLength = 0;
            telnetCtx.state = TELNET_STATE_SB_DATA;
            break;


        case TELNET_STATE_SB_DATA:

            if(c == IAC)
            {
                telnetCtx.state = TELNET_STATE_SB_IAC;
            }
            else
            {
                if(telnetCtx.sbLength < sizeof(telnetCtx.sbBuffer))
                {
                    telnetCtx.sbBuffer[telnetCtx.sbLength++] = c;
                }
            }

            break;


        case TELNET_STATE_SB_IAC:
            // Valid Telnet behavior at this point is strictly limited to:
            //  - IAC SE  : end of subnegotiation
            //  - IAC IAC : escaped 0xFF data byte
            if(c == SE)
            {
                TelnetHandleSubnegotiation();
                telnetCtx.state = TELNET_STATE_DATA;
            }
            else if(c == IAC)
            {
                // Escaped IAC inside subnegotiation
                if(telnetCtx.sbLength < sizeof(telnetCtx.sbBuffer))
                {
                    telnetCtx.sbBuffer[telnetCtx.sbLength++] = c;
                }

                telnetCtx.state = TELNET_STATE_SB_DATA;
            }
            else
            {
                telnetCtx.state = TELNET_STATE_DATA;

                #ifdef _DEBUG
                    LogByte(c);
                    PutStr(" Protocol corrupt or Telnet state desync inside subnegotiation!›m\n");
                #endif
            }

            break;
    }

    return FALSE;
}


/**
 * @brief ZModem transfer detection states.
 *
 * Defines the states used by the lightweight ZModem auto-start detector.
 *
 * The detector recognizes common ZModem startup sequences embedded inside
 * the terminal data stream in order to automatically switch to file transfer
 * handling mode.
 */
typedef enum
{
    ZMODEM_IDLE = 0,
    ZMODEM_GOT_FIRST_STAR,     // * seen
    ZMODEM_GOT_SECOND_STAR,    // * * seen
    ZMODEM_GOT_ZDLE,           // * * 0x18 seen (0x18 = ZDLE = ZModem Data Link Escape character)
    ZMODEM_GOT_B,              // * * 0x18 B seen
    ZMODEM_GOT_FIRST_ZERO,     // * * 0x18 B 0 seen
    ZMODEM_DOWNLOAD,           // * * 0x18 B 0 0 seen
    ZMODEM_UPLOAD,             // * * 0x18 B 0 1 seen
} ZmodemState;


/**
 * @brief Runtime context for the ZModem detector state machine.
 *
 * Stores the current state of the incremental ZModem startup sequence
 * detector.
 */
typedef struct
{
    ZmodemState state;
} ZmodemContext;

static ZmodemContext zmodemCtx = { ZMODEM_IDLE };

/**
 * @brief Reset the global Telnet parser and negotiation context.
 *
 * Clears the entire Telnet runtime context structure, including:
 *   - protocol parser state
 *   - subnegotiation buffer state
 *   - all RFC1143 negotiation states
 *
 * This function should typically be called:
 *   - before opening a new Telnet session
 *   - after a disconnect
 *   - when recovering from protocol desynchronization
 */
static void ResetZmodemContext(void)
{
    zmodemCtx.state = ZMODEM_IDLE;
}


/**
 * @brief Detect ZModem transfer initiation sequences in the incoming stream.
 *
 * This function implements a small state machine that recognizes the common ZModem start marker
 * sequence often emitted by BBSs and remote systems:
 *
 *   '*' '*' 0x18 'B' '0' '0'   -> ZMODEM_DOWNLOAD (sz)
 *   '*' '*' 0x18 'B' '0' '1'   -> ZMODEM_UPLOAD   (rz)
 *
 * The function updates the module-local `zmodemCtx.state` and returns TRUE when the provided byte
 * completes a ZModem start sequence (i.e. the state enters `ZMODEM_DOWNLOAD` or `ZMODEM_UPLOAD`).
 * When TRUE is returned, the caller should treat subsequent bytes as part of a file transfer stream
 * and hand them off to the transfer subsystem (`Xfer.c`).
 *
 * @param c Byte received from the remote host to inspect.
 * @return TRUE if this byte completed a ZModem transfer start (entering
 *         download or upload state); FALSE otherwise.
 */
static BOOL ZmodemDetect(UBYTE c)
{
    switch(zmodemCtx.state)
    {
        case ZMODEM_IDLE:
            if (c == '*') zmodemCtx.state = ZMODEM_GOT_FIRST_STAR;
        return FALSE;

        case ZMODEM_GOT_FIRST_STAR:
            zmodemCtx.state = (c == '*') ? ZMODEM_GOT_SECOND_STAR : ZMODEM_IDLE;
        return FALSE;

        case ZMODEM_GOT_SECOND_STAR:
            zmodemCtx.state = (c == 0x18) ? ZMODEM_GOT_ZDLE : ZMODEM_IDLE;
        return FALSE;

        case ZMODEM_GOT_ZDLE:
            zmodemCtx.state = (c == 'B') ? ZMODEM_GOT_B : ZMODEM_IDLE;
        return FALSE;

        case ZMODEM_GOT_B:
            zmodemCtx.state = (c == '0') ? ZMODEM_GOT_FIRST_ZERO : ZMODEM_IDLE;
        return FALSE;

        case ZMODEM_GOT_FIRST_ZERO:
            switch(c)
            {
                case '1':
                    zmodemCtx.state = ZMODEM_UPLOAD;
                return TRUE;

                case '0':
                    zmodemCtx.state = ZMODEM_DOWNLOAD;
                return TRUE;

                default:
                    zmodemCtx.state = ZMODEM_IDLE;
                return FALSE;
            }
    }
}




/**
 * @brief Check whether the current Telnet session is suitable for ZMODEM transfers.
 *
 * Verifies that the Telnet BINARY and SUPPRESS-GO-AHEAD (SGA) options are enabled in both
 * directions ("us" and "him"), which is required for reliable transparent 8-bit ZMODEM data
 * transmission over Telnet.
 *
 * If a required option is missing, a diagnostic message is displayed.
 *
 * @retval TRUE  The Telnet session is properly configured for ZMODEM.
 * @retval FALSE One or more required Telnet options are not enabled.
 */
static BOOL IsTelnetSessionReadyForXfer(void)
{
    BOOL result = TRUE;

    if (telnetCtx.optState[TELOPT_BINARY].us != YES)
    {
        LocalPrint("\r\n›33mTelnet BINARY option is not enabled on our side!›m\r\n");
        result = FALSE;
    }

    if (telnetCtx.optState[TELOPT_SGA].us != YES)
    {
        LocalPrint("\r\n›33mTelnet SGA option is not enabled on our side!›m\r\n");
        result = FALSE;
    }

    if (telnetCtx.optState[TELOPT_BINARY].him != YES)
    {
        LocalPrint("\r\n›33mTelnet BINARY option is not enabled on server side!›m\r\n");
        result = FALSE;
    }

    if (telnetCtx.optState[TELOPT_SGA].him != YES)
    {
        LocalPrint("\r\n›33mTelnet SGA option is not enabled on server side!›m\r\n");
        result = FALSE;
    }

    return result;
}
