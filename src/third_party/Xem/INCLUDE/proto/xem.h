#ifndef _PROTO_XEM_H
#define _PROTO_XEM_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_XEM_PROTOS_H) && !defined(__GNUC__)
#ifndef __SASC
#pragma stdargs-on
#endif
#include <clib/xem_protos.h>
#ifndef __SASC
#pragma stdargs-off
#endif
#endif

#ifndef __NOLIBBASE__
extern struct Library *XEmulatorBase;
#endif

#ifdef __GNUC__
#ifdef __AROS__
#include <defines/xem.h>
#else
#include <inline/xem.h>
#endif
#elif defined(__VBCC__)
#ifndef _NO_INLINE
#include <inline/xem_protos.h>
#endif
#else
#include <pragma/xem_lib.h>
#endif

#endif	/*  _PROTO_XEM_H  */
