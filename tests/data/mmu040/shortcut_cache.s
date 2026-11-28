; 68040/68060 MMU translation cache test, booted as a 512K Kickstart at
; $F80000 by tests/mmu040_shortcut_cache_test.py, which pads
; shortcut_cache.bin and fixes the Kickstart checksum.
;
; Rebuild shortcut_cache.bin after editing (vasm, e.g. from the
; amigadev/crosstools:m68k-amigaos image):
;   vasmm68k_mot -Fbin -m68040 -no-opt -o shortcut_cache.bin shortcut_cache.s
;
; Every case reads a logical page whose translation changes between reads
; (TTR, TC enable, page size, user/supervisor, instruction/data space), so a
; stale cached translation reads the wrong physical page. Caches are on
; (write-through) throughout.
;
; Page tables (root index 0 only, first 32M identity mapped):
;   TREE4I  4K pages, identity
;   TREE4R  4K pages, $40000->$50000, $42000->$52000
;   TREE8R  8K pages, $40000->$50000, $42000->$52000
; Physical markers: $40100 'IDEN', $50100 'REMP', $42100 'I42 ',
; $43100 'I43 ', $52100 'R52 ', $53100 'R53 '. $40000/$50000 hold
; "move.l #'IDEN'/'REMP',d0 ; rts".
;
;   case 1: DTT0 covers $40000 (identity), then is disabled, re-enabled
;           and moved away, with TREE4R active.
;   case 2: read with TC off, then enable TC (TREE4R) and read again, then
;           disable TC and read again. No PFLUSH in between.
;   case 3: 4K/8K page size switch (TREE4R <-> TREE8R).
;   case 4: SRP=TREE4I, URP=TREE4R: supervisor and user reads of the same
;           page; then both TREE4R with a supervisor-only and a user-only
;           DTT0.
;   case 5: trap through the vector page (VBR=0), then access page 0.
;   case 6: DTT0 covers $40000 but no ITT does: a data read is identity,
;           a code fetch goes through the page tables; then ITT0 covers it
;           too, then ITT0 is disabled again.
;   case 7: data writes while the translation changes: DTT0 on then off,
;           TC off then on, then a user write through a write-protected
;           user-only DTT0 after one through a writable one must take an
;           access fault. Read back with the MMU off.
; Results, one long each from $2000 (see RES_* below): magic ('DONE', or
; 'FAIL' after an unexpected exception), unexpected vector ($FFFF if none),
; then the value read by each step.

TREE4I	equ	$100000
TREE4R	equ	$110000
TREE8R	equ	$120000

RES	equ	$2000
RES_UNEXP	equ	RES+$04
RES_1A	equ	RES+$08
RES_1B	equ	RES+$0c
RES_1C	equ	RES+$10
RES_1D	equ	RES+$14
RES_2A	equ	RES+$18
RES_2B	equ	RES+$1c
RES_2C	equ	RES+$20
RES_3A	equ	RES+$24
RES_3B	equ	RES+$28
RES_3C	equ	RES+$2c
RES_3D	equ	RES+$30
RES_4A	equ	RES+$34
RES_4B	equ	RES+$38
RES_4C	equ	RES+$3c
RES_4D	equ	RES+$40
RES_4E	equ	RES+$44
RES_4F	equ	RES+$48
RES_4G	equ	RES+$4c
RES_5A	equ	RES+$50
RES_5B	equ	RES+$54
RES_5C	equ	RES+$58
RES_5X	equ	RES+$5c
RES_6A	equ	RES+$60
RES_6C	equ	RES+$64
RES_6D	equ	RES+$68
RES_6E	equ	RES+$6c
RES_7A	equ	RES+$70
RES_7B	equ	RES+$74
RES_7C	equ	RES+$78
RES_7D	equ	RES+$7c
RES_7E	equ	RES+$80
RES_7G	equ	RES+$84
RES_END	equ	RES+$88

