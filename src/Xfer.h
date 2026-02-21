#ifndef XFER_H
#define XFER_H

#include <exec/types.h>

// Functions exported
void Upload(char *library);
void Download(char *library);

/* DCTelnet.c needs these XPR functions to initialize XEM library (struct XEM_IO): */

/* Get char from serial */
long __ASM__ xpr_sread(__REG__(a0, char *buffer),
                       __REG__(d0, long size),
                       __REG__(d1, long timeout));
/* Put string to serial */
long __ASM__ xpr_swrite(__REG__(a0, char *buffer),
                        __REG__(d0, long size));
/* Flush serial input buffer */
long xpr_sflush(void);
/* Get string interactively */
long __ASM__ xpr_gets(__REG__(a0, char *prompt),
                      __REG__(a1, char *buffer));

#endif /* XFER_H */
