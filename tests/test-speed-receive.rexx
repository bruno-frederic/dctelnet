/* test-speed-receive.rexx -- run on the Amiga to benchmark DCTelnet's receive throughput
 *                            using the Docker test server defined in Dockerfile
 *
 * Start the Telnet test-server Docker container first (see tests/Dockerfile).
 *
 * DCTelnet must be running (its port is DCTELNET.1; a second DCTelnet
 * is DCTELNET.2). Change the prompts to what your BBS asks.
 */

options results
/* Terminate the PARSE ARG statement with a period so that the password variable receives its value
 * correctly; otherwise, the value is prefixed with a space. */
parse arg host port name password .
if password = '' then do
    say 'Usage: rx test-speed-receive.rexx <host> <port> <name> <password>'
    exit 20
end

if ( ~SHOW( 'P', 'DCTELNET.1') ) then CALL WarnAndExit "NO ARexx port!"

address 'DCTELNET.1'

'CONNECT' host port

/* Receiving the login prompt confirms the network connection */
/* FIXME: error not trapped an message displayed when telnetd server not started/unreachable */
'WAITFOR "login:" 30'
if rc ~= 0 then CALL WarnAndExit "No name prompt!"
'SENDLN "'name'"'

'WAITFOR "Password:" 30'
if rc ~= 0 then CALL WarnAndExit "No password prompt!"
'SENDLN "'password'"'

'WAITFOR ":~$ " 30'
if rc ~= 0 then CALL WarnAndExit "No shell prompt!"

/* Start the test and wait for it to finish. The shell prompt reappearing confirms that the test
 * has completed. */
dummy = TIME('R')
'SENDLN "./test-speed-receive-server.sh"'
'WAITFOR ":~$ " 60'
elapsed = TIME('E')
if rc ~= 0 then CALL WarnAndExit "No shell prompt after en of test!"

CALL WRITECH('STDOUT', '"receive_duration_seconds":'elapsed)

'DISCONNECT'

EXIT

WarnAndExit:
    PROCEDURE
    PARSE ARG msg

    SAY '    "receive_duration_seconds":null, "receive_error":"'msg'"'

    EXIT 5