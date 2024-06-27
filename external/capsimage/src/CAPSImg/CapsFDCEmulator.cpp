#include "stdafx.h"



// bits removed from track
int fdcrobbit[]= {
	0,
	1,
	0,
	3,
	0,
	2,
	0,
	-1
};

// track size for unformatted dd tracks
SDWORD fdcddnoise[]= {
	12425,
	11970,
	12277,
	12124,
	12372,
	-1
};

// init defaults for a new command
CapsFdcInit fdcinit[]= {
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 0
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 1
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 2
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 3
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 4
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 5
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 6
	cfdcrmType1, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 7
	cfdcrmType2R, CAPSFDC_SM_TYPE2R, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 8
	cfdcrmType2R, CAPSFDC_SM_TYPE2R, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // 9
	cfdcrmType2W, CAPSFDC_SM_TYPE2W, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // a
	cfdcrmType2W, CAPSFDC_SM_TYPE2W, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // b
	cfdcrmType3A, CAPSFDC_SM_TYPE2R, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // c
	cfdcrmType4, CAPSFDC_SM_TYPE1, CAPSFDC_SR_NCCLR, 0, ~0U, 0, // d
	cfdcrmType3R, CAPSFDC_SM_TYPE2R, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0, // e
	cfdcrmType3W, CAPSFDC_SM_TYPE2W, CAPSFDC_SR_NCCLR, CAPSFDC_SR_NCSET, ~0U, 0  // f
};

// no operation cycles
FDCCALL fdccall_nop[]= {
	FdcComT_NOP,
	NULL
};

// idle cycles
FDCCALL fdccall_idle[]= {
	FdcComT_Idle,
	NULL
};

// type 1 cycles
FDCCALL fdccall_t1[]= {
	FdcComT1_SpinupStart,
	FdcComT1_SpinupLoop,
	FdcComT1_StepStart,
	FdcComT1_Step,
	FdcComT1_StepLoop,
	FdcComT1_DelayStart,
	FdcComT1_DelayLoop,
	FdcComT1_VerifyStart,
	FdcComT1_VerifyLoop,
	NULL
};

// type 2 read cycles
FDCCALL fdccall_t2r[]= {
	FdcComT1_SpinupStart,
	FdcComT1_SpinupLoop,
	FdcComT2_DelayStart,
	FdcComT1_DelayLoop,
	FdcComT2_ReadStart,
	FdcComT2_ReadLoop,
	NULL
};

// type 2 write cycles
FDCCALL fdccall_t2w[]= {
	FdcComT2W,
	NULL
};

// type 3 read cycles
FDCCALL fdccall_t3r[]= {
	FdcComT1_SpinupStart,
	FdcComT1_SpinupLoop,
	FdcComT2_DelayStart,
	FdcComT1_DelayLoop,
	FdcComT3_IndexStart,
	FdcComT3_IndexLoop,
	FdcComT3_ReadStart,
	FdcComT3_ReadLoop,
	NULL
};

// type 3 write cycles
FDCCALL fdccall_t3w[]= {
	FdcComT1_SpinupStart,
	FdcComT1_SpinupLoop,
	FdcComT2_DelayStart,
	FdcComT1_DelayLoop,
	FdcComT3_WriteCheck,
	FdcComT3_IndexStart,
	FdcComT3_IndexLoop,
	FdcComT3_WriteStart,
	FdcComT3_WriteLoop,
	NULL
};

// type 3 address cycles
FDCCALL fdccall_t3a[]= {
	FdcComT1_SpinupStart,
	FdcComT1_SpinupLoop,
	FdcComT2_DelayStart,
	FdcComT1_DelayLoop,
	FdcComT2_ReadStart,
	FdcComT3_AddressLoop,
	NULL
};

// type 4 cycles
FDCCALL fdccall_t4[]= {
	NULL
};

// fdc cycle call tables reference, order must match he run mode enumeration
FDCCALL *fdccall[]= {
	fdccall_nop,  // no-operation
	fdccall_idle, // idle/wait loop
	fdccall_t1,   // type 1 loop
	fdccall_t2r,  // type 2 read loop
	fdccall_t2w,  // type 2 write loop
	fdccall_t3r,  // type 3 read loop
	fdccall_t3w,  // type 3 write loop
	fdccall_t3a,  // type 3 address loop
	fdccall_t4,   // type 4 loop
	NULL
};



// get fdc emulator information
UDWORD __cdecl CAPSFdcGetInfo(SDWORD iid, PCAPSFDC pc, SDWORD ext)
{
	UDWORD res=0;

	switch (iid) {
		case cfdciSize_Fdc:
			res=sizeof(CapsFdc);
			break;

		case cfdciSize_Drive:
			res=sizeof(CapsDrive);
			break;

		case cfdciR_Command:
			res=pc->r_command;
			break;

		case cfdciR_ST:
			res=(pc->r_st0 & ~pc->r_stm) | (pc->r_st1 & pc->r_stm);
			break;

		case cfdciR_Track:
			res=pc->r_track;
			break;

		case cfdciR_Sector:
			res=pc->r_sector;
			break;

		case cfdciR_Data:
			res=pc->r_data;
			break;
	}

	return res;
}

// initialise fdc emulator
SDWORD __cdecl CAPSFdcInit(PCAPSFDC pc)
{
	if (!pc)
		return imgeGeneric;

	// check fdc workspace size
	if (pc->type < sizeof(CapsFdc))
		return imgeUnsupportedType;

	// get presets
	CapsFdc fdc=*pc;

	// clear structure
	memset(pc, 0, sizeof(CapsFdc));

	// restore presets
	pc->type=fdc.type;
	pc->model=fdc.model;
	pc->clockfrq=fdc.clockfrq;
	pc->drive=fdc.drive;
	pc->drivecnt=fdc.drivecnt;
	pc->drivemax=fdc.drivemax;
	pc->userptr=fdc.userptr;
	pc->userdata=fdc.userdata;

	// no drive selected, force change
	pc->drivesel=-1;
	pc->driveact=-1;
	pc->drivenew=-2;

	// error if unsupported fdc
	if (pc->model != cfdcmWD1772)
		return imgeUnsupportedType;

	// check valid drive numbers
	if (pc->drivecnt<=0 || pc->drivemax<0 || pc->drivemax>pc->drivecnt)
		return imgeOutOfRange;

	// check attached drives
	if (!pc->drive)
		return imgeGeneric;

	// check workspace size for all drives
	for (int drv=0; drv < pc->drivecnt; drv++) {
		if (pc->drive[drv].type < sizeof(CapsDrive))
			return imgeUnsupportedType;
	}

	// 2 address lines, 8 data lines, 0...3 sector length
	pc->addressmask=0x03;
	pc->datamask=0xff;
	pc->seclenmask=0x03;

	// index counter constants
	pc->readlimit=5;
	pc->verifylimit=6;
	pc->spinuplimit=6;
	pc->idlelimit=10;

	// stepping rates
	pc->steptime[0]=6000;
	pc->steptime[1]=12000;
	pc->steptime[2]=2000;
	pc->steptime[3]=3000;

	// head settling time
	pc->hstime=15000;

	// index pulse hold
	pc->iptime=4000;

	// update delay
	pc->updatetime=8;

	// initialise to default emulation state
	FdcSetTiming(pc);
	FdcInit(pc);

	return imgeOk;
}

// fdc hardware reset
void __cdecl CAPSFdcReset(PCAPSFDC pc)
{
	FdcSetTiming(pc);
	FdcReset(pc);

	//pc->drive[0].diskattr|=CAPSDRIVE_DA_SS;
	// HACK: insert a disk automatically for testing
	//pc->drive[0].diskattr|=CAPSDRIVE_DA_IN;
	//pc->drive[0].diskattr&=~CAPSDRIVE_DA_WP;
	//pc->cbtrk=FdcHackChange;
}

// fdc emulator
void __cdecl CAPSFdcEmulate(PCAPSFDC pc, UDWORD cyclecnt)
{
	// no cycles done
	pc->clockact=0;

	// requested cycles
	pc->clockreq=cyclecnt;

	// no end request
	pc->endrequest=0;

	// run fdc for selected clock cycles
	if (pc->clockreq) {
		FDCCALL *fc=fdccall[pc->runmode];

		// process requested cycles
		while (!pc->endrequest && pc->clockact<pc->clockreq)
			fc[pc->runstate](pc);

		// stop processing after end request
		if (pc->endrequest)
			FdcComEnd(pc);
	}

	// update only if all cycles are complete
	if (pc->clockact >= pc->clockreq)
		FdcUpdateDrive(pc, pc->clockreq);
}

