//	Altirra - Atari 800/800XL/5200 aemulator
//	Coprocessor library - 65802 emulator
//	Copyright (C) 2009-2015 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along
//	with this program. If not, see <http://www.gnu.org/licenses/>.
//
//	As a special exception, this library can also be redistributed and/or
//	modified under an alternate license. See COPYING.RMT in the same source
//	archive for details.

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int32_t sint32;

#ifndef f_ATCOPROC_CO8048_H
#define f_ATCOPROC_CO8048_H

//#include <vd2/system/function.h>

class ATCoProc8048 {
public:
	ATCoProc8048();

	void SetProgramBanks(const void *p0, const void *p1);

	uint16 GetPC() const { return mPC; }

	uint8 GetSP() const { return mPSW & 7; }
	uint32 GetStepStackLevel() const { return (uint32)mPSW << 29; }
	uint8 GetPort1Output() const { return mP1; }
	uint8 GetPort2Output() const { return mP2; }

	uint32 GetTime() const { return mCyclesBase - mCyclesLeft; }
	uint32 GetTimeBase() const { return mCyclesBase; }

	const uint8 *GetInternalRAM() const { return mRAM; }
	uint8 *GetInternalRAM() { return mRAM; }
	uint8 ReadByte(uint8 addr) const;
	void WriteByte(uint8 addr, uint8 val);

	void ColdReset();
	void WarmReset();

	void AssertIrq();
	void NegateIrq();

	uint32 GetTStatesPending() const { return mTStatesLeft; }
	uint32 GetCyclesLeft() const { return mCyclesLeft; }
	void AddCycles(sint32 cycles) { mCyclesBase += cycles;  mCyclesLeft += cycles; }
	void Run();

	void DispatchIrq();
	void UpdateTimer();
	void UpdateTimerDeadline();

	int		mTStatesLeft = 0;

	uint8	mA;
	uint8	mPSW;
	uint8	*mpRegBank;
	const uint8	*mpProgramBanks[2];
	const uint8	*mpProgramBank;

	uint8	mT = 0;
	uint8	mP1 = 0xFF;
	uint8	mP2 = 0xFF;
	bool	mbDBF = false;
	bool	mbPBK = false;
	bool	mbIF = false;
	bool	mbTIF = false;
	bool	mbTF = false;
	bool	mbF1 = false;
	bool	mbIrqEnabled = false;
	bool	mbIrqPending = false;
	bool	mbIrqAttention = false;
	bool	mbTimerIrqPending = false;

	uint16		mPC = 0;
	sint32		mCyclesLeft = 0;
	uint32		mCyclesBase = 0;
	uint32		mCyclesSaved = 0;

	bool	mbTimerActive = false;
	uint32		mTimerDeadline = 0;

	const uint8	*mpNextState = nullptr;
	uint8		mReadOpcodeState = 0;

	alignas(2) uint8	mRAM[256];

	static const uint8 kInitialState;
	static const uint8 kInitialStateNoBreak;
	static const uint8 kIrqSequence[];
};

void mpFnWritePort(int, uint8);
uint8 mpFnReadBus(void);
void mpFnWriteBus(uint8);
uint8 mpFnReadPort(int, uint8);
uint8 mpFnReadT0(void);
uint8 mpFnReadT1(void);

#endif	// f_ATCOPROC_CO8048_H
