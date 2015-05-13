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
	moveq #0,d5
	jsr  -552(a6) ; OpenLibrary
	tst.l d0
	bne.b FSIN_explibok
	lea.l explibname(pc),a1 ; expansion lib name
	moveq #0,d0
	moveq #1,d5
	jsr  -552(a6) ; OpenLibrary
FSIN_explibok:
	move.l d0,a4
	move.l #PP_TOTAL,d0
	moveq #1,d1 ; MEMF_PUBLIC
	jsr AllocMem(a6)
	move.l d0,a3  ; param packet
	move.l a4,PP_EXPLIB(a3)

	moveq #0,d6
FSIN_init_units:
	cmp.l  $10c(a5),d6
	bcc.b FSIN_units_ok
	move.l d6,-(sp)
FSIN_nextsub:
	moveq   #1,d7
	move.l a3,-(sp)
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

	; add >2MB chip RAM to memory list
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

	movem.l (sp)+,d0-d7/a0-a6
general_ret:
	rts

exter_data:
exter_server:
	movem.l a2,-(sp)
	move.w #$FF50,d0
	bsr.w getrtbase
	moveq.l #0,d0
	jsr (a0)
	tst.l d0
	beq.w exter_server_exit
	; This is the hard part - we have to send some messages.
	move.l 4.w,a6
EXTS_loop:
	move.w #$FF50,d0 ;exter_int_helper
	bsr.w getrtbase
	moveq.l #2,d0
	jsr (a0)
	cmp.w #1,d0
	blt.b EXTS_done
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
	lsl.l #2,d2
	moveq #1,d1
	btst #30,d3 ; chip mem?
	beq.s r2
	bset #1,d1
r2	bset #16,d1
	bclr #31,d3
	bclr #30,d3
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
ree	move.w #30000,d0
re1	move.w #$f00,$dff180
	move.w #$00f,$dff180
	move.w #$0f0,$dff180
	dbf d0,re1
	moveq #0,d7
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
	
make_dev: ; IN: A0 param_packet, D6: unit_no, D7: boot, A4: expansionbase

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
	cmp.l #-2,d3
	beq.w general_ret
	cmp.l #2,d3
	beq.s mountalways
	
	; KS < V36: init regular hardfiles only if filesystem is loaded
	and.l d5,d0
	beq.s mountalways ; >= 36
	tst.l PP_FSSIZE(a0)
	beq.w general_ret ; no filesystem -> don't mount

mountalways
	; allocate memory for loaded filesystem
	move.l PP_FSSIZE(a0),d0
	beq.s nordbfs1
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
	bra.w dont_mount

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

dont_mount
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
	move.l #4000,20(a3)     ; dn_StackSize
	lea.l our_seglist(pc),a1
	move.l a1,d0
	lsr.l  #2,d0
	move.l d0,32(a3)        ; dn_SegList
	moveq #-1,d0
	move.l d0,36(a3)       ; dn_GlobalVec

MKDV_doboot:
	tst.l d7
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
	jmp  -270(a6) ; Enqueue()

MKDV_noboot:
	move.l a3,a0
	moveq #0,d1
	move.l d1,a1
	moveq #-1,d0
	move.l a4,a6 ; expansion base
	jmp  -150(a6) ; AddDosNode

filesys_mainloop:
	move.l 4.w,a6
	moveq.l #0,d0
	move.l d0,a1
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
	; 32: the volume (80+44+1 bytes)
	; 157: mousehack started-flag
	move.l #80+44+1+20+12+1,d0
	move.l #$10001,d1 ; MEMF_PUBLIC | MEMF_CLEAR
	jsr AllocMem(a6)
	move.l d0,a3
	moveq.l #0,d6
	move.l d6,(a3)
	move.l d6,4(a3)
	move.l d6,8(a3)

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
	jsr -384(a6) ; WaitPort
	move.l a5,a0
	jsr -372(a6) ; GetMsg
	move.l d0,a4

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
	bra.s FSML_loop

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
	beq.b FSML_loop
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
	move.w #$FF30,d0
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
	move.l d2,18(a0)
	move.l a2,14(a0)
.f	movem.l (sp)+,d2/a2/a6
	rts

createtask:
	movem.l d2/d3/a2/a3/a6,-(sp)
	move.l 4,a6
	move.l a0,d2
	move.l a1,d3
	move.l #92+2048,d0
	move.l #65536+1,d1
	jsr AllocMem(a6)
	tst.l d0
	beq .f
	move.l d0,a2
	move.b #1,8(a2) ;NT_TASK
	move.l d2,10(a2)
	lea 92(a2),a3
	move.l a3,58(a2)
	lea 2048(a3),a3
	move.l a3,62(a2)
	move.l a3,54(a2)
	move.l a2,a1
	move.l d3,a2
	sub.l a3,a3
	jsr -$011a(a6) ;AddTask
.f	movem.l (sp)+,d2/d3/a2/a3/a6
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
	tst.w (a0)
	beq.s .no
	lea mhname(pc),a0
	lea mousehack_task(pc),a1
	bsr createtask
	st 157(a3)
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

	bsr.w createport
	bsr.w createio
	move.l d0,MH_IO(a5)
	beq .f
	move.l d0,a1
	lea inp_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	jsr -$01bc(a6) ;OpenDevice
	tst.l d0
	bne .f

	bsr.w createport
	bsr.w createio
	move.l d0,MH_TM(a5)
	beq .f
	move.l d0,a1
	lea tim_dev(pc),a0
	moveq #0,d0
	moveq #0,d1
	jsr -$01bc(a6) ;OpenDevice	
	tst.l d0
	bne .f

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
.l2 lea $dff000,a0
	moveq #0,d0
	rts

inp_dev: dc.b 'input.device',0
tim_dev: dc.b 'timer.device',0
mhname: dc.b 'UAE mouse hack',0
exter_name: dc.b 'UAE filesystem',0
doslibname: dc.b 'dos.library',0
intlibname: dc.b 'intuition.library',0
explibname: dc.b 'expansion.library',0
fsresname: dc.b 'FileSystem.resource',0
	END
