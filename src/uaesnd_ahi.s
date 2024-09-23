
	; Amiga-side full featured AHI driver for WinUAE 4.3.1+ UAESND sound board.
	; by Toni Wilen (c) 2019-2020

	; License: GPL version 2 or any later version.
	
	incdir	sys:programming/asm/includes/

	include	exec/exec.i
	include exec/exec_lib.i
	include exec/libraries.i
	include exec/ports.i
	include	devices/ahi.i
	include	libraries/ahi_sub.i
	include libraries/configregs.i
	include libraries/configvars.i
	include utility/hooks.i
	include hardware/intbits.i
	include	utility/utility.i
	include	utility/hooks.i

DEBUG EQU 1
DEBUG_ADDR EQU $bfff00

VERSION EQU 4
REVISION EQU 1
VERSION_STRUCT EQU 1

call MACRO
	jsr	_LVO\1(a6)
	ENDM

	STRUCTURE UAESNDHW,$80
	ULONG base_uae_version
	UWORD base_snd_version
	UWORD base_snd_revision
	ULONG base_frequency
	UBYTE base_max_channels
	UBYTE base_max_actual_channels
	UBYTE base_max_streams
	UBYTE base_pad1
	ULONG base_ram_offset
	ULONG base_ram_size
	STRUCT base_reserved1,$48
	ULONG base_stream_allocate
	ULONG base_stream_latch
	ULONG base_stream_unlatch
	ULONG base_pad2
	ULONG base_stream_enable
	ULONG base_stream_intena
	ULONG base_stream_intreq

	STRUCTURE UAESNDSet,0
	UWORD set_flags
	UWORD set_format
	APTR set_pointer
	ULONG set_length
	ULONG set_frequency
	APTR set_repeat_pointer
	ULONG set_repeat_length
	UWORD set_repeat_count
	UWORD set_volume
	APTR set_next_pointer
	UBYTE set_channels
	UBYTE set_type
	UBYTE set_intena
	UBYTE set_mask
	UWORD set_hpan
	UWORD set_dpan
	APTR set_pointer_original
	ULONG set_length_original
	; extra
	ULONG set_offset
	UWORD set_sample_size
	LABEL UAESNDSet_SIZEOF	

STREAM_START = $100
STREAM_OFFSET = $100

	STRUCTURE UAESNDStream,0
	STRUCT UAESNDSetCurrent,$40
	STRUCT UAESNDSetLatched,$40
	APTR stream_sample_pointer
	UBYTE stream_pad01
	UBYTE stream_pad02
	UBYTE stream_pad03
	UBYTE stream_intreq
	UBYTE stream_pad04
	UBYTE stream_pad05
	UBYTE stream_pad06
	UBYTE stream_alt_intreq
	UBYTE stream_pad07
	UBYTE stream_pad08
	UBYTE stream_pad09
	UBYTE stream_master_intena
	UBYTE stream_pad10
	UBYTE stream_pad11
	UBYTE stream_pad12
	UBYTE stream_status
	ULONG stream_timer1_frequency
	ULONG stream_pad13
	ULONG stream_pad14
	APTR stream_sample_pointer_imm

	STRUCTURE sound,0
	APTR	so_Address
	ULONG	so_Length
	UWORD so_Type
	UWORD so_Samplesize
	LABEL	sound_SIZEOF

	STRUCTURE channel,0
	APTR ch_set_current
	STRUCT ch_UAESNDSet1,UAESNDSet_SIZEOF
	STRUCT ch_SndMsg,AHISoundMessage_SIZEOF
	LABEL	channel_SIZEOF

	STRUCTURE UAESNDBase,LIB_SIZE
	UBYTE	ub_Flags
	UBYTE	ub_Pad1
	UWORD	ub_Pad2
	APTR	ub_SysLib
	ULONG	ub_SegList
	ULONG	ub_Base
	APTR	ub_ConfigDev
	APTR	ub_UtilBase
	LABEL	UAESNDBase_SIZEOF

	STRUCTURE UAESND,0
	ULONG	p_Base
	APTR p_UAESNDBase
	UWORD p_DisableCount
	BOOL p_intadded
	APTR p_AudioCtrl
	ULONG p_SoundSize
	APTR	p_Sounds
	APTR p_Channels
	ULONG p_ChannelSize
	ULONG p_DriverDataSize
	APTR p_ChannelInfo
	UWORD p_StreamCnt
	UWORD p_Pad1
	STRUCT p_Interrupt,IS_SIZE
	LABEL UAESND_SIZEOF	

