	REM
/*****************************************************************************
 Name    : GetKeyMapData.c
 Project : RetroPlatform Player
 Support : http://www.retroplatform.com
 Legal   : Copyright 2016 Cloanto Italia srl - All rights reserved. This
         : file is multi-licensed under the terms of the Mozilla Public License
         : version 2.0 as published by Mozilla Corporation and the GNU General
         : Public License, version 2 or later, as published by the Free
         : Software Foundation.
 Authors : os
 Created : 2016-07-12 09:58:12
 Comment : this guest-side Amiga code allocates and initializes a keymap data buffer
           which can be used in RP_IPC_TO_HOST_KEYBOARDLAYOUT notifications
 *****************************************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <devices/inputevent.h>
#include <devices/console.h>
#include <devices/keymap.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <stdlib.h>
#include <string.h>

struct KeyStringData
{
	UBYTE nLength;
	UBYTE nOffset;
};

struct KeyDeadData
{
	UBYTE nType;
	UBYTE nValue;
};

struct KeyMapData
{
	WORD nHighestDeadKeyIndex;
	WORD nHighestDoubleDeadKeyIndex;
	WORD nDoubleDeadOffset;
	WORD nDeadableKeyCount;
	ULONG nDeadableKeyDataCount;
	ULONG nSize;
	void *pData;
	ULONG nDataPos;
};

struct KeyMapDataHeader
{
	UBYTE nVersion;
	UBYTE nDeadableKeyDataCount;
};
#define KDH_VERSION 1 // current version



/*****************************************************************************
 Name      : GetQualifierCount
 Arguments : UBYTE type - 
 Return    : int        - 
 Comment   : 
 *****************************************************************************/

static int GetQualifierCount(UBYTE type)
{
	int nCount = 0;
	if (type & KCF_SHIFT)
		nCount += 1;
	if (type & KCF_ALT)
		nCount += 1;
	if (type & KCF_CONTROL)
		nCount += 1;
	return nCount;
}

/*****************************************************************************
 Name      : ExcludeKey
 Arguments : int nKey - 
 Return    : int      - 
 Comment   : 
 *****************************************************************************/

static int ExcludeKey(int nKey)
{
	return (nKey > 0x40 &&
		   nKey != 0x4A && // numapd -
		   nKey != 0x5A && // numapd {
		   nKey != 0x5B && // numpad }
		   nKey != 0x5C && // numpad /
		   nKey != 0x5D && // numpad *
		   nKey != 0x5E)   // numpad +
		   ? 1 : 0;
}

/*****************************************************************************
 Name      : GetKeyMapDataInfo
 Arguments : struct KeyMapData *pKeyMapData - 
           : const UBYTE *pKeyMapTypes      - 
           : const ULONG *pKeyMap           - 
           : int nFirstKey                  - 
 Return    : void
 Comment   : 
 *****************************************************************************/

static void GetKeyMapDataInfo(struct KeyMapData *pKeyMapData, const UBYTE *pKeyMapTypes, const ULONG *pKeyMap, int nFirstKey)
{
	const struct KeyStringData *pStringData;
	const struct KeyDeadData *pDeadData;
	int nLastKey, nCount, nKey, i;
	WORD nIndex;

	nLastKey = nFirstKey + 0x3F;
	for (nKey = nFirstKey; nKey <= nLastKey; nKey++, pKeyMapTypes++, pKeyMap++)
	{
		if (ExcludeKey(nKey))
			continue;
		switch (*pKeyMapTypes & ~KC_VANILLA)
		{
			case KCF_STRING:
				nCount = 1 << GetQualifierCount(*pKeyMapTypes);
				pKeyMapData->nSize += nCount * sizeof(struct KeyStringData);
				pStringData = (const struct KeyStringData *)(*pKeyMap);
				for (i = 0; i < nCount; i++, pStringData++)
					pKeyMapData->nSize += (ULONG)pStringData->nLength;
				break;
			case KCF_DEAD:
				nCount = 1 << GetQualifierCount(*pKeyMapTypes);
				pKeyMapData->nSize += nCount * sizeof(struct KeyDeadData);
				pDeadData = (const struct KeyDeadData *)(*pKeyMap);
				for (i = 0; i < nCount; i++, pDeadData++)
				{
					if (pDeadData->nType == DPF_DEAD)
					{
						nIndex = (WORD)((pDeadData->nValue >> DP_2DFACSHIFT) & DP_2DINDEXMASK);
						if (nIndex)
						{
							pKeyMapData->nDoubleDeadOffset = nIndex;
							nIndex = (WORD)(pDeadData->nValue & DP_2DINDEXMASK);
							if (nIndex > pKeyMapData->nHighestDoubleDeadKeyIndex)
								pKeyMapData->nHighestDoubleDeadKeyIndex = nIndex;
						}
						else
						{
							nIndex = (WORD)(pDeadData->nValue & DP_2DINDEXMASK);
							if (nIndex > pKeyMapData->nHighestDeadKeyIndex)
								pKeyMapData->nHighestDeadKeyIndex = nIndex;
						}
					}
					else if (pDeadData->nType == DPF_MOD)
						pKeyMapData->nDeadableKeyCount += 1;
				}
				break;
		}
	}
}

/*****************************************************************************
 Name      : AddKeyMapData
 Arguments : struct KeyMapData *pKeyMapData - 
           : const void *pData              - 
           : ULONG nSize                    - 
 Return    : void *                         - 
 Comment   : 
 *****************************************************************************/

static void *AddKeyMapData(struct KeyMapData *pKeyMapData, const void *pData, ULONG nSize)
{
	void *pCopiedData;
	pCopiedData = pKeyMapData->pData;
	memcpy(pKeyMapData->pData, pData, nSize);
	pKeyMapData->pData = (UBYTE *)pKeyMapData->pData + nSize;
	pKeyMapData->nDataPos += nSize;
	return pCopiedData;
}

