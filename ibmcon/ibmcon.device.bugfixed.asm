;=====================================================================
; ibmcon.device 1.4 (Mar  9 1998) -- readable, annotated reconstruction
;=====================================================================
;
; Source provenance
; -----------------
; This file is a restructured and fully commented version of an
; "In_Go Reassembler" dump of the binary ibmcon.device (16.12.2022).
; The original symbol names were lost when the binary was built; every
; name in this file is a reconstruction from behaviour.  The code was
; originally written in C and compiled with SAS/C (the runtime helper
; functions, the autoinit/autoterm machinery and the calling
; conventions are unmistakable SAS/C artifacts).
;
; This file matches the reassembled dump instruction for instruction
; EXCEPT for a set of deliberate bug fixes, each marked in the code
; with a "FIXED:" comment (list below).
; It is NOT guaranteed to assemble to a byte-identical binary
; (instruction sizes may differ where the assembler picks another
; encoding), but it is semantically identical.
;
; What the device is
; ------------------
; An Exec device that emulates an IBM-PC style ANSI (ANSI.SYS) text
; console inside an Intuition window.  A client opens it with
;
;     OpenDevice("ibmcon.device", unit, ioreq, flags)
;
; where ioreq->io_Data must point to an already-open struct Window.
; The device then spawns a handler process ("IBMCON_Handler") that
; owns all rendering state.  The only supported command is CMD_WRITE
; (io_Data = text, io_Length = byte count or -1 for a NUL-terminated
; string).  The written stream is interpreted like an ANSI.SYS
; console: CSI (ESC [ or the 8-bit $9B) control sequences, IBM control
; characters, and SO/SI switching into the "upper" (high-bit) half of
; the font, i.e. the CP437 graphics characters of an IBM font.
; Rendering is done directly into the window's RastPort with
; graphics.library (Text, ScrollRaster, ClipBlit, RectFill...).
;
; The unit number selects small behaviour variations:
;   unit 0   : Amiga pen numbering used as-is for SGR colours
;   unit >= 1: pens 1 and 7 are swapped in SGR colour selection
;              (maps IBM colour order onto a standard Amiga palette)
;   (the original also gave units >= 2 a special two-line scroll at
;   the bottom margin -- removed, see fix 10)
;
; Process / data architecture
; ---------------------------
;   ROM tag (RTF_AUTOINIT) -> DevInit builds the "root" device base.
;   The device base is:  [LVO jump table $24][Library node + device
;   fields $3C][global data area $174].  The global data area (called
;   "globals", always addressed through A4 = base+$3C) is initialised
;   from the GlobalsInit blob in the data section.
;
;   DevOpen does NOT share the root: every successful OpenDevice gets
;   a full private CLONE of the device base (jump table + node +
;   globals), so every opener has its own window, its own handler
;   process and its own library bases.  A one-entry relocation table
;   (see GlobalsRelocs) fixes up the one absolute pointer inside the
;   cloned globals.  io_Device is repointed at the clone, so all
;   further DoIO/CloseDevice calls of this client go through the
;   clone's jump table.
;
;   The handler process receives IORequests through a private message
;   port (g_CmdPort).  DevBeginIO forwards CMD_WRITE requests to that
;   port; the handler parses the stream and replies.  DevClose sends
;   the private command CMD_DIE ($7FF0) to make the handler exit, then
;   frees the clone.
;
;   The complete console state (cursor, attributes, tab stops, pending
;   text run, CSI parser state) lives in one struct ("con", $20A
;   bytes) allocated on the handler process's STACK.
;
; SAS/C autoinit ("__STI/__STD") mechanism -- the $FC byte trap
; -------------------------------------------------------------
; The globals contain a constructor list (open graphics/intuition/dos)
; and a destructor list (close them) with a shared cursor variable at
; globals+$120 (g_AutoCursor).  Its initial value is $000000FC; the
; relocation applied to every clone turns that into
; "&globals + $FC" == address of g_AutoRunner.  RunAutoList always
; pre-increments the cursor by 4 before dereferencing, so the first
; run walks $100..$108 (constructors, stops at the NULL at $10C) and
; the second run continues at $110..$118 (destructors).  In the
; original dump this initial $FC byte was hiding as an unprintable
; character glued to the front of the "con:10/10/320/80/" string.
;
; Bugs in the original binary -- FIXED in this version
; ----------------------------------------------------
; Each fix is marked with a "FIXED:" comment at the changed code.
; 1. CSI 'S' was a byte-identical copy of 'T' (both scrolled DOWN).
;    'S' now scrolls the contents up.               (Csi_S_ScrollUp)
; 2. SetSoftStyle was called with enable mask 1, so the italic bit
;    never reached the rendering.  Mask is now 5 =
;    FSF_UNDERLINED|FSF_ITALIC.    (Csi_m_SetGraphics, Csi_u_Restore)
; 3. All erasing used ClipBlit minterm $00 / SetRast(0), i.e. always
;    colour 0.  Erasing now fills with the current SGR background pen
;    (matches what ScrollRaster reveals). (ClearRect, ClearScreenHome)
; 4. Text was positioned at "cell bottom - 2"; now uses the font's
;    real tf_Baseline.                                    (FlushText)
; 5. UnitOpen reported success even when the handler process failed
;    to create its command port, and leaked the temporary reply port.
;    Now always frees the port and returns failure.        (UnitOpen)
; 6. DevOpen never touched io_Error and left io_Device dangling on
;    failure.  Now: io_Error = IOERR_OPENFAIL and io_Device = NULL on
;    failure, io_Error = 0 on success.                      (DevOpen)
; 7. CreatePort_ zero-extended the AllocSignal result, so a -1
;    failure read as signal 255 and slipped through.  Now sign-
;    extended and tested with bmi.                      (CreatePort_)
; 8. Windows wider than the 200-byte tab-stop map / text-run buffer
;    overflowed the handler stack.  Columns are now clamped to
;    MAX_COLS, the tab map is initialised one byte longer (HT probes
;    index "cols"), and ConWrite flushes a text run that reaches the
;    buffer size.                                 (ConInit, ConWrite)
; 9. CSI parameters accumulated in bytes and wrapped mod 256 (e.g.
;    "260" read as 4).  Accumulation now saturates at 255. (ConWrite)
; 10. Units >= 2 moving past the bottom row scrolled TWO lines and
;     parked the cursor on the second to last row.  Special case
;     removed: all units scroll by the overshoot.       (CursorDownN)
; 11. Crossing the top margin from a row > 1 scrolled one line too
;     few (N-row instead of N-row+1).  Now folded into the fix-13
;     region math as "margin - target".                   (CursorUpN)
; 12. CSI 'J' mode 1 erased the left part of the cursor line twice.
;                                                 (Csi_J_EraseDisplay)
; 13. CSI 'r' ignored the top margin and simply set the row count
;     from the bottom parameter.  Now a real DECSTBM vertical scroll
;     region: con_RegTop/con_RegBot, validated (top < bottom, bottom
;     clamped to the window, invalid regions ignored), cursor homed
;     on success.  The region bounds the margin scrolling in
;     CursorDownN/CursorUpN -- and therefore LF and line wrapping.
;     A cursor positioned outside the region clamps instead of
;     scrolling.  CSI 't' (row count change) resets the region.
;     NOT region-aware, as in the ANSI.SYS era: IL/DL, SU/SD and the
;     erase commands still operate on the whole window.
;
; Supported control codes
; -----------------------
;   $07 BEL  DisplayBeep          $0E SO  select IBM upper charset
;   $08 BS   cursor left           $0F SI  back to lower charset
;   $09 HT   next tab stop         $18 CAN / $1A SUB  abort sequence
;   $0A LF   down (CR+down if LNM) $1B ESC  start sequence
;   $0B VT   cursor UP one line    $9B CSI  8-bit CSI
;   $0C FF   clear screen + home
;   $0D CR   column 1
;
; Supported CSI sequences (prefix char, if any, before final letter):
;   @ insert chars     A up           B down          C right
;   D left             E next line    F previous line H / f goto row;col
;   J erase display    K erase line   L insert lines  M delete lines
;   P delete chars     R (stub)       S scroll up
;   T scroll down      r set scroll region (DECSTBM top;bottom)
;   t set rows from 1st parameter     n (stub)
;   s save cursor+attrs               u restore cursor+attrs
;   h set mode:   20 = LNM (LF implies CR)
;                 >1 = scroll at margins    ?7 = auto-wrap
;   l reset mode: same codes
;   m SGR: 0 reset, 1 bold (bright pens / pen 3 on 4-colour screens),
;          3 italic, 4 underline, 7 reverse, 23/24 italic/underline
;          off, 30-37/39 foreground, 40-47/49 background
;
;=====================================================================

        MC68020                         ; reassembler artifact -- the code
Optimize68020 EQU 1                     ; itself is plain 68000

;---------------------------------------------------------------------
; Skip-next-word trick.
; $0C40 is the opcode of "cmpi.w #<imm>,d0"; executed in-line it
; swallows the following one-word instruction as its immediate.
; SAS/C uses it as a branchless skip (e.g. loop entry that skips the
; first "move.b (An)+,(An)+", or "moveq #1,d0 / SKIPNEXT / moveq #0,d0").
;---------------------------------------------------------------------
SKIPNEXT MACRO
        dc.w    $0C40
        ENDM

;---------------------------------------------------------------------
; Exec
;---------------------------------------------------------------------
AbsExecBase         EQU 4

_LVOForbid          EQU -$84
_LVOAllocMem        EQU -$C6
_LVOFreeMem         EQU -$D2
_LVORemove          EQU -$FC
_LVOFindTask        EQU -$126
_LVOAllocSignal     EQU -$14A
_LVOFreeSignal      EQU -$150
_LVOAddPort         EQU -$162
_LVORemPort         EQU -$168
_LVOPutMsg          EQU -$16E
_LVOGetMsg          EQU -$174
_LVOReplyMsg        EQU -$17A
_LVOWaitPort        EQU -$180
_LVOCloseLibrary    EQU -$19E
_LVORawDoFmt        EQU -$20A
_LVOOpenLibrary     EQU -$228
_LVOCacheClearU     EQU -$27C    ; V36+

MEMF_PUB_CLEAR      EQU $10001   ; MEMF_PUBLIC|MEMF_CLEAR

; struct Node / Library / Message / MsgPort ---------------------------
LN_TYPE             EQU $08
LN_PRI              EQU $09
LN_NAME             EQU $0A
NT_DEVICE           EQU 3
NT_MSGPORT          EQU 4
NT_MESSAGE          EQU 5

LIB_FLAGS           EQU $0E      ; bit 3 = LIBF_DELEXP
LIB_NEGSIZE         EQU $10
LIB_POSSIZE         EQU $12
LIB_VERSION         EQU $14
LIB_REVISION        EQU $16
LIB_IDSTRING        EQU $18
LIB_OPENCNT         EQU $20

MN_REPLYPORT        EQU $0E
MN_LENGTH           EQU $12

MP_FLAGS            EQU $0E
MP_SIGBIT           EQU $0F
MP_SIGTASK          EQU $10
MP_MSGLIST          EQU $14
MP_SIZE             EQU $22

; struct IOStdReq -----------------------------------------------------
IO_DEVICE           EQU $14
IO_UNIT             EQU $18
IO_COMMAND          EQU $1C
IO_FLAGS            EQU $1E     ; bit 0 = IOF_QUICK
IO_ERROR            EQU $1F
IO_ACTUAL           EQU $20
IO_LENGTH           EQU $24
IO_DATA             EQU $28

CMD_WRITE           EQU 3
CMD_DIE             EQU $7FF0   ; private: DevClose -> handler "exit"
; 1.5: a client hands over the 16 screen pens for the ANSI colours:
; io_Data -> UBYTE pens[16] in ANSI order (0 black .. 7 white, 8-15 bright).
; Colour codes then stay logical and are mapped at every pen that reaches
; graphics.library; the unit 1<->7 swap and the depth checks no longer apply.
IBMCMD_SETPENS      EQU $7FE0
rp_Mask             EQU $18
IOERR_OPENFAIL      EQU -1
IOERR_NOCMD         EQU $FD     ; -3 as a byte

; struct Process ------------------------------------------------------
pr_MsgPort          EQU $5C
pr_Result2          EQU $94

ERROR_INVALID_RESIDENT_LIBRARY EQU 122  ; written to pr_Result2 on lib fail

;---------------------------------------------------------------------
; dos.library
;---------------------------------------------------------------------
_LVODosOpen         EQU -$1E
_LVODosClose        EQU -$24
_LVODosWrite        EQU -$30
_LVOOutput          EQU -$3C
_LVODelay           EQU -$C6
_LVOCreateNewProc   EQU -$1F2

MODE_NEWFILE        EQU $3EE    ; 1006

NP_Entry            EQU $800003EB
NP_StackSize        EQU $800003F3
NP_Name             EQU $800003F4

;---------------------------------------------------------------------
; graphics.library
;---------------------------------------------------------------------
_LVOText            EQU -$3C
_LVOSetSoftStyle    EQU -$5A
_LVOSetRast         EQU -$EA
_LVOGfxMove         EQU -$F0
_LVORectFill        EQU -$132
_LVOSetAPen         EQU -$156
_LVOSetBPen         EQU -$15C
_LVOGetBitMapAttr   EQU -$3C0   ; V39 (1.5: the depth of RTG bitmaps)
BMA_DEPTH           EQU 4
_LVOSetDrMd         EQU -$162
_LVOScrollRaster    EQU -$18C
_LVOClipBlit        EQU -$228

; SetDrMd modes
JAM2                EQU 1
COMPLEMENT          EQU 2
JAM2_INVERS         EQU 5       ; JAM2|INVERSVID -- reverse video text

;---------------------------------------------------------------------
; intuition.library
;---------------------------------------------------------------------
_LVODisplayBeep     EQU -$60

; struct Window -------------------------------------------------------
wd_Width            EQU $08
wd_Height           EQU $0A
wd_Flags            EQU $18     ; ULONG; WFLG_GIMMEZEROZERO $400 = bit 2 of byte +2
wd_GZZWidth         EQU $70     ; drawable size of a GimmeZeroZero window
wd_GZZHeight        EQU $72
wd_WScreen          EQU $2E
wd_RPort            EQU $32

; struct Screen: sc_ViewPort($2C) + vp_RasInfo($24) = $50
sc_vpRasInfo        EQU $50
ri_BitMap           EQU $04
bm_Depth            EQU $05

; struct RastPort / TextFont ------------------------------------------
rp_Font             EQU $34
tf_YSize            EQU $14     ; character cell height in pixels
tf_XSize            EQU $18     ; character cell width in pixels
tf_Baseline         EQU $1A     ; pixels from cell top to the baseline

; soft style bits (graphics/text.h)
FSF_UNDERLINED      EQU 1
FSF_ITALIC          EQU 4
STYLE_ENABLE        EQU FSF_UNDERLINED|FSF_ITALIC ; SetSoftStyle mask

;---------------------------------------------------------------------
; Device base layout
;
;   base-$24 .. base-1 : LVO jump table (6 vectors x 6 bytes)
;   base+$00 .. $21    : struct Library
;   base+$22 .. $3B    : device specific fields
;   base+$3C .. $1AF   : globals (initialised from GlobalsInit)
;---------------------------------------------------------------------
dev_SegList         EQU $22     ; SegList passed to DevInit
dev_RelocTab        EQU $2E     ; root only: heap copy of GlobalsRelocs
dev_Root            EQU $32     ; -> root device base (self in the root)
dev_NegSize         EQU $36     ; size of the LVO table ($24)
dev_Globals         EQU $3C     ; start of the globals area

DEV_POSSIZE         EQU $1B0    ; Library + fields + globals
DEV_NEGSIZE         EQU $24     ; 6 LVOs
GLOBALS_SIZE        EQU $174
GLOBALS_INITLONGS   EQU $4E     ; longwords of GlobalsInit that get copied
                                ; ($4E*4 = $138 bytes)

;---------------------------------------------------------------------
; Globals (always addressed via A4 = device base + dev_Globals).
; $00-$137 are copied from GlobalsInit; $138-$173 start out zeroed
; and are filled at run time.
;---------------------------------------------------------------------
g_EscTable          EQU $2C     ; CSI dispatch table (6-byte entries)
g_ParamOne1         EQU $E0     ; six private {value 1, NUL} parameter
g_ParamOne2         EQU $E2     ; blocks -- passed as fake "params"
g_ParamOne3         EQU $E4     ; pointer when internal code reuses the
g_ParamOne4         EQU $E6     ; CSI handlers for single-step cursor
g_ParamOne5         EQU $E8     ; movement
g_ParamOne6         EQU $EA
g_ProcName          EQU $EC     ; "IBMCON_Handler"
g_AutoRunner        EQU $FC     ; -> RunAutoList
g_CtorList          EQU $100    ; NULL terminated constructor pointers
g_DtorList          EQU $110    ; NULL terminated destructor pointers
g_AutoCursor        EQU $120    ; SAS/C __STI/__STD cursor (see header)
g_ConSpec           EQU $124    ; "con:10/10/320/80/" error window spec
g_OwnConsole        EQU $138    ; <>0: error printer opens its own window
                                ; (always 0 at run time in this device)
g_Window            EQU $144    ; struct Window * (from io_Data)
g_RastPort          EQU $148    ; window->RPort
g_CmdPort           EQU $14C    ; handler's command MsgPort
g_UnitNum           EQU $150    ; unit number from OpenDevice
g_AutoInitDone      EQU $154    ; byte: constructor pass completed
g_LibVersion        EQU $158    ; OpenLibrary version (stays 0 = any)
g_DOSBase           EQU $15C
g_DOSBaseC          EQU $160    ; duplicate kept for the destructor
g_GfxBase           EQU $164
g_GfxBaseC          EQU $168
g_IntuiBase         EQU $16C
g_IntuiBaseC        EQU $170

;---------------------------------------------------------------------
; Console state ("con"), $20A bytes, lives on the handler's stack.
; All CSI handlers receive (con, params, paramcount) as C stack args.
;---------------------------------------------------------------------
con_RastPort        EQU $08     ; window RastPort (copy)
con_EscPending      EQU $14     ; word: ESC seen, waiting for '['
con_InCsi           EQU $16     ; word: inside CSI (holds the final
                                ;   character while a handler runs)
con_ParamIdx        EQU $1A     ; long: current parameter index (0..23)
con_Rows            EQU $1E     ; text rows    (window height / YSize)
con_Cols            EQU $20     ; text columns (window width / XSize)
con_WidthPx         EQU $22     ; window width in pixels
con_RegTop          EQU $24     ; DECSTBM scroll region top row
con_RegBot          EQU $26     ; DECSTBM scroll region bottom row
                                ; (fix 13 -- lives in a struct hole
                                ; the original left unused)
con_Col             EQU $28     ; cursor column, 1-based
con_Row             EQU $2A     ; cursor row,    1-based
con_Attrs           EQU $2C     ; long attribute state:
con_AttrFlags       EQU $2C     ;   byte: bit4 bold, bit5 reverse
con_SoftStyle       EQU $2F     ;   byte: bit0 underline, bit2 italic
                                ;   (word $2E/$2F is fed to SetSoftStyle)
con_FgPen           EQU $30
con_BgPen           EQU $32
con_SavedCol        EQU $34     ; CSI 's' snapshot ...
con_SavedRow        EQU $36
con_SavedAttrs      EQU $38
con_SavedFg         EQU $3C
con_SavedBg         EQU $3E     ; ... restored by CSI 'u'
con_DefaultPen      EQU $40     ; default foreground pen (depth based)
con_BoldPens        EQU $42     ; <>0: bold allowed (bright pens/pen 3)
con_FixedPen        EQU $44     ; <>0: SGR colour changes are ignored
con_Modes           EQU $46     ; current mode word, low byte:
con_ModeFlags       EQU $47     ;   bit0 LNM (LF implies CR)  [h/l 20]
                                ;   bit1 auto-wrap            [?7]
                                ;   bit2 scroll at margins    [>1]
con_SavedModes      EQU $48     ; 's'/'u' snapshot of con_Modes
con_InitModes       EQU $4A     ; initial mode word (set by handler)
con_TextCol         EQU $4E     ; column where the pending text starts
con_TextLen         EQU $50     ; pending text length (0 = none)
con_RawCnt          EQU $52     ; chars collected in con_RawBuf
con_TextBuf         EQU $54     ; TEXTBUF_SIZE bytes pending text run

TEXTBUF_SIZE        EQU $C8     ; capacity of con_TextBuf (200)
MAX_COLS            EQU 199     ; column clamp: tab map is 200 bytes
                                ; and HT probes index "cols"
con_TabStops        EQU $11C    ; one byte per column, 1 = tab stop
con_Params          EQU $1E4    ; 24 parameter BYTES
con_RawBuf          EQU $1FC    ; up to 10 raw chars + NUL ('>','?',...)
con_CharMask        EQU $207    ; OR-mask for printables ($80 after SO)
con_SavedMask       EQU $208    ; 's'/'u' snapshot of con_CharMask
                                ; 1.5: the console struct grows by $12
con_PenMap          EQU $20A    ; 16 bytes: logical ANSI pen -> screen pen
con_PenMapOn        EQU $21A    ; word: <>0 once a client sent IBMCMD_SETPENS
con_HeightPx        EQU $21C    ; drawable height when the grid was measured
con_WrapPending     EQU $21E    ; 1.5: word, <>0 = a character filled the last
                                ;   column; the next printable wraps first
CON_FRAME           EQU $226    ; handler stack frame holding con (was $210)

;=====================================================================
        SECTION "Segment0",CODE
        cnop    0,4
;=====================================================================

;---------------------------------------------------------------------
; DevNull -- executed when the file is started from CLI, and also the
; reserved 4th device vector (ExtFunc).  Returns 0.
;---------------------------------------------------------------------
DevNull:                                ; was SegmentBeginn0
        moveq   #0,D0
        rts

;---------------------------------------------------------------------
; ROM tag.  RTF_AUTOINIT: rt_Init points at the InitTable, exec builds
; the device base and calls DevInit.
;---------------------------------------------------------------------
RomTag:                                 ; was AL_0_4
        dc.w    $4AFC                   ; RTC_MATCHWORD
        dc.l    RomTag                  ; rt_MatchTag
        dc.l    RomTagEnd               ; rt_EndOfEntry
        dc.b    $80                     ; rt_Flags = RTF_AUTOINIT
        dc.b    $01                     ; rt_Version = 1
        dc.b    $03                     ; rt_Type = NT_DEVICE
        dc.b    $00                     ; rt_Pri = 0
        dc.l    DevName                 ; rt_Name    "ibmcon.device"
        dc.l    DevIdString             ; rt_IdString
        dc.l    InitTable               ; rt_Init (RTF_AUTOINIT table)
RomTagEnd:                              ; was AL_0_1E
        ds.w    1

;---------------------------------------------------------------------
; DevInit -- RTF_AUTOINIT init function.
; In:  D0 = freshly made device base (zeroed), A0 = SegList, A6 = exec
; Out: D0 = device base or 0 on failure
;
; Fills in the Library node, copies GlobalsInit into base+$3C, and
; keeps a heap copy of the relocation table (GlobalsRelocs) in
; dev_RelocTab -- DevOpen applies it to every clone.  The relocation
; is NOT applied to the root: the root's globals are never used for
; console work.
;---------------------------------------------------------------------
DevInit:                                ; was AJL_0_20
        subq.w  #8,A7
        movem.l D7/A2-A3/A5-A6,-(A7)
        movea.l D0,A5                   ; A5 = device base
        move.l  A0,dev_SegList(A5)
        move.b  #NT_DEVICE,LN_TYPE(A5)
        move.l  #DevName,LN_NAME(A5)
        move.b  #6,LIB_FLAGS(A5)        ; LIBF_SUMUSED|LIBF_CHANGED
        lea     1,A1                    ; (SAS/C constant-via-lea)
        move.l  A1,D0
        lea     LIB_VERSION(A5),A3
        move.w  D0,(A3)+                ; lib_Version  = 1
        lea     6,A1                    ; 1.6: revision 6 (deferred wrap)
        move.l  A1,D0
        move.w  D0,(A3)+                ; lib_Revision = 6
        move.l  #DevIdString,(A3)+      ; lib_IdString
        lea     dev_RelocTab(A5),A3
        clr.l   (A3)+                   ; dev_RelocTab = NULL
        move.l  A5,(A3)+                ; dev_Root     = self
        move.l  #DEV_NEGSIZE,(A3)+      ; dev_NegSize  = $24
        lea     dev_Globals(A5),A1
        lea     GLOBALS_INITLONGS,A2
        move.l  A2,D0                   ; D0 = $4E
        move.l  D0,D1
        asl.l   #2,D1                   ; D1 = $138 bytes to copy
        move.l  A6,$18(A7)              ; save exec base
        lea     GlobalsInit,A2
        SKIPNEXT                        ; enter loop at the subq
.copyGlobals:
        move.b  (A2)+,(A1)+             ; copy $138 bytes GlobalsInit
        subq.l  #1,D1                   ;   -> base+dev_Globals
        bcc.b   .copyGlobals
        asl.l   #2,D0                   ; D0 = $138
        lea     GlobalsInit,A3
        adda.l  D0,A3                   ; A3 = GlobalsRelocs (= init+$138)
        move.l  (A3),D0                 ; D0 = relocation entry count
        movea.l $18(A7),A6
        beq.b   .noRelocs               ; none? done
        move.l  D0,D7
        asl.l   #2,D7
        addq.l  #4,D7                   ; D7 = count*4+4 bytes
        move.l  D7,D0
        moveq   #1,D1                   ; MEMF_PUBLIC
        movea.l AbsExecBase.W,A6
        jsr     _LVOAllocMem(A6)
        move.l  D0,dev_RelocTab(A5)     ; keep heap copy for DevOpen
        movea.l $18(A7),A6
        beq.b   .fail                   ; no memory -> return 0
        movea.l D0,A1
        SKIPNEXT
.copyRelocs:
        move.b  (A3)+,(A1)+             ; copy the reloc table
        subq.l  #1,D7
        bcc.b   .copyRelocs
.noRelocs:
        move.l  A5,D0                   ; success: return device base
.fail:
        movem.l (A7)+,D7/A2-A3/A5-A6
        addq.w  #8,A7
        rts

;---------------------------------------------------------------------
; DevOpen -- device Open vector.
; In:  A6 = root base, A1 = ioreq (io_Data = Window *), D0 = unit,
;      D1 = flags
;
; Allocates a full private clone of the device base (jump table +
; node + globals), applies the globals relocation, repoints io_Device
; at the clone, opens the libraries (constructor list) and calls
; UnitOpen to spawn the handler process.
; Success: D0 = clone, io_Error = 0.
; Failure: D0 = 0, io_Error = IOERR_OPENFAIL, io_Device = NULL.
;
; Stack frame ($18 bytes above the movem):
;   $1C = exec base   $20 = raw allocation   $24 = ioreq
;   $28 = entry base  $2C = entry base (scratch copy)
;---------------------------------------------------------------------
DevOpen:                                ; was AJL_0_D0
        suba.w  #$18,A7
        movem.l D2/D7/A2-A3/A5-A6,-(A7)
        move.l  D0,D7                   ; D7 = unit number
        lea     $24(A7),A2
        move.l  A1,(A2)+                ; $24 = ioreq
        movea.l AbsExecBase.W,A1
        movea.l A6,A5                   ; A5 = base OpenDevice found
        addq.w  #1,LIB_OPENCNT(A5)
        bclr    #3,LIB_FLAGS(A5)        ; clear LIBF_DELEXP
        movea.l dev_Root(A5),A0
        move.l  A1,$1C(A7)              ; $1C = exec base
        lea     GLOBALS_SIZE,A1
        move.l  A1,D0
        add.l   dev_NegSize(A0),D0
        moveq   #dev_Globals,D1
        add.l   D1,D0                   ; size = $174+$24+$3C
        move.l  A6,(A2)+                ; $28 = base
        move.l  A6,(A2)+                ; $2C = base
        move.l  #MEMF_PUB_CLEAR,D1
        movea.l AbsExecBase.W,A6
        jsr     _LVOAllocMem(A6)
        movea.l D0,A3                   ; A3 = raw clone block
        move.l  D0,$20(A7)
        movea.l $2C(A7),A6
        beq.w   .noMem
        movea.l dev_Root(A5),A0
        movea.l A5,A1
        move.l  dev_NegSize(A0),D1
        suba.l  D1,A1                   ; A1 = root - negsize (source)
        lea     GLOBALS_INITLONGS,A6
        move.l  A6,D0
        asl.l   #2,D0                   ; $138 initialised globals bytes
        add.l   D1,D0                   ; + jump table
        moveq   #dev_Globals,D1
        add.l   D1,D0                   ; + node/fields = $198 bytes
        movea.l A3,A0
        SKIPNEXT
.copyBase:
        move.b  (A1)+,(A0)+             ; clone the whole device base
        subq.l  #1,D0
        bcc.b   .copyBase
        movea.l dev_Root(A5),A0
        movea.l A3,A2
        adda.l  dev_NegSize(A0),A2      ; A2 = clone base (after LVOs)
        clr.l   dev_RelocTab(A2)        ; clone owns no reloc table
        lea     dev_Globals(A2),A3      ; A3 = clone globals
        movea.l dev_Root(A2),A0
        movea.l dev_RelocTab(A0),A1     ; root's heap reloc table
        movea.l $2C(A7),A6
        move.l  A1,D1
        beq.b   .relocDone
        lea     4(A1),A5                ; A5 = first entry
        move.l  (A1),D2                 ; D2 = entry count
        bra.b   .relocNext
.relocLoop:
        move.l  A3,D0                   ; base = clone globals
        move.l  (A5)+,D1                ; entry = offset into globals
        add.l   D0,D1
        movea.l D1,A0
        add.l   D0,(A0)                 ; *(globals+off) += globals
        subq.l  #1,D2                   ;   (turns g_AutoCursor's $FC
.relocNext:                             ;    into &globals+$FC)
        tst.l   D2
        bgt.b   .relocLoop