Start:
	moveq	#-1,d0
	rts

RomTag:
	DC.W RTC_MATCHWORD
	DC.L RomTag
	DC.L EndCode
	DC.B RTF_AUTOINIT
	DC.B 4				;version
	DC.B NT_LIBRARY
	DC.B 0				;pri
	DC.L LibName
	DC.L IDString
	DC.L InitTable

LibName:
	dc.b "uaesnd.audio",0
	dc.b "$VER:"
IDString:
	dc.b "uaesnd.audio 4.1 (11.04.2020)",0
	cnop 0,4

InitTable:
	DC.L UAESNDBase_SIZEOF
	DC.L funcTable
	DC.L dataTable
	DC.L initRoutine

funcTable:
	dc.l Open
	dc.l Close
	dc.l Expunge
	dc.l Null

	dc.l AHIsub_AllocAudio
	dc.l AHIsub_FreeAudio
	dc.l AHIsub_Disable
	dc.l AHIsub_Enable
	dc.l AHIsub_Start
	dc.l AHIsub_Update
	dc.l AHIsub_Stop
	dc.l AHIsub_SetVol
	dc.l AHIsub_SetFreq
	dc.l AHIsub_SetSound
	dc.l AHIsub_SetEffect
	dc.l AHIsub_LoadSound
	dc.l AHIsub_UnloadSound
	dc.l AHIsub_GetAttr
	dc.l AHIsub_HardwareControl
	dc.l -1

dataTable:
	INITBYTE LN_TYPE,NT_LIBRARY
	INITLONG LN_NAME,LibName
	INITBYTE LIB_FLAGS,LIBF_SUMUSED|LIBF_CHANGED
	INITWORD LIB_VERSION,VERSION
	INITWORD LIB_REVISION,REVISION
	INITLONG LIB_IDSTRING,IDString
	DC.L		0

	IFD DEBUG=1
debugreturn
	move.l d0,DEBUG_ADDR
	move.l #debug_return,DEBUG_ADDR+4
	rts
debug_return
	dc.b "-> %x\n",0
	even
	ENDC

	IFD DEBUG=1
debug_setvol
	dc.b "SetVol(%u,%x,%x,%p,%x)\n",0
	even
	ENDC

	; UWORD channel D0
	; Fixed volume D1
	; sposition pan D2
	; struct AHIAudioCtrlDrv* A2
	; ULONG Flags D3

AHIsub_SetVol
	movem.l d2/a2,-(sp)

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.w d0,(a0)
	move.l d1,(a0)
	move.l d2,(a0)
	move.l a2,(a0)
	move.l d3,(a0)
	move.l #debug_setvol,4(a0)
	ENDC

	move.l ahiac_DriverData(a2),a2

	move.w d0,d2
	mulu #channel_SIZEOF,d0
	move.l p_Channels(a2),a1
	add.l d0,a1
	asr.l #1,d1
	bpl.s .pos
	neg.l d1
.pos
	cmp.l #32767,d1
	bcs.s .large
	move.w #32767,d1
.large
	move.l ch_set_current(a1),a0
	move.w d1,set_volume(a0)

	btst #AHISB_IMM,d3
	beq.s .noimm

	move.l p_Base(a2),a0
	mulu #STREAM_OFFSET,d2
	add.l d2,a0
	move.w d1,STREAM_START+UAESNDSetCurrent+set_volume(a0)

.noimm
	movem.l (sp)+,d2/a2
	moveq #0,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	rts

	IFD DEBUG=1
debug_setfreq
	dc.b "SetFreq(%u,%x,%p,%x)\n",0
	even
	ENDC

	; UWORD channel D0
	; ULONG freq D1
	; struct AHIAudioCtrlDrv* A2
	; ULONG Flags D2

AHIsub_SetFreq
	movem.l d3/a2,-(sp)

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.w d0,(a0)
	move.l d1,(a0)
	move.l d2,(a0)
	move.l d2,(a0)
	move.l #debug_setfreq,4(a0)
	ENDC

	move.l ahiac_DriverData(a2),a2

	move.w d0,d3
	mulu #channel_SIZEOF,d0
	move.l p_Channels(a2),a1
	add.l d0,a1
	asl.l #8,d1

	move.l ch_set_current(a1),a0
	move.l d1,set_frequency(a0)

	btst #AHISB_IMM,d2
	beq.s .noimm

	move.l p_Base(a2),a0
	mulu #STREAM_OFFSET,d3
	add.l d3,a0
	move.l d1,STREAM_START+UAESNDSetCurrent+set_frequency(a0)

