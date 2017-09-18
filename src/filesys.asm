	SECTION code,code
; This file can be translated with A68k or PhxAss and then linked with BLink
; to produce an Amiga executable. Make sure it does not contain any
; relocations, then run it through the filesys.sh script
;
; Patrick: It also works with SAS/C asm+slink, but I had to remove
; the .l from jsr.l and jmp.l.
; Toni Wilen: modified SECTION, compiles now with AsmOne and clones(?)
; Removed absolute RT_AREA references
; 2002.08.06 RDB automount/autoboot support (Toni Wilen)
; 2002.09.11 KS1.3 RDB support (TW)
; 200?.??.?? Picasso96 vblank hack (TW)
; 2006.03.04 Mousehack code integrated (TW)
; 2006.18.07 FileSystem.resource find routine access fault fixed (TW)
; 2007.03.30 mousehack do not start multiple times anymore (TW)
; 2007.06.15 uninitialized variable in memory type selection fixed (stupid me) (TW)
; 2007.08.09 started implementing removable drive support (TW)
; 2007.09.01 ACTION_EXAMINE_ALL (TW)
; 2007.09.05 full filesystem device mounting on the fly (TW)
; 2008.01.09 ACTION_EXAMINE_ALL does not anymore return eac_Entries = 0 with continue (fixes some broken programs)
; 2008.12.11 mousehack -> tablet driver
; 2008.12.25 mousehack cursor sync
; 2009.01.20 clipboard sharing
; 2009.12.27 console hook
; 2010.05.27 Z3Chip
; 2011.12.17 built-in CDFS support
; 2015.09.27 KS 1.2 boot hack supported
; 2015.09.28 KS 1.2 boot hack improved, 1.1 and older BCPL-only DOS support.
; 2016.01.14 'Indirect' boot ROM trap support.

AllocMem = -198
FreeMem = -210

TRAP_DATA_NUM = 4
TRAP_DATA_SEND_NUM = 1

TRAP_DATA = $4000
TRAP_DATA_SIZE = $8000
TRAP_DATA_SLOT_SIZE = 8192
TRAP_DATA_SECOND = 80
TRAP_DATA_TASKWAIT = (TRAP_DATA_SECOND-4)
TRAP_DATA_EXTRA = 144

TRAP_STATUS = $F000
TRAP_STATUS_SLOT_SIZE = 8
TRAP_STATUS_SECOND = 4

TRAP_STATUS_LOCK_WORD = 2
TRAP_STATUS_STATUS2 = 2
TRAP_STATUS_STATUS = 3

TRAP_DATA_DATA = 4

RTAREA_SYSBASE = $3FFC
RTAREA_GFXBASE = $3FF8
RTAREA_INTBASE = $3FF4
RTAREA_INTXY = $3FF0

RTAREA_MOUSEHACK = $3E00

RTAREA_TRAPTASK = $FFF4
RTAREA_EXTERTASK = $FFF8
RTAREA_INTREQ = $FFFC

; don't forget filesys.c! */
PP_MAXSIZE = 4 * 96
PP_FSSIZE = 400
PP_FSPTR = 404
PP_ADDTOFSRES = 408
PP_FSRES = 412
PP_FSRES_CREATED = 416
PP_DEVICEPROC = 420
PP_EXPLIB = 424
PP_FSHDSTART = 428
PP_TOTAL = (PP_FSHDSTART+140)

NOTIFY_CLASS = $40000000
NOTIFY_CODE = $1234
NRF_SEND_MESSAGE = 1
NRF_SEND_SIGNAL = 2
NRF_WAIT_REPLY = 8
NRF_NOTIFY_INITIAL = 16
NRF_MAGIC = $80000000

; normal filehandler segment entrypoint
	dc.l 16 								; 4
our_seglist:
	dc.l 0 									; 8 /* NextSeg */
start:
	bra.s startjmp
	dc.w 11						;0 12
startjmp:
	bra.w filesys_mainloop		;1 16
	dc.l make_dev-start			;2 20
	dc.l filesys_init-start		;3 24
	dc.l moverom-start				;  4 28
	dc.l bootcode-start			;5 32
	dc.l setup_exter-start		;6 36
	dc.l bcplwrapper-start ;7 40
	dc.l afterdos-start 	;8 44
	dc.l hwtrap_install-start ;  9 48
	dc.l hwtrap_entry-start 	; 10 52

bootcode:
	lea.l doslibname(pc),a1
	jsr -96(a6) ; FindResident
	move.l d0,a0
	move.l 22(a0),d0
	move.l d0,a0
	jsr (a0)
	rts

; BCPL filehandler segment entry point
; for KS 1.1 and older.
	cnop 0,4
	dc.l ((bcpl_end-bcpl_start)>>2)+1
our_bcpl_seglist:
	dc.l 0
	dc.l ((bcpl_end-bcpl_start)>>2)+1
bcpl_start:
	; d1 = startup packet
	lsl.l #2,d1
	move.l d1,d7
	bra.w filesys_mainloop_bcpl
	cnop 0,4
	dc.l 0
	dc.l 1
	dc.l 4
	dc.l 2
bcpl_end:

afterdos:
	movem.l d2-d7/a2-a6,-(sp)

	move.l 4.w,a6
	lea gfxlibname(pc),a1
	moveq #0,d0
	jsr -$0228(a6) ;OpenLibrary
	move.l d0,d1
	move.w #RTAREA_GFXBASE,d0
	bsr.w getrtbase
	move.l d1,(a0)
	lea intlibname(pc),a1
	moveq #0,d0
	jsr -$0228(a6) ;OpenLibrary
	move.l d0,d1
	move.w #RTAREA_INTBASE,d0
	bsr.w getrtbase
	move.l d1,(a0)
	
	movem.l (sp)+,d2-d7/a2-a6
	moveq #0,d0
	rts

filesys_init:
	movem.l d0-d7/a0-a6,-(sp)
	move.l 4.w,a6
	move.w #$FFEC,d0 ; filesys base
	bsr getrtbase
	move.l (a0),a5
	moveq #0,d5
	moveq #0,d0
	cmp.w #33,20(a6) ; 1.1 or older?
	bcs.s FSIN_explibok
	lea.l explibname(pc),a1 ; expansion lib name
	moveq #36,d0
	moveq #1,d5
	jsr  -552(a6) ; OpenLibrary
	tst.l d0
	bne.b FSIN_explibok
	lea.l explibname(pc),a1 ; expansion lib name
	moveq #0,d0
	moveq #0,d5
	jsr  -552(a6) ; OpenLibrary
FSIN_explibok:
	move.l d0,a4

	REM ; moved to early hwtrap_install
	; create fake configdev
	exg a4,a6
	move.l a6,d0
	beq.s .nocd
	btst #4,$110+3(a5)
	bne.s .nocd
	jsr -$030(a6) ;expansion/AllocConfigDev
	tst.l d0
	beq.s .nocd
	move.l d0,a0
	lea start(pc),a1
	move.l a1,d0
	clr.w d0
	move.l d0,32(a0)
	move.l #65536,36(a0)
	move.w #$0104,16(a0) ;type + product
	move.w #2011,16+4(a0) ;manufacturer
	moveq #1,d0
	move.l d0,22(a0) ;serial
	jsr -$01e(a6) ;expansion/AddConfigDev
.nocd
	exg a4,a6
	EREM

	tst.l $10c(a5)
	beq.w FSIN_none

	move.l #PP_TOTAL,d0
	move.l #$10001,d1
	jsr AllocMem(a6)
	move.l d0,a3  ; param packet
	move.l a4,PP_EXPLIB(a3)

	moveq #0,d6
FSIN_init_units:
	cmp.w $10e(a5),d6
	bcc.b FSIN_units_ok
	move.l d6,-(sp)
FSIN_nextsub:
	move.l $110(a5),d7
	tst.w d5
	beq.s .oldks
	bset #2,d7
.oldks	move.l a3,-(sp)
	move.l a3,a0
	bsr.w make_dev
	move.l (sp)+,a3
	move.l d1,PP_DEVICEPROC(a3)
	cmp.l #-2,d0
	beq.s FSIN_nomoresub
	swap d6
	addq.w #1,d6
	swap d6
	bra.s FSIN_nextsub
FSIN_nomoresub:	
	move.l (sp)+,d6
	addq.w #1,d6
	bra.b FSIN_init_units
FSIN_units_ok:

	move.l 4.w,a6
	move.l a3,a1
	move.l #PP_TOTAL,d0
	jsr FreeMem(a6)

FSIN_none:
	move.l 4.w,a6
	move.l a4,d0
	beq.s .noexpclose
	move.l a4,a1
	jsr -414(a6) ; CloseLibrary
.noexpclose

	cmp.w #34,20(a6) ; 1.2 or older?
	bcs.w FSIN_tooold

	; add MegaChipRAM
	moveq #3,d4 ; MEMF_CHIP | MEMF_PUBLIC
	cmp.w #36,20(a6)
	bcs.s FSIN_ksold
	or.w #256,d4 ; MEMF_LOCAL
FSIN_ksold
	move.w #$FF80,d0
	bsr.w getrtbase
	jsr (a0) ; d1 = size, a1 = start address
	move.l d0,d5
	move.l a1,a0
	move.l d1,d0
	beq.s FSIN_fchip_done
	move.l d4,d1
	moveq #-5,d2
	lea fchipname(pc),a1
	jsr -618(a6) ; AddMemList
FSIN_fchip_done

	; only if >=4M chip
	cmp.l #$400000,d5
	bcs.s FSIN_locmem
	cmp.l 62(a6),d5
	beq.s FSIN_locmem
	jsr -$78(a6)
	move.l d5,62(a6) ;LocMem
	moveq #0,d0
	moveq #(82-34)/2-1,d1
	lea 34(a6),a0
.FSIN_locmem1
	add.w (a0)+,d0
	dbf d1,.FSIN_locmem1
	not.w d0
	move.w d0,82(a6) ;ChkSum
	jsr -$7e(a6)
FSIN_locmem

	; add >2MB-6MB chip RAM to memory list (if not already done)
	lea $210000,a1
	; do not add if RAM detected already
	jsr -$216(a6) ; TypeOfMem
	tst.l d0
	bne.s FSIN_chip_done
	move.l d4,d1
	moveq #-10,d2
	move.l #$200000,a0
	move.l d5,d0
	sub.l a0,d0
	bcs.b FSIN_chip_done
	beq.b FSIN_chip_done
	sub.l a1,a1
	jsr -618(a6) ; AddMemList
FSIN_chip_done

	lea fstaskname(pc),a0
	lea fsmounttask(pc),a1
	moveq #10,d0
	bsr createtask
	move.l d0,a1
	moveq #1,d1
	move.w #$FF48,d0 ; store task pointer
	bsr.w getrtbase
	jsr (a0)

FSIN_tooold

	movem.l (sp)+,d0-d7/a0-a6
general_ret:
	rts

	REM
addextrachip:
	move.w #$FF80,d0
	bsr.w getrtbase
	jsr (a0)
	jsr -$0078(a6) ; Disable
	lea 322(a6),a0 ; MemHeader
FSIN_scanchip:
	move.l (a0),a0	; first MemList
	tst.l (a0)
	beq.w FSIN_scannotfound
	move.l 20(a0),d1 ; mh_Lower
	clr.w d1
	tst.l d1
	bne.s FSIN_scanchip	
	move.w 14(a0),d1 ; attributes
	bmi.s FSIN_scanchip
	and #2,d1 ; MEMF_CHIP?
	beq.s FSIN_scanchip
	sub.l 24(a0),d0 ; did KS detect all chip?
	bmi.s FSIN_scandone
	beq.s FSIN_scandone
	; add the missing piece
	move.l 24(a0),d1
	add.l d0,24(a0) ; mh_Upper
	add.l d0,28(a0) ; mh_Free
	add.l d0,62(a6) ; MaxLocMem
	; we still need to update last node's free space
	move.l 16(a0),a0 ; mh_First
FSIN_chiploop2
	tst.l (a0)
	beq.s FSIN_chiploop
	move.l (a0),a0
	bra.s FSIN_chiploop2
FSIN_chiploop:
	move.l a0,d2
	add.l 4(a0),d2
	;Last block goes to end of chip?
	cmp.l d2,d1
	beq.s FSIN_chipadd1
	;It didn't, add new MemChunk
	move.l d1,(a0)
	move.l d1,a1
	clr.l (a1)+
	move.l d0,(a1)
	bra.s FSIN_scandone
FSIN_chipadd1:
	add.l d0,4(a0)
FSIN_scandone:
	jsr -$007e(a6) ; Enable
	rts

FSIN_scannotfound:
	moveq #3,d4 ; MEMF_CHIP | MEMF_PUBLIC
	cmp.w #36,20(a6)
	bcs.s FSIN_ksold
	or.w #256,d4 ; MEMF_LOCAL
