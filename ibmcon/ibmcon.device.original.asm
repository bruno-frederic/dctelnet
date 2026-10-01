; In_Go Reassembler ibmcon.device: 16.12.2022 23:27:06
NP_Entry EQU $800003EB
NP_StackSize EQU $800003F3
NP_Name EQU $800003F4
_SysBase EQU $4
MinByteSgnd EQU $80
HUNK_RELOC_8__MODE_NEWFILE EQU $3EE
MemfPublic_MemfClear EQU $10001
MaxIntSgnd EQU $7FFFFFFF
 MC68020
Optimize68020 EQU 1
OWNINSTRUKTION MACRO
 dc.w \1
 ENDM
 SECTION "Segment0",CODE
 cnop 0,4
SegmentBeginn0:
  moveq.l #$00,D0
 rtS 
 
AL_0_4:
  dc.b "Jü"
  dc.l AL_0_4
  dc.l AL_0_1E
  dc.b $80,$01,$03,$00 ;....
  dc.l AL_2_1C
  dc.l AL_2_2C
  dc.l SegmentBeginn1
AL_0_1E:
  ds.w 1
AJL_0_20:
  subq.w #8,A7
  movem.l D7/A2-A3/A5-A6,-(A7)
  movea.l D0,A5
  move.l A0,$22(A5)
  move.b #$3,$8(A5)
  move.l #AL_2_1C,$A(A5)
  move.b #$6,$E(A5)
  lea $1,A1
  move.l A1,D0
  lea $14(A5),A3
  move.w D0,(A3)+
  lea _SysBase,A1
  move.l A1,D0
  move.w D0,(A3)+
  move.l #AL_2_2C,(A3)+
  lea $2E(A5),A3
  clr.l (A3)+
  move.l A5,(A3)+
  move.l #$24,(A3)+
  lea $3C(A5),A1
  lea $4E,A2
  move.l A2,D0
  move.l D0,D1
  asl.l #2,D1
  move.l A6,$18(A7)
  lea SegmentBeginn3,A2
  OWNINSTRUKTION $0C40 
JL_0_88:
  move.b (A2)+,(A1)+
  subq.l #1,D1
   bcc.b JL_0_88
  asl.l #2,D0
  lea SegmentBeginn3,A3
  adda.l D0,A3
  move.l (A3),D0
  movea.l $18(A7),A6
   beq.b JL_0_C6
  move.l D0,D7
  asl.l #2,D7
  addq.l #4,D7
  move.l D7,D0
  moveq.l #$01,D1
  movea.l $4.W,A6
AllocMem SET -$C6
   jsr AllocMem(A6)
  move.l D0,$2E(A5)
  movea.l $18(A7),A6
   beq.b JL_0_C8
  movea.l D0,A1
  OWNINSTRUKTION $0C40 
JL_0_C0:
  move.b (A3)+,(A1)+
  subq.l #1,D7
   bcc.b JL_0_C0
JL_0_C6:
  move.l A5,D0
JL_0_C8:
  movem.l (A7)+,D7/A2-A3/A5-A6
  addq.w #8,A7
 rtS 
 
AJL_0_D0:
  suba.w #$18,A7
  movem.l D2/D7/A2-A3/A5-A6,-(A7)
  move.l D0,D7
  lea $24(A7),A2
  move.l A1,(A2)+
  movea.l $4.W,A1
  movea.l A6,A5
  addq.w #1,$20(A5)
  bclr #$3,$E(A5)
  movea.l $32(A5),A0
  move.l A1,$1C(A7)
  lea $174,A1
  move.l A1,D0
  add.l $36(A0),D0
  moveq.l #$3C,D1
  add.l D1,D0
  move.l A6,(A2)+
  move.l A6,(A2)+
  move.l #MemfPublic_MemfClear,D1
  movea.l $4.W,A6
AllocMem SET -$C6
   jsr AllocMem(A6)
  movea.l D0,A3
  move.l D0,$20(A7)
  movea.l $2C(A7),A6
   beq.w JL_0_1F2
  movea.l $32(A5),A0
  movea.l A5,A1
  move.l $36(A0),D1
  suba.l D1,A1
  lea $4E,A6
  move.l A6,D0
  asl.l #2,D0
  add.l D1,D0
  moveq.l #$3C,D1
  add.l D1,D0
  movea.l A3,A0
  OWNINSTRUKTION $0C40 
JL_0_148:
  move.b (A1)+,(A0)+
  subq.l #1,D0
   bcc.b JL_0_148
  movea.l $32(A5),A0
  movea.l A3,A2
  adda.l $36(A0),A2
  clr.l $2E(A2)
  lea $3C(A2),A3
  movea.l $32(A2),A0
  movea.l $2E(A0),A1
  movea.l $2C(A7),A6
  move.l A1,D1
   beq.b JL_0_188
  lea $4(A1),A5
  move.l (A1),D2
 bra.b JL_0_184
 
JL_0_178:
  move.l A3,D0
  move.l (A5)+,D1
  add.l D0,D1
  movea.l D1,A0
  add.l D0,(A0)
  subq.l #1,D2
JL_0_184:
  tst.l D2
   bgt.b JL_0_178
JL_0_188:
  movea.l $1C(A7),A0
  cmpi.w #$24,$14(A0)
  movea.l $2C(A7),A6
   bcs.b JL_0_1A0
  movea.l $4.W,A6
CacheClearU SET -$27C
   jsr CacheClearU(A6)
JL_0_1A0:
  movea.l $24(A7),A5
  movea.l A2,A6
  move.l A6,$14(A5)
   bsr.w JL_0_1ADC
  movea.l $2C(A7),A6
  tst.l D0
   bne.b JL_0_1C8
  move.l D7,D0
  movea.l A5,A0
  movea.l A2,A6
   bsr.w JL_0_185A
  tst.l D0
  movea.l $2C(A7),A6
   beq.b JL_0_202
JL_0_1C8:
  movea.l A2,A6
   bsr.w JL_0_1B10
  movea.l $32(A2),A0
  lea $174,A1
  move.l A1,D0
  add.l $36(A0),D0
  moveq.l #$3C,D1
  add.l D1,D0
  movea.l $20(A7),A1
  movea.l $4.W,A6
FreeMem SET -$D2
   jsr FreeMem(A6)
  movea.l $2C(A7),A6
JL_0_1F2:
  movea.l $28(A7),A1
  movea.l $32(A1),A0
  subq.w #1,$20(A0)
  moveq.l #$00,D0
  OWNINSTRUKTION $0C40 
JL_0_202:
  move.l A2,D0
  movem.l (A7)+,D2/D7/A2-A3/A5-A6
  adda.w #$18,A7
 rtS 
 
AJL_0_20E:
  subq.w #8,A7
  movem.l D7/A3/A5-A6,-(A7)
  movea.l A6,A5
  moveq.l #$00,D7
  move.l A6,$14(A7)
  movea.l $32(A5),A0
  cmpa.l A5,A0
   beq.b JL_0_256
  movea.l A1,A0
  movea.l A5,A6
   bsr.w JL_0_1918
   bsr.w JL_0_1B10
  movea.l $32(A5),A3
  move.l $36(A3),D0
  suba.l D0,A5
  lea $174,A1
  move.l A1,D0
  add.l $36(A3),D0
  moveq.l #$3C,D1
  add.l D1,D0
  movea.l A5,A1
  movea.l $4.W,A6
FreeMem SET -$D2
   jsr FreeMem(A6)
  movea.l A3,A5
JL_0_256:
  subq.w #1,$20(A5)
  movea.l $14(A7),A6
   bne.b JL_0_270
  btst #$3,$E(A5)
   beq.b JL_0_270
  movea.l A5,A6
   bsr.w AJL_0_27A
  move.l D0,D7
JL_0_270:
  move.l D7,D0
  movem.l (A7)+,D7/A3/A5-A6
  addq.w #8,A7
 rtS 
 
AJL_0_27A:
  subq.w #8,A7
  movem.l D2/D7/A5-A6,-(A7)
  moveq.l #$00,D7
  movea.l $32(A6),A5
  bset #$3,$E(A5)
  move.l A6,$10(A7)
  move.l A6,$14(A7)
  tst.w $20(A5)
   bne.b JL_0_2E0
  move.l $2E(A5),D0
   beq.b JL_0_2B4
  movea.l D0,A0
  move.l (A0),D1
  asl.l #2,D1
  addq.l #4,D1
  movea.l D0,A1
  move.l D1,D0
  movea.l $4.W,A6
FreeMem SET -$D2
   jsr FreeMem(A6)
JL_0_2B4:
  move.l $22(A5),D7
  movea.l A5,A1
  movea.l $4.W,A6
Remove SET -$FC
   jsr Remove(A6)
  moveq.l #$00,D0
  move.w $12(A5),D0
  moveq.l #$00,D1
  move.w $10(A5),D1
  move.l D1,D2
  add.l D0,D2
  movea.l A5,A1
  moveq.l #$00,D0
  move.w D1,D0
  suba.l D0,A1
  move.l D2,D0
FreeMem SET -$D2
   jsr FreeMem(A6)
JL_0_2E0:
  move.l D7,D0
  movem.l (A7)+,D2/D7/A5-A6
  addq.w #8,A7
 rtS 
 

JL_0_2EC:
  suba.w #$210,A7
  movem.l A3-A6,-(A7)
  move.l A6,$10(A7)
  suba.l A1,A1
  movea.l $4.W,A6
FindTask SET -$126
   jsr FindTask(A6)
  movea.l D0,A0
pr_MsgPort SET $5C
  lea pr_MsgPort(A0),A5
 bra.b JL_0_314
 
JL_0_30A:
  movea.l A5,A0
  movea.l $4.W,A6
WaitPort SET -$180
   jsr WaitPort(A6)
JL_0_314:
  movea.l $10(A7),A6
  movea.l A5,A0
  movea.l $4.W,A6
GetMsg SET -$174
   jsr GetMsg(A6)
  movea.l D0,A3
  tst.l D0
  movea.l $10(A7),A6
   beq.b JL_0_30A
Class SET $14
  movea.l Class(A3),A6
  lea $3C(A6),A4
  clr.l -(A7)
  clr.l -(A7)
  movea.l $18(A7),A6
   bsr.w JL_0_19E4
  addq.w #8,A7
  move.l D0,$14C(A4)
  movea.l A3,A1
  movea.l $4.W,A6
ReplyMsg SET -$17A
   jsr ReplyMsg(A6)
  tst.l $14C(A4)
  movea.l $10(A7),A6
   beq.w JL_0_44E
  move.l $148(A4),$1E(A7)
  lea $61(A7),A5
  bclr #$0,(A5)
  bset #$2,(A5)
  bset #$1,(A5)+
  movea.l $144(A4),A1
  movea.l $2E(A1),A0
  movea.l $50(A0),A1
  movea.l $4(A1),A0
  moveq.l #$00,D0
  move.b $5(A0),D0
  subq.l #1,D0
   beq.b JL_0_398
  subq.l #1,D0
   beq.b JL_0_3A8
  subq.l #1,D0
   beq.b JL_0_3B8
  subq.l #1,D0
   beq.b JL_0_3C4
JL_0_398:
  lea $56(A7),A5
  move.w #$0001,(A5)+
  clr.w (A5)+
  move.w #$0001,(A5)+
 bra.b JL_0_3D2
 
JL_0_3A8:
  lea $56(A7),A5
  move.w #$0001,(A5)+
  moveq.l #$01,D0
  move.w D0,(A5)+
  move.w D0,(A5)+
 bra.b JL_0_3D2
 
JL_0_3B8:
  lea $56(A7),A5
  move.w #$0001,(A5)+
  clr.w (A5)+
 bra.b JL_0_3D0
 