.noimm
	movem.l (sp)+,d3/a2
	moveq #0,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	rts
	
	IFD DEBUG=1
debug_setsound
	dc.b "SetSound(%u,%u,%x,%d,%p,%x)\n",0
	even
	ENDC
	
	; UWORD channel D0
	; UWORD sound D1
	; ULONG offset D2
	; LONG length D3
	; struct AHIAudioCtrlDrv* A2
	; ULONG Flags D4
	
AHIsub_SetSound
	movem.l d2-d5/a2-a5,-(sp)

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.w d0,(a0)
	move.w d1,(a0)
	move.l d2,(a0)
	move.l d3,(a0)
	move.l a2,(a0)
	move.l d4,(a0)
	move.l #debug_setsound,4(a0)
	ENDC

	move.l ahiac_DriverData(a2),a3

	move.w d0,d5
	mulu #channel_SIZEOF,d0
	move.l p_Channels(a3),a1
	add.l d0,a1
	move.l ch_set_current(a1),a4

	move.l p_Base(a3),a5
	add.w #STREAM_START,a5
	mulu #STREAM_OFFSET,d5
	add.l d5,a5

	cmp.w #AHI_NOSOUND,d1
	bne.s .sound_ok
	moveq #0,d2
	moveq #0,d3
	lea empty_sound(pc),a2
	bra.s .update_channel

.sound_ok
	move.w d1,d0
	mulu #sound_SIZEOF,d0
	move.l p_Sounds(a3),a2
	add.l d0,a2
	tst.l d3
	bne.s .length_ok
	move.l so_Length(a2),d3
.length_ok

.update_channel

	move.b so_Type+0(a2),set_channels(a4)
	move.b so_Type+1(a2),set_type(a4)
	moveq #0,d0
	move.w so_Samplesize(a2),d0
	move.w d0,set_sample_size(a4)
	mulu.l d0,d2
	move.l d2,set_offset(a4)
	move.l so_Address(a2),d0
	move.l d0,set_repeat_pointer(a4)
	add.l  d2,d0
	move.l d0,set_pointer(a4)
	move.l d3,set_length(a4)
	move.w #-1,set_repeat_count(a4)
	move.b #3,set_mask(a4)
	move.l a4,set_next_pointer(a4)

	bset #0,stream_master_intena(a5)

	btst #AHISB_IMM,d4
	bne.s .doimm

	; tell the sound hardware that next set is ready
	; to play when previous one has finished (or has
	; already started looping)

	move.b #1,stream_status(a5)
	bra.s .noimm

.doimm

	; start stream immediately
	move.l a4,stream_sample_pointer_imm(a5)

.noimm

	movem.l (sp)+,d2-d5/a2-a5
	moveq #0,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	rts

empty_sound
	dc.l 0,0
	dc.w 0,0

	IFD DEBUG=1
debug_seteffect
	dc.b "SetEffect(%p,%p)\n",0
	even
	ENDC

	; Effect* A0
	; struct AHIAudioCtrlDrv* A2

AHIsub_SetEffect
	move.l a3,-(sp)

	IFD DEBUG=1
	lea DEBUG_ADDR,a1
	move.l a0,(a1)
	move.l a2,(a1)
	move.l #debug_seteffect,4(a1)
	ENDC

	moveq #AHIE_OK,d1
	move.l ahiac_DriverData(a2),a3
	move.l ahie_Effect(a0),d0
	cmp.l #AHIET_MASTERVOLUME,d0
	bne.s .nmastervolume
	nop
	bra.s .end	
.nmastervolume
	cmp.l #AHIET_CANCEL|AHIET_MASTERVOLUME,d0
	bne.s .nmastervolume_off
	nop
	bra.s .end
.nmastervolume_off
	cmp.l #AHIET_CHANNELINFO,d0
	bne.s .nchannelinfo
	move.l a0,p_ChannelInfo(a3)
	bra.s .end
.nchannelinfo
	cmp.l #AHIET_CANCEL|AHIET_CHANNELINFO,d0
	bne.s .nchannelinfo_off
	clr.l p_ChannelInfo(a3)
	bra.s .end
.nchannelinfo_off
	moveq	#AHIE_UNKNOWN,d1
.end
	move.l (sp)+,a3
	move.l d1,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	rts

update_playerfunc

	movem.l a2/a3,-(sp)
	move.l a0,a3
	move.l ahiac_DriverData(a3),a2
	move.l p_Base(a2),a0
	move.l ahiac_PlayerFreq(a3),d0
	cmp.l #$10000,d0
	bcs.s .notfixed
	lsr.l #8,d0
