# Build Warnings Review — DCTelnet v1.6

This document provides a detailed review and analysis of the compiler warnings
generated during the DCTelnet v1.6 build process.

The goal of this review is to:
- Identify the root cause of each warning
- Determine whether the warning is benign or requires action
- Document potential fixes or justifications
- Improve long-term maintainability and build cleanliness

This analysis is based on the original DCTelnet 1.5/1.6 source code publicly available on **Aminet** and focuses on warning diagnostics only. No functional changes are covered here.

---

**Build Environment and Source References**

Built with SAS/C Amiga Compiler 6.58 under AmigaOS 3.2 (dec 2025):
- **DCTelnet 1.5 / 1.6** sources:
  [comm/tcp/dct16_src](https://aminet.net/package/comm/tcp/dct16_src)
- **ReqTools 2.9** from [util/libs/ReqToolsDev](https://aminet.net/package/util/libs/ReqToolsDev)
- **AmiTCP SDK 4.3** from [comm/tcp/AmiTCP-SDK-4.3](https://aminet.net/package/comm/tcp/AmiTCP-SDK-4.3)
- **XPRotocol v2.001** from [comm/term/xpr2001](https://aminet.net/package/comm/term/xpr2001)
- **Xem 2.0** from [comm/term/XEM2_0](https://aminet.net/package/comm/term/XEM2_0)



---

    SC NoMultipleIncludes MCConstants StructureEquivalence StringMerge NoStackCheck Define=__PROTO__ Parameters=register GST=DCTelnet.gst NoStartup Optimize CPU=68020 DCTelnet.c

    SAS/C Amiga Compiler 6.58
    Copyright (c) 1988-1995 SAS Institute Inc.
---
    ***Can't open GST file "DCTelnet.gst" for read
-> Not a problem, GST (Global Symbol Table) files are only an help to speed up compilation

    DCTelnet.c 2422 Warning 315: Static variable is unreachable "__version"
-> False alert. Tried to hide but does not work : #pragma msg 315 ignore

    DCTelnet.c 1024 Warning 317: possibly uninitialized variable "iconsig"
    DCTelnet.c 1030 Warning 317: possibly uninitialized variable "iconsig"
    DCTelnet.c 1037 Warning 317: possibly uninitialized variable "iconsig"
-> FIXME: indeed, this a real warning

    DCTelnet.c 1977 Note 306: __inline function inlined: "CheckError"
-> Note 306 is completely normal.

---

    SC NoMultipleIncludes MCConstants StructureEquivalence StringMerge NoStackCheck Define=__PROTO__ Parameters=register GST=DCTelnet.gst NoStartup Optimize CPU=68020 Xfer.c

    SAS/C Amiga Compiler 6.58
    Copyright (c) 1988-1995 SAS Institute Inc.
---
    ***Can't open GST file "DCTelnet.gst" for read
-> Not a problem, GST (Global Symbol Table) files are only an help to speed up compilation

    ====================
        xio.xpr_fopen     = xpr_fopen
    Xfer.c 639 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 STRPTR , register __a1 STRPTR )"

    ====================
        xio.xpr_fclose    = xpr_fclose
    Xfer.c 640 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 BPTR )"

    ====================
        xio.xpr_fread     = xpr_fread
    Xfer.c 641 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 APTR , register __d0 LONG , register __d1 LONG , register __a1 BPTR )"

    ====================
        xio.xpr_fwrite    = xpr_fwrite
    Xfer.c 642 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 APTR , register __d0 LONG , register __d1 LONG , register __a1 BPTR )"

    ====================
        xio.xpr_sread     = xpr_sread
    Xfer.c 643 Warning 225: pointer type mismatch
                            "unsigned long (* )()" does not match "LONG (* __asm )(register __a0 UBYTE * , register __d0 ULONG , register __d1 ULONG )"

    ====================
        xio.xpr_swrite    = xpr_swrite
    Xfer.c 644 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 UBYTE * , register __d0 LONG )"

    ====================
        xio.xpr_sflush    = xpr_sflush
    Xfer.c 645 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(void)"

    ====================
        xio.xpr_update    = xpr_update
    Xfer.c 646 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 struct XPR_UPDATE * )"

    ====================
        xio.xpr_chkabort  = xpr_chkabort
    Xfer.c 647 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(void)"

    ====================
        xio.xpr_ffirst    = xpr_ffirst
    Xfer.c 648 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 STRPTR , register __a1 STRPTR )"

    ====================
        xio.xpr_fnext     = xpr_fnext
    Xfer.c 649 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __d0 LONG , register __a0 STRPTR , register __a1 STRPTR )"

    ====================
        xio.xpr_finfo     = xpr_finfo
    Xfer.c 650 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 STRPTR , register __d0 LONG )"

    ====================
        xio.xpr_fseek     = xpr_fseek
    Xfer.c 651 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 BPTR , register __d0 LONG , register __d1 LONG )"

    ====================
        xio.xpr_gets      = xpr_gets
    Xfer.c 652 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 STRPTR , register __a1 STRPTR )"

    ====================
        xio.xpr_unlink    = xpr_unlink
    Xfer.c 653 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(register __a0 STRPTR )"

    ====================
        xio.xpr_squery    = xpr_squery
    Xfer.c 654 Warning 225: pointer type mismatch
                            "long (* )()" does not match "LONG (* __asm )(void)"
