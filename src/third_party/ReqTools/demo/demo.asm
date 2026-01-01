**********************************
*                                *
*  reqtools.library (V38)        *
*                                *
*  Release 2.5                   *
*                                *
*  (c) 1991-1994 Nico François   *
*      1995-1996 Nico François   *
*                                *
*  Assembly version by           *
*  Dirk Vangestel                *
*                                *
*  demo.asm                      *
*                                *
*  This source is public domain  *
*  in all respects.              *
*                                *
**********************************

	INCLUDE	"exec/funcdef.i"
	INCLUDE	"exec/exec_lib.i"
	INCLUDE	"exec/libraries.i"
	INCLUDE	"libraries/dos_lib.i"
	INCLUDE	"libraries/reqtools.i"
	INCLUDE	"libraries/reqtools_lib.i"
	INCLUDE	"utility/tagitem.i"
	INCLUDE	"intuition/intuition.i"

FALSE	equ	0		;dummy for C-compatibility
TRUE	equ	1

start
	move.l	$4.w,a6
	lea	ReqName(pc),a1
	move.l	#REQTOOLSVERSION,d0
	jsr	_LVOOpenLibrary(a6)
	move.l	d0,ReqBase
	beq.w	NoReqtools
	move.l	d0,a6

	lea	Text1(pc),a1
	lea	Text2(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text3(pc),a1
	lea	Text4(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text5(pc),a1
	lea	Text6(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Buffer(pc),a1
	lea	Text7(pc),a0
.loop1
	move.b	(a0)+,(a1)+
	bne.s	.loop1
	moveq	#127,d0
	lea	Buffer(pc),a1
	lea	Text8(pc),a2
	sub.l	a3,a3
	move.l	#TAG_END,a0
	jsr	_LVOrtGetStringA(a6)
	tst.l	d0
	bne.s	.input1
	lea	Text9(pc),a1
	lea	Text10(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.s	.ok1
.input1
	lea	Text11(pc),a1
	lea	Text12(pc),a2
	sub.l	a3,a3
	lea	Buffer(pc),a0
	lea	Array(pc),a4
	move.l	a0,(a4)
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
.ok1

	lea	Buffer(pc),a1
	moveq	#127,d0
	lea	Text8(pc),a2
	sub.l	a3,a3
	lea	Tags1(pc),a0
	jsr	_LVOrtGetStringA(a6)
	subq	#2,d0
	bne.s	.ok2
	lea	Text13(pc),a1
	lea	Text14(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
.ok2

	lea	Buffer(pc),a1
	moveq	#127,d0
	lea	Text8(pc),a2
	sub.l	a3,a3
	lea	Tags2(pc),a0
	jsr	_LVOrtGetStringA(a6)
	subq	#2,d0
	bne.s	.ok3
	lea	Text15(pc),a1
	lea	Text16(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
.ok3

	lea	Text17(pc),a1
	lea	Text6(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	LongNum(pc),a1
	lea	Text18(pc),a2
	sub.l	a3,a3
	lea	Tags3(pc),a0
	jsr	_LVOrtGetLongA(a6)
	tst.l	d0
	bne.s	.input2
	lea	Text9(pc),a1
	lea	Text10(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.s	.ok5
.input2
	lea	Text19(pc),a1
	lea	Text20(pc),a2
	sub.l	a3,a3
	lea	Array(pc),a4
	move.l	LongNum(pc),d0
	move.l	d0,(a4)+
	cmpi.l	#666,d0
	bne.s	.nodevil
	lea	Text21(pc),a0
	move.l	a0,(a4)+
	bra.s	.ok4
.nodevil
	lea	Null(pc),a0
	move.l	a0,(a4)+
.ok4
	lea	Array(pc),a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
.ok5

	lea	Text22(pc),a1
	lea	Text23(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text24(pc),a1
	lea	Text25(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

.ret
	lea	Text26(pc),a1
	lea	Text27(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	tst.l	d0
	bne.s	.ok6
	lea	Text28(pc),a1
	lea	Text29(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.s	.ret
.ok6
	lea	Text30(pc),a1
	lea	Text31(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text32(pc),a1
	lea	Text33(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	cmpi.l	#FALSE,d0
	bne.s	.ok7
	lea	Text34(pc),a1
	lea	Text35(pc),a2
	bra.s	.ok9
.ok7
	cmpi.l	#TRUE,d0
	bne.s	.ok8
	lea	Text36(pc),a1
	lea	Text31(pc),a2
	bra.s	.ok9
.ok8
	lea	Text37(pc),a1
	lea	Text38(pc),a2
.ok9
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text39(pc),a1
	lea	Text40(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags4(pc),a0
	jsr	_LVOrtEZRequestA(a6)
	lea	Text41(pc),a1
	lea	Text42(pc),a2
	sub.l	a3,a3
	sub.l	a0,a0
	lea	Array(pc),a4
	move.l	d0,(a4)
	jsr	_LVOrtEZRequestA(a6)

	lea	Text43(pc),a1
	lea	Text44(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text45(pc),a1
	lea	Text46(pc),a2
	sub.l	a3,a3
	lea	Array(pc),a4
	move.l	#5,(a4)+
	lea	Buffer(pc),a0
	move.l	a0,(a4)+
	move.l	#"five",(a0)+
	clr.b	(a0)+
	lea	Array(pc),a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text47(pc),a1
	lea	Text48(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags6(pc),a0
	jsr	_LVOrtEZRequestA(a6)
	cmpi.l	#DISKINSERTED,d0
	bne.s	.ok10
	lea	Text49(pc),a1
	bra.s	.ok11
.ok10
	lea	Text50(pc),a1
.ok11
	lea	Text51(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text52(pc),a1
	lea	Text53(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags7(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text54(pc),a1
	lea	Text55(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags8(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text56(pc),a1
	lea	Text57(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	move.l	#RT_FILEREQ,d0
	sub.l	a0,a0
	jsr	_LVOrtAllocRequestA(a6)
	move.l	d0,filereq
	bne.s	.ok12
	lea	Text58(pc),a1
	lea	Text14(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.w	.ok13
.ok12
	move.l	filereq(pc),a1
	lea	filename(pc),a2
	clr.b	(a2)
	lea	Text59(pc),a3
	sub.l	a0,a0
	jsr	_LVOrtFileRequestA(a6)
	tst.l	d0
	beq.s	.ok14
	lea	Text60(pc),a1
	lea	Text61(pc),a2
	sub.l	a3,a3
	lea	Array(pc),a4
	lea	filename(pc),a0
	move.l	a0,(a4)+
	move.l	filereq(pc),a0
	move.l	rtfi_Dir(a0),(a4)+
	lea	Array(pc),a4
	bra.s	.ok15
.ok14
	lea	Text62(pc),a1
	lea	Text63(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
.ok15
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text64(pc),a1
	lea	Text65(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	move.l	filereq(pc),a1
	lea	filename(pc),a2
	lea	Text66(pc),a3
	lea	Tags9(pc),a0
	jsr	_LVOrtFileRequestA(a6)
	move.l	d0,flist
	beq.s	.ok16
	lea	Text67(pc),a1
	lea	Text68(pc),a2
	sub.l	a3,a3
	lea	Array(pc),a4
	move.l	flist(pc),a0
	move.l	rtfl_Name(a0),(a4)
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

;Traverse all selected files.
;This is not tested, but should work fine!
;	move.l	flist(pc),a0
;.next
;	move.l	rtfl_Name(a0),filename
;	move.l	rtfl_StrLen(a0),length
;	move.l	rtfl_Next(a0),a0
;	bne.s	.next

	move.l	flist(pc),a0
	jsr	_LVOrtFreeFileList(a6)
.ok16
	move.l	filereq(pc),a1
	jsr	_LVOrtFreeRequest(a6)
.ok13

	lea	Text69(pc),a1
	lea	Text70(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	move.l	#RT_FILEREQ,d0
	sub.l	a0,a0
	jsr	_LVOrtAllocRequestA(a6)
	move.l	d0,filereq
	bne.s	.ok17
	lea	Text58(pc),a1
	lea	Text14(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.w	.ok18
.ok17
	move.l	filereq(pc),a1
	lea	filename(pc),a2
	clr.b	(a2)
	lea	Text71(pc),a3
	lea	Tags10(pc),a0
	jsr	_LVOrtFileRequestA(a6)
	tst.l	d0
	beq.s	.ok19
	lea	Text72(pc),a1
	lea	Text73(pc),a2
	lea	Array(pc),a4
	move.l	filereq(pc),a0
	move.l	rtfi_Dir(a0),(a4)
	bra.s	.ok20
.ok19
	lea	Text74(pc),a1
	lea	Text63(pc),a2
	sub.l	a4,a4
.ok20
	sub.l	a3,a3
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	move.l	filereq(pc),a1
	jsr	_LVOrtFreeRequest(a6)
.ok18

	lea	Text75(pc),a1
	lea	Text76(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)

	move.l	#RT_FONTREQ,d0
	sub.l	a0,a0
	jsr	_LVOrtAllocRequestA(a6)
	move.l	d0,fontreq
	bne.s	.ok21
	lea	Text58(pc),a1
	lea	Text14(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.s	.ok22
.ok21
	move.l	d0,a1
	move.l	#FREQF_STYLE+FREQF_COLORFONTS,rtfo_Flags(a1)
	lea	Text77(pc),a3
	sub.l	a0,a0
	jsr	_LVOrtFontRequestA(a6)
	tst.l	d0
	beq.s	.ok23
	lea	Text78(pc),a1
	lea	Text61(pc),a2
	lea	Array(pc),a4
	move.l	fontreq(pc),a0
	move.l	rtfo_Attr+ta_Name(a0),(a4)+
	moveq	#0,d0
	move.w	rtfo_Attr+ta_YSize(a0),d0
	move.l	d0,(a4)+
	lea	Array(pc),a4
	sub.l	a0,a0
	bra.s	.ok24
.ok23
	lea	Text79(pc),a1
	lea	Text80(pc),a2
	sub.l	a4,a4
	lea	Tags5(pc),a0
.ok24
	sub.l	a3,a3
	jsr	_LVOrtEZRequestA(a6)
	move.l	fontreq(pc),a1
	jsr	_LVOrtFreeRequest(a6)
.ok22

	lea	Text81(pc),a1
	lea	Text46(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	lea	Text82(pc),a2
	sub.l	a3,a3
	sub.l	a0,a0
	jsr	_LVOrtPaletteRequestA(a6)
	cmpi.l	#-1,d0
	bne.s	.ok25
	lea	Text83(pc),a1
	lea	Text84(pc),a2
	sub.l	a4,a4
	bra.s	.ok26
.ok25
	lea	Text85(pc),a1
	lea	Text86(pc),a2
	lea	Array(pc),a4
	move.l	d0,(a4)
.ok26
	sub.l	a3,a3
	jsr	_LVOrtEZRequestA(a6)

	lea	Text87(pc),a1
	lea	Text88(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	move.l	#RT_FILEREQ,d0
	sub.l	a0,a0
	jsr	_LVOrtAllocRequestA(a6)
	move.l	d0,filereq
	bne.s	.ok27
	lea	Text58(pc),a1
	lea	Text14(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.s	.ok28
.ok27
	move.l	d0,a1
	sub.l	a2,a2
	lea	Text89(pc),a3
	lea	Tags11(pc),a0
	jsr	_LVOrtFileRequestA(a6)
	beq.s	.ok29
	lea	Text90(pc),a1
	lea	Text61(pc),a2
	move.l	filereq(pc),a0
	lea	Array(pc),a4
	move.l	rtfi_Dir(a0),(a4)
	bra.s	.ok30
.ok29
	lea	Text91(pc),a1
	lea	Text92(pc),a2
	sub.l	a4,a4
.ok30
	sub.l	a3,a3
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	move.l	filereq(pc),a1
	jsr	_LVOrtFreeRequest(a6)
.ok28

	lea	Text93(pc),a1
	lea	Text46(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	move.l	$4.w,a5
	move.l	LIB_VERSION(a5),d0
	cmpi.l	#37,d0
	bcc.s	.ok31
	lea	Text94(pc),a1
	lea	Text95(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)
	bra.w	.ok32
.ok31
	move.l	#RT_SCREENMODEREQ,d0
	sub.l	a0,a0
	jsr	_LVOrtAllocRequestA(a6)
	move.l	d0,scrmodereq
	bne.s	.ok33
	lea	Text58(pc),a1
	lea	Text14(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	sub.l	a0,a0
	jsr	_LVOrtEZRequestA(a6)
	bra.s	.ok32
.ok33
	move.l	d0,a1
	lea	Text96(pc),a3
	lea	Tags12(pc),a0
	jsr	_LVOrtScreenModeRequestA(a6)
	tst.l	d0
	beq.s	.ok37
	lea	Text97(pc),a1
	lea	Text61(pc),a2
	lea	Array(pc),a4
	move.l	scrmodereq(pc),a0
	move.l	rtsc_DisplayID(a0),(a4)+
	moveq	#0,d0
	move.w	rtsc_DisplayWidth(a0),d0
	move.l	d0,(a4)+
	moveq	#0,d0
	move.w	rtsc_DisplayHeight(a0),d0
	move.l	d0,(a4)+
	moveq	#0,d0
	move.w	rtsc_DisplayDepth(a0),d0
	move.l	d0,(a4)+
	moveq	#0,d0
	move.w	rtsc_OverscanType(a0),d0
	move.l	d0,(a4)+
	move.l	rtsc_AutoScroll(a0),d0
	beq.s	.ok35
	lea	Ontxt(pc),a0
	bra.s	.ok36
.ok35
	lea	Offtxt(pc),a0
.ok36
	move.l	a0,(a4)+
	lea	Array(pc),a4
	sub.l	a0,a0
	bra.s	.ok34
.ok37
	lea	Text98(pc),a1
	lea	Text80(pc),a2
	lea	Tags5(pc),a0
	sub.l	a4,a4
.ok34
	sub.l	a3,a3
	jsr	_LVOrtEZRequestA(a6)
	move.l	scrmodereq(pc),a1
	jsr	_LVOrtFreeRequest(a6)
.ok32

	lea	Text99(pc),a1
	lea	Text100(pc),a2
	sub.l	a3,a3
	sub.l	a4,a4
	lea	Tags5(pc),a0
	jsr	_LVOrtEZRequestA(a6)

	move.l	ReqBase(pc),a1
	move.l	$4.w,a6
	jsr	_LVOCloseLibrary(a6)
	moveq	#0,d0
	rts

NoReqtools
	move.l	$4.w,a6
	lea	DosLib(pc),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,DosBase
	move.l	d0,a6
	jsr	_LVOOutput(a6)
	move.l	d0,d1
	move.l	#NoReq,d2
	move.l	#Ontxt-NoReq,d3
	jsr	_LVOWrite(a6)
	move.l	a6,a1
	move.l	$4.w,a6
	jsr	_LVOCloseLibrary(a6)
	moveq	#0,d0
	rts

NoReq
	dc.b	"You need reqtools.library V38 or higher!",10
	dc.b	"Please install it in your Libs: directory.",10,0
Ontxt
	dc.b	"On",0
Offtxt
	dc.b	"Off",0
Text1
	dc.b	"Reqtools 2.0 Demo",10
	dc.b	"~~~~~~~~~~~~~~~~~",10
	dc.b	"'reqtools.library' offers several",10
	dc.b	"different types of requesters:",0
Text2
	dc.b	"Let's see them",0
Text3
	dc.b	"NUMBER 1:",10,"The larch :-)",0
Text4
	dc.b	"Be serious!",0
Text5
	dc.b	"NUMBER 1:",10,"String requester",10
	dc.b	"function: rtGetString()",0
Text6
	dc.b	"Show me",0
Text7
	dc.b	"A bit of text",0
Text8
	dc.b	"Enter anything:",0
Text9
	dc.b	"You entered nothing :-(",0
Text10
	dc.b	"I'm sorry",0
Text11
	dc.b	"You entered this string:",10,"%s",0
Text12
	dc.b	"So I did",0
Text13
	dc.b	"Yep, this is a new",10
	dc.b	"ReqTools 2.0 feature!",0
Text14
	dc.b	"Oh boy!",0
Text15
	dc.b	"What!! You pressed abort!?!",10
	dc.b	"You must be joking :-)",0
Text16
	dc.b	"Ok, Continue",0
Text17
	dc.b	"NUMBER 2:",10
	dc.b	"Number requester",10
	dc.b	"function: rtGetLong()",0
Text18
	dc.b	"Enter a number:",0
Text19
	dc.b	"The number you entered was:",10,"%ld%s",0
Text20
	dc.b	"So it was",0
Text21
	dc.b	" (you devil! :)",0
Text22
	dc.b	"NUMBER 3:",10
	dc.b	"Message requester, the requester",10
	dc.b	"you've been using all the time!",10
	dc.b	"function: rtEZRequestA()",0
Text23
	dc.b	"Show me more",0
Text24
	dc.b	"Simplest usage: some body text and",10
	dc.b	"a single centered gadget.",0
Text25
	dc.b	"Got it",0
Text26
	dc.b	"You can also use two gadgets to",10
	dc.b	"ask the user something.",10
	dc.b	"Do you understand?",0
Text27
	dc.b	"Of course|Not really",0
Text28
	dc.b	"You are not one of the brightest are you?",10
	dc.b	"We'll try again...",0
Text29
	dc.b	"Ok",0
Text30
	dc.b	"Great, we'll continue then.",0
Text31
	dc.b	"Fine",0
Text32
	dc.b	"You can also put up a requester with",10
	dc.b	"three choices.",10
	dc.b	"How do you like the demo so far ?",0
Text33
	dc.b	"Great|So so|Rubbish",0
Text34
	dc.b	"Too bad, I really hoped you",10
	dc.b	"would like it better.",0
Text35
	dc.b	"So what",0
Text36
	dc.b	"I'm glad you like it so much.",0
Text37
	dc.b	"Maybe if you run the demo again",10
	dc.b	"you'll REALLY like it.",0
Text38
	dc.b	"Perhaps",0
Text39
	dc.b	"The number of responses is not limited to three",10
	dc.b	"as you can see.  The gadgets are labeled with",10
	dc.b	"the return code from rtEZRequestA().",10
	dc.b	"Pressing Return will choose 4, note that",10
	dc.b	"4's button text is printed in boldface.",0
Text40
	dc.b	"1|2|3|4|5|0",0
Text41
	dc.b	"You picked '%ld'.",0
Text42
	dc.b	"How true",0
Text43
	dc.b	"New for Release 2.0 of ReqTools (V38) is",10
	dc.b	"the possibility to define characters in the",10
	dc.b	"buttons as keyboard shortcuts.",10
	dc.b	"As you can see these characters are underlined.",10
	dc.b	"Pressing shift while still holding down the key",10
	dc.b	"will cancel the shortcut.",10
	dc.b	"Note that in other requesters a string gadget may",10
	dc.b	"be active.  To use the keyboard shortcuts there",10
	dc.b	"you have to keep the Right Amiga key pressed down.",0
Text44
	dc.b	"_Great|_Fantastic|_Swell|Oh _Boy",0
Text45
	dc.b	"You may also use C-style formatting codes in the body text.",10
	dc.b	"Like this:",10,10
	dc.b	"'The number %%ld is written %%s.' will give:",10,10
	dc.b	"The number %ld is written %s.",10,10
	dc.b	"if you also pass '5' and '",'"five"',"' to rtEZRequestA().",0
Text46
	dc.b	"_Proceed",0
Text47
	dc.b	"It is also possible to pass extra IDCMP flags",10
	dc.b	"that will satisfy rtEZRequestA(). This requester",10
	dc.b	"has had DISKINSERTED passed to it.",10
	dc.b	"(Try inserting a disk).",0
Text48
	dc.b	"_Continue",0
Text49
	dc.b	"You inserted a disk.",0
Text50
	dc.b	"You used the 'Continue' gadget",10
	dc.b	"to satisfy the requester.",0
Text51
	dc.b	"I did",0
Text52
	dc.b	"Finally, it is possible to specify the position",10
	dc.b	"of the requester.",10
	dc.b	"E.g. at the top left of the screen, like this.",10
	dc.b	"This works for all requesters, not just rtEZRequestA()!",0
Text53
	dc.b	"_Amazing",0
Text54
	dc.b	"Alternatively, you can center the",10
	dc.b	"requester on the screen.",10
	dc.b	"Check out 'reqtools.doc' for all the possibilities.",0
Text55
	dc.b	"I'll do that",0
Text56
	dc.b	"NUMBER 4:",10
	dc.b	"function: rtFileRequestA()",0
Text57
	dc.b	"_Demonstrate",0
Text58
	dc.b	"Out of memory!",0
Text59
	dc.b	"Pick a file",0
Text60
	dc.b	"You picked the file:",10,"'%s'",10
	dc.b	"in directory:",10,"'%s'",0
Text61
	dc.b	"Right",0
Text62
	dc.b	"You didn't pick a file.",0
Text63
	dc.b	"No",0
Text64
	dc.b	"The file requester has the ability",10
	dc.b	"to allow you to pick more than one",10
	dc.b	"file (use Shift to extend-select).",10
	dc.b	"Note the extra gadgets you get.",0
Text65
	dc.b	"_Interesting",0
Text66
	dc.b	"Pick some files",0
Text67
	dc.b	"You selected some files, this is",10
	dc.b	"the first one:",10,"'%s'",10
	dc.b	"All the files are returned as a linked",10
	dc.b	"list (see demo.s and reqtools.i).",0
Text68
	dc.b	"Aha",0
Text69
	dc.b	"The file requester can be used",10
	dc.b	"as a directory requester as well.",0
Text70
	dc.b	"Let's _see that",0
Text71
	dc.b	"Pick a directory",0
Text72
	dc.b	"You picked the directory:",10,"'%s'",0
Text73
	dc.b	"Right",0
Text74
	dc.b	"You didn't pick a directory.",0
Text75
	dc.b	"NUMBER 5:",10
	dc.b	"Font requester",0
Text76
	dc.b	"Show",0
Text77
	dc.b	"Pick a font",0
Text78
	dc.b	"You picked the font:",10,"'%s'",10
	dc.b	"with size:",10,"'%ld'",0
Text79
	dc.b	"You canceled.",10
	dc.b	"Was there no font you liked ?",0
Text80
	dc.b	"_Nope",0
Text81
	dc.b	"NUMBER 6:",10
	dc.b	"Palette requester",10
	dc.b	"function: rtPaletteRequestA()",0
Text82
	dc.b	"Change palette",0
Text83
	dc.b	"You canceled.",10
	dc.b	"No nice colors to be picked ?",0
Text84
	dc.b	"Nah",0
Text85
	dc.b	"You picked color number %ld.",0
Text86
	dc.b	"Sure did",0
Text87
	dc.b	"NUMBER 7: (ReqTools 2.0)",10
	dc.b	"Volume requester",10
	dc.b	"function: rtFileRequestA() with",10
	dc.b	"          RTFI_VolumeRequest tag.",0
Text88
	dc.b	"_Show me",0
Text89
	dc.b	"Pick a volume",0
Text90
	dc.b	"You picked the volume:",10,"'%s'",0
Text91
	dc.b	"You didn't pick a volume.",0
Text92
	dc.b	"I did not",0
Text93
	dc.b	"NUMBER 8: (ReqTools 2.0)",10
	dc.b	"Screen mode requester",10
	dc.b	"function: rtScreenModeRequestA()",10
	dc.b	"Only available on Kickstart 2.0!",0
Text94
	dc.b	"Your Amiga doesn't seem to have",10
	dc.b	"Kickstart 2.0 in ROM so I am not",10
	dc.b	"able to show you the Screen mode",10
	dc.b	"requester.",10
	dc.b	"So upgrade to 2.0 *now* :-)",0
Text95
	dc.b	"_Allright",0
Text96
	dc.b	"Pick a screen mode:",0
Text97
	dc.b	"You picked this mode:",10
	dc.b	"ModeID  : 0x%lx",10
	dc.b	"Size    : %ld x %ld",10
	dc.b	"Depth   : %ld",10
	dc.b	"Overscan: %ld",10
	dc.b	"AutoScroll %s",0
Text98
	dc.b	"You didn't pick a screen mode.",0
Text99
	dc.b	"That's it!",10
	dc.b	"Hope you enjoyed the demo",0
Text100
	dc.b	"_Sure did",0
TagText1
	dc.b	" _Ok |New _2.0 feature!|_Cancel",0
TagText2
	dc.b	"These are two new features of ReqTools 2.0:",10
	dc.b	"Text above the entry gadget and more than",10
	dc.b	"one response gadget.",0
TagText3
	dc.b	" _Ok |_Abort|_Cancel",0
TagText4
	dc.b	"New is also the ability to switch off the",10
	dc.b	"backfill pattern.  You can also center the",10
	dc.b	"text above the entry gadget.",10
	dc.b	"These new features are also available in",10
	dc.b	"the rtGetLong() requester.",0

	cnop	0,2
Tags1
	dc.l	RTGS_GadFmt,TagText1
	dc.l	RTGS_TextFmt,TagText2
	dc.l	TAG_MORE,undertag
	dc.l	TAG_END
Tags2
	dc.l	RTGS_GadFmt,TagText3
	dc.l	RTGS_TextFmt,TagText4
	dc.l	RTGS_BackFill,FALSE
	dc.l	RTGS_Flags,GSREQF_CENTERTEXT+GSREQF_HIGHLIGHTTEXT
	dc.l	TAG_MORE,undertag
	dc.l	TAG_END
Tags3
	dc.l	RTGL_ShowDefault,FALSE
	dc.l	RTGL_Min,0
	dc.l	RTGL_Max,666
	dc.l	TAG_END
Tags4
	dc.l	RTEZ_DefaultResponse,4
	dc.l	TAG_END
Tags6
	dc.l	RT_IDCMPFlags,DISKINSERTED
Tags5
	dc.l	RT_Underscore,'_'
	dc.l	TAG_END
Tags7
	dc.l	RT_ReqPos,REQPOS_TOPLEFTSCR
	dc.l	RT_Underscore,'_'
	dc.l	TAG_END
Tags8
	dc.l	RT_ReqPos,REQPOS_CENTERSCR
	dc.l	TAG_END
Tags9
	dc.l	RTFI_Flags,FREQF_MULTISELECT
	dc.l	TAG_END
Tags10
	dc.l	RTFI_Flags,FREQF_NOFILES
	dc.l	TAG_END
Tags11
	dc.l	RTFI_VolumeRequest,0
	dc.l	TAG_END
Tags12
	dc.l	RTSC_Flags,SCREQF_DEPTHGAD+SCREQF_SIZEGADS+SCREQF_AUTOSCROLLGAD+SCREQF_OVERSCANGAD
	dc.l	TAG_END
undertag
	dc.l	RT_Underscore,'_'
DosLib
	dc.b	"dos.library",0
ReqName
	REQTOOLSNAME
	cnop	0,2
Null
	dc.l	0
DosBase
	ds.l	1
ReqBase
	ds.l	1
Buffer
	ds.b	128
Array
	ds.l	6
LongNum
	ds.l	1
filereq
	ds.l	1
flist
	ds.l	1
fontreq
	ds.l	1
scrmodereq
	ds.l	1
filename
	ds.b	34

	END