.notfixed
	bset.b #4,STREAM_START+stream_master_intena(a0)
	move.l d0,STREAM_START+stream_timer1_frequency(a0)
	movem.l (sp)+,a2/a3
	rts

	; ULONG Flags D0
	; struct AHIAudioCtrlDrv* A2

AHIsub_Update
	
	move.l a3,-(sp)

	move.l ahiac_DriverData(a2),a3
	move.l p_Base(a3),a0
	bsr.w uaesnd_disable
	
	move.l a2,a0
	bsr.w update_playerfunc

	move.l p_Base(a3),a0
	bsr.w uaesnd_enable
	
	move.l (sp)+,a3
	rts

	IFD DEBUG=1
debug_loadsound
	dc.b "LoadSound(%u,%x,%p,%p)\n",0
	even
	ENDC
	
	; UWORD Sound D0
	; ULONG Type D1
	; APTR Info A0
	; struct AHIAudioCtrlDrv* A2

AHIsub_LoadSound
	movem.l d2/d5/a2/a3,-(sp)
	
	IFD DEBUG=1
	lea DEBUG_ADDR,a1
	move.w d0,(a1)
	move.l d1,(a1)
	move.l a0,(a1)
	move.l a2,(a1)
	move.l #debug_loadsound,4(a1)
	ENDC

	move.l ahiac_DriverData(a2),a3

	move.l p_Sounds(a3),a2
	mulu #sound_SIZEOF,d0
	add.l d0,a2
	
	moveq #AHIE_BADSOUNDTYPE,d5
	cmp.w #AHIST_SAMPLE,d1
	beq.s .sample
	cmp.w #AHIST_DYNAMICSAMPLE,d1
	bne.s .exit
.sample
	moveq #AHIE_BADSAMPLETYPE,d5
	move.l ahisi_Type(a0),d0
	lea sampletypes(pc),a1
	moveq #-1,d1
.next
	move.l (a1),d2
	cmp.l d2,d1
	beq.s .exit
	cmp.l d2,d0
	beq.s .samplefound
	addq.l #4+2+2,a1
	bra.s .next
.samplefound

	move.w 4(a1),so_Type(a2)
	move.w 6(a1),so_Samplesize(a2)
	move.l ahisi_Address(a0),so_Address(a2)
	move.l ahisi_Length(a0),so_Length(a2)
	moveq #AHIE_OK,d5
.exit
	move.l d5,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	movem.l (sp)+,d2/d5/a2/a3
	rts

	IFD DEBUG=1
debug_unloadsound
	dc.b "UnloadSound(%u,%p)\n",0
	even
	ENDC

	; UWORD Sound D0
	; struct AHIAudioCtrlDrv* A2

AHIsub_UnloadSound
	
	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.w d0,(a0)
	move.l a2,(a0)
	move.l #debug_unloadsound,4(a0)
	ENDC

	move.l ahiac_DriverData(a2),a0
	move.l p_Sounds(a0),a0
	mulu #sound_SIZEOF,d0
	add.l d0,a0
	clr.w so_Type(a0)
	moveq #0,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	rts

sampletypes
	dc.l AHIST_M8S
	dc.b 1, 0, 0, 1
	dc.l AHIST_S8S
	dc.b 2, 0, 0, 2
	dc.l AHIST_M16S
	dc.b 1, 1, 0, 2
	dc.l AHIST_S16S
	dc.b 2, 1, 0, 4
	dc.l AHIST_M32S
	dc.b 1, 3, 0, 4
	dc.l AHIST_S32S
	dc.b 2, 3, 0, 8
	dc.l AHIST_L7_1
	dc.b 8, 3, 0, 32
	dc.l -1

uaesnd_enable
	move.l base_stream_enable(a0),base_stream_intena(a0)
	rts

uaesnd_disable
	clr.l base_stream_intena(a0)
	rts

	; struct AHIAudioCtrlDrv* A2

AHIsub_Disable
	move.l ahiac_DriverData(a2),a0
	addq.w #1,p_DisableCount(a0)
	move.l p_Base(a0),a0
	bsr uaesnd_disable
	rts

	; struct AHIAudioCtrlDrv* A2

AHIsub_Enable
	move.l ahiac_DriverData(a2),a0
	subq.w	#1,p_DisableCount(a0)
	bne.s .exit
	move.l p_Base(a0),a0
	bsr uaesnd_enable
