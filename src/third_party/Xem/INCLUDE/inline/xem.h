#ifndef _INLINE_XEM_H
#define _INLINE_XEM_H

#ifndef CLIB_XEM_PROTOS_H
#define CLIB_XEM_PROTOS_H
#endif

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#include <exec/types.h>

#ifndef XEM_BASE_NAME
#define XEM_BASE_NAME XEmulatorBase
#endif

#define XEmulatorSetup(io) \
	LP1(0x1e, BOOL, XEmulatorSetup, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorOpenConsole(io) \
	LP1(0x24, BOOL, XEmulatorOpenConsole, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorCloseConsole(io) \
	LP1NR(0x2a, XEmulatorCloseConsole, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorCleanup(io) \
	LP1NR(0x30, XEmulatorCleanup, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorWrite(io, string, len) \
	LP3NR(0x36, XEmulatorWrite, struct XEM_IO *, io, a0, UBYTE *, string, a1, LONG, len, d0, \
	, XEM_BASE_NAME)

#define XEmulatorSignal(io, signal) \
	LP2(0x3c, BOOL, XEmulatorSignal, struct XEM_IO *, io, a0, ULONG, signal, d0, \
	, XEM_BASE_NAME)

#define XEmulatorHostMon(io, buf, len) \
	LP3(0x42, ULONG, XEmulatorHostMon, struct XEM_IO *, io, a0, struct HostData *, buf, a1, ULONG, len, d0, \
	, XEM_BASE_NAME)

#define XEmulatorUserMon(io, buf, len, imsg) \
	LP4(0x48, ULONG, XEmulatorUserMon, struct XEM_IO *, io, a0, UBYTE *, buf, a1, ULONG, len, d0, struct IntuiMessage *, imsg, a2, \
	, XEM_BASE_NAME)

#define XEmulatorOptions(io) \
	LP1NR(0x4e, XEmulatorOptions, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorClearConsole(io) \
	LP1NR(0x54, XEmulatorClearConsole, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorResetConsole(io) \
	LP1NR(0x5a, XEmulatorResetConsole, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorResetTextStyles(io) \
	LP1NR(0x60, XEmulatorResetTextStyles, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorResetCharset(io) \
	LP1NR(0x66, XEmulatorResetCharset, struct XEM_IO *, io, a0, \
	, XEM_BASE_NAME)

#define XEmulatorGetFreeMacroKeys(io, qualifier) \
	LP2(0x6c, ULONG, XEmulatorGetFreeMacroKeys, struct XEM_IO *, io, a0, ULONG, qualifier, d0, \
	, XEM_BASE_NAME)

#define XEmulatorMacroKeyFilter(io, list) \
	LP2(0x72, BOOL, XEmulatorMacroKeyFilter, struct XEM_IO *, io, a0, struct List *, list, a1, \
	, XEM_BASE_NAME)

#define XEmulatorInfo(xem_io, type) \
	LP2(0x78, LONG, XEmulatorInfo, struct XEM_IO *, xem_io, a0, ULONG, type, d0, \
	, XEM_BASE_NAME)

#define XEmulatorPreferences(xem_io, filename, mode) \
	LP3(0x7e, BOOL, XEmulatorPreferences, struct XEM_IO *, xem_io, a0, STRPTR, filename, a1, ULONG, mode, d0, \
	, XEM_BASE_NAME)

#endif /*  _INLINE_XEM_H  */
