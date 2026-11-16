/* test-speed.rexx -- Run on the Amiga to benchmark DCTelnet startup, rendering, and receiving speed
 *
 * The renderer duration measures successive ConWrite() calls processing 4096-byte buffers from
 * tests\test-speed.ans, which represents navigation in the message reader of Mystic BBS.
 *
 * Call test-speed-receive.rexx to benchmark Receive() throughput.
 *
 * This script also reports the end-to-end duration, including renderer execution, application
 * startup and shutdown, and library, device, and component initialization. The receiving test
 * duration is measured separately.
 */

parse arg Renderer Iteration .
if Iteration = '' then do
    say 'Usage: rx test-speed.rexx <Renderer> <Iteration>'
    exit 20
end

ADDRESS COMMAND

/* Keep this path in sync with INSTALLDIR in src/Makefile; make install places the binaries used by
 * this script here.
 */
INSTALLDIR  = "/build/installed/DCTelnet"

DO i = 1 TO Iteration
    /* DCTelnet --test-speed will output: "renderer_duration_seconds":x.x */
    CALL WRITECH('STDOUT', ' { ')
    dummy = TIME('R')
    INSTALLDIR'/DCTelnet.68000 --test-speed' Renderer
    elapsed = TIME('E')
    SAY ','

    CALL WRITECH('STDOUT', '   "total_duration_seconds":'elapsed', ')

    /* Launch DCTelnet in the background, detached from the Shell window */
    'RUN >NIL: "'INSTALLDIR'/DCTelnet.68000"'
    'SYS:Rexxc/WaitForPort "DCTELNET.1"'
    ADDRESS 'REXX' 'test-speed-receive.rexx localhost 23 user ***'

    ADDRESS 'DCTELNET.1' QUIT

    IF i == Iteration THEN CALL WRITECH('STDOUT', ' }')
    ELSE                   SAY ' },'
END