.exit
	rts

reset_ch
	move.l p_ChannelSize(a0),d0
	beq.s .res0
	move.l p_Channels(a0),a0
	moveq #channel_SIZEOF,d1
.res1
	clr.l ch_UAESNDSet1+set_next_pointer(a0)
	sub.l d1,d0
	add.l d1,a0
	bne.s .res1
.res0
	rts

reset_hw
	clr.l base_stream_intena(a0)
	clr.l base_stream_enable(a0)
	clr.l STREAM_START+stream_timer1_frequency(a0)
	rts

	IFD DEBUG=1
debug_start
	dc.b "Start(%x,%p)\n",0
	even
	ENDC
	
	; ULONG Flags D0
	; struct AHIAudioCtrlDrv* A2

AHIsub_Start

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.l d0,(a0)
	move.l a2,(a0)
	move.l #debug_start,4(a0)
	ENDC

	move.l ahiac_DriverData(a2),a1
	move.l p_Base(a1),a0
	bsr reset_hw

	move.l a2,a0
	bsr.w update_playerfunc

	move.l ahiac_DriverData(a2),a1
	move.l p_Base(a1),a0
	moveq #1,d0
	move.b base_max_streams(a0),d1
	lsl.l d1,d0
	subq.l #1,d0
	move.l d0,base_stream_enable(a0)
	move.l d0,base_stream_intena(a0)
	moveq #AHIE_OK,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	rts	

	IFD DEBUG=1
debug_stop
	dc.b "Stop(%x,%p)\n",0
	even
	ENDC

	; ULONG Flags D0
	; struct AHIAudioCtrlDrv* A2

AHIsub_Stop:

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.l d0,(a0)
	move.l a2,(a0)
	move.l #debug_stop,4(a0)
	ENDC

	move.l ahiac_DriverData(a2),a0
	move.l p_Base(a0),a0
	bsr reset_hw

	move.l ahiac_DriverData(a2),a0
	bsr.w reset_ch

	moveq #AHIE_OK,d0
	rts

	IFD DEBUG=1
debug_allocaudio
	dc.b "AllocAudio(%p,%p)\n",0
	even
	ENDC

	; struct TagItem* A1
	; struct AHIAudioCtrlDrv* a2

AHIsub_AllocAudio
	movem.l d2-d3/d7/a3-a6,-(sp)

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.l a1,(a0)
	move.l a2,(a0)
	move.l #debug_allocaudio,4(a0)
	ENDC

	moveq #AHISF_ERROR,d7
	
	move.l a6,a5
	move.l a1,d3

	move.l ub_SysLib(a5),a6

	move.l #UAESND_SIZEOF,d0
	move.l #MEMF_PUBLIC|MEMF_CLEAR,d1
	call AllocMem
	move.l d0,ahiac_DriverData(a2)
	beq .error
	move.l d0,a3

	move.l a5,p_UAESNDBase(a3)
	move.l ub_Base(a5),a4
	moveq #0,d0
	move.b base_max_streams(a4),d0
	move.w d0,p_StreamCnt(a3)
	move.l a4,p_Base(a3)
	move.l a2,p_AudioCtrl(a3)
	
	moveq #0,d0
	move.b base_max_streams(a4),d0
	cmp.w ahiac_Channels(a2),d0
	bcs .error

	mulu #channel_SIZEOF,d0
	move.l d0,p_ChannelSize(a3)
	move.l #MEMF_PUBLIC|MEMF_CLEAR,d1
	call AllocMem
	move.l d0,p_Channels(a3)
	beq	.error

	move.l d0,a1
	move.w ahiac_Channels(a2),d1
	subq.w #1,d1
.setch
	lea ch_UAESNDSet1(a1),a0
	move.l a0,ch_set_current(a1)
	add.w #channel_SIZEOF,a1
	dbf d1,.setch

	move.w ahiac_Sounds(a2),d0
	mulu.w #sound_SIZEOF,d0
	move.l d0,p_SoundSize(a3)
	move.l #MEMF_PUBLIC|MEMF_CLEAR,d1
	call AllocMem
	move.l d0,p_Sounds(a3)
	beq	.error

	move.l a3,a0
	bsr.w reset_ch

	move.l a4,a0
	bsr.w reset_hw

	lea p_Interrupt(a3),a1
	lea interrupt_code(pc),a0
	move.l a0,IS_CODE(a1)
	lea intname(pc),a0
	move.l a0,LN_NAME(a1)
	move.b #NT_INTERRUPT,LN_TYPE(a1)
	move.b #40,LN_PRI(a1)
	move.l a3,IS_DATA(a1)
	moveq #INTB_EXTER,d0
	call AddIntServer
	st p_intadded(a3)

	move.l a4,a0
	moveq #1,d0
	move.w p_StreamCnt(a3),d1
	lsl.l d1,d0
	subq.l #1,d0
	move.l d0,base_stream_allocate(a0)

	move.l #AHISF_KNOWSTEREO|AHISF_KNOWHIFI|AHISF_KNOWMULTICHANNEL,d7