FSIN_ksold
	; add >2MB-6MB chip RAM to memory list
	lea $210000,a1
	; do not add if RAM detected already
	jsr -$216(a6) ; TypeOfMem
	tst.l d0
	bne.s FSIN_chip_done
	move.w #$FF80,d0
	bsr.w getrtbase
	jsr (a0)
	move.l d4,d1
	moveq #-10,d2
	move.l #$200000,a0
	sub.l a0,d0
	bcs.b FSIN_chip_done
	beq.b FSIN_chip_done
	sub.l a1,a1
	jsr -618(a6) ; AddMemList
FSIN_chip_done
	rts
	EREM

createproc
	movem.l d2-d5/a2/a6,-(sp)
	moveq #0,d5
	move.l 4.w,a6
	move.l d0,d2
	move.l d1,d4
	move.l a1,d3
	move.l a0,a2
	lea doslibname(pc),a1
	moveq #0,d0
	jsr -$0228(a6) ; OpenLibrary
	tst.l d0
	beq.s .noproc
	move.l d0,a6
	move.l a2,d1
	lsr.l #2,d3
	jsr -$08a(a6) ; CreateProc
	move.l d0,d5
	move.l a6,a1
	move.l 4.w,a6
	jsr -$019e(a6); CloseLibrary
.noproc
	move.l d5,d0
	movem.l (sp)+,d2-d5/a2/a6
	rts

	; this is getting ridiculous but I don't see any other solutions..
fsmounttask
.fsm1	move.l 4.w,a6
	moveq #0,d0
	bset #13,d0 ; SIGBREAK_CTRL_D
	jsr -$013e(a6) ;Wait
	lea fsprocname(pc),a0
	lea mountproc(pc),a1
	moveq #15,d0
	move.l #8000,d1
	bsr.w createproc
	bra.s .fsm1	

	; dummy process here because can't mount devices with ADNF_STARTPROC from task..
	; (AddDosNode() internally calls GetDeviceProc() which accesses ExecBase->ThisTask->pr_GlobVec)
	cnop 0,4
	dc.l 16
mountproc
	dc.l 0
	moveq #2,d1
	move.w #$FF48,d0 ; get new unit number
	bsr.w getrtbaselocal
	jsr (a0)
	move.l d0,d1
	bmi.s .out
	bsr.w addfsonthefly
.out	moveq #0,d0
	rts

trap_task:
	move.l 4.w,a6

trap_task_wait
	move.l #$100,d0
	jsr -$13e(a6) ;Wait

trap_task_check:
	moveq #0,d7
	; check if we have call lib/func request
	move.l #TRAP_STATUS,d0
	bsr.w getrtbase
	move.l a0,a1
	move.l #TRAP_DATA,d0
	bsr.w getrtbase
	moveq #TRAP_DATA_NUM-1,d6
.nexttrap
	tst.b TRAP_STATUS_STATUS(a1)
	beq.s .next
	cmp.b #$fd,TRAP_STATUS_SECOND+TRAP_STATUS_STATUS(a1)
	bne.s .next
	addq.l #1,d7
	lea TRAP_DATA_SECOND+4(a0),a4
	lea TRAP_STATUS_SECOND(a1),a5
	movem.l d6/d7/a0/a1/a4/a5/a6,-(sp)
	move.w (a5),d4 ;command
	movem.l (a4),a0-a2
	movem.l (a4),d0-d2
	cmp.w #18,d4
	bne.s .notcalllib
	bsr.w hw_call_lib
	bra.s .calldone
.notcalllib
	cmp.w #19,d4
	bne.s .calldone
	bsr.w hw_call_func
.calldone
	movem.l (sp)+,d6/d7/a0-a1/a4/a5/a6
	move.l d0,(a4)
	move.b #2,TRAP_STATUS_STATUS(a5)
.next
	add.w #TRAP_DATA_SLOT_SIZE,a0
	add.w #TRAP_STATUS_SLOT_SIZE,a1
	dbf d6,.nexttrap
	tst.l d7
	beq.w trap_task_wait
	bra.w trap_task_check

exter_task:
	move.l 4.w,a6

exter_task_wait
	move.l #$100,d0
	jsr -$13e(a6) ;Wait
	bsr.s exter_do
	bra.s exter_task_wait

exter_done
	rts
exter_do
	moveq #10,d7
EXTT_loop
	move.w #$FF50,d0 ; exter_int_helper
	bsr.w getrtbaselocal
	move.l d7,d0
	jsr (a0)
	tst.l d0
	beq.s exter_done
	moveq #11,d7
	cmp.w #1,d0
	blt.w EXTT_loop
	bgt.b EXTT_signal_reply
	jsr -366(a6) ; PutMsg
	bra.b EXTT_loop
EXTT_signal_reply:
	cmp.w #2,d0
	bgt.b EXTT_reply
	move.l d1,d0
	jsr -$144(a6)	; Signal
	bra.b EXTT_loop
EXTT_reply:
	cmp.w #3,d0
	bgt.b EXTT_cause
	jsr -$17a(a6)   ; ReplyMsg
	bra.b EXTT_loop
EXTT_cause:
	cmp.w #4,d0
	bgt.b EXTT_notificationhack
	jsr -$b4(a6)	; Cause
	bra.b EXTT_loop
EXTT_notificationhack:
	cmp.w #5,d0
	bgt.b EXTT_loop
	movem.l a0-a1,-(sp)
	moveq #38,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	movem.l (sp)+,a0-a1
	move.l d0,a2
	move.b #8,8(a2)
	move.l a0,14(a2)
	move.w #38,18(a2)
	move.l #NOTIFY_CLASS,20(a2)
	move.w #NOTIFY_CODE,24(a2)
	move.l a1,26(a2)
	move.l 16(a1),a0
	move.l a2,a1
	jsr -366(a6) ; PutMsg
	bra.w EXTT_loop

exter_server_new:
	moveq #0,d0
	move.l (a1)+,a0 ;IO Base
	tst.b (a0)
	beq.s .nouaeint
	move.l (a1)+,a6 ; SysBase
	
	move.l (a1),a1 ; Task
	move.l #$100,d0 ; SIGF_DOS
	jsr -$144(a6) ; Signal

	moveq #1,d0
.nouaeint
	tst.w d0
	rts

	cnop 0,4

	; d0 = exter task, d1 = trap task
heartbeatvblank:
	movem.l d0-d3/a0-a4,-(sp)
	move.l d0,d2
	move.l d1,d3

	move.w #$FF38,d0
	moveq #18,d1
	bsr.w getrtbaselocal
	move.l d2,d0
	move.l d3,d2
	jsr (a0)
	move.l d0,a2 ; intreq

	moveq #22+5*4,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	move.l d0,a4

	lea 22(a4),a3
	move.l a3,a1
	move.l a2,(a1)+
	move.l d2,(a1)+
	move.l d3,(a1)+
	move.w #RTAREA_INTBASE,d0
	bsr.w getrtbase
	move.l a0,(a1)+
	move.w #RTAREA_INTXY,d0
	bsr.w getrtbase
	move.l a0,(a1)+
	
	move.l a3,14(a4)

	move.b #2,8(a4) ;NT_INTERRUPT
	move.b #-10,9(a4) ;priority
	lea kaname(pc),a0
	move.l a0,10(a4)
	lea kaint(pc),a0
	move.l a0,18(a4)
	move.l a4,a1
	moveq #5,d0 ;INTB_VERTB
	jsr -$00a8(a6)

	movem.l (sp)+,d0-d3/a0-a4
	rts

kaint:
	move.l (a1),a0
	addq.l #1,(a0)
	move.l 3*4(a1),a0 ;RTAREA_INTBASE
	move.l (a0),d0
	beq.s .noint
	move.l d0,a0
	cmp.w #31,20(a0) ;version < 31
	bcs.s .noint
	tst.l 34(a0) ;ViewPort == NULL
	beq.s .noint
	tst.l 60(a0) ;FirstScreen == NULL
	beq.s .noint
	move.l 4*4(a1),a1
	move.l 68(a0),(a1) ;Y.W X.W
.noint
	moveq #0,d0
	rts

setup_exter:
	movem.l d0-d3/d7/a0-a2,-(sp)
	move.l d0,d7
	move.l #RTAREA_INTREQ,d0
	bsr.w getrtbase
	move.l a0,a2

	moveq #0,d2
	btst #0,d7
	beq.s .nofstask
	lea fswtaskname(pc),a0
	lea exter_task(pc),a1
	moveq #20,d0
	bsr createtask
	move.l d0,d2
.nofstask

	moveq #0,d3
	btst #1,d7
	beq.s .notraptask
	lea fstraptaskname(pc),a0
	lea trap_task(pc),a1
	moveq #25,d0
	bsr createtask
	move.l d0,d3
.notraptask

	moveq #26+4*4,d0
	move.l #$10001,d1
	jsr AllocMem(a6)
	move.l d0,a1

	lea 26(a1),a0
	move.l a2,(a0)+
	move.l a6,(a0)+
	move.l d2,(a0)+
	move.l d3,(a0)

	lea.l exter_name(pc),a0
	move.l a0,10(a1)
	lea 26(a1),a2
	move.l a2,14(a1)
	lea.l exter_server_new(pc),a0
	move.l a0,18(a1)
	move.w #$0214,8(a1)
	moveq #3,d0
	jsr -168(a6) ; AddIntServer

	move.l d2,d0 ; extertask
	move.l d3,d1 ; traptask
	bsr.w heartbeatvblank

	move.w #$FF38,d0
	moveq #4,d1
	bsr.w getrtbaselocal
	jsr (a0)
	tst.l d0
	beq.s .nomh
	bsr.w mousehack_init
.nomh
	movem.l (sp)+,d0-d3/d7/a0-a2
	rts

addfs: ; a0 = first hunk, a1 = parmpacket
	movem.l d0-d1/a0-a3/a6,-(sp)
	move.l 4.w,a6
	move.l a0,a2
	move.l a1,a3
	move.l #62+128,d0 ; sizeof(FileSysEntry) + patchflags;
	move.l #$10001,d1
	jsr -198(a6)
	move.l d0,a0
	moveq #0,d0
	lea PP_FSHDSTART(a3),a1
af1	move.b 0(a1,d0.w),14(a0,d0.w)
	addq.w #1,d0
	cmp.w #140,d0
	bne.s af1
	move.l a2,d0
	lsr.l #2,d0
	move.l d0,54(a0) ;seglist
	move.l a0,a1
	lea exter_name(pc),a0
	move.l a0,10(a1) ; name
	move.l PP_FSRES(a3),a0 ; FileSystem.resource
	lea 18(a0),a0 ; fsr_FileSysEntries
	jsr -$f0(a6) ;AddHead
	movem.l (sp)+,d0-d1/a0-a3/a6
	rts

relocate: ;a0=pointer to executable, returns first segment in A0
	movem.l d1-d7/a1-a6,-(sp)
	move.l 4.w,a6
	move.l a0,a2
	cmp.l #$3f3,(a2)+
	bne.w ree
	addq.l #8,a2 ; skip name end and skip total hunks + 1
	move.l 4(a2),d7 ; last hunk
	sub.l (a2),d7 ; first hunk
	addq.l #8,a2 ; skip hunk to load first and hunk to load last
	addq.l #1,d7
	move.l a2,a3
	move.l d7,d0
	add.l d0,d0
	add.l d0,d0
	add.l d0,a3
	move.l a2,a4

	; allocate hunks
	sub.l a5,a5 ;prev segment
	moveq #0,d6
r15	move.l (a2),d2 ; hunk size (header)
	moveq #1,d1
	btst #30,d2 ; chip mem?
	beq.s r2
	bset #1,d1
r2	bset #16,d1
	lsl.l #2,d2
	bne.s r17
	clr.l (a2)+ ; empty hunk
	bra.s r18
r17	addq.l #8,d2 ; size + pointer to next hunk + hunk size
	move.l d2,d0
	jsr AllocMem(a6)
	tst.l d0
	beq.w ree
	move.l d0,a0
	move.l d2,(a0)+ ; store size
	move.l a0,(a2)+ ; store new pointer
	move.l a5,d1
	beq.s r10
	move.l a0,d0
	lsr.l #2,d0
	move.l d0,(a5)
r10	move.l a0,a5
r18	addq.l #1,d6
	cmp.l d6,d7
	bne.s r15

	moveq #0,d6
r3	move.l d6,d1
	add.l d1,d1
	add.l d1,d1
	move.l 0(a4,d1.l),a0
	addq.l #4,a0
	move.l (a3)+,d3 ; hunk type
	move.l (a3)+,d4 ; hunk size
	lsl.l #2,d4
	cmp.l #$3e9,d3 ;code
	beq.s r4
	cmp.l #$3ea,d3 ;data
	bne.s r5
r4
	; code and data
	move.l d4,d0
r6	tst.l d0
	beq.s r7
	move.b (a3)+,(a0)+
	subq.l #1,d0
	bra.s r6
r5
	cmp.l #$3eb,d3 ;bss
	bne.s ree