.relocDone:
        movea.l $1C(A7),A0              ; exec base
        cmpi.w  #36,LIB_VERSION(A0)     ; V36+ ?
        movea.l $2C(A7),A6
        bcs.b   .noCacheFlush
        movea.l AbsExecBase.W,A6
        jsr     _LVOCacheClearU(A6)     ; copied code -> flush caches
.noCacheFlush:
        movea.l $24(A7),A5              ; A5 = ioreq
        movea.l A2,A6                   ; A6 = clone
        move.l  A6,IO_DEVICE(A5)        ; client now talks to the clone
        bsr.w   RunConstructors         ; open gfx/intuition/dos
        movea.l $2C(A7),A6
        tst.l   D0
        bne.b   .fail
        move.l  D7,D0                   ; unit number
        movea.l A5,A0                   ; ioreq
        movea.l A2,A6                   ; clone
        bsr.w   UnitOpen                ; spawn handler process
        tst.l   D0
        movea.l $2C(A7),A6
        beq.b   .done                   ; 0 = success -> return clone
.fail:
        movea.l A2,A6
        bsr.w   RunDestructors          ; close libraries again
        movea.l dev_Root(A2),A0
        lea     GLOBALS_SIZE,A1
        move.l  A1,D0
        add.l   dev_NegSize(A0),D0
        moveq   #dev_Globals,D1
        add.l   D1,D0
        movea.l $20(A7),A1              ; raw clone block
        movea.l AbsExecBase.W,A6
        jsr     _LVOFreeMem(A6)
        movea.l $2C(A7),A6
.noMem:
        movea.l $28(A7),A1              ; base OpenDevice was called on
        movea.l dev_Root(A1),A0
        subq.w  #1,LIB_OPENCNT(A0)      ; undo the count
        movea.l $24(A7),A1              ; FIXED: report the failure in
        clr.l   IO_DEVICE(A1)           ;   the ioreq -- no dangling
        move.b  #IOERR_OPENFAIL,IO_ERROR(A1) ; device pointer, io_Error
        moveq   #0,D0                   ;   set for the caller
        bra.b   .exit
.done:
        movea.l $24(A7),A1
        clr.b   IO_ERROR(A1)            ; FIXED: explicit success
        move.l  A2,D0                   ; return the clone
.exit:
        movem.l (A7)+,D2/D7/A2-A3/A5-A6
        adda.w  #$18,A7
        rts

;---------------------------------------------------------------------
; DevClose -- device Close vector.
; In: A6 = clone base (or root), A1 = ioreq.  Out: D0 = SegList or 0.
;
; For a clone: tell the handler process to die (UnitClose), close the
; libraries, free the clone, then decrement the ROOT open count.  If
; the count hits 0 and LIBF_DELEXP was set, expunge immediately.
;---------------------------------------------------------------------
DevClose:                               ; was AJL_0_20E
        subq.w  #8,A7
        movem.l D7/A3/A5-A6,-(A7)
        movea.l A6,A5                   ; A5 = base being closed
        moveq   #0,D7                   ; default result: 0
        move.l  A6,$14(A7)
        movea.l dev_Root(A5),A0
        cmpa.l  A5,A0
        beq.b   .decCount               ; base IS the root: no teardown
        movea.l A1,A0                   ; A0 = ioreq
        movea.l A5,A6
        bsr.w   UnitClose               ; send CMD_DIE to the handler
        bsr.w   RunDestructors          ; close gfx/intuition/dos
        movea.l dev_Root(A5),A3
        move.l  dev_NegSize(A3),D0
        suba.l  D0,A5                   ; A5 = raw clone block
        lea     GLOBALS_SIZE,A1
        move.l  A1,D0
        add.l   dev_NegSize(A3),D0
        moveq   #dev_Globals,D1
        add.l   D1,D0
        movea.l A5,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOFreeMem(A6)         ; free the clone
        movea.l A3,A5                   ; continue with the root
.decCount:
        subq.w  #1,LIB_OPENCNT(A5)
        movea.l $14(A7),A6
        bne.b   .exit                   ; still in use
        btst    #3,LIB_FLAGS(A5)        ; delayed expunge requested?
        beq.b   .exit
        movea.l A5,A6
        bsr.w   DevExpunge
        move.l  D0,D7                   ; return its SegList
.exit:
        move.l  D7,D0
        movem.l (A7)+,D7/A3/A5-A6
        addq.w  #8,A7
        rts

;---------------------------------------------------------------------
; DevExpunge -- device Expunge vector.
; Out: D0 = SegList to unload, or 0 if the device is still open.
;---------------------------------------------------------------------
DevExpunge:                             ; was AJL_0_27A
        subq.w  #8,A7
        movem.l D2/D7/A5-A6,-(A7)
        moveq   #0,D7
        movea.l dev_Root(A6),A5         ; always work on the root
        bset    #3,LIB_FLAGS(A5)        ; set LIBF_DELEXP
        move.l  A6,$10(A7)
        move.l  A6,$14(A7)
        tst.w   LIB_OPENCNT(A5)
        bne.b   .busy
        move.l  dev_RelocTab(A5),D0     ; free the reloc table copy
        beq.b   .noRelocTab
        movea.l D0,A0
        move.l  (A0),D1
        asl.l   #2,D1
        addq.l  #4,D1                   ; count*4+4 bytes
        movea.l D0,A1
        move.l  D1,D0
        movea.l AbsExecBase.W,A6
        jsr     _LVOFreeMem(A6)
.noRelocTab:
        move.l  dev_SegList(A5),D7      ; result: SegList
        movea.l A5,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVORemove(A6)          ; unlink from the device list
        moveq   #0,D0
        move.w  LIB_POSSIZE(A5),D0
        moveq   #0,D1
        move.w  LIB_NEGSIZE(A5),D1
        move.l  D1,D2
        add.l   D0,D2                   ; total allocation size
        movea.l A5,A1
        moveq   #0,D0
        move.w  D1,D0
        suba.l  D0,A1                   ; back to the raw block start
        move.l  D2,D0
        jsr     _LVOFreeMem(A6)         ; free the root base
.busy:
        move.l  D7,D0
        movem.l (A7)+,D2/D7/A5-A6
        addq.w  #8,A7
        rts

;---------------------------------------------------------------------
; HandlerProc -- entry point of the "IBMCON_Handler" process
; (spawned by UnitOpen via CreateNewProc, 4K stack).
;
; Waits for the startup message (whose payload word at offset $14 is
; the clone device base), creates the command port, initialises the
; console state on its own stack and then serves CMD_WRITE requests
; until CMD_DIE arrives.
;
; Stack frame F (A7 after movem):
;   $10 = clone base       $16.. = con (console state, $20A bytes)
;   $1E = con_RastPort     $56/$58/$5A = con defaults (pen/bold/fixed)
;   $60 = con_InitModes
;---------------------------------------------------------------------
HandlerProc:                            ; was JL_0_2EC
        suba.w  #CON_FRAME,A7
        movem.l A3-A6,-(A7)
        move.l  A6,$10(A7)
        suba.l  A1,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOFindTask(A6)        ; FindTask(NULL)
        movea.l D0,A0
        lea     pr_MsgPort(A0),A5       ; our process message port
        bra.b   .pollStartup
.waitStartup:
        movea.l A5,A0
        movea.l AbsExecBase.W,A6
        jsr     _LVOWaitPort(A6)
.pollStartup:
        movea.l $10(A7),A6
        movea.l A5,A0
        movea.l AbsExecBase.W,A6
        jsr     _LVOGetMsg(A6)
        movea.l D0,A3                   ; A3 = startup message
        tst.l   D0
        movea.l $10(A7),A6
        beq.b   .waitStartup
        movea.l $14(A3),A6              ; msg payload = clone base
        lea     dev_Globals(A6),A4      ; A4 = globals (stays put)
        clr.l   -(A7)
        clr.l   -(A7)
        movea.l $18(A7),A6
        bsr.w   CreatePort_             ; CreatePort(NULL, 0)
        addq.w  #8,A7
        move.l  D0,g_CmdPort(A4)        ; command port for BeginIO
        movea.l A3,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOReplyMsg(A6)        ; release UnitOpen
        tst.l   g_CmdPort(A4)
        movea.l $10(A7),A6
        beq.w   .exit                   ; no port -> die silently
        move.l  g_RastPort(A4),$1E(A7)  ; con_RastPort
        clr.w   $16+con_PenMapOn(A7)    ; 1.5: no pen table yet
        lea     $61(A7),A5              ; low byte of con_InitModes
        bclr    #0,(A5)                 ; LNM off (LF = pure linefeed)
        bset    #2,(A5)                 ; scroll at margins on
        bset    #1,(A5)+                ; auto-wrap on
        bsr.w   ScreenDepth             ; screen depth decides colour
                                        ;   handling (D0.l):
        subq.l  #1,D0
        beq.b   .depth1
        subq.l  #1,D0
        beq.b   .depth2
        subq.l  #1,D0
        beq.b   .depth3
        subq.l  #1,D0
        beq.b   .depth4plus
        bpl.b   .depth4plus             ; 1.5 FIXED: depth 5 and up (AGA
                                        ;   256, RTG) fell into mono below
.depth1:                                ; 2 colours: mono, pen 1,
        lea     $56(A7),A5              ;   colours+bold ignored
        move.w  #1,(A5)+                ; con_DefaultPen = 1
        clr.w   (A5)+                   ; con_BoldPens   = 0
        move.w  #1,(A5)+                ; con_FixedPen   = 1
        bra.b   .initCon
.depth2:                                ; 4 colours: colours fixed,
        lea     $56(A7),A5              ;   bold shown as pen 3
        move.w  #1,(A5)+
        moveq   #1,D0
        move.w  D0,(A5)+
        move.w  D0,(A5)+
        bra.b   .initCon
.depth3:                                ; 8 colours: SGR colours ok,
        lea     $56(A7),A5              ;   no bold (no bright pens)
        move.w  #1,(A5)+
        clr.w   (A5)+
        bra.b   .clearFixed
.depth4plus:                            ; 16+ colours: SGR colours ok,
        lea     $56(A7),A5              ;   bold = pen+8 (bright set)
        move.w  #1,(A5)+
        move.w  #1,(A5)+
.clearFixed:
        clr.w   (A5)+
.initCon:
        pea     $16(A7)                 ; &con
        bsr.w   ConInit
        pea     $1A(A7)                 ; &con (first pea still pushed)
        bsr.w   ToggleCursor            ; draw the initial cursor
        addq.w  #8,A7
