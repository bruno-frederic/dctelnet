#ifndef _INCLUDE_PRAGMA_XEM_LIB_H
#define _INCLUDE_PRAGMA_XEM_LIB_H

#ifndef CLIB_XEM_PROTOS_H
#include <clib/xem_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(XEmulatorBase,0x01e,XEmulatorSetup(a0))
#pragma amicall(XEmulatorBase,0x024,XEmulatorOpenConsole(a0))
#pragma amicall(XEmulatorBase,0x02a,XEmulatorCloseConsole(a0))
#pragma amicall(XEmulatorBase,0x030,XEmulatorCleanup(a0))
#pragma amicall(XEmulatorBase,0x036,XEmulatorWrite(a0,a1,d0))
#pragma amicall(XEmulatorBase,0x03c,XEmulatorSignal(a0,d0))
#pragma amicall(XEmulatorBase,0x042,XEmulatorHostMon(a0,a1,d0))
#pragma amicall(XEmulatorBase,0x048,XEmulatorUserMon(a0,a1,d0,a2))
#pragma amicall(XEmulatorBase,0x04e,XEmulatorOptions(a0))
#pragma amicall(XEmulatorBase,0x054,XEmulatorClearConsole(a0))
#pragma amicall(XEmulatorBase,0x05a,XEmulatorResetConsole(a0))
#pragma amicall(XEmulatorBase,0x060,XEmulatorResetTextStyles(a0))
#pragma amicall(XEmulatorBase,0x066,XEmulatorResetCharset(a0))
#pragma amicall(XEmulatorBase,0x06c,XEmulatorGetFreeMacroKeys(a0,d0))
#pragma amicall(XEmulatorBase,0x072,XEmulatorMacroKeyFilter(a0,a1))
#pragma amicall(XEmulatorBase,0x078,XEmulatorInfo(a0,d0))
#pragma amicall(XEmulatorBase,0x07e,XEmulatorPreferences(a0,a1,d0))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma  libcall XEmulatorBase XEmulatorSetup         01e 801
#pragma  libcall XEmulatorBase XEmulatorOpenConsole   024 801
#pragma  libcall XEmulatorBase XEmulatorCloseConsole  02a 801
#pragma  libcall XEmulatorBase XEmulatorCleanup       030 801
#pragma  libcall XEmulatorBase XEmulatorWrite         036 09803
#pragma  libcall XEmulatorBase XEmulatorSignal        03c 0802
#pragma  libcall XEmulatorBase XEmulatorHostMon       042 09803
#pragma  libcall XEmulatorBase XEmulatorUserMon       048 a09804
#pragma  libcall XEmulatorBase XEmulatorOptions       04e 801
#pragma  libcall XEmulatorBase XEmulatorClearConsole  054 801
#pragma  libcall XEmulatorBase XEmulatorResetConsole  05a 801
#pragma  libcall XEmulatorBase XEmulatorResetTextStyles 060 801
#pragma  libcall XEmulatorBase XEmulatorResetCharset  066 801
#pragma  libcall XEmulatorBase XEmulatorGetFreeMacroKeys 06c 0802
#pragma  libcall XEmulatorBase XEmulatorMacroKeyFilter 072 9802
#pragma  libcall XEmulatorBase XEmulatorInfo          078 0802
#pragma  libcall XEmulatorBase XEmulatorPreferences   07e 09803
#endif

#endif	/*  _INCLUDE_PRAGMA_XEM_LIB_H  */