// read from fdc register
UDWORD __cdecl CAPSFdcRead(PCAPSFDC pc, UDWORD address)
{
	UDWORD data;

	// mask valid address lines
	switch (address & pc->addressmask) {
		// status, clear intrq
		case 0:
			data=(pc->r_st0 & ~pc->r_stm) | (pc->r_st1 & pc->r_stm);
			FdcSetLine(pc, pc->lineout&~CAPSFDC_LO_INTRQ);
			break;

		// track
		case 1:
			data=pc->r_track;
			break;

		// sector
		case 2:
			data=pc->r_sector;
			break;

		// data, clear drq if set
		case 3:
			data=pc->r_data;
			FdcSetLine(pc, pc->lineout&~CAPSFDC_LO_DRQ);
			break;

		default:
			NODEFAULT;
	}

	// return valid data bits only
	data&=pc->datamask;

	// set data bus
	pc->dataline=data;

	return data;
}

// write to fdc register
void   __cdecl CAPSFdcWrite(PCAPSFDC pc, UDWORD address, UDWORD data)
{
	// allow valid data bits only
	data&=pc->datamask;

	// set data bus
	pc->dataline=data;

	// mask valid address lines
	switch (address & pc->addressmask) {
		// command, process type 4 always, type 1...3 only if not busy
		case 0:
			if (!(pc->r_st0 & CAPSFDC_SR_BUSY) || ((data & 0xf0) == 0xd0))
				FdcCom(pc, data);
			break;

		// track, not protected during busy despite documentation
		case 1:
			pc->r_track = data;
			break;

		// sector, not protected during busy despite documentation
		case 2:
			pc->r_sector = data;
			break;

		// data, clear drq if set
		case 3:
			pc->r_data = data;
			FdcSetLine(pc, pc->lineout&~CAPSFDC_LO_DRQ);
			break;

		default:
			NODEFAULT;
	}
}

// invalidate track data, next read cycle will request a track change callback as well as re-lock the stream
SDWORD __cdecl CAPSFdcInvalidateTrack(PCAPSFDC pc, SDWORD drive)
{
	if (!pc)
		return imgeGeneric;

	if (drive<0 || drive>=pc->drivecnt)
		return imgeOutOfRange;

	PCAPSDRIVE pd=pc->drive+drive;
	pd->buftrack=-1;
	pd->bufside=-1;

	return imgeOk;
}

// precalculate timing constants
void FdcSetTiming(PCAPSFDC pc)
{
	// drive specific values
	for (int drv=0; drv < pc->drivecnt; drv++) {
		PCAPSDRIVE pd=pc->drive+drv;

		// clock cycles per revolution
		pd->clockrev=UDWORD(((UQUAD)pc->clockfrq*60)/pd->rpm);

		// clock cycles for index pulse hold
		pd->clockip=UDWORD(((UQUAD)pc->clockfrq*pc->iptime)/1000000);
	}

	// clock cycles for stepping rates
	for (int drv=0; drv < 4; drv++)
		pc->clockstep[drv]=UDWORD(((UQUAD)pc->clockfrq*pc->steptime[drv])/1000000);

	// clock cycles for head settling
	pc->clockhs=UDWORD(((UQUAD)pc->clockfrq*pc->hstime)/1000000);

	// clock cycles for short operations (some SR updates)
	pc->clockupdate=UDWORD(((UQUAD)pc->clockfrq*pc->updatetime)/1000000);
}

// initialise fdc and drive
void FdcInit(PCAPSFDC pc)
{
	// reset drives, no disk inserted
	for (int drv=0; drv < pc->drivecnt; drv++) {
		PCAPSDRIVE pd=pc->drive+drv;
		pd->track=0;
		pd->buftrack=-1;
		pd->side=0;
		pd->bufside=-1;
		pd->newside=0;
		pd->diskattr=CAPSDRIVE_DA_WP;
		pd->idistance=0;
		pd->ipcnt=0;
		pd->ovlact=0;
		pd->nact=0;
		pd->nseed=0x87654321;

		// no track data
		FdcClearTrackData(pd);
	}

	// reset internal state
	FdcResetState(pc);

	// all output lines are clear, without callbacks from init
	pc->dataline=0;
	pc->lineout=0;

	// forced update
	pc->drivesel=pc->drivenew-1;
	FdcUpdateDrive(pc, 0);
}

// reset fdc and drive lines
void FdcReset(PCAPSFDC pc)
{
	// reset internal state
	FdcResetState(pc);

	// all output lines are clear
	pc->dataline=0;
	FdcSetLine(pc, 0);

	// forced update
	pc->drivesel=pc->drivenew-1;
	FdcUpdateDrive(pc, 0);
}

// reset fdc internal state
void FdcResetState(PCAPSFDC pc)
{
	// clear internal registers
	pc->r_st0=0;
	pc->r_st1=0;
	pc->r_command=0;
	pc->r_track=0;
	pc->r_sector=0;
	pc->r_data=0;

	// type 1 status, idle
	pc->r_stm=CAPSFDC_SM_TYPE1;
	pc->runmode=cfdcrmIdle;
	pc->runstate=0;
	pc->indexcount=0;
	pc->indexlimit=-1;
	pc->spinupcnt=0;
	pc->idlecnt=0;
	pc->clockcnt=0;
}

// process commands
void FdcCom(PCAPSFDC pc, UDWORD data)
{
	// store command
	pc->r_command=data;

#ifdef _DEBUG
	{
		char dbuf[64];
		int rtrack=pc->driveprc ? pc->driveprc->track : -1;
		int rside=pc->driveprc ? pc->driveprc->side : -1;
		sprintf(dbuf, "%2.2x: %2.2x %2.2x %2.2x %d.%d\n",
			pc->r_command,
			pc->r_track,
			pc->r_sector,
			pc->r_data,
			rtrack,
			rside);
		OutputDebugString(dbuf);
	}
#endif

	// reset command state
	pc->runstate=0;
	pc->indexlimit=-1;

	int comcl=(pc->r_command>>4) & 0xf;

	if (comcl == 0xd)
		FdcComT4(pc);
	else {
		// start loop
		pc->runmode=fdcinit[comcl].runmode;
		pc->idlecnt=0;

		// set status type
		pc->r_stm=fdcinit[comcl].stmask;

		// clear status bits
		pc->r_st0&=~fdcinit[comcl].st0clr;
		pc->r_st1&=~fdcinit[comcl].st1clr;

		// set status bits
		pc->r_st0|=fdcinit[comcl].st0set;
		pc->r_st1|=fdcinit[comcl].st1set;

		// clear drq, intrq, forced interrupt and index enable lines
		FdcSetLine(pc, pc->lineout & ~(CAPSFDC_LO_DRQ | CAPSFDC_LO_INTRQ | CAPSFDC_LO_INTFRC | CAPSFDC_LO_INTIP));
	}
}

// end commands
void FdcComEnd(PCAPSFDC pc)
{
	// command complete
	if (pc->endrequest & CAPSFDC_ER_COMEND) {
		// start idle
		pc->runmode=cfdcrmIdle;
		pc->runstate=0;
		pc->indexlimit=-1;
		pc->idlecnt=0;

		// clear busy
		pc->r_st0&=~CAPSFDC_SR_BUSY;

		// set intrq
		FdcSetLine(pc, pc->lineout|CAPSFDC_LO_INTRQ);
	}

	// run remaining cycles
	FdcComIdle(pc, pc->clockreq-pc->clockact);
}

// no operation command; use all cycles, never switch to idle state
void FdcComT_NOP(PCAPSFDC pc)
{
	pc->clockact=pc->clockreq;
}

// idle operation, use all remaining cycles
void FdcComT_Idle(PCAPSFDC pc)
{
	FdcComIdle(pc, pc->clockreq-pc->clockact);
}



// start type 1 spin-up sequence
void FdcComT1_SpinupStart(PCAPSFDC pc)
{
	UDWORD lo=pc->lineout;

	// set motor line
	pc->lineout|=CAPSFDC_LO_MO;

	// if valid drive selected turn on its motor
	if (pc->driveprc)
		pc->driveprc->diskattr|=CAPSDRIVE_DA_MO;

	// set sr motor clear spin-up
	pc->r_st0=(pc->r_st0 & ~CAPSFDC_SR_SU_RT) | CAPSFDC_SR_MO;

	// reset spin-up counter
	pc->spinupcnt=0;

	// skip loop if no spin-up or motor running 
	if ((pc->r_command & DF_3) || (lo & CAPSFDC_LO_MO)) {
		pc->r_st0|=CAPSFDC_SR_SU_RT;
		pc->runstate+=2;
		return;
	}

	// set loop
	pc->runstate++;
}

