/* Login.rexx -- logs in to a BBS through DCTelnet's ARexx port.
 *
 *   rx Login.rexx <host> <port> <name> <password>
 *
 * DCTelnet must be running (its port is DCTELNET.1; a second DCTelnet
 * is DCTELNET.2). Change the prompts to what your BBS asks. */
options results
parse arg host port name password
if password = '' then do
    say 'Usage: rx Login.rexx <host> <port> <name> <password>'
    exit 5
end
address 'DCTELNET.1'
'CONNECT' host port
'WAITFOR "Name:" 30'
if rc ~= 0 then do; say 'No name prompt.'; exit 10; end
'SENDLN "'name'"'
'WAITFOR "Password:" 30'
if rc ~= 0 then do; say 'No password prompt.'; exit 10; end
'SENDLN "'password'"'
'GETSTATUS'
say result