JL_0_3C4:
  lea $56(A7),A5
  move.w #$0001,(A5)+
  move.w #$0001,(A5)+
JL_0_3D0:
  clr.w (A5)+
JL_0_3D2:
  pea $16(A7)
   bsr.w JL_0_458
  pea $1A(A7)
   bsr.w JL_0_1182
  addq.w #8,A7
JL_0_3E4:
  movea.l $14C(A4),A0
  movea.l $4.W,A6
WaitPort SET -$180
   jsr WaitPort(A6)
 bra.b JL_0_432
 
JL_0_3F2:
  moveq.l #$00,D0
  move.w $1C(A5),D0
  subq.l #3,D0
   beq.b JL_0_414
  subi.l #$7FED,D0
   bne.b JL_0_428
  movea.l $4.W,A6
Forbid SET -$84
   jsr Forbid(A6)
  movea.l A5,A1
ReplyMsg SET -$17A
   jsr ReplyMsg(A6)
 bra.b JL_0_44E
 
JL_0_414:
  move.l $24(A5),-(A7)
  move.l $28(A5),-(A7)
  pea $1E(A7)
   bsr.w JL_0_14F6
  lea $C(A7),A7
JL_0_428:
  movea.l A5,A1
  movea.l $4.W,A6
ReplyMsg SET -$17A
   jsr ReplyMsg(A6)
JL_0_432:
  movea.l $10(A7),A6
  movea.l $14C(A4),A0
  movea.l $4.W,A6
GetMsg SET -$174
   jsr GetMsg(A6)
  movea.l D0,A5
  movea.l $10(A7),A6
  tst.l D0
   bne.b JL_0_3F2
 bra.b JL_0_3E4
 
JL_0_44E:
  movem.l (A7)+,A3-A6
  adda.w #$210,A7
 rtS 
 
JL_0_458:
  suba.w #$10,A7
  movem.l A5-A6,-(A7)
  movea.l $1C(A7),A5
  move.l A5,-(A7)
  move.l A6,$18(A7)
   bsr.w JL_0_1228
  movea.l $8(A5),A1
  movea.l $164(A4),A6
  moveq.l #$00,D0
   jsr -$EA(A6)
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  movea.l $144(A4),A1
  moveq.l #$00,D0
  move.w $14(A0),D0
  move.w $A(A1),D1
  ext.l D1
  move.l D0,$14(A7)
  move.l D1,D0
  move.l $14(A7),D1
  movea.l $18(A7),A6
   bsr.w JL_0_1B54
  move.w D0,$1E(A5)
  moveq.l #$00,D0
  move.w $18(A0),D0
  move.w $8(A1),D1
  ext.l D1
  move.l D0,$C(A7)
  move.l D1,D0
  move.l $C(A7),D1
   bsr.w JL_0_1B54
  move.w D0,$20(A5)
  move.w $8(A1),D1
  move.w D1,$22(A5)
  moveq.l #$01,D1
  move.w D1,$2A(A5)
  move.w D1,$28(A5)
  clr.l $2C(A5)
  move.w $40(A5),$30(A5)
  clr.w $32(A5)
  move.w D1,$4E(A5)
  clr.w $50(A5)
  clr.b $207(A5)
  clr.b $208(A5)
  move.w $4A(A5),$46(A5)
  moveq.l #$00,D1
  move.w D0,D1
  clr.l (A7)
  move.l D1,-(A7)
  pea $11C(A5)
   bsr.w JL_0_1B3C
  lea $C(A7),A7
  moveq.l #$09,D1
 bra.b JL_0_528
 
JL_0_516:
  moveq.l #$00,D0
  move.w D1,D0
  addi.l #$11C,D0
  move.b #$1,$0(A5,D0.L)
  addq.w #8,D1
JL_0_528:
  cmp.w $20(A5),D1
   bcs.b JL_0_516
  moveq.l #$00,D0
  move.w $30(A5),D0
  movea.l $8(A5),A1
  movea.l $164(A4),A6
   jsr -$156(A6)
  moveq.l #$00,D0
  move.w $32(A5),D0
  movea.l $8(A5),A1
   jsr -$15C(A6)
  clr.l -(A7)
  clr.l -(A7)
  move.l A5,-(A7)
  movea.l $20(A7),A6
   bsr.w AJL_0_6AA
  lea $C(A7),A7
  movem.l (A7)+,A5-A6
  adda.w #$10,A7
 rtS 
 
AJL_0_56A:
  subq.w #4,A7
  movem.l D2/A5,-(A7)
  movea.l $14(A7),A0
  movea.l $10(A7),A5
  move.l A6,$8(A7)
  move.b (A0),D0
   bne.b JL_0_5A4
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D1
  move.w $14(A0),D1
  movea.l $144(A4),A0
  move.w $A(A0),D2
  ext.l D2
  move.l D2,D0
   bsr.w JL_0_1B54
  move.w D0,$1E(A5)
 bra.b JL_0_5AC
 
JL_0_5A4:
  moveq.l #$00,D1
  move.b D0,D1
  move.w D1,$1E(A5)
JL_0_5AC:
  movem.l (A7)+,D2/A5
  addq.w #4,A7
 rtS 
 
AJL_0_5B4:
  subq.w #4,A7
  movem.l D2/A5,-(A7)
  movea.l $14(A7),A0
  movea.l $10(A7),A5
  move.l A6,$8(A7)
  move.b $1(A0),D0
   bne.b JL_0_5F0
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D1
  move.w $14(A0),D1
  movea.l $144(A4),A0
  move.w $A(A0),D2
  ext.l D2
  move.l D2,D0
   bsr.w JL_0_1B54
  move.w D0,$1E(A5)
 bra.b JL_0_5F8
 
JL_0_5F0:
  moveq.l #$00,D1
  move.b D0,D1
  move.w D1,$1E(A5)
JL_0_5F8:
  movem.l (A7)+,D2/A5
  addq.w #4,A7
 rtS 
 
AJL_0_600:
  suba.w #$10,A7
  movem.l D2-D5/D7/A5-A6,-(A7)
  movea.l $30(A7),A5
  movea.l $34(A7),A0
  move.b (A0),D7
  move.l A6,$28(A7)
  tst.b D7
   bne.b JL_0_61C
  moveq.l #$01,D7
JL_0_61C:
  move.w $28(A5),D0
  cmp.w $20(A5),D0
   bcc.b JL_0_69A
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D1
  move.w $18(A0),D1
  moveq.l #$00,D0
  move.b D7,D0
   bsr.w JL_0_1C18
  neg.l D0
  moveq.l #$00,D1
  move.w $18(A0),D1
  moveq.l #$00,D2
  move.w $28(A5),D2
  subq.l #1,D2
  move.l D0,$20(A7)
  move.l D2,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $14(A0),D1
  moveq.l #$00,D2
  move.w $2A(A5),D2
  move.l D2,D3
  subq.l #1,D3
  move.l D0,$24(A7)
  move.l D3,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $22(A5),D1
  subq.l #1,D1
  mulu $14(A0),D2
  subq.l #1,D2
  move.l D0,D3
  move.l D1,D4
  move.l D2,D5
  movea.l $8(A5),A1
  move.l $20(A7),D0
  move.l $24(A7),D2
  movea.l $164(A4),A6
  moveq.l #$00,D1
   jsr -$18C(A6)
JL_0_69A:
  movem.l (A7)+,D2-D5/D7/A5-A6
  adda.w #$10,A7
 rtS 
 
AJL_0_6A4:
  subq.w #4,A7
  addq.w #4,A7
 rtS 
 
AJL_0_6AA:
  subq.w #4,A7
  movea.l $8(A7),A0
  lea $2A(A0),A1
  move.w (A1)+,$36(A0)
  move.w $28(A0),$34(A0)
  move.l (A1)+,$38(A0)
  move.w (A1)+,$3C(A0)
  move.w (A1)+,$3E(A0)
  move.b $207(A0),$208(A0)
  move.w $46(A0),$48(A0)
  addq.w #4,A7
 rtS 
 
AJL_0_6DA:
  subq.w #4,A7
  movem.l A5-A6,-(A7)
  movea.l $10(A7),A5
  lea $36(A5),A0
  lea $2A(A5),A1
  move.w (A0)+,(A1)+
  move.w $34(A5),$28(A5)
  move.l (A0)+,(A1)+
  move.w (A0)+,D0
  move.w D0,(A1)+
  move.w (A0)+,(A1)+
  move.b $208(A5),$207(A5)
  move.w $48(A5),$46(A5)
  moveq.l #$00,D0
  move.w $3C(A5),D0
  move.l A6,$8(A7)
  movea.l $8(A5),A1
  movea.l $164(A4),A6
   jsr -$156(A6)
  moveq.l #$00,D0
  move.w $32(A5),D0
  movea.l $8(A5),A1
   jsr -$15C(A6)
  move.l $2C(A5),D0
  moveq.l #$00,D1
  move.w D0,D1
  move.l D1,D0
  movea.l $8(A5),A1
  moveq.l #$01,D1
   jsr -$5A(A6)
  movem.l (A7)+,A5-A6
  addq.w #4,A7
 rtS 
 
AJL_0_748:
  subq.w #4,A7
  movem.l D6-D7,-(A7)
  movea.l $14(A7),A0
  movea.l $10(A7),A1
  moveq.l #$00,D6
  move.l $18(A7),D7
  addq.l #1,D7
  move.l A6,$8(A7)
 bra.b JL_0_7C0
 
JL_0_764:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$00,D1
  addi.l #$1FC,D0
  move.b $0(A1,D0.L),D1
  tst.l D1
   beq.b JL_0_784
  moveq.l #$3E,D0
  sub.l D0,D1
   beq.b JL_0_798
  subq.l #1,D1
   beq.b JL_0_7AC
 bra.b JL_0_7BE
 
JL_0_784:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$14,D1
  cmp.b $0(A0,D0.L),D1
   bne.b JL_0_7BE
  bset #$0,$47(A1)
 bra.b JL_0_7BE
 
JL_0_798:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$01,D1
  cmp.b $0(A0,D0.L),D1
   bne.b JL_0_7BE
  bset #$2,$47(A1)
 bra.b JL_0_7BE
 
JL_0_7AC:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$07,D1
  cmp.b $0(A0,D0.L),D1
   bne.b JL_0_7BE
  bset #$1,$47(A1)
JL_0_7BE:
  addq.w #1,D6
JL_0_7C0:
  moveq.l #$00,D0
  move.w D6,D0
  cmp.l D7,D0
   bcs.b JL_0_764
  movem.l (A7)+,D6-D7
  addq.w #4,A7
 rtS 
 
AJL_0_7D0:
  subq.w #4,A7
  movem.l D6-D7,-(A7)
  movea.l $14(A7),A0
  movea.l $10(A7),A1
  moveq.l #$00,D6
  move.l $18(A7),D7
  addq.l #1,D7
  move.l A6,$8(A7)
 bra.b JL_0_848
 
JL_0_7EC:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$00,D1
  addi.l #$1FC,D0
  move.b $0(A1,D0.L),D1
  tst.l D1
   beq.b JL_0_80C
  moveq.l #$3E,D0
  sub.l D0,D1
   beq.b JL_0_820
  subq.l #1,D1
   beq.b JL_0_834
 bra.b JL_0_846
 
JL_0_80C:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$14,D1
  cmp.b $0(A0,D0.L),D1
   bne.b JL_0_846
  bclr #$0,$47(A1)
 bra.b JL_0_846
 
JL_0_820:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$01,D1
  cmp.b $0(A0,D0.L),D1
   bne.b JL_0_846
  bclr #$2,$47(A1)
 bra.b JL_0_846
 