/*****************************************************************************
 Name      : CopyKeyMapData
 Arguments : struct KeyMapData *pKeyMapData - 
           : const UBYTE *pKeyMapTypes      - 
           : UBYTE *pKeyMapTypesDest        - 
           : const ULONG *pKeyMap           - 
           : ULONG *pKeyMapDest             - 
           : int nFirstKey                  - 
 Return    : void
 Comment   : 
 *****************************************************************************/

static void CopyKeyMapData(struct KeyMapData *pKeyMapData, const UBYTE *pKeyMapTypes, UBYTE *pKeyMapTypesDest, const ULONG *pKeyMap, ULONG *pKeyMapDest, int nFirstKey)
{
	const struct KeyStringData *pStringData;
	const struct KeyDeadData *pDeadData;
	struct KeyStringData *pStringDataDest;
	struct KeyDeadData *pDeadDataDest;
	int nLastKey, nCount, nKey, i;
	UBYTE nByteOffset, tp;

	nLastKey = nFirstKey + 0x3F;
	for (nKey = nFirstKey; nKey <= nLastKey; nKey++, pKeyMapTypes++, pKeyMapTypesDest++, pKeyMap++, pKeyMapDest++)
	{
		tp = ExcludeKey(nKey) ? KCF_NOP : *pKeyMapTypes;
		switch (tp & ~KC_VANILLA)
		{
			case 0:
				break;
			case KCF_STRING:
				nCount = 1 << GetQualifierCount(*pKeyMapTypes);
				pStringData = (const struct KeyStringData *)(*pKeyMap);
				*pKeyMapDest = pKeyMapData->nDataPos;
				pStringDataDest = AddKeyMapData(pKeyMapData, pStringData, nCount * sizeof(struct KeyStringData));
				nByteOffset = (UBYTE)(nCount * sizeof(struct KeyStringData));

				for (i = 0; i < nCount; i++, pStringData++, pStringDataDest++)
				{
					pStringDataDest->nLength = pStringData->nLength;
					pStringDataDest->nOffset = nByteOffset;
					AddKeyMapData(pKeyMapData, (UBYTE *)*pKeyMap + pStringData->nOffset, pStringData->nLength);
					nByteOffset += pStringData->nLength;
				}
				break;
			case KCF_DEAD:
				nCount = 1 << GetQualifierCount(*pKeyMapTypes);
				pDeadData = (const struct KeyDeadData *)(*pKeyMap);
				*pKeyMapDest = pKeyMapData->nDataPos;
				pDeadDataDest = AddKeyMapData(pKeyMapData, pDeadData, nCount * sizeof(struct KeyDeadData));
				nByteOffset = (UBYTE)(nCount * sizeof(struct KeyDeadData));

				for (i = 0; i < nCount; i++, pDeadData++, pDeadDataDest++)
				{
					pDeadDataDest->nType = pDeadData->nType;
					if (pDeadData->nType == DPF_MOD)
					{
						pDeadDataDest->nValue = nByteOffset;
						AddKeyMapData(pKeyMapData, (UBYTE *)(*pKeyMap) + pDeadData->nValue, pKeyMapData->nDeadableKeyDataCount);
						nByteOffset += (UBYTE)pKeyMapData->nDeadableKeyDataCount;
					}
					else pDeadDataDest->nValue = pDeadData->nValue;
				}
				break;
			default:
				*pKeyMapTypesDest = KCF_NOP;
				break;
		}
	}
}

/*****************************************************************************
 Name      : GetKeyMapData
 Arguments : const struct KeyMap *pKeyMap - 
           : ULONG *pnSize                - 
 Return    : void *                       - use FreeMem() to free the returned data
 Comment   : 
 *****************************************************************************/

void *GetKeyMapData(const struct KeyMap *pKeyMap, ULONG *pnSize)
{
	struct KeyMapData kd;
	struct KeyMapDataHeader kdh;
	ULONG *pLoKeyMap, *pHiKeyMap;
	UBYTE *pLoKeyMapTypes, *pHiKeyMapTypes;
	void *pData;

	if (!pKeyMap || !pnSize)
		return NULL;

	if (pKeyMap->km_LoKeyMapTypes == NULL ||
		pKeyMap->km_HiKeyMapTypes == NULL ||
		pKeyMap->km_LoKeyMap == NULL ||
		pKeyMap->km_HiKeyMap == NULL ||
		pKeyMap->km_LoCapsable == NULL ||
		pKeyMap->km_HiCapsable == NULL ||
		pKeyMap->km_LoRepeatable == NULL ||
		pKeyMap->km_HiRepeatable == NULL)
		return NULL;

	memset(&kd, 0, sizeof(kd));
	kd.nSize = (sizeof(struct KeyMapDataHeader) +
		        0x40 + // km_LoKeyMapTypes data size
	            0x40 + // km_HiKeyMapTypes data size
	            (0x40 * sizeof(ULONG)) + // km_LoKeyMap data size
	            (0x40 * sizeof(ULONG)) + // km_HiKeyMap data size
	            8 + // km_LoCapsable data size
	            8 + // km_HiCapsable data size
	            8 + // km_LoRepeatable data size
	            8); // km_HiRepeatable data size

	GetKeyMapDataInfo(&kd, pKeyMap->km_LoKeyMapTypes, pKeyMap->km_LoKeyMap, 0);
	GetKeyMapDataInfo(&kd, pKeyMap->km_HiKeyMapTypes, pKeyMap->km_HiKeyMap, 0x40);

	if (kd.nDeadableKeyCount)
	{
		kd.nDeadableKeyDataCount = kd.nDoubleDeadOffset ? ((WORD)(1 + kd.nHighestDoubleDeadKeyIndex) * (WORD)kd.nDoubleDeadOffset) : (1 + kd.nHighestDeadKeyIndex); // WORD cast needed to use MULS.W asm operator in place of CXM3 functon call (long multiplication)
		kd.nSize += (WORD)kd.nDeadableKeyCount * (WORD)kd.nDeadableKeyDataCount; // WORD cast needed to use MULS.W asm operator in place of CXM3 functon call (long multiplication)
	}
	pData = AllocMem(kd.nSize, MEMF_ANY);
	if (pData)
	{
		kd.pData = pData;
		kdh.nVersion = KDH_VERSION;
		kdh.nDeadableKeyDataCount = kd.nDeadableKeyDataCount;
		AddKeyMapData(&kd, &kdh, sizeof(struct KeyMapDataHeader));
		pLoKeyMapTypes = AddKeyMapData(&kd, pKeyMap->km_LoKeyMapTypes, 0x40);
		pHiKeyMapTypes = AddKeyMapData(&kd, pKeyMap->km_HiKeyMapTypes, 0x40);
		pLoKeyMap = AddKeyMapData(&kd, pKeyMap->km_LoKeyMap, (0x40 * sizeof(ULONG)));
		pHiKeyMap = AddKeyMapData(&kd, pKeyMap->km_HiKeyMap, (0x40 * sizeof(ULONG)));
		AddKeyMapData(&kd, pKeyMap->km_LoCapsable, 8);
		AddKeyMapData(&kd, pKeyMap->km_HiCapsable, 8);
		AddKeyMapData(&kd, pKeyMap->km_LoRepeatable, 8);
		AddKeyMapData(&kd, pKeyMap->km_HiRepeatable, 8);
		CopyKeyMapData(&kd, pKeyMap->km_LoKeyMapTypes, pLoKeyMapTypes, pKeyMap->km_LoKeyMap, pLoKeyMap, 0);
		CopyKeyMapData(&kd, pKeyMap->km_HiKeyMapTypes, pHiKeyMapTypes, pKeyMap->km_HiKeyMap, pHiKeyMap, 0x40);
		*pnSize = kd.nSize;
	}
	return pData;
}

	EREM

