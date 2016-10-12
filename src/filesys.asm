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

AllocMem = -198
FreeMem = -210

; don't forget filesys.c! */
PP_MAXSIZE = 4 * 96
PP_FSSIZE = 400
PP_FSPTR = 404
PP_ADDTOFSRES = 408
PP_FSRES = 412
PP_FSRES_CREATED = 416
PP_EXPLIB = 420
PP_FSHDSTART = 424
PP_TOTAL = (PP_FSHDSTART+140)

NOTIFY_CLASS = $40000000
NOTIFY_CODE = $1234
NRF_SEND_MESSAGE = 1
NRF_SEND_SIGNAL = 2
NRF_WAIT_REPLY = 8
NRF_NOTIFY_INITIAL = 16
NRF_MAGIC = $80000000

	dc.l 16 								; 4
our_seglist:
	dc.l 0 									; 8 /* NextSeg */
start:
	bra.s startjmp
	dc.w 9						;0 12
startjmp:
	bra.w filesys_mainloop		;1 16
	dc.l make_dev-start			;2 20
	dc.l filesys_init-start		;3 24
	dc.l exter_server-start		;4 28
	dc.l bootcode-start			;5 32
	dc.l setup_exter-start		;6 36
	dc.l 0      				;7 40
	dc.l clipboard_init-start 	;8 44
	;52

bootcode:
	lea.l doslibname(pc),a1
	jsr -96(a6) ; FindResident
	move.l d0,a0
	move.l 22(a0),d0
	move.l d0,a0
	jsr (a0)
	rts

residenthack
	movem.l d0-d2/a0-a2/a6,-(sp)

	move.w #$FF38,d0
	moveq #17,d1
	bsr.w getrtbase
	jsr (a0)
	tst.l d0
	beq.s .rsh

	move.l 4.w,a6
	cmp.w #37,20(a6)
	bcs.s .rsh
	moveq #residentcodeend-residentcodestart,d0
	move.l d0,d2
	moveq #1,d1
	jsr AllocMem(a6)
	tst.l d0
	beq.s .rsh
	move.l d0,a2

	move.l a2,a0
	lea residentcodestart(pc),a1
.cp1
	move.l (a1)+,(a0)+
	subq.l #4,d2
	bne.s .cp1

	jsr -$0078(a6) ;Disable
	move.l a6,a1
	move.w #-$48,a0 ;InitCode
	move.l a2,d0
	jsr -$01a4(a6) ;SetFunction
	move.l d0,residentcodejump2-residentcodestart+2(a2)
	lea myafterdos(pc),a0
	move.l a0,residentcodejump1-residentcodestart+2(a2)
	jsr -$27C(a6) ;CacheClearU
	jsr -$007e(a6) ;Enable
.rsh
	movem.l (sp)+,d0-d2/a0-a2/a6
	rts
	
myafterdos
	move.l (sp),a0
	move.l 2(a0),a0
	move.l a0,-(sp)
	jsr (a0) ;jump to original InitCode
	move.l (sp)+,a0
	addq.l #4,sp ;remove return address
	movem.l d0-d7/a1-a6,-(sp)
	move.l a6,a1
	move.l a0,d0
	move.w #-$48,a0 ;InitCode
	jsr -$01a4(a6) ;SetFunction (restore original)
	bsr.w clipboard_init
	bsr.w consolehook
	movem.l (sp)+,d0-d7/a1-a6
	rts ;return directly to caller

	cnop 0,4
residentcodestart:
	btst #2,d0 ;RTF_AFTERDOS?
	beq.s resjump
residentcodejump1
	jsr $f00000
resjump
residentcodejump2
	jmp $f00000
	cnop 0,4
residentcodeend:

filesys_init:
	movem.l d0-d7/a0-a6,-(sp)
	move.l 4.w,a6
	move.w #$FFEC,d0 ; filesys base
	bsr getrtbase
	move.l (a0),a5
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

	; create fake configdev
	exg a4,a6
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
	moveq #1,d7
	tst.w d5
	beq.s .oldks
	bset #2,d7
.oldks	move.l a3,-(sp)
	move.l a3,a0
	bsr.w make_dev
	move.l (sp)+,a3
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
	move.l a4,a1
	jsr -414(a6) ; CloseLibrary

	; add MegaChipRAM
	moveq #3,d4 ; MEMF_CHIP | MEMF_PUBLIC
	cmp.w #36,20(a6)
	bcs.s FSIN_ksold
	or.w #256,d4 ; MEMF_LOCAL
FSIN_ksold
	move.w #$FF80,d0
	bsr.w getrtbase
	jsr (a0) ; d1 = size, a1 = start address
	move.l a1,a0
	move.l d1,d0
	beq.s FSIN_fchip_done
	move.l d4,d1
	moveq #-5,d2
	lea fchipname(pc),a1
	jsr -618(a6) ; AddMemList
