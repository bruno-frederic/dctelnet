/* tests/shim/clib/compiler-specific.h -- no register arguments on the host. */
#ifndef CLIB_COMPILER_SPECIFIC_H
#define CLIB_COMPILER_SPECIFIC_H
#define __ASM__
#define __REG__(r, p) p
#endif