TC4K	equ	$8000
TC8K	equ	$c000
TT_ANY	equ	$0000c000		; base 0, mask 0, enabled, ignore FC2
TT_SUP	equ	$0000a000		; supervisor only
TT_USR	equ	$00008000		; user only
TT_MOVED	equ	$0100c000		; $01xxxxxx only

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

	moveq	#0,d0
	movec	d0,cacr
	movec	d0,tc
	movec	d0,itt0
	movec	d0,itt1
	movec	d0,dtt0
	movec	d0,dtt1
	cinva	bc
	pflusha

	lea	RES,a0
	moveq	#(RES_END-RES)/4-1,d0
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
	lea	trap0(pc),a1
	move.l	a1,32*4
	lea	accfault(pc),a1
	move.l	a1,2*4

	; markers and code snippets
	move.l	#'IDEN',$40100
	move.l	#'REMP',$50100
	move.l	#'I42 ',$42100
	move.l	#'I43 ',$43100
	move.l	#'R52 ',$52100
	move.l	#'R53 ',$53100
	move.l	#$203c4944,$40000	; move.l #'IDEN',d0
	move.l	#$454e4e75,$40004	; rts
	move.l	#$203c5245,$50000	; move.l #'REMP',d0
	move.l	#$4d504e75,$50004	; rts

	lea	TREE4I,a0
	bsr	build4k
	lea	TREE4R,a0
	bsr	build4k
	move.l	#$50001,TREE4R+$400+($40000>>12)*4
	move.l	#$52001,TREE4R+$400+($42000>>12)*4
	lea	TREE8R,a0
	move.l	#128,d5			; page table size
	move.l	#$2000,d3		; page size
	move.w	#128*32-1,d4
	bsr	build
	move.l	#$50001,TREE8R+$400+($40000>>13)*4
	move.l	#$52001,TREE8R+$400+($42000>>13)*4

	move.l	#$80008000,d0		; data and instruction caches on
	movec	d0,cacr

	lea	$40100,a1

	; case 2: MMU off, then on, then off
	move.l	#TREE4R,d0
	movec	d0,urp
	movec	d0,srp
	pflusha
	move.l	(a1),RES_2A
	move.l	#TC4K,d0
	movec	d0,tc
	move.l	(a1),RES_2B
	moveq	#0,d0
	movec	d0,tc
	move.l	(a1),RES_2C

	; case 1: DTT0 on, off, on, moved
	move.l	#TC4K,d0
	movec	d0,tc
	pflusha
	move.l	#TT_ANY,d0
	movec	d0,dtt0
	move.l	(a1),RES_1A
	moveq	#0,d0
	movec	d0,dtt0
	move.l	(a1),RES_1B
	move.l	#TT_ANY,d0
	movec	d0,dtt0
	move.l	(a1),RES_1C
	move.l	#TT_MOVED,d0
	movec	d0,dtt0
	move.l	(a1),RES_1D
	moveq	#0,d0
	movec	d0,dtt0

	; case 3: 4K -> 8K -> 4K
	move.l	$43100,RES_3A
	moveq	#0,d0
	movec	d0,tc
	move.l	#TREE8R,d0
	movec	d0,urp
	movec	d0,srp
	move.l	#TC8K,d0
	movec	d0,tc
	pflusha
	move.l	$42100,RES_3B
	move.l	$43100,RES_3C
	moveq	#0,d0
	movec	d0,tc
	move.l	#TREE4R,d0
	movec	d0,urp
	movec	d0,srp
	move.l	#TC4K,d0
	movec	d0,tc
	pflusha
	move.l	$43100,RES_3D

	; case 4: user/supervisor on the same page
	move.l	#TREE4I,d0
	movec	d0,srp
	pflusha
	lea	$70000,a0
	move.l	a0,usp
	move.l	(a1),RES_4A
	lea	.u1(pc),a5
	andi.w	#$dfff,sr
	move.l	(a1),d7
	trap	#0
.u1:	move.l	d7,RES_4B
	move.l	(a1),RES_4C

	move.l	#TREE4R,d0
	movec	d0,srp
	pflusha
	move.l	#TT_SUP,d0
	movec	d0,dtt0
	move.l	(a1),RES_4D
	lea	.u2(pc),a5
	andi.w	#$dfff,sr
	move.l	(a1),d7
	trap	#0
.u2:	move.l	d7,RES_4E
	move.l	#TT_USR,d0
	movec	d0,dtt0
	move.l	(a1),RES_4F
	lea	.u3(pc),a5
	andi.w	#$dfff,sr
	move.l	(a1),d7
	trap	#0
