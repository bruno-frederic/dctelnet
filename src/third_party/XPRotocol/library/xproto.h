/** xproto.h
*
*   Include file for External Protocol Handling
*
**/
/*
*   The structure
*/
struct XPR_IO {
                STRPTR   xpr_filename;      /* File name(s)             */
                  LONG (*xpr_fopen)();      /* Open file                */
                  LONG (*xpr_fclose)();     /* Close file               */
                  LONG (*xpr_fread)();      /* Get UBYTE from file       */
                  LONG (*xpr_fwrite)();     /* Put string to file       */
                 ULONG (*xpr_sread)();      /* Get UBYTE from serial     */
                  LONG (*xpr_swrite)();     /* Put string to serial     */
                  LONG (*xpr_sflush)();     /* Flush serial input buffer*/
                  LONG (*xpr_update)();     /* Print stuff              */
                  LONG (*xpr_chkabort)();   /* Check for abort          */
                  LONG (*xpr_chkmisc)();    /* Check misc. stuff        */
                  LONG (*xpr_gets)();       /* Get string interactively */
                  LONG (*xpr_setserial)();  /* Set and Get serial info  */
                  LONG (*xpr_ffirst)();     /* Find first file name     */
                  LONG (*xpr_fnext)();      /* Find next file name      */
                  LONG (*xpr_finfo)();      /* Return file info         */
                  LONG (*xpr_fseek)();      /* Seek in a file           */
                  LONG   xpr_extension;     /* Number of extensions     */
                  LONG  *xpr_data;          /* Initialized by Setup.    */
                 ULONG (*xpr_options)();    /* Multiple XPR options.    */
                  LONG (*xpr_unlink)();     /* Delete a file.           */
                  LONG (*xpr_squery)();     /* Query serial device      */
                  LONG (*xpr_getptr)();     /* Get various host ptrs    */
              };

/*
*   Number of defined extensions
*/
#define XPR_EXTENSION 4L

/*
*   The functions
*/

LONG XProtocolCleanup(struct XPR_IO *);
LONG XProtocolSetup(struct XPR_IO *);
LONG XProtocolSend(struct XPR_IO *);
LONG XProtocolReceive(struct XPR_IO *);
LONG XProtocolHostMon(struct XPR_IO *,APTR,LONG,LONG);
LONG XProtocolUserMon(struct XPR_IO *,APTR,LONG,LONG);

#ifdef LATTICE

/* Pragmas for SAS/Lattice-C V5.0 */

#pragma libcall XProtocolBase XProtocolCleanup 1e 801
#pragma libcall XProtocolBase XProtocolSetup 24 801
#pragma libcall XProtocolBase XProtocolSend 2a 801
#pragma libcall XProtocolBase XProtocolReceive 30 801
#pragma libcall XProtocolBase XProtocolHostMon 36 109804
#pragma libcall XProtocolBase XProtocolUserMon 3c 109804

#endif	/* LATTICE */

#ifdef AZTEC_C

/* Pragmas for Manx Aztec-C V5.0 */

#pragma amicall(XProtocolBase, 0x1e, XProtocolCleanup(a0))
#pragma amicall(XProtocolBase, 0x24, XProtocolSetup(a0))
#pragma amicall(XProtocolBase, 0x2a, XProtocolSend(a0))
#pragma amicall(XProtocolBase, 0x30, XProtocolReceive(a0))
#pragma amicall(XProtocolBase, 0x36, XProtocolHostMon(a0,a1,d0,d1))
#pragma amicall(XProtocolBase, 0x3c, XProtocolUserMon(a0,a1,d0,d1))

#endif	/* AZTEC */

/*
*   Flags returned by XProtocolSetup()
*/
#define XPRS_FAILURE    0x00000000L
#define XPRS_SUCCESS    0x00000001L
#define XPRS_NORECREQ   0x00000002L
#define XPRS_NOSNDREQ   0x00000004L
#define XPRS_HOSTMON    0x00000008L
#define XPRS_USERMON    0x00000010L
#define XPRS_HOSTNOWAIT 0x00000020L
#define XPRS_NOUPDATE   0x00008000L	/* NEW! */
#define XPRS_XPR2001    0x00010000L	/* NEW! */
#define XPRS_DOUBLE     0x00020000L	/* NEW! */
/*
*   The update structure
*/
struct XPR_UPDATE {     ULONG  xpru_updatemask;
                        STRPTR xpru_protocol;
                        STRPTR xpru_filename;
                        LONG   xpru_filesize;
                        STRPTR xpru_msg;
                        STRPTR xpru_errormsg;
                        LONG   xpru_blocks;
                        LONG   xpru_blocksize;
                        LONG   xpru_bytes;
                        LONG   xpru_errors;
                        LONG   xpru_timeouts;
                        LONG   xpru_packettype;
                        LONG   xpru_packetdelay;
                        LONG   xpru_chardelay;
                        STRPTR xpru_blockcheck;
                        STRPTR xpru_expecttime;
                        STRPTR xpru_elapsedtime;
                        LONG   xpru_datarate;
                        LONG   xpru_reserved1;
                        LONG   xpru_reserved2;
                        LONG   xpru_reserved3;
                        LONG   xpru_reserved4;
                        LONG   xpru_reserved5;
                   };
/*
*   The possible bit values for the xpru_updatemask are:
*/
#define XPRU_UPLOAD             0x80000000L
#define XPRU_DNLOAD             0x40000000L
#define XPRU_PROTOCOL           0x00000001L
#define XPRU_FILENAME           0x00000002L
#define XPRU_FILESIZE           0x00000004L
#define XPRU_MSG                0x00000008L
#define XPRU_ERRORMSG           0x00000010L
#define XPRU_BLOCKS             0x00000020L
#define XPRU_BLOCKSIZE          0x00000040L
#define XPRU_BYTES              0x00000080L
#define XPRU_ERRORS             0x00000100L
#define XPRU_TIMEOUTS           0x00000200L
#define XPRU_PACKETTYPE         0x00000400L
#define XPRU_PACKETDELAY        0x00000800L
#define XPRU_CHARDELAY          0x00001000L
#define XPRU_BLOCKCHECK         0x00002000L
#define XPRU_EXPECTTIME         0x00004000L
#define XPRU_ELAPSEDTIME        0x00008000L
#define XPRU_DATARATE           0x00010000L
/*
*   The xpro_option structure
*/
struct xpr_option {
   STRPTR xpro_description;      /* description of the option                  */
   LONG   xpro_type;             /* type of option                             */
   STRPTR xpro_value;            /* pointer to a buffer with the current value */
   LONG   xpro_length;           /* buffer size                                */
};
/*
*   Valid values for xpro_type are:
*/
#define XPRO_BOOLEAN 1L         /* xpro_value is "yes", "no", "on" or "off"   */
#define XPRO_LONG    2L         /* xpro_value is string representing a number */
#define XPRO_STRING  3L         /* xpro_value is a string                     */
#define XPRO_HEADER  4L         /* xpro_value is ignored                      */
#define XPRO_COMMAND 5L         /* xpro_value is ignored                      */
#define XPRO_COMMPAR 6L         /* xpro_value contains command parameters     */