FSIN_fchip_done

	lea fstaskname(pc),a0
	lea fsmounttask(pc),a1
	moveq #10,d0
	bsr createtask
	move.l d0,a1
	moveq #1,d1
	move.w #$FF48,d0 ; store task pointer
	bsr.w getrtbase
	jsr (a0)

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
	movem.l d2-d4/a2/a6,-(sp)
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
	move.l a6,a1
	move.l 4.w,a6
	jsr -$019e(a6); CloseLibrary
.noproc
	movem.l (sp)+,d2-d4/a2/a6
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
	bsr.w getrtbase
	jsr (a0)
	move.l d0,d1
	bmi.s .out
	bsr.w addfsonthefly
.out	moveq #0,d0
	rts

exter_data:
exter_server:
	movem.l a2,-(sp)
	move.w #$FF50,d0 ; exter_int_helper
	bsr.w getrtbase
	moveq.l #0,d0
	jsr (a0)
	tst.l d0
	beq.w exter_server_exit
	; This is the hard part - we have to send some messages.
	move.l 4.w,a6
EXTS_loop:
	move.w #$FF50,d0 ; exter_int_helper
	bsr.w getrtbase
	moveq.l #2,d0
	jsr (a0)
	cmp.w #1,d0
	blt.w EXTS_done
	bgt.b EXTS_signal_reply
	jsr -366(a6) ; PutMsg
	bra.b EXTS_loop
EXTS_signal_reply:
	cmp.w #2,d0
	bgt.b EXTS_reply
	move.l d1,d0
	jsr -$144(a6)	; Signal
	bra.b EXTS_loop
EXTS_reply:
	cmp.w #3,d0
	bgt.b EXTS_cause
	jsr -$17a(a6)   ; ReplyMsg
	bra.b EXTS_loop
EXTS_cause:
	cmp.w #4,d0
	bgt.b EXTS_notificationhack
	jsr -$b4(a6)	; Cause
	bra.b EXTS_loop
EXTS_notificationhack:
	cmp.w #5,d0
	bgt.b EXTS_done
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
	bra.w EXTS_loop
EXTS_done:
	move.w #$FF50,d0 ;exter_int_helper
	bsr.w getrtbase
	moveq.l #4,d0
	jsr (a0)	
	moveq.l #1,d0 ; clear Z - it was for us.
exter_server_exit:
	movem.l (sp)+,a2
	rts

heartbeatvblank:
	movem.l d0-d1/a0-a2,-(sp)

	move.w #$FF38,d0
	moveq #18,d1
	bsr.w getrtbase
	jsr (a0)
	move.l d0,a2

	moveq #22,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	move.l d0,a1

	move.b #2,8(a1) ;NT_INTERRUPT
	move.b #-10,9(a1) ;priority
	lea kaname(pc),a0
	move.l a0,10(a1)
	lea kaint(pc),a0
	move.l a0,18(a1)
	move.l a2,14(a1)
	moveq #5,d0 ;INTB_VERTB
	jsr -$00a8(a6)

	movem.l (sp)+,d0-d1/a0-a2
	rts

kaint:
	addq.l #1,(a1)
	moveq #0,d0
	rts

setup_exter:
	movem.l d0-d1/a0-a1,-(sp)
	bsr.w residenthack
	moveq.l #26,d0
	move.l #$10001,d1
	jsr AllocMem(a6)
	move.l d0,a1
	lea.l exter_name(pc),a0
	move.l a0,10(a1)
	lea.l exter_data(pc),a0
	move.l a0,14(a1)
	lea.l exter_server(pc),a0
	move.l a0,18(a1)
	move.w #$0214,8(a1)
	moveq.l #3,d0
	jsr -168(a6) ; AddIntServer

	bsr.w heartbeatvblank

	move.w #$FF38,d0
	moveq #4,d1
	bsr.w getrtbase
	jsr (a0)
	tst.l d0
	beq.s .nomh
	bsr.w mousehack_init
.nomh
	movem.l (sp)+,d0-d1/a0-a1
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
	tst.b 32+44(a3)
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
	tst.b 32+44(a3)
	bne.s .end
	moveq #0,d0
.dc2
	tst.b 32+45(a3,d0.w)
	beq.s .dc1
	addq.b #1,d0
	bra.s .dc2
.dc1
	move.b d0,32+44(a3)
	beq.s .end
	move.l d1,d0
	bsr.w addvolumenode
	moveq #1,d0
	bsr.w diskinsertremove
	bra.s .end
.eject
	tst.b 32+44(a3)
	beq.s .end
	clr.b 32+44(a3)
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

diskchange
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