// type 1 spin-up sequence loop
void FdcComT1_SpinupLoop(PCAPSFDC pc)
{
	// clock cycles to complete
	UDWORD cmax=pc->clockreq-pc->clockact;

	// if no drive selected run all cycles index can't happen
	if (!pc->driveprc) {
		FdcComIdle(pc, cmax);
		return;
	}

	// if index can't happen on enabled drive run all cycles
	PCAPSDRIVE pd=pc->driveprc;
	if ((pd->diskattr & CAPSDRIVE_DA_IPMASK) != CAPSDRIVE_DA_IPMASK) {
		FdcComIdle(pc, cmax);
		return;
	}

	// if index or spin-up event won't happen, run it all
	if ((pd->idistance+cmax<pd->clockrev) || (pc->spinupcnt+1 < pc->spinuplimit)){
		FdcComIdle(pc, cmax);
		return;
	}

	// cycles until index
	cmax=pd->clockrev-pd->idistance;

	// index and spin-up happens now, only run cycles until index
	if (FdcComIdle(pc, cmax) != cmax)
		return;

	// end loop, but only if wait was complete
	pc->runstate++;
}

// type 1 start step sequence
void FdcComT1_StepStart(PCAPSFDC pc)
{
	// pre-process step
	switch ((pc->r_command>>4) & 0xf) {
		// restore
		case 0x0:
			// move from track#255 to 0 (ie step-out until stopped)
			pc->r_track=0xff;
			pc->r_data=0;
			break;

		// seek, step
		case 0x1:
		case 0x2:
		case 0x3:
			break;

		// step-in
		case 0x4:
		case 0x5:
			pc->lineout|=CAPSFDC_LO_DIRC;
			break;

		// step-out
		case 0x6:
		case 0x7:
			pc->lineout&=~CAPSFDC_LO_DIRC;
			break;

		default:
			NODEFAULT;
	}

	// set step
	pc->runstate++;
}

// type 1 step sequence
void FdcComT1_Step(PCAPSFDC pc)
{
	// seek or restore
	if (pc->r_command < 0x20) {
		// skip loop if destination track reached
		if (pc->r_track == pc->r_data) {
			pc->runstate+=2;
			return;
		}

		// if destination>track step-in, else step-out
		if (pc->r_data > pc->r_track)
			pc->lineout|=CAPSFDC_LO_DIRC;
		else
			pc->lineout&=~CAPSFDC_LO_DIRC;
	}

	// step direction, 0: out
	int dir=pc->lineout & CAPSFDC_LO_DIRC;

	// if seek, restore or step with update requested
	if (pc->r_command<0x20 || (pc->r_command & DF_4)) {
		// update track register
		if (dir) {
			// step-in, 255 wraps to 1 (due to add with carry)
			if (pc->r_track == 0xff)
				pc->r_track=0x01;
			else
				pc->r_track++;
		} else {
			// step-out, 0 wraps to 0xfe (due to subtract with carry)
			if (!pc->r_track)
				pc->r_track=0xfe;
			else
				pc->r_track--;
		}
	}

	// get track# if valid drive or -1
	int track=pc->driveprc ? pc->driveprc->track : -1;

	// if on track0 and step-out, track=0, set tr0, skip loop
	if (!track && !dir) {
		pc->r_track=0;
		pc->r_st0|=CAPSFDC_SR_TR0_LD;
		pc->runstate+=2;
		return;
	}

	// step pulse if drive is valid
	if (track >= 0) {
		if (dir) {
			// step-in, hard stop head on last valid track
			if (track >= pc->driveprc->maxtrack)
				track=pc->driveprc->maxtrack;
			else
				track++;

			// clear tr0, since we can't be on track0 anymore
			pc->r_st0&=~CAPSFDC_SR_TR0_LD;
		} else {
			// step-out, hard stop head on track0
			if (track > 0)
				track--;

			// set tr0 if on track0
			if (!track)
				pc->r_st0|=CAPSFDC_SR_TR0_LD;
		}

		// set new track#
		pc->driveprc->track=track;
	}

	// set step delay
	pc->clockcnt=pc->clockstep[pc->r_command & 3];

	// set loop
	pc->runstate++;
}

// type 1 step sequence loop
void FdcComT1_StepLoop(PCAPSFDC pc)
{
	// clock cycles to complete
	UDWORD cmax=pc->clockreq-pc->clockact;

	// only allow delay cycles
	if (cmax > pc->clockcnt)
		cmax=pc->clockcnt;

	// run cycles
	pc->clockcnt-=FdcComIdle(pc, cmax);

	// if delay finished
	if (!pc->clockcnt) {
		// repeat step phase if seek or restore, or end loop
		if (pc->r_command < 0x20)
			pc->runstate--;
		else
			pc->runstate++;
	}
}

// type 1 delay sequence
void FdcComT1_DelayStart(PCAPSFDC pc)
{
	// end if no verify
	if (!(pc->r_command & DF_2)) {
		pc->endrequest|=CAPSFDC_ER_COMEND;
		return;
	}

	// set settle delay
	pc->clockcnt=pc->clockhs;

	// set loop
	pc->runstate++;
}

// type 1 delay sequence loop
void FdcComT1_DelayLoop(PCAPSFDC pc)
{
	// clock cycles to complete
	UDWORD cmax=pc->clockreq-pc->clockact;

	// only allow delay cycles
	if (cmax > pc->clockcnt)
		cmax=pc->clockcnt;

	// run cycles
	pc->clockcnt-=FdcComIdle(pc, cmax);

	// end loop if delay finished
	if (!pc->clockcnt)
		pc->runstate++;
}

// type 1 verify sequence
void FdcComT1_VerifyStart(PCAPSFDC pc)
{
	// reset logic
	FdcResetData(pc);

	// set abort on maximum revolutions for verify
	pc->indexlimit=pc->verifylimit;

	// set loop
	pc->runstate++;
}

// type 1 verify loop
void FdcComT1_VerifyLoop(PCAPSFDC pc)
{
	// update data access
	FdcUpdateData(pc);

	// get access hook, crc
	FDCREAD ph=FdcGetReadAccess(pc);

	// clock cycles to complete
	UDWORD cst=pc->clockact;

	// process requested cycles
	while (!pc->endrequest && pc->clockact<pc->clockreq) {
		// read byte when ready
		if (!ph(pc))
			continue;

		// check ID
		switch (pc->dataphase) {
			case 0:
				// AM detected, read returns on dsr ready
				pc->amisigmask=CAPSFDC_AI_DSRREADY;
				pc->dataphase++;
				break;

			case 1:
				//if not fc...ff, restart
				if (pc->dsr < 0xfc) {
					FdcResetAm(pc);
					continue;
				}

				pc->dataphase++;
				break;

			case 2:
				//if not the track, restart
				if (pc->dsr != pc->r_track) {
					FdcResetAm(pc);
					continue;
				}

				pc->dataphase++;
				break;

			// skip rest ofID
			case 3:
			case 4:
			case 5:
			case 6:
				pc->dataphase++;
				break;

			case 7:
				// if crc error, set crc flag, restart
				if (pc->crc & 0xffff) {
					FdcResetAm(pc);
					pc->r_st0|=CAPSFDC_SR_CRCERR;
					continue;
				}

				// clear crc flag, stop
				pc->r_st0&=~CAPSFDC_SR_CRCERR;

				// run cycles used on unselected drives
				FdcComIdleOther(pc, pc->clockact-cst);

				// end
				pc->endrequest|=CAPSFDC_ER_COMEND;
				return;

			default:
				NODEFAULT;
		}
	}

	// end requested, set seek error
	if (pc->endrequest & CAPSFDC_ER_COMEND)
		pc->r_st0|=CAPSFDC_SR_RNF;

	// run cycles on unselected drives
	FdcComIdleOther(pc, pc->clockact-cst);
}



// type 2 delay sequence
void FdcComT2_DelayStart(PCAPSFDC pc)
{
	// skip if no delay
	if (!(pc->r_command & DF_2)) {
		pc->runstate+=2;
		return;
	}

	// set settle delay
	pc->clockcnt=pc->clockhs;

	// set loop
	pc->runstate++;
}

// type 2 read sequence
void FdcComT2_ReadStart(PCAPSFDC pc)
{
	// reset logic
	FdcResetData(pc);

	// set abort on maximum revolutions for verify
	pc->indexlimit=pc->readlimit;

	// set loop
	pc->runstate++;
}

