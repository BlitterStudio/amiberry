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
; 2007.09.05 fully filesystem device mounting on the fly (TW)
; 2008.01.09 ACTION_EXAMINE_ALL does not anymore return eac_Entries = 0 with continue (fixes some broken programs)

AllocMem = -198
FreeMem = -210

; don't forget filesys.c! */
PP_MAXSIZE = 4 * 96
PP_FSSIZE = 400
PP_FSPTR = 404
PP_FSRES = 408
PP_EXPLIB = 412
PP_FSHDSTART = 416
PP_TOTAL = (PP_FSHDSTART+140)

NOTIFY_CLASS = $40000000
NOTIFY_CODE = $1234
NRF_SEND_MESSAGE = 1
NRF_SEND_SIGNAL = 2
NRF_WAIT_REPLY = 8
NRF_NOTIFY_INITIAL = 16
NRF_MAGIC = $80000000

	dc.l 16
our_seglist:
	dc.l 0 ;/* NextSeg */
start:
	bra filesys_mainloop
	dc.l make_dev-start
	dc.l filesys_init-start
	dc.l exter_server-start
	dc.l bootcode-start
	dc.l setup_exter-start

	dc.l p96vsyncfix1-start
	dc.l mousehack_x-start

bootcode:
	lea.l doslibname(pc),a1
	jsr -96(a6) ; FindResident
	move.l d0,a0
	move.l 22(a0),d0
	move.l d0,a0
	jsr (a0)
	rts

filesys_init:
	movem.l d0-d7/a0-a6,-(sp)
	move.l 4.w,a6
	move.w #$FFFC,d0 ; filesys base
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

	tst.l $10c(a5)
	beq.s FSIN_none

	move.l #PP_TOTAL,d0
	move.l #$10001,d1
	jsr AllocMem(a6)
	move.l d0,a3  ; param packet
	move.l a4,PP_EXPLIB(a3)
	moveq #0,d6
FSIN_init_units:
	cmp.l $10c(a5),d6
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
	bra.b  FSIN_init_units
FSIN_units_ok:
	move.l 4.w,a6
	move.l a3,a1
	move.l #PP_TOTAL,d0
	jsr FreeMem(a6)

FSIN_none:
	move.l 4.w,a6
	move.l a4,a1
	jsr -414(a6) ; CloseLibrary

;	move.w #$FF80,d0
;	bsr.w getrtbase
;	jsr (a0)
;	jsr -$0078(a6) ; Disable
;	lea 322(a6),a0 ; MemHeader
;FSIN_scanchip:
;	move.l (a0),a0	; first MemList
;	tst.l (a0)
;	beq.s FSIN_scandone
;	move.w 14(a0),d1 ; attributes
;	and #2,d1 ; MEMF_CHIP?
;	beq.s FSIN_scanchip
;	sub.l 24(a0),d0 ; did KS detect all chip?
;	bmi.s FSIN_scandone
;	beq.s FSIN_scandone
;	; add the missing piece
;	add.l d0,24(a0) ; mh_Upper
;	add.l d0,28(a0) ; mh_Free
;	add.l d0,62(a6) ; MaxLocMem
;	; we still need to update last node's free space
;	move.l 16(a0),a0 ; mh_First
;FSIN_chiploop2
;	tst.l (a0)
;	beq.s FSIN_chiploop
;	move.l (a0),a0
;	bra.s FSIN_chiploop2
;FSIN_chiploop:
;	add.l d0,4(a0)
;FSIN_scandone:
;	jsr -$007e(a6) ; Enable


filesys_dev_storeinfo	; add >2MB chip RAM to memory list
	move.w #$FF80,d0
	bsr.w getrtbase
	jsr (a0)
	moveq.l #3,d1
	moveq.l #-10,d2
	move.l #$200000,a0
	sub.l a0,d0
	bcs.b FSIN_chip_done
	beq.b FSIN_chip_done
	moveq.l #0,d4
	move.l d4,a1
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

	movem.l (sp)+,d0-d7/a0-a6
general_ret:
	rts

	; this is getting ridiculous but I don't see any other solutions..