make_dev: ; IN: A0 param_packet, D6: unit_no, D7: b0=autoboot,b1=onthefly,b2=v36+
	; A4: expansionbase

	bsr.w fsres
	move.l d0,PP_FSRES(a0) ; pointer to FileSystem.resource
	move.l a0,-(sp)
	move.w #$FFEC,d0 ; filesys base
	bsr.w getrtbase
	move.l (a0),a5
	move.w #$FF28,d0 ; fill in unit-dependent info (filesys_dev_storeinfo)
	bsr.w getrtbase
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
	bsr.w getrtbase
	jsr (a0)
	bra.s dont_mount

do_mount:
	move.l a4,a6
	move.l a0,-(sp)
	jsr -144(a6) ; MakeDosNode()
	move.l (sp)+,a0 ; parmpacket
	move.l a0,a1
	move.l d0,a3 ; devicenode
	move.w #$FF20,d0 ; record in ui.startup (filesys_dev_remember)
	bsr.w getrtbase
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

	move.w #$FF18,d0 ; update dn_SegList if needed (filesys_dev_bootfilesys)
	bsr.w getrtbase
	jsr (a0)

	move.l d3,d0
	move.b 79(a1),d3 ; bootpri
	tst.l d0
	bne.b MKDV_doboot ; not directory harddrive?

MKDV_is_filesys:
	move.l #6000,20(a3)     ; dn_StackSize
	lea.l our_seglist(pc),a0
	move.l a0,d0
	lsr.l  #2,d0
	move.l d0,32(a3)        ; dn_SegList
	moveq #-1,d0
	move.l d0,36(a3)       ; dn_GlobalVec

MKDV_doboot:
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
	moveq #0,d0
	rts

MKDV_noboot:
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
	jsr  -150(a6) ; AddDosNode
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
	moveq #0,d0
	jsr -$0228(a6) ; OpenLibrary
	move.l d0,a6
	move.l a2,d1
	jsr -$0AE(a6) ; DeviceProc (start fs handler, ignore return code)
	move.l a6,a1
	move.l 4.w,a6
	jsr -$019e(a6); CloseLibrary
	move.l a2,a1
	move.l d2,d0
	jsr FreeMem(a6)
.noproc
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
	bsr.w getrtbase
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
	move.l 4.w,a6
	sub.l a1,a1
	jsr -294(a6) ; FindTask
	move.l d0,a0
	lea.l $5c(a0),a5 ; pr_MsgPort

	; Open DOS library
	lea.l doslibname(pc),a1
	moveq.l #0,d0
	jsr -552(a6) ; OpenLibrary
	move.l d0,a2

	; Allocate some memory. Usage:
	; 0: lock chain
	; 4: command chain
	; 8: second thread's lock chain
	; 12: dummy message
	; 32: the volume (44+80+1 bytes)
	; 157: mousehack started-flag
	; 158: device node on/off status
	; 160: dosbase
	; 164: input.device ioreq (disk inserted/removed input message)
	; 168: timer.device ioreq
	; 172: disk change from host
	; 173: clock reset
	; 176: my task
	; 180: device node
	move.l #12+20+(80+44+1)+(1+3)+4+4+4+(1+3)+4+4,d0
	move.l #$10001,d1 ; MEMF_PUBLIC | MEMF_CLEAR
	jsr AllocMem(a6)
	move.l d0,a3
	moveq.l #0,d6
	move.l d6,(a3)
	move.l d6,4(a3)
	move.l d6,8(a3)
	move.l a2,160(a3)
	st 158(a3)

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

	moveq.l #0,d5 ; No commands queued.

	; Fetch our startup packet
	move.l a5,a0
	jsr -384(a6) ; WaitPort
	move.l a5,a0
	jsr -372(a6) ; GetMsg
	move.l d0,a4
	move.l 10(a4),d3 ; ln_Name
	move.w #$FF40,d0 ; startup_handler
	bsr.w getrtbase
	moveq.l #0,d0
	jsr (a0)
	move.l d0,d2

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
	tst.b 173(a3)
	beq.s .noclk
	bsr.w clockreset
	clr.b 173(a3)
.noclk
	; disk change notification from native code
	tst.b 172(a3)
	beq.s .nodc
	; call filesys_media_change_reply (pre)
	move.w #$ff58,d0 ; fsmisc_helper
	bsr.w getrtbase
	moveq #1,d0 ; filesys_media_change_reply
	jsr (a0)
	tst.l d0
	beq.s .nodc2
	bsr.w diskchange
.nodc2
	clr.b 172(a3)
	; call filesys_media_change_reply (post)
	move.w #$ff58,d0 ; fsmisc_helper
	bsr.w getrtbase
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
	bsr.w getrtbase
	moveq.l #1,d0
	jsr (a0)
