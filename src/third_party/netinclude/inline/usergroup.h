/*
 * :ts=8
 *
 * 'Roadshow' -- Amiga TCP/IP stack; "usergroup.library" API
 * Copyright (C) 2001-2022 by Olaf Barthel.
 * All Rights Reserved.
 *
 * Amiga specific TCP/IP 'C' header files;
 * Freely Distributable
 *
 * WARNING: The "usergroup.library" API must be considered obsolete and
 *          should not be used in new software. It is provided solely
 *          for backwards compatibility and legacy application software.
 */

/*
 * This file was created with fd2pragma V2.197g using the following options:
 *
 *    fd2pragma --special 40 --infile usergroup_lib.sfd
 */

#ifndef _INLINE_USERGROUP_H
#define _INLINE_USERGROUP_H

#ifndef CLIB_USERGROUP_PROTOS_H
#define CLIB_USERGROUP_PROTOS_H
#endif

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef  LIBRARIES_USERGROUP_H
#include <libraries/usergroup.h>
#endif

#ifndef  PWD_H
#include <pwd.h>
#endif

#ifndef  GRP_H
#include <grp.h>
#endif

#ifndef USERGROUP_BASE_NAME
#define USERGROUP_BASE_NAME UserGroupBase
#endif

#define ug_SetupContextTagList(name, tags) \
	LP2(0x1e, LONG, ug_SetupContextTagList, STRPTR, name, a0, struct TagItem *, tags, a1, \
	, USERGROUP_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ug_SetupContextTags(name, tags...) \
	({ULONG _tags[] = {tags}; ug_SetupContextTagList((name), (struct TagItem *) _tags);})
#endif

#define ug_GetErr() \
	LP0(0x24, LONG, ug_GetErr, \
	, USERGROUP_BASE_NAME)

#define ug_StrError(err) \
	LP1(0x2a, STRPTR, ug_StrError, LONG, err, d1, \
	, USERGROUP_BASE_NAME)

#define getuid() \
	LP0(0x30, LONG, getuid, \
	, USERGROUP_BASE_NAME)

#define geteuid() \
	LP0(0x36, LONG, geteuid, \
	, USERGROUP_BASE_NAME)

#define setreuid(real, effective) \
	LP2(0x3c, LONG, setreuid, LONG, real, d0, LONG, effective, d1, \
	, USERGROUP_BASE_NAME)

#define setuid(uid) \
	LP1(0x42, LONG, setuid, LONG, uid, d0, \
	, USERGROUP_BASE_NAME)

#define getgid() \
	LP0(0x48, LONG, getgid, \
	, USERGROUP_BASE_NAME)

#define getegid() \
	LP0(0x4e, LONG, getegid, \
	, USERGROUP_BASE_NAME)

#define setregid(real, effective) \
	LP2(0x54, LONG, setregid, LONG, real, d0, LONG, effective, d1, \
	, USERGROUP_BASE_NAME)

#define setgid(gid) \
	LP1(0x5a, LONG, setgid, LONG, gid, d0, \
	, USERGROUP_BASE_NAME)

#define getgroups(gidsetlen, gidset) \
	LP2(0x60, LONG, getgroups, LONG, gidsetlen, d0, LONG *, gidset, a1, \
	, USERGROUP_BASE_NAME)

#define setgroups(gidsetlen, gidset) \
	LP2(0x66, LONG, setgroups, LONG, gidsetlen, d0, LONG *, gidset, a1, \
	, USERGROUP_BASE_NAME)

#define initgroups(name, basegid) \
	LP2(0x6c, LONG, initgroups, STRPTR, name, a1, LONG, basegid, d0, \
	, USERGROUP_BASE_NAME)

#define getpwnam(login) \
	LP1(0x72, struct passwd *, getpwnam, STRPTR, login, a1, \
	, USERGROUP_BASE_NAME)

#define getpwuid(uid) \
	LP1(0x78, struct passwd *, getpwuid, LONG, uid, d0, \
	, USERGROUP_BASE_NAME)

#define setpwent() \
	LP0NR(0x7e, setpwent, \
	, USERGROUP_BASE_NAME)

#define getpwent() \
	LP0(0x84, struct passwd *, getpwent, \
	, USERGROUP_BASE_NAME)

#define endpwent() \
	LP0NR(0x8a, endpwent, \
	, USERGROUP_BASE_NAME)

#define getgrnam(name) \
	LP1(0x90, struct group *, getgrnam, STRPTR, name, a1, \
	, USERGROUP_BASE_NAME)

#define getgrgid(gid) \
	LP1(0x96, struct group *, getgrgid, LONG, gid, d0, \
	, USERGROUP_BASE_NAME)

#define setgrent() \
	LP0NR(0x9c, setgrent, \
	, USERGROUP_BASE_NAME)

#define getgrent() \
	LP0(0xa2, struct group *, getgrent, \
	, USERGROUP_BASE_NAME)

#define endgrent() \
	LP0NR(0xa8, endgrent, \
	, USERGROUP_BASE_NAME)

#define crypt(key, set) \
	LP2(0xae, UBYTE *, crypt, UBYTE *, key, a0, UBYTE *, set, a1, \
	, USERGROUP_BASE_NAME)

#define ug_GetSalt(user, buf, size) \
	LP3(0xb4, UBYTE *, ug_GetSalt, struct passwd *, user, a0, UBYTE *, buf, a1, ULONG, size, d0, \
	, USERGROUP_BASE_NAME)

#define getpass(prompt) \
	LP1(0xba, STRPTR, getpass, STRPTR, prompt, a1, \
	, USERGROUP_BASE_NAME)

#define umask(mask) \
	LP1(0xc0, ULONG, umask, ULONG, mask, d0, \
	, USERGROUP_BASE_NAME)

#define getumask() \
	LP0(0xc6, ULONG, getumask, \
	, USERGROUP_BASE_NAME)

#define setsid() \
	LP0(0xcc, LONG, setsid, \
	, USERGROUP_BASE_NAME)

#define getpgrp() \
	LP0(0xd2, LONG, getpgrp, \
	, USERGROUP_BASE_NAME)

#define getlogin() \
	LP0(0xd8, STRPTR, getlogin, \
	, USERGROUP_BASE_NAME)

#define setlogin(name) \
	LP1(0xde, LONG, setlogin, STRPTR, name, a1, \
	, USERGROUP_BASE_NAME)

#define setutent() \
	LP0NR(0xe4, setutent, \
	, USERGROUP_BASE_NAME)

#define getutent() \
	LP0(0xea, struct utmp *, getutent, \
	, USERGROUP_BASE_NAME)

#define endutent() \
	LP0NR(0xf0, endutent, \
	, USERGROUP_BASE_NAME)

#define getlastlog(uid) \
	LP1(0xf6, struct lastlog *, getlastlog, LONG, uid, d0, \
	, USERGROUP_BASE_NAME)

#define setlastlog(uid, name, host) \
	LP3(0xfc, LONG, setlastlog, LONG, uid, d0, STRPTR, name, a0, STRPTR, host, a1, \
	, USERGROUP_BASE_NAME)

#define getcredentials(task) \
	LP1(0x102, struct UserGroupCredentials *, getcredentials, struct Task *, task, a0, \
	, USERGROUP_BASE_NAME)

#endif /* _INLINE_USERGROUP_H  */
