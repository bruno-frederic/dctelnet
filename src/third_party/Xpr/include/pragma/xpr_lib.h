#ifndef _INCLUDE_PRAGMA_XPR_LIB_H
#define _INCLUDE_PRAGMA_XPR_LIB_H

#ifndef CLIB_XPR_PROTOS_H
#include <clib/xpr_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(XProtocolBase,0x01e,XProtocolCleanup(a0))
#pragma amicall(XProtocolBase,0x024,XProtocolSetup(a0))
#pragma amicall(XProtocolBase,0x02a,XProtocolSend(a0))
#pragma amicall(XProtocolBase,0x030,XProtocolReceive(a0))
#pragma amicall(XProtocolBase,0x036,XProtocolHostMon(a0,a1,d0,d1))
#pragma amicall(XProtocolBase,0x03c,XProtocolUserMon(a0,a1,d0,d1))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma  libcall XProtocolBase XProtocolCleanup       01e 801
#pragma  libcall XProtocolBase XProtocolSetup         024 801
#pragma  libcall XProtocolBase XProtocolSend          02a 801
#pragma  libcall XProtocolBase XProtocolReceive       030 801
#pragma  libcall XProtocolBase XProtocolHostMon       036 109804
#pragma  libcall XProtocolBase XProtocolUserMon       03c 109804
#endif

#endif	/*  _INCLUDE_PRAGMA_XPR_LIB_H  */