FSML_check_queue_other:
	; Go through the queue and reply all those that finished.
	lea.l 4(a3),a2
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
	moveq.l #1,d0
	move.l d0,4(a4)
	bra.b FSML_Enqueue

FSML_DoCommand:
	bsr.b LockCheck  ; Make sure there are enough locks for the C code to grab.
	move.w #$FF30,d0 ; filesys_handler
	bsr.w getrtbase
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
	bsr.w getrtbase
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
	moveq.l #-4,d5  ; Keep three locks
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
	moveq.l #24,d0 ; sizeof Lock is 20, 4 for chain
	moveq.l #1,d1 ; MEMF_PUBLIC
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

	moveq.l #0,d0 ; Now we are sure that we really have too many. Delete some.
	move.l d0,(a1)
LKCK_TooManyLoop:
	move.l a2,a1
	move.l (a1),a2
	moveq.l #24,d0
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
.f	move.l a0,d0
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
.f	tst.l d0
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
.f	tst.l d0
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
.f	movem.l (sp)+,d2/d3/d4/a2/a3/a6
	rts

; mousehack/tablet

mousehack_init:
	lea mhname(pc),a0
	lea mousehack_task(pc),a1
	moveq #19,d0
	bsr createtask
	rts

mhdoiotimer:
	move.l MH_TM(a5),a1
	move.w #10,28(a1) ;TR_GETSYSTIME
	move.b #1,30(a1) ;IOF_QUICK
	jsr -$01c8(a6) ;DoIO
	move.l MH_TM(a5),a1
	move.l 32(a1),14(a2)
	move.l 36(a1),18(a2)
	move.l MH_IO(a5),a1
	move.b #1,30(a1) ;IOF_QUICK
	jsr -$01c8(a6) ;DoIO
	rts
mhdoio:
	clr.l 14(a2)
	clr.l 18(a2)
	move.l MH_IO(a5),a1
	move.b #1,30(a1) ;IOF_QUICK
	jsr -$01c8(a6) ;DoIO
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
MH_DATA = (MH_TM+4)
MH_END = (MH_DATA+MH_DATA_SIZE)

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
	bsr.w getrtbase
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
	move.l d0,a4
	
	moveq #20,d0
	move.l a4,a1
	jsr -$012c(a6) ;SetTaskPri

	moveq #0,d0
	move.w #MH_END,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	move.l d0,a5

	lea MH_FOO(a5),a3
	move.l a6,MH_FOO_EXECBASE(a3)
	move.l a4,MH_FOO_TASK(a3)
	move.l d6,MH_FOO_MASK(a3)
	moveq #-1,d0
	move.w d0,MH_FOO_CNT(a3)

    ; send data structure address
	move.w #$FF38,d0
	moveq #5,d1
	bsr.w getrtbase
	move.l a5,d0
	add.l #MH_DATA,d0
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

mhloop
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
	beq.s mhloop
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
	bsr.w getrtbase
	jsr (a0)
	bra.w mhloop
.yestim

	cmp.w #36,d7
	bcs.s .nodims
	subq.l #1,MH_FOO_LIMITCNT(a3)
	bpl.s .nodims
	move.l a3,a0
	bsr.w getgfxlimits
	moveq #50,d0
	move.l d0,MH_FOO_LIMITCNT(a3)
.nodims

	move.l MH_IO(a5),a1
	lea MH_IEV(a5),a2
	move.w #11,28(a1) ;IND_WRITEEVENT
	move.l #22,36(a1) ;sizeof(struct InputEvent)
	move.l a2,40(a1)

	move.b MH_E+MH_DATA(a5),d0
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

	move.w MH_X+MH_DATA(a5),12+2(a0) ;ient_TabletX
	clr.w 16(a0)
	move.w MH_Y++MH_DATA(a5),16+2(a0) ;ient_TabletY
	clr.w 20(a0)
	move.w MH_MAXX+MH_DATA(a5),20+2(a0) ;ient_RangeX
	clr.w 24(a0)
	move.w MH_MAXY+MH_DATA(a5),24+2(a0) ;ient_RangeY
	lea MH_IENTTAGS(a5),a1
	move.l a1,28(a0) ;ient_TagList
	move.l #TABLETA_Pressure,(a1)+
	move.w MH_PRESSURE+MH_DATA(a5),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
	move.l #TABLETA_ButtonBits,(a1)+
	move.l MH_BUTTONBITS+MH_DATA(a5),(a1)+
	
	moveq #0,d0

	move.w MH_RESX+MH_DATA(a5),d0
	bmi.s .noresx
	move.l #TABLETA_ResolutionX,(a1)+
	move.l d0,(a1)+
.noresx
	move.w MH_RESY+MH_DATA(a5),d0
	bmi.s .noresy
	move.l #TABLETA_ResolutionY,(a1)+
	move.l d0,(a1)+