xAllocMem
	move.l 8(sp),d1
	move.l 4(sp),d0
	jsr -$c6(a6)
	rts

GetQualifierCount:
              MOVEM.L        D6/D7,-(A7)              ;48e7 0300 
___GetQualifierCount__1:
              MOVE.B         $f(A7),D7                ;1e2f 000f 
              MOVEQ.L        #$0,D6                   ;7c00 
              BTST           #$0,D7                   ;0807 0000 
              BEQ.B          ___GetQualifierCount__3  ;6702 
___GetQualifierCount__2:
              ADDQ.L         #$1,D6                   ;5286 
___GetQualifierCount__3:
              BTST           #$1,D7                   ;0807 0001 
              BEQ.B          ___GetQualifierCount__5  ;6702 
___GetQualifierCount__4:
              ADDQ.L         #$1,D6                   ;5286 
___GetQualifierCount__5:
              BTST           #$2,D7                   ;0807 0002 
              BEQ.B          ___GetQualifierCount__7  ;6702 
___GetQualifierCount__6:
              ADDQ.L         #$1,D6                   ;5286 
___GetQualifierCount__7:
              MOVE.L         D6,D0                    ;2006 
___GetQualifierCount__8:
___GetQualifierCount__9:
              MOVEM.L        (A7)+,D6/D7              ;4cdf 00c0 
              RTS                                     ;4e75 
ExcludeKey:
              MOVE.L         D7,-(A7)                 ;2f07 
___ExcludeKey__1:
              MOVE.L         $8(A7),D7                ;2e2f 0008 
              MOVEQ.L        #$40,D0                  ;7040 
              CMP.L          D0,D7                    ;be80 
              BLE.B          ___ExcludeKey__9         ;6f28 
___ExcludeKey__2:
              MOVEQ.L        #$4a,D0                  ;704a 
              CMP.L          D0,D7                    ;be80 
              BEQ.B          ___ExcludeKey__9         ;6722 
___ExcludeKey__3:
              MOVEQ.L        #$5a,D0                  ;705a 
              CMP.L          D0,D7                    ;be80 
              BEQ.B          ___ExcludeKey__9         ;671c 
___ExcludeKey__4:
              MOVEQ.L        #$5b,D0                  ;705b 
              CMP.L          D0,D7                    ;be80 
              BEQ.B          ___ExcludeKey__9         ;6716 
___ExcludeKey__5:
              MOVEQ.L        #$5c,D0                  ;705c 
              CMP.L          D0,D7                    ;be80 
              BEQ.B          ___ExcludeKey__9         ;6710 
___ExcludeKey__6:
              MOVEQ.L        #$5d,D0                  ;705d 
              CMP.L          D0,D7                    ;be80 
              BEQ.B          ___ExcludeKey__9         ;670a 
___ExcludeKey__7:
              MOVEQ.L        #$5e,D0                  ;705e 
              CMP.L          D0,D7                    ;be80 
              BEQ.B          ___ExcludeKey__9         ;6704 
___ExcludeKey__8:
              MOVEQ.L        #$1,D0                   ;7001 
              BRA.B          ___ExcludeKey__10        ;6002 
___ExcludeKey__9:
              MOVEQ.L        #$0,D0                   ;7000 
___ExcludeKey__10:
___ExcludeKey__11:
___ExcludeKey__12:
              MOVE.L         (A7)+,D7                 ;2e1f 
              RTS                                     ;4e75 
GetKeyMapDataInfo:
              SUB.W          #$14,A7                  ;9efc 0014 
              MOVEM.L        D2/D3/D5/D6/D7/A2/A3/A5,-(A7);48e7 3734 
___GetKeyMapDataInfo__1:
              MOVE.L         $44(A7),D7               ;2e2f 0044 
              MOVE.L         $40(A7),A2               ;246f 0040 
              MOVE.L         $3c(A7),A3               ;266f 003c 
              MOVE.L         $38(A7),A5               ;2a6f 0038 
              MOVE.L         D7,D6                    ;2c07 
              MOVEQ.L        #$3f,D0                  ;703f 
              ADD.L          D0,D6                    ;dc80 
              MOVE.L         D7,$28(A7)               ;2f47 0028 