r7 ; scan for reloc32 or hunk_end
	move.l (a3)+,d3
	cmp.l #$3ec,d3 ;reloc32
	bne.s r13

	; relocate
	move.l d6,d1
	add.l d1,d1
	add.l d1,d1
	move.l 0(a4,d1.l),a0 ; current hunk
	addq.l #4,a0
r11	move.l (a3)+,d0 ;number of relocs
	beq.s r7
	move.l (a3)+,d1 ;hunk
	add.l d1,d1
	add.l d1,d1
	move.l 0(a4,d1.l),d3 ;hunk start address
	addq.l #4,d3
r9	move.l (a3)+,d2 ;offset
	add.l d3,0(a0,d2.l)
	subq.l #1,d0
	bne.s r9
	bra.s r11
r13
	cmp.l #$3f2,d3 ;end
	bne.s ree
	
	addq.l #1,d6
	cmp.l d6,d7
	bne.w r3
	
	moveq #1,d7
	move.l (a4),a0
r0	move.l d7,d0
	movem.l (sp)+,d1-d7/a1-a6
	rts
ree	sub.l a0,a0
	moveq #0,d7
	bra.s r0

fsres:
	movem.l d1/a0-a3/a6,-(sp)
	move.l a0,a3
	move.l 4.w,a6
	lea $150(a6),a0 ;ResourceList
	move.l (a0),a0 ;lh_Head
fsres3
	tst.l (a0) ;end of list?
	beq.s fsres1
	move.l 10(a0),a1 ;name
	lea fsresname(pc),a2
fsres5
	move.b (a1)+,d0
	move.b (a2)+,d1
	cmp.b d1,d0
	bne.s fsres2
	tst.b d0
	beq.s fsres4
	bra.s fsres5
fsres2
	move.l (a0),a0
	bra.s fsres3
	; FileSystem.resource does not exist -> create it
fsres1
	moveq #32,d0 ; sizeof(FileSysResource)
	move.l #$10001,d1
	jsr AllocMem(a6)
	move.l d0,a2
	move.b #8,8(a2) ; NT_RESOURCE
	lea fsresname(pc),a0
	move.l a0,10(a2) ; node name
	lea exter_name(pc),a0
	move.l a0,14(a2) ; fsr_Creator
	lea 18(a2),a0
	move.l a0,(a0) ; NewList() fsr_FileSysEntries
	addq.l #4,(a0)
	move.l a0,8(a0)
	lea $150(a6),a0 ; ResourceList
	move.l a2,a1
	jsr -$f6(a6) ; AddTail
	move.l a2,a0
	move.l a0,PP_FSRES_CREATED(a3)
fsres4
	move.l a0,d0
	movem.l (sp)+,d1/a0-a3/a6
	rts

addvolumenode
	movem.l d7/a6,-(sp)
	move.l d0,d7
	tst.b 32+64(a3)
	beq.s .end ;empty volume string = empty drive
	move.l 160(a3),a6
	cmp.w #37, 20(a6)
	bcs.s .prev37
	moveq #(1<<1)+(1<<3)+(1<<2),d1 ;LDF_WRITE | LDF_VOLUMES | LDF_DEVICES
	jsr -$29A(a6) ;AttemptLockDosList
	and.l #~1,d0
	beq.s .end
	btst #0,d7
	beq.s .nvol
	lea 32(a3),a0
	move.l a0,d1
	jsr -$2A6(a6) ;AddDosEntry (Volume)
.nvol	btst #1,d7
	beq.s .ndev
	tst.b 158(a3)
	bne.s .ndev
	st 158(a3)
	move.l 180(a3),d1
	jsr -$2A6(a6) ;AddDosEntry (Device)
.ndev	moveq #(1<<1)+(1<<3)+(1<<2),d1 ;LDF_WRITE | LDF_VOLUMES | LDF_DEVICES
	jsr -$294(a6) ;UnLockDosList
	bra.s .end
.prev37	move.l 4.w,a6
	jsr -$0084(a6) ; Forbid
	btst #0,d7
	beq.s .nvol13
	lea 32(a3),a0
	bsr.w adddosentry13
.nvol13	btst #1,d7
	beq.s .ndev13
	tst.b 158(a3)
	bne.s .ndev13
	st 158(a3)
	move.l 180(a3),a0
	bsr.w adddosentry13
.ndev13	jsr -$008a(a6) ;Permit
.end	movem.l (sp)+,d7/a6
	rts	

remvolumenode
	movem.l d7/a2/a6,-(sp)
	move.l d0,d7
	move.l 160(a3),a6
	cmp.w #37,20(a6)
	bcs.s .prev37
	moveq #(1<<1)+(1<<3)+(1<<2),d1 ;LDF_WRITE | LDF_VOLUMES | LDF_DEVICES
	jsr -$29A(a6) ;AttemptLockDosList
	and.l #~1,d0
	beq.s .end
	btst #0,d7
	beq.s .nvol
	lea 32(a3),a0
	move.l a0,d1
	jsr -$2A0(a6) ;RemDosEntry (Volume)
.nvol	btst #1,d7
	beq.s .ndev
	tst.b 158(a3)
	beq.s .ndev
	clr.b 158(a3)
	move.l 180(a3),d1
	jsr -$2A0(a6) ;RemDosEntry (Device)
.ndev	moveq #(1<<1)+(1<<3)+(1<<2),d1 ;LDF_WRITE | LDF_VOLUMES | LDF_DEVICES
	jsr -$294(a6) ;UnLockDosList
	bra.s .end
.prev37	move.l 4.w,a6
	jsr -$0084(a6) ; Forbid
	btst #0,d7
	beq.s .nvol13
	lea 32(a3),a0
	bsr.w remdosentry13
.nvol13	btst #1,d7
	beq.s .ndev13
	tst.b 158(a3)
	beq.s .ndev13
	clr.b 158(a3)
	move.l 180(a3),a0
	bsr.w remdosentry13
.ndev13	jsr -$008a(a6) ;Permit
.end	movem.l (sp)+,d7/a2/a6
	rts

adddosentry13:
	move.l a0,a1
	move.l 160(a3),a0
	move.l 34(a0),a0 ; RootNode
	move.l 24(a0),a0 ; DosInfo
	add.l a0,a0
	add.l a0,a0
	move.l 4(a0),(a1) ; myentry->dl_Next = di_DevInfo
	move.l a1,d0
	lsr.l #2,d0
	move.l d0,4(a0) ; di_DevInfo = myentry
	rts

remdosentry13:
	move.l a0,a2
	move.l 160(a3),a0
	move.l 34(a0),a0 ; RootNode
	move.l 24(a0),a0 ; DosInfo
	add.l a0,a0
	add.l a0,a0
	move.l 4(a0),a1 ; DosInfo->di_DevInfo
	add.l a1,a1
	add.l a1,a1
	cmp.l a2,a1
	bne.s .pr2
	; was first entry
	move.l (a2),4(a0) ; di_DevInfo = myentry->dl_Next
	bra.s .pr1
.pr2	move.l a1,d0
	beq.s .pr3
	move.l (a1),d0 ; prevEntry->dl_Next
	add.l d0,d0
	add.l d0,d0
	cmp.l d0,a2 ; next is our entry?
	beq.s .pr3
	move.l d0,a1
	bra.s .pr2
.pr3	move.l a1,d0
	beq.s .pr1
	move.l (a2),(a1) ; prevEntry->dl_Next = myentry->dl_Next
.pr1	rts

diskinsertremove:
	movem.l d2/a2/a6,-(sp)
	moveq #22,d2
	sub.l d2,sp
	move.l sp,a2
	move.w d2,d1
.l1	clr.b -1(a2,d1.w)
	subq.w #1,d1
	bne.s .l1
	move.l 4.w,a6
	moveq #15,d1 ;IECLASS_DISKREMOVED
	tst.l d0
	beq.s .l2
	moveq #16,d1 ;IECLASS_DISKINSERTED
.l2	move.b d1,4(a2) ;ie_Class
	move.w #$0800,8(a2); ie_Qualifier=IEQUALIFIER_MULTIBROADCAST

	move.l 164(a3),a1
	move.w #11,28(a1) ;IND_WRITEEVENT
	move.l #22,36(a1) ;sizeof(struct InputEvent)
	move.l a2,40(a1)
	move.b #1,30(a1) ;IOF_QUICK

	move.l 168(a3),a1
	move.w #10,28(a1) ;TR_GETSYSTIME
	move.b #1,30(a1) ;IOF_QUICK
	jsr -$01c8(a6) ;DoIO

	move.l 168(a3),a1
	move.l 32(a1),14(a2)
	move.l 36(a1),18(a2)

	move.l 164(a3),a1
	jsr -$01c8(a6) ;DoIO

	add.l d2,sp
	movem.l (sp)+,d2/a2/a6
	rts

dodiskchange
	tst.b d0
	beq.s .eject
	tst.b 32+64(a3)
	bne.s .end
	moveq #0,d0
.dc2
	tst.b 32+65(a3,d0.w)
	beq.s .dc1
	addq.b #1,d0
	bra.s .dc2
.dc1
	move.b d0,32+64(a3)
	beq.s .end
	move.l d1,d0
	bsr.w addvolumenode
	moveq #1,d0
	bsr.w diskinsertremove
	bra.s .end
.eject
	tst.b 32+64(a3)
	beq.s .end
	clr.b 32+64(a3)
	move.l d1,d0
	bsr.w remvolumenode
	moveq #0,d0
	bsr.w diskinsertremove
.end
	rts	

action_inhibit
	tst.l 20(a4) ;dp_Arg1
	beq.s .add
	moveq #0,d0
	moveq #1,d1
	bsr dodiskchange
	rts
.add
	moveq #1,d0
	moveq #3,d1
	bsr dodiskchange
	rts

sethighcyl
	move.l 184(a3),d0 ;highcyl or -1
	cmp.l #-1,d0
	beq.s .nohighcyl
	move.l 180(a3),a0 ;devicenode
	move.l 7*4(a0),a0 ;startup
	add.l a0,a0
	add.l a0,a0
	move.l 8(a0),a0 ; dosenvec
	add.l a0,a0
	add.l a0,a0
	move.l d0,10*4(a0)
.nohighcyl
	rts

diskchange
	bsr.w sethighcyl
	move.b 172(a3),d0
	bmi.s .nodisk
	moveq #1,d0
	moveq #3,d1
	bsr dodiskchange
	rts
.nodisk
	moveq #1,d1
	cmp.b #-2,d0 ;remove device node?
	bne.s .nod1
	moveq #3,d1
.nod1	moveq #0,d0
	bsr dodiskchange
	rts

	; exall is complex, need to emulate eac_MatchString and/or eac_MatchFunc
action_exall
	move.l 36(a4),a0 ; dp_Arg5, struct ExAllControl
	tst.l (a0)
	beq.s .ex0
	tst.l 8(a0) ; eac_MatchString
	bne.s .ex1
	tst.l 12(a0) ; eac_MatchFunc
	bne.s .ex1
.ex0	moveq #1,d0 ; no need to get more entries
	rts ;nothing to do here
.ex1:	movem.l d2-d7/a2-a6,-(sp)
	move.l a0,a5
	move.l 24(a4),a2 ;dp_Arg2, ExAllData
	move.l (a5),d7 ; eac_Entries
	moveq #0,d5 ; previous entry
.ex4	tst.l d7
	beq.s .ex3
	move.l a2,d0
	beq.s .ex3
	moveq #0,d6
	move.l 8(a5),d1 ; MatchString
	beq.s .ex5
	move.l 4(a2),d2 ; dir/file name
	move.l 160(a3),a6 ; dosbase
	jsr -$03cc(a6) ; MatchPatternNoCase
	tst.l d0
	bne.s .ex5
	st d6
.ex5	move.l 12(a5),d1 ; MatchFunc
	beq.s .ex6
	move.l d1,a0
	move.l a2,a1
	move.l a2,-(sp)
	lea 32(a4),a2 ; dp_Arg4, Type
	pea .pop(pc)
	move.l 8(a0),-(sp)
	rts
.pop	move.l (sp)+,a2
	tst.l d0
	bne.s .ex6
	st d6
.ex6	tst.b d6
	beq.s .ex7
	; need to delete current entry.. this is not really the proper way..
	move.l 4(a2),d0 ; pointer to filename (which is first address after structure)
	sub.l a2,d0 ; copy this much data
	tst.l (a2) ; delete last? (eac_Next)
	bne.s .ex10
	; need to clear previous eac_Next
	move.l d5,d0
	beq.s .ex8
	move.l d0,a0
	clr.l (a0)
	bra.s .ex8
.ex10	move.l (a2),a0
	move.l a2,a1
.ex9	move.l (a0)+,(a1)+
	subq.l #4,d0
	bpl.s .ex9
.ex8	subq.l #1,(a5) ; eac_Entries
	subq.l #1,d7
	bra.s .ex4