.noresy

	move.w MH_MAXZ+MH_DATA(a5),d0
	bmi.s .noz
	move.l #TABLETA_RangeZ,(a1)+
	move.l d0,(a1)+
	move.w MH_Z+MH_DATA(a5),d0
	move.l #TABLETA_TabletZ,(a1)+
	move.l d0,(a1)+
.noz

	move.w MH_MAXAX+MH_DATA(a5),d0
	bmi.s .noax
	move.l #TABLETA_AngleX,(a1)+
	move.w MH_AX+MH_DATA(a5),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
.noax
	move.w MH_MAXAY++MH_DATA(a5),d0
	bmi.s .noay
	move.l #TABLETA_AngleY,(a1)+
	move.w MH_AY+MH_DATA(a5),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
.noay
	move.w MH_MAXAZ++MH_DATA(a5),d0
	bmi.s .noaz
	move.l #TABLETA_AngleZ,(a1)+
	move.w MH_AZ+MH_DATA(a5),d0
	ext.l d0
	asl.l #8,d0
	move.l d0,(a1)+
.noaz
	
	moveq #0,d0
	move.w MH_INPROXIMITY+MH_DATA(a5),d0
	bmi.s .noproxi
	move.l #TABLETA_InProximity,(a1)+
	move.l d0,(a1)+
.noproxi
	clr.l (a1) ;TAG_DONE

	bsr.w mhdoio
	
	;create mouse button events if button state changed
	move.w #$68,d3 ;IECODE_LBUTTON->IECODE_RBUTTON->IECODE_MBUTTON
	moveq #1,d2
	move.l MH_BUTTONBITS+MH_DATA(a5),d4
.nextbut
	move.l d4,d0
	and.l d2,d0
	move.l MH_FOO_BUTTONS(a3),d1
	and.l d2,d1
	cmp.l d0,d1
	beq.s .nobut
	
	clr.l (a2)
	move.w #$0200,4(a2) ;ie_Class=IECLASS_RAWMOUSE,ie_SubClass=0
	clr.l 10(a2)	;ie_Addr/X+Y
	move.w d3,d1
	tst.b d0
	bne.s .butdown
	bset #7,d1 ;IECODE_UP_PREFIX
.butdown
	move.w d1,6(a2) ;ie_Code
	clr.w 8(a2) ;ie_Qualifier

	bsr.w mhdoio

.nobut
	addq.w #1,d3
	add.w d2,d2
	cmp.w #8,d2
	bne.s .nextbut
	move.l d4,MH_FOO_BUTTONS(a3)

.notablet

	move.b MH_E+MH_DATA(a5),d0
	btst #MH_MOUSEHACK,d0
	beq.w mhloop

	clr.l (a2)
	move.w #$0400,4(a2) ;IECLASS_POINTERPOS
	clr.w 6(a2) ;ie_Code
	bsr.w buttonstoqual

	move.l MH_FOO_INTBASE(a3),a0

	move.w MH_ABSX+MH_DATA(a5),d0
	move.w 34+14(a0),d1
	add.w d1,d1
	sub.w d1,d0
	bpl.s .xn
	moveq #0,d0
.xn
	move.w d0,10(a2)

	move.w MH_ABSY+MH_DATA(a5),d0
	move.w 34+12(a0),d1
	add.w d1,d1
	sub.w d1,d0
	bpl.s .yn
	moveq #0,d0
.yn
	move.w d0,12(a2)

	bsr.w mhdoiotimer

	bra.w mhloop

mhend
	rts

buttonstoqual:
	;IEQUALIFIER_MIDBUTTON=0x1000/IEQUALIFIER_RBUTTON=0x2000/IEQUALIFIER_LEFTBUTTON=0x4000
	move.l MH_BUTTONBITS+MH_DATA(a5),d1
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
	move.w MH_CNT+MH_DATA(a1),d0
	cmp.w MH_FOO+MH_FOO_CNT(a1),d0
	beq.s .l2
	move.w d0,MH_FOO+MH_FOO_CNT(a1)
.l1
	move.l MH_FOO+MH_FOO_EXECBASE(a1),a6
	move.l MH_FOO+MH_FOO_MASK(a1),d0
	move.l MH_FOO+MH_FOO_TASK(a1),a1
	jsr -$0144(a6) ; Signal
.l2
	subq.w #1,MH_FOO+MH_FOO_ALIVE(a1)
	bpl.s .l3
	move.w #50,MH_FOO+MH_FOO_ALIVE(a1)
	move.w #$FF38,d0
	moveq #2,d1
	bsr.w getrtbase
	jsr (a0)
.l3
	lea $dff000,a0
	moveq #0,d0
	rts

; clipboard sharing