___GetKeyMapDataInfo__2:
              MOVE.L         $28(A7),D0               ;202f 0028 
              CMP.L          D6,D0                    ;b086 
              BGT.W          ___GetKeyMapDataInfo__33 ;6e00 00fe 
___GetKeyMapDataInfo__3:
              MOVE.L         D0,-(A7)                 ;2f00 
              BSR.B          ExcludeKey               ;6196 
___GetKeyMapDataInfo__4:
              ADDQ.W         #$4,A7                   ;584f 
              TST.L          D0                       ;4a80 
              BNE.W          ___GetKeyMapDataInfo__31 ;6600 00e6 
___GetKeyMapDataInfo__5:
___GetKeyMapDataInfo__6:
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         (A3),D0                  ;1013 
              ANDI.W         #$fffffff8,D0            ;0240 fff8 
___GetKeyMapDataInfo__7:
              MOVEQ.L        #$20,D1                  ;7220 
              SUB.L          D1,D0                    ;9081 
              BEQ.B          ___GetKeyMapDataInfo__16 ;674a 
___GetKeyMapDataInfo__8:
              MOVEQ.L        #$20,D1                  ;7220 
              SUB.L          D1,D0                    ;9081 
              BNE.W          ___GetKeyMapDataInfo__31 ;6600 00d0 
___GetKeyMapDataInfo__9:
___GetKeyMapDataInfo__10:
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         (A3),D0                  ;1013 
              MOVE.L         D0,-(A7)                 ;2f00 
              BSR.W          GetQualifierCount        ;6100 ff46 
___GetKeyMapDataInfo__11:
              ADDQ.W         #$4,A7                   ;584f 
              MOVEQ.L        #$0,D1                   ;7200 
              BSET           D0,D1                    ;01c1 
              MOVE.L         D1,D5                    ;2a01 
              MOVE.L         D5,D0                    ;2005 
              ADD.L          D0,D0                    ;d080 
              ADD.L          D0,$c(A5)                ;d1ad 000c 
              MOVE.L         (A2),$30(A7)             ;2f52 0030 
              CLR.L          $24(A7)                  ;42af 0024 
___GetKeyMapDataInfo__12:
              MOVE.L         $24(A7),D0               ;202f 0024 
              CMP.L          D5,D0                    ;b085 
              BGE.W          ___GetKeyMapDataInfo__31 ;6c00 00a4 
___GetKeyMapDataInfo__13:
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.L         $30(A7),A0               ;206f 0030 
              MOVE.B         (A0),D0                  ;1010 
              ADD.L          D0,$c(A5)                ;d1ad 000c 
___GetKeyMapDataInfo__14:
              ADDQ.L         #$1,$24(A7)              ;52af 0024 
              ADDQ.L         #$2,$30(A7)              ;54af 0030 
              BRA.B          ___GetKeyMapDataInfo__12 ;60e0 
___GetKeyMapDataInfo__15:
___GetKeyMapDataInfo__16:
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         (A3),D0                  ;1013 
              MOVE.L         D0,-(A7)                 ;2f00 
              BSR.W          GetQualifierCount        ;6100 ff04 
___GetKeyMapDataInfo__17:
              ADDQ.W         #$4,A7                   ;584f 
              MOVEQ.L        #$0,D1                   ;7200 
              BSET           D0,D1                    ;01c1 
              MOVE.L         D1,D5                    ;2a01 
              MOVE.L         D5,D0                    ;2005 
              ADD.L          D0,D0                    ;d080 
              ADD.L          D0,$c(A5)                ;d1ad 000c 
              MOVE.L         (A2),$2c(A7)             ;2f52 002c 
              CLR.L          $24(A7)                  ;42af 0024 
___GetKeyMapDataInfo__18:
              MOVE.L         $24(A7),D0               ;202f 0024 
              CMP.L          D5,D0                    ;b085 
              BGE.B          ___GetKeyMapDataInfo__31 ;6c62 
___GetKeyMapDataInfo__19:
              MOVE.L         $2c(A7),A0               ;206f 002c 
              MOVE.B         (A0),D0                  ;1010 
              MOVEQ.L        #$8,D1                   ;7208 
              CMP.B          D1,D0                    ;b001 
              BNE.B          ___GetKeyMapDataInfo__27 ;6644 
___GetKeyMapDataInfo__20:
              MOVEQ.L        #$0,D1                   ;7200 
              MOVE.B         $1(A0),D1                ;1228 0001 
              MOVE.L         D1,D2                    ;2401 
              ASR.L          #$4,D2                   ;e882 
              MOVEQ.L        #$f,D3                   ;760f 
              AND.L          D3,D2                    ;c483 
              MOVE.W         D2,$22(A7)               ;3f42 0022 
              BEQ.B          ___GetKeyMapDataInfo__24 ;671e 
___GetKeyMapDataInfo__21:
              MOVE.W         D2,$4(A5)                ;3b42 0004 
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         $1(A0),D0                ;1028 0001 
              MOVE.L         D0,D1                    ;2200 
              AND.L          D3,D1                    ;c283 
              MOVE.W         D1,$22(A7)               ;3f41 0022 
              CMP.W          $2(A5),D1                ;b26d 0002 
              BLE.B          ___GetKeyMapDataInfo__29 ;6f20 
___GetKeyMapDataInfo__22:
              MOVE.W         D1,$2(A5)                ;3b41 0002 
___GetKeyMapDataInfo__23:
              BRA.B          ___GetKeyMapDataInfo__29 ;601a 