.ex7	move.l a2,d5
	move.l (a2),a2 ; eac_Next
	subq.l #1,d7
	bra.s .ex4
.ex3	movem.l (sp)+,d2-d7/a2-a6
	move.l 36(a4),a0 ; dp_Arg5, struct ExAllControl
	tst.l (a0) ; eac_Entries == 0 -> get more
	rts

	; mount harddrives, virtual or hdf

make_dev: ; IN: A0 param_packet, D6: unit_no
	; D7: b0=autoboot,b1=onthefly,b2=v36+,b3=force manual add,b4=nofakeboard
	; A4: expansionbase

	bsr.w fsres
	move.l d0,PP_FSRES(a0) ; pointer to FileSystem.resource
	move.l a0,-(sp)
	move.w #$FFEC,d0 ; filesys base
	bsr.w getrtbase
	move.l (a0),a5
	move.w #$FF28,d0 ; fill in unit-dependent info (filesys_dev_storeinfo)
	bsr.w getrtbaselocal
	move.l a0,a1
	move.l (sp)+,a0
	clr.l PP_FSSIZE(a0) ; filesystem size
	clr.l PP_FSPTR(a0) ; filesystem memory
	jsr (a1)
	; ret:0=virtual,1=hardfile,2=rdbhardfile,-1=hardfile-donotmount,-2=no more subunits
	move.l d0,d3
	cmp.w #-2,d3
	beq.w general_ret

	;cmp.w #1,d3
	;bne.s mountalways
	; KS < V36: init regular hardfiles only if filesystem is loaded
	;btst #2,d7
	;bne.s mountalways ; >= 36
	;btst #1,d7
	;bne.w mountalways

mountalways
	; allocate memory for loaded filesystem
	move.l PP_FSSIZE(a0),d0
	beq.s .nordbfs1
	bmi.s .nordbfs1
	move.l a0,-(sp)
	moveq #1,d1
	move.l 4.w,a6
	jsr  AllocMem(a6)
	move.l (sp)+,a0
	move.l d0,PP_FSPTR(a0)
.nordbfs1:

	tst.l d3
	bpl.s do_mount

	; do not mount but we might need to load possible custom filesystem(s)
	move.l a0,a1
	move.w #$FF20,d0 ; record in ui.startup (filesys_dev_remember)
	bsr.w getrtbaselocal
	jsr (a0)
	bra.s dont_mount

do_mount:
	move.l a4,a6
	move.l a0,-(sp)
	bsr.w makedosnode
	move.l (sp)+,a0 ; parmpacket
	move.l a0,a1
	move.l d0,a3 ; devicenode
	move.w #$FF20,d0 ; record in ui.startup (filesys_dev_remember)
	bsr.w getrtbaselocal
	jsr (a0)
	moveq #0,d0
	move.l d0,8(a3)          ; dn_Task
	move.l d0,16(a3)         ; dn_Handler
	move.l d0,32(a3)         ; dn_SegList

dont_mount:
	move.l PP_FSPTR(a1),a0
	tst.l PP_FSSIZE(a1)
	beq.s nordbfs3
	; filesystem needs relocation?
	move.l a0,d0
	beq.s nordbfs2
	bsr.w relocate
	movem.l d0/a0-a1,-(sp)
	move.l PP_FSSIZE(a1),d0
	move.l PP_FSPTR(a1),a1
	move.l 4.w,a6
	jsr FreeMem(a6)
	movem.l (sp)+,d0/a0-a1
	clr.l PP_FSSIZE(a1)
	move.l a0,PP_FSPTR(a1)
	tst.l d0
	beq.s nordbfs2
nordbfs3:
	tst.l PP_ADDTOFSRES(a1)
	beq.s nordbfs2
	bsr.w addfs
nordbfs2:

	tst.l d3
	bmi.w general_ret

	move.l 4.w,a6
	move.l a1,-(sp)
	lea bcplfsname(pc),a1
	jsr -$126(a6) ; FindTask
	move.l (sp)+,a1
	move.l d0,d1

	move.w #$FF18,d0 ; update dn_SegList if needed (filesys_dev_bootfilesys)
	bsr.w getrtbaselocal
	jsr (a0)

	move.l d3,d0
	move.b 79(a1),d3 ; bootpri
	tst.l d0
	bne.b MKDV_doboot ; not directory harddrive?

MKDV_is_filesys:
	move.l #6000,20(a3)     ; dn_StackSize
	lea	our_seglist(pc),a0
	moveq #-1,d0
	move.l a4,d1
	bne.s .expgv
	lea our_bcpl_seglist(pc),a0
	moveq #0,d0
.expgv
	move.l d0,36(a3)       ; dn_GlobalVec
	move.l a0,d0
	lsr.l  #2,d0
	move.l d0,32(a3)        ; dn_SegList

MKDV_doboot:
	btst #3,d7
	bne.s MKDV_noboot
	btst #0,d7
	beq.b MKDV_noboot
	cmp.b #-128,d3
	beq.s MKDV_noboot

	move.l 4.w,a6
	moveq #20,d0
	move.l #65536+1,d1
	jsr  AllocMem(a6)
	move.l d0,a1 ; bootnode
	move.w #$1000,d0
	or.b d3,d0
	move.w d0,8(a1)
	move.l $104(a5),10(a1) ; filesys_configdev
	move.l a3,16(a1) ; devicenode
	lea.l  74(a4),a0 ; MountList
	jsr -$0084(a6) ;Forbid
	jsr  -270(a6) ; Enqueue()
	jsr -$008a(a6) ;Permit
	moveq #0,d1
	moveq #0,d0
	rts

MKDV_noboot:
	moveq #0,d3
	move.l a1,a2 ; bootnode
	move.l a3,a0 ; parmpacket
	moveq #0,d1
	move.l d1,a1
	btst #1,d7
	beq.s .nob
	btst #2,d7
	beq.s .nob
	moveq #1,d1 ; ADNF_STARTPROC (v36+)
.nob
	moveq #-128,d0
	move.l a4,a6 ; expansion base
	bsr.w adddosnode
	btst #1,d7
	beq.s .noproc
	btst #2,d7
	bne.s .noproc

	; 1.3 and need to start immediately
	move.l (a2),a0 ; 'dh0' but need 'dh0:'
	moveq #2,d2
.devpr1	addq.l #1,d2
	tst.b -3(a0,d2.l)
	bne.s .devpr1
	move.l 4.w,a6
	add.l #4100,d2
	move.l d2,d0
	moveq #1,d1
	jsr AllocMem(a6)
	tst.l d0
	beq.s .noproc
	move.l (a2),a0
	move.l d0,a2
	move.l a2,a1
.devpr2	move.b (a0)+,(a1)+
	bne.s .devpr2
	move.b #':',-1(a1)
	clr.b (a1)
	move.l 4.w,a6
	lea doslibname(pc),a1
	jsr -$0198(a6) ; OldOpenLibrary
	move.l d0,a6
	move.l a2,d1
	move.l sp,d3
	; deviceproc needs lots of stack
	lea 4100(a2),sp
	jsr -$0AE(a6) ; DeviceProc (start fs handler)
	move.l d3,sp
	move.l d1,d3
	move.l a6,a1
	move.l 4.w,a6
	jsr -$019e(a6); CloseLibrary
	move.l a2,a1
	move.l d2,d0
	jsr FreeMem(a6)
.noproc
	move.l d3,d1
	moveq #0,d0
	rts

addfsonthefly ; d1 = fs index
	movem.l d2-d7/a2-a6,-(sp)
	move.l d1,d6
	moveq #2+4,d7
	move.l 4.w,a6
	lea.l explibname(pc),a1 ; expansion lib name
	moveq #36,d0
	jsr  -552(a6) ; OpenLibrary
	tst.l d0
	bne.s .newks
	bclr #2,d7
	lea.l explibname(pc),a1 ; expansion lib name
	moveq #0,d0
	jsr  -552(a6) ; OpenLibrary
.newks	move.l d0,a4
	move.l #PP_TOTAL,d0
	move.l #$10001,d1
	jsr AllocMem(a6)
	move.l d0,a0  ; param packet
	tst.l d0
	beq.s .nomem
	move.l a4,PP_EXPLIB(a0)
.next	movem.l a0/a4/a6,-(sp)
	bsr.w make_dev
	movem.l (sp)+,a0/a4/a6
	cmp.l #-2,d0
	beq.s .nomsub
	swap d6
	addq.w #1,d6
	swap d6
	bra.s .next
.nomsub	move.l a0,a1
	move.l #PP_TOTAL,d0
	jsr FreeMem(a6)
.nomem	move.l a4,a1
	jsr -414(a6) ; CloseLibrary
	movem.l (sp)+,d2-d7/a2-a6
	rts

clockreset:
	move.w #$ff58,d0 ; fsmisc_helper
	bsr.w getrtbaselocal
	moveq #3,d0 ; get time
	jsr (a0)
	move.l 168(a3),a1
	move.l d0,32(a1)
	beq.s .cr
	moveq #0,d0
	move.l d0,36(a1)
	move.w #11,28(a1) ;TR_SETSYSTIME
	move.b #1,30(a1) ;IOF_QUICK
	jsr -$01c8(a6) ;DoIO
.cr	rts

filesys_mainloop:
	moveq #0,d7
filesys_mainloop_bcpl:
	move.l 4.w,a6
	sub.l a1,a1
	jsr -294(a6) ; FindTask
	move.l d0,a0
	lea.l $5c(a0),a5 ; pr_MsgPort

	; Open DOS library
	lea.l doslibname(pc),a1
	jsr -$198(a6) ; OpenLibrary
	move.l d0,a2

	; Allocate some memory. Usage:
	; 0: lock chain
	; 4: command chain
	; 8: second thread's lock chain
	; 12: dummy message
	; 32: the volume (44+20+60+1 bytes. 20 = extra volnode space)
	; 157: mousehack started-flag
	; 158: device node on/off status
	; 160: dosbase
	; 164: input.device ioreq (disk inserted/removed input message)
	; 168: timer.device ioreq
	; 172: disk change from host
	; 173: bit 0: clock reset, bit 1: debugger start
	; 176: my task
	; 180: device node
	; 184: highcyl (-1 = ignore)

	move.l #12+20+(44+20+60+1)+3+4+4+4+(1+3)+4+4+4,d1
	move.w #$FF40,d0 ; startup_handler
	bsr.w getrtbaselocal
	moveq #1,d0
	jsr (a0)
	; if d0 != 0, it becomes our memory space
	; "allocated" from our autoconfig RAM space
	tst.l d0
	bne.s .havemem
	move.l d1,d0
	move.l #$10001,d1 ; MEMF_PUBLIC | MEMF_CLEAR
	jsr AllocMem(a6)
.havemem
	move.l d0,a3
	moveq #0,d6
	move.l d6,(a3)
	move.l d6,4(a3)
	move.l d6,8(a3)
	move.l a2,160(a3)
	st 158(a3)
	moveq #-1,d0
	move.l d0,184(a3)

	sub.l a1,a1
	jsr -294(a6) ; FindTask
	move.l d0,176(a3)

	lea inp_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	bsr.w allocdevice
	move.l d0,164(a3)
	lea tim_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	bsr.w allocdevice
	move.l d0,168(a3)

	moveq #0,d5 ; No commands queued.

	; Fetch our startup packet
	move.l d7,d3
	bne.s .got_bcpl
	move.l a5,a0
	jsr -384(a6) ; WaitPort
	move.l a5,a0
	jsr -372(a6) ; GetMsg
	move.l d0,a4
	move.l 10(a4),d3 ; ln_Name -> startup dos packet
.got_bcpl

	move.w #$FF40,d0 ; startup_handler
	bsr.w getrtbaselocal
	moveq #0,d0
	jsr (a0)
	move.l d0,d2

	bsr.w sethighcyl

	moveq #1,d0
	bsr.w addvolumenode

	btst #1,d2
	beq.s .nonotif
	moveq #1,d0
	bsr.w diskinsertremove
.nonotif

	bra.w FSML_Reply

	; We abuse some of the fields of the message we get. Offset 0 is
	; used for chaining unprocessed commands, and offset 1 is used for
	; indicating the status of a command. 0 means the command was handed
	; off to some UAE thread and did not complete yet, 1 means we didn't
	; even hand it over yet because we were afraid that might blow some
	; pipe limit, and -1 means the command was handed over and has completed
	; processing by now, so it's safe to reply to it.

FSML_loop:

	move.l a5,a0
	jsr -372(a6) ; GetMsg
	move.l d0,a4
	tst.l d0
	bne.s .msg

	moveq #0,d0
	move.b 15(a5),d1 ;mp_SigBit
	bset d1,d0
	bset #13,d0 ; SIGBREAK_CTRL_D
	jsr -$013e(a6) ;Wait
.msg
	; SIGBREAK_CTRL_D checks
	; clock reset
	btst #0,173(a3)
	beq.s .noclk
	bsr.w clockreset
	bclr #0,173(a3)