-> Not a problem.
The initialization of the "xio" struct in ProtoStart() produces this warning.
ProtoStart() fills "xio" with pointers to callback function implemented in DCTelnet Xfer.c :

    struct XPR_IO xio;
    (...)

    xio.xpr_fopen     = xpr_fopen;
    xio.xpr_fclose    = xpr_fclose;
    (...)
    xio.xpr_squery    = xpr_squery;

These callback functions are implemented in DCTelnet Xfer.C

    LONG __saveds __asm
    xpr_fopen(register __a0 STRPTR FileName,register __a1 STRPTR AccessMode)
    {  (...)

    LONG __saveds __asm
    xpr_fclose(register __a0 BPTR File)
    {  (...)

Type XPR_IO is defined in third_party\XPRotocol\library\xproto.h without specifying callback
function arguments. This cause the warnings.

    struct XPR_IO {
        STRPTR   xpr_filename;    /* File name(s)             */
        LONG (*xpr_fopen)();      /* Open file                */
        LONG (*xpr_fclose)();     /* Close file               */
        (...)
        LONG (*xpr_squery)();     /* Query serial device      */
        LONG (*xpr_getptr)();     /* Get various host ptrs    */
    };

Actually, function arguments are specified in third_party\XPRotocol\xprotocol.doc :

    long fp = (*xpr_fopen)(char *filename, char *accessmode)
    D0                     A0              A1
<BR>

    Xfer.c 444 Warning 317: possibly uninitialized variable "gs"
    Xfer.c 447 Warning 317: possibly uninitialized variable "gs"
    Xfer.c 449 Warning 317: possibly uninitialized variable "gs"
-> FIXME: indeed, this a real warning (and there is a bug in save of the address book)

---

    SC NoMultipleIncludes MCConstants StructureEquivalence StringMerge NoStackCheck Define=__PROTO__ Parameters=register GST=DCTelnet.gst NoStartup Optimize CPU=68020 Guis.c

    SAS/C Amiga Compiler 6.58
    Copyright (c) 1988-1995 SAS Institute Inc.
---
    ***Can't open GST file "DCTelnet.gst" for read
-> Not a problem, GST (Global Symbol Table) files are only an help to speed up compilation

    guis.c 391 Note 306: __inline function inlined: "OpenProject0Window"
    guis.c 696 Note 306: __inline function inlined: "OpenProject1Window"
    guis.c 1134 Note 306: __inline function inlined: "OpenProject2Window"
-> Note 306 is completely normal.

    guis.c 524 Warning 317: possibly uninitialized variable "conbook"
    guis.c 527 Warning 317: possibly uninitialized variable "conbook"
    guis.c 529 Warning 317: possibly uninitialized variable "conbook"
    guis.c 787 Warning 317: possibly uninitialized variable "ret"
-> FIXME: indeed, this a real warning (and there is a bug in save of the address book)

---

    SC NoMultipleIncludes MCConstants StructureEquivalence StringMerge NoStackCheck Define=__PROTO__ Parameters=register GST=DCTelnet.gst NoStartup Optimize CPU=68020 Connect.c

    SAS/C Amiga Compiler 6.58
    Copyright (c) 1988-1995 SAS Institute Inc.
---
    ***Can't open GST file "DCTelnet.gst" for read
-> Not a problem, GST (Global Symbol Table) files are only an help to speed up compilation

---

    SLINK "c.o" DCTelnet.o Xfer.o Guis.o Connect.o TO DCTelnet LIB LIB:sc.lib SC SD NOICONS STRIPDEBUG
    Slink - Version 6.58
    Copyright (c) 1988-1995 SAS Institute, Inc.  All Rights Reserved.

    SLINK Complete - Maximum code size = 39756 ($00009b4c) bytes
    Final output file size = 34736 ($000087b0) bytes