.mainWait:
        movea.l g_CmdPort(A4),A0
        movea.l AbsExecBase.W,A6
        jsr     _LVOWaitPort(A6)
        bra.b   .pollNext
.gotMsg:                                ; A5 = IOStdReq
        moveq   #0,D0
        move.w  IO_COMMAND(A5),D0
        cmpi.l  #IBMCMD_SETPENS,D0      ; 1.5: pen table from the client
        beq.b   .doSetPens
        subq.l  #3,D0                   ; CMD_WRITE?
        beq.b   .doWrite
        subi.l  #CMD_DIE-CMD_WRITE,D0   ; CMD_DIE?
        bne.b   .reply                  ; anything else: just reply
        movea.l AbsExecBase.W,A6        ; CMD_DIE: reply under Forbid
        jsr     _LVOForbid(A6)          ;   and fall off the process
        movea.l A5,A1
        jsr     _LVOReplyMsg(A6)
        bra.b   .exit
.doSetPens:
        move.l  IO_DATA(A5),-(A7)
        pea     $1A(A7)                 ; &con (one long pushed)
        bsr.w   SetPenMap
        addq.w  #8,A7
        bra.b   .reply
.doWrite:
        move.l  IO_LENGTH(A5),-(A7)
        move.l  IO_DATA(A5),-(A7)
        pea     $1E(A7)                 ; &con
        bsr.w   ConWrite                ; interpret the stream
        lea     $C(A7),A7
.reply:
        movea.l A5,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOReplyMsg(A6)
.pollNext:
        movea.l $10(A7),A6
        movea.l g_CmdPort(A4),A0
        movea.l AbsExecBase.W,A6
        jsr     _LVOGetMsg(A6)
        movea.l D0,A5
        movea.l $10(A7),A6
        tst.l   D0
        bne.b   .gotMsg
        bra.w   .mainWait
.exit:
        movem.l (A7)+,A3-A6
        adda.w  #CON_FRAME,A7
        rts                             ; process terminates

;---------------------------------------------------------------------
; ConInit(con) -- initialise the console state.
; Clears the window, measures rows/columns from window size and font
; cell size, resets cursor/attributes/tab stops and takes the initial
; 's' snapshot.
;---------------------------------------------------------------------
ConInit:                                ; was JL_0_458 (1.5: grid via MeasureGrid)
        movem.l A5-A6,-(A7)
        movea.l $C(A7),A5               ; A5 = con
        move.l  A5,-(A7)                ; arg for ParserReset / MeasureGrid
        bsr.w   ParserReset
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0
        bsr.w   MapPen                  ; 1.5: ANSI black, not UI pen 0
        jsr     _LVOSetRast(A6)         ; clear window to the background
        moveq   #1,D1
        move.w  D1,con_Row(A5)          ; home the cursor
        move.w  D1,con_Col(A5)
        bsr.w   MeasureGrid             ; rows, cols, region, tab stops
        addq.w  #4,A7
        clr.w   con_WrapPending(A5)
        moveq   #1,D1
        move.w  D1,con_TextCol(A5)
        clr.l   con_Attrs(A5)
        move.w  con_DefaultPen(A5),con_FgPen(A5)
        clr.w   con_BgPen(A5)
        clr.w   con_TextLen(A5)
        clr.b   con_CharMask(A5)
        clr.b   con_SavedMask(A5)
        move.w  con_InitModes(A5),con_Modes(A5)
        moveq   #0,D0
        move.w  con_FgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        jsr     _LVOSetAPen(A6)
        moveq   #0,D0
        move.w  con_BgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        jsr     _LVOSetBPen(A6)
        clr.l   -(A7)
        clr.l   -(A7)
        move.l  A5,-(A7)
        movea.l $10(A7),A6              ; the caller's A6 (saved by movem)
        bsr.w   Csi_s_SaveCursor        ; initialise the 's' snapshot
        lea     $C(A7),A7
        movem.l (A7)+,A5-A6
        rts

;---------------------------------------------------------------------
; MeasureGrid(con) (1.5) -- rows and columns from the window's drawable
; area (the inner area of a GimmeZeroZero window, as on the Workbench;
; 1.4 counted a GZZ window's borders) and the font cell. Scroll region =
; whole window, cursor kept inside, tab stops every 8 columns from 9.
; Used by ConInit and, after a window resize, by CheckResize.
;---------------------------------------------------------------------
MeasureGrid:
        movem.l D2-D3/A2/A5,-(A7)
        movea.l $14(A7),A5              ; con
        movea.l g_Window(A4),A1
        move.w  wd_Width(A1),D2
        move.w  wd_Height(A1),D3
        btst    #2,wd_Flags+2(A1)       ; WFLG_GIMMEZEROZERO?
        beq.b   .sized
        move.w  wd_GZZWidth(A1),D2
        move.w  wd_GZZHeight(A1),D3
.sized:
        move.w  D2,con_WidthPx(A5)
        move.w  D3,con_HeightPx(A5)
        movea.l g_RastPort(A4),A2
        movea.l rp_Font(A2),A2          ; A2 = font (read live)
        moveq   #0,D0
        move.w  D3,D0
        divu    tf_YSize(A2),D0         ; rows = height / YSize
        tst.w   D0
        bne.b   .rowsOk
        moveq   #1,D0
.rowsOk:
        move.w  D0,con_Rows(A5)
        moveq   #0,D0
        move.w  D2,D0
        divu    tf_XSize(A2),D0         ; cols = width / XSize
        tst.w   D0
        bne.b   .colsSome
        moveq   #1,D0
.colsSome:
        cmpi.w  #MAX_COLS,D0            ; clamp to the tab map / text run
        bls.b   .colsOk
        move.w  #MAX_COLS,D0
.colsOk:
        move.w  D0,con_Cols(A5)
        clr.w   con_WrapPending(A5)     ; a new grid: no wrap due
        move.w  #1,con_RegTop(A5)       ; scroll region = full window
        move.w  con_Rows(A5),con_RegBot(A5)
        move.w  con_Rows(A5),D1         ; keep the cursor inside
        cmp.w   con_Row(A5),D1
        bcc.b   .rowIn
        move.w  D1,con_Row(A5)
.rowIn:
        cmp.w   con_Col(A5),D0
        bcc.b   .colIn
        move.w  D0,con_Col(A5)
.colIn:
        moveq   #0,D1
        move.w  D0,D1
        addq.l  #1,D1                   ; +1 -- HT probes index "cols"
        clr.l   -(A7)                   ; fill value 0
        move.l  D1,-(A7)                ; count = columns+1
        pea     con_TabStops(A5)
        bsr.w   FillBytes
        lea     $C(A7),A7
        moveq   #9,D1                   ; tab stops at 9, 17, 25, ...
        bra.b   .tabCheck
.setTab:
        moveq   #0,D0
        move.w  D1,D0
        addi.l  #con_TabStops,D0
        move.b  #1,0(A5,D0.L)
        addq.w  #8,D1
.tabCheck:
        cmp.w   con_Cols(A5),D1
        bcs.b   .setTab
        movem.l (A7)+,D2-D3/A2/A5
        rts

;---------------------------------------------------------------------
; CheckResize(con) (1.5) -- the window was resized since the grid was
; measured (a Workbench window): measure it again. Called at every write,
; with the cursor removed.
;---------------------------------------------------------------------
CheckResize:
        movem.l D2-D3/A5,-(A7)
        movea.l $10(A7),A5              ; con
        movea.l g_Window(A4),A1
        move.w  wd_Width(A1),D2
        move.w  wd_Height(A1),D3
        btst    #2,wd_Flags+2(A1)       ; WFLG_GIMMEZEROZERO?
        beq.b   .sized
        move.w  wd_GZZWidth(A1),D2
        move.w  wd_GZZHeight(A1),D3
.sized:
        cmp.w   con_WidthPx(A5),D2
        bne.b   .changed
        cmp.w   con_HeightPx(A5),D3
        beq.b   .same
.changed:
        move.l  A5,-(A7)
        bsr.w   MeasureGrid
        addq.w  #4,A7
.same:
        movem.l (A7)+,D2-D3/A5
        rts

;---------------------------------------------------------------------
; ScreenDepth (1.5) -- D0.l = depth of the window's screen. On graphics V39+
; GetBitMapAttr(BMA_DEPTH): an RTG bitmap's bm_Depth is not its depth (a
; 24-bit Picasso96 screen says 6 -- measured with rtgprobe), and 1.4 read it.
; bm_Depth below V39. Changes D0/D1/A0/A1.
;---------------------------------------------------------------------
ScreenDepth:
        move.l  A6,-(A7)
        movea.l g_Window(A4),A1
        movea.l wd_WScreen(A1),A0
        movea.l sc_vpRasInfo(A0),A1
        movea.l ri_BitMap(A1),A0
        movea.l g_GfxBase(A4),A6
        cmpi.w  #39,LIB_VERSION(A6)
        bcs.b   .old
        moveq   #BMA_DEPTH,D1
        jsr     _LVOGetBitMapAttr(A6)   ; (a0 = bitmap, d1 = attribute)
        bra.b   .done
.old:
        moveq   #0,D0
        move.b  bm_Depth(A0),D0
.done:
        movea.l (A7)+,A6
        rts

;---------------------------------------------------------------------
; MapPen (1.5) -- D0.l = logical ANSI pen (0-15). With a client pen table
; returns the screen pen for it in D0.l; without one D0 is unchanged.
; In: A5 = con. Changes only D0.
;---------------------------------------------------------------------
MapPen:
        tst.w   con_PenMapOn(A5)
        beq.b   .raw
        move.l  A0,-(A7)                ; 68000: an indexed displacement is
        lea     con_PenMap(A5),A0       ;   8-bit, con_PenMap ($20A) is not
        andi.w  #15,D0
        move.b  0(A0,D0.W),D0
        andi.l  #$FF,D0
        movea.l (A7)+,A0
.raw:
        rts

;---------------------------------------------------------------------
; SetPenMap(con, pens) (1.5) -- IBMCMD_SETPENS: take the client's 16
; screen pens for the ANSI colours, switch to full colour (all 16 pens and
; bold exist), and redraw the empty console with them.
;---------------------------------------------------------------------
SetPenMap:
        movem.l A2/A5-A6,-(A7)
        movea.l $10(A7),A5              ; con
        movea.l $14(A7),A2              ; pens
        move.l  A5,-(A7)
        bsr.w   ToggleCursor            ; remove the cursor (old pens)
        addq.w  #4,A7
        lea     con_PenMap(A5),A0
        moveq   #15,D0
.copy:
        move.b  (A2)+,(A0)+
        dbf     D0,.copy
        move.w  #1,con_PenMapOn(A5)
        move.w  #1,con_BoldPens(A5)
        clr.w   con_FixedPen(A5)
        move.w  #7,con_DefaultPen(A5)   ; ANSI order: white is 7 (1 is red;
        move.w  #7,con_FgPen(A5)        ;   pen 1 was white only in the unit-1
        clr.w   con_BgPen(A5)           ;   swapped order)
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0
        move.w  con_FgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        jsr     _LVOSetAPen(A6)
        moveq   #0,D0
        move.w  con_BgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        jsr     _LVOSetBPen(A6)
        moveq   #0,D0
        move.w  con_BgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        jsr     _LVOSetRast(A6)         ; clear to the ANSI background
        move.l  A5,-(A7)
        bsr.w   ToggleCursor            ; draw the cursor with the new pens
        addq.w  #4,A7
        movem.l (A7)+,A2/A5-A6
        rts

