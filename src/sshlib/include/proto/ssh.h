#ifndef PROTO_SSH_H
#define PROTO_SSH_H

#include <exec/types.h>
#ifndef CLIB_SSH_PROTOS_H
#include <clib/ssh_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *SshBase;
#endif

#ifdef __VBCC__
#include <inline/ssh_protos.h>
#endif

#endif /* PROTO_SSH_H */
