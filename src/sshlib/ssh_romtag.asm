; sshlib/ssh_romtag.asm -- the ROM tag of ssh.library. It must be first in the
; file (link it before every other object): AmigaOS finds the library by
; scanning the first hunk for it, and there is no startup code. Same layout as
; ibmcon/ibmcon.device.bugfixed.asm (RTF_AUTOINIT), with NT_LIBRARY.
; The InitTable (base size, vectors, init routine) is in ssh_lib.c.
        xref    _SshLib_InitTable
        xdef    _SshLib_Name
        xdef    _SshLib_IdString

        SECTION "CODE",CODE
        cnop    0,4

;---------------------------------------------------------------------
; Executed if the file is started from CLI. Returns 0 (a library is not
; a program).
;---------------------------------------------------------------------
LibNull:
        moveq   #0,d0
        rts

;---------------------------------------------------------------------
; ROM tag.  RTF_AUTOINIT: rt_Init points at the InitTable, exec builds
; the library base from it and calls LibInit.
;---------------------------------------------------------------------
RomTag:
        dc.w    $4AFC                   ; RTC_MATCHWORD
        dc.l    RomTag                  ; rt_MatchTag
        dc.l    RomTagEnd               ; rt_EndOfEntry
        dc.b    $80                     ; rt_Flags = RTF_AUTOINIT
        dc.b    $01                     ; rt_Version = 1
        dc.b    $09                     ; rt_Type = NT_LIBRARY
        dc.b    $00                     ; rt_Pri = 0
        dc.l    _SshLib_Name            ; rt_Name    "ssh.library"
        dc.l    _SshLib_IdString        ; rt_IdString
        dc.l    _SshLib_InitTable       ; rt_Init (RTF_AUTOINIT table)
RomTagEnd:

_SshLib_Name:
        dc.b    "ssh.library",0
_SshLib_IdString:
        dc.b    "ssh.library 1.0 (SSH-2 client engine)",13,10,0
        even