.error
	move.l d7,d0

	IFD DEBUG=1
	bsr debugreturn
	ENDC

	movem.l (sp)+,d2-d3/d7/a3-a6
	rts

	IFD DEBUG=1
debug_freeaudio
	dc.b "FreeAudio(%p)\n",0
	even
	ENDC

AHIsub_FreeAudio
	movem.l a3/a5/a6,-(sp)

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.l a2,(a0)
	move.l #debug_freeaudio,4(a0)
	ENDC

	move.l a6,a5
	move.l ub_SysLib(a5),a6
	move.l ahiac_DriverData(a2),d0
	beq.s .noaudio
	move.l d0,a3
	
	tst.b p_intadded(a3)
	beq.s .noint
	lea p_Interrupt(a3),a1
	moveq #INTB_EXTER,d0
	call RemIntServer	

	move.l p_Base(a3),a0
	bsr.w reset_hw
	clr.l base_stream_allocate(a0)
.noint
	
	move.l p_Sounds(a3),d0
	beq.s .nosounds
	move.l d0,a1
	move.l p_SoundSize(a3),d0
	call FreeMem
.nosounds

	move.l p_Channels(a3),d0
	beq.s .nochannels
	move.l d0,a1
	move.l p_ChannelSize(a3),d0
	call FreeMem
.nochannels
	
	move.l a3,a1
	move.l p_DriverDataSize(a3),d0
	call FreeMem

	clr.l ahiac_DriverData(a2)
	
.noaudio
	movem.l (sp)+,a3/a5/a6
	rts

	IFD DEBUG=1
debug_getattr
	dc.b "GetAttr(%x,%x,%x,%p,%p)\n",0
	even
	ENDC

	; ULONG Attribute D0
	; LONG Argument D1
	; LONG DefValue D2
	; struct TagItem* A1
	; struct AHIAudioCtrlDrv* A2

AHIsub_GetAttr
	move.l d2,-(sp)

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.l d0,(a0)
	move.l d1,(a0)
	move.l d2,(a0)
	move.l a1,(a0)
	move.l a2,(a0)
	move.l #debug_getattr,4(a0)
	ENDC

	cmp.l #AHIDB_Frequency,d0
	bne.s .notfreq
	lea tag_frequencies(pc),a0
	move.l 0(a0,d1.w*4),d2
	bra.s .exit
.notfreq
	cmp.l #AHIDB_Index,d0
	bne.s .notindex
	lea tag_frequencies(pc),a0
	moveq #0,d2
.freqnext
	tst.l (a0)
	beq.s .freqexit
	cmp.l (a0)+,d1
	bls.s .exit
	addq.l #1,d2
	bra.s .freqnext
.freqexit
	moveq #FREQUENCIES-1,d2
	bra.s .exit
.notindex
	lea GetAttrTags(pc),a0
.next
	tst.l (a0)
	beq.s .exit
	cmp.l (a0),d0
	addq.l #4+4,a0
	bne.s .next
	move.l -4(a0),d2
.exit
	move.l d2,d0
	
	IFD DEBUG=1
	bsr debugreturn
	ENDC
	
	move.l (sp)+,d2
	rts

tag_frequencies
	dc.l 4410	; CD/10
	dc.l 4800	; DAT/10
	dc.l 5513	; CD/8
	dc.l 6000	; DAT/8
	dc.l 7350	; CD/6
	dc.l 8000	; µ- and A-Law, DAT/6
	dc.l 9600	; DAT/5
	dc.l 11025	; CD/4
	dc.l 12000	; DAT/4
	dc.l 14700	; CD/3
	dc.l 16000	; DAT/3
	dc.l 17640	; CD/2.5
	dc.l 18900
	dc.l 19200	; DAT/2.5
	dc.l 22050	; CD/2
	dc.l 24000	; DAT/2
	dc.l 27429
	dc.l 29400	; CD/1.5
	dc.l 32000	; DAT/1.5
	dc.l 33075
	dc.l 37800
	dc.l 44100	; CD
	dc.l 48000	; DAT
	dc.l 56000
	dc.l 96000