JL_0_834:
  moveq.l #$00,D0
  move.w D6,D0
  moveq.l #$07,D1
  cmp.b $0(A0,D0.L),D1
   bne.b JL_0_846
  bclr #$1,$47(A1)
JL_0_846:
  addq.w #1,D6
JL_0_848:
  moveq.l #$00,D0
  move.w D6,D0
  cmp.l D7,D0
   bcs.b JL_0_7EC
  movem.l (A7)+,D6-D7
  addq.w #4,A7
 rtS 
 
AJL_0_858:
  suba.w #$C,A7
  movem.l D2/D4-D7/A3/A5-A6,-(A7)
  movea.l $34(A7),A3
  movea.l $30(A7),A5
  moveq.l #$00,D6
  clr.w $2A(A7)
  moveq.l #$00,D5
  moveq.l #$00,D4
  move.l $38(A7),D7
  addq.l #1,D7
  move.l A6,$24(A7)
 bra.w JL_0_A2C
 
JL_0_880:
  moveq.l #$00,D0
  move.w D4,D0
  move.b $0(A3,D0.L),D1
  moveq.l #$00,D0
  move.b D1,D0
  move.b D1,$20(A7)
  cmpi.l #$32,D0
   bcc.w JL_0_A2A
  add.w D0,D0
  move.w L_0_8A4(PC,D0.W),D0
 jmp L_0_8A6(PC,D0.W)
 
L_0_8A4:
 dc.w JL_0_908-(L_0_8A6)
L_0_8A6:
 dc.w JL_0_924-(L_0_8A6)
L_0_8A8:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8AA:
 dc.w JL_0_938-(L_0_8A6)
L_0_8AC:
 dc.w JL_0_944-(L_0_8A6)
L_0_8AE:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8B0:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8B2:
 dc.w JL_0_950-(L_0_8A6)
L_0_8B4:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8B6:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8B8:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8BA:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8BC:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8BE:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8C0:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8C2:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8C4:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8C6:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8C8:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8CA:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8CC:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8CE:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8D0:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8D2:
 dc.w JL_0_95A-(L_0_8A6)
L_0_8D4:
 dc.w JL_0_966-(L_0_8A6)
L_0_8D6:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8D8:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8DA:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8DC:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8DE:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8E0:
 dc.w JL_0_972-(L_0_8A6)
L_0_8E2:
 dc.w JL_0_972-(L_0_8A6)
L_0_8E4:
 dc.w JL_0_972-(L_0_8A6)
L_0_8E6:
 dc.w JL_0_972-(L_0_8A6)
L_0_8E8:
 dc.w JL_0_972-(L_0_8A6)
L_0_8EA:
 dc.w JL_0_972-(L_0_8A6)
L_0_8EC:
 dc.w JL_0_972-(L_0_8A6)
L_0_8EE:
 dc.w JL_0_972-(L_0_8A6)
L_0_8F0:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_8F2:
 dc.w JL_0_972-(L_0_8A6)
L_0_8F4:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_8F6:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_8F8:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_8FA:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_8FC:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_8FE:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_900:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_902:
 dc.w JL_0_9C0-(L_0_8A6)
L_0_904:
 dc.w JL_0_A2A-(L_0_8A6)
L_0_906:
 dc.w JL_0_9C0-(L_0_8A6)
JL_0_908:
  clr.l $2C(A5)
  move.w $40(A5),$30(A5)
  clr.w $32(A5)
  moveq.l #$01,D6
  moveq.l #$01,D0
  move.w D0,D5
  move.w D0,$2A(A7)
 bra.w JL_0_A2A
 
JL_0_924:
  tst.w $42(A5)
   beq.w JL_0_A2A
  bset #$4,$2C(A5)
  moveq.l #$01,D6
 bra.w JL_0_A2A
 
JL_0_938:
  bset #$2,$2F(A5)
  moveq.l #$01,D5
 bra.w JL_0_A2A
 
JL_0_944:
  bset #$0,$2F(A5)
  moveq.l #$01,D5
 bra.w JL_0_A2A
 
JL_0_950:
  bset #$5,$2C(A5)
 bra.w JL_0_A2A
 
JL_0_95A:
  bclr #$2,$2F(A5)
  moveq.l #$01,D5
 bra.w JL_0_A2A
 
JL_0_966:
  bclr #$0,$2F(A5)
  moveq.l #$01,D5
 bra.w JL_0_A2A
 
JL_0_972:
  tst.w $44(A5)
   bne.w JL_0_A2A
  moveq.l #$00,D0
  move.b $20(A7),D0
  move.l D0,D1
  moveq.l #$1E,D0
  sub.l D0,D1
  move.w D1,$30(A5)
  moveq.l #$09,D0
  cmp.w D0,D1
   bne.b JL_0_998
  move.w $40(A5),D0
  move.w D0,$30(A5)
JL_0_998:
  moveq.l #$01,D6
  move.l $150(A4),D0
   ble.w JL_0_A2A
  moveq.l #$01,D0
  cmp.w $30(A5),D0
   bne.b JL_0_9B2
  moveq.l #$07,D1
  move.w D1,$30(A5)
 bra.b JL_0_A2A
 
JL_0_9B2:
  moveq.l #$07,D1
  cmp.w $30(A5),D1
   bne.b JL_0_A2A
  move.w D0,$30(A5)
 bra.b JL_0_A2A
 
JL_0_9C0:
  tst.w $44(A5)
   bne.b JL_0_A2A
  moveq.l #$00,D0
  move.b $20(A7),D0
  moveq.l #$28,D1
  sub.l D1,D0
  move.w D0,$32(A5)
  moveq.l #$09,D1
  cmp.w D1,D0
   beq.b JL_0_9FC
  movea.l $144(A4),A1
  movea.l $2E(A1),A0
  movea.l $50(A0),A1
  movea.l $4(A1),A0
  moveq.l #$00,D1
  move.b $5(A0),D1
  moveq.l #$00,D2
  bset D1,D2
  moveq.l #$00,D1
  move.w D0,D1
  cmp.l D2,D1
   blt.b JL_0_A02
JL_0_9FC:
  moveq.l #$00,D1
  move.w D1,$32(A5)
JL_0_A02:
  move.w #$0001,$2A(A7)
  move.l $150(A4),D0
   ble.b JL_0_A2A
  moveq.l #$01,D0
  cmp.w $32(A5),D0
   bne.b JL_0_A1E
  moveq.l #$07,D1
  move.w D1,$32(A5)
 bra.b JL_0_A2A
 
JL_0_A1E:
  moveq.l #$07,D1
  cmp.w $32(A5),D1
   bne.b JL_0_A2A
  move.w D0,$32(A5)
JL_0_A2A:
  addq.w #1,D4
JL_0_A2C:
  moveq.l #$00,D0
  move.w D4,D0
  cmp.l D7,D0
   bcs.w JL_0_880
  move.w $30(A5),D0
  cmp.w $32(A5),D0
   bne.b JL_0_A58
  tst.w $42(A5)
   bne.b JL_0_A58
  move.w $40(A5),$30(A5)
  clr.w $32(A5)
  moveq.l #$01,D6
  move.w #$0001,$2A(A7)
JL_0_A58:
  tst.w D6
   beq.b JL_0_A92
  btst #$4,$2C(A5)
   beq.b JL_0_A80
  tst.w $44(A5)
   beq.b JL_0_A6E
  moveq.l #$03,D0
 bra.b JL_0_A7C
 
JL_0_A6E:
  move.w $30(A5),D0
  moveq.l #$07,D1
  cmp.w D1,D0
   bhi.b JL_0_A80
  bset #$3,D0
JL_0_A7C:
  move.w D0,$30(A5)
JL_0_A80:
  moveq.l #$00,D0
  move.w $30(A5),D0
  movea.l $8(A5),A1
  movea.l $164(A4),A6
   jsr -$156(A6)
JL_0_A92:
  movea.l $24(A7),A6
  tst.w $2A(A7)
   beq.b JL_0_AAE
  moveq.l #$00,D0
  move.w $32(A5),D0
  movea.l $8(A5),A1
  movea.l $164(A4),A6
   jsr -$15C(A6)
JL_0_AAE:
  movea.l $24(A7),A6
  tst.w D5
   beq.b JL_0_ACE
  move.l $2C(A5),D0
  moveq.l #$00,D1
  move.w D0,D1
  move.l D1,D0
  movea.l $8(A5),A1
  movea.l $164(A4),A6
  moveq.l #$01,D1
   jsr -$5A(A6)
JL_0_ACE:
  movem.l (A7)+,D2/D4-D7/A3/A5-A6
  adda.w #$C,A7
 rtS 
 
AJL_0_AD8:
  subq.w #4,A7
  addq.w #4,A7
 rtS 
 
AJL_0_ADE:
  suba.w #$10,A7
  movem.l D2-D5/D7/A6,-(A7)
  movea.l $30(A7),A0
  move.b (A0),D7
  move.l A6,$24(A7)
  tst.b D7
   bne.b JL_0_AF6
  moveq.l #$01,D7
JL_0_AF6:
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D0
  move.w $14(A0),D0
  moveq.l #$00,D1
  move.b D7,D1
   bsr.w JL_0_1C18
  neg.l D0
  moveq.l #$00,D1
  movea.l $2C(A7),A1
  move.w $22(A1),D1
  subq.l #1,D1
  move.w $14(A0),D2
  move.w $1E(A1),D3
  mulu D2,D3
  subq.l #1,D3
  move.l D1,$20(A7)
  move.l D0,D1
  move.l D3,D5
  movea.l $8(A1),A1
  move.l $20(A7),D4
  movea.l $164(A4),A6
  moveq.l #$00,D0
  move.l D0,D2
  move.l D0,D3
   jsr -$18C(A6)
  movem.l (A7)+,D2-D5/D7/A6
  adda.w #$10,A7
 rtS 
 
AJL_0_B4E:
  suba.w #$10,A7
  movem.l D2-D5/D7/A6,-(A7)
  movea.l $30(A7),A0
  move.b (A0),D7
  move.l A6,$24(A7)
  tst.b D7
   bne.b JL_0_B66
  moveq.l #$01,D7
JL_0_B66:
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D0
  move.w $14(A0),D0
  moveq.l #$00,D1
  move.b D7,D1
   bsr.w JL_0_1C18
  neg.l D0
  moveq.l #$00,D1
  movea.l $2C(A7),A1
  move.w $22(A1),D1
  subq.l #1,D1
  move.w $14(A0),D2
  move.w $1E(A1),D3
  mulu D2,D3
  subq.l #1,D3
  move.l D1,$20(A7)
  move.l D0,D1
  move.l D3,D5
  movea.l $8(A1),A1
  move.l $20(A7),D4
  movea.l $164(A4),A6
  moveq.l #$00,D0
  move.l D0,D2
  move.l D0,D3
   jsr -$18C(A6)
  movem.l (A7)+,D2-D5/D7/A6
  adda.w #$10,A7
 rtS 
 
AJL_0_BBE:
  suba.w #$10,A7
  movem.l D2-D5/D7/A5-A6,-(A7)
  movea.l $30(A7),A5
  movea.l $34(A7),A0
  move.b (A0),D7
  move.l A6,$28(A7)
  tst.b D7
   bne.b JL_0_BDA
  moveq.l #$01,D7
