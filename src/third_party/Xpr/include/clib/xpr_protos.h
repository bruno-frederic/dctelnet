#ifndef CLIB_XPR_PROTOS_H
#define CLIB_XPR_PROTOS_H
/*
**	$VER: Xpr_protos.h 3.1 (03.02.2026)
**
**	'C' prototypes extracted from XprZmodem.h (XPRZModem v3.1 by Rainer Hess),
**	taken from the "comm/misc/xprz31.lha" archive on Aminet CD 2.
**
**	This header is used for rebuilding DCTelnet.
**
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* struct XPR_IO, XPR_UPDATE and XPR preprocessor macros */
#ifndef  LIBRARIES_XPR_H
#include <libraries/Xpr.h>
#endif

/* XPRotocol functions */

long __saveds __asm XProtocolSend (register __a0 struct XPR_IO *xio);
long __saveds __asm XProtocolReceive (register __a0 struct XPR_IO *xio);
long __saveds __asm XProtocolSetup (register __a0 struct XPR_IO *xio);
long __saveds __asm XProtocolCleanup (register __a0 struct XPR_IO *xio);
long __saveds __asm XProtocolHostMon (
				       register __a0 struct XPR_IO *xio,
				       register __a1 char *serbuff,
				       register __d0 long actual,
				       register __d1 long maxsize);
long __saveds __asm XProtocolUserMon (
				       register __a0 struct XPR_IO *xio,
				       register __a1 char *serbuff,
				       register __d0 long actual,
				       register __d1 long maxsize);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_XPR_PROTOS_H */
