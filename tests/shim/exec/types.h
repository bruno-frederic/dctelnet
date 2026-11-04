/* tests/shim/exec/types.h -- the few AmigaOS exec types DCTelnet.h needs, for host tests. */
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H
#include <stdint.h>
typedef uint32_t ULONG;
typedef int32_t  LONG;
typedef uint16_t UWORD;
typedef int16_t  WORD;
typedef int8_t   BYTE;
typedef uint8_t  UBYTE;
typedef int16_t  BOOL;
typedef unsigned char TEXT;
typedef void *APTR;
#define TRUE  1
#define FALSE 0
#endif