___GetKeyMapDataInfo__24:
              MOVEQ.L        #$0,D2                   ;7400 
              MOVE.B         D1,D2                    ;1401 
              AND.L          D3,D2                    ;c483 
              MOVE.W         D2,$22(A7)               ;3f42 0022 
              CMP.W          (A5),D2                  ;b455 
              BLE.B          ___GetKeyMapDataInfo__29 ;6f0c 
___GetKeyMapDataInfo__25:
              MOVE.W         D2,(A5)                  ;3a82 
___GetKeyMapDataInfo__26:
              BRA.B          ___GetKeyMapDataInfo__29 ;6008 
___GetKeyMapDataInfo__27:
              SUBQ.B         #$1,D0                   ;5300 
              BNE.B          ___GetKeyMapDataInfo__29 ;6604 
___GetKeyMapDataInfo__28:
              ADDQ.W         #$1,$6(A5)               ;526d 0006 
___GetKeyMapDataInfo__29:
              ADDQ.L         #$1,$24(A7)              ;52af 0024 
              ADDQ.L         #$2,$2c(A7)              ;54af 002c 
              BRA.B          ___GetKeyMapDataInfo__18 ;6096 
___GetKeyMapDataInfo__30:
___GetKeyMapDataInfo__31:
              ADDQ.L         #$1,$28(A7)              ;52af 0028 
              ADDQ.L         #$1,A3                   ;528b 
              ADDQ.L         #$4,A2                   ;588a 
              BRA.W          ___GetKeyMapDataInfo__2  ;6000 fefc 
___GetKeyMapDataInfo__32:
___GetKeyMapDataInfo__33:
              MOVEM.L        (A7)+,D2/D3/D5/D6/D7/A2/A3/A5;4cdf 2cec 
              ADD.W          #$14,A7                  ;defc 0014 
              RTS                                     ;4e75 
AddKeyMapData:
              MOVEM.L        D7/A2/A3/A5,-(A7)        ;48e7 0134 
___AddKeyMapData__1:
              MOVE.L         $1c(A7),D7               ;2e2f 001c 
              MOVE.L         $18(A7),A3               ;266f 0018 
              MOVE.L         $14(A7),A5               ;2a6f 0014 
              MOVE.L         $10(A5),A2               ;246d 0010 
              MOVE.L         A3,A0                    ;204b 
              MOVE.L         $10(A5),A1               ;226d 0010 
              MOVE.L         D7,D0                    ;2007 
              BRA.B          ___AddKeyMapData__3      ;6002 
___AddKeyMapData__2:
              MOVE.B         (A0)+,(A1)+              ;12d8 
___AddKeyMapData__3:
              SUBQ.L         #$1,D0                   ;5380 
              BCC.B          ___AddKeyMapData__2      ;64fa 
___AddKeyMapData__4:
              MOVE.L         $10(A5),A0               ;206d 0010 
              ADD.L          D7,A0                    ;d1c7 
              MOVE.L         A0,$10(A5)               ;2b48 0010 
              ADD.L          D7,$14(A5)               ;dfad 0014 
              MOVE.L         A2,D0                    ;200a 
___AddKeyMapData__5:
___AddKeyMapData__6:
              MOVEM.L        (A7)+,D7/A2/A3/A5        ;4cdf 2c80 
              RTS                                     ;4e75 
CopyKeyMapData:
              SUB.W          #$1c,A7                  ;9efc 001c 
              MOVEM.L        D5/D6/D7/A2/A3/A5,-(A7)  ;48e7 0734 
___CopyKeyMapData__1:
              MOVE.L         $4c(A7),D7               ;2e2f 004c 
              MOVE.L         $40(A7),A2               ;246f 0040 
              MOVE.L         $3c(A7),A3               ;266f 003c 
              MOVE.L         $38(A7),A5               ;2a6f 0038 
              MOVE.L         D7,D6                    ;2c07 
              MOVEQ.L        #$3f,D0                  ;703f 
              ADD.L          D0,D6                    ;dc80 
              MOVE.L         D7,$20(A7)               ;2f47 0020 
___CopyKeyMapData__2:
              MOVE.L         $20(A7),D0               ;202f 0020 
              CMP.L          D6,D0                    ;b086 
              BGT.W          ___CopyKeyMapData__34    ;6e00 01a0 
___CopyKeyMapData__3:
              MOVE.L         D0,-(A7)                 ;2f00 
              BSR.W          ExcludeKey               ;6100 fe2a 
___CopyKeyMapData__4:
              ADDQ.W         #$4,A7                   ;584f 
              TST.L          D0                       ;4a80 
              BEQ.B          ___CopyKeyMapData__6     ;6706 
___CopyKeyMapData__5:
              MOVEQ.L        #$40,D0                  ;7040 
              ADD.L          D0,D0                    ;d080 
              BRA.B          ___CopyKeyMapData__7     ;6004 
___CopyKeyMapData__6:
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         (A3),D0                  ;1013 
___CopyKeyMapData__7:
              MOVEQ.L        #$0,D1                   ;7200 
              MOVE.B         D0,D1                    ;1200 
              ANDI.W         #$fffffff8,D1            ;0241 fff8 
              MOVE.B         D0,$1a(A7)               ;1f40 001a 
___CopyKeyMapData__8:
              TST.L          D1                       ;4a81 
              BEQ.W          ___CopyKeyMapData__32    ;6700 0164 
___CopyKeyMapData__9:
              MOVEQ.L        #$20,D0                  ;7020 
              SUB.L          D0,D1                    ;9280 
              BEQ.W          ___CopyKeyMapData__21    ;6700 00a8 
___CopyKeyMapData__10:
              MOVEQ.L        #$20,D0                  ;7020 
              SUB.L          D0,D1                    ;9280 
              BEQ.B          ___CopyKeyMapData__13    ;6704 
___CopyKeyMapData__11:
              BRA.W          ___CopyKeyMapData__31    ;6000 014e 
___CopyKeyMapData__12:
___CopyKeyMapData__13:
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         (A3),D0                  ;1013 
              MOVE.L         D0,-(A7)                 ;2f00 
              BSR.W          GetQualifierCount        ;6100 fdc2 
