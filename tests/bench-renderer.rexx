/**
 * @file  bench-renderer.rexx
 * @brief Run on the Amiga to benchmark DCTelnet startup, rendering, and receiving speed
 *
 * Launch it from tests/ folder
 *
 * The renderer duration measures successive ConWrite() calls processing 4096-byte buffers from
 * tests/test-speed.ans, which represents navigation in the message reader of Mystic BBS.
 *
 * Call bench-receive-throughput.rexx to benchmark Receive() throughput.
 *
 * This script also reports the end-to-end duration, including renderer execution, application
 * startup and shutdown, and library, device, and component initialization.
 * The receiving throughput duration is measured separately.
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

/* Terminate the PARSE ARG statement with a period so that the last variable receives its value
 * correctly; otherwise, the value is prefixed with a space. */
parse arg Renderer Iteration Option .
UPPER Option
if Iteration = '' | (Option ~= '' & Option ~= 'NOAREXX') then do
    say 'Usage: rx bench-renderer.rexx <Renderer> <Iteration> [NoArexx]'
    say '  NoArexx: skip the ARexx-driven receive-throughput benchmark'
    exit 20
end
NoArexx = (Option = 'NOAREXX')

ADDRESS COMMAND

/* Keep this path in sync with INSTALLDIR in src/Makefile; make install places the binaries used by
 * this script here.
 */
INSTALLDIR  = "/build/installed/DCTelnet"

initialDir = PRAGMA(DIRECTORY, INSTALLDIR)

do i = 1 to Iteration
    /* DCTelnet --test-speed will output: "render_duration_ms":x.x */
    CALL WRITECH('STDOUT', ' {')
    'TimeIt DCTelnet --test-speed' Renderer

    if ~NoArexx then do
        CALL WRITECH('STDOUT', ', ')
        /* Launch DCTelnet in the background, detached from the Shell window */
        'RUN >NIL: DCTelnet'
        /* Under KS 2.04, WaitForPort needs the port name without double quotes */
        'SYS:Rexxc/WaitForPort DCTELNET.1'
        if rc == 0 then do
            ADDRESS 'REXX' initialDir'/bench-receive-throughput.rexx localhost 5201'
            ADDRESS 'DCTELNET.1' 'QUIT'
        end
        else do
            SAY '"receive_duration_seconds":null, "receive_error":"bench-renderer.rexx: DCTelnet ARexx port not found"'
        end
    end

    if i == Iteration then CALL WRITECH('STDOUT', '}')
    else                   SAY '},'
end

dummy=PRAGMA(DIRECTORY, initialDir)