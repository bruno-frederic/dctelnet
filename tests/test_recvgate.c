/* test/test_recvgate.c -- when the main loop may call the blocking Receive(). */
#include <assert.h>
#include <stdio.h>
#include "recvgate.h"

int main(void)
{
    /* the freeze: WaitSelect error / signal-only wakeup must not reach recv() */
    assert(RecvGate(-1, 0, 0, 1) == RECV_SKIP);          /* EINTR */
    assert(RecvGate(1, 0, 0, 0) == RECV_SKIP);           /* ready fd is not the socket */
    assert(RecvGate(0, 0, 0, 0) == RECV_SKIP);           /* timeout / signal */
    /* real data */
    assert(RecvGate(1, 1, 0, 0) == RECV_RECEIVE);
    /* real error: connection error, not a blocking recv() */
    assert(RecvGate(-1, 0, 0, 0) == RECV_DISCONNECT);
    /* SSH bytes already decrypted: socket silent, must still read */
    assert(RecvGate(0, 0, 1, 0) == RECV_RECEIVE);
    assert(RecvGate(-1, 0, 1, 1) == RECV_RECEIVE);
    puts("test_recvgate: ok");
    return 0;
}