CLIP_WRITE_SIZE = 0
CLIP_WRITE_ALLOC = (CLIP_WRITE_SIZE+4)
CLIP_TASK = (CLIP_WRITE_ALLOC+4)
CLIP_UNIT = (CLIP_TASK+4)
CLIP_ID = (CLIP_UNIT+4)
CLIP_EXEC = (CLIP_ID+4)
CLIP_DOS = (CLIP_EXEC+4)
CLIP_HOOK = (CLIP_DOS+4)
CLIP_BUF = (CLIP_HOOK+20)
CLIP_BUF_SIZE = 8
CLIP_POINTER_NOTIFY = (CLIP_BUF+CLIP_BUF_SIZE)
CLIP_POINTER_PREFS = (CLIP_POINTER_NOTIFY+48)
CLIP_END = (CLIP_POINTER_PREFS+32)

clipboard_init:
	movem.l a5/a6,-(sp)

	move.w #$FF38,d0
	moveq #17,d1
	bsr.w getrtbase
	jsr (a0)
	btst #0,d0
	beq.s .noclip

	move.l 4.w,a6
	move.l #CLIP_END,d0
	move.l #$10001,d1
	jsr AllocMem(a6)
	tst.l d0
	beq.w clipdie
	move.l d0,a5
	move.l a6,CLIP_EXEC(a5)

	move.w #$FF38,d0
	moveq #14,d1
	bsr.w getrtbase
	move.l a5,d0
	jsr (a0)

	; we need to be a process, LoadLibrary() needs to call dos
	lea clname(pc),a0
	lea clipboard_proc(pc),a1
	moveq #-10,d0
	move.l #10000,d1
	bsr.w createproc
.noclip
	moveq #0,d0
	movem.l (sp)+,a5/a6
	rts

clipkill
	move.w #$FF38,d0
	moveq #10,d1
	bsr.w getrtbase
	jsr (a0)
	rts

clipdie:
	bsr.s clipkill
	move.l a5,d0
	beq.s .cd1
	move.l CLIP_EXEC(a5),a6
	move.l CLIP_DOS(a5),d0
	beq.s .cd2
	move.l d0,a1
	jsr -414(a6) ; CloseLibrary
.cd2
	move.l a5,a1
	move.l #CLIP_END,d0
	jsr FreeMem(a6)	
.cd1
	moveq #0,d0
	rts

prefsread:
	movem.l d2-d4/a2-a6,-(sp)
	move.l CLIP_DOS(a5),a6
	lea pointer_prefs(pc),a0
	move.l a0,d1
	move.l #1005,d2
	jsr -$001e(a6) ;Open
	move.l d0,d4
	beq.s .pr1
	lea CLIP_POINTER_PREFS(a5),a2
.pr4
	clr.l (a2)
.pr3
	move.w 2(a2),(a2)
	move.l a2,d2
	addq.l #2,d2
	moveq	#2,d3
	move.l d4,d1
	jsr -$002a(a6) ;Read
	cmp.l d0,d3
	bne.s .pr1
	cmp.l #'PNTR',(a2)
	bne.s .pr3
	move.l a2,d2
	moveq #4,d3
	move.l d4,d1
	jsr -$002a(a6) ;Read	
	move.l a2,d2
	moveq #32,d3
	move.l d4,d1
	jsr -$002a(a6) ;Read	
	cmp.l d0,d3
	bne.s .pr1
	tst.w 16(a2) ;pp_Which
	bne.s .pr4
	move.w #$FF38,d0
	moveq #16,d1
	bsr.w getrtbase
	jsr (a0)
.pr1
	move.l d4,d1
	beq.s .pr2
	jsr -$0024(a6) ;Close
.pr2
	movem.l (sp)+,d2-d4/a2-a6
	rts

prefshook:
	move.l CLIP_DOS(a5),a6
	lea ram_name(pc),a0
	move.l a0,d1
	moveq #-2,d2
	jsr -$0054(a6) ;Lock
	move.l d0,d1
	beq.s .ph1
	jsr -$005a(a6) ;Unlock
	move.l CLIP_EXEC(a5),a6
	lea CLIP_POINTER_NOTIFY(a5),a2
	moveq #-1,d0
	jsr -$014a(a6) ;AllocSignal
	move.b d0,20(a2) ;nr_SignalNum
	lea pointer_prefs(pc),a0
	move.l a0,(a2) ;nr_Name
	move.l #NRF_SEND_SIGNAL|NRF_NOTIFY_INITIAL,12(a2) ;nr_Flags 
	move.l CLIP_TASK(a5),16(a2) ;nr_Task
	move.l CLIP_DOS(a5),a6
	move.l a2,d1
	jsr -$378(a6) ;StartNotify
.ph1
	move.l CLIP_EXEC(a5),a6
	rts

	cnop 0,4
	dc.l 16