;---------------------------------------------------------------------
; CSI 't' -- set the number of text rows from parameter 1.
; Parameter 0 (or missing): re-derive rows from window height / font.
; (Loose take on "set page length".)
; FIXED(1.5): the default used wd_Height, which on a Workbench window
; (GimmeZeroZero) counts the title bar and borders -- one or two rows too
; many, so the bottom rows were drawn below the window and lost. It now
; measures the grid exactly as a resize does (MeasureGrid). A row count
; is clamped to the rows that fit. A sequence with more than one
; parameter is not a page length -- xterm window operations
; (ESC[8;24;80t) and PabloDraw true colour (ESC[0;r;g;bt) use CSI t --
; and is ignored.
;---------------------------------------------------------------------
Csi_t_SetRows:                          ; was AJL_0_56A
        subq.w  #4,A7
        movem.l D2/A5,-(A7)
        movea.l $14(A7),A0              ; A0 = params
        movea.l $10(A7),A5              ; A5 = con
        move.l  A6,$8(A7)
        tst.l   $18(A7)                 ; last param index > 0: more
        bne.b   .out                    ;   than one parameter, not ours
        move.b  (A0),D0                 ; param[0]
        bne.b   .fromParam
        move.l  A5,-(A7)                ; default: the window's grid
        bsr.w   MeasureGrid
        addq.w  #4,A7
        bra.b   .out
.fromParam:
        moveq   #0,D2
        move.b  D0,D2                   ; D2 = requested rows
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D0
        move.w  con_HeightPx(A5),D0
        divu    tf_YSize(A0),D0         ; rows that fit
        tst.w   D0
        bne.b   .fitSome
        moveq   #1,D0
.fitSome:
        cmp.w   D0,D2
        bls.b   .fits
        move.w  D0,D2                   ; clamp to the window
.fits:
        move.w  D2,con_Rows(A5)
        cmp.w   con_Row(A5),D2          ; keep the cursor inside
        bcc.b   .done
        move.w  D2,con_Row(A5)
.done:
        move.w  #1,con_RegTop(A5)       ; FIXED(r): row count changed
        move.w  con_Rows(A5),con_RegBot(A5) ; -> region = full window
.out:
        movem.l (A7)+,D2/A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'r' -- DECSTBM, set the vertical scroll region.
;   parameters: top ; bottom   (missing/0 -> row 1 / last row)
; Bottom is clamped to the window; an invalid region (top >= bottom)
; is ignored.  On success the cursor is homed, like the real DECSTBM.
; The region bounds the margin scrolling in CursorDownN/CursorUpN and
; therefore LF and line wrapping.
; FIXED: the original ignored the top margin completely and just set
; the row count from the bottom parameter.
;---------------------------------------------------------------------
Csi_r_SetRegion:                        ; was AJL_0_5B4
        subq.w  #4,A7
        movem.l D2/A5,-(A7)
        movea.l $14(A7),A0              ; A0 = params
        movea.l $10(A7),A5              ; A5 = con
        move.l  A6,$8(A7)
        moveq   #0,D0
        move.b  (A0),D0                 ; top (0 -> 1)
        bne.b   .haveTop
        moveq   #1,D0
.haveTop:
        moveq   #0,D1
        move.b  1(A0),D1                ; bottom (0 -> last row)
        bne.b   .haveBot
        move.w  con_Rows(A5),D1
.haveBot:
        move.w  con_Rows(A5),D2
        cmp.w   D2,D1
        bls.b   .botOk                  ; clamp bottom to the window
        move.w  D2,D1
.botOk:
        cmp.w   D1,D0
        bcc.b   .done                   ; top >= bottom: ignore
        move.w  D0,con_RegTop(A5)
        move.w  D1,con_RegBot(A5)
        moveq   #1,D0                   ; DECSTBM homes the cursor
        move.w  D0,con_Row(A5)
        move.w  D0,con_Col(A5)
.done:
        movem.l (A7)+,D2/A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI '@' -- insert N blank character cells at the cursor.
; The rest of the line is pushed right with ScrollRaster (negative dx).
;---------------------------------------------------------------------
Csi_AtSign_InsertChars:                 ; was AJL_0_600
        suba.w  #$10,A7
        movem.l D2-D5/D7/A5-A6,-(A7)
        movea.l $30(A7),A5              ; con
        movea.l $34(A7),A0              ; params
        move.b  (A0),D7                 ; N (0 -> 1)
        move.l  A6,$28(A7)
        tst.b   D7
        bne.b   .haveCount
        moveq   #1,D7
.haveCount:
        move.w  con_Col(A5),D0
        cmp.w   con_Cols(A5),D0
        bcc.b   .done                   ; cursor beyond line: nothing
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0          ; A0 = font
        moveq   #0,D1
        move.w  tf_XSize(A0),D1
        moveq   #0,D0
        move.b  D7,D0
        bsr.w   Mul32                   ; N * XSize
        neg.l   D0                      ; dx = -N*XSize (shift right)
        moveq   #0,D1
        move.w  tf_XSize(A0),D1
        moveq   #0,D2
        move.w  con_Col(A5),D2
        subq.l  #1,D2
        move.l  D0,$20(A7)              ; save dx
        move.l  D2,D0
        bsr.w   Mul32                   ; xmin = (col-1)*XSize
        moveq   #0,D1
        move.w  tf_YSize(A0),D1
        moveq   #0,D2
        move.w  con_Row(A5),D2
        move.l  D2,D3
        subq.l  #1,D3
        move.l  D0,$24(A7)              ; save xmin
        move.l  D3,D0
        bsr.w   Mul32                   ; ymin = (row-1)*YSize
        moveq   #0,D1
        move.w  con_WidthPx(A5),D1
        subq.l  #1,D1                   ; xmax = width-1
        mulu    tf_YSize(A0),D2
        subq.l  #1,D2                   ; ymax = row*YSize-1
        move.l  D0,D3                   ; ymin
        move.l  D1,D4                   ; xmax
        move.l  D2,D5                   ; ymax
        movea.l con_RastPort(A5),A1
        move.l  $20(A7),D0              ; dx
        move.l  $24(A7),D2              ; xmin
        movea.l g_GfxBase(A4),A6
        moveq   #0,D1                   ; dy = 0
        jsr     _LVOScrollRaster(A6)
.done:
        movem.l (A7)+,D2-D5/D7/A5-A6
        adda.w  #$10,A7
        rts

;---------------------------------------------------------------------
; CSI 'R' -- cursor position report: not implemented (the device has
; no read channel), accepted and ignored.
;---------------------------------------------------------------------
Csi_R_Stub:                             ; was AJL_0_6A4
        subq.w  #4,A7
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 's' -- save cursor position, attributes, pens, charset mask and
; mode flags (ANSI.SYS "save cursor position").
;---------------------------------------------------------------------
Csi_s_SaveCursor:                       ; was AJL_0_6AA
        subq.w  #4,A7
        movea.l $8(A7),A0               ; A0 = con
        lea     con_Row(A0),A1
        move.w  (A1)+,con_SavedRow(A0)
        move.w  con_Col(A0),con_SavedCol(A0)
        move.l  (A1)+,con_SavedAttrs(A0)
        move.w  (A1)+,con_SavedFg(A0)
        move.w  (A1)+,con_SavedBg(A0)
        move.b  con_CharMask(A0),con_SavedMask(A0)
        move.w  con_Modes(A0),con_SavedModes(A0)
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'u' -- restore the state saved by 's' and re-apply pens and
; soft style to the RastPort.
;---------------------------------------------------------------------
Csi_u_RestoreCursor:                    ; was AJL_0_6DA
        subq.w  #4,A7
        movem.l A5-A6,-(A7)
        movea.l $10(A7),A5              ; A5 = con
        lea     con_SavedRow(A5),A0
        lea     con_Row(A5),A1
        move.w  (A0)+,(A1)+             ; row
        move.w  con_SavedCol(A5),con_Col(A5)
        move.l  (A0)+,(A1)+             ; attrs
        move.w  (A0)+,D0                ; fg
        move.w  D0,(A1)+
        move.w  (A0)+,(A1)+             ; bg
        move.b  con_SavedMask(A5),con_CharMask(A5)
        move.w  con_SavedModes(A5),con_Modes(A5)
        moveq   #0,D0
        move.w  con_SavedFg(A5),D0
        bsr.w   MapPen
        move.l  A6,$8(A7)
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        jsr     _LVOSetAPen(A6)
        moveq   #0,D0
        move.w  con_BgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        jsr     _LVOSetBPen(A6)
        move.l  con_Attrs(A5),D0
        moveq   #0,D1
        move.w  D0,D1                   ; style word ($2E/$2F)
        move.l  D1,D0
        movea.l con_RastPort(A5),A1
        moveq   #STYLE_ENABLE,D1        ; FIXED: enable underline AND
        jsr     _LVOSetSoftStyle(A6)    ;   italic (was 1)
        movem.l (A7)+,A5-A6
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'h' -- set mode.  Walks all collected parameters; the raw
; character buffer (con_RawBuf) provides the prefix for each one:
;   (none) 20 -> LNM: LF implies CR        (bit 0)
;   '>'     1 -> scroll at margins on      (bit 2)
;   '?'     7 -> auto-wrap on (DECAWM)     (bit 1)
;---------------------------------------------------------------------
Csi_h_SetMode:                          ; was AJL_0_748
        subq.w  #4,A7
        movem.l D6-D7,-(A7)
        movea.l $14(A7),A0              ; A0 = params (values)
        movea.l $10(A7),A1              ; A1 = con
        moveq   #0,D6                   ; index
        move.l  $18(A7),D7              ; parameter count
        addq.l  #1,D7
        move.l  A6,$8(A7)
        bra.b   .loopCheck
.body:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #0,D1
        addi.l  #con_RawBuf,D0
        move.b  0(A1,D0.L),D1           ; raw char for this param
        tst.l   D1
        beq.b   .noPrefix
        moveq   #'>',D0
        sub.l   D0,D1
        beq.b   .gtPrefix
        subq.l  #1,D1                   ; '?'
        beq.b   .qmPrefix
        bra.b   .next
.noPrefix:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #20,D1
        cmp.b   0(A0,D0.L),D1           ; mode 20 = LNM
        bne.b   .next
        bset    #0,con_ModeFlags(A1)
        bra.b   .next
.gtPrefix:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #1,D1
        cmp.b   0(A0,D0.L),D1           ; >1 = scroll at margins
        bne.b   .next
        bset    #2,con_ModeFlags(A1)
        bra.b   .next
.qmPrefix:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #7,D1
        cmp.b   0(A0,D0.L),D1           ; ?7 = auto-wrap
        bne.b   .next
        bset    #1,con_ModeFlags(A1)
.next:
        addq.w  #1,D6
.loopCheck:
        moveq   #0,D0
        move.w  D6,D0
        cmp.l   D7,D0
        bcs.b   .body
        movem.l (A7)+,D6-D7
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'l' -- reset mode.  Mirror image of Csi_h_SetMode (bclr).
;---------------------------------------------------------------------
Csi_l_ResetMode:                        ; was AJL_0_7D0
        subq.w  #4,A7
        movem.l D6-D7,-(A7)
        movea.l $14(A7),A0
        movea.l $10(A7),A1
        moveq   #0,D6
        move.l  $18(A7),D7
        addq.l  #1,D7
        move.l  A6,$8(A7)
        bra.b   .loopCheck
.body:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #0,D1
        addi.l  #con_RawBuf,D0
        move.b  0(A1,D0.L),D1
        tst.l   D1
        beq.b   .noPrefix
        moveq   #'>',D0
        sub.l   D0,D1
        beq.b   .gtPrefix
        subq.l  #1,D1
        beq.b   .qmPrefix
        bra.b   .next
.noPrefix:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #20,D1
        cmp.b   0(A0,D0.L),D1
        bne.b   .next
        bclr    #0,con_ModeFlags(A1)    ; LNM off
        bra.b   .next
.gtPrefix:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #1,D1
        cmp.b   0(A0,D0.L),D1
        bne.b   .next
        bclr    #2,con_ModeFlags(A1)    ; margin scroll off
        bra.b   .next
.qmPrefix:
        moveq   #0,D0
        move.w  D6,D0
        moveq   #7,D1
        cmp.b   0(A0,D0.L),D1
        bne.b   .next
        bclr    #1,con_ModeFlags(A1)    ; auto-wrap off
.next:
        addq.w  #1,D6
.loopCheck:
        moveq   #0,D0
        move.w  D6,D0
        cmp.l   D7,D0
        bcs.b   .body
        movem.l (A7)+,D6-D7
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'm' -- SGR, select graphic rendition.
; Walks all parameters through a 50-entry jump table, collects the
; wanted changes, then applies pen/style changes to the RastPort once
; at the end.
;
; Locals: D4 = param index, D5 = "style changed", D6 = "fg changed",
;         $2A(A7) word = "bg changed", $20(A7) byte = current param.
;---------------------------------------------------------------------
Csi_m_SetGraphics:                      ; was AJL_0_858
        suba.w  #$C,A7
        movem.l D2/D4-D7/A3/A5-A6,-(A7)
        movea.l $34(A7),A3              ; A3 = params
        movea.l $30(A7),A5              ; A5 = con
        moveq   #0,D6                   ; fg changed = no
        clr.w   $2A(A7)                 ; bg changed = no
        moveq   #0,D5                   ; style changed = no
        moveq   #0,D4                   ; param index
        move.l  $38(A7),D7              ; parameter count
        addq.l  #1,D7
        move.l  A6,$24(A7)
        bra.w   .loopCheck
.body:
        moveq   #0,D0
        move.w  D4,D0
        move.b  0(A3,D0.L),D1           ; parameter value
        moveq   #0,D0
        move.b  D1,D0
        move.b  D1,$20(A7)
        cmpi.l  #50,D0
        bcc.w   .next                   ; >49: ignore
        add.w   D0,D0
        move.w  .jTab(PC,D0.W),D0
        jmp     .jBase(PC,D0.W)

.jTab:  dc.w    .sgrReset-.jBase        ;  0 reset all attributes
.jBase: dc.w    .sgrBold-.jBase         ;  1 bold
        dc.w    .next-.jBase            ;  2 (faint) unsupported
        dc.w    .sgrItalic-.jBase       ;  3 italic
        dc.w    .sgrUnderline-.jBase    ;  4 underline
        dc.w    .next-.jBase            ;  5 (blink) unsupported
        dc.w    .next-.jBase            ;  6
        dc.w    .sgrReverse-.jBase      ;  7 reverse video
        dc.w    .next-.jBase            ;  8
        dc.w    .next-.jBase            ;  9
        dc.w    .next-.jBase            ; 10
        dc.w    .next-.jBase            ; 11
        dc.w    .next-.jBase            ; 12
        dc.w    .next-.jBase            ; 13
        dc.w    .next-.jBase            ; 14
        dc.w    .next-.jBase            ; 15
        dc.w    .next-.jBase            ; 16
        dc.w    .next-.jBase            ; 17
        dc.w    .next-.jBase            ; 18
        dc.w    .next-.jBase            ; 19
        dc.w    .next-.jBase            ; 20
        dc.w    .next-.jBase            ; 21
        dc.w    .next-.jBase            ; 22 (normal) unsupported
        dc.w    .sgrItalicOff-.jBase    ; 23 italic off
        dc.w    .sgrUnderlOff-.jBase    ; 24 underline off
        dc.w    .next-.jBase            ; 25
        dc.w    .next-.jBase            ; 26
        dc.w    .next-.jBase            ; 27 (reverse off) unsupported
        dc.w    .next-.jBase            ; 28
        dc.w    .next-.jBase            ; 29
        dc.w    .sgrForeground-.jBase   ; 30..37 foreground colour
        dc.w    .sgrForeground-.jBase   ; 31
        dc.w    .sgrForeground-.jBase   ; 32
        dc.w    .sgrForeground-.jBase   ; 33
        dc.w    .sgrForeground-.jBase   ; 34
        dc.w    .sgrForeground-.jBase   ; 35
        dc.w    .sgrForeground-.jBase   ; 36
        dc.w    .sgrForeground-.jBase   ; 37
        dc.w    .next-.jBase            ; 38
        dc.w    .sgrForeground-.jBase   ; 39 default foreground
        dc.w    .sgrBackground-.jBase   ; 40..47 background colour
        dc.w    .sgrBackground-.jBase   ; 41
        dc.w    .sgrBackground-.jBase   ; 42
        dc.w    .sgrBackground-.jBase   ; 43
        dc.w    .sgrBackground-.jBase   ; 44
        dc.w    .sgrBackground-.jBase   ; 45
        dc.w    .sgrBackground-.jBase   ; 46
        dc.w    .sgrBackground-.jBase   ; 47
        dc.w    .next-.jBase            ; 48
        dc.w    .sgrBackground-.jBase   ; 49 default background

.sgrReset:                              ; 0: everything back to default
        clr.l   con_Attrs(A5)
        move.w  con_DefaultPen(A5),con_FgPen(A5)
        clr.w   con_BgPen(A5)
        moveq   #1,D6                   ; fg changed
        moveq   #1,D0
        move.w  D0,D5                   ; style changed
        move.w  D0,$2A(A7)              ; bg changed
        bra.w   .next
.sgrBold:                               ; 1: only when the screen depth
        tst.w   con_BoldPens(A5)        ;    allows a bold rendering
        beq.w   .next
        bset    #4,con_AttrFlags(A5)
        moveq   #1,D6
        bra.w   .next
.sgrItalic:                             ; 3: FSF_ITALIC
        bset    #2,con_SoftStyle(A5)
        moveq   #1,D5
        bra.w   .next
.sgrUnderline:                          ; 4: FSF_UNDERLINED
        bset    #0,con_SoftStyle(A5)
        moveq   #1,D5
        bra.w   .next
.sgrReverse:                            ; 7
        bset    #5,con_AttrFlags(A5)
        bra.w   .next
.sgrItalicOff:                          ; 23
        bclr    #2,con_SoftStyle(A5)
        moveq   #1,D5
        bra.w   .next
.sgrUnderlOff:                          ; 24
        bclr    #0,con_SoftStyle(A5)
        moveq   #1,D5
        bra.w   .next
.sgrForeground:                         ; 30-37, 39
        tst.w   con_FixedPen(A5)
        bne.w   .next                   ; colours locked on this depth
        moveq   #0,D0
        move.b  $20(A7),D0
        move.l  D0,D1
        moveq   #30,D0
        sub.l   D0,D1                   ; pen = param-30
        move.w  D1,con_FgPen(A5)
        moveq   #9,D0
        cmp.w   D0,D1
        bne.b   .fgNotDefault
        move.w  con_DefaultPen(A5),D0   ; 39: default foreground
        move.w  D0,con_FgPen(A5)
.fgNotDefault:
        moveq   #1,D6                   ; fg changed
        tst.w   con_PenMapOn(A5)
        bne.w   .next                   ; 1.5: a pen table is in ANSI order
        move.l  g_UnitNum(A4),D0
        ble.w   .next                   ; unit 0: pens used as-is
        moveq   #1,D0                   ; unit >=1: swap pens 1 and 7
        cmp.w   con_FgPen(A5),D0        ;   (IBM: 1=blue 7=white,
        bne.b   .fgTry7                 ;    Amiga: 1=black 7=...)
        moveq   #7,D1
        move.w  D1,con_FgPen(A5)
        bra.w   .next
.fgTry7:
        moveq   #7,D1
        cmp.w   con_FgPen(A5),D1
        bne.w   .next
        move.w  D0,con_FgPen(A5)
        bra.b   .next
.sgrBackground:                         ; 40-47, 49
        tst.w   con_FixedPen(A5)
        bne.b   .next
        moveq   #0,D0
        move.b  $20(A7),D0
        moveq   #40,D1
        sub.l   D1,D0                   ; pen = param-40
        move.w  D0,con_BgPen(A5)
        moveq   #9,D1
        cmp.w   D1,D0
        beq.b   .bgDefault              ; 49: default background = 0
        tst.w   con_PenMapOn(A5)
        bne.b   .bgOk                   ; 1.5: the table has all 16 pens
        move.l  D0,-(A7)                ; validate against screen depth
        bsr.w   ScreenDepth
        move.l  D0,D1                   ; D1 = depth
        move.l  (A7)+,D0                ; D0 = pen again
        cmpi.b  #4,D1
        bhi.b   .bgOk                   ; 1.5 FIXED: 32+ colours have all 16
                                        ;   (bset is mod 32: depth 32 = 1 pen)
        moveq   #0,D2
        bset    D1,D2                   ; D2 = number of pens
        moveq   #0,D1
        move.w  D0,D1
        cmp.l   D2,D1
        blt.b   .bgOk                   ; pen exists on this screen
.bgDefault:
        moveq   #0,D1
        move.w  D1,con_BgPen(A5)
.bgOk:
        move.w  #1,$2A(A7)              ; bg changed
        tst.w   con_PenMapOn(A5)
        bne.b   .next                   ; 1.5: a pen table is in ANSI order
        move.l  g_UnitNum(A4),D0
        ble.b   .next
        moveq   #1,D0                   ; unit >=1: swap pens 1 and 7
        cmp.w   con_BgPen(A5),D0
        bne.b   .bgTry7
        moveq   #7,D1
        move.w  D1,con_BgPen(A5)
        bra.b   .next
.bgTry7:
        moveq   #7,D1
        cmp.w   con_BgPen(A5),D1
        bne.b   .next
        move.w  D0,con_BgPen(A5)
.next:
        addq.w  #1,D4
.loopCheck:
        moveq   #0,D0
        move.w  D4,D0
        cmp.l   D7,D0
        bcs.w   .body
        move.w  con_FgPen(A5),D0        ; sanity: fg == bg would make
        cmp.w   con_BgPen(A5),D0        ; text invisible -> reset pens
        bne.b   .applyFg                ; (unless bold pens can fix it)
        tst.w   con_BoldPens(A5)
        bne.b   .applyFg
        move.w  con_DefaultPen(A5),con_FgPen(A5)
        clr.w   con_BgPen(A5)
        moveq   #1,D6
        move.w  #1,$2A(A7)
.applyFg:
        tst.w   D6
        beq.b   .applyBg
        btst    #4,con_AttrFlags(A5)    ; bold?
        beq.b   .setAPen
        tst.w   con_FixedPen(A5)
        beq.b   .brightPen
        moveq   #3,D0                   ; 4-colour screen: bold = pen 3
        bra.b   .forcePen
.brightPen:
        move.w  con_FgPen(A5),D0
        moveq   #7,D1
        cmp.w   D1,D0
        bhi.b   .setAPen
        bset    #3,D0                   ; 16+ colours: pen += 8
.forcePen:
        move.w  D0,con_FgPen(A5)
.setAPen:
        moveq   #0,D0
        move.w  con_FgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        jsr     _LVOSetAPen(A6)
.applyBg:
        movea.l $24(A7),A6
        tst.w   $2A(A7)
        beq.b   .applyStyle
        moveq   #0,D0
        move.w  con_BgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        jsr     _LVOSetBPen(A6)
.applyStyle:
        movea.l $24(A7),A6
        tst.w   D5
        beq.b   .done
        move.l  con_Attrs(A5),D0
        moveq   #0,D1
        move.w  D0,D1                   ; style word ($2E/$2F)
        move.l  D1,D0
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        moveq   #STYLE_ENABLE,D1        ; FIXED: enable underline AND
        jsr     _LVOSetSoftStyle(A6)    ;   italic (was 1)
.done:
        movem.l (A7)+,D2/D4-D7/A3/A5-A6
        adda.w  #$C,A7
        rts

;---------------------------------------------------------------------
; CSI 'n' -- device status report: not implemented, ignored.
;---------------------------------------------------------------------
Csi_n_Stub:                             ; was AJL_0_AD8
        subq.w  #4,A7
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'S' -- scroll the window contents UP by N lines; blank space
; appears at the bottom.
; FIXED: the original was a byte-identical copy of 'T' (a stray neg.l
; made dy negative), so 'S' scrolled the wrong way.
;---------------------------------------------------------------------
Csi_S_ScrollUp:                         ; was AJL_0_ADE
        suba.w  #$10,A7
        movem.l D2-D5/D7/A6,-(A7)
        movea.l $30(A7),A0              ; params
        move.b  (A0),D7                 ; N (0 -> 1)
        move.l  A6,$24(A7)
        tst.b   D7
        bne.b   .haveCount
        moveq   #1,D7
.haveCount:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D0
        move.w  tf_YSize(A0),D0
        moveq   #0,D1
        move.b  D7,D1
        bsr.w   Mul32                   ; N * YSize
                                        ; FIXED: no neg.l here --
                                        ;   dy > 0: contents move up
        moveq   #0,D1
        movea.l $2C(A7),A1              ; con
        move.w  con_WidthPx(A1),D1
        subq.l  #1,D1                   ; xmax
        move.w  tf_YSize(A0),D2
        move.w  con_Rows(A1),D3
        mulu    D2,D3
        subq.l  #1,D3                   ; ymax = rows*YSize-1
        move.l  D1,$20(A7)
        move.l  D0,D1                   ; dy
        move.l  D3,D5
        movea.l con_RastPort(A1),A1
        move.l  $20(A7),D4
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0                   ; dx=0, xmin=0, ymin=0
        move.l  D0,D2
        move.l  D0,D3
        jsr     _LVOScrollRaster(A6)
        movem.l (A7)+,D2-D5/D7/A6
        adda.w  #$10,A7
        rts

;---------------------------------------------------------------------
; CSI 'T' -- scroll the window contents down by N lines.
; (In the original binary this was byte-identical to 'S'; 'S' is
; fixed to scroll up, 'T' keeps the original downward behaviour.)
;---------------------------------------------------------------------
Csi_T_ScrollDown:                       ; was AJL_0_B4E
        suba.w  #$10,A7
        movem.l D2-D5/D7/A6,-(A7)
        movea.l $30(A7),A0
        move.b  (A0),D7
        move.l  A6,$24(A7)
        tst.b   D7
        bne.b   .haveCount
        moveq   #1,D7
.haveCount:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D0
        move.w  tf_YSize(A0),D0
        moveq   #0,D1
        move.b  D7,D1
        bsr.w   Mul32
        neg.l   D0
        moveq   #0,D1
        movea.l $2C(A7),A1
        move.w  con_WidthPx(A1),D1
        subq.l  #1,D1
        move.w  tf_YSize(A0),D2
        move.w  con_Rows(A1),D3
        mulu    D2,D3
        subq.l  #1,D3
        move.l  D1,$20(A7)
        move.l  D0,D1
        move.l  D3,D5
        movea.l con_RastPort(A1),A1
        move.l  $20(A7),D4
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0
        move.l  D0,D2
        move.l  D0,D3
        jsr     _LVOScrollRaster(A6)
        movem.l (A7)+,D2-D5/D7/A6
        adda.w  #$10,A7
        rts

;---------------------------------------------------------------------
; CSI 'P' -- delete N character cells at the cursor; the rest of the
; line moves left (ScrollRaster with positive dx).
;---------------------------------------------------------------------
Csi_P_DeleteChars:                      ; was AJL_0_BBE
        suba.w  #$10,A7
        movem.l D2-D5/D7/A5-A6,-(A7)
        movea.l $30(A7),A5              ; con
        movea.l $34(A7),A0              ; params
        move.b  (A0),D7                 ; N (0 -> 1)
        move.l  A6,$28(A7)
        tst.b   D7
        bne.b   .haveCount
        moveq   #1,D7
.haveCount:
        move.w  con_Col(A5),D0
        cmp.w   con_Cols(A5),D0
        bcc.b   .done
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D1
        move.w  tf_XSize(A0),D1
        moveq   #0,D0
        move.b  D7,D0
        bsr.w   Mul32                   ; dx = +N*XSize (move left)
        moveq   #0,D1
        move.w  tf_XSize(A0),D1
        moveq   #0,D2
        move.w  con_Col(A5),D2
        subq.l  #1,D2
        move.l  D0,$20(A7)              ; save dx
        move.l  D2,D0
        bsr.w   Mul32                   ; xmin = (col-1)*XSize
        moveq   #0,D1
        move.w  tf_YSize(A0),D1
        moveq   #0,D2
        move.w  con_Row(A5),D2
        move.l  D2,D3
        subq.l  #1,D3
        move.l  D0,$24(A7)              ; save xmin
        move.l  D3,D0
        bsr.w   Mul32                   ; ymin = (row-1)*YSize
        moveq   #0,D1
        move.w  con_WidthPx(A5),D1
        subq.l  #1,D1                   ; xmax
        mulu    tf_YSize(A0),D2
        subq.l  #1,D2                   ; ymax = row*YSize-1
        move.l  D0,D3
        move.l  D1,D4
        move.l  D2,D5
        movea.l con_RastPort(A5),A1
        move.l  $20(A7),D0              ; dx
        move.l  $24(A7),D2              ; xmin
        movea.l g_GfxBase(A4),A6
        moveq   #0,D1                   ; dy = 0
        jsr     _LVOScrollRaster(A6)
.done:
        movem.l (A7)+,D2-D5/D7/A5-A6
        adda.w  #$10,A7
        rts

;---------------------------------------------------------------------
; CSI 'M' -- delete N lines at the cursor row; lines below move up,
; blank space appears at the bottom.  N is clamped to the rows left.
;---------------------------------------------------------------------
Csi_M_DeleteLines:                      ; was AJL_0_C60
        suba.w  #$C,A7
        movem.l D2-D5/D7/A5-A6,-(A7)
        movea.l $2C(A7),A5              ; con
        movea.l $30(A7),A0              ; params
        move.b  (A0),D7                 ; N (0 -> 1)
        move.l  A6,$24(A7)
        tst.b   D7
        bne.b   .haveCount
        moveq   #1,D7
.haveCount:
        move.w  con_Row(A5),D0
        moveq   #1,D1
        cmp.w   D1,D0
        bcs.b   .done                   ; row out of range
        move.w  con_Rows(A5),D1
        cmp.w   D1,D0
        bhi.b   .done
        moveq   #0,D2
        move.w  D0,D2
        moveq   #0,D0
        move.w  D1,D0
        sub.l   D2,D0
        addq.l  #1,D0                   ; rows below incl. current
        moveq   #0,D1
        move.b  D7,D1
        cmp.l   D0,D1
        ble.b   .clamped
        move.l  D0,D7                   ; clamp N
.clamped:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D2
        move.w  tf_YSize(A0),D2
        moveq   #0,D0
        move.b  D7,D0
        move.l  D2,D1
        bsr.w   Mul32                   ; dy = +N*YSize (move up)
        moveq   #0,D1
        move.w  D2,D1
        moveq   #0,D3
        move.w  con_Row(A5),D3
        subq.l  #1,D3
        move.l  D0,$20(A7)              ; save dy
        move.l  D3,D0
        bsr.w   Mul32                   ; ymin = (row-1)*YSize
        moveq   #0,D1
        move.w  con_WidthPx(A5),D1
        subq.l  #1,D1                   ; xmax
        move.w  con_Rows(A5),D3
        mulu    D2,D3
        subq.l  #1,D3                   ; ymax = rows*YSize-1
        move.l  D3,$1C(A7)
        move.l  D0,D3                   ; ymin
        move.l  D1,D4                   ; xmax
        movea.l con_RastPort(A5),A1
        move.l  $20(A7),D1              ; dy
        move.l  $1C(A7),D5              ; ymax
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0                   ; dx=0
        move.l  D0,D2                   ; xmin=0
        jsr     _LVOScrollRaster(A6)
.done:
        movem.l (A7)+,D2-D5/D7/A5-A6
        adda.w  #$C,A7
        rts

;---------------------------------------------------------------------
; CSI 'L' -- insert N blank lines at the cursor row; lines from the
; cursor down move towards the bottom (negative dy scroll).
;---------------------------------------------------------------------
Csi_L_InsertLines:                      ; was AJL_0_D0C
        suba.w  #$C,A7
        movem.l D2-D5/D7/A5-A6,-(A7)
        movea.l $2C(A7),A5              ; con
        movea.l $30(A7),A0              ; params
        move.b  (A0),D7                 ; N (0 -> 1)
        move.l  A6,$24(A7)
        tst.b   D7
        bne.b   .haveCount
        moveq   #1,D7
.haveCount:
        move.w  con_Row(A5),D0
        moveq   #1,D1
        cmp.w   D1,D0
        bcs.b   .done
        move.w  con_Rows(A5),D1
        cmp.w   D1,D0
        bhi.b   .done
        moveq   #0,D2
        move.w  D0,D2
        moveq   #0,D0
        move.w  D1,D0
        sub.l   D2,D0                   ; rows below the cursor
        moveq   #0,D1
        move.b  D7,D1
        cmp.l   D0,D1
        ble.b   .clamped
        move.l  D0,D7                   ; clamp N
.clamped:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D2
        move.w  tf_YSize(A0),D2
        moveq   #0,D0
        move.b  D7,D0
        move.l  D2,D1
        bsr.w   Mul32
        neg.l   D0                      ; dy = -N*YSize (move down)
        moveq   #0,D1
        move.w  D2,D1
        moveq   #0,D2
        move.w  con_Row(A5),D2
        subq.l  #1,D2
        move.l  D0,$20(A7)              ; save dy
        move.l  D2,D0
        bsr.w   Mul32                   ; ymin = (row-1)*YSize
        moveq   #0,D1
        move.w  con_WidthPx(A5),D1
        subq.l  #1,D1                   ; xmax
        moveq   #0,D2
        move.w  con_Rows(A5),D2
        subq.l  #1,D2                   ; NOTE: ymax = rows-1 (pixel
        move.l  D0,D3                   ; row!), original quirk
        move.l  D1,D4
        move.l  D2,D5
        movea.l con_RastPort(A5),A1
        move.l  $20(A7),D1              ; dy
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0
        move.l  D0,D2
        jsr     _LVOScrollRaster(A6)
.done:
        movem.l (A7)+,D2-D5/D7/A5-A6
        adda.w  #$C,A7
        rts

;---------------------------------------------------------------------
; CSI 'K' -- erase in line.
;   0 (default): cursor to end of line
;   1:           start of line to cursor
;   2:           whole line
;---------------------------------------------------------------------
Csi_K_EraseLine:                        ; was AJL_0_DB2
        subq.w  #4,A7
        move.l  A5,-(A7)
        movea.l $C(A7),A5               ; con
        moveq   #0,D0
        movea.l $10(A7),A0              ; params
        move.b  (A0),D0
        move.l  A6,$4(A7)
        tst.l   D0
        beq.b   .toEol
        subq.l  #1,D0
        beq.b   .toCursor
        subq.l  #1,D0
        beq.b   .wholeLine
        bra.b   .done
.toEol:
        move.w  con_Cols(A5),D0
        sub.w   con_Col(A5),D0
        addq.w  #1,D0                   ; cells to the right edge
        moveq   #0,D1
        move.w  D0,D1
        move.l  D1,-(A7)
        move.l  A5,-(A7)
        bsr.w   EraseCellsRight
        bra.b   .pop8
.toCursor:
        move.l  A5,-(A7)
        bsr.w   EraseLineToCursor
        addq.w  #4,A7
        bra.b   .done
.wholeLine:
        move.l  A5,-(A7)
        bsr.w   EraseLineToCursor
        move.w  con_Cols(A5),D0
        sub.w   con_Col(A5),D0
        addq.w  #1,D0
        moveq   #0,D1
        move.w  D0,D1
        move.l  D1,(A7)                 ; reuse arg slot
        move.l  A5,-(A7)
        bsr.w   EraseCellsRight
.pop8:
        addq.w  #8,A7
.done:
        movea.l (A7)+,A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; EraseCellsRight(con, n) -- blank n character cells starting at the
; cursor, clipped to the right edge.
;---------------------------------------------------------------------
EraseCellsRight:                        ; was JL_0_E1A
        subq.w  #4,A7
        movem.l D2-D5/D7/A5,-(A7)
        movea.l $20(A7),A5              ; con
        move.w  con_Col(A5),D0
        move.w  $26(A7),D1              ; n
        add.w   D0,D1
        move.w  D1,D7
        subq.w  #1,D7                   ; last column touched
        move.l  A6,$18(A7)
        move.w  con_Cols(A5),D1
        cmp.w   D1,D7
        bls.b   .clipped
        move.w  D1,D7                   ; clip to the right edge
.clipped:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        subq.w  #1,D0
        move.w  tf_XSize(A0),D1
        muls    D1,D0
        moveq   #0,D2
        move.w  D0,D2                   ; xmin = (col-1)*XSize
        move.w  con_Row(A5),D0
        move.l  D0,D3
        subq.w  #1,D3
        move.w  tf_YSize(A0),D4
        muls    D4,D3
        moveq   #0,D5
        move.w  D3,D5                   ; ymin = (row-1)*YSize
        move.w  D7,D3
        mulu    D1,D3
        moveq   #0,D1
        move.w  D3,D1                   ; xmax = lastcol*XSize
        mulu    D4,D0
        moveq   #0,D3
        move.w  D0,D3                   ; ymax = row*YSize
        move.l  D3,-(A7)                ; ClearRect(con,xmin,ymin,
        move.l  D1,-(A7)                ;           xmax,ymax)
        move.l  D5,-(A7)
        move.l  D2,-(A7)
        move.l  A5,-(A7)                ; FIXED: pass con (ClearRect
        bsr.w   ClearRect               ;   needs the background pen)
        lea     $14(A7),A7
        movem.l (A7)+,D2-D5/D7/A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; EraseLineToCursor(con) -- blank the current line from column 1 up to
; and including the cursor column.  Does nothing at column 1.
;---------------------------------------------------------------------
EraseLineToCursor:                      ; was JL_0_E90
        subq.w  #4,A7
        movem.l D2-D4/A5,-(A7)
        movea.l $18(A7),A5              ; con
        move.l  A6,$10(A7)
        move.w  con_Col(A5),D0
        moveq   #1,D1
        cmp.w   D1,D0
        bls.b   .done
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        move.w  con_Row(A5),D1
        move.l  D1,D2
        subq.w  #1,D2
        move.w  tf_YSize(A0),D3
        muls    D3,D2
        moveq   #0,D4
        move.w  D2,D4                   ; ymin = (row-1)*YSize
        mulu    tf_XSize(A0),D0
        moveq   #0,D2
        move.w  D0,D2                   ; xmax = col*XSize
        mulu    D3,D1
        moveq   #0,D0
        move.w  D1,D0                   ; ymax = row*YSize
        move.l  D0,-(A7)
        move.l  D2,-(A7)
        move.l  D4,-(A7)
        clr.l   -(A7)                   ; xmin = 0
        move.l  A5,-(A7)                ; FIXED: pass con (ClearRect
        bsr.w   ClearRect               ;   needs the background pen)
        lea     $14(A7),A7
.done:
        movem.l (A7)+,D2-D4/A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; ClearRect(con, xmin, ymin, xmax, ymax) -- fill a pixel rectangle
; (xmax/ymax exclusive) with the CURRENT SGR BACKGROUND PEN.
; FIXED: the original took the RastPort and did a ClipBlit onto
; itself with minterm $00, i.e. it always erased to colour 0.  Now it
; takes con, RectFills with con_BgPen and restores the text pen --
; consistent with the areas ScrollRaster reveals (rp_BgPen).
;---------------------------------------------------------------------
ClearRect:                              ; was JL_0_EEC
        subq.w  #4,A7
        movem.l D2-D3/A5-A6,-(A7)
        movea.l $18(A7),A5              ; con
        movea.l con_RastPort(A5),A1
        moveq   #0,D0
        move.w  con_BgPen(A5),D0
        bsr.w   MapPen
        movea.l g_GfxBase(A4),A6
        jsr     _LVOSetAPen(A6)         ; fill colour = background
        moveq   #0,D0
        move.w  $1E(A7),D0              ; xmin
        moveq   #0,D1
        move.w  $22(A7),D1              ; ymin
        moveq   #0,D2
        move.w  $26(A7),D2              ; xmax (exclusive)
        subq.l  #1,D2                   ; RectFill xmax is inclusive
        moveq   #0,D3
        move.w  $2A(A7),D3              ; ymax (exclusive)
        subq.l  #1,D3
        cmp.l   D0,D2
        blt.b   .restorePen             ; degenerate rectangle: skip
        cmp.l   D1,D3
        blt.b   .restorePen
        movea.l con_RastPort(A5),A1
        jsr     _LVORectFill(A6)
.restorePen:
        moveq   #0,D0
        move.w  con_FgPen(A5),D0
        bsr.w   MapPen
        movea.l con_RastPort(A5),A1
        jsr     _LVOSetAPen(A6)         ; back to the text pen
        movem.l (A7)+,D2-D3/A5-A6
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; EraseTopToCursor(con) -- blank all full lines above the cursor, then
; tail-call EraseLineToCursor for the partial current line.
; (CSI 'J' mode 1 helper.)
;---------------------------------------------------------------------
EraseTopToCursor:                       ; was JL_0_F3E
        subq.w  #4,A7
        movem.l D2/A5,-(A7)
        movea.l $10(A7),A5              ; con
        move.l  A6,$8(A7)
        move.w  con_Row(A5),D0
        moveq   #1,D1
        cmp.w   D1,D0
        bls.b   .noFullLines
        moveq   #0,D1
        move.w  con_WidthPx(A5),D1      ; xmax = window width
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        subq.w  #1,D0
        mulu    tf_YSize(A0),D0
        moveq   #0,D2
        move.w  D0,D2                   ; ymax = (row-1)*YSize
        move.l  D2,-(A7)
        move.l  D1,-(A7)
        moveq   #0,D0
        move.l  D0,-(A7)                ; ymin = 0
        move.l  D0,-(A7)                ; xmin = 0
        move.l  A5,-(A7)                ; FIXED: pass con (ClearRect
        bsr.w   ClearRect               ;   needs the background pen)
        lea     $14(A7),A7
.noFullLines:
        move.l  A5,$10(A7)              ; re-arm the argument and
        movem.l (A7)+,D2/A5             ; tail-call
        addq.l  #4,A7
        bra.w   EraseLineToCursor

;---------------------------------------------------------------------
; EraseLinesFrom(con, n) -- blank n whole lines starting at the
; current row (clipped to the bottom).  (CSI 'J' mode 0 helper.)
;---------------------------------------------------------------------
EraseLinesFrom:                         ; was JL_0_F92
        subq.w  #4,A7
        movem.l D2-D3/D7/A5,-(A7)
        movea.l $18(A7),A5              ; con
        move.w  con_Row(A5),D0
        move.w  $1E(A7),D1              ; n
        add.w   D0,D1
        move.l  D1,D7                   ; last row + 1
        move.l  A6,$10(A7)
        move.w  con_Rows(A5),D1
        cmp.w   D1,D7
        bls.b   .clipped
        move.w  D1,D7                   ; clip to the bottom
.clipped:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        subq.w  #1,D0
        move.w  tf_YSize(A0),D1
        muls    D1,D0
        moveq   #0,D2
        move.w  D0,D2                   ; ymin = (row-1)*YSize
        moveq   #0,D0
        move.w  con_WidthPx(A5),D0      ; xmax
        move.w  D7,D3
        mulu    D1,D3
        moveq   #0,D1
        move.w  D3,D1                   ; ymax = lastrow*YSize
        move.l  D1,-(A7)
        move.l  D0,-(A7)
        move.l  D2,-(A7)
        clr.l   -(A7)                   ; xmin = 0
        move.l  A5,-(A7)                ; FIXED: pass con (ClearRect
        bsr.w   ClearRect               ;   needs the background pen)
        lea     $14(A7),A7
        movem.l (A7)+,D2-D3/D7/A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'J' -- erase in display.
;   0 (default): cursor to end of screen
;   1:           top of screen to cursor
;   2:           whole screen + home (ANSI.SYS behaviour)
;---------------------------------------------------------------------
Csi_J_EraseDisplay:                     ; was AJL_0_FF4
        subq.w  #4,A7
        move.l  A5,-(A7)
        movea.l $C(A7),A5               ; con
        moveq   #0,D0
        movea.l $10(A7),A0              ; params
        move.b  (A0),D0
        move.l  A6,$4(A7)
        tst.l   D0
        beq.b   .toEnd
        subq.l  #1,D0
        beq.b   .fromTop
        subq.l  #1,D0
        beq.b   .all
        bra.b   .done
.toEnd:
        move.w  con_Rows(A5),D0
        move.w  con_Row(A5),D1
        cmp.w   D0,D1
        bcc.b   .lastLine               ; already on the last row
        addq.w  #1,con_Row(A5)          ; temporarily row+1:
        move.w  con_Rows(A5),D0
        sub.w   con_Row(A5),D0
        moveq   #0,D1
        move.w  D0,D1
        move.l  D1,-(A7)
        move.l  A5,-(A7)
        bsr.w   EraseLinesFrom          ; blank all lines below
        addq.w  #8,A7
        subq.w  #1,con_Row(A5)
.lastLine:
        move.w  con_Cols(A5),D0
        sub.w   con_Col(A5),D0
        addq.w  #1,D0
        moveq   #0,D1
        move.w  D0,D1
        move.l  D1,-(A7)
        move.l  A5,-(A7)
        bsr.w   EraseCellsRight         ; blank cursor..EOL
        addq.w  #8,A7
        bra.b   .done
.fromTop:
        move.l  A5,-(A7)                ; FIXED: dropped the original's
        bsr.w   EraseTopToCursor        ;   redundant second
        bra.b   .pop4                   ;   EraseLineToCursor call
.all:
        move.l  A5,-(A7)
        bsr.w   ClearScreenHome
.pop4:
        addq.w  #4,A7
.done:
        movea.l (A7)+,A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'H' / 'f' -- set cursor position: params are row;col, 1-based,
; missing/0 become 1, both clamped to the window.
;---------------------------------------------------------------------
Csi_H_SetCursor:                        ; was AJL_0_1076
        subq.w  #4,A7
        movea.l $C(A7),A0               ; params
        movea.l $8(A7),A1               ; con
        moveq   #0,D0
        move.b  (A0),D0                 ; param[0] = row
        move.w  D0,con_Row(A1)
        move.l  A6,(A7)
        tst.w   D0
        bne.b   .rowGiven
        moveq   #1,D1
        bra.b   .clampRow
.rowGiven:
        move.w  con_Rows(A1),D1
        cmp.w   D1,D0
        bls.b   .rowOk
.clampRow:
        move.w  D1,con_Row(A1)
.rowOk:
        moveq   #0,D0
        move.b  1(A0),D0                ; param[1] = column
        move.w  D0,con_Col(A1)
        bne.b   .colGiven
        moveq   #1,D1
        bra.b   .clampCol
.colGiven:
        move.w  con_Cols(A1),D1
        cmp.w   D1,D0
        bls.b   .colOk
.clampCol:
        move.w  D1,con_Col(A1)
.colOk:
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'F' -- cursor to column 1, N lines up.
;---------------------------------------------------------------------
Csi_F_PrevLine:                         ; was AJL_0_10BE
        subq.w  #4,A7
        movea.l $8(A7),A0               ; con
        move.w  #1,con_Col(A0)
        move.l  A6,(A7)
        addq.l  #4,A7
        bra.w   CursorUpN               ; reuse the caller's arguments

;---------------------------------------------------------------------
; CSI 'C' -- cursor right N.  At the right edge with auto-wrap on it
; wraps (CR + cursor down); with auto-wrap off it stops there and
; abandons the remaining count.
;---------------------------------------------------------------------
Csi_C_CursorRight:                      ; was AJL_0_10D2
        subq.w  #4,A7
        movem.l D7/A5,-(A7)
        movea.l $10(A7),A5              ; con
        movea.l $14(A7),A0              ; params
        move.b  (A0),D7                 ; N (0 -> 1)
        move.l  A6,$8(A7)
        tst.b   D7
        bne.b   .loopCheck
        moveq   #1,D7
        bra.b   .loopCheck
.step:
        move.w  con_Col(A5),D0
        cmp.w   con_Cols(A5),D0
        bcs.b   .advance
        btst    #1,con_ModeFlags(A5)    ; at the edge: wrap allowed?
        beq.b   .done
        pea     1.W                     ; count (unused by callee)
        pea     g_ParamOne1(A4)         ; params = {1}
        move.l  A5,-(A7)
        bsr.w   Csi_E_NextLine          ; CR + cursor down 1
        lea     $C(A7),A7
        bra.b   .loopCheck
.advance:
        addq.w  #1,con_Col(A5)
.loopCheck:
        move.l  D7,D0
        subq.b  #1,D7
        tst.b   D0
        bne.b   .step
.done:
        movem.l (A7)+,D7/A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; CSI 'D' -- cursor left N.  At column 1 with auto-wrap on it jumps to
; the last column of the previous line (cursor up); with auto-wrap off
; it stops and abandons the remaining count.
;---------------------------------------------------------------------
Csi_D_CursorLeft:                       ; was AJL_0_1128
        subq.w  #4,A7
        movem.l D7/A5,-(A7)
        movea.l $10(A7),A5              ; con
        movea.l $14(A7),A0              ; params
        move.b  (A0),D7                 ; N (0 -> 1)
        move.l  A6,$8(A7)
        tst.b   D7
        bne.b   .loopCheck
        moveq   #1,D7
        bra.b   .loopCheck
.step:
        cmpi.w  #1,con_Col(A5)
        bhi.b   .retreat
        btst    #1,con_ModeFlags(A5)
        beq.b   .done
        move.w  con_Cols(A5),con_Col(A5)
        pea     1.W
        pea     g_ParamOne2(A4)         ; params = {1}
        move.l  A5,-(A7)
        bsr.w   CursorUpN               ; up one line
        lea     $C(A7),A7
        bra.b   .loopCheck
.retreat:
        subq.w  #1,con_Col(A5)
.loopCheck:
        move.l  D7,D0
        subq.b  #1,D7
        tst.b   D0
        bne.b   .step
.done:
        movem.l (A7)+,D7/A5
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; ToggleCursor(con) -- draw or remove the cursor block: COMPLEMENT
; RectFill over the character cell.  Called once before and once after
; every write, so the block XORs in and out.
;---------------------------------------------------------------------
ToggleCursor:                           ; was JL_0_1182
        suba.w  #$14,A7
        movem.l D2-D3/A5-A6,-(A7)
        movea.l $28(A7),A5              ; con
        move.l  A6,$20(A7)
        move.w  con_Cols(A5),D0
        move.w  con_Col(A5),D1
        cmp.w   D0,D1
        bls.b   .colOk
        move.w  D0,con_Col(A5)          ; clamp a stuck-right cursor
.colOk:
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        moveq   #COMPLEMENT,D0
        jsr     _LVOSetDrMd(A6)
        tst.w   con_PenMapOn(A5)        ; 1.5: with a pen table the ANSI
        beq.b   .maskDone               ;   pens are anywhere in the palette,
        moveq   #0,D0                   ;   so COMPLEMENT only flips the bits
        move.w  con_FgPen(A5),D0        ;   in which fg and bg differ: the
        bsr.w   MapPen                  ;   cell swaps fg and bg exactly
        move.l  D0,D1
        moveq   #0,D0
        move.w  con_BgPen(A5),D0
        bsr.w   MapPen
        eor.b   D1,D0
        movea.l con_RastPort(A5),A1
        move.b  D0,rp_Mask(A1)
.maskDone:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D0
        move.w  tf_XSize(A0),D0
        moveq   #0,D1
        move.w  con_Col(A5),D1
        subq.l  #1,D1
        movea.l $20(A7),A6
        bsr.w   Mul32                   ; xmin = (col-1)*XSize
        moveq   #0,D1
        move.w  tf_YSize(A0),D1
        moveq   #0,D2
        move.w  con_Row(A5),D2
        move.l  D2,D3
        subq.l  #1,D3
        move.l  D0,$14(A7)
        move.l  D3,D0
        bsr.w   Mul32                   ; ymin = (row-1)*YSize
        move.w  con_Col(A5),D1
        mulu    tf_XSize(A0),D1
        subq.l  #1,D1                   ; xmax = col*XSize-1
        mulu    tf_YSize(A0),D2
        subq.l  #1,D2                   ; ymax = row*YSize-1
        move.l  D1,$1C(A7)
        move.l  D0,D1                   ; ymin
        move.l  D2,D3                   ; ymax
        movea.l con_RastPort(A5),A1
        move.l  $14(A7),D0              ; xmin
        move.l  $1C(A7),D2              ; xmax
        movea.l g_GfxBase(A4),A6
        jsr     _LVORectFill(A6)        ; XOR the cell
        movea.l con_RastPort(A5),A1
        move.b  #$FF,rp_Mask(A1)        ; 1.5: all planes again
        moveq   #JAM2,D0
        jsr     _LVOSetDrMd(A6)         ; back to normal text mode
        movem.l (A7)+,D2-D3/A5-A6
        adda.w  #$14,A7
        rts

;---------------------------------------------------------------------
; ParserReset(con) -- clear all escape-sequence parser state.
;---------------------------------------------------------------------
ParserReset:                            ; was JL_0_1228
        subq.w  #4,A7
        movea.l $8(A7),A0               ; con
        clr.w   con_EscPending(A0)
        clr.w   con_InCsi(A0)
        clr.l   con_ParamIdx(A0)
        clr.b   con_Params(A0)          ; param[0] = 0
        clr.b   con_Params+1(A0)
        clr.w   con_RawCnt(A0)
        clr.b   con_RawBuf(A0)
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; FlushText(con) -- render the pending text run (con_TextBuf,
; con_TextLen chars, starting at column con_TextCol on the current
; row) with graphics Text().  Honours the reverse-video attribute by
; temporarily switching the draw mode to JAM2|INVERSVID.
;---------------------------------------------------------------------
FlushText:                              ; was JL_0_124E
        suba.w  #$10,A7
        movem.l D2-D3/A5-A6,-(A7)
        movea.l $24(A7),A5              ; con
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D0
        move.w  tf_XSize(A0),D0
        moveq   #0,D1
        move.w  con_TextCol(A5),D1
        subq.l  #1,D1
        bsr.w   Mul32                   ; x = (startcol-1)*XSize
        moveq   #0,D1
        move.w  tf_YSize(A0),D1
        moveq   #0,D3
        move.w  con_Row(A5),D3
        subq.l  #1,D3
        move.l  D0,$14(A7)              ; save x
        move.l  D3,D0
        bsr.w   Mul32                   ; (row-1)*YSize
        moveq   #0,D1                   ; FIXED: place the pen on the
        move.w  tf_Baseline(A0),D1      ;   font's real baseline (was
        add.l   D1,D0                   ;   "cell bottom - 2")
        move.l  A6,$1C(A7)
        move.l  D0,D1
        movea.l con_RastPort(A5),A1
        move.l  $14(A7),D0
        movea.l g_GfxBase(A4),A6
        jsr     _LVOGfxMove(A6)         ; Move(rp, x, y)
        btst    #5,con_AttrFlags(A5)    ; reverse video?
        movea.l $1C(A7),A6
        beq.b   .plainText
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        moveq   #JAM2_INVERS,D0
        jsr     _LVOSetDrMd(A6)
        lea     con_TextBuf(A5),A0
        moveq   #0,D0
        move.w  con_TextLen(A5),D0
        movea.l con_RastPort(A5),A1
        jsr     _LVOText(A6)
        movea.l con_RastPort(A5),A1
        moveq   #JAM2,D0
        jsr     _LVOSetDrMd(A6)
        movea.l $1C(A7),A6
        bra.b   .flushed