JL_0_BDA:
  move.w $28(A5),D0
  cmp.w $20(A5),D0
   bcc.b JL_0_C56
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D1
  move.w $18(A0),D1
  moveq.l #$00,D0
  move.b D7,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $18(A0),D1
  moveq.l #$00,D2
  move.w $28(A5),D2
  subq.l #1,D2
  move.l D0,$20(A7)
  move.l D2,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $14(A0),D1
  moveq.l #$00,D2
  move.w $2A(A5),D2
  move.l D2,D3
  subq.l #1,D3
  move.l D0,$24(A7)
  move.l D3,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $22(A5),D1
  subq.l #1,D1
  mulu $18(A0),D2
  subq.l #1,D2
  move.l D0,D3
  move.l D1,D4
  move.l D2,D5
  movea.l $8(A5),A1
  move.l $20(A7),D0
  move.l $24(A7),D2
  movea.l $164(A4),A6
  moveq.l #$00,D1
   jsr -$18C(A6)
JL_0_C56:
  movem.l (A7)+,D2-D5/D7/A5-A6
  adda.w #$10,A7
 rtS 
 
AJL_0_C60:
  suba.w #$C,A7
  movem.l D2-D5/D7/A5-A6,-(A7)
  movea.l $2C(A7),A5
  movea.l $30(A7),A0
  move.b (A0),D7
  move.l A6,$24(A7)
  tst.b D7
   bne.b JL_0_C7C
  moveq.l #$01,D7
JL_0_C7C:
  move.w $2A(A5),D0
  moveq.l #$01,D1
  cmp.w D1,D0
   bcs.b JL_0_D02
  move.w $1E(A5),D1
  cmp.w D1,D0
   bhi.b JL_0_D02
  moveq.l #$00,D2
  move.w D0,D2
  moveq.l #$00,D0
  move.w D1,D0
  sub.l D2,D0
  addq.l #1,D0
  moveq.l #$00,D1
  move.b D7,D1
  cmp.l D0,D1
   ble.b JL_0_CA4
  move.l D0,D7
JL_0_CA4:
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D2
  move.w $14(A0),D2
  moveq.l #$00,D0
  move.b D7,D0
  move.l D2,D1
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w D2,D1
  moveq.l #$00,D3
  move.w $2A(A5),D3
  subq.l #1,D3
  move.l D0,$20(A7)
  move.l D3,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $22(A5),D1
  subq.l #1,D1
  move.w $1E(A5),D3
  mulu D2,D3
  subq.l #1,D3
  move.l D3,$1C(A7)
  move.l D0,D3
  move.l D1,D4
  movea.l $8(A5),A1
  move.l $20(A7),D1
  move.l $1C(A7),D5
  movea.l $164(A4),A6
  moveq.l #$00,D0
  move.l D0,D2
   jsr -$18C(A6)
JL_0_D02:
  movem.l (A7)+,D2-D5/D7/A5-A6
  adda.w #$C,A7
 rtS 
 
AJL_0_D0C:
  suba.w #$C,A7
  movem.l D2-D5/D7/A5-A6,-(A7)
  movea.l $2C(A7),A5
  movea.l $30(A7),A0
  move.b (A0),D7
  move.l A6,$24(A7)
  tst.b D7
   bne.b JL_0_D28
  moveq.l #$01,D7
JL_0_D28:
  move.w $2A(A5),D0
  moveq.l #$01,D1
  cmp.w D1,D0
   bcs.b JL_0_DA8
  move.w $1E(A5),D1
  cmp.w D1,D0
   bhi.b JL_0_DA8
  moveq.l #$00,D2
  move.w D0,D2
  moveq.l #$00,D0
  move.w D1,D0
  sub.l D2,D0
  moveq.l #$00,D1
  move.b D7,D1
  cmp.l D0,D1
   ble.b JL_0_D4E
  move.l D0,D7
JL_0_D4E:
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D2
  move.w $14(A0),D2
  moveq.l #$00,D0
  move.b D7,D0
  move.l D2,D1
   bsr.w JL_0_1C18
  neg.l D0
  moveq.l #$00,D1
  move.w D2,D1
  moveq.l #$00,D2
  move.w $2A(A5),D2
  subq.l #1,D2
  move.l D0,$20(A7)
  move.l D2,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $22(A5),D1
  subq.l #1,D1
  moveq.l #$00,D2
  move.w $1E(A5),D2
  subq.l #1,D2
  move.l D0,D3
  move.l D1,D4
  move.l D2,D5
  movea.l $8(A5),A1
  move.l $20(A7),D1
  movea.l $164(A4),A6
  moveq.l #$00,D0
  move.l D0,D2
   jsr -$18C(A6)
JL_0_DA8:
  movem.l (A7)+,D2-D5/D7/A5-A6
  adda.w #$C,A7
 rtS 
 
AJL_0_DB2:
  subq.w #4,A7
  move.l A5,-(A7)
  movea.l $C(A7),A5
  moveq.l #$00,D0
  movea.l $10(A7),A0
  move.b (A0),D0
  move.l A6,$4(A7)
  tst.l D0
   beq.b JL_0_DD4
  subq.l #1,D0
   beq.b JL_0_DEC
  subq.l #1,D0
   beq.b JL_0_DF6
 bra.b JL_0_E14
 
JL_0_DD4:
  move.w $20(A5),D0
  sub.w $28(A5),D0
  addq.w #1,D0
  moveq.l #$00,D1
  move.w D0,D1
  move.l D1,-(A7)
  move.l A5,-(A7)
   bsr.w JL_0_E1A
 bra.b JL_0_E12
 
JL_0_DEC:
  move.l A5,-(A7)
   bsr.w JL_0_E90
  addq.w #4,A7
 bra.b JL_0_E14
 
JL_0_DF6:
  move.l A5,-(A7)
   bsr.w JL_0_E90
  move.w $20(A5),D0
  sub.w $28(A5),D0
  addq.w #1,D0
  moveq.l #$00,D1
  move.w D0,D1
  move.l D1,(A7)
  move.l A5,-(A7)
   bsr.w JL_0_E1A
JL_0_E12:
  addq.w #8,A7
JL_0_E14:
  movea.l (A7)+,A5
  addq.w #4,A7
 rtS 
 
JL_0_E1A:
  subq.w #4,A7
  movem.l D2-D5/D7/A5,-(A7)
  movea.l $20(A7),A5
  move.w $28(A5),D0
  move.w $26(A7),D1
  add.w D0,D1
  move.w D1,D7
  subq.w #1,D7
  move.l A6,$18(A7)
  move.w $20(A5),D1
  cmp.w D1,D7
   bls.b JL_0_E40
  move.w D1,D7
JL_0_E40:
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  subq.w #1,D0
  move.w $18(A0),D1
  muls D1,D0
  moveq.l #$00,D2
  move.w D0,D2
  move.w $2A(A5),D0
  move.l D0,D3
  subq.w #1,D3
  move.w $14(A0),D4
  muls D4,D3
  moveq.l #$00,D5
  move.w D3,D5
  move.w D7,D3
  mulu D1,D3
  moveq.l #$00,D1
  move.w D3,D1
  mulu D4,D0
  moveq.l #$00,D3
  move.w D0,D3
  move.l D3,-(A7)
  move.l D1,-(A7)
  move.l D5,-(A7)
  move.l D2,-(A7)
  move.l $8(A5),-(A7)
   bsr.w JL_0_EEC
  lea $14(A7),A7
  movem.l (A7)+,D2-D5/D7/A5
  addq.w #4,A7
 rtS 
 
JL_0_E90:
  subq.w #4,A7
  movem.l D2-D4/A5,-(A7)
  movea.l $18(A7),A5
  move.l A6,$10(A7)
  move.w $28(A5),D0
  moveq.l #$01,D1
  cmp.w D1,D0
   bls.b JL_0_EE4
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  move.w $2A(A5),D1
  move.l D1,D2
  subq.w #1,D2
  move.w $14(A0),D3
  muls D3,D2
  moveq.l #$00,D4
  move.w D2,D4
  mulu $18(A0),D0
  moveq.l #$00,D2
  move.w D0,D2
  mulu D3,D1
  moveq.l #$00,D0
  move.w D1,D0
  move.l D0,-(A7)
  move.l D2,-(A7)
  move.l D4,-(A7)
  clr.l -(A7)
  move.l $8(A5),-(A7)
   bsr.w JL_0_EEC
  lea $14(A7),A7
JL_0_EE4:
  movem.l (A7)+,D2-D4/A5
  addq.w #4,A7
 rtS 
 
JL_0_EEC:
  subq.w #4,A7
  movem.l D2-D6/A6,-(A7)
  moveq.l #$00,D0
  move.w $26(A7),D0
  moveq.l #$00,D1
  move.w $2A(A7),D1
  moveq.l #$00,D2
  move.w D0,D2
  moveq.l #$00,D3
  move.w D1,D3
  moveq.l #$00,D4
  move.w D0,D4
  moveq.l #$00,D5
  move.w $2E(A7),D5
  sub.l D4,D5
  moveq.l #$00,D4
  move.w D1,D4
  moveq.l #$00,D6
  move.w $32(A7),D6
  sub.l D4,D6
  move.l A6,$18(A7)
  move.l D5,D4
  move.l D6,D5
  movea.l $20(A7),A0
  movea.l A0,A1
  movea.l $164(A4),A6
  moveq.l #$00,D6
   jsr -$228(A6)
  movem.l (A7)+,D2-D6/A6
  addq.w #4,A7
 rtS 
 
JL_0_F3E:
  subq.w #4,A7
  movem.l D2/A5,-(A7)
  movea.l $10(A7),A5
  move.l A6,$8(A7)
  move.w $2A(A5),D0
  moveq.l #$01,D1
  cmp.w D1,D0
   bls.b JL_0_F84
  moveq.l #$00,D1
  move.w $22(A5),D1
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  subq.w #1,D0
  mulu $14(A0),D0
  moveq.l #$00,D2
  move.w D0,D2
  move.l D2,-(A7)
  move.l D1,-(A7)
  moveq.l #$00,D0
  move.l D0,-(A7)
  move.l D0,-(A7)
  move.l $8(A5),-(A7)
   bsr.w JL_0_EEC
  lea $14(A7),A7
JL_0_F84:
  move.l A5,$10(A7)
  movem.l (A7)+,D2/A5
  addq.l #4,A7
 bra.w JL_0_E90
 
JL_0_F92:
  subq.w #4,A7
  movem.l D2-D3/D7/A5,-(A7)
  movea.l $18(A7),A5
  move.w $2A(A5),D0
  move.w $1E(A7),D1
  add.w D0,D1
  move.l D1,D7
  move.l A6,$10(A7)
  move.w $1E(A5),D1
  cmp.w D1,D7
   bls.b JL_0_FB6
  move.w D1,D7
JL_0_FB6:
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  subq.w #1,D0
  move.w $14(A0),D1
  muls D1,D0
  moveq.l #$00,D2
  move.w D0,D2
  moveq.l #$00,D0
  move.w $22(A5),D0
  move.w D7,D3
  mulu D1,D3
  moveq.l #$00,D1
  move.w D3,D1
  move.l D1,-(A7)
  move.l D0,-(A7)
  move.l D2,-(A7)
  clr.l -(A7)
  move.l $8(A5),-(A7)
   bsr.w JL_0_EEC
  lea $14(A7),A7
  movem.l (A7)+,D2-D3/D7/A5
  addq.w #4,A7
 rtS 
 
AJL_0_FF4:
  subq.w #4,A7
  move.l A5,-(A7)
  movea.l $C(A7),A5
  moveq.l #$00,D0
  movea.l $10(A7),A0
  move.b (A0),D0
  move.l A6,$4(A7)
  tst.l D0
   beq.b JL_0_1016
  subq.l #1,D0
   beq.b JL_0_105A
  subq.l #1,D0
   beq.b JL_0_1068
 bra.b JL_0_1070
 
