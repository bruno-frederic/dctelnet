; src/crypto/bn_muladd.s -- the one multiply loop of the SSH crypto
; (bignum.c: RSA, group 14; x25519.c): r[0..n) += a[0..n) * w, 32-bit
; limbs, returning the limb carried out. vbcc turns a 64-bit product in C
; into a library call per limb; here it is 4 mulu.w (68000) or 1 mulu.l
; (68020+). bignum.c picks one at run time from SysBase->AttnFlags.
;
; bn_limb Bn_MulAdd000(a0 r, a1 a, d0.w n, d1.l w)   (n >= 1)
; bn_limb Bn_MulAdd020(a0 r, a1 a, d0.w n, d1.l w)

        section code,code

        xdef    _Bn_MulAdd000
        xdef    _Bn_MulAdd020

_Bn_MulAdd000:
        movem.l d2-d7,-(sp)
        move.w  d1,d6                   ; wl
        swap    d1
        move.w  d1,d7                   ; wh
        moveq   #0,d5                   ; carry
        subq.w  #1,d0
.loop:
        move.l  (a1)+,d1                ; ah:al
        move.w  d1,d2
        mulu.w  d6,d2                   ; lo  = al*wl
        move.w  d1,d3
        mulu.w  d7,d3                   ; mid = al*wh
        swap    d1
        move.w  d1,d4
        mulu.w  d6,d4                   ;       ah*wl
        mulu.w  d7,d1                   ; hi  = ah*wh
        add.l   d4,d3                   ; mid += ah*wl (X: bit 32 of mid)
        moveq   #0,d4                   ; (moveq keeps X)
        addx.w  d4,d4
        swap    d4
        add.l   d4,d1                   ; hi += that bit << 16
        move.l  d3,d4
        swap    d4
        clr.w   d4                      ; mid << 16
        clr.w   d3
        swap    d3                      ; mid >> 16
        add.l   d4,d2                   ; lo += mid << 16
        addx.l  d3,d1                   ; hi += (mid >> 16) + X
        add.l   d5,d2                   ; lo += carry in
        moveq   #0,d5
        addx.l  d5,d1
        add.l   (a0),d2                 ; lo += r[i]
        addx.l  d5,d1
        move.l  d2,(a0)+
        move.l  d1,d5                   ; carry out
        dbra    d0,.loop
        move.l  d5,d0
        movem.l (sp)+,d2-d7
        rts

        machine 68020

_Bn_MulAdd020:
        movem.l d2-d5,-(sp)
        moveq   #0,d4                   ; carry
        moveq   #0,d5
        subq.w  #1,d0
.loop:
        move.l  (a1)+,d2
        mulu.l  d1,d3:d2                ; d3:d2 = a[i] * w
        add.l   d4,d2
        addx.l  d5,d3
        add.l   (a0),d2
        addx.l  d5,d3
        move.l  d2,(a0)+
        move.l  d3,d4
        dbra    d0,.loop
        move.l  d4,d0
        movem.l (sp)+,d2-d5
        rts

; t[0..2n) = 2 * t + the squares x[i]^2 at limb 2i (Bn_Square's last
; step: the doubled cross products plus the diagonal).
;
; void Bn_DoubleAddSquares000(a0 t, a1 x, d0.w n)
; void Bn_DoubleAddSquares020(a0 t, a1 x, d0.w n)

        machine 68000

        xdef    _Bn_DoubleAddSquares000
        xdef    _Bn_DoubleAddSquares020

_Bn_DoubleAddSquares000:
        movem.l d2-d7/a2,-(sp)
        move.l  a0,a2
        move.w  d0,d7
        add.w   d7,d7
        subq.w  #1,d7                   ; 2n - 1
        and.b   #$EF,ccr                ; X = 0 (bit 4)
.dbl:   move.l  (a2),d1
        roxl.l  #1,d1                   ; (move keeps X)
        move.l  d1,(a2)+
        dbra    d7,.dbl
        moveq   #0,d6                   ; carry between limb pairs
        subq.w  #1,d0
.sq:    move.l  (a1)+,d1                ; ah:al
        move.w  d1,d2
        mulu.w  d2,d2                   ; lo = al^2
        move.w  d1,d3
        swap    d1
        mulu.w  d1,d3                   ; mid = al*ah (doubled below)
        mulu.w  d1,d1                   ; hi = ah^2
        moveq   #0,d4
        add.l   d3,d3                   ; mid * 2, X = bit 32
        addx.w  d4,d4
        swap    d4
        add.l   d4,d1                   ; hi += that bit << 16
        move.l  d3,d4
        swap    d4
        clr.w   d4                      ; mid << 16
        clr.w   d3
        swap    d3                      ; mid >> 16
        add.l   d4,d2
        addx.l  d3,d1                   ; hi:lo = x[i]^2
        moveq   #0,d5
        add.l   d6,d2                   ; + carry in
        addx.l  d5,d1
        add.l   d2,(a0)+
        addx.l  d5,d1                   ; (hi <= 2^32 - 2: no overflow)
        move.l  d5,d6
        add.l   d1,(a0)+
        addx.l  d5,d6                   ; carry out of the pair
        dbra    d0,.sq
        movem.l (sp)+,d2-d7/a2
        rts

        machine 68020

_Bn_DoubleAddSquares020:
        movem.l d2-d7/a2,-(sp)
        move.l  a0,a2
        move.w  d0,d7
        add.w   d7,d7
        subq.w  #1,d7
        and.b   #$EF,ccr
.dbl:   move.l  (a2),d1
        roxl.l  #1,d1
        move.l  d1,(a2)+
        dbra    d7,.dbl
        moveq   #0,d6
        moveq   #0,d5
        subq.w  #1,d0
.sq:    move.l  (a1)+,d2
        mulu.l  d2,d1:d2                ; d1:d2 = x[i]^2
        add.l   d6,d2
        addx.l  d5,d1
        add.l   d2,(a0)+
        addx.l  d5,d1
        moveq   #0,d6
        add.l   d1,(a0)+
        addx.l  d5,d6
        dbra    d0,.sq
        movem.l (sp)+,d2-d7/a2
        rts