// type 2 read loop
void FdcComT2_ReadLoop(PCAPSFDC pc)
{
	// update data access
	FdcUpdateData(pc);

	// get access hook, crc
	FDCREAD ph=FdcGetReadAccess(pc);

	// clock cycles to complete
	UDWORD cst=pc->clockact;

	// process requested cycles
	while (!pc->endrequest && pc->clockact<pc->clockreq) {
		// read byte when ready
		if (!ph(pc))
			continue;

		// check ID
		switch (pc->dataphase) {
			case 0:
				// AM detected, read returns on dsr ready
				pc->amisigmask=CAPSFDC_AI_DSRREADY;
				pc->dataphase++;
				break;

			case 1:
				//if not fc...ff, restart
				if (pc->dsr < 0xfc) {
					FdcResetAm(pc);
					continue;
				}

				pc->dataphase++;
				break;

			case 2:
				//if not the track, restart
				if (pc->dsr != pc->r_track) {
					FdcResetAm(pc);
					continue;
				}

				pc->dataphase++;
				break;

			case 4:
				//if not the sector, restart
				if (pc->dsr != pc->r_sector) {
					FdcResetAm(pc);
					continue;
				}

				pc->dataphase++;
				break;

			case 5:
				// get sector length
				pc->seclen=0x80<<(pc->dsr & pc->seclenmask);
				pc->dataphase++;
				break;

			// skip rest ofID
			case 3:
			case 6:
				pc->dataphase++;
				break;

			case 7:
				// if crc error, set crc flag, restart
				if (pc->crc & 0xffff) {
					FdcResetAm(pc);
					pc->r_st0|=CAPSFDC_SR_CRCERR;
					continue;
				}

				// clear crc flag
				pc->r_st0&=~CAPSFDC_SR_CRCERR;

				// read on dsr ready
				pc->dataphase++;
				break;

			case 8:
				// ignore 28 bytes, the fdc can't read them
				if (pc->datapcnt < 28) {
					if (pc->datapcnt == 27) {
						FdcResetAm(pc, 1);
						pc->amisigmask=CAPSFDC_AI_DSRREADY|CAPSFDC_AI_DSRMA1;
					}

					pc->datapcnt++;
					break;
				}
				pc->dataphase++;

			case 9:
				// wait maximum 43 bytes for A1 mark, then restart
				if (!(pc->aminfo & CAPSFDC_AI_DSRMA1)) {
					if (pc->datapcnt >= 43)
						FdcResetAm(pc);
					else
						pc->datapcnt++;
					continue;
				}
				pc->dataphase++;
				pc->datapcnt=0;

			case 10:
				//if not A1 mark, restart
				if (!(pc->aminfo & CAPSFDC_AI_DSRMA1)) {
					FdcResetAm(pc);
					continue;
				}

				// wait for AM
				if (!(pc->aminfo & CAPSFDC_AI_DSRAM))
					continue;

				// AM detected, read returns on dsr ready
				pc->amisigmask=CAPSFDC_AI_DSRREADY;
				pc->dataphase++;
				break;

			case 11:
				// check data type
				switch (pc->dsr) {
					// f8, f9 deleted data
					case 0xf8:
					case 0xf9:
						pc->r_st1|=CAPSFDC_SR_SU_RT;
						break;

					// fa, fb normal data
					case 0xfa:
					case 0xfb:
						pc->r_st1&=~CAPSFDC_SR_SU_RT;
						break;

					default:
						FdcResetAm(pc);
						continue;
				}

				pc->dataphase++;
				break;

			case 12:
				// dsr to data, drq signal
				pc->r_data=pc->dsr;
				FdcSetLine(pc, pc->lineout|CAPSFDC_LO_DRQSET);

				if (!--pc->seclen)
					pc->dataphase++;
				break;

			case 13:
				pc->dataphase++;
				break;

			case 14:
				// if crc error, set crc flag, stop
				if (pc->crc & 0xffff) {
					pc->r_st0|=CAPSFDC_SR_CRCERR;

					// run cycles used on unselected drives
					FdcComIdleOther(pc, pc->clockact-cst);

					// end
					pc->endrequest|=CAPSFDC_ER_COMEND;
					return;
				}

				// clear crc flag
				pc->r_st0&=~CAPSFDC_SR_CRCERR;

				// if multiple sector, restart on next sector
				if (pc->r_command & DF_4) {
					// next sector, 255 wraps to 1 (due to add with carry)
					if (pc->r_sector == 0xff)
						pc->r_sector=0x01;
					else
						pc->r_sector++;

					FdcResetAm(pc);
					continue;
				}

				// run cycles used on unselected drives
				FdcComIdleOther(pc, pc->clockact-cst);

				// end
				pc->endrequest|=CAPSFDC_ER_COMEND;
				return;

			default:
				NODEFAULT;
		}
	}

	// end requested, set seek error
	if (pc->endrequest & CAPSFDC_ER_COMEND)
		pc->r_st0|=CAPSFDC_SR_RNF;

	// run cycles on unselected drives
	FdcComIdleOther(pc, pc->clockact-cst);
}



// type 2 write commands
void FdcComT2W(PCAPSFDC pc)
{
	// TODO: implement
	pc->endrequest|=CAPSFDC_ER_COMEND;
}



// type 3 index sequence
void FdcComT3_IndexStart(PCAPSFDC pc)
{
	// nothing changes if no input
	if (pc->driveprc) {
		// skip if on index
		if (!pc->driveprc->idistance) {
			pc->runstate+=2;
			return;
		}
	}

	// set loop
	pc->runstate++;
}

// type 3 index sequence loop
void FdcComT3_IndexLoop(PCAPSFDC pc)
{
	// clock cycles to complete
	UDWORD cmax=pc->clockreq-pc->clockact;

	// if no drive selected run all cycles index can't happen
	if (!pc->driveprc) {
		FdcComIdle(pc, cmax);
		return;
	}

	// if index can't happen on enabled drive run all cycles
	PCAPSDRIVE pd=pc->driveprc;
	if ((pd->diskattr & CAPSDRIVE_DA_IPMASK) != CAPSDRIVE_DA_IPMASK) {
		FdcComIdle(pc, cmax);
		return;
	}

	// if index event won't happen, run it all
	if (pd->idistance+cmax < pd->clockrev){
		FdcComIdle(pc, cmax);
		return;
	}

	// cycles until index
	cmax=pd->clockrev-pd->idistance;

	// index happens now, only run cycles until index
	if (FdcComIdle(pc, cmax) != cmax)
		return;

	// end loop, but only if wait was complete
	pc->runstate++;
}

// type 3 read sequence
void FdcComT3_ReadStart(PCAPSFDC pc)
{
	// reset logic
	FdcResetData(pc);
	pc->aminfo&=~CAPSFDC_AI_CRCENABLE;
	pc->amisigmask=CAPSFDC_AI_DSRREADY;

	// abort on next index
	pc->indexlimit=1;

	// set loop
	pc->runstate++;
}

// type 3 read loop
void FdcComT3_ReadLoop(PCAPSFDC pc)
{
	// update data access
	FdcUpdateData(pc);

	// get access hook, crc
	FDCREAD ph=FdcGetReadAccess(pc);

	// clock cycles to complete
	UDWORD cst=pc->clockact;

	// process requested cycles
	while (!pc->endrequest && pc->clockact<pc->clockreq) {
		// read byte when ready
		if (!ph(pc))
			continue;

		// dsr to data, drq signal
		pc->r_data=pc->dsr;
		FdcSetLine(pc, pc->lineout|CAPSFDC_LO_DRQSET);
	}

	// run cycles on unselected drives
	FdcComIdleOther(pc, pc->clockact-cst);
}



// check write protect
void FdcComT3_WriteCheck(PCAPSFDC pc)
{
	// if wp signal, end command
	if (pc->r_st0 & CAPSFDC_SR_WP) {
		pc->endrequest|=CAPSFDC_ER_COMEND;
		return;
	}

	// next
	pc->runstate++;
}

// type 3 write sequence
void FdcComT3_WriteStart(PCAPSFDC pc)
{
	// reset logic
	FdcResetData(pc);

	// abort on next index
	pc->indexlimit=1;

	// set loop
	pc->runstate++;
}

// type 3 write loop
void FdcComT3_WriteLoop(PCAPSFDC pc)
{
	pc->endrequest|=CAPSFDC_ER_COMEND;
}



// type 3 address loop
void FdcComT3_AddressLoop(PCAPSFDC pc)
{
	// update data access
	FdcUpdateData(pc);

	// get access hook, crc
	FDCREAD ph=FdcGetReadAccess(pc);

	// clock cycles to complete
	UDWORD cst=pc->clockact;

	// process requested cycles
	while (!pc->endrequest && pc->clockact<pc->clockreq) {
		// read byte when ready
		if (!ph(pc))
			continue;

		// check ID
		switch (pc->dataphase) {
			// detect a1 sync
			case 0:
				// AM detected, read returns on dsr ready
				pc->amisigmask=CAPSFDC_AI_DSRREADY;
				pc->dataphase++;
				break;

			case 1:
				//if not fc...ff, restart
				if (pc->dsr < 0xfc) {
					FdcResetAm(pc);
					continue;
				}

				pc->dataphase++;
				break;

			case 2:
				// set the sector register to the track number
				pc->r_sector=pc->dsr;

			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				// dsr to data, drq signal
				pc->r_data=pc->dsr;
				FdcSetLine(pc, pc->lineout|CAPSFDC_LO_DRQSET);

				// next phase
				if (pc->dataphase++ == 7) {
					// if crc error, set crc flag
					if (pc->crc & 0xffff)
						pc->r_st0|=CAPSFDC_SR_CRCERR;

					// run cycles used on unselected drives
					FdcComIdleOther(pc, pc->clockact-cst);

					// end
					pc->endrequest|=CAPSFDC_ER_COMEND;
					return;
				}
				break;

			default:
				NODEFAULT;
		}
	}

	// end requested, set seek error
	if (pc->endrequest & CAPSFDC_ER_COMEND)
		pc->r_st0|=CAPSFDC_SR_RNF;

	// run cycles on unselected drives
	FdcComIdleOther(pc, pc->clockact-cst);
}