JL_0_1016:
  move.w $1E(A5),D0
  move.w $2A(A5),D1
  cmp.w D0,D1
   bcc.b JL_0_1040
  addq.w #1,$2A(A5)
  move.w $1E(A5),D0
  sub.w $2A(A5),D0
  moveq.l #$00,D1
  move.w D0,D1
  move.l D1,-(A7)
  move.l A5,-(A7)
   bsr.w JL_0_F92
  addq.w #8,A7
  subq.w #1,$2A(A5)
JL_0_1040:
  move.w $20(A5),D0
  sub.w $28(A5),D0
  addq.w #1,D0
  moveq.l #$00,D1
  move.w D0,D1
  move.l D1,-(A7)
  move.l A5,-(A7)
   bsr.w JL_0_E1A
  addq.w #8,A7
 bra.b JL_0_1070
 
JL_0_105A:
  move.l A5,-(A7)
   bsr.w JL_0_F3E
  move.l A5,(A7)
   bsr.w JL_0_E90
 bra.b JL_0_106E
 
JL_0_1068:
  move.l A5,-(A7)
   bsr.w JL_0_14C8
JL_0_106E:
  addq.w #4,A7
JL_0_1070:
  movea.l (A7)+,A5
  addq.w #4,A7
 rtS 
 
AJL_0_1076:
  subq.w #4,A7
  movea.l $C(A7),A0
  movea.l $8(A7),A1
  moveq.l #$00,D0
  move.b (A0),D0
  move.w D0,$2A(A1)
  move.l A6,(A7)
  tst.w D0
   bne.b JL_0_1092
  moveq.l #$01,D1
 bra.b JL_0_109A
 
JL_0_1092:
  move.w $1E(A1),D1
  cmp.w D1,D0
   bls.b JL_0_109E
JL_0_109A:
  move.w D1,$2A(A1)
JL_0_109E:
  moveq.l #$00,D0
  move.b $1(A0),D0
  move.w D0,$28(A1)
   bne.b JL_0_10AE
  moveq.l #$01,D1
 bra.b JL_0_10B6
 
JL_0_10AE:
  move.w $20(A1),D1
  cmp.w D1,D0
   bls.b JL_0_10BA
JL_0_10B6:
  move.w D1,$28(A1)
JL_0_10BA:
  addq.w #4,A7
 rtS 
 
AJL_0_10BE:
  subq.w #4,A7
  movea.l $8(A7),A0
  move.w #$0001,$28(A0)
  move.l A6,(A7)
  addq.l #4,A7
 bra.w AJL_0_1406
 
AJL_0_10D2:
  subq.w #4,A7
  movem.l D7/A5,-(A7)
  movea.l $10(A7),A5
  movea.l $14(A7),A0
  move.b (A0),D7
  move.l A6,$8(A7)
  tst.b D7
   bne.b JL_0_1118
  moveq.l #$01,D7
 bra.b JL_0_1118
 
JL_0_10EE:
  move.w $28(A5),D0
  cmp.w $20(A5),D0
   bcs.b JL_0_1114
  btst #$1,$47(A5)
   beq.b JL_0_1120
  pea $1.W
  pea $E0(A4)
  move.l A5,-(A7)
   bsr.w AJL_0_1312
  lea $C(A7),A7
 bra.b JL_0_1118
 
JL_0_1114:
  addq.w #1,$28(A5)
JL_0_1118:
  move.l D7,D0
  subq.b #1,D7
  tst.b D0
   bne.b JL_0_10EE
JL_0_1120:
  movem.l (A7)+,D7/A5
  addq.w #4,A7
 rtS 
 
AJL_0_1128:
  subq.w #4,A7
  movem.l D7/A5,-(A7)
  movea.l $10(A7),A5
  movea.l $14(A7),A0
  move.b (A0),D7
  move.l A6,$8(A7)
  tst.b D7
   bne.b JL_0_1172
  moveq.l #$01,D7
 bra.b JL_0_1172
 
JL_0_1144:
  cmpi.w #$1,$28(A5)
   bhi.b JL_0_116E
  btst #$1,$47(A5)
   beq.b JL_0_117A
  move.w $20(A5),$28(A5)
  pea $1.W
  pea $E2(A4)
  move.l A5,-(A7)
   bsr.w AJL_0_1406
  lea $C(A7),A7
 bra.b JL_0_1172
 
JL_0_116E:
  subq.w #1,$28(A5)
JL_0_1172:
  move.l D7,D0
  subq.b #1,D7
  tst.b D0
   bne.b JL_0_1144
JL_0_117A:
  movem.l (A7)+,D7/A5
  addq.w #4,A7
 rtS 
 
JL_0_1182:
  suba.w #$14,A7
  movem.l D2-D3/A5-A6,-(A7)
  movea.l $28(A7),A5
  move.l A6,$20(A7)
  move.w $20(A5),D0
  move.w $28(A5),D1
  cmp.w D0,D1
   bls.b JL_0_11A2
  move.w D0,$28(A5)
JL_0_11A2:
  movea.l $8(A5),A1
  movea.l $164(A4),A6
  moveq.l #$02,D0
   jsr -$162(A6)
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D0
  move.w $18(A0),D0
  moveq.l #$00,D1
  move.w $28(A5),D1
  subq.l #1,D1
  movea.l $20(A7),A6
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $14(A0),D1
  moveq.l #$00,D2
  move.w $2A(A5),D2
  move.l D2,D3
  subq.l #1,D3
  move.l D0,$14(A7)
  move.l D3,D0
   bsr.w JL_0_1C18
  move.w $28(A5),D1
  mulu $18(A0),D1
  subq.l #1,D1
  mulu $14(A0),D2
  subq.l #1,D2
  move.l D1,$1C(A7)
  move.l D0,D1
  move.l D2,D3
  movea.l $8(A5),A1
  move.l $14(A7),D0
  move.l $1C(A7),D2
  movea.l $164(A4),A6
   jsr -$132(A6)
  movea.l $8(A5),A1
  moveq.l #$01,D0
   jsr -$162(A6)
  movem.l (A7)+,D2-D3/A5-A6
  adda.w #$14,A7
 rtS 
 
JL_0_1228:
  subq.w #4,A7
  movea.l $8(A7),A0
  clr.w $14(A0)
  clr.w $16(A0)
  clr.l $1A(A0)
  clr.b $1E4(A0)
  clr.b $1E5(A0)
  clr.w $52(A0)
  clr.b $1FC(A0)
  addq.w #4,A7
 rtS 
 
JL_0_124E:
  suba.w #$10,A7
  movem.l D2-D3/A5-A6,-(A7)
  movea.l $24(A7),A5
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D0
  move.w $18(A0),D0
  moveq.l #$00,D1
  move.w $4E(A5),D1
  subq.l #1,D1
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $14(A0),D1
  moveq.l #$00,D2
  move.w D1,D2
  moveq.l #$00,D3
  move.w $2A(A5),D3
  subq.l #1,D3
  move.l D0,$14(A7)
  move.l D3,D0
  move.l D1,$18(A7)
  move.l D2,D1
   bsr.w JL_0_1C18
  add.l $18(A7),D0
  subq.l #2,D0
  move.l A6,$1C(A7)
  move.l D0,D1
  movea.l $8(A5),A1
  move.l $14(A7),D0
  movea.l $164(A4),A6
   jsr -$F0(A6)
  btst #$5,$2C(A5)
  movea.l $1C(A7),A6
   beq.b JL_0_12EE
  movea.l $8(A5),A1
  movea.l $164(A4),A6
  moveq.l #$05,D0
   jsr -$162(A6)
  lea $54(A5),A0
  moveq.l #$00,D0
  move.w $50(A5),D0
  movea.l $8(A5),A1
   jsr -$3C(A6)
  movea.l $8(A5),A1
  moveq.l #$01,D0
   jsr -$162(A6)
  movea.l $1C(A7),A6
 bra.b JL_0_1304
 
JL_0_12EE:
  lea $54(A5),A0
  moveq.l #$00,D0
  move.w $50(A5),D0
  movea.l $8(A5),A1
  movea.l $164(A4),A6
   jsr -$3C(A6)
JL_0_1304:
  clr.w $50(A5)
  movem.l (A7)+,D2-D3/A5-A6
  adda.w #$10,A7
 rtS 
 
AJL_0_1312:
  subq.w #4,A7
  movea.l $8(A7),A0
  move.w #$0001,$28(A0)
  move.l A6,(A7)
  addq.l #4,A7
 bra.w AJL_0_1326
 
AJL_0_1326:
  suba.w #$10,A7
  movem.l D2-D7/A5-A6,-(A7)
  movea.l $34(A7),A5
  movea.l $38(A7),A0
  move.b (A0),D6
  move.l A6,$2C(A7)
  tst.b D6
   bne.b JL_0_1342
  moveq.l #$01,D6
JL_0_1342:
  moveq.l #$00,D0
  move.b D6,D0
  moveq.l #$00,D1
  move.w $2A(A5),D1
  add.l D0,D1
  move.l D1,D7
  btst #$2,$47(A5)
   beq.w JL_0_13E8
  moveq.l #$00,D0
  move.w $1E(A5),D0
  cmp.l D0,D7
   bgt.b JL_0_136A
  move.w D7,$2A(A5)
 bra.b JL_0_13E2
 
JL_0_136A:
  cmpi.l #$1,$150(A4)
   ble.b JL_0_1392
  subq.b #1,D6
   bne.b JL_0_1392
  moveq.l #$00,D1
  move.w D0,D1
  moveq.l #$00,D2
  move.w $2A(A5),D2
  sub.l D1,D2
  move.l D2,D7
  addq.l #2,D7
  move.w D0,D1
  subq.w #1,D1
  move.w D1,$2A(A5)
 bra.b JL_0_139C
 
JL_0_1392:
  moveq.l #$00,D1
  move.w D0,D1
  sub.l D1,D7
  move.w D0,$2A(A5)
JL_0_139C:
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D0
  move.w $14(A0),D0
  move.l D7,D1
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $22(A5),D1
  subq.l #1,D1
  move.w $14(A0),D2
  move.w $1E(A5),D3
  mulu D2,D3
  subq.l #1,D3
  move.l D1,$28(A7)
  move.l D0,D1
  move.l D3,D5
  movea.l $8(A5),A1
  move.l $28(A7),D4
  movea.l $164(A4),A6
  moveq.l #$00,D0
  move.l D0,D2
  move.l D0,D3
   jsr -$18C(A6)
JL_0_13E2:
  movea.l $2C(A7),A6
 bra.b JL_0_13FC
 
JL_0_13E8:
  moveq.l #$00,D0
  move.w $1E(A5),D0
  cmp.l D0,D7
   bge.b JL_0_13F8
  move.w D7,$2A(A5)
 bra.b JL_0_13FC
 
JL_0_13F8:
  move.w D0,$2A(A5)
JL_0_13FC:
  movem.l (A7)+,D2-D7/A5-A6
  adda.w #$10,A7
 rtS 
 
AJL_0_1406:
  suba.w #$10,A7
  movem.l D2-D6/A5-A6,-(A7)
  movea.l $30(A7),A5
  movea.l $34(A7),A0
  move.b (A0),D6
  move.l A6,$28(A7)
  tst.b D6
   bne.b JL_0_1422
  moveq.l #$01,D6
JL_0_1422:
  moveq.l #$00,D0
  move.b D6,D0
  moveq.l #$00,D1
  move.w $2A(A5),D1
  sub.l D0,D1
  btst #$2,$47(A5)
   beq.b JL_0_14AC
  moveq.l #$01,D0
  cmp.l D0,D1
   blt.b JL_0_1442
  move.w D1,$2A(A5)
 bra.b JL_0_14A6
 