.noclk
	btst #1,173(a3)
	beq.s .nodebug
	bclr #1,173(a3)
.nodebug
	; disk change notification from native code
	tst.b 172(a3)
	beq.s .nodc
	; call filesys_media_change_reply (pre)
	move.w #$ff58,d0 ; fsmisc_helper
	bsr.w getrtbaselocal
	moveq #1,d0 ; filesys_media_change_reply
	jsr (a0)
	tst.l d0
	beq.s .nodc2
	bsr.w diskchange
.nodc2
	clr.b 172(a3)
	; call filesys_media_change_reply (post)
	move.w #$ff58,d0 ; fsmisc_helper
	bsr.w getrtbaselocal
	moveq #2,d0 ; filesys_media_change_reply
	jsr (a0)
.nodc
	move.l a4,d0
	beq.s nonnotif

	; notify reply?
	cmp.w #38, 18(a4)
	bne.s nonnotif
	cmp.l #NOTIFY_CLASS, 20(a4)
	bne.s nonnotif
	cmp.w #NOTIFY_CODE, 24(a4)
	bne.s nonnotif
	move.l 26(a4),a0 ; NotifyRequest
	move.l 12(a0),d0 ; flags
	and.l #NRF_WAIT_REPLY|NRF_MAGIC,d0
	cmp.l #NRF_WAIT_REPLY|NRF_MAGIC,d0
	bne.s nonoti
	and.l #~NRF_MAGIC,12(a0)
	move.l 16(a0),a0
	move.l a4,a1
	move.b #8,(a1)
	jsr -366(a6) ; PutMsg
	bra.w FSML_loop
nonoti
	move.l a4,a1
	moveq #38,d0
	jsr FreeMem(a6)
	bra.w FSML_loop

nonnotif
	moveq #-2,d2 ; lock timeout "done" value
	move.l a4,d0
	beq.s FSML_check_queue_other
	move.l 10(a4),d3 ; ln_Name
	bne.b FSML_FromDOS
	moveq #-1,d2 ; normal "done" value

	; It's a dummy packet indicating that some queued command finished.
	move.w #$FF50,d0 ; exter_int_helper
	bsr.w getrtbaselocal
	moveq #1,d0
	jsr (a0)
FSML_check_queue_other:
	; Go through the queue and reply all those that finished.
	lea 4(a3),a2
	move.l (a2),a0
FSML_check_old:
	move.l a0,d0
	beq.w FSML_loop
	move.l (a0),a1
	move.l d0,a0
	; This field may be accessed concurrently by several UAE threads.
	; This _should_ be harmless on all reasonable machines.
	move.l 4(a0),d0
	cmp.l d0,d2
	bne.b FSML_check_next
	movem.l a0/a1,-(a7)
	move.l 10(a0),a4
	bsr.b ReplyOne
	subq.l #1,d5  ; One command less in the queue
	movem.l (a7)+,a0/a1
	move.l a1,(a2)
	move.l a1,a0
	bra.b FSML_check_old
FSML_check_next:
	move.l a0,a2
	move.l a1,a0
	bra.b FSML_check_old

FSML_FromDOS:
	; Limit the number of outstanding started commands. We can handle an
	; unlimited number of unstarted commands.
	cmp.l #20,d5
	bcs  FSML_DoCommand
	; Too many commands queued.
	moveq #1,d0
	move.l d0,4(a4)
	bra.b FSML_Enqueue

FSML_DoCommand:
	bsr.b LockCheck  ; Make sure there are enough locks for the C code to grab.
	move.w #$FF30,d0 ; filesys_handler
	bsr.w getrtbaselocal
	jsr (a0)
	tst.l d0
	beq.b FSML_Reply
	; The command did not complete yet. Enqueue it and increase number of
	; queued commands
	; The C code already set 4(a4) to 0
	addq.l #1,d5
FSML_Enqueue:
	move.l 4(a3),(a4)
	move.l a4,4(a3)
	bra.w FSML_loop

FSML_Reply:
	move.l d3,a4
	bsr.b ReplyOne
	bra.w FSML_loop

ReplyOne:
	cmp.l #31,8(a4) ;ACTION_INHIBIT?
	bne.s FSML_ReplyOne2
	bsr.w action_inhibit
FSML_ReplyOne2:
	cmp.l #1033,8(a4) ;ACTION_EXAMINE_ALL
	bne.s FSML_ReplyOne3
.exaretry:
	bsr.w action_exall
	bne.s FSML_ReplyOne3
	; Arghh.. we need more entries. (some buggy programs fail if eac_Entries = 0 with continue enabled)
	move.w #$ff58,d0 ; fsmisc_helper
	bsr.w getrtbaselocal
	moveq #0,d0 ; exall
	jsr (a0)
	bra.s .exaretry
	
FSML_ReplyOne3:
	move.l (a4),a1  ; dp_Link
	move.l 4(a4),a0 ; dp_Port
	move.l a5,4(a4)
	jmp -366(a6) ; PutMsg

; ugly code to avoid calling AllocMem / FreeMem from native C code.
; We keep a linked list of 3 locks. In theory, only one should ever
; be used. Before handling every packet, we check that the list has the
; right length.

LockCheck:
	move.l d5,-(a7)
	moveq #-4,d5  ; Keep three locks
	move.l (a3),a2
	move.l a2,d7
LKCK_Loop:
	move.l a2,d1
	beq LKCK_ListEnd
	addq.l #1,d5
	beq.b LKCK_TooMany
	move.l a2,a1
	move.l (a2),a2
	bra.b LKCK_Loop
LKCK_ListEnd:
	addq.l #1,d5
	beq.b LKCK_ret
	move.l d7,a2
	moveq #24,d0 ; sizeof Lock is 20, 4 for chain
	moveq #1,d1 ; MEMF_PUBLIC
	jsr AllocMem(a6)
	addq.w #1,d6
	move.l d0,a2
	move.l d7,(a2)
	move.l a2,d7
	bra.b LKCK_ListEnd
LKCK_TooMany:
	move.l (a2),d0 ; We have too many, but we tolerate that to some extent.
	beq.b LKCK_ret
	move.l d0,a0
	move.l (a0),d0
	beq.b LKCK_ret
	move.l d0,a0
	move.l (a0),d0
	beq.b LKCK_ret

	moveq #0,d0 ; Now we are sure that we really have too many. Delete some.
	move.l d0,(a1)
LKCK_TooManyLoop:
	move.l a2,a1
	move.l (a1),a2
	moveq #24,d0
	jsr FreeMem(a6)
	add.l #$10000,d6
	move.l a2,d0
	bne.b LKCK_TooManyLoop
LKCK_ret:
	move.l d7,(a3)
	move.l (a7)+,d5
	rts

; mouse hack

newlist:
	move.l a0,(a0)
	addq.l #4,(a0)
	clr.l 4(a0)
	move.l a0,8(a0)
	rts

createport:
	movem.l d2/a2/a6,-(sp)
	move.l 4.w,a6
	moveq #-1,d0
	jsr -$014a(a6) ;AllocSignal
	sub.l a0,a0
	move.l d0,d2
	bmi.s .f
	moveq #34,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	sub.l a0,a0
	move.l d0,a2
	tst.l d0
	beq.s .f
	move.b #4,8(a2) ;NT_MSGPORT
	move.b d2,15(a2)
	sub.l a1,a1
	jsr -$0126(a6) ;FindTask
	move.l d0,16(a2)
	lea 20(a2),a0
	bsr.w newlist
	move.l a2,a0
.f
	move.l a0,d0
	movem.l (sp)+,d2/a2/a6
	rts

createio:
	movem.l d2/a2/a6,-(sp)
	move.l 4.w,a6
	tst.l d0
	beq.s .f
	move.l d0,a2
	move.l d1,d2
	bne.s .ci
	moveq #48,d2
.ci
	move.l d2,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	move.l d0,a0
	move.b #10,8(a0) ;NT_MESSAGE
	move.w d2,18(a0)
	move.l a2,14(a0)
.f
	tst.l d0
	movem.l (sp)+,d2/a2/a6
	rts

allocdevice
	movem.l d2-d3/a2/a6,-(sp)
	move.l a0,a2
	move.l d0,d2
	move.l d1,d3
	move.l 4.w,a6
	bsr.w createport
	move.l d3,d1
	bsr.w createio
	beq.s .f
	move.l a2,a0
	move.l d0,a1
	move.l d0,a2
	move.l d2,d0
	moveq #0,d1
	jsr -$01bc(a6) ;OpenDevice
	move.l d0,d1
	moveq #0,d0
	tst.l d1
	bne.s .f
	move.l a2,d0
.f
	tst.l d0
	movem.l (sp)+,d2-d3/a2/a6
	rts

createtask:
	movem.l d2/d3/d4/a2/a3/a6,-(sp)
	move.l 4.w,a6
	move.l d0,d4
	move.l a0,d2
	move.l a1,d3
	move.l #92+2048,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	tst.l d0
	beq .f
	move.l d0,a2
	move.b #1,8(a2) ;NT_TASK
	move.b d4,9(a2) ; priority
	move.l d2,10(a2)
	lea 92(a2),a3
	move.l a3,58(a2)
	lea 2048(a3),a3
	move.l a3,62(a2)
	move.l a3,54(a2)
	move.l a2,a1
	move.l d3,a2
	sub.l a3,a3
	move.l a1,d2
	jsr -$011a(a6) ;AddTask
	move.l d2,d0
.f
	movem.l (sp)+,d2/d3/d4/a2/a3/a6
	rts

; mousehack/tablet

mousehack_init:
	lea mhname(pc),a0
	lea mousehack_task(pc),a1
	moveq #19,d0
	bsr createtask
	rts

mhdoio:
	sub.w #24,sp
	clr.l (a2)
	clr.l 14(a2)
	clr.l 18(a2)
	move.l a2,a0
	move.l sp,a1
	moveq #24/4-1,d0
.mhdoioc
	move.l (a0)+,(a1)+
	dbf d0,.mhdoioc
	cmp.w #36,20(a6)
	bcc.s .mhdoio36
	move.l MH_TM(a5),a1
	move.w #10,28(a1) ;TR_GETSYSTIME
	move.b #1,30(a1) ;IOF_QUICK
	jsr -$01c8(a6) ;DoIO
	move.l MH_TM(a5),a1
	move.l 32(a1),14(sp)
	move.l 36(a1),18(sp)
.mhdoio36
	move.l MH_IO(a5),a1
	move.w #11,28(a1) ;IND_WRITEEVENT
	move.l #22,36(a1) ;sizeof(struct InputEvent)
	move.b #1,30(a1) ;IOF_QUICK
	move.l sp,40(a1)
	jsr -$01c8(a6) ;DoIO
	add.w #24,sp
	rts

MH_E = 0
MH_CNT = 2
MH_MAXX = 4
MH_MAXY = 6
MH_MAXZ = 8
MH_X = 10
MH_Y = 12
MH_Z = 14
MH_RESX = 16
MH_RESY = 18
MH_MAXAX = 20
MH_MAXAY = 22
MH_MAXAZ = 24
MH_AX = 26
MH_AY = 28
MH_AZ = 30
MH_PRESSURE = 32
MH_BUTTONBITS = 34
MH_INPROXIMITY = 38
MH_ABSX = 40
MH_ABSY = 42
MH_DATA_SIZE = 44

MH_INT = 0
MH_FOO = (MH_INT+22)
MH_FOO_CNT = 0
MH_FOO_BUTTONS = 4
MH_FOO_TASK = 8
MH_FOO_MASK = 12
MH_FOO_EXECBASE = 16
MH_FOO_INTBASE = 20
MH_FOO_GFXBASE = 24
MH_FOO_DELAY = 28
MH_FOO_DIMS_X = 32
MH_FOO_DIMS_Y = 36
MH_FOO_VPXY = 40
MH_FOO_MOFFSET = 44
MH_FOO_ALIVE = 48
MH_FOO_LIMITCNT = 52
MH_FOO_DIMS = 56
MH_FOO_DISP = (MH_FOO_DIMS+88)
MH_FOO_PREFS = (MH_FOO_DISP+48)
MH_FOO_SIZE = (MH_FOO_PREFS+102)

PREFS_SIZE = 102

MH_IEV = (MH_FOO+MH_FOO_SIZE) ;InputEvent
MH_IEH = (MH_IEV+22) ;InputHandler (Interrupt)
MH_IEPT = (MH_IEH+22) ;IEPointerTable/IENewTablet
MH_IENTTAGS = (MH_IEPT+32) ;space for ient_TagList
MH_IO = (MH_IENTTAGS+16*4*2)
MH_TM = (MH_IO+4)
MH_DATAPTR = (MH_TM+4)
MH_END = (MH_DATAPTR+4)

MH_MOUSEHACK = 0
MH_TABLET = 1
MH_ACTIVE = 7

