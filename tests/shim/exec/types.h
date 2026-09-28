/* tests/shim/exec/types.h -- the few AmigaOS exec types DCTelnet.h needs, for host tests. */
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H
#include <stdint.h>
typedef uint32_t ULONG;
typedef int32_t  LONG;
typedef uint16_t UWORD;
typedef int16_t  WORD;
typedef uint8_t  UBYTE;
typedef int16_t  BOOL;
typedef unsigned char TEXT;
#define TRUE  1
#define FALSE 0
#endif