.plainText:
        lea     con_TextBuf(A5),A0
        moveq   #0,D0
        move.w  con_TextLen(A5),D0
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        jsr     _LVOText(A6)
.flushed:
        clr.w   con_TextLen(A5)
        movem.l (A7)+,D2-D3/A5-A6
        adda.w  #$10,A7
        rts

;---------------------------------------------------------------------
; CSI 'E' -- cursor to column 1, then N lines down (also the internal
; "CR+LF" used for wrapping and LNM linefeeds).
;---------------------------------------------------------------------
Csi_E_NextLine:                         ; was AJL_0_1312
        subq.w  #4,A7
        movea.l $8(A7),A0               ; con
        move.w  #1,con_Col(A0)
        move.l  A6,(A7)
        addq.l  #4,A7
        bra.w   CursorDownN             ; reuse the caller's arguments

;---------------------------------------------------------------------
; CursorDownN(con, params) -- CSI 'B': move the cursor down N rows.
; With "scroll at margins" (mode >1) set, running past the BOTTOM
; SCROLL MARGIN (con_RegBot, fix 13) scrolls the region contents up
; by the overshoot; a cursor below the region, or margin scrolling
; off, just clamps to the window.
; FIXED: removed the original's special case for units >= 2 with N=1
; (scrolled two lines, parked the cursor on the second to last row).
;---------------------------------------------------------------------
CursorDownN:                            ; was AJL_0_1326
        suba.w  #$10,A7
        movem.l D2-D7/A5-A6,-(A7)
        movea.l $34(A7),A5              ; con
        movea.l $38(A7),A0              ; params
        move.b  (A0),D6                 ; N (0 -> 1)
        move.l  A6,$2C(A7)
        tst.b   D6
        bne.b   .haveCount
        moveq   #1,D6