fsmounttask
.fsm1	move.l 4.w,a6
	moveq #0,d0
	bset #17,d0 ; SIGBREAK_CTRL_D
	jsr -$013e(a6) ;Wait
	lea doslibname(pc),a1
	moveq #0,d0
	jsr -$0228(a6) ; OpenLibrary
	tst.l d0
	beq.s .fsm1
	move.l d0,a6
	lea fsprocname(pc),a0
	move.l a0,d1
	moveq #15,d2
	lea mountproc(pc),a0
	move.l a0,d3
	lsr.l #2,d3
	move.l #8000,d4
	jsr -$08a(a6) ; CreateProc
	move.l a6,a1
	move.l 4.w,a6
	jsr -$019e(a6); CloseLibrary
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

setup_exter:
	movem.l d0-d1/a0-a1,-(sp)
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
	move.l d2,d0
	bne.s r17
	clr.l (a2)+ ; empty hunk
	bra.s r18
r17	addq.l #8,d0 ; size + pointer to next hunk + hunk size
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
ree	moveq #0,d7
	bra.s r0

fsres
	movem.l d1/a0-a2/a6,-(sp)
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
fsres4
	move.l a0,d0
	movem.l (sp)+,d1/a0-a2/a6
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


make_dev: ; IN: A0 param_packet, D6: unit_no, D7: b0=autoboot,b1=onthefly,b2=v36+
	; A4: expansionbase

	bsr.w fsres
	move.l d0,PP_FSRES(a0) ; pointer to FileSystem.resource
	move.l a0,-(sp)
	move.w #$FFFC,d0 ; filesys base
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
	cmp.w #1,d3
	bne.s mountalways

	; KS < V36: init regular hardfiles only if filesystem is loaded
	btst #2,d7
	bne.s mountalways ; >= 36
	btst #1,d7
	bne.w mountalways

mountalways
	; allocate memory for loaded filesystem
	move.l PP_FSSIZE(a0),d0
	beq.s nordbfs1
	bmi.s nordbfs1
	move.l a0,-(sp)
	moveq #1,d1
	move.l 4.w,a6
	jsr  AllocMem(a6)
	move.l (sp)+,a0
	move.l d0,PP_FSPTR(a0)
nordbfs1:

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
	tst.l PP_FSPTR(a1)	; filesystem?
	beq.s nordbfs2
	move.l PP_FSPTR(a1),a0
	bsr.w relocate
	movem.l d0/a0-a1,-(sp)
	move.l PP_FSSIZE(a1),d0
	move.l PP_FSPTR(a1),a1
	move.l 4.w,a6
	jsr FreeMem(a6)
	movem.l (sp)+,d0/a0-a1
	tst.l d0
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
	bne.b MKDV_doboot

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

	move.l 4.w,a6
	moveq.l #20,d0
	moveq.l #0,d1
	jsr  AllocMem(a6)
	move.l d0,a1 ; bootnode
	moveq #0,d0
	move.l d0,(a1)
	move.l d0,4(a1)
	move.w d0,14(a1)
	move.w #$1000,d0
	or.b d3,d0
	move.w d0,8(a1)
	move.l $104(a5),10(a1) ; filesys_configdev
	move.l a3,16(a1)        ; devicenode
	lea.l  74(a4),a0 ; MountList
	jsr  -270(a6) ; Enqueue()
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
.nob	moveq #-20,d0
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

	bsr.w allocinputdevice
	move.l d0,164(a3)
	bsr.w alloctimerdevice
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

	bsr.w mousehack_init
	bra.w FSML_Reply

	; We abuse some of the fields of the message we get. Offset 0 is
	; used for chaining unprocessed commands, and offset 1 is used for
	; indicating the status of a command. 0 means the command was handed
	; off to some UAE thread and did not complete yet, 1 means we didn't
	; even hand it over yet because we were afraid that might blow some
	; pipe limit, and -1 means the command was handed over and has completed
	; processing by now, so it's safe to reply to it.

FSML_loop:
	bsr.w mousehack_init

	move.l a5,a0
	jsr -372(a6) ; GetMsg
	move.l d0,a4
	tst.l d0
	bne.s .msg

	moveq #0,d0
	move.b 15(a5),d1 ;mp_SigBit
	bset d1,d0
	bset #17,d0 ; SIGBREAK_CTRL_D
	jsr -$013e(a6) ;Wait