clipboard_proc:
	dc.l 0

	move.w #$FF38,d0
	moveq #13,d1
	bsr.w getrtbase
	jsr (a0)
	tst.l d0
	beq.w clipdie
	move.l d0,a5
	move.l CLIP_EXEC(a5),a6

	sub.l a1,a1
	jsr -294(a6) ; FindTask
	move.l d0,CLIP_TASK(a5)

	lea doslibname(pc),a1
	moveq #0,d0
	jsr -$0228(a6) ; OpenLibrary
	move.l d0,CLIP_DOS(a5)
	beq.w clipdie
	move.l d0,a6

.devsloop
	moveq #50,d1
	jsr -$00c6(a6) ;Delay
	lea devs_name(pc),a0
	move.l a0,d1
	moveq #-2,d2
	jsr -$0054(a6) ;Lock
	tst.l d0
	beq.s .devsloop
	move.l d0,d1
	jsr -$005a(a6) ;Unlock
	moveq #50,d1
	jsr -$00c6(a6) ;Delay
	lea clip_name(pc),a0
	move.l a0,d1
	moveq #-2,d2
	jsr -$0054(a6) ;Lock
	tst.l d0
	beq.w clipdie
	move.l d0,d1
	jsr -$005a(a6) ;Unlock
	
	move.l CLIP_EXEC(a5),a6

	bsr.w createport
	moveq #0,d1
	move.w #52,d1
	bsr.w createio
	move.l d0,a4
	tst.l d0
	beq.w clipdie	

cfloop2
	moveq #0,d0
	bset #13,d0
	jsr -$013e(a6) ;Wait
	
	moveq #0,d1
	move.l CLIP_UNIT(a5),d0
	lea clip_dev(pc),a0
	move.l a4,a1
	jsr -$01bc(a6) ;OpenDevice
	tst.l d0
	bne.s cfloop2
	move.l 20(a4),a0 ;device node
	cmp.w #37,20(a0) ;must be at least v37
	bcc.s cfversion
	;too lazy to free everything..
	bsr.w clipkill
cfloop3
	moveq #0,d0
	jsr -$013e(a6) ;Wait
	bra.s cfloop3
	
cfversion
	bsr.w prefshook

	lea CLIP_HOOK(a5),a0
	move.l a0,40(a4)
	moveq #1,d0
	move.l d0,36(a4)
	move.w #12,28(a4) ;CBD_CHANGEHOOK
	move.l a5,CLIP_HOOK+16(a5)
	lea cliphook(pc),a0
	move.l a0,CLIP_HOOK+8(a5)
	move.l a4,a1
	jsr -$01c8(a6) ;DoIO

	move.w #$FF38,d0
	moveq #15,d1
	bsr.w getrtbase
	jsr (a0)
	tst.l CLIP_WRITE_SIZE(a5)
	bne.s clipsignal

cfloop
	moveq #0,d0
	moveq #0,d2
	move.b CLIP_POINTER_NOTIFY+20(a5),d2
	bset d2,d0
	bset #13,d0
	jsr -$013e(a6) ;Wait
	btst d2,d0
	beq.s clipsignal
	bsr.w prefsread
	bra.s cfloop

clipsignal
	move.l CLIP_WRITE_SIZE(a5),d0
	beq.w clipread
	;allocate amiga-side space
	moveq #1,d1
	jsr AllocMem(a6)
	move.l d0,CLIP_WRITE_ALLOC(a5)
	;and notify host-side
	move.w #$FF38,d0
	moveq #12,d1
	bsr.w getrtbase
	jsr (a0)
	tst.l d0
	beq.s .nowrite
	; and now we should have the data in CLIP_WRITE_ALLOC
	tst.l CLIP_WRITE_ALLOC(a5)
	beq.s .nowrite

	move.w #3,28(a4) ;CMD_WRITE
	clr.b 31(a4)
	clr.l 32(a4)
	move.l CLIP_WRITE_SIZE(a5),36(a4)
	move.l CLIP_WRITE_ALLOC(a5),40(a4)
	clr.l 44(a4)
	clr.l 48(a4)
	move.l a4,a1
	jsr -$01c8(a6) ;DoIO
	move.l 48(a4),CLIP_ID(a5)
	move.w #4,28(a4) ;CMD_UPDATE
	move.l a4,a1
	jsr -$01c8(a6) ;DoIO

.nowrite
	move.l CLIP_WRITE_SIZE(a5),d0
	clr.l CLIP_WRITE_SIZE(a5)
	move.l CLIP_WRITE_ALLOC(a5),d1
	beq.w cfloop
	move.l d1,a1
	jsr FreeMem(a6)
	bra.w cfloop