FREQUENCIES EQU (*-tag_frequencies)>>2	
tag_frequencies_end
	dc.l 0

GetAttrTags
	dc.l AHIDB_Bits, 32
	dc.l AHIDB_Frequencies, FREQUENCIES
	dc.l AHIDB_MaxChannels, 8
	dc.l AHIDB_Author, tag_author
	dc.l AHIDB_Copyright, tag_copyright
	dc.l AHIDB_Version, IDString
	dc.l AHIDB_Record, 0
	dc.l AHIDB_Outputs, 1
	dc.l AHIDB_Output, tag_output
	dc.l AHIDB_PingPong, 1
	dc.l AHIDB_Realtime, 1
	dc.l AHIDB_MinOutputVolume, 0
	dc.l AHIDB_MaxOutputVolume, $10000
	dc.l 0

tag_author
	dc.b "Toni Wilen",0
tag_copyright
	dc.b "© 2020 Toni Wilen",0
tag_output
	dc.b "UAE",0
	even

	IFD DEBUG=1
debug_hardwarecontrol
	dc.b "HardwareControl(%x,%x,%p)\n",0
	even
	ENDC

	; ULONG Attribute D0
	; LONG Argument D1
	; struct AHIAudioCtrlDrv* A2

AHIsub_HardwareControl

	IFD DEBUG=1
	lea DEBUG_ADDR,a0
	move.l d0,(a0)
	move.l d1,(a0)
	move.l a2,(a0)
	move.l #debug_hardwarecontrol,4(a0)
	ENDC

	move.l ahiac_DriverData(a2),a0
	move.l p_Base(a0),a0

	cmp.l #AHIC_MonitorVolume,d0
	bne.s .dontsetmonvol
	nop
	bra.s .done
.dontsetmonvol
	cmp.l #AHIC_MonitorVolume_Query,d0
	bne.s .dontgetmonvol
	moveq #1,d0
	swap d0
	bra.s .doneout
.dontgetmonvol
	cmp.l #AHIC_OutputVolume,d0
	bne.s .dontsetoutvol
	lsr.l #1,d1
	nop
	bra.s .done
.dontsetoutvol
	cmp.l #AHIC_OutputVolume_Query,d0
	bne.s .dontgetoutvol
	
	moveq #1,d0
	swap d0
	bra.s .doneout
.dontgetoutvol
	cmp.l #AHIC_InputGain,d0
	bne.s .dontsetinputgain
	nop
	bra.s .done
.dontsetinputgain
	cmp.l #AHIC_InputGain_Query,d0
	bne.s .dontgetinputgain
	moveq #1,d0
	swap d0
	bra.s .doneout
.dontgetinputgain
	cmp.l #AHIC_Input,d0
	bne.s .dontsetinput
	nop
	bra.s .done
.dontsetinput
	cmp.l #AHIC_Input_Query,d0
	bne.b .dontgetinput
	moveq #0,d0
	bra.s .doneout
.dontgetinput
	cmp.l #AHIC_Output,d0
	bne.s .dontsetoutput
	nop
	bra.s .done
.dontsetoutput
	cmp.l #AHIC_Output_Query,d0
	bne.b .dontgetoutput
	moveq #0,d0
	bra.s .doneout
.dontgetoutput
	moveq #0,d0
	bra.s .doneout
.done
	moveq #1,d0
.doneout
	rts

	; d0.w = channel
	; a0 = AHIAudioCtrlDrv
callsoundfunc:
	movem.l a2/a6,-(sp)
	move.l a0,a2
	move.l ahiac_SoundFunc(a2),d1
	beq.s .nosoundfunc
	move.l d1,a0
	move.l ahiac_DriverData(a2),a1
	move.l p_Channels(a1),a1
	move.w d0,d1
	mulu #channel_SIZEOF,d1
	add.l d1,a1
	add.w #ch_SndMsg,a1
	move.w d0,(a1)
	ext.l d0
	move.l h_Entry(a0),a6
	jsr	(a6)
.nosoundfunc
	movem.l (sp)+,a2/a6
	rts

	; a1 = UAESND
interrupt_code
	move.l p_Base(a1),a0
	move.l base_stream_intreq(a0),d0
	beq.s .notours

	movem.l d2-d7/a2-a4,-(sp)
	move.l d0,d7
	move.l a1,a5
	move.l p_AudioCtrl(a5),a2
	move.l ahiac_DriverData(a2),a3
	lea STREAM_START(a0),a4

	moveq #0,d6