JL_0_1442:
  move.w $2A(A5),D1
  moveq.l #$01,D0
  cmp.w D0,D1
   bls.b JL_0_1458
  moveq.l #$00,D3
  move.w D1,D3
  moveq.l #$00,D1
  move.b D6,D1
  sub.l D3,D1
 bra.b JL_0_145C
 
JL_0_1458:
  moveq.l #$00,D1
  move.b D6,D1
JL_0_145C:
  move.w D0,$2A(A5)
  movea.l $148(A4),A1
  movea.l $34(A1),A0
  moveq.l #$00,D2
  move.w $14(A0),D2
  move.l D2,D3
  neg.l D3
  move.l D3,D0
   bsr.w JL_0_1C18
  moveq.l #$00,D1
  move.w $22(A5),D1
  subq.l #1,D1
  move.w $1E(A5),D3
  mulu D2,D3
  subq.l #1,D3
  move.l D1,$24(A7)
  move.l D0,D1
  move.l D3,D5
  movea.l $8(A5),A1
  move.l $24(A7),D4
  movea.l $164(A4),A6
  moveq.l #$00,D0
  move.l D0,D2
  move.l D0,D3
   jsr -$18C(A6)
JL_0_14A6:
  movea.l $28(A7),A6
 bra.b JL_0_14BE
 
JL_0_14AC:
  moveq.l #$01,D0
  cmp.l D0,D1
   ble.b JL_0_14B8
  move.w D1,$2A(A5)
 bra.b JL_0_14BE
 
JL_0_14B8:
  move.w #$0001,$2A(A5)
JL_0_14BE:
  movem.l (A7)+,D2-D6/A5-A6
  adda.w #$10,A7
 rtS 
 
JL_0_14C8:
  subq.w #4,A7
  movem.l A5-A6,-(A7)
  movea.l $10(A7),A5
  move.l A6,$8(A7)
  movea.l $8(A5),A1
  movea.l $164(A4),A6
  moveq.l #$00,D0
   jsr -$EA(A6)
  moveq.l #$01,D0
  move.w D0,$2A(A5)
  move.w D0,$28(A5)
  movem.l (A7)+,A5-A6
  addq.w #4,A7
 rtS 
 
JL_0_14F6:
  subq.w #4,A7
  movem.l D4-D7/A2-A3/A5-A6,-(A7)
  move.l $30(A7),D7
  movea.l $28(A7),A5
  movea.l $2C(A7),A2
  move.l A5,-(A7)
  move.l A6,$24(A7)
   bsr.w JL_0_1182
  addq.w #4,A7
  move.l D7,D0
  addq.l #1,D0
   bne.w JL_0_1834
  movea.l A2,A0
JL_0_151E:
  tst.b (A0)+
   bne.b JL_0_151E
  subq.l #1,A0
  suba.l A2,A0
  move.l A0,D7
 bra.w JL_0_1834
 
JL_0_152C:
  move.b (A2)+,D4
  moveq.l #$18,D0
  cmp.b D0,D4
   beq.b JL_0_153A
  moveq.l #$1A,D0
  cmp.b D0,D4
   bne.b JL_0_1546
JL_0_153A:
  move.l A5,-(A7)
   bsr.w JL_0_1228
  addq.w #4,A7
 bra.w JL_0_1834
 
JL_0_1546:
  tst.w $14(A5)
   beq.b JL_0_156A
  clr.w $14(A5)
  moveq.l #$5B,D0
  cmp.b D0,D4
   bne.w JL_0_1834
  move.l A5,-(A7)
   bsr.w JL_0_1228
  addq.w #4,A7
  moveq.l #$01,D0
  move.w D0,$16(A5)
 bra.w JL_0_1834
 
JL_0_156A:
  tst.w $16(A5)
   beq.w JL_0_16A0
  moveq.l #$40,D0
  cmp.b D0,D4
   bcc.w JL_0_163A
  moveq.l #$30,D0
  cmp.b D0,D4
   bcs.b JL_0_15D2
  moveq.l #$39,D0
  cmp.b D0,D4
   bhi.b JL_0_15D2
  movea.l $1A(A5),A0
  moveq.l #$00,D0
  move.l A0,D1
  addi.l #$1E4,D1
  move.b $0(A5,D1.L),D0
  move.l D0,D1
  asl.l #2,D1
  add.l D0,D1
  add.l D1,D1
  move.l A0,D0
  addi.l #$1E4,D0
  move.b D1,$0(A5,D0.L)
  movea.l $1A(A5),A0
  move.l A0,D0
  addi.l #$1E4,D0
  move.b $0(A5,D0.L),D0
  add.b D4,D0
  subi.b #$30,D0
  move.l A0,D1
  addi.l #$1E4,D1
  move.b D0,$0(A5,D1.L)
 bra.w JL_0_1834
 
JL_0_15D2:
  moveq.l #$3B,D0
  cmp.b D0,D4
   bne.b JL_0_1600
  lea $1A(A5),A3
  addq.l #1,(A3)
  movea.l (A3),A0
  move.l A0,D0
  addi.l #$1E4,D0
  clr.b $0(A5,D0.L)
  cmpi.l #$17,(A3)+
   bls.w JL_0_1834
  moveq.l #$17,D0
  move.l D0,$1A(A5)
 bra.w JL_0_1834
 
JL_0_1600:
  moveq.l #$1B,D0
  cmp.b D0,D4
   beq.w JL_0_1700
  move.w $52(A5),D0
  moveq.l #$0A,D1
  cmp.w D1,D0
   bcc.w JL_0_1834
  addq.w #1,$52(A5)
  moveq.l #$00,D1
  move.w D0,D1
  addi.l #$1FC,D1
  move.b D4,$0(A5,D1.L)
  moveq.l #$00,D1
  move.w $52(A5),D1
  addi.l #$1FC,D1
  clr.b $0(A5,D1.L)
 bra.w JL_0_1834
 
JL_0_163A:
  moveq.l #$00,D5
 bra.b JL_0_167E
 
JL_0_163E:
  cmp.b $5(A3),D4
   bne.b JL_0_167C
  move.b $1FC(A5),D0
  lea $2C(A4),A0
  cmp.b $4(A0,D6.L),D0
   bne.b JL_0_167C
  tst.w $50(A5)
   beq.b JL_0_1660
  move.l A5,-(A7)
   bsr.w JL_0_124E
  addq.w #4,A7
JL_0_1660:
  moveq.l #$00,D0
  move.b D4,D0
  move.w D0,$16(A5)
  movea.l (A3),A0
  move.l $1A(A5),-(A7)
  pea $1E4(A5)
  move.l A5,-(A7)
L_0_1674:
   jsr (A0)
  lea $C(A7),A7
 bra.b JL_0_1698
 
JL_0_167C:
  addq.w #1,D5
JL_0_167E:
  swap D5
  clr.w D5
  swap D5
  move.l D5,D0
  asl.l #2,D0
  sub.l D5,D0
  add.l D0,D0
  move.l D0,D6
  lea $2C(A4),A3
  adda.l D6,A3
  tst.l (A3)
   bne.b JL_0_163E
JL_0_1698:
  clr.w $16(A5)
 bra.w JL_0_1834
 
JL_0_16A0:
  moveq.l #$20,D0
  cmp.b D0,D4
  movea.l $20(A7),A6
   bcc.b JL_0_16B8
  tst.w $50(A5)
   beq.b JL_0_16B8
  move.l A5,-(A7)
   bsr.w JL_0_124E
  addq.w #4,A7
JL_0_16B8:
  moveq.l #$00,D0
  move.b D4,D0
  subq.l #7,D0
   beq.b JL_0_1724
  subq.l #1,D0
   beq.b JL_0_173C
  subq.l #1,D0
   beq.w JL_0_176A
  subq.l #1,D0
   beq.w JL_0_1778
  subq.l #1,D0
   beq.w JL_0_17A8
  subq.l #1,D0
   beq.w JL_0_17BA
  subq.l #1,D0
   beq.w JL_0_17C4
  subq.l #1,D0
   beq.w JL_0_17CC
  subq.l #1,D0
   beq.w JL_0_17D4
  moveq.l #$0C,D1
  sub.l D1,D0
   beq.b JL_0_1700
  subi.l #MinByteSgnd,D0
   beq.b JL_0_1712
 bra.w JL_0_17DA
 
JL_0_1700:
  move.l A5,-(A7)
   bsr.w JL_0_1228
  addq.w #4,A7
  move.w #$0001,$14(A5)
 bra.w JL_0_1834
 
JL_0_1712:
  move.l A5,-(A7)
   bsr.w JL_0_1228
  addq.w #4,A7
  move.w #$0001,$16(A5)
 bra.w JL_0_1834
 
JL_0_1724:
  movea.l $144(A4),A0
  movea.l $2E(A0),A0
  movea.l $16C(A4),A6
   jsr -$60(A6)
  movea.l $20(A7),A6
 bra.w JL_0_1834
 
JL_0_173C:
  move.w $28(A5),D0
  moveq.l #$01,D1
  cmp.w D1,D0
   bls.w JL_0_1834
  subq.w #1,$28(A5)
 bra.w JL_0_1834
 
JL_0_1750:
  addq.w #1,$28(A5)
  moveq.l #$00,D0
  move.w $28(A5),D0
  move.l D0,D1
  addi.l #$11C,D1
  tst.b $0(A5,D1.L)
   bne.w JL_0_1834
JL_0_176A:
  move.w $28(A5),D0
  cmp.w $20(A5),D0
   bcs.b JL_0_1750
 bra.w JL_0_1834
 
JL_0_1778:
  btst #$0,$47(A5)
   beq.b JL_0_1794
  clr.l -(A7)
  pea $E4(A4)
  move.l A5,-(A7)
   bsr.w AJL_0_1312
  lea $C(A7),A7
 bra.w JL_0_1834
 
JL_0_1794:
  clr.l -(A7)
  pea $E6(A4)
  move.l A5,-(A7)
   bsr.w AJL_0_1326
  lea $C(A7),A7
 bra.w JL_0_1834
 
JL_0_17A8:
  clr.l -(A7)
  pea $E8(A4)
  move.l A5,-(A7)
   bsr.w AJL_0_1406
  lea $C(A7),A7
 bra.b JL_0_1834
 
JL_0_17BA:
  move.l A5,-(A7)
   bsr.w JL_0_14C8
  addq.w #4,A7
 bra.b JL_0_1834
 
JL_0_17C4:
  move.w #$0001,$28(A5)
 bra.b JL_0_1834
 
JL_0_17CC:
  move.b #$80,$207(A5)
 bra.b JL_0_1834
 
JL_0_17D4:
  clr.b $207(A5)
 bra.b JL_0_1834
 
JL_0_17DA:
  move.w $50(A5),D0
   bne.b JL_0_17E8
  move.w $28(A5),D1
  move.w D1,$4E(A5)
JL_0_17E8:
  moveq.l #$00,D0
  move.w $50(A5),D0
  move.b D4,D1
  or.b $207(A5),D1
  move.b D1,$54(A5,D0.L)
  move.w $28(A5),D0
  addq.w #1,D0
  move.w D0,$28(A5)
  addq.w #1,$50(A5)
  cmp.w $20(A5),D0
   bls.b JL_0_1834
  move.l A5,-(A7)
   bsr.w JL_0_124E
  addq.w #4,A7
  btst #$1,$47(A5)
   beq.b JL_0_182E
  clr.l -(A7)
  pea $EA(A4)
  move.l A5,-(A7)
   bsr.w AJL_0_1312
  lea $C(A7),A7
 bra.b JL_0_1834
 
JL_0_182E:
  move.w $20(A5),$28(A5)