.haveCount:
        moveq   #0,D0
        move.b  D6,D0
        moveq   #0,D1
        move.w  con_Row(A5),D1
        add.l   D0,D1
        move.l  D1,D7                   ; D7 = target row
        btst    #2,con_ModeFlags(A5)    ; scroll at margins?
        beq.w   .noScrollMode
        moveq   #0,D0
        move.w  con_RegBot(A5),D0       ; FIXED(r): margin = region
        cmp.w   con_Row(A5),D0          ;   bottom; a cursor below the
        bcs.w   .noScrollMode           ;   region just clamps
        cmp.l   D0,D7
        bgt.b   .belowBottom
        move.w  D7,con_Row(A5)          ; fits: just move
        bra.b   .restoreA6
.belowBottom:
        moveq   #0,D1                   ; scroll the overshoot,
        move.w  D0,D1                   ;   cursor to the bottom margin
        sub.l   D1,D7                   ; lines = row+N-margin
        move.w  D0,con_Row(A5)
.doScroll:
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D0
        move.w  tf_YSize(A0),D0
        move.l  D7,D1
        bsr.w   Mul32                   ; dy = lines*YSize (moves up)
        moveq   #0,D1
        move.w  con_WidthPx(A5),D1
        subq.l  #1,D1                   ; xmax
        move.w  tf_YSize(A0),D2
        move.w  con_RegBot(A5),D3       ; FIXED(r): scroll only the
        mulu    D2,D3                   ;   region rows
        subq.l  #1,D3                   ; ymax = RegBot*YSize-1
        move.l  D1,$28(A7)
        move.l  D0,D1                   ; dy
        move.l  D3,D5
        moveq   #0,D3
        move.w  con_RegTop(A5),D3
        subq.w  #1,D3
        mulu    D2,D3                   ; ymin = (RegTop-1)*YSize
        movea.l con_RastPort(A5),A1
        move.l  $28(A7),D4
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0                   ; dx = 0
        move.l  D0,D2                   ; xmin = 0
        jsr     _LVOScrollRaster(A6)
.restoreA6:
        movea.l $2C(A7),A6
        bra.b   .done
.noScrollMode:
        moveq   #0,D0
        move.w  con_Rows(A5),D0
        cmp.l   D0,D7
        bge.b   .clampBottom
        move.w  D7,con_Row(A5)
        bra.b   .done
.clampBottom:
        move.w  D0,con_Row(A5)
.done:
        movem.l (A7)+,D2-D7/A5-A6
        adda.w  #$10,A7
        rts

;---------------------------------------------------------------------
; CursorUpN(con, params) -- CSI 'A': move the cursor up N rows.
; With "scroll at margins" set, running past the TOP SCROLL MARGIN
; (con_RegTop, fix 13) scrolls the region contents down by the
; deficit; a cursor above the region, or margin scrolling off, just
; clamps to row 1.
;---------------------------------------------------------------------
CursorUpN:                              ; was AJL_0_1406
        suba.w  #$10,A7
        movem.l D2-D6/A5-A6,-(A7)
        movea.l $30(A7),A5              ; con
        movea.l $34(A7),A0              ; params
        move.b  (A0),D6                 ; N (0 -> 1)
        move.l  A6,$28(A7)
        tst.b   D6
        bne.b   .haveCount
        moveq   #1,D6
.haveCount:
        moveq   #0,D0
        move.b  D6,D0
        moveq   #0,D1
        move.w  con_Row(A5),D1
        sub.l   D0,D1                   ; D1 = target row
        btst    #2,con_ModeFlags(A5)
        beq.b   .noScrollMode
        moveq   #0,D0
        move.w  con_RegTop(A5),D0       ; FIXED(r): margin = region
        cmp.w   con_Row(A5),D0          ;   top; a cursor above the
        bhi.b   .noScrollMode           ;   region just clamps
        cmp.l   D0,D1
        blt.b   .aboveTop
        move.w  D1,con_Row(A5)          ; fits: just move
        bra.b   .restoreA6
.aboveTop:
        move.w  D0,con_Row(A5)          ; cursor to the top margin
        sub.l   D1,D0                   ; lines = margin - target
        move.l  D0,D1                   ;   (subsumes fix 11)
        movea.l g_RastPort(A4),A1
        movea.l rp_Font(A1),A0
        moveq   #0,D2
        move.w  tf_YSize(A0),D2
        move.l  D2,D3
        neg.l   D3
        move.l  D3,D0
        bsr.w   Mul32                   ; dy = -lines*YSize (moves down)
        moveq   #0,D1
        move.w  con_WidthPx(A5),D1
        subq.l  #1,D1                   ; xmax
        move.w  con_RegBot(A5),D3       ; FIXED(r): scroll only the
        mulu    D2,D3                   ;   region rows
        subq.l  #1,D3                   ; ymax = RegBot*YSize-1
        move.l  D1,$24(A7)
        move.l  D0,D1                   ; dy
        move.l  D3,D5
        moveq   #0,D3
        move.w  con_RegTop(A5),D3
        subq.w  #1,D3
        mulu    D2,D3                   ; ymin = (RegTop-1)*YSize
        movea.l con_RastPort(A5),A1
        move.l  $24(A7),D4
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0                   ; dx = 0
        move.l  D0,D2                   ; xmin = 0
        jsr     _LVOScrollRaster(A6)
.restoreA6:
        movea.l $28(A7),A6
        bra.b   .done
.noScrollMode:
        moveq   #1,D0
        cmp.l   D0,D1
        ble.b   .clampTop
        move.w  D1,con_Row(A5)
        bra.b   .done
.clampTop:
        move.w  #1,con_Row(A5)
.done:
        movem.l (A7)+,D2-D6/A5-A6
        adda.w  #$10,A7
        rts

;---------------------------------------------------------------------
; ClearScreenHome(con) -- fill the whole window with the background
; pen, cursor home.  Used by FF and CSI 2J.
;---------------------------------------------------------------------
ClearScreenHome:                        ; was JL_0_14C8
        subq.w  #4,A7
        movem.l A5-A6,-(A7)
        movea.l $10(A7),A5              ; con
        move.l  A6,$8(A7)
        movea.l con_RastPort(A5),A1
        movea.l g_GfxBase(A4),A6
        moveq   #0,D0
        move.w  con_BgPen(A5),D0        ; FIXED: clear to the current
        bsr.w   MapPen                  ;   background pen (was 0)
        jsr     _LVOSetRast(A6)
        moveq   #1,D0
        move.w  D0,con_Row(A5)
        move.w  D0,con_Col(A5)
        movem.l (A7)+,A5-A6
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; ConWrite(con, data, length) -- the heart of the device: interpret a
; CMD_WRITE buffer.  length -1 means NUL-terminated.
;
; Printable characters are collected into con_TextBuf and only drawn
; (FlushText) when a control character, a CSI sequence, a wrap or the
; end of the buffer forces it -- one Text() call per run.
; The cursor block is XORed away on entry and XORed back at the end.
;
; Parser states:
;   con_EscPending: ESC seen, only '[' continues, anything else drops
;   con_InCsi:      collecting parameters:
;       '0'-'9'  param = param*10+digit  (byte, wraps > 255)
;       ';'      next parameter (max 24)
;       ESC      restart sequence
;       >= '@'   final character: look up (prefix,char) in g_EscTable
;                and call the handler(con, params, paramcount)
;       other    remembered in con_RawBuf (prefixes '>', '?', ...)
;   CAN/SUB abort the sequence in any state.
;---------------------------------------------------------------------
ConWrite:                               ; was JL_0_14F6
        subq.w  #4,A7
        movem.l D4-D7/A2-A3/A5-A6,-(A7)
        move.l  $30(A7),D7              ; D7 = length
        movea.l $28(A7),A5              ; A5 = con
        movea.l $2C(A7),A2              ; A2 = data
        move.l  A5,-(A7)
        move.l  A6,$24(A7)
        bsr.w   ToggleCursor            ; remove the cursor block
        bsr.w   CheckResize             ; 1.5: follow a window resize
        addq.w  #4,A7
        move.l  D7,D0
        addq.l  #1,D0
        bne.w   .nextChar               ; length != -1
        movea.l A2,A0                   ; length = -1: strlen(data)
.strlen:
        tst.b   (A0)+
        bne.b   .strlen
        subq.l  #1,A0
        suba.l  A2,A0
        move.l  A0,D7
        bra.w   .nextChar

.charLoop:
        move.b  (A2)+,D4                ; D4 = current character
        moveq   #$18,D0                 ; CAN?
        cmp.b   D0,D4
        beq.b   .cancel
        moveq   #$1A,D0                 ; SUB?
        cmp.b   D0,D4
        bne.b   .noCancel
.cancel:
        move.l  A5,-(A7)                ; CAN/SUB: abort any sequence
        bsr.w   ParserReset
        addq.w  #4,A7
        bra.w   .nextChar

.noCancel:
        tst.w   con_EscPending(A5)      ; --- state: after ESC -------
        beq.b   .notEsc
        clr.w   con_EscPending(A5)
        moveq   #'[',D0
        cmp.b   D0,D4
        bne.w   .nextChar               ; ESC + anything else: drop
        move.l  A5,-(A7)
        bsr.w   ParserReset
        addq.w  #4,A7
        moveq   #1,D0
        move.w  D0,con_InCsi(A5)        ; enter CSI collection
        bra.w   .nextChar

.notEsc:
        tst.w   con_InCsi(A5)           ; --- state: inside CSI ------
        beq.w   .normalChar
        moveq   #'@',D0
        cmp.b   D0,D4
        bcc.w   .dispatch               ; >= '@': final character
        moveq   #'0',D0
        cmp.b   D0,D4
        bcs.b   .notDigit
        moveq   #'9',D0
        cmp.b   D0,D4
        bhi.b   .notDigit
        subi.b  #'0',D4                 ; digit: param = param*10+d
        andi.w  #$F,D4                  ; digit value, stale bits off
        movea.l con_ParamIdx(A5),A0
        move.l  A0,D1
        addi.l  #con_Params,D1          ; D1 = byte offset of the param
        moveq   #0,D0
        move.b  0(A5,D1.L),D0
        mulu    #10,D0                  ; old*10 (word, max 2550)
        add.w   D4,D0                   ; + digit
        cmpi.w  #255,D0                 ; FIXED: saturate at 255
        bls.b   .storeParam             ;   instead of wrapping mod 256
        move.w  #255,D0                 ;   ("260" used to read as 4)
.storeParam:
        move.b  D0,0(A5,D1.L)
        bra.w   .nextChar

.notDigit:
        moveq   #';',D0
        cmp.b   D0,D4
        bne.b   .notSemi
        lea     con_ParamIdx(A5),A3     ; ';': next parameter
        addq.l  #1,(A3)
        movea.l (A3),A0
        move.l  A0,D0
        addi.l  #con_Params,D0
        clr.b   0(A5,D0.L)              ; new param starts at 0
        cmpi.l  #23,(A3)+
        bls.w   .nextChar
        moveq   #23,D0                  ; clamp to 24 parameters
        move.l  D0,con_ParamIdx(A5)
        bra.w   .nextChar

.notSemi:
        moveq   #$1B,D0                 ; ESC inside CSI: restart
        cmp.b   D0,D4
        beq.w   .escChar
        move.w  con_RawCnt(A5),D0       ; other: keep the raw char
        moveq   #10,D1                  ;   (prefixes '>', '?', ...)
        cmp.w   D1,D0
        bcc.w   .nextChar               ; buffer full: drop
        addq.w  #1,con_RawCnt(A5)
        moveq   #0,D1
        move.w  D0,D1
        addi.l  #con_RawBuf,D1
        move.b  D4,0(A5,D1.L)
        moveq   #0,D1
        move.w  con_RawCnt(A5),D1
        addi.l  #con_RawBuf,D1
        clr.b   0(A5,D1.L)              ; keep it NUL terminated
        bra.w   .nextChar

