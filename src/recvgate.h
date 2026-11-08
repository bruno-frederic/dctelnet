/* src/recvgate.h -- when the main loop may call Receive() after WaitSelect().
 *
 * Receive() does a blocking recv() on the socket, so it must only run when
 * the socket is readable (or the SSH layer holds decrypted bytes, which the
 * socket will not announce). WaitSelect() returns -1 on error and N > 0 when
 * N descriptors are ready -- N > 0 can also be a signal-only wakeup with the
 * TCP socket not readable. The old gate (i != 0) let both reach recv() and
 * froze the program. Pure, unit-tested on the host. */
#ifndef RECVGATE_H
#define RECVGATE_H

#define RECV_SKIP        0      /* nothing to read: go round the loop again */
#define RECV_RECEIVE     1      /* call Receive() */
#define RECV_DISCONNECT  2      /* WaitSelect failed for real: connection error */

/* i = WaitSelect() result, readable = FD_ISSET(tcpSocket), sshBuffered =
 * SshConn_Buffered(), interrupted = Errno() == EINTR (only used when i < 0) */
static int RecvGate(int i, int readable, int sshBuffered, int interrupted)
{
    if (i > 0 && readable) return RECV_RECEIVE;
    if (sshBuffered)       return RECV_RECEIVE;
    if (i < 0 && !interrupted) return RECV_DISCONNECT;
    return RECV_SKIP;
}

#endif