clipread:
  ; read first 8 bytes	
	move.w #2,28(a4) ;CMD_READ
	lea CLIP_BUF(a5),a0
	clr.l (a0)
	clr.l 4(a0)
	clr.b 31(a4)
	clr.l 44(a4)
	clr.l 48(a4)
	move.l a0,40(a4)
	moveq #8,d0
	move.l d0,36(a4)
	move.l a4,a1
	jsr -$01c8(a6) ;DoIO
	cmp.l #'FORM',CLIP_BUF(a5)
	bne.s .cf1
	move.l CLIP_BUF+4(a5),d0
	beq.s .cf1
	bmi.s .cf1
	move.l 48(a4),CLIP_ID(a5)
	addq.l #8,d0
	move.l d0,d2
	moveq #1,d1
	jsr AllocMem(a6)
	tst.l d0
	beq.s .cf1
	move.l d0,a2
	; read the rest
	move.l a2,a0
	move.l CLIP_BUF(a5),(a0)+
	move.l CLIP_BUF+4(a5),(a0)+
	move.l a0,40(a4)
	move.l d2,d0
	subq.l #8,d0
	move.l d0,36(a4)
	move.l a4,a1
	jsr -$01c8(a6) ;DoIO
	move.w #$FF38,d0
	moveq #11,d1
	bsr.w getrtbase
	move.l 32(a4),d0
	jsr (a0)
	move.l a2,a1
	move.l d2,d0
	jsr FreeMem(a6)
.cf1
	; tell clipboard.device that we are done (read until io_Actual==0)
	tst.l 32(a4)
	beq.w cfloop
	lea CLIP_BUF(a5),a0
	move.l a0,40(a4)
	moveq #1,d0
	move.l d0,36(a4)
	clr.l 32(a4)
	move.l a4,a1
	jsr -$01c8(a6) ;DoIO
	bra.s .cf1

cliphook:
	lea -CLIP_HOOK(a0),a0
	move.l 8(a1),d0
	cmp.l CLIP_ID(a0),d0 ;ClipHookMsg->chm_ClipID
	beq.s .same
	move.l d0,CLIP_ID(a0)
	move.l a6,-(sp)
	move.l CLIP_EXEC(a0),a6
	move.l CLIP_TASK(a0),a1
	moveq #0,d0
	bset #13,d0 ;SIG_D
	jsr -$0144(a6) ;Signal
	move.l (sp)+,a6
.same
	moveq #0,d0
	rts

consolehook:
	move.l 4.w,a6

	moveq #-1,d2
	move.w #$FF38,d0
	moveq #17,d1
	bsr.w getrtbase
	jsr (a0)
	btst #1,d0
	beq.s .ch2

	moveq #0,d2
	jsr -$0084(a6) ;Forbid
	lea 350(a6),a0 ;DeviceList
	lea con_dev(pc),a1
	jsr -$114(a6) ;FindName
	tst.l d0
	beq.s .ch1
	move.l d0,a0
	lea chook(pc),a1
	move.l -$1e+2(a0),a2 ; BeginIO
	move.l a1,-$1e+2(a0)
	move.l a0,a1
	move.w #$FF38,d0
	moveq #101,d1
	bsr.w getrtbase
	jsr (a0)
	moveq #1,d2
.ch1
	jsr -$008a(a6) ;Permit
.ch2
	move.l d2,d0
	rts	
	
chook:
	subq.l #4,sp ; native code fills with original return address
	movem.l d0-d1/a0,-(sp)
	move.w #$FF38,d0
	moveq #102,d1
	bsr.w getrtbase
	jsr (a0)
	movem.l (sp)+,d0-d1/a0
	rts

getrtbase:
	lea start-8-4(pc),a0
	and.l #$FFFF,d0
	add.l d0,a0
	rts

inp_dev: dc.b 'input.device',0
tim_dev: dc.b 'timer.device',0
con_dev: dc.b 'console.device',0
devsn_name: dc.b 'DEVS',0
devs_name: dc.b 'DEVS:',0
clip_name: dc.b 'DEVS:clipboard.device',0
ram_name: dc.b 'RAM:',0
clip_dev: dc.b 'clipboard.device',0
 ;argghh but StartNotify()ing non-existing ENV: causes "Insert disk ENV: in any drive" dialog..
pointer_prefs: dc.b 'RAM:Env/Sys/Pointer.prefs',0
clname: dc.b 'UAE clipboard sharing',0
mhname: dc.b 'UAE mouse driver',0
kaname: dc.b 'UAE heart beat',0
exter_name: dc.b 'UAE filesystem',0
fstaskname: dc.b 'UAE fs automounter',0
fsprocname: dc.b 'UAE fs automount process',0
doslibname: dc.b 'dos.library',0
intlibname: dc.b 'intuition.library',0
gfxlibname: dc.b 'graphics.library',0
explibname: dc.b 'expansion.library',0
fsresname: dc.b 'FileSystem.resource',0
fchipname: dc.b 'megachip memory',0
	END
