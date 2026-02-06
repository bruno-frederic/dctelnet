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

long XProtocolSend (struct XPR_IO *xio);
long XProtocolReceive (struct XPR_IO *xio);
long XProtocolSetup (struct XPR_IO *xio);
long XProtocolCleanup (struct XPR_IO *xio);
long XProtocolHostMon (
				       struct XPR_IO *xio,
				       char *serbuff,
				       long actual,
				       long maxsize);
long XProtocolUserMon (
				       struct XPR_IO *xio,
				       char *serbuff,
				       long actual,
				       long maxsize);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_XPR_PROTOS_H */
