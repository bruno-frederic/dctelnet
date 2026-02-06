/*
 * :ts=8
 *
 * 'Roadshow' -- Amiga TCP/IP stack
 * Copyright © 2001-2022 by Olaf Barthel.
 * All Rights Reserved.
 *
 * Amiga specific TCP/IP 'C' header files;
 * Freely Distributable
 */

/*
 * This file was created with fd2pragma V2.197g using the following options:
 *
 * fd2pragma --special 70 --voidbase --infile usergroup_lib.sfd
 */

#ifndef _VBCCINLINE_USERGROUP_H
#define _VBCCINLINE_USERGROUP_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

LONG __ug_SetupContextTagList(__reg("a6") void *, __reg("a0") STRPTR name, __reg("a1") struct TagItem * tags)="\tjsr\t-30(a6)";
#define ug_SetupContextTagList(name, tags) __ug_SetupContextTagList(UserGroupBase, (name), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __ug_SetupContextTags(__reg("a6") void *, __reg("a0") STRPTR name, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-30(a6)\n\tmovea.l\t(a7)+,a1";
#define ug_SetupContextTags(...) __ug_SetupContextTags(UserGroupBase, __VA_ARGS__)
#endif

LONG __ug_GetErr(__reg("a6") void *)="\tjsr\t-36(a6)";
#define ug_GetErr() __ug_GetErr(UserGroupBase)

STRPTR __ug_StrError(__reg("a6") void *, __reg("d1") LONG err)="\tjsr\t-42(a6)";
#define ug_StrError(err) __ug_StrError(UserGroupBase, (err))

LONG __getuid(__reg("a6") void *)="\tjsr\t-48(a6)";
#define getuid() __getuid(UserGroupBase)

LONG __geteuid(__reg("a6") void *)="\tjsr\t-54(a6)";
#define geteuid() __geteuid(UserGroupBase)

LONG __setreuid(__reg("a6") void *, __reg("d0") LONG real, __reg("d1") LONG effective)="\tjsr\t-60(a6)";
#define setreuid(real, effective) __setreuid(UserGroupBase, (real), (effective))

LONG __setuid(__reg("a6") void *, __reg("d0") LONG uid)="\tjsr\t-66(a6)";
#define setuid(uid) __setuid(UserGroupBase, (uid))

LONG __getgid(__reg("a6") void *)="\tjsr\t-72(a6)";
#define getgid() __getgid(UserGroupBase)

LONG __getegid(__reg("a6") void *)="\tjsr\t-78(a6)";
#define getegid() __getegid(UserGroupBase)

LONG __setregid(__reg("a6") void *, __reg("d0") LONG real, __reg("d1") LONG effective)="\tjsr\t-84(a6)";
#define setregid(real, effective) __setregid(UserGroupBase, (real), (effective))

LONG __setgid(__reg("a6") void *, __reg("d0") LONG gid)="\tjsr\t-90(a6)";
#define setgid(gid) __setgid(UserGroupBase, (gid))

LONG __getgroups(__reg("a6") void *, __reg("d0") LONG gidsetlen, __reg("a1") LONG * gidset)="\tjsr\t-96(a6)";
#define getgroups(gidsetlen, gidset) __getgroups(UserGroupBase, (gidsetlen), (gidset))

LONG __setgroups(__reg("a6") void *, __reg("d0") LONG gidsetlen, __reg("a1") LONG * gidset)="\tjsr\t-102(a6)";
#define setgroups(gidsetlen, gidset) __setgroups(UserGroupBase, (gidsetlen), (gidset))

LONG __initgroups(__reg("a6") void *, __reg("a1") STRPTR name, __reg("d0") LONG basegid)="\tjsr\t-108(a6)";
#define initgroups(name, basegid) __initgroups(UserGroupBase, (name), (basegid))

struct passwd * __getpwnam(__reg("a6") void *, __reg("a1") STRPTR login)="\tjsr\t-114(a6)";
#define getpwnam(login) __getpwnam(UserGroupBase, (login))

struct passwd * __getpwuid(__reg("a6") void *, __reg("d0") LONG uid)="\tjsr\t-120(a6)";
#define getpwuid(uid) __getpwuid(UserGroupBase, (uid))

