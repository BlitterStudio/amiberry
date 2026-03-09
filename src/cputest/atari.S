| cputester's machine specific support for Atari's STF/STE/TT/Falcon
|
| 2019/09	Nicolas Pomarede
| 2020/07/07	Updates for latest cputester version
| 2020/07/16	Disable second MFP when machine is a TT


	.text

	| Color register used to show activity during the tests (changing background color)
	.equ ACTIVITYREG,0xff8240
	| Video counter register used to measure number of cycles after calling 'sync'
	| Not used for now, cycle counting is only in Amiga mode
	| On ST this requires supervisor mode, so we use a dummy address for now as we don't use 'sync' 
	.equ CYCLEREG,0x10000		| 0xff8209

	.globl _allocate_absolute
	.globl _free_absolute
	.globl _touser
	.globl _tosuper
	.globl _testexit
	.globl _get_cpu_model
	.globl sync

	| sync with video counter to start opcode test at a specific position
	| and to measure number of cycles taken by each tested opcode
	| (not used in atari version for now)
sync:
	rts

	| check "space" key is pressed
_testexit:
	moveq	#0,d0
	cmp.b	#0x39,0xfffc02
	bne.s	.done
	moveq	#1,d0
.done:
	| d0 != 0: -> exit
	rts

	| super mode, disable MFP interrupts
	| save hbl and vbl vectors and set them to a 'RTE'
_tosuper:
 	move.l	#0,-(sp)
	move.w	#0x20,-(sp)
	trap	#1
	addq.l	#6,sp
	move.l	d0,old_ssp

	move.l	0x68,save_hbl
	move.l	0x70,save_vbl
	move.l	#int_rte,0x68
	move.l	#int_rte,0x70

	move.b	0xfffa13,save_imra
	move.b	0xfffa15,save_imrb
	clr.b	0xfffa13		| disable level 6 MFP interrupts
	clr.b	0xfffa15
	cmp.w	#2,machine_level.l	| use absolute addr because cmp #x,pc is not supported on 68000
	bne.s	.not_tt_save
	move.b	0xfffa93,save_imra_tt
	move.b	0xfffa95,save_imrb_tt
	clr.b	0xfffa93		| disable level 6 TT MFP interrupts
	clr.b	0xfffa95
.not_tt_save:
	move.w	0xff8240,save_color0

	move.w	#0x2700,sr		| disable HBL/VBL
	rts

	| back to user mode, enable MFP interrupts
	| and restore hbl/vbl vectors
	| keep the value of A7/SSP into USP
_touser:
	move.w	save_color0,0xff8240

	move.b	save_imra,0xfffa13
	move.b	save_imrb,0xfffa15
	cmp.w	#2,machine_level.l	| use absolute addr because cmp #x,pc is not supported on 68000
	bne.s	.not_tt_restore
	move.b	save_imra_tt,0xfffa93
	move.b	save_imrb_tt,0xfffa95
.not_tt_restore:
	move.l	save_hbl,0x68
	move.l	save_vbl,0x70

	move.w	#0x2300,sr		| enable VBL/MFP interrupts

	move.l	old_ssp(pc),-(sp)
	move.w	#0x20,-(sp)
        | we must keep the same value of A7 when going back to user mode
	| else the main C program will crash later
	move.l a7,usp
	trap	#1
	addq.l	#6,sp
	rts

	| free absolute allocated test memory
	| -> not supported for now
_free_absolute:
	rts

	| allocate absolute memory
	| -> not supported by TOS, assume memory can always be used
	| (requires 4 MB RAM)
	| in  : 4(sp) absolute memory address to allocate
	| in  : 8(sp) size of absolute memory address to allocate
	| out : d0=absolute memory address if success, or 0 if error
_allocate_absolute:
	move.l	4(sp),d0		| success
	rts

	| return CPU model (68000=0, 68010=1, 68020=2, 68030=3, 68040=4, 68060=5)
	| we also read machine model (0=STF 1=STE 2=TT 3=Falcon) for HW init
	| we read TOS' cookie jar at $5A0, or return 0 if CPU model is not found
_get_cpu_model:
	move.l	#0,-(sp)
	move.w	#0x20,-(sp)
	trap	#1
	addq.l	#6,sp
	move.l	d0,old_ssp

	moveq	#0,d0			| CPU defaults to 68000
	moveq	#0,d1			| machine defaults to STF
	move.l	0x5a0,d2
	beq	.all_done		| no cookie jar, assume 68000 STF

	move.l	d2,a0
.loop_cookie:
	move.l	(a0),d2
	beq.s	.all_done		| end of cookie list
	cmp.l	#0x5f435055,d2		| '_CPU'
	beq.s	.cpu_found
	cmp.l	#0x5f4d4348,d2		| '_MCH'
	beq.s	.machine_found
.next_cookie:
	addq.w	#8,a0			| next cookie
	bra.s	.loop_cookie

.cpu_found:
	move.l	4(a0),d0		| _CPU value : 0, 10, 20, 30, 40 or 60
	divu	#10,d0
	ext.l	d0
	cmp.w	#6,d0
	bne.s	.next_cookie
	moveq	#5,d0			| 68060 -> d0=5
	bra.s	.next_cookie

.machine_found:
	move.l	4(a0),d1		| _MCH value for upper word : 0=STF 1=STE 2=TT 3=Falcon
	bra.s	.next_cookie

.all_done:
	move.l	d0,cpu_level
	move.l	d1,machine_level

	move.l	old_ssp(pc),-(sp)
	move.w	#0x20,-(sp)
	trap	#1
	addq.l	#6,sp

	move.l	cpu_level,d0
	rts


	| empty interrupt function to replace TOS' HBL/VBL
int_rte:
	rte


old_ssp:
	dc.l	0
save_hbl:
	dc.l	0
save_vbl:
	dc.l	0
save_imra:
	dc.b	0
save_imrb:
	dc.b	0
save_imra_tt:
	dc.b	0
save_imrb_tt:
	dc.b	0
save_color0:
	dc.w	0
cpu_level:
	dc.l	0
machine_level:
	dc.l	0

	.include "asm.S"