JL_0_1834:
  move.l D7,D0
  subq.l #1,D7
  tst.l D0
   bne.w JL_0_152C
  tst.w $50(A5)
   beq.b JL_0_184C
  move.l A5,-(A7)
   bsr.w JL_0_124E
  addq.w #4,A7
JL_0_184C:
  move.l A5,$28(A7)
  movem.l (A7)+,D4-D7/A2-A3/A5-A6
  addq.l #4,A7
 bra.w JL_0_1182
 
JL_0_185A:
  suba.w #$20,A7
  movem.l A4-A6,-(A7)
  lea $3C(A6),A4
  move.l D0,$150(A4)
  move.l $28(A0),$144(A4)
  movea.l $144(A4),A1
  move.l $32(A1),$148(A4)
  move.l A6,$10(A7)
  clr.l -(A7)
  pea $EC(A4)
  move.l #NP_Name,-(A7)
  pea $1000.W
  move.l #NP_StackSize,-(A7)
  pea JL_0_2EC(PC)
  move.l #NP_Entry,-(A7)
  movea.l $15C(A4),A6
  move.l A7,D1
   jsr -$1F2(A6)
  lea $1C(A7),A7
  movea.l D0,A5
  tst.l D0
  movea.l $10(A7),A6
   bne.b JL_0_18BA
  moveq.l #$01,D0
 bra.b JL_0_190E
 
JL_0_18BA:
  move.w #$0004,$26(A7)
  clr.l -(A7)
  clr.l -(A7)
  movea.l $18(A7),A6
   bsr.w JL_0_19E4
  addq.w #8,A7
  move.l D0,$22(A7)
  move.b #$5,$1C(A7)
  move.l A6,$28(A7)
  lea $5C(A5),A0
  lea $14(A7),A1
  movea.l $4.W,A6
PutMsg SET -$16E
   jsr PutMsg(A6)
  movea.l $22(A7),A0
WaitPort SET -$180
   jsr WaitPort(A6)
  move.l $14C(A4),D0
  movea.l $10(A7),A6
   beq.b JL_0_190E
  move.l $22(A7),-(A7)
  movea.l $14(A7),A6
   bsr.w JL_0_1A62
  addq.w #4,A7
  moveq.l #$00,D0
JL_0_190E:
  movem.l (A7)+,A4-A6
  adda.w #$20,A7
 rtS 
 
JL_0_1918:
  suba.w #$24,A7
  movem.l A4-A6,-(A7)
  movea.l A0,A5
  lea $3C(A6),A4
  move.l A6,$C(A7)
  movea.l $4.W,A6
CreateMsgport SET -$29A
   jsr CreateMsgport(A6)
  move.l D0,$1E(A7)
  move.w #$7FF0,$2C(A7)
  move.l $18(A5),$28(A7)
  movea.l $14C(A4),A0
  lea $10(A7),A1
PutMsg SET -$16E
   jsr PutMsg(A6)
  movea.l $1E(A7),A0
WaitPort SET -$180
   jsr WaitPort(A6)
  movea.l $1E(A7),A0
DeleteMsgport SET -$2A0
   jsr DeleteMsgport(A6)
  move.l $14C(A4),-(A7)
  movea.l $10(A7),A6
   bsr.w JL_0_1A62
  addq.w #4,A7
  movem.l (A7)+,A4-A6
  adda.w #$24,A7
 rtS 
 
AJL_0_1976:
  subq.w #4,A7
  movem.l A4/A6,-(A7)
  lea $3C(A6),A4
  clr.b $1F(A1)
  bclr #$0,$1E(A1)
  moveq.l #$00,D0
  move.w $1C(A1),D0
  move.l A6,$8(A7)
  subq.l #3,D0
   bne.b JL_0_19A6
  movea.l $14C(A4),A0
  movea.l $4.W,A6
PutMsg SET -$16E
   jsr PutMsg(A6)
 bra.b JL_0_19B4
 
JL_0_19A6:
  move.b #$FD,$1F(A1)
  movea.l $4.W,A6
ReplyMsg SET -$17A
   jsr ReplyMsg(A6)
JL_0_19B4:
  movea.l $8(A7),A6
  movem.l (A7)+,A4/A6
  addq.w #4,A7
 rtS 
 
AJL_0_19C0:
  subq.w #4,A7
  move.l A4,-(A7)
  lea $3C(A6),A4
  movea.l (A7)+,A4
  addq.w #4,A7
 rtS 
 
JL_0_19D4:
  clr.l $4(A0)
  move.l A0,$8(A0)
  addq.l #4,A0
  move.l A0,-(A0)
 rtS 
 

JL_0_19E4:
  movea.l $4(A7),A0
  move.l $8(A7),D0
  movem.l D6-D7/A3/A5-A6,-(A7)
  move.l D0,D7
  moveq.l #-$01,D0
  movea.l $4.W,A6
  movea.l A0,A5
AllocSignal SET -$14A
   jsr AllocSignal(A6)
  moveq.l #$00,D6
  move.b D0,D6
  tst.l D6
   ble.b JL_0_1A5A
  moveq.l #$22,D0
  move.l #MemfPublic_MemfClear,D1
AllocMem SET -$C6
   jsr AllocMem(A6)
  movea.l D0,A3
  tst.l D0
   beq.b JL_0_1A50
  lea $A(A3),A0
  move.l A5,(A0)+
  move.l D7,D0
  move.b D0,$9(A3)
  move.b #$4,$8(A3)
  clr.b (A0)+
  suba.l A1,A1
  move.b D6,(A0)+
FindTask SET -$126
   jsr FindTask(A6)
  move.l D0,$10(A3)
  move.l A5,D0
   beq.b JL_0_1A44
  movea.l A3,A1
AddPort SET -$162
   jsr AddPort(A6)
 bra.b JL_0_1A4C
 
JL_0_1A44:
  lea $14(A3),A0
   bsr.w JL_0_19D4
JL_0_1A4C:
  move.l A3,D0
 bra.b JL_0_1A5C
 
JL_0_1A50:
  move.l D6,D0
  movea.l $4.W,A6
FreeSignal SET -$150
   jsr FreeSignal(A6)
JL_0_1A5A:
  moveq.l #$00,D0
JL_0_1A5C:
  movem.l (A7)+,D6-D7/A3/A5-A6
 rtS 
 
JL_0_1A62:
  movea.l $4(A7),A0
  movem.l A5-A6,-(A7)
  movea.l A0,A5
  tst.l $A(A5)
   beq.b JL_0_1A7C
  movea.l A5,A1
  movea.l $4.W,A6
RemPort SET -$168
   jsr RemPort(A6)
JL_0_1A7C:
  St $8(A5)
  moveq.l #-$01,D0
  move.l D0,$14(A5)
  moveq.l #$00,D0
  move.b $F(A5),D0
  movea.l $4.W,A6
FreeSignal SET -$150
   jsr FreeSignal(A6)
  movea.l A5,A1
  moveq.l #$22,D0
FreeMem SET -$D2
   jsr FreeMem(A6)
  movem.l (A7)+,A5-A6
 rtS 
 

AJL_0_1AA4:
  move.l D7,-(A7)
  moveq.l #$00,D7
  addq.l #4,$120(A4)
 bra.b JL_0_1AC6
 
JL_0_1AAE:
  tst.b $154(A4)
   bne.b JL_0_1AB8
  tst.l D7
   bne.b JL_0_1AC2
JL_0_1AB8:
  movea.l $120(A4),A1
  movea.l (A1),A0
L_0_1ABE:
   jsr (A0)
  move.l D0,D7
JL_0_1AC2:
  addq.l #4,$120(A4)
JL_0_1AC6:
  movea.l $120(A4),A0
  tst.l (A0)
   bne.b JL_0_1AAE
  move.b #$1,$154(A4)
  move.l D7,D0
  move.l (A7)+,D7
 rtS 
 

JL_0_1ADC:
  subq.w #8,A7
  move.l A4,-(A7)
  lea AL_3_100,A0
SysStkLower SET $3A
  lea SysStkLower+2(A6),A4
  move.l A0,$8(A7)
  move.l A6,$4(A7)
  tst.l $8(A7)
   beq.b JL_0_1B08
  movea.l AL_3_FC,A0
L_0_1AFE:
   jsr (A0)
  tst.l D0
   beq.b JL_0_1B08
  moveq.l #$01,D0
  OWNINSTRUKTION $0C40 
JL_0_1B08:
  moveq.l #$00,D0
  movea.l (A7)+,A4
  addq.w #8,A7
 rtS 
 
JL_0_1B10:
  subq.w #8,A7
  move.l A4,-(A7)
  lea AL_3_110,A0
SysStkLower SET $3A
  lea SysStkLower+2(A6),A4
  move.l A0,$8(A7)
  move.l A6,$4(A7)
  tst.l $8(A7)
   beq.b JL_0_1B34
  movea.l AL_3_FC,A0
L_0_1B32:
   jsr (A0)
JL_0_1B34:
  movea.l (A7)+,A4
  addq.w #8,A7
 rtS 
 

JL_0_1B3C:
  movea.l $4(A7),A0
  move.l $8(A7),D0
  move.l $C(A7),D1
  tst.l D0
   ble.b JL_0_1B52
JL_0_1B4C:
  move.b D1,(A0)+
  subq.l #1,D0
   bne.b JL_0_1B4C
JL_0_1B52:
 rtS 
 
JL_0_1B54:
  tst.l D0
   bpl.w JL_0_1B76
  neg.l D0
  tst.l D1
   bpl.w JL_0_1B6C
  neg.l D1
   bsr.w JL_0_1B86
  neg.l D1
 rtS 
 
JL_0_1B6C:
   bsr.w JL_0_1B86
  neg.l D0
  neg.l D1
 rtS 
 
JL_0_1B76:
  tst.l D1
   bpl.w JL_0_1B86
  neg.l D1
   bsr.w JL_0_1B86
  neg.l D0
 rtS 
 
JL_0_1B86:
  move.l D2,-(A7)
  swap D1
  move.w D1,D2
   bne.w JL_0_1BB0
  swap D0
  swap D1
  swap D2
  move.w D0,D2
   beq.w JL_0_1BA0
  divu D1,D2
  move.w D2,D0
JL_0_1BA0:
  swap D0
  move.w D0,D2
  divu D1,D2
  move.w D2,D0
  swap D2
  move.w D2,D1
  move.l (A7)+,D2
 rtS 
 
JL_0_1BB0:
  move.l D3,-(A7)
  moveq.l #$10,D3
  cmpi.w #$100,D1
   bcc.w JL_0_1BC0
  rol.l #8,D1
  subq.w #8,D3
JL_0_1BC0:
  cmpi.w #$1000,D1
   bcc.w JL_0_1BCC
  rol.l #4,D1
  subq.w #4,D3
JL_0_1BCC:
  cmpi.w #$4000,D1
   bcc.w JL_0_1BD8
  rol.l #2,D1
  subq.w #2,D3
JL_0_1BD8:
  tst.w D1
   bmi.w JL_0_1BE2
  rol.l #1,D1
  subq.w #1,D3
JL_0_1BE2:
  move.w D0,D2
  lsr.l D3,D0
  swap D2
  clr.w D2
  lsr.l D3,D2
  swap D3
  divu D1,D0
  move.w D0,D3
  move.w D2,D0
  move.w D3,D2
  swap D1
  mulu D1,D2
  sub.l D2,D0
   bcc.w JL_0_1C04
  subq.w #1,D3
  add.l D1,D0
JL_0_1C04:
  moveq.l #$00,D1
  move.w D3,D1
  swap D3
  rol.l D3,D0
  swap D0
  exg.l D0,D1
  move.l (A7)+,D3
  move.l (A7)+,D2
 rtS 
 
  ds.w 1