___CopyKeyMapData__14:
              MOVEQ.L        #$0,D1                   ;7200 
              BSET           D0,D1                    ;01c1 
              MOVE.L         D1,D5                    ;2a01 
              MOVE.L         $48(A7),A1               ;226f 0048 
              MOVE.L         (A1),A0                  ;2051 
              MOVE.L         $4c(A7),A1               ;226f 004c 
              MOVE.L         $14(A5),(A1)             ;22ad 0014 
              MOVE.L         D5,D0                    ;2005 
              ADD.L          D0,D0                    ;d080 
              MOVE.L         D0,(A7)                  ;2e80 
              MOVE.L         A0,-(A7)                 ;2f08 
              MOVE.L         A5,-(A7)                 ;2f0d 
              MOVE.L         A0,$3c(A7)               ;2f48 003c 
              BSR.W          AddKeyMapData            ;6100 ff32 
___CopyKeyMapData__15:
              LEA            $c(A7),A7                ;4fef 000c 
              MOVE.L         D5,D1                    ;2205 
              ADD.L          D1,D1                    ;d281 
              CLR.L          $1c(A7)                  ;42af 001c 
              MOVE.L         D0,$28(A7)               ;2f40 0028 
              MOVE.B         D1,$1b(A7)               ;1f41 001b 
___CopyKeyMapData__16:
              MOVE.L         $1c(A7),D0               ;202f 001c 
              CMP.L          D5,D0                    ;b085 
              BGE.W          ___CopyKeyMapData__32    ;6c00 0104 
___CopyKeyMapData__17:
              MOVE.L         $30(A7),A0               ;206f 0030 
              MOVE.B         (A0),D0                  ;1010 
              MOVE.L         $28(A7),A0               ;206f 0028 
              MOVE.B         D0,(A0)                  ;1080 
              MOVE.B         $1b(A7),$1(A0)           ;116f 001b 0001 
              MOVE.L         $44(A7),A1               ;226f 0044 
              MOVE.L         (A1),A0                  ;2051 
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.L         $30(A7),A1               ;226f 0030 
              MOVE.B         $1(A1),D0                ;1029 0001 
              ADD.W          D0,A0                    ;d0c0 
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         (A1),D0                  ;1011 
              MOVE.L         D0,-(A7)                 ;2f00 
              MOVE.L         A0,-(A7)                 ;2f08 
              MOVE.L         A5,-(A7)                 ;2f0d 
              BSR.W          AddKeyMapData            ;6100 fee2 
___CopyKeyMapData__18:
              LEA            $c(A7),A7                ;4fef 000c 
              MOVE.L         $30(A7),A0               ;206f 0030 
              MOVE.B         (A0),D0                  ;1010 
              ADD.B          D0,$1b(A7)               ;d12f 001b 
___CopyKeyMapData__19:
              ADDQ.L         #$1,$1c(A7)              ;52af 001c 
              ADDQ.L         #$2,$30(A7)              ;54af 0030 
              ADDQ.L         #$2,$28(A7)              ;54af 0028 
              BRA.B          ___CopyKeyMapData__16    ;60a8 
___CopyKeyMapData__20:
___CopyKeyMapData__21:
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.B         (A3),D0                  ;1013 
              MOVE.L         D0,-(A7)                 ;2f00 
              BSR.W          GetQualifierCount        ;6100 fd26 
___CopyKeyMapData__22:
              MOVEQ.L        #$0,D1                   ;7200 
              BSET           D0,D1                    ;01c1 
              MOVE.L         D1,D5                    ;2a01 
              MOVE.L         $48(A7),A1               ;226f 0048 
              MOVE.L         (A1),A0                  ;2051 
              MOVE.L         $4c(A7),A1               ;226f 004c 
              MOVE.L         $14(A5),(A1)             ;22ad 0014 
              MOVE.L         D5,D0                    ;2005 
              ADD.L          D0,D0                    ;d080 
              MOVE.L         D0,(A7)                  ;2e80 
              MOVE.L         A0,-(A7)                 ;2f08 
              MOVE.L         A5,-(A7)                 ;2f0d 
              MOVE.L         A0,$38(A7)               ;2f48 0038 
              BSR.W          AddKeyMapData            ;6100 fe96 
___CopyKeyMapData__23:
              LEA            $c(A7),A7                ;4fef 000c 
              MOVE.L         D5,D1                    ;2205 
              ADD.L          D1,D1                    ;d281 
              CLR.L          $1c(A7)                  ;42af 001c 
              MOVE.L         D0,$24(A7)               ;2f40 0024 
              MOVE.B         D1,$1b(A7)               ;1f41 001b 
___CopyKeyMapData__24:
              MOVE.L         $1c(A7),D0               ;202f 001c 
              CMP.L          D5,D0                    ;b085 
              BGE.B          ___CopyKeyMapData__32    ;6c68 
___CopyKeyMapData__25:
              MOVE.L         $2c(A7),A0               ;206f 002c 
              MOVE.B         (A0),D0                  ;1010 
              MOVE.L         $24(A7),A0               ;206f 0024 
              MOVE.B         D0,(A0)                  ;1080 
              MOVEQ.L        #$1,D0                   ;7001 
              MOVE.L         $2c(A7),A0               ;206f 002c 
              CMP.B          (A0),D0                  ;b010 
              BNE.B          ___CopyKeyMapData__28    ;6636 