// type 4 commands
void FdcComT4(PCAPSFDC pc)
{
	// set motor line
	pc->lineout |= CAPSFDC_LO_MO;

	// if valid drive selected turn on its motor
	if (pc->driveprc)
		pc->driveprc->diskattr |= CAPSDRIVE_DA_MO;

	// set sr motor
	pc->r_st0 |= CAPSFDC_SR_MO;

	// start type 1 idle
	pc->runmode=cfdcrmIdle;
	pc->runstate=0;
	pc->indexlimit=-1;
	pc->idlecnt=0;

	// check busy
	if (pc->r_st0 & CAPSFDC_SR_BUSY) {
		// busy, clear busy, leave sr unchanged
		pc->r_st0&=~CAPSFDC_SR_BUSY;
	} else {
		// not busy, set type 1 status
		pc->r_stm=CAPSFDC_SM_TYPE1;
		pc->r_st0 &= ~(CAPSFDC_SR_SU_RT | CAPSFDC_SR_RNF | CAPSFDC_SR_CRCERR);
		pc->r_st1 = 0;
	}

	// clear drq, intrq, forced interrupt and index enable lines
	UDWORD line=pc->lineout & ~(CAPSFDC_LO_DRQ | CAPSFDC_LO_INTRQ | CAPSFDC_LO_INTFRC | CAPSFDC_LO_INTIP);

	// set index interrupt if requested
	if (pc->r_command & DF_2)
		line|=CAPSFDC_LO_INTIP;

	// set forced interrupt
	if (pc->r_command & DF_3)
		line|=CAPSFDC_LO_INTFRC;

	// set line state
	FdcSetLine(pc, line);
}



// type 1 idle process
UDWORD FdcComIdle(PCAPSFDC pc, UDWORD cyc)
{
	// wait state cancelled if end requested
	if (pc->endrequest & CAPSFDC_ER_REQEND)
		return 0;

	// process all drives
	for (int drv=0; drv < pc->drivemax; drv++) {
		// get drive
		PCAPSDRIVE pd=pc->drive+drv;

		// if motor off no distance change and no index signals to count
		if (!(pd->diskattr & CAPSDRIVE_DA_MO))
			continue;

		// motor on, advance distance
		UDWORD dist=pd->idistance+cyc;

		// restart distance count on new revolution
		if (dist >= pd->clockrev) {
			dist-=pd->clockrev;

			// process disk index
			pd->idistance=0;
			FdcIndex(pc, drv);
		}

		// set distance
		pd->idistance=dist;
	}

	// !!!
	pc->clockact+=cyc;
	return cyc;
}

// type 1 idle process for all drives except the active one, catchup with active one
void FdcComIdleOther(PCAPSFDC pc, UDWORD cyc)
{
	// process all drives
	for (int drv=0; drv < pc->drivemax; drv++) {
		// skip active drive
		if (drv == pc->driveact)
			continue;

		// get drive
		PCAPSDRIVE pd=pc->drive+drv;

		// if motor off no distance change
		if (!(pd->diskattr & CAPSDRIVE_DA_MO))
			continue;

		// motor on, advance distance
		UDWORD dist=pd->idistance+cyc;

		// restart distance count on new revolution
		if (dist >= pd->clockrev) {
			dist-=pd->clockrev;

			// process disk index
			pd->idistance=0;
			FdcIndex(pc, drv);
		}

		// set distance
		pd->idistance=dist;
	}
}

// update drive/side selection, cycles are complete when called
void FdcUpdateDrive(PCAPSFDC pc, UDWORD cyc)
{
	// process all drives
	for (int drv=0; drv < pc->drivemax; drv++) {
		// get drive
		PCAPSDRIVE pd=pc->drive+drv;

		// set side selected
		pd->side=pd->newside;

		// skip stopped index counter
		int ic=pd->ipcnt;
		if (!ic)
			continue;

		// initialise counter to distance from index after start, add 1 to avoid 0 value (stopped)
		if (ic < 0) {
			pd->ipcnt=pd->idistance+1;
			continue;
		}

		// add cycles to counter
		ic+=cyc;

		// stop index counter when expired
		if (ic > pd->clockip) {
			ic=0;

			// clear index signal if active drive counter stopped
			if (drv == pc->driveact)
				pc->r_st0&=~CAPSFDC_SR_IP_DRQ;
		}

		pd->ipcnt=ic;
	}

	// no changes if drive selection is the same
	if (pc->drivenew == pc->drivesel)
		return;

	// drive selection changed, data lock lost
	pc->datalock=-1;

	// if no drive selected, clear wp, ip, tr0, enable force change
	if (pc->drivenew < 0) {
		pc->drivenew=-1;
		pc->drivesel=-1;
		pc->driveact=-1;
		pc->driveprc=NULL;
		pc->r_st0&=~(CAPSFDC_SR_WP|CAPSFDC_SR_TR0_LD|CAPSFDC_SR_IP_DRQ);
		return;
	}

	// select new drive
	pc->drivesel=pc->drivenew;

	// if invalid drive selected, clear wp, ip, tr0
	if (pc->drivenew >= pc->drivemax) {
		pc->driveact=-1;
		pc->driveprc=NULL;
		pc->r_st0&=~(CAPSFDC_SR_WP|CAPSFDC_SR_TR0_LD|CAPSFDC_SR_IP_DRQ);
		return;
	}

	// new drive is valid
	pc->driveact=pc->drivenew;
	pc->driveprc=pc->drive+pc->driveact;

	// set motor state
	UDWORD attr=pc->driveprc->diskattr;
	if (pc->lineout & CAPSFDC_LO_MO)
		attr|=CAPSDRIVE_DA_MO;
	else
		attr&=~CAPSDRIVE_DA_MO;
	pc->driveprc->diskattr=attr;

	// check selected drive, wp, ip, tr0 default to clear
	UDWORD st0=pc->r_st0 & ~(CAPSFDC_SR_WP|CAPSFDC_SR_TR0_LD|CAPSFDC_SR_IP_DRQ);

	// get tr0
	if (!pc->driveprc->track)
		st0|=CAPSFDC_SR_TR0_LD;

	if (attr & CAPSDRIVE_DA_IN) {
		// disk inserted, get wp
		if (attr & CAPSDRIVE_DA_WP)
			st0 |= CAPSFDC_SR_WP;
	} else {
		// no disk, set wp
		st0|=CAPSFDC_SR_WP;
	}

	// set index signal if counter is active on selected drive
	if (pc->driveprc->ipcnt)
		st0|=CAPSFDC_SR_IP_DRQ;

	// set new status
	pc->r_st0=st0;
}

// reset data access logic
void FdcResetData(PCAPSFDC pc)
{
	pc->seclen=0;
	pc->amdecode=0;
	pc->aminfo=0;
	pc->amisigmask=0;
	pc->amdatadelay=2;
	pc->amdataskip=0;
	pc->ammarkdist=0;
	pc->ammarktype=0;
	pc->dsr=0;
	pc->dsrcnt=0;
	pc->datalock=-1;
	pc->datamode=0;
	pc->datacycle=0;
	pc->indexcount=0;
	FdcResetAm(pc);
}

// reset am detector; read returns only on AM detected or clocks elapsed
void FdcResetAm(PCAPSFDC pc, int keepphase)
{
	pc->aminfo|=CAPSFDC_AI_AMDETENABLE|CAPSFDC_AI_CRCENABLE;
	pc->aminfo&=~(CAPSFDC_AI_CRCACTIVE|CAPSFDC_AI_AMACTIVE);
	pc->amisigmask=CAPSFDC_AI_DSRAM;
	if (!keepphase) {
		pc->dataphase=0;
		pc->datapcnt=0;
	}
}

// clear track data
void FdcClearTrackData(PCAPSDRIVE pd)
{
	// no track data available
	pd->ttype=ctitNA;
	pd->trackbuf=NULL;
	pd->timebuf=NULL;
	pd->tracklen=0;
	pd->overlap=-1;
	pd->trackbits=0;
	pd->ovlmin=-1;
	pd->ovlmax=-1;
	pd->ovlcnt=0;
}