VOID __setpwent(__reg("a6") void *)="\tjsr\t-126(a6)";
#define setpwent() __setpwent(UserGroupBase)

struct passwd * __getpwent(__reg("a6") void *)="\tjsr\t-132(a6)";
#define getpwent() __getpwent(UserGroupBase)

VOID __endpwent(__reg("a6") void *)="\tjsr\t-138(a6)";
#define endpwent() __endpwent(UserGroupBase)

struct group * __getgrnam(__reg("a6") void *, __reg("a1") STRPTR name)="\tjsr\t-144(a6)";
#define getgrnam(name) __getgrnam(UserGroupBase, (name))

struct group * __getgrgid(__reg("a6") void *, __reg("d0") LONG gid)="\tjsr\t-150(a6)";
#define getgrgid(gid) __getgrgid(UserGroupBase, (gid))

VOID __setgrent(__reg("a6") void *)="\tjsr\t-156(a6)";
#define setgrent() __setgrent(UserGroupBase)

struct group * __getgrent(__reg("a6") void *)="\tjsr\t-162(a6)";
#define getgrent() __getgrent(UserGroupBase)

VOID __endgrent(__reg("a6") void *)="\tjsr\t-168(a6)";
#define endgrent() __endgrent(UserGroupBase)

UBYTE * __crypt(__reg("a6") void *, __reg("a0") UBYTE * key, __reg("a1") UBYTE * set)="\tjsr\t-174(a6)";
#define crypt(key, set) __crypt(UserGroupBase, (key), (set))

UBYTE * __ug_GetSalt(__reg("a6") void *, __reg("a0") struct passwd * user, __reg("a1") UBYTE * buf, __reg("d0") ULONG size)="\tjsr\t-180(a6)";
#define ug_GetSalt(user, buf, size) __ug_GetSalt(UserGroupBase, (user), (buf), (size))

STRPTR __getpass(__reg("a6") void *, __reg("a1") STRPTR prompt)="\tjsr\t-186(a6)";
#define getpass(prompt) __getpass(UserGroupBase, (prompt))

ULONG __umask(__reg("a6") void *, __reg("d0") ULONG mask)="\tjsr\t-192(a6)";
#define umask(mask) __umask(UserGroupBase, (mask))

ULONG __getumask(__reg("a6") void *)="\tjsr\t-198(a6)";
#define getumask() __getumask(UserGroupBase)

LONG __setsid(__reg("a6") void *)="\tjsr\t-204(a6)";
#define setsid() __setsid(UserGroupBase)

LONG __getpgrp(__reg("a6") void *)="\tjsr\t-210(a6)";
#define getpgrp() __getpgrp(UserGroupBase)

STRPTR __getlogin(__reg("a6") void *)="\tjsr\t-216(a6)";
#define getlogin() __getlogin(UserGroupBase)

LONG __setlogin(__reg("a6") void *, __reg("a1") STRPTR name)="\tjsr\t-222(a6)";
#define setlogin(name) __setlogin(UserGroupBase, (name))

VOID __setutent(__reg("a6") void *)="\tjsr\t-228(a6)";
#define setutent() __setutent(UserGroupBase)

struct utmp * __getutent(__reg("a6") void *)="\tjsr\t-234(a6)";
#define getutent() __getutent(UserGroupBase)

VOID __endutent(__reg("a6") void *)="\tjsr\t-240(a6)";
#define endutent() __endutent(UserGroupBase)

struct lastlog * __getlastlog(__reg("a6") void *, __reg("d0") LONG uid)="\tjsr\t-246(a6)";
#define getlastlog(uid) __getlastlog(UserGroupBase, (uid))

LONG __setlastlog(__reg("a6") void *, __reg("d0") LONG uid, __reg("a0") STRPTR name, __reg("a1") STRPTR host)="\tjsr\t-252(a6)";
#define setlastlog(uid, name, host) __setlastlog(UserGroupBase, (uid), (name), (host))

struct UserGroupCredentials * __getcredentials(__reg("a6") void *, __reg("a0") struct Task * task)="\tjsr\t-258(a6)";
#define getcredentials(task) __getcredentials(UserGroupBase, (task))

#endif /*  _VBCCINLINE_USERGROUP_H  */
