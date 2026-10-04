#ifndef _INLINE_XPR_H
#define _INLINE_XPR_H

#ifndef CLIB_XPR_PROTOS_H
#define CLIB_XPR_PROTOS_H
#endif

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef  LIBRARIES_XPR_H
#include <libraries/Xpr.h>
#endif

#ifndef XPR_BASE_NAME
#define XPR_BASE_NAME XProtocolBase
#endif

#define XProtocolCleanup(io) \
	LP1(0x1e, long, XProtocolCleanup, struct XPR_IO *, io, a0, \
	, XPR_BASE_NAME)

#define XProtocolSetup(io) \
	LP1(0x24, long, XProtocolSetup, struct XPR_IO *, io, a0, \
	, XPR_BASE_NAME)

#define XProtocolSend(io) \
	LP1(0x2a, long, XProtocolSend, struct XPR_IO *, io, a0, \
	, XPR_BASE_NAME)

#define XProtocolReceive(io) \
	LP1(0x30, long, XProtocolReceive, struct XPR_IO *, io, a0, \
	, XPR_BASE_NAME)

#define XProtocolHostMon(io, serbuff, actual, maxsize) \
	LP4(0x36, long, XProtocolHostMon, struct XPR_IO *, io, a0, char *, serbuff, a1, long, actual, d0, long, maxsize, d1, \
	, XPR_BASE_NAME)

#define XProtocolUserMon(io, serbuff, actual, maxsize) \
	LP4(0x3c, long, XProtocolUserMon, struct XPR_IO *, io, a0, char *, serbuff, a1, long, actual, d0, long, maxsize, d1, \
	, XPR_BASE_NAME)

#endif /*  _INLINE_XPR_H  */