___CopyKeyMapData__26:
              MOVE.L         $24(A7),A0               ;206f 0024 
              MOVE.B         $1b(A7),$1(A0)           ;116f 001b 0001 
              MOVE.L         $44(A7),A1               ;226f 0044 
              MOVE.L         (A1),A0                  ;2051 
              MOVEQ.L        #$0,D0                   ;7000 
              MOVE.L         $2c(A7),A1               ;226f 002c 
              MOVE.B         $1(A1),D0                ;1029 0001 
              ADD.W          D0,A0                    ;d0c0 
              MOVE.L         $8(A5),-(A7)             ;2f2d 0008 
              MOVE.L         A0,-(A7)                 ;2f08 
              MOVE.L         A5,-(A7)                 ;2f0d 
              BSR.W          AddKeyMapData            ;6100 fe3c 
___CopyKeyMapData__27:
              LEA            $c(A7),A7                ;4fef 000c 
              MOVE.L         $8(A5),D0                ;202d 0008 
              ADD.B          D0,$1b(A7)               ;d12f 001b 
              BRA.B          ___CopyKeyMapData__29    ;600a 
___CopyKeyMapData__28:
              MOVE.L         $24(A7),A1               ;226f 0024 
              MOVE.B         $1(A0),$1(A1)            ;1368 0001 0001 
___CopyKeyMapData__29:
              ADDQ.L         #$1,$1c(A7)              ;52af 001c 
              ADDQ.L         #$2,$2c(A7)              ;54af 002c 
              ADDQ.L         #$2,$24(A7)              ;54af 0024 
              BRA.B          ___CopyKeyMapData__24    ;6094 
___CopyKeyMapData__30:
___CopyKeyMapData__31:
              MOVE.B         #$80,(A2)                ;14bc 0080 
___CopyKeyMapData__32:
              ADDQ.L         #$1,$20(A7)              ;52af 0020 
              ADDQ.L         #$1,A3                   ;528b 
              ADDQ.L         #$1,A2                   ;528a 
              ADDQ.L         #$4,$44(A7)              ;58af 0044 
              ADDQ.L         #$4,$48(A7)              ;58af 0048 
              BRA.W          ___CopyKeyMapData__2     ;6000 fe5a 
___CopyKeyMapData__33:
___CopyKeyMapData__34:
              MOVEM.L        (A7)+,D5/D6/D7/A2/A3/A5  ;4cdf 2ce0 
              ADD.W          #$1c,A7                  ;defc 001c 
              RTS                                     ;4e75 
GetKeyMapData:
              SUB.W          #$2c,A7                  ;9efc 002c 
              MOVEM.L        D2/A2/A3/A5,-(A7)        ;48e7 2034 
___GetKeyMapData__1:
              MOVE.L         $44(A7),A3               ;266f 0044 
              MOVE.L         $40(A7),A5               ;2a6f 0040 
              MOVE.L         A5,D0                    ;200d 
              BEQ.B          ___GetKeyMapData__3      ;6704 
___GetKeyMapData__2:
              MOVE.L         A3,D0                    ;200b 
              BNE.B          ___GetKeyMapData__4      ;6606 
___GetKeyMapData__3:
              MOVEQ.L        #$0,D0                   ;7000 
              BRA.W          ___GetKeyMapData__37     ;6000 01a2 
___GetKeyMapData__4:
              TST.L          (A5)                     ;4a95 
              BEQ.B          ___GetKeyMapData__12     ;672a 
___GetKeyMapData__5:
              TST.L          $10(A5)                  ;4aad 0010 
              BEQ.B          ___GetKeyMapData__12     ;6724 
___GetKeyMapData__6:
              TST.L          $4(A5)                   ;4aad 0004 
              BEQ.B          ___GetKeyMapData__12     ;671e 
___GetKeyMapData__7:
              TST.L          $14(A5)                  ;4aad 0014 
              BEQ.B          ___GetKeyMapData__12     ;6718 
___GetKeyMapData__8:
              TST.L          $8(A5)                   ;4aad 0008 
              BEQ.B          ___GetKeyMapData__12     ;6712 
___GetKeyMapData__9:
              TST.L          $18(A5)                  ;4aad 0018 
              BEQ.B          ___GetKeyMapData__12     ;670c 
___GetKeyMapData__10:
              TST.L          $c(A5)                   ;4aad 000c 
              BEQ.B          ___GetKeyMapData__12     ;6706 
___GetKeyMapData__11:
              TST.L          $1c(A5)                  ;4aad 001c 
              BNE.B          ___GetKeyMapData__13     ;6606 
___GetKeyMapData__12:
              MOVEQ.L        #$0,D0                   ;7000 
              BRA.W          ___GetKeyMapData__37     ;6000 016e 
___GetKeyMapData__13:
              MOVEQ.L        #$0,D0                   ;7000 
              LEA            $24(A7),A0               ;41ef 0024 
              MOVEQ.L        #$17,D1                  ;7217 
___GetKeyMapData__14:
              MOVE.B         D0,(A0)+                 ;10c0 
              DBRA           D1,___GetKeyMapData__14  ;51c9 fffc 
___GetKeyMapData__15:
              MOVE.L         #$2a2,$30(A7)            ;2f7c 0000 02a2 0030 
              MOVE.L         D0,-(A7)                 ;2f00 
              MOVE.L         $4(A5),-(A7)             ;2f2d 0004 
              MOVE.L         (A5),-(A7)               ;2f15 
              PEA            $30(A7)                  ;486f 0030 
              BSR.W          GetKeyMapDataInfo        ;6100 fc4a 
___GetKeyMapData__16:
              PEA            ($40).w                  ;4878 0040 
              MOVE.L         $14(A5),-(A7)            ;2f2d 0014 
              MOVE.L         $10(A5),-(A7)            ;2f2d 0010 
              PEA            $40(A7)                  ;486f 0040 
              BSR.W          GetKeyMapDataInfo        ;6100 fc36 
___GetKeyMapData__17:
              LEA            $20(A7),A7               ;4fef 0020 
              MOVE.W         $2a(A7),D0               ;302f 002a 
              BEQ.B          ___GetKeyMapData__22     ;6724 
___GetKeyMapData__18:
              MOVE.W         $28(A7),D1               ;322f 0028 
              BEQ.B          ___GetKeyMapData__20     ;670a 
