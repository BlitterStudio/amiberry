; 68030 MMU fault-resume test, booted as a 512K Kickstart at $F80000 by
; tests/mmu030_fault_resume_test.py, which pads fault_resume.bin and fixes
; the Kickstart checksum.
;
; Rebuild fault_resume.bin after editing (vasm, e.g. from the
; amigadev/crosstools:m68k-amigaos-gcc10 image):
;   vasmm68k_mot -Fbin -m68030 -no-opt -o fault_resume.bin fault_resume.s
;
; Page $40000 starts out invalid. The bus error handler maps it and returns,
; so the faulting instruction is resumed from its format B frame.
;   case 1: user mode move.w (a7)+,d0 faults on the read.
;   case 2: user mode move.w -(a7),d0 faults on the read.
;   case 3: user mode jump to an odd address. The address error frame is
;           format B but has no address register side effect to undo.
; Results, one long each from $2000:
;   magic ('DONE', or 'FAIL' after an unexpected exception), case 1 USP,
;   case 1 SSP, case 2 USP, case 2 SSP, case 3 format word, case 3 frame
;   words at +$36 and +$34, bus error count, unexpected vector, initial SSP.

TABLEA	equ	$10000
TABLEB	equ	$10400
TABLEC	equ	$10800
TESTPG	equ	$40

RES	equ	$2000
RES_C1USP	equ	RES+$04
RES_C1SSP	equ	RES+$08
RES_C2USP	equ	RES+$0c
RES_C2SSP	equ	RES+$10
RES_C3FMT	equ	RES+$14
RES_C3W36	equ	RES+$18
RES_C3W34	equ	RES+$1c
RES_BERR	equ	RES+$20
RES_UNEXP	equ	RES+$24
RES_ISP	equ	RES+$28

	org	$f80000
	dc.l	$11144ef9
	dc.l	start

start:
	move.w	#$2700,sr
	move.b	#3,$bfe201
	move.b	#2,$bfe001		; overlay off: chip RAM at 0
	move.w	#$7fff,$dff09a
	move.w	#$7fff,$dff096
	move.w	#$7fff,$dff09c
	lea	$80000,sp

	lea	RES,a0
	moveq	#15,d0
.clr:	clr.l	(a0)+
	dbf	d0,.clr
	move.l	#$ffff,RES_UNEXP

	; vectors at 0
	moveq	#0,d0
	movec	d0,vbr
	sub.l	a0,a0
	move.w	#255,d0
	lea	unexp(pc),a1
.vec:	move.l	a1,(a0)+
	dbf	d0,.vec
	lea	buserr(pc),a1
	move.l	a1,2*4
	lea	adrerr(pc),a1
	move.l	a1,3*4
	lea	trap0(pc),a1
	move.l	a1,32*4

	; TC $80c08480: 4K pages, TIA 8, TIB 4, TIC 8, identity mapping
	; level A: 256 x 16M, entry 0 -> table B
	lea	TABLEA,a0
	move.l	#TABLEB+2,(a0)+
	move.l	#$01000001,d1
	move.w	#254,d0
.ta:	move.l	d1,(a0)+
	add.l	#$01000000,d1
	dbf	d0,.ta
	; level B: 16 x 1M, entry 0 -> table C
	lea	TABLEB,a0
	move.l	#TABLEC+2,(a0)+
	move.l	#$00100001,d1
	moveq	#14,d0
.tb:	move.l	d1,(a0)+
	add.l	#$00100000,d1
	dbf	d0,.tb
	; level C: 256 x 4K
	lea	TABLEC,a0
	moveq	#1,d1
	move.w	#255,d0
.tc:	move.l	d1,(a0)+
	add.l	#$1000,d1
	dbf	d0,.tc
	clr.l	TABLEC+TESTPG*4

	lea	crp_val(pc),a0
	pmove	(a0),crp
	lea	tc_val(pc),a0
	pmove	(a0),tc

	; case 1
	lea	c1_done(pc),a5
	move.l	#$40000,a0
	move.l	a0,usp
	move.l	sp,RES_ISP
	andi.w	#$dfff,sr
	move.w	(a7)+,d0
	move.l	a7,d7
	trap	#0
c1_done:
	move.l	d7,RES_C1USP
	move.l	sp,RES_C1SSP
	clr.l	TABLEC+TESTPG*4
	pflusha

	; case 2
	lea	c2_done(pc),a5
	move.l	#$41000,a0
	move.l	a0,usp
	andi.w	#$dfff,sr
	move.w	-(a7),d0
	move.l	a7,d7
	trap	#0
c2_done:
	move.l	d7,RES_C2USP
	move.l	sp,RES_C2SSP

	; case 3
	lea	c3_done(pc),a5
	move.l	#$30000,a0
	move.l	a0,usp
	andi.w	#$dfff,sr
	lea	$3001,a0
	jmp	(a0)
c3_done:

	move.l	#'DONE',RES
idle:	stop	#$2700
	bra.s	idle

buserr:
	move.l	#(TESTPG<<12)+1,TABLEC+TESTPG*4
	pflusha
	addq.l	#1,RES_BERR
	rte

adrerr:
	move.w	6(sp),RES_C3FMT+2
	move.w	$36(sp),RES_C3W36+2
	move.w	$34(sp),RES_C3W34+2
	lea	92(sp),sp
	jmp	(a5)

trap0:
	addq.l	#8,sp
	jmp	(a5)

unexp:
	move.w	6(sp),d0
	and.l	#$fff,d0
	lsr.l	#2,d0
	move.l	d0,RES_UNEXP
	move.l	#'FAIL',RES
	bra.s	idle

	cnop	0,4
crp_val:	dc.l	$7fff0002,TABLEA
tc_val:	dc.l	$80c08480