.msg
	; disk changed?
	tst.b 172(a3)
	beq.s .nodc
	bsr.w diskchange
	clr.b 172(a3)
.nodc
	move.l a4,d0
	beq.s FSML_loop

	; notify reply?
	cmp.w #38, 18(a4)
	bne.s nonotif
	cmp.l #NOTIFY_CLASS, 20(a4)
	bne.s nonotif
	cmp.w #NOTIFY_CODE, 24(a4)
	bne.s nonotif
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
	bra.s FSML_loop
nonoti
	move.l a4,a1
	moveq #38,d0
	jsr FreeMem(a6)
	bra.w FSML_loop

nonotif
	move.l 10(a4),d3 ; ln_Name
	bne.b FSML_FromDOS

	; It's a dummy packet indicating that some queued command finished.
	move.w #$FF50,d0 ; exter_int_helper
	bsr.w getrtbase
	moveq.l #1,d0
	jsr (a0)
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
	bpl.b FSML_check_next
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
	move.w #$ff58,d0
	bsr.w getrtbase
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

getrtbase:
	lea start-8-4(pc),a0
	and.l #$FFFF,d0
	add.l d0,a0
	rts

	;p96 stuff

p96flag	dc.w 0
p96vsyncfix1
	cmp.l #34,8(sp) ; picasso_WaitVerticalSync?
	bne.s p961
	movem.l d0-d1/a0-a2/a6,-(sp)
	move.l 4.w,a6
	sub.l a1,a1
	jsr -$126(a6) ; FindTask
	move.l d0,a2
	move.l a2,a1
	moveq #-20,d0
	jsr -$12c(a6) ; SetTaskPri
	lea p96flag(pc),a0
	move.w (a0),d1
p962 cmp.w (a0),d1
	beq.s p962
	move.l a2,a1
	jsr -$12c(a6) ; SetTaskPri
	moveq #1,d1
	movem.l (sp)+,d0-d1/a0-a2/a6
	addq.l #4,sp ; return directly to caller
p961 rts


; mouse hack

newlist:
	move.l a0,(a0)
	addq.l #4,(a0)
	clr.l 4(a0)
	move.l a0,8(a0)
	rts

createport:
	movem.l d2/a2/a6,-(sp)
	move.l 4,a6
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
	move.l 4,a6
	tst.l d0
	beq.s .f
	move.l d0,a2
	moveq #48,d2
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

allocinputdevice
	movem.l a2/a6,-(sp)
	move.l 4.w,a6
	bsr.w createport
	bsr.w createio
	beq.s .f
	move.l d0,a1
	move.l d0,a2
	lea inp_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	jsr -$01bc(a6) ;OpenDevice
	move.l d0,d1
	moveq #0,d0
	tst.l d1
	bne.s .f
	move.l a2,d0
.f	tst.l d0
	movem.l (sp)+,a2/a6
	rts

alloctimerdevice
	movem.l a2/a6,-(sp)
	move.l 4.w,a6
	bsr.w createport
	bsr.w createio
	beq.s .f
	move.l d0,a2
	move.l d0,a1
	lea tim_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	jsr -$01bc(a6) ;OpenDevice	
	move.l d0,d1
	moveq #0,d0
	tst.l d1
	bne.s .f
	move.l a2,d0
.f	tst.l d0
	movem.l (sp)+,a2/a6
	rts

createtask:
	movem.l d2/d3/d4/a2/a3/a6,-(sp)
	move.l 4,a6
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

mousehack_e: dc.w 0
mousehack_x: dc.w 0
mousehack_y: dc.w 0

MH_INT = 0
MH_FOO = (MH_INT+22)
MH_IEV = (MH_FOO+16)
MH_IO = (MH_IEV+22)
MH_TM = (MH_IO+4)
MH_END = (MH_TM+4)

mousehack_init:
	move.l a0,-(sp)
	tst.b 157(a3)
	bne.s .no
	lea mousehack_e(pc),a0
	cmp.b #1,(a0)
	bne.s .no
	lea mhname(pc),a0
	lea mousehack_task(pc),a1
	moveq #5,d0
	bsr createtask
	st 157(a3)
	;tell native side that mousehack is active
	move.w #$FF38,d0
	bsr.w getrtbase
	jsr (a0)