TAG_USER   equ $80000000
TABLETA_Dummy		EQU	TAG_USER+$3A000
TABLETA_TabletZ		EQU	TABLETA_Dummy+$01
TABLETA_RangeZ		EQU	TABLETA_Dummy+$02
TABLETA_AngleX		EQU	TABLETA_Dummy+$03
TABLETA_AngleY		EQU	TABLETA_Dummy+$04
TABLETA_AngleZ		EQU	TABLETA_Dummy+$05
TABLETA_Pressure	EQU	TABLETA_Dummy+$06
TABLETA_ButtonBits	EQU	TABLETA_Dummy+$07
TABLETA_InProximity	EQU	TABLETA_Dummy+$08
TABLETA_ResolutionX	EQU	TABLETA_Dummy+$09
TABLETA_ResolutionY	EQU	TABLETA_Dummy+$0A


DTAG_DISP EQU $80000000
DTAG_DIMS EQU $80001000
DTAG_MNTR EQU $80002000
DTAG_NAME EQU $80003000

getgfxlimits:
	movem.l d0-d4/a0-a6,-(sp)
	move.l a0,a5
	sub.l a2,a2
	sub.l a3,a3
	sub.l a4,a4
	moveq #0,d4

	move.l MH_FOO_GFXBASE(a5),a6
	move.l MH_FOO_INTBASE(a5),a0
	move.l 60(a0),d0 ;FirstScreen
	beq.s .end

	move.l d0,a0
	lea 44(a0),a0 ;ViewPort
	move.l a0,a4
	jsr -$318(a6) ;GetVPModeID
	moveq #-1,d1
	moveq #-1,d2
	cmp.l d0,d1
	beq.s .end
	move.l d0,d3

	; mouse offset
	move.l MH_FOO_INTBASE(a5),a6
	lea MH_FOO_PREFS(a5),a0
	moveq #PREFS_SIZE,d0
	jsr -$84(a6) ;GetPrefs
	lea MH_FOO_PREFS(a5),a0
	move.w 100(a0),d4
	move.l MH_FOO_GFXBASE(a5),a6

  ; Text Overscan area needed
	sub.l a0,a0
	lea MH_FOO_DIMS(a5),a1
	moveq #0,d0
	move.w #88,d0
	move.l #DTAG_DIMS,d1
	move.l d3,d2
	jsr -$2f4(a6) ;GetDisplayInfoData
	moveq #-1,d1
	moveq #-1,d2
	tst.l d0
	bmi.s .end
	lea MH_FOO_DIMS(a5),a2
	move.l 50(a2),d1
	move.l 54(a2),d2
.end

	move.l 28(a4),d0
	cmp.w MH_FOO_MOFFSET(a5),d4
	bne.s .dosend
	cmp.l MH_FOO_VPXY(a5),d0
	bne.s .dosend
	cmp.l MH_FOO_DIMS_X(a5),d1
	bne.s .dosend
	cmp.l MH_FOO_DIMS_Y(a5),d2
	beq.s .nosend
.dosend
	move.l d0,MH_FOO_VPXY(a5)
	move.l d1,MH_FOO_DIMS_X(a5)
	move.l d2,MH_FOO_DIMS_Y(a5)
	move.w d4,MH_FOO_MOFFSET(a5)

	; This only for doublescan properties bit..
	sub.l a0,a0
	lea MH_FOO_DISP(a5),a1
	moveq #0,d0
	move.w #88,d0
	move.l #DTAG_DISP,d1
	move.l d3,d2
	jsr -$2f4(a6) ;GetDisplayInfoData
	tst.l d0
	bmi.s .nomntr
	lea MH_FOO_DISP(a5),a3
.nomntr

	;send updated data to native side
	move.w MH_FOO_MOFFSET(a5),d2
	move.w #$FF38,d0
	moveq #1,d1
	bsr.w getrtbaselocal
	jsr (a0)

.nosend
	movem.l (sp)+,d0-d4/a0-a6
	rts

mousehack_task:
	move.l 4.w,a6

	move.w 20(a6),d7 ; KS version

	moveq #-1,d0
	jsr -$014a(a6) ;AllocSignal
	moveq #0,d6
	bset d0,d6

	sub.l a1,a1
	jsr -$0126(a6) ;FindTask
	move.l d0,a2
	
	moveq #20,d0
	move.l a2,a1
	jsr -$012c(a6) ;SetTaskPri

	moveq #0,d0
	move.w #MH_END,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	move.l d0,a5

	move.w #RTAREA_MOUSEHACK,d0
	bsr.w getrtbase
	move.l a0,MH_DATAPTR(a5)
	move.l a0,a4

	lea MH_FOO(a5),a3
	move.l a6,MH_FOO_EXECBASE(a3)
	move.l a2,MH_FOO_TASK(a3)
	move.l d6,MH_FOO_MASK(a3)
	moveq #-1,d0
	move.w d0,MH_FOO_CNT(a3)

    ; send data structure address
	move.w #$FF38,d0
	moveq #5,d1
	bsr.w getrtbaselocal
	move.l a4,d0
	jsr (a0)

	lea MH_INT(a5),a1
	move.b #2,8(a1) ;NT_INTERRUPT
	move.b #5,9(a1) ;priority
	lea mhname(pc),a0
	move.l a0,10(a1)
	lea mousehackint(pc),a0
	move.l a0,18(a1)
	move.l a5,14(a1)
	moveq #5,d0 ;INTB_VERTB
	jsr -$00a8(a6)

mhloop:
	move.l d6,d0
	jsr -$013e(a6) ;Wait

	moveq #0,d0
	subq.l #1,MH_FOO_DELAY(a3)
	bpl.s .delay1
	moveq #10,d0
	move.l d0,MH_FOO_DELAY(a3)
.delay1

	tst.l MH_FOO_INTBASE(a3)
	bne.s .intyes
	tst.l MH_FOO_DELAY(a3)
	bne.s mhloop
	lea intlibname(pc),a1
	moveq #0,d0
	jsr -$0228(a6) ;OpenLibrary
	move.l d0,MH_FOO_INTBASE(a3)
	beq.s mhloop
	move.l d0,d1
	move.w #RTAREA_INTBASE,d0
	bsr.w getrtbase
	move.l d1,(a0)
.intyes
	tst.l MH_FOO_GFXBASE(a3)
	bne.s .gfxyes
	tst.l MH_FOO_DELAY(a3)
	bne.s mhloop
	lea gfxlibname(pc),a1
	moveq #0,d0
	jsr -$0228(a6) ;OpenLibrary
	move.l d0,MH_FOO_GFXBASE(a3)
	beq.w mhloop
	move.l d0,d1
	move.w #RTAREA_GFXBASE,d0
	bsr.w getrtbase
	move.l d1,(a0)
.gfxyes

	tst.l MH_IO(a5)
	bne.s .yesio
	tst.l MH_FOO_DELAY(a3)
	bne.s mhloop
	jsr -$0084(a6) ;Forbid
	lea 350(a6),a0 ;DeviceList
	lea inp_dev(pc),a1
	jsr -$114(a6) ;FindName
	move.l d0,d2
	jsr -$008a(a6) ;Permit
	tst.l d2
	beq.w mhloop
	lea inp_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	bsr.w allocdevice
	move.l d0,MH_IO(a5)
	beq.w mhend
	bra.w mhloop
.yesio
	tst.l MH_TM(a5)
	bne.s .yestim
	tst.l MH_FOO_DELAY(a3)
	bne.w mhloop
	jsr -$0084(a6) ;Forbid
	lea 350(a6),a0 ;DeviceList
	lea tim_dev(pc),a1
	jsr -$114(a6) ;FindName
	move.l d0,d2
	jsr -$008a(a6) ;Permit
	tst.l d2
	beq.w mhloop
	lea tim_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	bsr.w allocdevice
	move.l d0,MH_TM(a5)
	beq.w mhend

	;tell native side that mousehack is now active
	move.w #$FF38,d0
	moveq #0,d1
	bsr.w getrtbaselocal
	jsr (a0)
	bra.w mhloop
.yestim

	cmp.w #36,d7
	bcs.s .nodims
	cmp.w #50,d7
	bcc.s .nodims
	subq.l #1,MH_FOO_LIMITCNT(a3)
	bpl.s .nodims
	move.l a3,a0
	bsr.w getgfxlimits
	moveq #50,d0
	move.l d0,MH_FOO_LIMITCNT(a3)
.nodims

	lea MH_IEV(a5),a2

	move.b MH_E(a4),d0
	cmp.w #39,d7
	bcs.w .notablet
	btst #MH_TABLET,d0
	beq.w .notablet

	;IENewTablet

	lea MH_IEPT(a5),a0
	move.l a0,10(a2) ;ie_Addr

	move.b #$13,4(a2) ;ie_Class=IECLASS_NEWPOINTERPOS
	move.b #3,5(a2) ;ie_SubClass = IESUBCLASS_NEWTABLET
	clr.l (a0) ;ient_CallBack
	clr.l 4(a0)
	clr.l 8(a0)
	clr.w 12(a0)

	clr.w 6(a2) ;ie_Code
	bsr.w buttonstoqual

	move.w MH_X(a4),12+2(a0) ;ient_TabletX
	clr.w 16(a0)
	move.w MH_Y(a4),16+2(a0) ;ient_TabletY
	clr.w 20(a0)
	move.w MH_MAXX(a4),20+2(a0) ;ient_RangeX
	clr.w 24(a0)
	move.w MH_MAXY(a4),24+2(a0) ;ient_RangeY
	lea MH_IENTTAGS(a5),a1
	move.l a1,28(a0) ;ient_TagList
	move.l #TABLETA_Pressure,(a1)+
	move.w MH_PRESSURE(a4),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
	move.l #TABLETA_ButtonBits,(a1)+
	move.l MH_BUTTONBITS(a4),(a1)+
	
	moveq #0,d0

	move.w MH_RESX(a4),d0
	bmi.s .noresx
	move.l #TABLETA_ResolutionX,(a1)+
	move.l d0,(a1)+
.noresx
	move.w MH_RESY(a4),d0
	bmi.s .noresy
	move.l #TABLETA_ResolutionY,(a1)+
	move.l d0,(a1)+
.noresy

	move.w MH_MAXZ(a4),d0
	bmi.s .noz
	move.l #TABLETA_RangeZ,(a1)+
	move.l d0,(a1)+
	move.w MH_Z(a4),d0
	move.l #TABLETA_TabletZ,(a1)+
	move.l d0,(a1)+
.noz

	move.w MH_MAXAX(a4),d0
	bmi.s .noax
	move.l #TABLETA_AngleX,(a1)+
	move.w MH_AX(a4),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
.noax
	move.w MH_MAXAY(a4),d0
	bmi.s .noay
	move.l #TABLETA_AngleY,(a1)+
	move.w MH_AY(a4),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
.noay
	move.w MH_MAXAZ(a4),d0
	bmi.s .noaz
	move.l #TABLETA_AngleZ,(a1)+
	move.w MH_AZ(a4),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
.noaz
	
	moveq #0,d0
	move.w MH_INPROXIMITY(a4),d0
	bmi.s .noproxi
	move.l #TABLETA_InProximity,(a1)+
	move.l d0,(a1)+
.noproxi
	clr.l (a1) ;TAG_DONE

	bsr.w mhdoio
	
.notablet
	move.b MH_E(a4),d0
	btst #MH_TABLET,d0
	bne.s .yestablet
	btst #MH_MOUSEHACK,d0
	beq.w mhloop
.yestablet

	move.l MH_FOO_INTBASE(a3),a0

	move.w MH_ABSX(a4),d0
	move.w 34+14(a0),d1
	add.w d1,d1
	sub.w d1,d0
	bpl.s .xn
	moveq #0,d0
.xn
	move.w d0,10(a2)	;ie_Addr/X

	move.w MH_ABSY(a4),d0
	move.w 34+12(a0),d1
	add.w d1,d1
	sub.w d1,d0
	bpl.s .yn
	moveq #0,d0
.yn
	move.w d0,12(a2)	;ie_Addr/Y

	btst #MH_TABLET,MH_E(a4)
	beq.s .notablet2

	;create mouse button events if button state changed
	move.w #$68,d3 ;IECODE_LBUTTON->IECODE_RBUTTON->IECODE_MBUTTON
	moveq #1,d2
	move.l MH_BUTTONBITS(a4),d4
.nextbut
	move.l d4,d0
	and.l d2,d0
	move.l MH_FOO_BUTTONS(a3),d1
	and.l d2,d1
	cmp.l d0,d1
	beq.s .nobut
	
	move.w #$0200,4(a2) ;ie_Class=IECLASS_RAWMOUSE,ie_SubClass=0
	move.w d3,d1
	tst.b d0
	bne.s .butdown
	bset #7,d1 ;IECODE_UP_PREFIX
.butdown
	move.w d1,6(a2) ;ie_Code
	bsr.w buttonstoqual

	bsr.w mhdoio

.nobut
	addq.w #1,d3
	add.w d2,d2
	cmp.w #8,d2
	bne.s .nextbut
	move.l d4,MH_FOO_BUTTONS(a3)