// update data access logic
void FdcUpdateData(PCAPSFDC pc)
{
	// if no drive selected
	if (!pc->driveprc) {
		// and still locked, keep state
		if (pc->datalock >= 0)
			return;

		// lock on data
		FdcLockData(pc);
		return;
	}

	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// keep state if track.side are the same as buffered data
	if (pd->track==pd->buftrack && pd->side==pd->bufside) {
		// keep state if still locked
		if (pc->datalock >= 0)
			return;

		// lock on data
		FdcLockData(pc);
		return;
	}

	// no track data
	FdcClearTrackData(pd);

	// set buffered data track.side
	pd->buftrack=pd->track;
	pd->bufside=pd->side;

	// request track data if disk inserted
	if (pd->diskattr & CAPSDRIVE_DA_IN) {
		// this is required to keep the host informed about the track change, even if the data may not be used later
		pc->cbtrk(pc, pc->driveact);

		// destroy track data if:
		// - drive is single sided and side 1 is being accessed
		// This will result in noise being fed to the DATA line. 
		// Since ttype is reset, track update callbacks won't happen - no need to replicate this code anywhere else.
		if ((pd->diskattr & CAPSDRIVE_DA_SS) && (pd->bufside == 1))
			FdcClearTrackData(pd);
	}

	// update track parameters
	FdcUpdateTrack(pc, pc->driveact);

	// lock on data
	FdcLockData(pc);
}

// update track parameters
void FdcUpdateTrack(PCAPSFDC pc, int drive)
{
	// must be valid drive
	if (drive < 0)
		return;

	// get selected drive
	PCAPSDRIVE pd=pc->drive+drive;

	// no disk inserted, no input
	if (!(pd->diskattr & CAPSDRIVE_DA_IN))
		return;

	// get base track length in bits
	if (!pd->trackbuf || !pd->tracklen) {
		// generate different track size on track updates
		PSDWORD ns=fdcddnoise;
		pd->trackbits=ns[pd->nact++]<<3;
		if (ns[pd->nact] < 0)
			pd->nact=0;
		pd->overlap=0;
	} else
		pd->trackbits=pd->tracklen<<3;

	// if overlap position exists
	int rb;
	if (pd->overlap >= 0) {
		// rob bit count, updated on track updates
		rb=fdcrobbit[pd->ovlact++];
		if (fdcrobbit[pd->ovlact] < 0)
			pd->ovlact=0;
	} else
		rb=0;

	pd->ovlcnt=rb;

	// set bits to remove and their start if any
	if (rb) {
		pd->ovlmin=((pd->overlap+1)<<3)-rb;
		pd->ovlmax=pd->ovlmin+rb-1;
	} else {
		pd->ovlmin=-1;
		pd->ovlmax=-1;
	}
}

// set data locking
void FdcLockData(PCAPSFDC pc)
{
	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// check input available
	if (!pd || !(pd->diskattr & CAPSDRIVE_DA_IN)) {
		// if no input read 0 on reference clock
		pc->datamode=cfdcdmNoline;

		// lock onto reference clock 2us mfm*(8clock+8data)
		pc->datalock=UDWORD(((UQUAD)pc->clockfrq*2*16)/1000000);
		pc->datacycle=0;
		return;
	}

	// true if track data is defined
	int dat=pd->trackbuf && pd->tracklen;

	// density map is only used when track data is available
	if (pd->timebuf && dat) {
		pc->datamode=cfdcdmDMap;
		FdcLockTime(pc);
		return;
	}

	// noise or data input read
	pc->datamode=dat ? cfdcdmData : cfdcdmNoise;

	// lock onto bit in progress at the drive cycle
	pc->datalock=UDWORD(((UQUAD)pd->idistance*pd->trackbits)/pd->clockrev);
	pc->datacycle=0;
}

// set data locking using the density map
void FdcLockTime(PCAPSFDC pc)
{
	PCAPSDRIVE pd=pc->driveprc;
	int lo=0;
	int hi=pd->tracklen-1;
	UDWORD sum=pd->timebuf[hi];

	// binary search for the nearest item to the actual clock distance
	while (lo <= hi) {
		int mid=(lo+hi)/2;
		UDWORD atm=UDWORD(((UQUAD)pd->timebuf[mid]*pd->clockrev)/sum);

		if (atm <= pd->idistance)
			lo=mid+1;
		else
			hi=mid-1;
	}

	// safety check can only happen if distance was >= clockrev
	if ((unsigned)lo >= pd->tracklen)
		lo=pd->tracklen-1;

	UDWORD base=lo ? pd->timebuf[lo-1] : 0;
	UDWORD diff=pd->timebuf[lo]-base;

	// linear search for nearest distance
	for (hi=1; hi < 8; hi++) {
		UDWORD atm=UDWORD(((UQUAD)(base+(hi*diff)/8)*pd->clockrev)/sum);
		if (pd->idistance < atm)
			break;
	}

	pc->datalock=lo*8+hi-1;
	pc->datacycle=base;
}

// get read access call
FDCREAD FdcGetReadAccess(PCAPSFDC pc)
{
	// get hook
	switch (pc->datamode) {
		case cfdcdmNoline:
			return FdcComReadNoline;

		case cfdcdmNoise:
			return FdcComReadNoise;

		case cfdcdmData:
			return FdcComReadData;

		case cfdcdmDMap:
			return FdcComReadDMap;

		default:
			NODEFAULT;
	}

	// shouldn't happen
	return FdcComReadNoline;
}

// read no input until a byte assembled or clock expires
int FdcComReadNoline(PCAPSFDC pc)
{
	// clear dsr signals
	pc->aminfo&=~(CAPSFDC_AI_DSRREADY|CAPSFDC_AI_DSRAM|CAPSFDC_AI_DSRMA1);

	// clock cycles to complete
	UDWORD cmax=pc->clockreq-pc->clockact;

	// set byte read time if expired
	if (!pc->datacycle)
		pc->datacycle=pc->datalock;

	// if byte can be read in this round
	if (cmax >= pc->datacycle) {
		// spend remaining time
		pc->clockact+=pc->datacycle;
		pc->datacycle=0;
		pc->dsr=0;
		return 1;
	}

	// takes more time to complete than what is available
	pc->datacycle-=cmax;
	pc->clockact=pc->clockreq;
	return 0;
}

// read noise until a byte assembled or clock expires
int FdcComReadNoise(PCAPSFDC pc)
{
	// clear dsr signals
	pc->aminfo&=~(CAPSFDC_AI_DSRREADY|CAPSFDC_AI_DSRAM|CAPSFDC_AI_DSRMA1);

	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// all cycles possibly advanced this time
	UDWORD dist=pd->idistance+(pc->clockreq-pc->clockact);

	// next bit complete divident, increased by clockrev
	UQUAD nextccn=(UQUAD)(pc->datalock+1)*pd->clockrev;

	// process until broken
	while (true) {
		// next bit is complete at this disk cycle
		UDWORD nextcyc=UDWORD(nextccn/pd->trackbits);

		// if not enough time to read bit, return
		if (dist < nextcyc) {
			pd->idistance=dist;
			pc->clockact=pc->clockreq;
			return 0;
		}

		// read bit into shifter
		FdcShiftBit(pc);

		// next bit
		pc->datalock++;
		nextccn+=pd->clockrev;

		// if index reached
		if (nextcyc >= pd->clockrev) {
			// adjust to index position
			dist-=pd->clockrev;
			pc->clockact+=pd->clockrev-pd->idistance;
			pd->idistance=0;
			pc->datalock=0;
			nextccn=pd->clockrev;

			// process disk index
			FdcIndex(pc, pc->driveact);

			// we read a complete byte exactly at index, set flag
			if (pc->aminfo & pc->amisigmask)
				return 1;

			// stop on end
			if (pc->endrequest)
				return 0;
		}

		// if a byte is complete, break and signal new byte
		if (pc->aminfo & pc->amisigmask) {
			int dst=pd->idistance;
			pd->idistance=nextcyc;
			pc->clockact+=nextcyc-dst;
			return 1;
		}
	}
}

// read data until a byte assembled or clock expires
int FdcComReadData(PCAPSFDC pc)
{
	// clear dsr signals
	pc->aminfo&=~(CAPSFDC_AI_DSRREADY|CAPSFDC_AI_DSRAM|CAPSFDC_AI_DSRMA1);

	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// all cycles possibly advanced this time
	UDWORD dist=pd->idistance+(pc->clockreq-pc->clockact);

	// next bit complete divident, increased by clockrev
	UQUAD nextccn=(UQUAD)(pc->datalock+1)*pd->clockrev;

	// process until broken
	while (true) {
		// next bit is complete at this disk cycle
		UDWORD nextcyc=UDWORD(nextccn/pd->trackbits);

		// if not enough time to read bit, return
		if (dist < nextcyc) {
			pd->idistance=dist;
			pc->clockact=pc->clockreq;
			return 0;
		}

		// read bit into shifter
		FdcShiftBit(pc);

		// next bit
		pc->datalock++;
		nextccn+=pd->clockrev;

		// if index reached
		if (nextcyc >= pd->clockrev) {
			// adjust to index position
			dist-=pd->clockrev;
			pc->clockact+=pd->clockrev-pd->idistance;
			pd->idistance=0;
			pc->datalock=0;
			nextccn=pd->clockrev;

			// process disk index
			FdcIndex(pc, pc->driveact);

			// we read a complete byte exactly at index, set flag
			if (pc->aminfo & pc->amisigmask)
				return 1;

			// stop on end
			if (pc->endrequest)
				return 0;
		}

		// if a byte is complete, break and signal new byte
		if (pc->aminfo & pc->amisigmask) {
			int dst=pd->idistance;
			pd->idistance=nextcyc;
			pc->clockact+=nextcyc-dst;
			return 1;
		}
	}
}

// read data with density map until a byte assembled or clock expires
int FdcComReadDMap(PCAPSFDC pc)
{
	// clear dsr signals
	pc->aminfo&=~(CAPSFDC_AI_DSRREADY|CAPSFDC_AI_DSRAM|CAPSFDC_AI_DSRMA1);

	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// all cycles possibly advanced this time
	UDWORD dist=pd->idistance+(pc->clockreq-pc->clockact);

	// bit complete
	UDWORD sum=pd->timebuf[pd->tracklen-1];
	UDWORD base=pc->datacycle;
	PUDWORD time=pd->timebuf+(pc->datalock>>3);
	UDWORD diff=*time-base;
	int hi=(pc->datalock&7)+1;

	// process until broken
	while (true) {
		// bit is complete at this disk cycle
		UDWORD nextcyc=UDWORD(((UQUAD)(base+(hi*diff)/8)*pd->clockrev)/sum);

		// if not enough time to read bit, return
		if (dist < nextcyc) {
			pd->idistance=dist;
			pc->clockact=pc->clockreq;
			return 0;
		}

		// read bit into shifter
		FdcShiftBit(pc);

		// next bit
		pc->datalock++;

		// next bit timing
		if (hi == 8) {
			hi=1;
			base=*time++;
			pc->datacycle=base;
			diff=*time-base;
		} else
			hi++;

		// if index reached
		if (nextcyc >= pd->clockrev) {
			// adjust to index position
			dist-=pd->clockrev;
			pc->clockact+=pd->clockrev-pd->idistance;
			pd->idistance=0;
			pc->datalock=0;
			base=0;
			pc->datacycle=0;
			time=pd->timebuf;
			diff=*time;
			hi=1;

			// process disk index
			FdcIndex(pc, pc->driveact);

			// we read a complete byte exactly at index, set flag
			if (pc->aminfo & pc->amisigmask)
				return 1;

			// stop on end
			if (pc->endrequest)
				return 0;
		}

		// if a byte is complete, break and signal new byte
		if (pc->aminfo & pc->amisigmask) {
			int dst=pd->idistance;
			pd->idistance=nextcyc;
			pc->clockact+=nextcyc-dst;
			return 1;
		}
	}
}

// read data bit from track buffer, nonzero if bit is true
int FdcReadBit(PCAPSFDC pc)
{
	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// get current position
	int lock=pc->datalock;

	// bit position in track buffer
	PUBYTE trk=pd->trackbuf+(lock>>3);
	UBYTE msk=1<<((lock&7)^7);

	return *trk & msk;
}

// generate random data bit, nonzero if bit is true
int FdcReadBitNoise(PCAPSFDC pc)
{
	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// bit position
	UDWORD msk=1<<((pc->datalock&7)^7);

	// random data
	UDWORD seed=pd->nseed;

	// accessing the first bit in the stream triggers new data
	if (msk == 0x80) {
		seed<<=1;
		if ((seed>>22 ^ seed) & DF_1)
			seed++;
		pd->nseed=seed;
	}

	return seed & msk;
}

// add data bit to shifter and am detector
void FdcShiftBit(PCAPSFDC pc)
{
	// get selected drive
	PCAPSDRIVE pd=pc->driveprc;

	// do nothing in bit skip area
	if (pc->datalock<=pd->ovlmax && pc->datalock>=pd->ovlmin)
		return;

	// read real or random data
	int bval=(pc->datamode == cfdcdmNoise) ? FdcReadBitNoise(pc) : FdcReadBit(pc);

	// shift read bit into am decoder
	UDWORD amdecode=pc->amdecode<<1;
	if (bval)
		amdecode|=1;
	pc->amdecode=amdecode;

	// get am info, clear AM found, A1 and C2 mark signals
	UDWORD aminfo=pc->aminfo & ~(CAPSFDC_AI_AMFOUND|CAPSFDC_AI_MARKA1|CAPSFDC_AI_MARKC2);

	// bitcell distance of last mark detected
	if (pc->ammarkdist)
		pc->ammarkdist--;

	// am detector if enabled
	if (aminfo & CAPSFDC_AI_AMDETENABLE) {
		// not a mark in shifter/decoder
		int amt=0;

		// check if shifter/decoder has a mark
		// the real hardware probably has two shifters (clocked and fed by data separator) connected to a decoder
		// each bit of the shifter/decoder is for shifter0#0, shifter1#0...shifter0#7, shifter1#7, 
		// only 2 comparisons is needed per cell instead of 8 (A1/0A, C2/14 and 0A/A1, 14/C2)
		switch (amdecode & 0xffff) {
			// A1 mark only enabled if not overlapped with another A1
			case 0x4489:
				if (!pc->ammarkdist || pc->ammarktype!=1)
					amt=1;
				break;

			// C2 mark always enabled
			case 0x5224:
				amt=2;
				break;
		}

		// process mark if found
		if (amt) {
			// we just read the last data bit of a mark, delay by a clock bit to read from decoder#1
			pc->amdatadelay=1;

			// if overlapped with a different mark
			if (pc->ammarkdist && pc->ammarktype!=amt) {
				// dsr value is invalid
				pc->amdataskip++;

				// delay by an additional data bit (data, clock)
				pc->amdatadelay+=2;
			}

			// if dsr is empty dsr shouldn't be flushed, dsr value is invalid
			if (!pc->dsrcnt)
				pc->amdataskip++;

			// force dsr to flush its current value, since data values start from next data bit
			pc->dsrcnt=7;

			// 16 bitcells must be read before next mark, otherwise marks are overlapped (8 clock+data bits)
			pc->ammarkdist=16;

			// save last mark type; only used when marks overlap
			pc->ammarktype=amt;

			// set mark signal, the shifter/decoder must be connected to the crc logic, not dsr
			if (amt == 1) {
				aminfo|=CAPSFDC_AI_MARKA1|CAPSFDC_AI_MA1ACTIVE;

				// if CRC is enabled, first A1 mark activates the CRC logic
				if (aminfo & CAPSFDC_AI_CRCENABLE) {
					// if CRC logic is not activate yet reset CRC value and counter, 16 cells already available as mark
					if (!(aminfo & CAPSFDC_AI_CRCACTIVE)) {
						aminfo|=CAPSFDC_AI_CRCACTIVE;
						pc->crc=~0;
						pc->crccnt=16;
					}
				}
			} else
				aminfo|=CAPSFDC_AI_MARKC2;
		}
	}

	// process CRC if activated
	if (aminfo & CAPSFDC_AI_CRCACTIVE) {
		// process new value at every 16 cells (8 clock/data)
		if (!(pc->crccnt & 0xf)) {
			// reset CRC process if less than 3 consecutive A1 marks detected
			if (pc->crccnt>48 || (aminfo & CAPSFDC_AI_MARKA1)) {
				// 3 consecutive A1 marks found: set AM found signal, disable AM detector
				if (pc->crccnt == 48) {
					aminfo|=CAPSFDC_AI_AMFOUND|CAPSFDC_AI_AMACTIVE;
					aminfo&=~CAPSFDC_AI_AMDETENABLE;
				}

				// select data bits for CRC source byte value; needed without separate clock and data shifters
				int value=0;
				for (int mask=0x4000; mask; mask>>=2) {
					value<<=1;
					if (amdecode & mask)
						value|=1;
				}

				// update CRC
				UWORD crc=(UWORD)pc->crc;
				pc->crc=crctab_ccitt[value^(crc>>8)] ^ (crc << 8);
			} else
				aminfo&=~(CAPSFDC_AI_CRCACTIVE|CAPSFDC_AI_AMACTIVE);
		}

		pc->crccnt++;
	}

	// wait for data clock cycle plus bitcell delay
	if (!pc->amdatadelay) {
		// set next delay
		// just read a clock bit here, next cell is data, that gets processed at the clock bit after that
		pc->amdatadelay=1;

		// clear dsr signals
		aminfo&=~(CAPSFDC_AI_DSRREADY|CAPSFDC_AI_DSRAM|CAPSFDC_AI_DSRMA1);

		// shift data bit into dsr, this is a clock bit, data bit is at decoder#1
		pc->dsr=((pc->dsr<<1) | (amdecode>>1 & 1)) & 0xff;

		// process data if 8 bits are dsr now, otherwise just increase dsr counter
		if (++pc->dsrcnt == 8) {
			// reset dsr counter
			pc->dsrcnt=0;

			// if AM found set dsr signal
			if (aminfo & CAPSFDC_AI_AMACTIVE) {
				aminfo&=~CAPSFDC_AI_AMACTIVE;
				aminfo|=CAPSFDC_AI_DSRAM;
			}

			// if A1 mark found set dsr signal
			if (aminfo & CAPSFDC_AI_MA1ACTIVE) {
				aminfo&=~CAPSFDC_AI_MA1ACTIVE;
				aminfo|=CAPSFDC_AI_DSRMA1;
			}

			// set dsr ready signal, unless data is invalid
			if (!pc->amdataskip)
				aminfo|=CAPSFDC_AI_DSRREADY;
			else
				pc->amdataskip--;
		}
	} else
		pc->amdatadelay--;

	// save new am info
	pc->aminfo=aminfo;
}

// index processing for any drive
void FdcIndex(PCAPSFDC pc, int drive)
{
	// must be valid drive
	if (drive < 0)
		return;

	// get selected drive
	PCAPSDRIVE pd=pc->drive+drive;

	// if drive empty, no index signal, stop
	if (!(pd->diskattr & CAPSDRIVE_DA_IN))
		return;

	// start index pulse
	pd->ipcnt=-1;

	// if track is changing, request new track data
	if ((pd->ttype & CTIT_FLAG_FLAKEY))
		pc->cbtrk(pc, drive);

	// update track parameters
	FdcUpdateTrack(pc, drive);

	// if drive not selected, index signal not routed, stop
	if (drive != pc->driveact)
		return;

	// set index signal
	pc->r_st0|=CAPSFDC_SR_IP_DRQ;

	// increment index pulse counter
	pc->indexcount++;

	// if index abort allowed and limit reached set end and disable abort
	if (pc->indexlimit>=0 && pc->indexcount>=pc->indexlimit) {
		pc->endrequest|=CAPSFDC_ER_COMEND;
		pc->indexlimit=-1;
	}

	// set spin-up flag if limit reached
	if (pc->spinupcnt>=pc->spinuplimit || ++pc->spinupcnt>=pc->spinuplimit)
		pc->r_st0|=CAPSFDC_SR_SU_RT;

	// if fdc is not busy turn motor off once idle limit reached, apparently SR spin-up is not reset
	if (!(pc->r_st0 & CAPSFDC_SR_BUSY)) {
		if (pc->idlecnt>=pc->idlelimit || ++pc->idlecnt>=pc->idlelimit) {
			pc->lineout&=~CAPSFDC_LO_MO;
			pd->diskattr&=~CAPSDRIVE_DA_MO;
			pc->r_st0&=~CAPSFDC_SR_MO;
			pc->spinupcnt=0;
		}
	}

	// set index pulse interrupt if enabled
	if (pc->lineout & CAPSFDC_LO_INTIP)
		FdcSetLine(pc, pc->lineout | CAPSFDC_LO_INTRQ);
}

// change lineout states atomically
void FdcSetLine(PCAPSFDC pc, UDWORD lineout)
{
	// forced interrupt keeps intrq set
	if (lineout & CAPSFDC_LO_INTFRC)
		lineout|=CAPSFDC_LO_INTRQ;

	// get old line state
	UDWORD oldline=pc->lineout;

	// check for drq set
	if (lineout & CAPSFDC_LO_DRQSET) {
		lineout&=~CAPSFDC_LO_DRQSET;
		lineout|=CAPSFDC_LO_DRQ;

		// data lost, if trying to set drq and drq is already set
		if (oldline & CAPSFDC_LO_DRQ)
			pc->r_st1|=CAPSFDC_SR_TR0_LD;
	}

	// changed bits are 1
	UDWORD chgmask=oldline ^ lineout;

	// set new line state
	pc->lineout=lineout;

	// drq change
	if (chgmask & CAPSFDC_LO_DRQ) {
		UDWORD setting=lineout & CAPSFDC_LO_DRQ;
		if (setting)
			pc->r_st1|=CAPSFDC_SR_IP_DRQ;
		else
			pc->r_st1&=~CAPSFDC_SR_IP_DRQ;
		pc->cbdrq(pc, setting);
	}

	// irq change
	if (chgmask & CAPSFDC_LO_INTRQ)
		pc->cbirq(pc, lineout & CAPSFDC_LO_INTRQ);
}



/*
void __cdecl FdcHackChange(PCAPSFDC pc, UDWORD state)
{
	static int imgload=false;
	static int filesize, sector, side, sectorsize, sectorcnt, tracklen, trackcnt;
	static PUBYTE buf;
	static PUBYTE conv=NULL;

	if (!imgload) {
		imgload=true;
		if (CAPSLockImage(0, "a") != imgeOk) {
			CFile file;
			if (!file.Open("a", CFile::modeRead|CFile::shareDenyWrite)) {
				AfxMessageBox("File 'A' not found");
				return;
			}
			filesize=file.GetLength();
			buf=new UBYTE[filesize];
			file.Read(buf, filesize);
			sectorsize=512;
			sectorcnt=filesize/512;
			if (filesize%512)
				AfxMessageBox("Bad file 'A'");
			for (int tc=80, found=false; tc <= 84; tc++) {
				for (int sc=9; sc <= 11; sc++) {
					int sdiv=sectorcnt/sc;
					int smod=sectorcnt%sc;
					if (!smod) {
						if (sdiv==tc || (sdiv==2*tc)) {
							side=sdiv/tc;
							trackcnt=sdiv/side;
							sector=sc;
							found=true;
							break;
						}
					}
				}

				if (found)
					break;
			}

			if (!found)
				AfxMessageBox("Not supported, please report");

			tracklen=sectorsize*sector;
			conv=new UBYTE[32768];
		}
		else
			if (CAPSLoadImage(0, DI_LOCK_DENALT|DI_LOCK_DENVAR|DI_LOCK_UPDATEFD) != imgeOk)
				AfxMessageBox("Bad file 'A'");
	}

	PCAPSDRIVE pd=pc->drive+state;

	if (!conv) {
		CapsTrackInfoT1 cti;
		cti.type=1;
		if (CAPSLockTrack(&cti, 0, pd->buftrack, pd->bufside, 
				DI_LOCK_DENALT|DI_LOCK_DENVAR|DI_LOCK_UPDATEFD|DI_LOCK_TYPE) != imgeOk)
			return;

		pd->ttype=cti.type;
		pd->trackbuf=cti.trackbuf;
		pd->timebuf=cti.timebuf;
		pd->tracklen=cti.tracklen;
		pd->overlap=cti.overlap;
		return;
	}

	if (pd->buftrack<0 || pd->buftrack>=trackcnt)
		return;

	if (pd->bufside<0 || pd->bufside>=side)
		return;

	CapsFormatBlock blk[16];
	CapsFormatTrack trk;
	memset(blk, 0, sector*sizeof(CapsFormatBlock));
	memset(&trk, 0, sizeof(CapsFormatTrack));

	trk.type=0;
	trk.gapacnt=(sector < 11) ? 60 : 2;
	trk.gapavalue=0x4e;
	trk.gapbvalue=0x4e;
	trk.trackbuf=conv;
	trk.tracklen=12500;
	trk.buflen=32768;
	trk.startpos=0;
	trk.blockcnt=sector;
	trk.block=blk;

	for (int sec=0; sec < sector; sec++) {
		PCAPSFORMATBLOCK pb=trk.block+sec;
		pb->gapacnt=(sector < 11) ? 12 : 8;
		pb->gapavalue=0;
		pb->gapbcnt=22;
		pb->gapbvalue=0x4e;
		pb->gapccnt=(sector < 11) ? 12 : 6;
		pb->gapcvalue=0;
		pb->gapdcnt=(sector < 11) ? 40 : 4;
		pb->gapdvalue=0x4e;
		pb->blocktype=cfrmbtData;
		pb->track=pd->buftrack;
		pb->side=pd->bufside;
		pb->sector=sec+1;
		pb->sectorlen=sectorsize;
		pb->databuf=buf;
		pb->databuf+=tracklen*side*pd->buftrack;
		pb->databuf+=tracklen*pd->bufside;
		pb->databuf+=sec*sectorsize;
	}

	if (CAPSFormatDataToMFM(&trk, DI_LOCK_TYPE) != imgeOk)
		return;

	pd->ttype=ctitAuto;
	pd->trackbuf=trk.trackbuf;
	pd->tracklen=trk.tracklen;
	pd->overlap=trk.startpos;
}
*/
