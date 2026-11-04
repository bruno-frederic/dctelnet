; sshlib/ssh_div.asm -- 32-bit division helpers the 68000 code generator calls
; (the 68000 has no 32/32 divide). Replaces vc.lib's __divu / __divs so the
; library links without it. Only the 68000 build links this object; 68020 and
; up use the DIVU.L / DIVS.L instructions.
; Convention (vbcc, read off vc.lib use: the compiler takes a % b from d1):
; dividend in d0, divisor in d1; quotient returns in d0, remainder in d1; d2
; and up are preserved. Division by zero returns quotient 0, remainder 0.
        xdef    __divu
        xdef    __divs

        SECTION "CODE",CODE

__divu:
        movem.l d2-d3,-(sp)
        tst.l   d1
        beq.s   .zero
        cmp.l   #$10000,d1
        bhs.s   .big
; Divisor fits 16 bits: two DIVU steps (high word, then low word).
        move.l  d0,d2
        clr.w   d2
        swap    d2              ; d2 = high word of the dividend
        divu    d1,d2           ; d2 = remainder<<16 | high quotient word
        move.w  d2,d3           ; d3 = high quotient word
        move.w  d0,d2           ; d2 = remainder<<16 | low dividend word
        divu    d1,d2           ; d2.w = low quotient word
        move.w  d3,d0
        swap    d0
        move.w  d2,d0
        swap    d2              ; remainder: high word of the last DIVU
        moveq   #0,d1
        move.w  d2,d1
        bra.s   .done
; Divisor wider than 16 bits: shift-subtract, 32 steps.
.big:
        moveq   #0,d2           ; remainder
        moveq   #31,d3
.loop:
        add.l   d0,d0           ; shift the next dividend bit into the remainder
        addx.l  d2,d2
        bcs.s   .sub            ; remainder overflowed 32 bits: must subtract
        cmp.l   d1,d2
        bcs.s   .next
.sub:
        sub.l   d1,d2
        addq.l  #1,d0           ; quotient bit
.next:
        dbra    d3,.loop
        move.l  d2,d1
        bra.s   .done
.zero:
        moveq   #0,d0
        moveq   #0,d1
.done:
        movem.l (sp)+,d2-d3
        rts

__divs:
        movem.l d2-d3,-(sp)
        moveq   #0,d2           ; d2.w = 1 when the quotient is negative
        moveq   #0,d3           ; d3.w = 1 when the remainder is negative
        tst.l   d0
        bpl.s   .pos0
        neg.l   d0
        eori.w  #1,d2
        moveq   #1,d3
.pos0:
        tst.l   d1
        bpl.s   .pos1
        neg.l   d1
        eori.w  #1,d2
.pos1:
        bsr     __divu
        tst.w   d2
        beq.s   .q
        neg.l   d0
.q:
        tst.w   d3
        beq.s   .out
        neg.l   d1
.out:
        movem.l (sp)+,d2-d3
        rts
