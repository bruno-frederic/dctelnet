/**
 * @file  bench-receive-throughput.rexx
 * @brief run on the Amiga to benchmark DCTelnet's receive throughput
 *
 * Use the test-server Docker image and its bench-throughput.sh script.
 *
 * Start the Telnet test-server Docker container first (see tests/Dockerfile).
 *
 * DCTelnet must be running (its port is DCTELNET.1; a second DCTelnet is DCTELNET.2).
 *
 * If @a name and @a password are both specified, the script waits for the login and password
 * prompts from the Telnet server before sending them. If omitted, the script sends the bench
 * command directly.
 *
 * @param name     Optional login name.
 * @param password Optional password.
 *
 * @author Bruno FREDERIC
 * @date 2026
 */

/* WAITFOR returns 10 sometimes, which is the default FAILAT level: raise it so that ARexx does not
 * display "Command returned 10" and the script reports the error with its own message. */
options failat 11
options results
parse arg host port name password .
if host = '' then do
    say 'Usage: rx bench-receive-throughput.rexx <host> <port> [<name> <password>]'
    exit 20
end

if ( ~SHOW( 'P', 'DCTELNET.1') ) then
    CALL WarnAndExit "bench-receive-throughput.rexx: DCTelnet ARexx port not found"

address 'DCTELNET.1'

'CONNECT' host port

/* Receiving the awaited prompt confirms the network connection */
if name ~= '' & password  ~= ''
THEN DO
    'WAITFOR "login:" 30'
    if rc ~= 0 then CALL WarnAndExit "No name prompt!"
    'SENDLN "'name'"'

    'WAITFOR "Password:" 30'
    if rc ~= 0 then CALL WarnAndExit "No password prompt!"
    'SENDLN "'password'"'
END

/* GETSTATUS before WAITFOR:
   without it, the first WAITFOR failed about 1 time out of 10 (cause not identified) */
'GETSTATUS'
statusBefore = result

/* Wait for the shell prompt */
'WAITFOR ":~$ " 30'
if rc ~= 0 then do
    returnCode = rc
    msg = 'WAITFOR failed, rc='returnCode', statusBefore='statusBefore
    CALL WarnAndExit msg
    /* Without the GETSTATUS above, the first WAITFOR sometimes failed and a second one was needed*/
    /*
    'WAITFOR ":~$ " 30'
    if rc ~= 0 then CALL WarnAndExit "No shell prompt!"
    */
end


/* Start the test and wait for it to finish. The shell prompt reappearing confirms that the test
 * has completed. */
dummy = TIME('R')
'SENDLN "cat ~/test-speed.ans"'
'WAITFOR ":~$ " 60'
elapsed = TIME('E')
if rc ~= 0 then CALL WarnAndExit "No shell prompt after end of test!"

CALL WRITECH('STDOUT', '"receive_duration_seconds":'elapsed)

'DISCONNECT'

EXIT

WarnAndExit:
    PROCEDURE
    PARSE ARG msg

    CALL WRITECH('STDOUT', '"receive_duration_seconds":null, "receive_error":"'msg'"')

    if ( SHOW( 'P', 'DCTELNET.1') ) then do
        address 'DCTELNET.1' 'DISCONNECT'
    end

    EXIT 5