.notablet2
	btst #MH_MOUSEHACK,MH_E(a4)
	beq.w mhloop

	move.w #$0400,4(a2) ;IECLASS_POINTERPOS
	clr.w 6(a2) ;ie_Code

	bsr.w buttonstoqual
	cmp.w #36,20(a6)
	bcs.s .mhvpre36
	move.l a6,-(sp)
	move.l MH_IO(a5),a6
	move.l 20(a6),a6
	jsr -$2a(a6) ;PeekQualifier
	move.l (sp)+,a6
	and.w #$7fff,d0
	or.w d0,8(a2) ;ie_Qualifier
.mhvpre36	

	bsr.w mhdoio

	bra.w mhloop

mhend
	rts

buttonstoqual:
	;IEQUALIFIER_MIDBUTTON=0x1000/IEQUALIFIER_RBUTTON=0x2000/IEQUALIFIER_LEFTBUTTON=0x4000
	move.l MH_BUTTONBITS(a4),d1
	moveq #0,d0
	btst #0,d1
	beq.s .btq1
	bset #14,d0
.btq1:
	btst #1,d1
	beq.s .btq2
	bset #13,d0
.btq2:
	btst #2,d1
	beq.s .btq3
	bset #12,d0
.btq3:
	move.w d0,8(a2) ;ie_Qualifier
	rts

mousehackint:
	tst.l MH_IO(a1)
	beq.s .l1
	tst.l MH_TM(a1)
	beq.s .l1
	move.l MH_DATAPTR(a1),a0
	move.w MH_CNT(a0),d0
	cmp.w MH_FOO+MH_FOO_CNT(a1),d0
	beq.s .l2
	move.w d0,MH_FOO+MH_FOO_CNT(a1)
.l1
	move.l a1,-(sp)
	move.l MH_FOO+MH_FOO_EXECBASE(a1),a6
	move.l MH_FOO+MH_FOO_MASK(a1),d0
	move.l MH_FOO+MH_FOO_TASK(a1),a1
	jsr -$0144(a6) ; Signal
	move.l (sp)+,a1
.l2
	subq.w #1,MH_FOO+MH_FOO_ALIVE(a1)
	bpl.s .l3
	move.w #50,MH_FOO+MH_FOO_ALIVE(a1)
	move.w #RTAREA_INTREQ+3,d0
	bsr.w getrtbase
	st (a0)
.l3
	lea $dff000,a0
	moveq #0,d0
	rts


bootres_code:
	
	rts

; KS 1.1 and older don't have expansion.library
; emulate missing functions

makedosnode:
	cmp.w #0,a6
	beq.s makedosnodec
	jsr -144(a6) ; MakeDosNode()
	rts
makedosnodec
	movem.l d2-d7/a2-a6,-(sp)
	move.l a0,a2
	move.l 4.w,a6
	move.l #44+16+4*17+128,d0
	move.l #65536+1,d1
	jsr -$c6(a6)
	move.l d0,a3 ;devicenode
	lea 44(a3),a4 ;fssm
	lea 16(a4),a5 ;environ
	lea 4*17(a5),a6

	move.l a5,a1
	lea 4*4(a2),a0
	moveq #17-1,d0
.cenv
	move.l (a0)+,(a1)+
	dbf d0,.cenv

	move.l a6,a0
	move.l (a2),a1
	addq.l #1,a0
.copy1
	move.b (a1)+,(a0)+
	bne.s .copy1
	move.l a0,d0
	addq.l #3,d0
	and.b #~3,d0
	move.l d0,d6
	move.l a0,d0
	subq.l #2,d0
	sub.l a6,d0
	move.b d0,(a6)

	move.l d6,a0
	move.l 1*4(a2),a1
	addq.l #1,a0
.copy2
	move.b (a1)+,(a0)+
	bne.s .copy2
	move.l a0,d0
	subq.l #2,d0
	sub.l d6,d0
	move.l d6,a0
	move.b d0,(a0)

	move.l 2*4(a2),(a4) ;fssm_unit
	move.l d6,d0
	lsr.l #2,d0
	move.l d0,4(a4) ;fssm_device
	lea 4*4(a2),a0
	move.l a5,d0
	lsr.l #2,d0
	move.l d0,8(a4) ;fssm_Environ
	move.l 3*4(a2),12(a4) ;fssm_flags

	move.l a4,d0
	lsr.l #2,d0
	move.l d0,28(a3) ;dn_startup
	move.l a6,d0
	lsr.l #2,d0
	move.l d0,40(a3) ;dn_name
	moveq #10,d0
	move.l d0,24(a3) ;dn_priority
	move.l #4000,20(a3) ;dn_stack
	
	move.l a3,d0
	
	movem.l (sp)+,d2-d7/a2-a6
	rts

adddosnode:
	;d0 = bootpri
	;d1 = flags
	;a0 = devicenode
	cmp.w #0,a6
	beq.s adddosnodec
	jsr  -150(a6) ; AddDosNode
	rts
adddosnodec
	movem.l d2-d4/a2-a6,-(sp)
	move.l a0,a2
	move.l 4.w,a6
	lea doslibname(pc),a1
	jsr -$0198(a6) ; OldOpenLibrary
	move.l d0,a5

	move.l 34(a5),a0 ;dl_root
	move.l 24(a0),d0 ;rn_info
	lsl.l #2,d0
	move.l d0,a3 ;dosinfo
	
	move.l 4(a3),(a2)
	move.l a2,d0
	lsr.l #2,d0
	move.l d0,4(a3)

	move.l a5,a1
	move.l 4.w,a6
	jsr -$019e(a6); CloseLibrary
	movem.l (sp)+,d2-d4/a2-a6
	rts

	;wrap BCPL filesystem call to non-BCPL filesystem
	;d1 = dospacket that BCPL entry point fetched
	;We need to put dospacket back in pr_MsgPort.

	cnop 0,4
	dc.l ((bcplwrapper_end-bcplwrapper_start)>>2)+1
bcplwrapper:
	dc.l 0
	dc.l ((bcplwrapper_end-bcplwrapper_start)>>2)+1
bcplwrapper_start:
	move.l d1,d2
	move.l 4.w,a6
	sub.l a1,a1
	jsr -$126(a6) ;FindTask
	move.l d0,a0
	lea 92(a0),a0 ;pr_MsgPort
	lsl.l #2,d2
	move.l d2,a1
	move.l (a1),a1 ;dp_Link = Message
	jsr -$16e(a6) ;PutMsg
	move.l d2,d1
	lea wb13ffspatches(pc),a1
	move.w #$FF68,d0
	bsr.w getrtbaselocal
	jsr (a0)
	jmp (a0)
	;CopyMem() patches
wb13ffspatches:
	moveq #$30,d0
	bra.s copymem
	moveq #$28,d0
	bra.s copymem
	move.l d6,d0
	bra.s copymem
	move.l d6,d0
copymem:
	move.b (a0)+,(a1)+
	subq.l #1,d0
	bgt.s copymem
	rts	

	cnop 0,4
	dc.l 0
	dc.l 1
	dc.l 4
	dc.l 2
bcplwrapper_end:

	; d0 bit 0 = install trap
	; d0 bit 1 = add fake configdev

hwtrap_install:
	movem.l d2/a2/a6,-(sp)
	move.l d0,d2
	move.l 4.w,a6

	btst #1,d2
	beq.s .nocd2
	; create fake configdev
	lea.l explibname(pc),a1
	moveq #0,d0
	jsr  -552(a6) ; OpenLibrary
	tst.l d0
	beq.s .nocd2
	move.l a6,a2
	move.l d0,a6
	jsr -$030(a6) ;expansion/AllocConfigDev
	tst.l d0
	beq.s .nocd1
	move.l d0,a0
	lea start(pc),a1
	move.l a1,d0
	clr.w d0
	move.l d0,32(a0)
	move.l #65536,36(a0)
	move.w #$0104,16(a0) ;type + product
	move.w #2011,16+4(a0) ;manufacturer
	moveq #1,d0
	move.l d0,22(a0) ;serial
	jsr -$01e(a6) ;expansion/AddConfigDev
.nocd1
	move.l a6,a1
	move.l a2,a6
	jsr -$19e(a6)
.nocd2
	
	btst #0,d2
	beq.s .notrap
	move.l #RTAREA_INTREQ,d0
	bsr.w getrtbase
	move.l a0,a2
	move.l a0,d0
	clr.w d0
	move.l d0,a0
	move.l a6,RTAREA_SYSBASE(a0)
	moveq #26,d0
	move.l #$10001,d1
	jsr AllocMem(a6)
	move.l d0,a1
	lea.l hwtrap_name(pc),a0
	move.l a0,10(a1)
	move.l a2,14(a1)
	lea.l hwtrap_interrupt(pc),a0
	move.l a0,18(a1)
	move.w #$027a,8(a1)
	moveq #13,d0 ;EXTER
	jsr -168(a6) ; AddIntServer
.notrap

	movem.l (sp)+,d2/a2/a6
	rts

hwtrap_entry:
	movem.l d0-d1/a0-a1,-(sp)
.retry
	move.l #TRAP_STATUS,d0
	bsr.w getrtbase
	move.l a0,a1
	move.l #TRAP_DATA,d0
	bsr.w getrtbase
	moveq #TRAP_DATA_NUM-1,d0
.nexttrap
	tst.w TRAP_STATUS_LOCK_WORD(a1)
	beq.s .foundfree
.nexttrap2
	add.w #TRAP_DATA_SLOT_SIZE,a0
	add.w #TRAP_STATUS_SLOT_SIZE,a1
	dbf d0,.nexttrap
	bra.s .retry
.foundfree

	; store registers
	movem.l d2-d7,TRAP_DATA_DATA+2*4(a0)
	movem.l a2-a6,TRAP_DATA_DATA+8*4+2*4(a0)
	move.l 0*4(sp),TRAP_DATA_DATA(a0) ;D0
	move.l 1*4(sp),TRAP_DATA_DATA+1*4(a0) ;D1
	move.l 2*4(sp),TRAP_DATA_DATA+8*4(a0) ;A0
	move.l 3*4(sp),TRAP_DATA_DATA+8*4+1*4(a0) ;A1

	move.l a0,a2 ; data
	move.l a1,a3 ; status

	move.l 4.w,a6
	clr.l TRAP_DATA_TASKWAIT(a2)
	tst.b (a3)
	beq.s .nowait
	sub.l a1,a1
	jsr -$126(a6) ; FindTask
	move.l d0,TRAP_DATA_TASKWAIT(a2)
.nowait
	
	; trap number, this triggers the trap
	move.w 4*4(sp),(a3)

	move.l #$80000000,d5
	move.w #10000,d4

.waittrap
	tst.b TRAP_STATUS_STATUS2(a3)
	bne.s .triggered
	cmp.b #$fe,TRAP_STATUS_SECOND+TRAP_STATUS_STATUS(a3)
	bne.s .waittrap1
	move.b #4,TRAP_STATUS_SECOND+TRAP_STATUS_STATUS(a3)
	move.l a2,a0
	move.l a3,a1
	moveq #1,d0
	bsr.w hwtrap_command
	move.b #3,TRAP_STATUS_SECOND+TRAP_STATUS_STATUS(a3)
	bra.s .waittrap
.waittrap1
	tst.l TRAP_DATA_TASKWAIT(a2)
	bne.s .waittrap2
	cmp.l #$80000000,d5
	bne.s .waittrap
	subq.w #1,d4
	bpl.s .waittrap
	; lower priority after some busy wait rounds
	sub.l a1,a1
	jsr -$126(a6) ; FindTask
	move.l d0,a1
	moveq #-120,d0
	jsr -$12c(a6) ; SetTaskPri
	move.l d0,d5
	bra.s .waittrap
.waittrap2
	move.l #$100,d0
	jsr -$13e(a6) ; Wait
	bra.s .waittrap
.triggered
	cmp.l #$80000000,d5
	beq.s .triggered1
	; restore original priority
	sub.l a1,a1
	jsr -$126(a6) ; FindTask
	move.l d0,a1
	move.l d5,d0
	jsr -$12c(a6) ; SetTaskPri
.triggered1
	move.b #1,TRAP_STATUS_STATUS2(a3)

	move.l a2,a0
	move.l a3,a1

	; restore registers
	movem.l TRAP_DATA_DATA(a0),d0-d7
	movem.l TRAP_DATA_DATA+8*4+2*4(a0),a2-a6
	move.l TRAP_DATA_DATA+8*4(a0),-(sp) ;A0
	move.l TRAP_DATA_DATA+8*4+1*4(a0),-(sp) ;A1

	clr.l TRAP_DATA_TASKWAIT(a0)

	; free trap data entry
	clr.w TRAP_STATUS_LOCK_WORD(a1)

	move.l (sp)+,a1
	move.l (sp)+,a0
	; pop trap number and d0-d1/a0-a1
	lea 4*4+2(sp),sp
	rts