.no	move.l (sp)+,a0
	rts

mousehack_task:
	move.l 4,a6

	moveq #-1,d0
	jsr -$014a(a6) ;AllocSignal
	moveq #0,d2
	bset d0,d2

	sub.l a1,a1
	jsr -$0126(a6) ;FindTask
	move.l d0,a4
	
	moveq #20,d0
	move.l a4,a1
	jsr -$012c(a6) ;SetTaskPri

	moveq #0,d0
	lea intlibname(pc),a1
	jsr -$0228(a6)
	move.l d0,d7

	moveq #0,d0
	move.w #MH_END,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	move.l d0,a5

	bsr.w allocinputdevice
	move.l d0,MH_IO(a5)
	beq.w .f
	bsr.w alloctimerdevice
	move.l d0,MH_TM(a5)
	beq.w .f

	lea MH_FOO(a5),a3
	move.l a4,12(a3);task
	move.l d2,8(a3) ;sigmask
	moveq #-1,d0
	move.l d0,(a3)	;mx
	move.l d0,4(a3)	;my

	lea MH_INT(a5),a1
	move.b #2,8(a1) ;NT_INTERRUPT
	move.b #5,9(a1) ;priority
	lea mhname(pc),a0
	move.l a0,10(a1)
	lea mousehackint(pc),a0
	move.l a0,18(a1)
	move.l a3,14(a1)
	moveq #5,d0 ;INTB_VERTB
	jsr -$00a8(a6)
	bra.s mhloop
.f	rts

mhloop
	move.l d2,d0
	jsr -$013e(a6) ;Wait
	
	move.l MH_IO(a5),a1
	lea MH_IEV(a5),a2
	move.w #11,28(a1) ;IND_WRITEEVENT
	move.l #22,36(a1) ;sizeof(struct InputEvent)
	move.l a2,40(a1)
	move.b #1,30(a1) ;IOF_QUICK

	move.b #4,4(a2) ;IECLASS_POINTERPOS
	clr.b 5(a2) ;ie_SubClass
	clr.w 6(a2) ;ie_Code
	clr.w 8(a2) ;ie_Qualifier

	move.l d7,a0 ;intbase

	move.l MH_FOO+0(a5),d0
	move.w 34+14(a0),d1
	add.w d1,d1
	sub.w d1,d0
	move.w d0,10(a2)

	move.l MH_FOO+4(a5),d0
	move.w 34+12(a0),d1
	add.w d1,d1
	sub.w d1,d0
	ext.l d0
	move.w d0,12(a2)

	move.l MH_TM(a5),a1
	move.w #10,28(a1) ;TR_GETSYSTIME
	move.b #1,30(a1) ;IOF_QUICK
	jsr -$01c8(a6) ;DoIO
	move.l MH_TM(a5),a1
	move.l 32(a1),14(a2)
	move.l 36(a1),18(a2)

	move.l MH_IO(a5),a1
	jsr -$01c8(a6) ;DoIO
	
	bra.w mhloop

mousehackint:
	move.w mousehack_x(pc),d0
	ext.l d0
	move.w mousehack_y(pc),d1
	ext.l d1
	cmp.l (a1),d0
	bne .l1
	cmp.l 4(a1),d1
	beq .l2
.l1 move.l d1,4(a1)
	move.l d0,(a1)
	move.l 8(a1),d0
	move.l 12(a1),a1
	move.l 4.w,a6
	jsr -$0144(a6) ; Signal
.l2	lea $dff000,a0
	moveq #0,d0
	rts

inp_dev: dc.b 'input.device',0
tim_dev: dc.b 'timer.device',0
mhname: dc.b 'UAE mouse hack',0
exter_name: dc.b 'UAE filesystem',0
fstaskname: dc.b 'UAE fs automounter',0
fsprocname: dc.b 'UAE fs automount process',0
doslibname: dc.b 'dos.library',0
intlibname: dc.b 'intuition.library',0
explibname: dc.b 'expansion.library',0
fsresname: dc.b 'FileSystem.resource',0
	END