JL_0_1C18:
  movem.l D2-D3,-(A7)
  move.l D0,D2
  move.l D1,D3
  swap D2
  swap D3
  mulu D1,D2
  mulu D0,D3
  mulu D1,D0
  add.w D3,D2
  swap D2
  clr.w D2
  add.l D2,D0
  movem.l (A7)+,D2-D3
 rtS 
 
JL_0_1C38:
  suba.w #$34,A7
  movem.l D2-D3/D7/A2-A3/A5-A6,-(A7)
  lea dosname(PC),A1
  movea.l $4.W,A6
  moveq.l #$00,D0
  movea.l A0,A5
OpenLibrary SET -$228
   jsr OpenLibrary(A6)
  movea.l D0,A3
  tst.l $138(A4)
   bne.b JL_0_1C62
  movea.l A3,A6
Output SET -$3C
   jsr Output(A6)
  move.l D0,D7
 bra.b JL_0_1C76
 
JL_0_1C62:
  lea $124(A4),A0
  move.l A0,D1
  move.l #HUNK_RELOC_8__MODE_NEWFILE,D2
  movea.l A3,A6
Open SET -$1E
   jsr Open(A6)
  move.l D0,D7
JL_0_1C76:
  tst.l D7
   beq.b JL_0_1CE8
  lea L_0_1D32(PC),A0
  move.l A3,-(A7)
  lea $158(A4),A1
  lea $22(A7),A3
  movea.l $4.W,A6
  movea.l A0,A2
  lea L_0_1D16(PC),A0
RawDoFmt SET -$20A
   jsr RawDoFmt(A6)
  movea.l (A7)+,A3
  lea $1E(A7),A0
  movea.l A0,A1
JL_0_1C9E:
  tst.b (A1)+
   bne.b JL_0_1C9E
  subq.l #1,A1
  move.l A0,D2
  suba.l A0,A1
  movea.l A3,A6
  move.l A1,D3
  move.l D7,D1
Write SET -$30
   jsr Write(A6)
  movea.l A5,A0
JL_0_1CB4:
  tst.b (A0)+
   bne.b JL_0_1CB4
  subq.l #1,A0
  suba.l A5,A0
  move.l A0,D3
  move.l D7,D1
  move.l A5,D2
Write SET -$30
   jsr Write(A6)
  lea L_0_1D38(PC),A0
  move.l D7,D1
  move.l A0,D2
  moveq.l #$01,D3
Write SET -$30
   jsr Write(A6)
  tst.l $138(A4)
   beq.b JL_0_1CE8
  moveq.l #$64,D1
  add.l D1,D1
Delay SET -$C6
   jsr Delay(A6)
  move.l D7,D1
Close SET -$24
   jsr Close(A6)
JL_0_1CE8:
  movea.l A3,A1
  movea.l $4.W,A6
CloseLibrary SET -$19E
   jsr CloseLibrary(A6)
  suba.l A1,A1
FindTask SET -$126
   jsr FindTask(A6)
  moveq.l #$7A,D1
  movea.l D0,A0
pr_Result2 SET $94
  move.l D1,pr_Result2(A0)
  movem.l (A7)+,D2-D3/D7/A2-A3/A5-A6
  adda.w #$34,A7
 rtS 
 
dosname:
  dc.b "dos.library",0
L_0_1D16:
  dc.b "Can't open version %ld of ",0,0
L_0_1D32:
  dc.b $16 ;.
  dc.b $C0 ;.
  dc.b "Nu",0,0
L_0_1D38:
  dc.b 10,0
  dc.b "Nq"
AJL_0_1D3C:
  move.l A6,-(A7)
  lea B2_dosname(PC),A1
  move.l $158(A4),D0
  movea.l $4.W,A6
OpenLibrary SET -$228
   jsr OpenLibrary(A6)
  move.l D0,$15C(A4)
  move.l D0,$160(A4)
   bne.b JL_0_1D64
  lea B2_dosname(PC),A0
   bsr.w JL_0_1C38
  moveq.l #$01,D0
  OWNINSTRUKTION $0C40 
JL_0_1D64:
  moveq.l #$00,D0
  movea.l (A7)+,A6
 rtS 
 
B2_dosname:
  dc.b "dos.library",0
AJL_0_1D76:
  move.l A6,-(A7)
  move.l $160(A4),D0
   beq.b JL_0_1D92
  movea.l D0,A1
  movea.l $4.W,A6
CloseLibrary SET -$19E
   jsr CloseLibrary(A6)
  suba.l A0,A0
  move.l A0,$15C(A4)
  move.l A0,$160(A4)
JL_0_1D92:
  movea.l (A7)+,A6
 rtS 
 

AJL_0_1D98:
  move.l A6,-(A7)
  lea graficsname(PC),A1
  move.l $158(A4),D0
  movea.l $4.W,A6
OpenLibrary SET -$228
   jsr OpenLibrary(A6)
  move.l D0,$164(A4)
  move.l D0,$168(A4)
   bne.b JL_0_1DC0
  lea graficsname(PC),A0
   bsr.w JL_0_1C38
  moveq.l #$01,D0
  OWNINSTRUKTION $0C40 
JL_0_1DC0:
  moveq.l #$00,D0
  movea.l (A7)+,A6
 rtS 
 
graficsname:
  dc.b "graphics.library",0,0
AJL_0_1DD8:
  move.l A6,-(A7)
  move.l $168(A4),D0
   beq.b JL_0_1DF4
  movea.l D0,A1
  movea.l $4.W,A6
CloseLibrary SET -$19E
   jsr CloseLibrary(A6)
  suba.l A0,A0
  move.l A0,$164(A4)
  move.l A0,$168(A4)
JL_0_1DF4:
  movea.l (A7)+,A6
 rtS 
 
AJL_0_1DF8:
  move.l A6,-(A7)
  lea intname(PC),A1
  move.l $158(A4),D0
  movea.l $4.W,A6
OpenLibrary SET -$228
   jsr OpenLibrary(A6)
  move.l D0,$16C(A4)
  move.l D0,$170(A4)
   bne.b JL_0_1E20
  lea intname(PC),A0
   bsr.w JL_0_1C38
  moveq.l #$01,D0
  OWNINSTRUKTION $0C40 
JL_0_1E20:
  moveq.l #$00,D0
  movea.l (A7)+,A6
 rtS 
 
intname:
  dc.b "intuition.library",0
AJL_0_1E38:
  move.l A6,-(A7)
  move.l $170(A4),D0
   beq.b JL_0_1E54
  movea.l D0,A1
  movea.l $4.W,A6
CloseLibrary SET -$19E
   jsr CloseLibrary(A6)
  suba.l A0,A0
  move.l A0,$16C(A4)
  move.l A0,$170(A4)
JL_0_1E54:
  movea.l (A7)+,A6
 rtS 
 
 SECTION "Segment1",DATA
 cnop 0,4
SegmentBeginn1:
  dc.l $1B0
AL_1_4:
 dc.l SegmentBeginn2
  ds.l 1
AL_1_C:
 dc.l AJL_0_20
 SECTION "Segment2",CODE
 cnop 0,4
SegmentBeginn2:
 dc.l AJL_0_D0
AL_2_4:
 dc.l AJL_0_20E
AL_2_8:
 dc.l AJL_0_27A
AL_2_C:
 dc.l SegmentBeginn0
AL_2_10:
 dc.l AJL_0_1976
AL_2_14:
 dc.l AJL_0_19C0
  dc.b $FF,$FF,$FF ;...
  dc.b $FF ;.
AL_2_1C:
  dc.b "ibmcon.device",0,0
  dc.b $00 ;.
AL_2_2C:
  dc.b "ibmcon.device 1.4",0,0
  dc.b $00 ;.
 SECTION "Segment3",DATA
 cnop 0,4
SegmentBeginn3:
  ds.l 1
  dc.b $00 ;.
  dc.b "$VER: ibmcon.device 1.4 (Mar  9 1998)",0,0
AL_3_2C:
 dc.l AJL_0_600
  dc.b $00,$40 ;.@
AL_3_32:
 dc.l AJL_0_1406
  dc.b $00,$41 ;.A
AL_3_38:
 dc.l AJL_0_1326
  dc.b $00,$42 ;.B
AL_3_3E:
 dc.l AJL_0_10D2
  dc.b $00,$43 ;.C
AL_3_44:
 dc.l AJL_0_1128
  dc.b $00,$44 ;.D
AL_3_4A:
 dc.l AJL_0_1312
  dc.b $00,$45 ;.E
AL_3_50:
 dc.l AJL_0_10BE
  dc.b $00,$46 ;.F
AL_3_56:
 dc.l AJL_0_1076
  dc.b $00,$48 ;.H
AL_3_5C:
 dc.l AJL_0_FF4
  dc.b $00,$4A ;.J
AL_3_62:
 dc.l AJL_0_DB2
  dc.b $00,$4B ;.K
AL_3_68:
 dc.l AJL_0_D0C
  dc.b $00,$4C ;.L
AL_3_6E:
 dc.l AJL_0_C60
  dc.b $00,$4D ;.M
AL_3_74:
 dc.l AJL_0_BBE
  dc.b $00,$50 ;.P
AL_3_7A:
 dc.l AJL_0_6A4
  dc.b $00,$52 ;.R
AL_3_80:
 dc.l AJL_0_5B4
  dc.b $00,$72 ;.r
AL_3_86:
 dc.l AJL_0_ADE
  dc.b $00,$53 ;.S
AL_3_8C:
 dc.l AJL_0_B4E
  dc.b $00,$54 ;.T
AL_3_92:
 dc.l AJL_0_56A
  dc.b $00,$74 ;.t
AL_3_98:
 dc.l AJL_0_1076
  dc.b $00,$66 ;.f
AL_3_9E:
 dc.l AJL_0_748
  dc.b $00,$68 ;.h
AL_3_A4:
 dc.l AJL_0_7D0
  dc.b $00,$6C ;.l
AL_3_AA:
 dc.l AJL_0_748
  dc.b ">h"
AL_3_B0:
 dc.l AJL_0_7D0
  dc.b ">l"
AL_3_B6:
 dc.l AJL_0_748
  dc.b "?h"
AL_3_BC:
 dc.l AJL_0_7D0
  dc.b "?l"
AL_3_C2:
 dc.l AJL_0_858
  dc.b $00,$6D ;.m
AL_3_C8:
 dc.l AJL_0_AD8
  dc.b $00,$6E ;.n
AL_3_CE:
 dc.l AJL_0_6AA
  dc.b $00,$73 ;.s
AL_3_D4:
 dc.l AJL_0_6DA
  dc.b $00,$75,$00,$00 ;.u..
  ds.l 1
  dc.b $01,$00,$01,$00 ;....
  dc.b $01,$00,$01,$00 ;....
  dc.b $01,$00,$01 ;...
  dc.b $00 ;.
  dc.b "IBMCON_Handler",0,0
AL_3_FC:
 dc.l AJL_0_1AA4
AL_3_100:
 dc.l AJL_0_1D98
AL_3_104:
 dc.l AJL_0_1DF8
AL_3_108:
 dc.l AJL_0_1D3C
  ds.l 1
AL_3_110:
 dc.l AJL_0_1D76
AL_3_114:
 dc.l AJL_0_1E38
AL_3_118:
 dc.l AJL_0_1DD8
  ds.w 3
  dc.b $00 ;.
  dc.b "ücon:10/10/320/80/",0,0
  ds.b 3
  dc.b $00,$01,$00,$00 ;....
  dc.b $01,$20 ;. 
 End