.u3:	move.l	d7,RES_4G
	moveq	#0,d0
	movec	d0,dtt0

	; case 5: exception through the vector page, then access page 0
	lea	$3f0,a2
	move.l	#'VEC1',(a2)
	move.l	(a2),RES_5A
	lea	.t5(pc),a5
	trap	#0
.t5:	move.l	#'VEC2',(a2)
	move.l	(a2),RES_5B
	move.l	32*4,RES_5C
	lea	trap0(pc),a0
	move.l	a0,RES_5X

	; case 6: DTT0 but no ITT covering $40000
	move.l	#TT_ANY,d0
	movec	d0,dtt0
	move.l	(a1),RES_6A
	moveq	#0,d0
	jsr	$40000
	move.l	d0,RES_6C
	move.l	#TT_ANY,d0
	movec	d0,itt0
	moveq	#0,d0
	jsr	$40000
	move.l	d0,RES_6D
	moveq	#0,d0
	movec	d0,itt0
	jsr	$40000
	move.l	d0,RES_6E
	moveq	#0,d0
	movec	d0,dtt0

	; case 7: writes while the translation changes
	move.l	#TREE4R,d0
	movec	d0,urp
	movec	d0,srp
	move.l	#TC4K,d0
	movec	d0,tc
	pflusha
	move.l	#TT_ANY,d0
	movec	d0,dtt0
	move.l	#'W7A ',$40200		; DTT0: untranslated
	moveq	#0,d0
	movec	d0,dtt0
	move.l	#'W7B ',$40200		; page tables: $50200
	movec	d0,tc
	move.l	#'W7C ',$40204		; MMU off: untranslated
	move.l	#TC4K,d0
	movec	d0,tc
	move.l	#'W7D ',$40204		; page tables: $50204
	lea	$70000,a0
	move.l	a0,usp
	move.l	#TT_USR,d0
	movec	d0,dtt0
	lea	.w7a(pc),a5
	andi.w	#$dfff,sr
	move.l	#'W7E ',$40208		; user write, writable DTT0
	trap	#0
.w7a:	move.l	#TT_USR|4,d0
	movec	d0,dtt0
	lea	.w7(pc),a5
	move.l	sp,a6
	andi.w	#$dfff,sr
	move.l	#'W7F ',$40208		; user write, write-protected DTT0: access fault
	trap	#0
.w7:	moveq	#0,d0
	movec	d0,dtt0
	movec	d0,tc
	move.l	$40200,RES_7A
	move.l	$50200,RES_7B
	move.l	$40204,RES_7C
	move.l	$50204,RES_7D
	move.l	$40208,RES_7E

	moveq	#0,d0
	movec	d0,tc
	move.l	#'DONE',RES
idle:	stop	#$2700
	bra.s	idle

; a0 = tree base: root table at +0, pointer table at +$200, page tables
; from +$400. build takes d3 = page size, d4 = page descriptors - 1,
; d5 = page table size.
build4k:
	move.l	#256,d5
	move.l	#$1000,d3
	move.w	#128*64-1,d4
build:
	move.l	a0,a1
	lea	$200(a0),a2
	move.l	a2,d0
	or.w	#2,d0
	move.l	d0,(a1)+
	moveq	#126,d1
.r:	clr.l	(a1)+
	dbf	d1,.r
	lea	$400(a0),a2
	move.l	a2,d0
	or.w	#2,d0
	moveq	#127,d1
.p:	move.l	d0,(a1)+
	add.l	d5,d0
	dbf	d1,.p
	moveq	#1,d0
.pg:	move.l	d0,(a2)+
	add.l	d3,d0
	dbf	d4,.pg
	rts

; return to a5 in supervisor mode, whatever the frame format
trap0:
	move.w	#$2700,(sp)
	move.l	a5,2(sp)
	rte

; access fault in case 7: drop the frame and continue at a5 (supervisor)
accfault:
	move.l	a6,sp
	move.l	#'FALT',RES_7G
	jmp	(a5)

unexp:
	move.w	6(sp),d0
	and.l	#$fff,d0
	lsr.l	#2,d0
	move.l	d0,RES_UNEXP
	move.l	#'FAIL',RES
	bra.w	idle