.dispatch:                              ; --- final char: run handler -
        moveq   #'m',D0                 ; 1.5: every sequence but SGR
        cmp.b   D0,D4                   ;   (colour at the line's edge)
        beq.b   .wrapKept               ;   cancels a deferred wrap
        clr.w   con_WrapPending(A5)
.wrapKept:
        moveq   #0,D5                   ; D5 = table index
        bra.b   .entryLoop
.tryEntry:
        cmp.b   5(A3),D4                ; entry final char match?
        bne.b   .nextEntry
        move.b  con_RawBuf(A5),D0       ; entry prefix match?
        lea     g_EscTable(A4),A0       ;   (0 = no prefix collected)
        cmp.b   4(A0,D6.L),D0
        bne.b   .nextEntry
        tst.w   con_TextLen(A5)         ; matched: flush pending text
        beq.b   .noFlush
        move.l  A5,-(A7)
        bsr.w   FlushText
        addq.w  #4,A7
.noFlush:
        moveq   #0,D0
        move.b  D4,D0
        move.w  D0,con_InCsi(A5)        ; remember the final char
        movea.l (A3),A0                 ; handler function
        move.l  con_ParamIdx(A5),-(A7)  ; (con, params, paramcount)
        pea     con_Params(A5)
        move.l  A5,-(A7)
        jsr     (A0)
        lea     $C(A7),A7
        bra.b   .csiDone
.nextEntry:
        addq.w  #1,D5
.entryLoop:
        swap    D5                      ; (zero-extend D5 to long)
        clr.w   D5
        swap    D5
        move.l  D5,D0
        asl.l   #2,D0
        sub.l   D5,D0
        add.l   D0,D0                   ; D6 = index * 6
        move.l  D0,D6
        lea     g_EscTable(A4),A3
        adda.l  D6,A3                   ; A3 = table entry
        tst.l   (A3)                    ; NULL function = end of table
        bne.b   .tryEntry               ;   -> unknown sequence: drop
.csiDone:
        clr.w   con_InCsi(A5)
        bra.w   .nextChar

.normalChar:                            ; --- state: plain text ------
        moveq   #$20,D0
        cmp.b   D0,D4
        movea.l $20(A7),A6
        bcc.b   .ctrlSwitch             ; printable: switch defaults
        tst.w   con_TextLen(A5)         ; control char: flush pending
        beq.b   .ctrlSwitch             ;   text first
        move.l  A5,-(A7)
        bsr.w   FlushText
        addq.w  #4,A7
.ctrlSwitch:
        moveq   #0,D0
        move.b  D4,D0
        subq.l  #7,D0                   ; $07 BEL
        beq.b   .bell
        subq.l  #1,D0                   ; $08 BS
        beq.b   .backspace
        subq.l  #1,D0                   ; $09 HT
        beq.w   .tab
        subq.l  #1,D0                   ; $0A LF
        beq.w   .linefeed
        subq.l  #1,D0                   ; $0B VT
        beq.w   .vertTab
        subq.l  #1,D0                   ; $0C FF
        beq.w   .formfeed
        subq.l  #1,D0                   ; $0D CR
        beq.w   .carriageRet
        subq.l  #1,D0                   ; $0E SO
        beq.w   .shiftOut
        subq.l  #1,D0                   ; $0F SI
        beq.w   .shiftIn
        moveq   #$C,D1
        sub.l   D1,D0                   ; $1B ESC
        beq.b   .escChar
        subi.l  #$80,D0                 ; $9B 8-bit CSI
        beq.b   .csi8bit
        bra.w   .printable

.escChar:
        move.l  A5,-(A7)
        bsr.w   ParserReset
        addq.w  #4,A7
        move.w  #1,con_EscPending(A5)
        bra.w   .nextChar
.csi8bit:
        move.l  A5,-(A7)
        bsr.w   ParserReset
        addq.w  #4,A7
        move.w  #1,con_InCsi(A5)
        bra.w   .nextChar
.bell:
        movea.l g_Window(A4),A0
        movea.l wd_WScreen(A0),A0
        movea.l g_IntuiBase(A4),A6
        jsr     _LVODisplayBeep(A6)
        movea.l $20(A7),A6
        bra.w   .nextChar
.backspace:
        clr.w   con_WrapPending(A5)     ; moves the cursor: no wrap due
        move.w  con_Col(A5),D0
        moveq   #1,D1
        cmp.w   D1,D0
        bls.w   .nextChar
        subq.w  #1,con_Col(A5)
        bra.w   .nextChar
.tabStep:
        addq.w  #1,con_Col(A5)
        moveq   #0,D0
        move.w  con_Col(A5),D0
        move.l  D0,D1
        addi.l  #con_TabStops,D1
        tst.b   0(A5,D1.L)              ; landed on a tab stop?
        bne.w   .nextChar
.tab:
        clr.w   con_WrapPending(A5)     ; moves the cursor: no wrap due
        move.w  con_Col(A5),D0
        cmp.w   con_Cols(A5),D0
        bcs.b   .tabStep                ; advance until stop or edge
        bra.w   .nextChar
.linefeed:
        clr.w   con_WrapPending(A5)     ; moves the cursor: no wrap due
        btst    #0,con_ModeFlags(A5)    ; LNM set?
        beq.b   .lfPlain
        clr.l   -(A7)
        pea     g_ParamOne3(A4)         ; LF = CR + down 1
        move.l  A5,-(A7)
        bsr.w   Csi_E_NextLine
        lea     $C(A7),A7
        bra.w   .nextChar
.lfPlain:
        clr.l   -(A7)
        pea     g_ParamOne4(A4)         ; LF = down 1 (may scroll)
        move.l  A5,-(A7)
        bsr.w   CursorDownN
        lea     $C(A7),A7
        bra.w   .nextChar
.vertTab:
        clr.w   con_WrapPending(A5)     ; moves the cursor: no wrap due
        clr.l   -(A7)
        pea     g_ParamOne5(A4)         ; VT = up 1
        move.l  A5,-(A7)
        bsr.w   CursorUpN
        lea     $C(A7),A7
        bra.w   .nextChar
.formfeed:
        clr.w   con_WrapPending(A5)     ; moves the cursor: no wrap due
        move.l  A5,-(A7)                ; FF = clear screen + home
        bsr.w   ClearScreenHome
        addq.w  #4,A7
        bra.w   .nextChar
.carriageRet:
        clr.w   con_WrapPending(A5)     ; moves the cursor: no wrap due
        move.w  #1,con_Col(A5)
        bra.w   .nextChar
.shiftOut:
        move.b  #$80,con_CharMask(A5)   ; SO: IBM upper charset
        bra.w   .nextChar
.shiftIn:
        clr.b   con_CharMask(A5)        ; SI: back to lower charset
        bra.b   .nextChar

.printable:
        tst.w   con_WrapPending(A5)     ; 1.5: deferred wrap -- the last
        beq.b   .noWrapDue              ;   column was filled: this
        clr.w   con_WrapPending(A5)     ;   character starts the next
        clr.l   -(A7)                   ;   line (CR + down 1, scrolls)
        pea     g_ParamOne6(A4)
        move.l  A5,-(A7)
        bsr.w   Csi_E_NextLine
        lea     $C(A7),A7
.noWrapDue:
        move.w  con_TextLen(A5),D0      ; first char of a run:
        bne.b   .append                 ;   remember the start column
        move.w  con_Col(A5),D1
        move.w  D1,con_TextCol(A5)
.append:
        moveq   #0,D0
        move.w  con_TextLen(A5),D0
        move.b  D4,D1
        or.b    con_CharMask(A5),D1     ; apply the SO/SI mask
        move.b  D1,con_TextBuf(A5,D0.L)
        move.w  con_Col(A5),D0
        addq.w  #1,D0
        move.w  D0,con_Col(A5)
        addq.w  #1,con_TextLen(A5)
        cmp.w   con_Cols(A5),D0
        bls.b   .runCheck               ; still inside the line
        move.l  A5,-(A7)                ; ran off the right edge:
        bsr.w   FlushText               ;   draw the run, then wrap
        addq.w  #4,A7
        btst    #1,con_ModeFlags(A5)    ; auto-wrap?
        beq.b   .stickRight
        move.w  #1,con_WrapPending(A5)  ; 1.5 FIXED: wrap when the next
                                        ;   printable comes (VT100/ANSI
                                        ;   terminals): an 80-column line
                                        ;   followed by CR LF was two lines
.stickRight:
        move.w  con_Cols(A5),con_Col(A5) ; park at the edge
        bra.w   .nextChar
.runCheck:
        cmpi.w  #TEXTBUF_SIZE,con_TextLen(A5) ; FIXED: never let a run
        bcs.b   .nextChar               ;   outgrow con_TextBuf --
        move.l  A5,-(A7)                ;   flush and continue (the
        bsr.w   FlushText               ;   next char reopens the run)
        addq.w  #4,A7

.nextChar:
        move.l  D7,D0
        subq.l  #1,D7
        tst.l   D0
        bne.w   .charLoop

        tst.w   con_TextLen(A5)         ; end of buffer: flush the
        beq.b   .finish                 ;   last run
        move.l  A5,-(A7)
        bsr.w   FlushText
        addq.w  #4,A7
.finish:
        move.l  A5,$28(A7)              ; re-arm the argument and
        movem.l (A7)+,D4-D7/A2-A3/A5-A6 ; tail-call: redraw cursor
        addq.l  #4,A7
        bra.w   ToggleCursor

;---------------------------------------------------------------------
; UnitOpen -- device specific part of DevOpen.
; In:  D0 = unit number, A0 = ioreq (io_Data = Window *), A6 = clone
; Out: D0 = 0 ok / 1 failed
;
; Stores window/rastport/unit in the globals, spawns the
; "IBMCON_Handler" process (entry HandlerProc, 4K stack) and hands it
; the clone base in a startup message ($14(msg)), then waits for the
; reply.  Fails cleanly when the handler could not create its command
; port (the handler exits by itself in that case).
;---------------------------------------------------------------------
UnitOpen:                               ; was JL_0_185A
        suba.w  #$20,A7
        movem.l A4-A6,-(A7)
        lea     dev_Globals(A6),A4
        move.l  D0,g_UnitNum(A4)
        move.l  IO_DATA(A0),g_Window(A4)
        movea.l g_Window(A4),A1
        move.l  wd_RPort(A1),g_RastPort(A4)
        move.l  A6,$10(A7)
        clr.l   -(A7)                   ; TAG_DONE
        pea     g_ProcName(A4)          ; "IBMCON_Handler"
        move.l  #NP_Name,-(A7)
        pea     $1000.W                 ; 4K stack
        move.l  #NP_StackSize,-(A7)
        pea     HandlerProc(PC)
        move.l  #NP_Entry,-(A7)
        movea.l g_DOSBase(A4),A6
        move.l  A7,D1                   ; tag list on the stack
        jsr     _LVOCreateNewProc(A6)
        lea     $1C(A7),A7
        movea.l D0,A5                   ; A5 = new process
        tst.l   D0
        movea.l $10(A7),A6
        bne.b   .haveProc
        moveq   #1,D0                   ; process creation failed
        bra.b   .done
.haveProc:
        move.w  #4,$26(A7)              ; msg mn_Length = 4
        clr.l   -(A7)                   ; CreatePort(NULL, 0)
        clr.l   -(A7)
        movea.l $18(A7),A6
        bsr.w   CreatePort_
        addq.w  #8,A7
        move.l  D0,$22(A7)              ; msg mn_ReplyPort = temp port
        move.b  #NT_MESSAGE,$1C(A7)     ; msg ln_Type
        move.l  A6,$28(A7)              ; msg+$14 = clone base payload
        lea     pr_MsgPort(A5),A0       ; to the new process's port
        lea     $14(A7),A1              ; message lives on our stack
        movea.l AbsExecBase.W,A6
        jsr     _LVOPutMsg(A6)
        movea.l $22(A7),A0
        jsr     _LVOWaitPort(A6)        ; wait for the handler's reply
        move.l  $22(A7),-(A7)           ; FIXED: always free the temp
        movea.l $14(A7),A6              ;   reply port (used to leak on
        bsr.w   DeletePort_             ;   handler failure)
        addq.w  #4,A7
        movea.l $10(A7),A6
        moveq   #0,D0                   ; success
        tst.l   g_CmdPort(A4)
        bne.b   .done
        moveq   #1,D0                   ; FIXED: handler has no command
.done:                                  ;   port -> report the failure
        movem.l (A7)+,A4-A6
        adda.w  #$20,A7
        rts

;---------------------------------------------------------------------
; UnitClose -- device specific part of DevClose.
; In: A0 = ioreq, A6 = clone base.
; Builds a fake IORequest with io_Command = CMD_DIE on the stack,
; sends it to the handler's command port and waits for the reply,
; then deletes the handler's command port.
;---------------------------------------------------------------------
UnitClose:                              ; was JL_0_1918
        suba.w  #$24,A7
        movem.l A4-A6,-(A7)
        movea.l A0,A5                   ; A5 = ioreq
        lea     dev_Globals(A6),A4
        move.l  A6,$C(A7)
        movea.l AbsExecBase.W,A6
        jsr     -$29A(A6)               ; CreateMsgPort (V36+)
        move.l  D0,$1E(A7)              ; fake ioreq mn_ReplyPort
        move.w  #CMD_DIE,$2C(A7)        ; fake ioreq io_Command
        move.l  IO_UNIT(A5),$28(A7)     ; fake ioreq io_Unit
        movea.l g_CmdPort(A4),A0
        lea     $10(A7),A1              ; fake ioreq on our stack
        jsr     _LVOPutMsg(A6)
        movea.l $1E(A7),A0
        jsr     _LVOWaitPort(A6)        ; handler replies under Forbid
        movea.l $1E(A7),A0
        jsr     -$2A0(A6)               ; DeleteMsgPort
        move.l  g_CmdPort(A4),-(A7)
        movea.l $10(A7),A6
        bsr.w   DeletePort_             ; free the handler's port
        addq.w  #4,A7
        movem.l (A7)+,A4-A6
        adda.w  #$24,A7
        rts

;---------------------------------------------------------------------
; DevBeginIO -- device BeginIO vector.  A1 = ioreq, A6 = clone.
; Only CMD_WRITE is accepted; it is always made asynchronous
; (IOF_QUICK cleared) and forwarded to the handler's command port.
; Everything else is replied immediately with IOERR_NOCMD.
;---------------------------------------------------------------------
DevBeginIO:                             ; was AJL_0_1976
        subq.w  #4,A7
        movem.l A4/A6,-(A7)
        lea     dev_Globals(A6),A4
        clr.b   IO_ERROR(A1)
        bclr    #0,IO_FLAGS(A1)         ; never quick
        moveq   #0,D0
        move.w  IO_COMMAND(A1),D0
        move.l  A6,$8(A7)
        cmpi.l  #IBMCMD_SETPENS,D0      ; 1.5: pen table -> handler too
        beq.b   .forward
        subq.l  #CMD_WRITE,D0
        bne.b   .badCmd
.forward:
        movea.l g_CmdPort(A4),A0
        movea.l AbsExecBase.W,A6
        jsr     _LVOPutMsg(A6)          ; handler will ReplyMsg
        bra.b   .done
.badCmd:
        move.b  #IOERR_NOCMD,IO_ERROR(A1)
        movea.l AbsExecBase.W,A6
        jsr     _LVOReplyMsg(A6)
.done:
        movea.l $8(A7),A6
        movem.l (A7)+,A4/A6
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; DevAbortIO -- device AbortIO vector: does nothing.
;---------------------------------------------------------------------
DevAbortIO:                             ; was AJL_0_19C0
        subq.w  #4,A7
        move.l  A4,-(A7)
        lea     dev_Globals(A6),A4
        movea.l (A7)+,A4
        addq.w  #4,A7
        rts

;---------------------------------------------------------------------
; NewList_(A0 = list) -- amiga.lib NewList.
;---------------------------------------------------------------------
NewList_:                               ; was JL_0_19D4
        clr.l   4(A0)                   ; lh_Tail
        move.l  A0,8(A0)                ; lh_TailPred
        addq.l  #4,A0
        move.l  A0,-(A0)                ; lh_Head -> &lh_Tail
        rts

;---------------------------------------------------------------------
; CreatePort_(name, pri) -- amiga.lib CreatePort clone.
; Returns the port or NULL.
;---------------------------------------------------------------------
CreatePort_:                            ; was JL_0_19E4
        movea.l 4(A7),A0                ; name
        move.l  8(A7),D0                ; pri
        movem.l D6-D7/A3/A5-A6,-(A7)
        move.l  D0,D7
        moveq   #-1,D0
        movea.l AbsExecBase.W,A6
        movea.l A0,A5
        jsr     _LVOAllocSignal(A6)
        move.b  D0,D6                   ; FIXED: sign-extend the result
        ext.w   D6                      ;   so a -1 failure is caught
        ext.l   D6                      ;   (was zero-extended: $FF
        bmi.b   .noSignal               ;   read as "signal 255")
        moveq   #MP_SIZE,D0
        move.l  #MEMF_PUB_CLEAR,D1
        jsr     _LVOAllocMem(A6)
        movea.l D0,A3
        tst.l   D0
        beq.b   .noMem
        lea     LN_NAME(A3),A0
        move.l  A5,(A0)+                ; ln_Name
        move.l  D7,D0
        move.b  D0,LN_PRI(A3)
        move.b  #NT_MSGPORT,LN_TYPE(A3)
        clr.b   (A0)+                   ; mp_Flags = PA_SIGNAL
        suba.l  A1,A1
        move.b  D6,(A0)+                ; mp_SigBit
        jsr     _LVOFindTask(A6)
        move.l  D0,MP_SIGTASK(A3)
        move.l  A5,D0                   ; named?
        beq.b   .unnamed
        movea.l A3,A1
        jsr     _LVOAddPort(A6)         ; public port
        bra.b   .retPort
.unnamed:
        lea     MP_MSGLIST(A3),A0
        bsr.w   NewList_
.retPort:
        move.l  A3,D0
        bra.b   .done
.noMem:
        move.l  D6,D0
        movea.l AbsExecBase.W,A6
        jsr     _LVOFreeSignal(A6)
.noSignal:
        moveq   #0,D0
.done:
        movem.l (A7)+,D6-D7/A3/A5-A6
        rts

;---------------------------------------------------------------------
; DeletePort_(port) -- amiga.lib DeletePort clone.
;---------------------------------------------------------------------
DeletePort_:                            ; was JL_0_1A62
        movea.l 4(A7),A0
        movem.l A5-A6,-(A7)
        movea.l A0,A5
        tst.l   LN_NAME(A5)             ; public?
        beq.b   .noRemove
        movea.l A5,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVORemPort(A6)
.noRemove:
        st      LN_TYPE(A5)             ; poison the node
        moveq   #-1,D0
        move.l  D0,MP_MSGLIST(A5)
        moveq   #0,D0
        move.b  MP_SIGBIT(A5),D0
        movea.l AbsExecBase.W,A6
        jsr     _LVOFreeSignal(A6)
        movea.l A5,A1
        moveq   #MP_SIZE,D0
        jsr     _LVOFreeMem(A6)
        movem.l (A7)+,A5-A6
        rts

;---------------------------------------------------------------------
; RunAutoList -- SAS/C __STI/__STD driver.
; Walks the pointer list at *++g_AutoCursor, calling each function
; until a NULL entry stops it.  On the FIRST pass (g_AutoInitDone
; clear) a non-zero return aborts the remaining constructors; on
; later passes (destructors) everything is always called.
; The cursor is left ON the NULL entry, so the next call continues
; with the following list (constructors -> destructors).
; Returns the last called function's result.
;---------------------------------------------------------------------
RunAutoList:                            ; was AJL_0_1AA4
        move.l  D7,-(A7)
        moveq   #0,D7
        addq.l  #4,g_AutoCursor(A4)
        bra.b   .loopCheck
.checkRun:
        tst.b   g_AutoInitDone(A4)
        bne.b   .call                   ; destructor pass: always call
        tst.l   D7
        bne.b   .advance                ; a constructor failed: skip
.call:
        movea.l g_AutoCursor(A4),A1
        movea.l (A1),A0
        jsr     (A0)
        move.l  D0,D7
.advance:
        addq.l  #4,g_AutoCursor(A4)
.loopCheck:
        movea.l g_AutoCursor(A4),A0
        tst.l   (A0)
        bne.b   .checkRun
        move.b  #1,g_AutoInitDone(A4)
        move.l  D7,D0
        move.l  (A7)+,D7
        rts

;---------------------------------------------------------------------
; RunConstructors -- open graphics/intuition/dos via the constructor
; list.  A6 = clone base.  Returns 0 ok / 1 failed.
; (The list address stored on the stack is a vestige -- RunAutoList
; finds the list through g_AutoCursor instead.)
;---------------------------------------------------------------------
RunConstructors:                        ; was JL_0_1ADC
        subq.w  #8,A7
        move.l  A4,-(A7)
        lea     CtorList,A0
        lea     dev_Globals(A6),A4
        move.l  A0,$8(A7)
        move.l  A6,$4(A7)
        tst.l   $8(A7)
        beq.b   .ok
        movea.l AutoRunnerPtr,A0        ; -> RunAutoList
        jsr     (A0)
        tst.l   D0
        beq.b   .ok
        moveq   #1,D0                   ; a constructor failed
        SKIPNEXT
.ok:
        moveq   #0,D0
        movea.l (A7)+,A4
        addq.w  #8,A7
        rts

;---------------------------------------------------------------------
; RunDestructors -- close the libraries via the destructor list.
; A6 = clone base.  The autoinit cursor already rests on the ctor
; list's NULL terminator, so RunAutoList continues into DtorList.
;---------------------------------------------------------------------
RunDestructors:                         ; was JL_0_1B10
        subq.w  #8,A7
        move.l  A4,-(A7)
        lea     DtorList,A0
        lea     dev_Globals(A6),A4
        move.l  A0,$8(A7)
        move.l  A6,$4(A7)
        tst.l   $8(A7)
        beq.b   .done
        movea.l AutoRunnerPtr,A0        ; -> RunAutoList
        jsr     (A0)
.done:
        movea.l (A7)+,A4
        addq.w  #8,A7
        rts

;---------------------------------------------------------------------
; FillBytes(ptr, count, value) -- SAS/C memset-style helper.
;---------------------------------------------------------------------
FillBytes:                              ; was JL_0_1B3C
        movea.l 4(A7),A0
        move.l  8(A7),D0
        move.l  $C(A7),D1
        tst.l   D0
        ble.b   .done
.loop:
        move.b  D1,(A0)+
        subq.l  #1,D0
        bne.b   .loop
.done:
        rts

;---------------------------------------------------------------------
; SDivMod32 -- SAS/C signed 32/32 divide.
; In: D0 = dividend, D1 = divisor.  Out: D0 = quotient, D1 = remainder.
;---------------------------------------------------------------------
SDivMod32:                              ; was JL_0_1B54
        tst.l   D0
        bpl.w   .posDividend
        neg.l   D0
        tst.l   D1
        bpl.w   .negOnly
        neg.l   D1
        bsr.w   UDivMod32               ; -a / -b: quotient +, rem -
        neg.l   D1
        rts
.negOnly:
        bsr.w   UDivMod32               ; -a / b: both negative
        neg.l   D0
        neg.l   D1
        rts
.posDividend:
        tst.l   D1
        bpl.w   UDivMod32               ; a / b: plain unsigned
        neg.l   D1
        bsr.w   UDivMod32               ; a / -b: quotient negative
        neg.l   D0
        rts

;---------------------------------------------------------------------
; UDivMod32 -- SAS/C unsigned 32/32 divide.
; In: D0 = dividend, D1 = divisor.  Out: D0 = quotient, D1 = remainder.
; Divisor < $10000 uses two divu's; otherwise shift-normalised divide.
;---------------------------------------------------------------------
UDivMod32:                              ; was JL_0_1B86
        move.l  D2,-(A7)
        swap    D1
        move.w  D1,D2
        bne.w   .bigDivisor
        swap    D0
        swap    D1
        swap    D2
        move.w  D0,D2
        beq.w   .lowOnly
        divu    D1,D2                   ; high word
        move.w  D2,D0
.lowOnly:
        swap    D0
        move.w  D0,D2
        divu    D1,D2                   ; low word
        move.w  D2,D0
        swap    D2
        move.w  D2,D1                   ; remainder
        move.l  (A7)+,D2
        rts
.bigDivisor:
        move.l  D3,-(A7)
        moveq   #$10,D3                 ; normalise the divisor into
        cmpi.w  #$100,D1                ;   the top bit, remember the
        bcc.w   .n8                     ;   shift count in D3
        rol.l   #8,D1
        subq.w  #8,D3
.n8:
        cmpi.w  #$1000,D1
        bcc.w   .n4
        rol.l   #4,D1
        subq.w  #4,D3
.n4:
        cmpi.w  #$4000,D1
        bcc.w   .n2
        rol.l   #2,D1
        subq.w  #2,D3
.n2:
        tst.w   D1
        bmi.w   .n1
        rol.l   #1,D1
        subq.w  #1,D3
.n1:
        move.w  D0,D2
        lsr.l   D3,D0
        swap    D2
        clr.w   D2
        lsr.l   D3,D2
        swap    D3
        divu    D1,D0                   ; estimate quotient
        move.w  D0,D3
        move.w  D2,D0
        move.w  D3,D2
        swap    D1
        mulu    D1,D2
        sub.l   D2,D0                   ; correct the estimate
        bcc.w   .noFix
        subq.w  #1,D3
        add.l   D1,D0
.noFix:
        moveq   #0,D1
        move.w  D3,D1
        swap    D3
        rol.l   D3,D0
        swap    D0
        exg.l   D0,D1                   ; D0 = quotient, D1 = remainder
        move.l  (A7)+,D3
        move.l  (A7)+,D2
        rts

        ds.w    1                       ; alignment pad

;---------------------------------------------------------------------
; Mul32 -- SAS/C 32x32 multiply.  D0 = D0 * D1.
;---------------------------------------------------------------------
Mul32:                                  ; was JL_0_1C18
        movem.l D2-D3,-(A7)
        move.l  D0,D2
        move.l  D1,D3
        swap    D2
        swap    D3
        mulu    D1,D2                   ; hi(a)*lo(b)
        mulu    D0,D3                   ; lo(a)*hi(b)
        mulu    D1,D0                   ; lo(a)*lo(b)
        add.w   D3,D2
        swap    D2
        clr.w   D2
        add.l   D2,D0
        movem.l (A7)+,D2-D3
        rts

;---------------------------------------------------------------------
; LibOpenError(A0 = library name) -- SAS/C "__oslibversion" style
; error report: "Can't open version %ld of <libname>".
; With g_OwnConsole set it would open its own "con:10/10/320/80/"
; window (never happens in this device: the flag is always 0 in the
; runtime copy of the globals, so Output() of the calling process is
; used).  Finally sets pr_Result2 = ERROR_INVALID_RESIDENT_LIBRARY.
;---------------------------------------------------------------------
LibOpenError:                           ; was JL_0_1C38
        suba.w  #$34,A7
        movem.l D2-D3/D7/A2-A3/A5-A6,-(A7)
        lea     DosName(PC),A1
        movea.l AbsExecBase.W,A6
        moveq   #0,D0
        movea.l A0,A5                   ; A5 = failing library's name
        jsr     _LVOOpenLibrary(A6)
        movea.l D0,A3                   ; A3 = DOSBase
        tst.l   g_OwnConsole(A4)
        bne.b   .openWindow
        movea.l A3,A6
        jsr     _LVOOutput(A6)          ; fh = Output()
        move.l  D0,D7
        bra.b   .haveFile
.openWindow:
        lea     g_ConSpec(A4),A0        ; fh = Open("con:...", NEWFILE)
        move.l  A0,D1
        move.l  #MODE_NEWFILE,D2
        movea.l A3,A6
        jsr     _LVODosOpen(A6)
        move.l  D0,D7
.haveFile:
        tst.l   D7
        beq.b   .noFile
        lea     PutChProc(PC),A0        ; RawDoFmt the message into
        move.l  A3,-(A7)                ;   the stack buffer
        lea     g_LibVersion(A4),A1     ; %ld argument
        lea     $22(A7),A3              ; A3 = output buffer
        movea.l AbsExecBase.W,A6
        movea.l A0,A2
        lea     FmtCantOpen(PC),A0
        jsr     _LVORawDoFmt(A6)
        movea.l (A7)+,A3
        lea     $1E(A7),A0              ; strlen of the formatted part
        movea.l A0,A1
.fmtLen:
        tst.b   (A1)+
        bne.b   .fmtLen
        subq.l  #1,A1
        move.l  A0,D2
        suba.l  A0,A1
        movea.l A3,A6
        move.l  A1,D3
        move.l  D7,D1
        jsr     _LVODosWrite(A6)        ; "Can't open version %ld of "
        movea.l A5,A0
.nameLen:
        tst.b   (A0)+
        bne.b   .nameLen
        subq.l  #1,A0
        suba.l  A5,A0
        move.l  A0,D3
        move.l  D7,D1
        move.l  A5,D2
        jsr     _LVODosWrite(A6)        ; the library name
        lea     NewlineStr(PC),A0
        move.l  D7,D1
        move.l  A0,D2
        moveq   #1,D3
        jsr     _LVODosWrite(A6)        ; "\n"
        tst.l   g_OwnConsole(A4)
        beq.b   .noFile
        moveq   #100,D1                 ; own window: let the user read
        add.l   D1,D1                   ;   it (Delay(200) = 4s), then
        jsr     _LVODelay(A6)           ;   close the window
        move.l  D7,D1
        jsr     _LVODosClose(A6)
.noFile:
        movea.l A3,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOCloseLibrary(A6)
        suba.l  A1,A1
        jsr     _LVOFindTask(A6)
        moveq   #ERROR_INVALID_RESIDENT_LIBRARY,D1
        movea.l D0,A0
        move.l  D1,pr_Result2(A0)       ; IoErr() = 122
        movem.l (A7)+,D2-D3/D7/A2-A3/A5-A6
        adda.w  #$34,A7
        rts

DosName:
        dc.b    "dos.library",0
FmtCantOpen:
        dc.b    "Can't open version %ld of ",0,0
PutChProc:                              ; RawDoFmt character stuffer,
        dc.w    $16C0                   ;   move.b d0,(a3)+
        dc.w    $4E75                   ;   rts
        dc.w    0                       ;   (pad)
NewlineStr:
        dc.b    10,0
        dc.w    $4E71                   ; nop (pad)

;---------------------------------------------------------------------
; Constructor / destructor set: open and close dos, graphics and
; intuition.  Each keeps the base twice (g_XxxBase for use,
; g_XxxBaseC for the destructor).  On failure: report and return 1.
; g_LibVersion is always 0 here, so any version is accepted.
;---------------------------------------------------------------------
OpenDosLib:                             ; was AJL_0_1D3C
        move.l  A6,-(A7)
        lea     DosName2(PC),A1
        move.l  g_LibVersion(A4),D0
        movea.l AbsExecBase.W,A6
        jsr     _LVOOpenLibrary(A6)
        move.l  D0,g_DOSBase(A4)
        move.l  D0,g_DOSBaseC(A4)
        bne.b   .ok
        lea     DosName2(PC),A0
        bsr.w   LibOpenError
        moveq   #1,D0
        SKIPNEXT
.ok:
        moveq   #0,D0
        movea.l (A7)+,A6
        rts

DosName2:
        dc.b    "dos.library",0

CloseDosLib:                            ; was AJL_0_1D76
        move.l  A6,-(A7)
        move.l  g_DOSBaseC(A4),D0
        beq.b   .done
        movea.l D0,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOCloseLibrary(A6)
        suba.l  A0,A0
        move.l  A0,g_DOSBase(A4)
        move.l  A0,g_DOSBaseC(A4)
.done:
        movea.l (A7)+,A6
        rts

OpenGfxLib:                             ; was AJL_0_1D98
        move.l  A6,-(A7)
        lea     GfxName(PC),A1
        move.l  g_LibVersion(A4),D0
        movea.l AbsExecBase.W,A6
        jsr     _LVOOpenLibrary(A6)
        move.l  D0,g_GfxBase(A4)
        move.l  D0,g_GfxBaseC(A4)
        bne.b   .ok
        lea     GfxName(PC),A0
        bsr.w   LibOpenError
        moveq   #1,D0
        SKIPNEXT
.ok:
        moveq   #0,D0
        movea.l (A7)+,A6
        rts

GfxName:
        dc.b    "graphics.library",0,0

CloseGfxLib:                            ; was AJL_0_1DD8
        move.l  A6,-(A7)
        move.l  g_GfxBaseC(A4),D0
        beq.b   .done
        movea.l D0,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOCloseLibrary(A6)
        suba.l  A0,A0
        move.l  A0,g_GfxBase(A4)
        move.l  A0,g_GfxBaseC(A4)
.done:
        movea.l (A7)+,A6
        rts

OpenIntuiLib:                           ; was AJL_0_1DF8
        move.l  A6,-(A7)
        lea     IntuiName(PC),A1
        move.l  g_LibVersion(A4),D0
        movea.l AbsExecBase.W,A6
        jsr     _LVOOpenLibrary(A6)
        move.l  D0,g_IntuiBase(A4)
        move.l  D0,g_IntuiBaseC(A4)
        bne.b   .ok
        lea     IntuiName(PC),A0
        bsr.w   LibOpenError
        moveq   #1,D0
        SKIPNEXT
.ok:
        moveq   #0,D0
        movea.l (A7)+,A6
        rts

IntuiName:
        dc.b    "intuition.library",0

CloseIntuiLib:                          ; was AJL_0_1E38
        move.l  A6,-(A7)
        move.l  g_IntuiBaseC(A4),D0
        beq.b   .done
        movea.l D0,A1
        movea.l AbsExecBase.W,A6
        jsr     _LVOCloseLibrary(A6)
        suba.l  A0,A0
        move.l  A0,g_IntuiBase(A4)
        move.l  A0,g_IntuiBaseC(A4)
.done:
        movea.l (A7)+,A6
        rts

;=====================================================================
        SECTION "Segment1",DATA
        cnop    0,4
;=====================================================================
; RTF_AUTOINIT init table: exec's InitResident/MakeLibrary reads this.
;---------------------------------------------------------------------
InitTable:                              ; was SegmentBeginn1
        dc.l    DEV_POSSIZE             ; positive size of the base
        dc.l    FuncTable               ; the six device vectors
        ds.l    1                       ; no InitStruct data
        dc.l    DevInit                 ; init function

;=====================================================================
        SECTION "Segment2",CODE
        cnop    0,4
;=====================================================================
; Device function table (becomes the LVO jump table, $24 bytes).
;---------------------------------------------------------------------
FuncTable:                              ; was SegmentBeginn2
        dc.l    DevOpen                 ; -6
        dc.l    DevClose                ; -12
        dc.l    DevExpunge              ; -18
        dc.l    DevNull                 ; -24 reserved/ExtFunc
        dc.l    DevBeginIO              ; -30
        dc.l    DevAbortIO              ; -36
        dc.l    -1                      ; table terminator

DevName:                                ; was AL_2_1C
        dc.b    "ibmcon.device",0,0
        dc.b    0
DevIdString:                            ; was AL_2_2C
        dc.b    "ibmcon.device 1.6",0,0
        dc.b    0

;=====================================================================
        SECTION "Segment3",DATA
        cnop    0,4
;=====================================================================
; GlobalsInit -- image of the globals area.  DevInit copies the first
; $138 bytes to root_base+$3C; DevOpen clones them again for every
; opener.  All g_* offsets index into this block.
;---------------------------------------------------------------------
GlobalsInit:                            ; was SegmentBeginn3
        ds.l    1                       ; $000: (unused)
        dc.b    0                       ; $004
        dc.b    "$VER: ibmcon.device 1.6 (Sep 27 2026)",0,0

;--- $02C: CSI dispatch table ----------------------------------------
; 6 bytes per entry: function pointer, prefix char (0 = none), final
; char.  Linear search, first match wins, NULL function = end.
EscTable:                               ; = g_EscTable, was AL_3_2C
        dc.l    Csi_AtSign_InsertChars
        dc.b    0,'@'                   ; insert characters
        dc.l    CursorUpN
        dc.b    0,'A'                   ; cursor up
        dc.l    CursorDownN
        dc.b    0,'B'                   ; cursor down
        dc.l    Csi_C_CursorRight
        dc.b    0,'C'                   ; cursor right
        dc.l    Csi_D_CursorLeft
        dc.b    0,'D'                   ; cursor left
        dc.l    Csi_E_NextLine
        dc.b    0,'E'                   ; next line
        dc.l    Csi_F_PrevLine
        dc.b    0,'F'                   ; previous line
        dc.l    Csi_H_SetCursor
        dc.b    0,'H'                   ; set cursor position
        dc.l    Csi_J_EraseDisplay
        dc.b    0,'J'                   ; erase display
        dc.l    Csi_K_EraseLine
        dc.b    0,'K'                   ; erase line
        dc.l    Csi_L_InsertLines
        dc.b    0,'L'                   ; insert lines
        dc.l    Csi_M_DeleteLines
        dc.b    0,'M'                   ; delete lines
        dc.l    Csi_P_DeleteChars
        dc.b    0,'P'                   ; delete characters
        dc.l    Csi_R_Stub
        dc.b    0,'R'                   ; (cursor position report)
        dc.l    Csi_r_SetRegion
        dc.b    0,'r'                   ; set scroll region (DECSTBM)
        dc.l    Csi_S_ScrollUp
        dc.b    0,'S'                   ; scroll up (FIXED)
        dc.l    Csi_T_ScrollDown
        dc.b    0,'T'                   ; scroll down
        dc.l    Csi_t_SetRows
        dc.b    0,'t'                   ; set rows
        dc.l    Csi_H_SetCursor
        dc.b    0,'f'                   ; set cursor position (HVP)
        dc.l    Csi_h_SetMode
        dc.b    0,'h'                   ; set mode
        dc.l    Csi_l_ResetMode
        dc.b    0,'l'                   ; reset mode
        dc.l    Csi_h_SetMode
        dc.b    '>','h'                 ; set private mode  (>1)
        dc.l    Csi_l_ResetMode
        dc.b    '>','l'                 ; reset private mode
        dc.l    Csi_h_SetMode
        dc.b    '?','h'                 ; set DEC mode      (?7)
        dc.l    Csi_l_ResetMode
        dc.b    '?','l'                 ; reset DEC mode
        dc.l    Csi_m_SetGraphics
        dc.b    0,'m'                   ; select graphic rendition
        dc.l    Csi_n_Stub
        dc.b    0,'n'                   ; (device status report)
        dc.l    Csi_s_SaveCursor
        dc.b    0,'s'                   ; save cursor
        dc.l    Csi_u_RestoreCursor
        dc.b    0,'u'                   ; restore cursor
        dc.w    0                       ; $0DA: NULL terminator
        dc.l    0                       ; $0DC: (pad)

;--- $0E0: the six {1,0} pseudo parameter blocks ---------------------
        dc.b    1,0                     ; g_ParamOne1 (wrap right)
        dc.b    1,0                     ; g_ParamOne2 (wrap left)
        dc.b    1,0                     ; g_ParamOne3 (LF with CR)
        dc.b    1,0                     ; g_ParamOne4 (plain LF)
        dc.b    1,0                     ; g_ParamOne5 (VT = up)
        dc.b    1,0                     ; g_ParamOne6 (printable wrap)

;--- $0EC ------------------------------------------------------------
        dc.b    "IBMCON_Handler",0,0    ; g_ProcName

;--- $0FC: SAS/C autoinit machinery ----------------------------------
AutoRunnerPtr:                          ; = g_AutoRunner, was AL_3_FC
        dc.l    RunAutoList
CtorList:                               ; = g_CtorList, was AL_3_100
        dc.l    OpenGfxLib
        dc.l    OpenIntuiLib
        dc.l    OpenDosLib
        ds.l    1                       ; NULL terminator
DtorList:                               ; = g_DtorList, was AL_3_110
        dc.l    CloseDosLib
        dc.l    CloseIntuiLib
        dc.l    CloseGfxLib
        dc.w    0,0                     ; $11C: NULL terminator
        dc.l    $FC                     ; $120: g_AutoCursor initial
                                        ;   value; the clone reloc adds
                                        ;   &globals -> &AutoRunnerPtr
                                        ;   (i.e. CtorList - 4)
;--- $124 ------------------------------------------------------------
        dc.b    "con:10/10/320/80/",0,0 ; g_ConSpec (error window)
        dc.b    0                       ; $137: pad

;--- $138: relocation table (NOT part of the copied globals) ---------
; DevInit keeps a heap copy; DevOpen adds the clone's globals address
; to each listed globals offset.  One entry: g_AutoCursor.
GlobalsRelocs:
        dc.l    1                       ; entry count
        dc.l    $120                    ; offset of g_AutoCursor

        End