.cont
	btst d6,d7
	beq.s .next

	; read and clear interrupt status
	move.b stream_intreq(a4),d2
	bpl.s .next
	btst #4,d2
	beq.s .notimer

	move.l p_ChannelInfo(a3),d0
	beq.s .noci
	move.l d0,a0
	lea	ahieci_Offset(a0),a1
	move.w ahieci_Channels(a0),d0
	subq.w #1,d0
	bmi.s .noci
	move.l p_Channels(a3),a6
	move.l a4,a0
.loop
	move.l set_length_original(a0),d1
	sub.l set_length(a0),d1
	move.l d1,(a1)+
	add.w #STREAM_OFFSET,a0
	dbf d0,.loop
	
	move.l p_ChannelInfo(a3),a1
	move.l ahieci_Func(a1),d0
	beq.s .noci
	move.l d0,a0
	move.l h_Entry(a0),a6
	jsr	(a6)
.noci

	; playerfunc
	move.l ahiac_PlayerFunc(a2),d0
	beq.s .noplayerfunc
	move.l d0,a0
	sub.l a1,a1
	move.l h_Entry(a0),a6
	jsr (a6)
.noplayerfunc
	
.notimer

	; sample started interrupt?
	btst #0,d2
	beq.s .next
	
	; soundfunc
	move.w d6,d0
	move.l a2,a0
	bsr.w callsoundfunc

.next
	add.w #STREAM_OFFSET,a4
	addq.w #1,d6
	cmp.w p_StreamCnt(a3),d6
	bne.s .cont
	
	movem.l (sp)+,d2-d7/a2-a4
	moveq #0,d0
	rts
.notours
	moveq #1,d0
	rts

initRoutine:
	movem.l	d1/a0/a1/a4/a5/a6,-(sp)
	sub.l a4,a4
	move.l d0,a5
	move.l a6,ub_SysLib(a5)
	move.l a0,ub_SegList(a5)

	lea utilname(pc),a1
	moveq #0,d0
	call OpenLibrary
	move.l d0,ub_UtilBase(a5)
	beq.s .end

	lea expname(pc),a1
	moveq #0,d0
	call OpenLibrary
	tst.l d0
	beq.s .end
	move.l d0,a6
	sub.l a0,a0
	move.w #6502,d0
	moveq #2,d1
	jsr -$048(a6) ; FindConfigDev
	move.l d0,ub_ConfigDev(a5)
	move.l a6,a1
	move.l ub_SysLib(a5),a6
	call CloseLibrary

	move.l ub_ConfigDev(a5),d0
	beq.s .end
	move.l d0,a0
	move.l cd_BoardAddr(a0),ub_Base(a5)

	move.l a5,a4
.end
	move.l a4,d0
	movem.l	(sp)+,d1/a0/a1/a4/a5/a6
	rts

Open:
	addq.w #1,LIB_OPENCNT(a6)
	bclr.b #LIBB_DELEXP,ub_Flags(a6)
	move.l a6,d0
	rts

Close:
	moveq #0,d0
	subq.w #1,LIB_OPENCNT(a6)
	bne.b .exit
	btst.b #LIBB_DELEXP,ub_Flags(a6)
	beq.b .exit
	bsr	Expunge
.exit
	rts

Expunge:
	movem.l	d1/d2/a0/a1/a5/a6,-(sp)
	move.l a6,a5
	move.l ub_SysLib(a5),a6
	tst.w LIB_OPENCNT(a5)
	beq.b .notopen
	bset.b #LIBB_DELEXP,ub_Flags(a5)
	moveq #0,d0
	bra.b .Expunge_end
.notopen:

	move.l a5,a1
	call Remove

	move.l ub_UtilBase(a5),d0
	beq.s .noutil
	move.l d0,a1
	call CloseLibrary
.noutil

	move.l ub_SegList(a5),d2
	moveq #0,d0
	move.l a5,a1
	move.w LIB_NEGSIZE(a5),d0
	sub.l d0,a1
	add.w LIB_POSSIZE(a5),d0
	call FreeMem

	move.l d2,d0
.Expunge_end
	movem.l	(sp)+,d1/d2/a0/a1/a5/a6
	rts

Null:
	moveq #0,d0
	rts

expname
	dc.b "expansion.library",0
utilname
	dc.b "utility.library",0
intname
	dc.b "uaesnd_ahi",0
	cnop 0,4

EndCode:
