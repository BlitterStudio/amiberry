#ifndef CAPSFDCEMULATOR_H
#define CAPSFDCEMULATOR_H

// init state
struct CapsFdcInit {
	int runmode;   // run mode to set
	UDWORD stmask; // status mask register to set (1 bits select st1)
	UDWORD st0clr; // clear these bits in ST0
	UDWORD st0set; // set these bits in ST0
	UDWORD st1clr; // clear these bits in ST1
	UDWORD st1set; // set these bits in ST1
};

typedef struct CapsFdcInit *PCAPSFDCINIT;



typedef void (*FDCCALL)(PCAPSFDC pc);
typedef int (*FDCREAD)(PCAPSFDC pc);

void FdcSetTiming(PCAPSFDC pc);
void FdcInit(PCAPSFDC pc);
void FdcReset(PCAPSFDC pc);
void FdcResetState(PCAPSFDC pc);
void FdcCom(PCAPSFDC pc, UDWORD data);
void FdcComEnd(PCAPSFDC pc);

void FdcComT_NOP(PCAPSFDC pc);
void FdcComT_Idle(PCAPSFDC pc);

void FdcComT1_SpinupStart(PCAPSFDC pc);
void FdcComT1_SpinupLoop(PCAPSFDC pc);
void FdcComT1_StepStart(PCAPSFDC pc);
void FdcComT1_Step(PCAPSFDC pc);
void FdcComT1_StepLoop(PCAPSFDC pc);
void FdcComT1_DelayStart(PCAPSFDC pc);
void FdcComT1_DelayLoop(PCAPSFDC pc);
void FdcComT1_VerifyStart(PCAPSFDC pc);
void FdcComT1_VerifyLoop(PCAPSFDC pc);

void FdcComT2_DelayStart(PCAPSFDC pc);
void FdcComT2_ReadStart(PCAPSFDC pc);
void FdcComT2_ReadLoop(PCAPSFDC pc);

void FdcComT2W(PCAPSFDC pc);

void FdcComT3_IndexStart(PCAPSFDC pc);
void FdcComT3_IndexLoop(PCAPSFDC pc);
void FdcComT3_ReadStart(PCAPSFDC pc);
void FdcComT3_ReadLoop(PCAPSFDC pc);

void FdcComT3_WriteCheck(PCAPSFDC pc);
void FdcComT3_WriteStart(PCAPSFDC pc);
void FdcComT3_WriteLoop(PCAPSFDC pc);

void FdcComT3_AddressLoop(PCAPSFDC pc);

void FdcComT4(PCAPSFDC pc);

UDWORD FdcComIdle(PCAPSFDC pc, UDWORD cyc);
void FdcComIdleOther(PCAPSFDC pc, UDWORD cyc);
void FdcUpdateDrive(PCAPSFDC pc, UDWORD cyc);
void FdcUpdateDrive(PCAPSFDC pc, UDWORD cyc);
void FdcResetData(PCAPSFDC pc);
void FdcResetAm(PCAPSFDC pc, int keepphase=0);
void FdcClearTrackData(PCAPSDRIVE pd);
void FdcUpdateData(PCAPSFDC pc);
void FdcUpdateTrack(PCAPSFDC pc, int drive);
void FdcLockData(PCAPSFDC pc);
void FdcLockTime(PCAPSFDC pc);
FDCREAD FdcGetReadAccess(PCAPSFDC pc);
int FdcComReadNoline(PCAPSFDC pc);
int FdcComReadNoise(PCAPSFDC pc);
int FdcComReadData(PCAPSFDC pc);
int FdcComReadDMap(PCAPSFDC pc);
int FdcReadBit(PCAPSFDC pc);
int FdcReadBitNoise(PCAPSFDC pc);
void FdcShiftBit(PCAPSFDC pc);
void FdcIndex(PCAPSFDC pc, int drive);
void FdcSetLine(PCAPSFDC pc, UDWORD lineout);

void __cdecl FdcHackChange(PCAPSFDC pc, UDWORD state);

#endif
