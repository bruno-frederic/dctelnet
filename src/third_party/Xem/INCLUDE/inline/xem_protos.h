#ifndef _VBCCINLINE_XEM_H
#define _VBCCINLINE_XEM_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

BOOL __XEmulatorSetup(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-30(a6)";
#define XEmulatorSetup(io) __XEmulatorSetup(XEmulatorBase, (io))

BOOL __XEmulatorOpenConsole(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-36(a6)";
#define XEmulatorOpenConsole(io) __XEmulatorOpenConsole(XEmulatorBase, (io))

VOID __XEmulatorCloseConsole(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-42(a6)";
#define XEmulatorCloseConsole(io) __XEmulatorCloseConsole(XEmulatorBase, (io))

VOID __XEmulatorCleanup(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-48(a6)";
#define XEmulatorCleanup(io) __XEmulatorCleanup(XEmulatorBase, (io))

VOID __XEmulatorWrite(__reg("a6") void *, __reg("a0") struct XEM_IO * io, __reg("a1") UBYTE * string, __reg("d0") LONG len)="\tjsr\t-54(a6)";
#define XEmulatorWrite(io, string, len) __XEmulatorWrite(XEmulatorBase, (io), (string), (len))

BOOL __XEmulatorSignal(__reg("a6") void *, __reg("a0") struct XEM_IO * io, __reg("d0") ULONG signal)="\tjsr\t-60(a6)";
#define XEmulatorSignal(io, signal) __XEmulatorSignal(XEmulatorBase, (io), (signal))

ULONG __XEmulatorHostMon(__reg("a6") void *, __reg("a0") struct XEM_IO * io, __reg("a1") struct HostData * buf, __reg("d0") ULONG len)="\tjsr\t-66(a6)";
#define XEmulatorHostMon(io, buf, len) __XEmulatorHostMon(XEmulatorBase, (io), (buf), (len))

ULONG __XEmulatorUserMon(__reg("a6") void *, __reg("a0") struct XEM_IO * io, __reg("a1") UBYTE * buf, __reg("d0") ULONG len, __reg("a2") struct IntuiMessage * imsg)="\tjsr\t-72(a6)";
#define XEmulatorUserMon(io, buf, len, imsg) __XEmulatorUserMon(XEmulatorBase, (io), (buf), (len), (imsg))

VOID __XEmulatorOptions(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-78(a6)";
#define XEmulatorOptions(io) __XEmulatorOptions(XEmulatorBase, (io))

VOID __XEmulatorClearConsole(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-84(a6)";
#define XEmulatorClearConsole(io) __XEmulatorClearConsole(XEmulatorBase, (io))

VOID __XEmulatorResetConsole(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-90(a6)";
#define XEmulatorResetConsole(io) __XEmulatorResetConsole(XEmulatorBase, (io))

VOID __XEmulatorResetTextStyles(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-96(a6)";
#define XEmulatorResetTextStyles(io) __XEmulatorResetTextStyles(XEmulatorBase, (io))

VOID __XEmulatorResetCharset(__reg("a6") void *, __reg("a0") struct XEM_IO * io)="\tjsr\t-102(a6)";
#define XEmulatorResetCharset(io) __XEmulatorResetCharset(XEmulatorBase, (io))

ULONG __XEmulatorGetFreeMacroKeys(__reg("a6") void *, __reg("a0") struct XEM_IO * io, __reg("d0") ULONG qualifier)="\tjsr\t-108(a6)";
#define XEmulatorGetFreeMacroKeys(io, qualifier) __XEmulatorGetFreeMacroKeys(XEmulatorBase, (io), (qualifier))

BOOL __XEmulatorMacroKeyFilter(__reg("a6") void *, __reg("a0") struct XEM_IO * io, __reg("a1") struct List * list)="\tjsr\t-114(a6)";
#define XEmulatorMacroKeyFilter(io, list) __XEmulatorMacroKeyFilter(XEmulatorBase, (io), (list))

LONG __XEmulatorInfo(__reg("a6") void *, __reg("a0") struct XEM_IO * xem_io, __reg("d0") ULONG type)="\tjsr\t-120(a6)";
#define XEmulatorInfo(xem_io, type) __XEmulatorInfo(XEmulatorBase, (xem_io), (type))

BOOL __XEmulatorPreferences(__reg("a6") void *, __reg("a0") struct XEM_IO * xem_io, __reg("a1") STRPTR filename, __reg("d0") ULONG mode)="\tjsr\t-126(a6)";
#define XEmulatorPreferences(xem_io, filename, mode) __XEmulatorPreferences(XEmulatorBase, (xem_io), (filename), (mode))

#endif /*  _VBCCINLINE_XEM_H  */