___GetKeyMapData__19:
              MOVE.W         $26(A7),D2               ;342f 0026 
              ADDQ.W         #$1,D2                   ;5242 
              MULS.W         D1,D2                    ;c5c1 
              BRA.B          ___GetKeyMapData__21     ;600a 
___GetKeyMapData__20:
              MOVE.W         $24(A7),D1               ;322f 0024 
              EXT.L          D1                       ;48c1 
              ADDQ.L         #$1,D1                   ;5281 
              MOVE.L         D1,D2                    ;2401 
___GetKeyMapData__21:
              MOVE.L         D2,$2c(A7)               ;2f42 002c 
              MULS.W         D2,D0                    ;c1c2 
              ADD.L          D0,$30(A7)               ;d1af 0030 
___GetKeyMapData__22:
              CLR.L          -(A7)                    ;42a7 
              MOVE.L         $34(A7),-(A7)            ;2f2f 0034 
              BSR.W          xAllocMem                ;6100 0000 
              ADDQ.W         #$8,A7                   ;504f 
              MOVE.L         D0,$14(A7)               ;2f40 0014 
              BEQ.W          ___GetKeyMapData__35     ;6700 00ee 
___GetKeyMapData__23:
              MOVE.L         D0,$34(A7)               ;2f40 0034 
              MOVE.B         #$1,$12(A7)              ;1f7c 0001 0012 
              MOVE.L         $2c(A7),D0               ;202f 002c 
              MOVE.B         D0,$13(A7)               ;1f40 0013 
              PEA            ($2).w                   ;4878 0002 
              PEA            $16(A7)                  ;486f 0016 
              PEA            $2c(A7)                  ;486f 002c 
              BSR.W          AddKeyMapData            ;6100 fd04 
___GetKeyMapData__24:
              PEA            ($40).w                  ;4878 0040 
              MOVE.L         (A5),-(A7)               ;2f15 
              PEA            $38(A7)                  ;486f 0038 
              BSR.W          AddKeyMapData            ;6100 fcf6 
___GetKeyMapData__25:
              PEA            ($40).w                  ;4878 0040 
              MOVE.L         $10(A5),-(A7)            ;2f2d 0010 
              PEA            $44(A7)                  ;486f 0044 
              MOVE.L         D0,$40(A7)               ;2f40 0040 
              BSR.W          AddKeyMapData            ;6100 fce2 
___GetKeyMapData__26:
              PEA            ($100).w                 ;4878 0100 
              MOVE.L         $4(A5),-(A7)             ;2f2d 0004 
              PEA            $50(A7)                  ;486f 0050 
              MOVE.L         D0,$48(A7)               ;2f40 0048 
              BSR.W          AddKeyMapData            ;6100 fcce 
___GetKeyMapData__27:
              MOVE.L         D0,A2                    ;2440 
              PEA            ($100).w                 ;4878 0100 
              MOVE.L         $14(A5),-(A7)            ;2f2d 0014 
              PEA            $5c(A7)                  ;486f 005c 
              BSR.W          AddKeyMapData            ;6100 fcbc 
___GetKeyMapData__28:
              PEA            ($8).w                   ;4878 0008 
              MOVE.L         $8(A5),-(A7)             ;2f2d 0008 
              PEA            $68(A7)                  ;486f 0068 
              MOVE.L         D0,$68(A7)               ;2f40 0068 
              BSR.W          AddKeyMapData            ;6100 fca8 
___GetKeyMapData__29:
              LEA            $48(A7),A7               ;4fef 0048 
              PEA            ($8).w                   ;4878 0008 
              MOVE.L         $18(A5),-(A7)            ;2f2d 0018 
              PEA            $2c(A7)                  ;486f 002c 
              BSR.W          AddKeyMapData            ;6100 fc94 
___GetKeyMapData__30:
              PEA            ($8).w                   ;4878 0008 
              MOVE.L         $c(A5),-(A7)             ;2f2d 000c 
              PEA            $38(A7)                  ;486f 0038 
              BSR.W          AddKeyMapData            ;6100 fc84 
___GetKeyMapData__31:
              PEA            ($8).w                   ;4878 0008 
              MOVE.L         $1c(A5),-(A7)            ;2f2d 001c 
              PEA            $44(A7)                  ;486f 0044 
              BSR.W          AddKeyMapData            ;6100 fc74 
___GetKeyMapData__32:
              CLR.L          (A7)                     ;4297 
              MOVE.L         A2,-(A7)                 ;2f0a 
              MOVE.L         $4(A5),-(A7)             ;2f2d 0004 
              MOVE.L         $48(A7),-(A7)            ;2f2f 0048 
              MOVE.L         (A5),-(A7)               ;2f15 
              PEA            $58(A7)                  ;486f 0058 
              BSR.W          CopyKeyMapData           ;6100 fc98 
___GetKeyMapData__33:
              PEA            ($40).w                  ;4878 0040 
              MOVE.L         $5c(A7),-(A7)            ;2f2f 005c 
              MOVE.L         $14(A5),-(A7)            ;2f2d 0014 
              MOVE.L         $5c(A7),-(A7)            ;2f2f 005c 
              MOVE.L         $10(A5),-(A7)            ;2f2d 0010 
              PEA            $70(A7)                  ;486f 0070 
              BSR.W          CopyKeyMapData           ;6100 fc7c 
___GetKeyMapData__34:
              LEA            $50(A7),A7               ;4fef 0050 
              MOVE.L         $30(A7),(A3)             ;26af 0030 
___GetKeyMapData__35:
              MOVE.L         $14(A7),D0               ;202f 0014 
___GetKeyMapData__36:
___GetKeyMapData__37:
              MOVEM.L        (A7)+,D2/A2/A3/A5        ;4cdf 2c04 
              ADD.W          #$2c,A7                  ;defc 002c 
              RTS                                     ;4e75 
