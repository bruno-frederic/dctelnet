#ifndef _VBCCINLINE_XPR_H
#define _VBCCINLINE_XPR_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

long __XProtocolCleanup(__reg("a6") void *, __reg("a0") struct XPR_IO * io)="\tjsr\t-30(a6)";
#define XProtocolCleanup(io) __XProtocolCleanup(XProtocolBase, (io))

long __XProtocolSetup(__reg("a6") void *, __reg("a0") struct XPR_IO * io)="\tjsr\t-36(a6)";
#define XProtocolSetup(io) __XProtocolSetup(XProtocolBase, (io))

long __XProtocolSend(__reg("a6") void *, __reg("a0") struct XPR_IO * io)="\tjsr\t-42(a6)";
#define XProtocolSend(io) __XProtocolSend(XProtocolBase, (io))

long __XProtocolReceive(__reg("a6") void *, __reg("a0") struct XPR_IO * io)="\tjsr\t-48(a6)";
#define XProtocolReceive(io) __XProtocolReceive(XProtocolBase, (io))

long __XProtocolHostMon(__reg("a6") void *, __reg("a0") struct XPR_IO * io, __reg("a1") char * serbuff, __reg("d0") long actual, __reg("d1") long maxsize)="\tjsr\t-54(a6)";
#define XProtocolHostMon(io, serbuff, actual, maxsize) __XProtocolHostMon(XProtocolBase, (io), (serbuff), (actual), (maxsize))

long __XProtocolUserMon(__reg("a6") void *, __reg("a0") struct XPR_IO * io, __reg("a1") char * serbuff, __reg("d0") long actual, __reg("d1") long maxsize)="\tjsr\t-60(a6)";
#define XProtocolUserMon(io, serbuff, actual, maxsize) __XProtocolUserMon(XProtocolBase, (io), (serbuff), (actual), (maxsize))

#endif /*  _VBCCINLINE_XPR_H  */