hwtrap_interrupt:
	movem.l d2/d3/a2/a3,-(sp)
	moveq #0,d3
	move.l a1,a2

	tst.b 1(a2)
	beq.s .notrapint
	moveq #1,d3
.checkagain
	move.l a2,d0
	clr.w d0
	move.l d0,a0
	moveq #TRAP_DATA_NUM+TRAP_DATA_SEND_NUM-1,d1
	move.l a0,a1
	move.l RTAREA_SYSBASE(a0),a6
	add.l #TRAP_DATA,a0
	add.l #TRAP_STATUS,a1
.nexttrap
	tst.b TRAP_STATUS_STATUS(a1)
	beq.s .next
	cmp.b #$ff,TRAP_STATUS_SECOND+TRAP_STATUS_STATUS(a1)
	bne.s .next
	moveq #0,d0
	bsr.s hwtrap_command
	beq.s .trapdone
	move.b #$fd,TRAP_STATUS_SECOND+TRAP_STATUS_STATUS(a1)
	bra.s .checkagain
.trapdone
	move.b #1,TRAP_STATUS_SECOND+TRAP_STATUS_STATUS(a1)
	bra.s .checkagain
.next
	add.w #TRAP_DATA_SLOT_SIZE,a0
	add.w #TRAP_STATUS_SLOT_SIZE,a1
	dbf d1,.nexttrap
.notrapint

	tst.b 2(a2)
	beq.s .notrapackint
	moveq #1,d3
	move.l a2,d0
	clr.w d0
	move.l d0,a2
	move.l d0,a3
	move.l RTAREA_SYSBASE(a2),a6
	add.l #TRAP_DATA,a2
	add.l #TRAP_STATUS,a3
	moveq #TRAP_DATA_NUM-1,d2
.esn2
	move.l TRAP_DATA_TASKWAIT(a2),d0
	beq.s .esn1
	tst.b TRAP_STATUS_STATUS(a3)
	beq.s .esn1
	tst.b TRAP_STATUS_STATUS2(a3)
	bpl.s .esn1
	clr.l TRAP_DATA_TASKWAIT(a2)
	move.l d0,a1
	move.l #$100,d0
	jsr -$144(a6) ; Signal
.esn1
	add.w #TRAP_DATA_SLOT_SIZE,a2
	add.w #TRAP_STATUS_SLOT_SIZE,a3
	dbf d2,.esn2
.notrapackint

	move.l d3,d0
	movem.l (sp)+,d2/d3/a2/a3
	rts

	; a0: data
	; a1: status
	; d0: task=1,int=0
hwtrap_command:
	movem.l d1-d5/a1-a5,-(sp)
	move.w d0,d5
	lea TRAP_DATA_SECOND+4(a0),a4
	lea TRAP_STATUS_SECOND(a1),a5
	move.w (a5),d4 ;command
	add.w d4,d4
	lea trapjumps(pc),a3
	move.w 0(a3,d4.w),d1
	bne.s .intisok
	tst.w d5
	beq.s .needint
	move.w #hw_call_lib-trapjumps,d1
	cmp.w #2*18,d4
	beq.s .intisok
	move.w #hw_call_func-trapjumps,d1
	cmp.w #2*19,d4
	beq.s .intisok
.needint
	move.l #RTAREA_TRAPTASK,d0
	bsr.w getrtbase
	move.l (a0),d0
	beq.s .intisok
	; call func or lib, task needed
	move.l d0,a1
	move.l #$0100,d0
	jsr -$144(a6) ; Signal
	moveq #1,d0
	bra.s .exitactive
.intisok
	add.w d1,a3
	movem.l (a4),a0-a2
	movem.l (a4),d0-d2
	jsr (a3)
	move.l d0,(a4)
	moveq #0,d0
.exitactive
	movem.l (sp)+,d1-d5/a1-a5
	rts

trapjumps:
	dc.w hw_multi-trapjumps; 0

	dc.w hw_put_long-trapjumps ; 1
	dc.w hw_put_word-trapjumps ; 2
	dc.w hw_put_byte-trapjumps ; 3
	
	dc.w hw_get_long-trapjumps ; 4
	dc.w hw_get_word-trapjumps ; 5
	dc.w hw_get_byte-trapjumps ; 6
	
	dc.w hw_copy_bytes-trapjumps ; 7
	dc.w hw_copy_words-trapjumps ; 8
	dc.w hw_copy_longs-trapjumps ; 9

	dc.w hw_copy_bytes-trapjumps ; 10
	dc.w hw_copy_words-trapjumps ; 11
	dc.w hw_copy_longs-trapjumps ; 12

	dc.w hw_copy_string-trapjumps; 13
	dc.w hw_copy_string-trapjumps; 14

	dc.w hw_set_longs-trapjumps ; 15
	dc.w hw_set_words-trapjumps ; 16
	dc.w hw_set_bytes-trapjumps ; 17

	dc.w 0 ; 18 (call lib)
	dc.w 0 ; 19 (call func)

	dc.w hw_op_nop-trapjumps ; 20
	
	dc.w hw_get_bstr-trapjumps ; 21


hw_put_long:
	move.l d1,(a0)
	rts
hw_put_word:
	move.w d1,(a0)
	rts
hw_put_byte:
	move.b d1,(a0)
	rts
hw_get_long:
	move.l (a0),d0
	rts
hw_get_word:
	moveq #0,d0
	move.w (a0),d0
	rts
hw_get_byte:
	moveq #0,d0
	move.b (a0),d0
	rts
hw_copy_bytes:
	; a0 = src, a1 = dest, d2 = bytes
	move.l d2,d0
	jmp -$270(a6) ; CopyMem
hw_copy_words:
	; a0 = src, a1 = dest, d2 = words
	move.l d2,d0
	add.l d0,d0
	jmp -$270(a6) ; CopyMem
hw_copy_longs:
	; a0 = src, a1 = dest, d2 = longs
	move.l d2,d0
	lsl.l #2,d0
	jmp -$270(a6) ; CopyMem
hw_copy_string:
	; a0 = src, a1 = dest, d2 = maxlen
	moveq #0,d0
	subq.w #1,d2
	beq.s .endstring
	addq.w #1,d0
	move.b (a0)+,(a1)+
	bne.s hw_copy_string
.endstring
	clr.b -1(a1)
	rts
hw_set_longs:
	; a0 = addr, d1 = data, d2 = num
	move.l d1,(a0)+
	subq.l #1,d2
	bne.s hw_set_longs
	rts
hw_set_words:
	; a0 = addr, d1 = data, d2 = num
	move.w d1,(a0)+
	subq.l #1,d2
	bne.s hw_set_words
	rts
hw_set_bytes:
	; a0 = addr, d1 = data, d2 = num
	move.b d1,(a0)+
	subq.l #1,d2
	bne.s hw_set_bytes
	rts
hw_get_bstr:
	; a0 = src, a1 = dest, d2 = maxlen
	moveq #0,d0
	move.b (a0)+,d0
.bcopy2
	subq.w #1,d0
	bmi.s .bcopy1
	subq.w #1,d2
	bmi.s .bcopy1
	move.b (a0)+,(a1)+
	bra.s .bcopy2
.bcopy1
	clr.b (a1)
	rts

hw_call_lib:
	; a0 = base
	; d1 = offset
	; a2 = regs
	movem.l d2-d7/a2-a6,-(sp)
	move.l a0,a6
	add.w d1,a0
	pea funcret(pc)
	move.l a0,-(sp)
	movem.l (a2),d0-d7/a0-a5
	rts
funcret	
	movem.l (sp)+,d2-d7/a2-a6
	rts

hw_call_func:
	; a0 = func
	; a1 = regs
	movem.l d2-d7/a2-a6,-(sp)
	pea funcret(pc)
	move.l a0,-(sp)
	movem.l (a1),d0-d7/a0-a6
	rts

hw_op_nop:
	move.l d5,d0
	rts

hw_multi:
	movem.l d0-d7/a0-a6,-(sp)
	; a0 = data, d1 = num
	move.l a0,a4
	move.l a4,a5
	move.l d1,d7
	moveq #0,d5
.multi0
	move.w (a4)+,d4
	moveq #0,d6
	move.w (a4)+,d6
	add.w d4,d4
	lea trapjumps(pc),a3
	add.w 0(a3,d4.w),a3
	movem.l (a4),a0-a2
	movem.l (a4),d0-d2
	jsr (a3)
	move.l d0,(a4)
	move.l d0,d5
	tst.w d6
	beq.s .multi1
	move.w d6,d3
	and.w #15,d6
	lsr.w #8,d3
	mulu #5*4,d3
	lsl.w #2,d6
	add.w d6,d3
	move.l d0,4(a5,d3.w)
.multi1	
	add.w #5*4-4,a4
	subq.l #1,d7
	bne.s .multi0
	movem.l (sp)+,d0-d7/a0-a6
	rts


	; this is called from external executable

moverom:
	; a0 = ram copy

	move.l a0,a5 ; ram copy
	
	moveq #20,d7
	move.l 4.w,a6

	jsr -$0084(a6) ;Forbid
	
	move.w #$FF38,d0
	bsr.w getrtbase
	move.l a5,d2
	moveq #19,d1
	jsr (a0)
	;ROM is now write-enabled
	;d0 = temp space
	tst.l d0
	beq.w .mov1
	move.l d0,a3
	move.l (a3)+,a4 ; ROM

	; Copy ROM to RAM
	move.l a4,a0
	move.l a5,a1
	move.w #65536/4-1,d0
.mov5
	move.l (a0)+,(a1)+
	dbf d0,.mov5

	move.l a5,d1
	sub.l a4,d1

	; Handle relocations from UAE side
	
	; Relative
	move.l a3,a0
.mov7
	moveq #0,d0
	move.w (a0)+,d0
	beq.s .mov6
	lea 0(a4,d0.l),a1
	add.l d1,(a1)
	lea 0(a5,d0.l),a1
	add.l d1,(a1)
	bra.s .mov7
.mov6

	; Absolute
.mov9
	moveq #0,d0
	move.l (a0)+,d0
	beq.s .mov8
	move.l d0,a1
	add.l d1,(a1)
	bra.s .mov9
.mov8

	moveq #0,d0
	bsr.w getrtbase
	lea getrtbase(pc),a1
	sub.l a0,a1
	
	add.l a5,a1
	; replace RAM copy relative getrtbase
	; fetch with absolute lea rombase,a0
	move.w #$41f9,(a1)+
	move.l a4,(a1)+
	; and.l #$ffff,d0
	move.w #$0280,(a1)+
	move.l #$0000ffff,(a1)+
	; add.l d0,a0; rts
	move.l #$d1c04e75,(a1)
	
	; redirect some commonly used
	; functions to RAM code
	lea moveromreloc(pc),a0
.mov3
	moveq #0,d0
	move.w (a0)+,d0
	beq.s .mov2
	add.w #8+4,d0
	lea 0(a4,d0.l),a1
	; JMP xxxxxxxx
	move.w #$4ef9,(a1)+
	lea 0(a5,d0.l),a2
	move.l a2,(a1)
	bra.s .mov3
.mov2

	cmp.w #36,20(a6)
	bcs.s .mov4
	jsr -$27c(a6) ; CacheClearU
.mov4

	; Tell UAE that all is done
	; Re-enables write protection
	move.w #$FF38,d0
	bsr.w getrtbase
	move.l a5,d2
	moveq #20,d1
	jsr (a0)
	moveq #0,d7

.mov1
	jsr -$008a(a6)
	
	move.l d7,d0
	rts
	
moveromreloc:
	dc.w FSML_loop-start
	dc.w mhloop-start
	dc.w kaint-start
	dc.w trap_task_wait-start
	dc.w exter_task_wait-start
	dc.w 0

	cnop 0,4
getrtbaselocal:
	lea start-8-4(pc),a0
	and.l #$FFFF,d0
	add.l d0,a0
	rts
	cnop 0,4
getrtbase:
	lea start-8-4(pc),a0
	and.l #$FFFF,d0
	add.l d0,a0
	rts
	nop
	nop

inp_dev: dc.b 'input.device',0
tim_dev: dc.b 'timer.device',0
mhname: dc.b 'UAE mouse driver',0
kaname: dc.b 'UAE heart beat',0
exter_name: dc.b 'UAE fs',0
fstaskname: dc.b 'UAE fs automounter',0
fswtaskname: dc.b 'UAE fs worker',0
fstraptaskname: dc.b 'UAE trap worker',0
fsprocname: dc.b 'UAE fs automount process',0
doslibname: dc.b 'dos.library',0
intlibname: dc.b 'intuition.library',0
gfxlibname: dc.b 'graphics.library',0
explibname: dc.b 'expansion.library',0
fsresname: dc.b 'FileSystem.resource',0
fchipname: dc.b 'megachip memory',0
bcplfsname: dc.b "File System",0
hwtrap_name:
	dc.b "UAE board",0
	even
rom_end:

	END
