#ifndef _PROTO_XPR_H
#define _PROTO_XPR_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_XPR_PROTOS_H) && !defined(__GNUC__)
#ifndef __SASC
#pragma stdargs-on
#endif
#include <clib/xpr_protos.h>
#ifndef __SASC
#pragma stdargs-off
#endif
#endif

#ifndef __NOLIBBASE__
extern struct Library *XProtocolBase;
#endif

#ifdef __GNUC__
#ifdef __AROS__
#include <defines/xpr.h>
#else
#include <inline/xpr.h>
#endif
#elif defined(__VBCC__)
#ifndef _NO_INLINE
#include <inline/xpr_protos.h>
#endif
#else
#include <pragma/xpr_lib.h>
#endif

#endif	/*  _PROTO_XPR_H  */
