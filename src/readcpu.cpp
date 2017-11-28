/*
* UAE - The Un*x Amiga Emulator
*
* Read 68000 CPU specs from file "table68k"
*
* Copyright 1995,1996 Bernd Schmidt
*/

#include "sysconfig.h"
#include "sysdeps.h"
#include <ctype.h>

#include "readcpu.h"

int nr_cpuop_funcs;

struct mnemolookup lookuptab[] = {
	{ i_ILLG, _T("ILLEGAL") },
	{ i_OR, _T("OR") },
	{ i_CHK, _T("CHK") },
	{ i_CHK2, _T("CHK2") },
	{ i_AND, _T("AND") },
	{ i_EOR, _T("EOR") },
	{ i_ORSR, _T("ORSR") },
	{ i_ANDSR, _T("ANDSR") },
	{ i_EORSR, _T("EORSR") },
	{ i_SUB, _T("SUB") },
	{ i_SUBA, _T("SUBA") },
	{ i_SUBX, _T("SUBX") },
	{ i_SBCD, _T("SBCD") },
	{ i_ADD, _T("ADD") },
	{ i_ADDA, _T("ADDA") },
	{ i_ADDX, _T("ADDX") },
	{ i_ABCD, _T("ABCD") },
	{ i_NEG, _T("NEG") },
	{ i_NEGX, _T("NEGX") },
	{ i_NBCD, _T("NBCD") },
	{ i_CLR, _T("CLR") },
	{ i_NOT, _T("NOT") },
	{ i_TST, _T("TST") },
	{ i_BTST, _T("BTST") },
	{ i_BCHG, _T("BCHG") },
	{ i_BCLR, _T("BCLR") },
	{ i_BSET, _T("BSET") },
	{ i_CMP, _T("CMP") },
	{ i_CMPM, _T("CMPM") },
	{ i_CMPA, _T("CMPA") },
	{ i_MVPRM, _T("MVPRM") },
	{ i_MVPMR, _T("MVPMR") },
	{ i_MOVE, _T("MOVE") },
	{ i_MOVEA, _T("MOVEA") },
	{ i_MVSR2, _T("MVSR2") },
	{ i_MV2SR, _T("MV2SR") },
	{ i_SWAP, _T("SWAP") },
	{ i_EXG, _T("EXG") },
	{ i_EXT, _T("EXT") },
	{ i_MVMEL, _T("MVMEL"), _T("MOVEM") },
	{ i_MVMLE, _T("MVMLE"), _T("MOVEM") },
	{ i_TRAP, _T("TRAP") },
	{ i_MVR2USP, _T("MVR2USP") },
	{ i_MVUSP2R, _T("MVUSP2R") },
	{ i_NOP, _T("NOP") },
	{ i_RESET, _T("RESET") },
	{ i_RTE, _T("RTE") },
	{ i_RTD, _T("RTD") },
	{ i_LINK, _T("LINK") },
	{ i_UNLK, _T("UNLK") },
	{ i_RTS, _T("RTS") },
	{ i_STOP, _T("STOP") },
	{ i_TRAPV, _T("TRAPV") },
	{ i_RTR, _T("RTR") },
	{ i_JSR, _T("JSR") },
	{ i_JMP, _T("JMP") },
	{ i_BSR, _T("BSR") },
	{ i_Bcc, _T("Bcc") },
	{ i_LEA, _T("LEA") },
	{ i_PEA, _T("PEA") },
	{ i_DBcc, _T("DBcc") },
	{ i_Scc, _T("Scc") },
	{ i_DIVU, _T("DIVU") },
	{ i_DIVS, _T("DIVS") },
	{ i_MULU, _T("MULU") },
	{ i_MULS, _T("MULS") },
	{ i_ASR, _T("ASR") },
	{ i_ASL, _T("ASL") },
	{ i_LSR, _T("LSR") },
	{ i_LSL, _T("LSL") },
	{ i_ROL, _T("ROL") },
	{ i_ROR, _T("ROR") },
	{ i_ROXL, _T("ROXL") },
	{ i_ROXR, _T("ROXR") },
	{ i_ASRW, _T("ASRW") },
	{ i_ASLW, _T("ASLW") },
	{ i_LSRW, _T("LSRW") },
	{ i_LSLW, _T("LSLW") },
	{ i_ROLW, _T("ROLW") },
	{ i_RORW, _T("RORW") },
	{ i_ROXLW, _T("ROXLW") },
	{ i_ROXRW, _T("ROXRW") },

	{ i_MOVE2C, _T("MOVE2C"), _T("MOVEC") },
	{ i_MOVEC2, _T("MOVEC2"), _T("MOVEC") },
	{ i_CAS, _T("CAS") },
	{ i_CAS2, _T("CAS2") },
	{ i_MULL, _T("MULL") },
	{ i_DIVL, _T("DIVL") },
	{ i_BFTST, _T("BFTST") },
	{ i_BFEXTU, _T("BFEXTU") },
	{ i_BFCHG, _T("BFCHG") },
	{ i_BFEXTS, _T("BFEXTS") },
	{ i_BFCLR, _T("BFCLR") },
	{ i_BFFFO, _T("BFFFO") },
	{ i_BFSET, _T("BFSET") },
	{ i_BFINS, _T("BFINS") },
	{ i_PACK, _T("PACK") },
	{ i_UNPK, _T("UNPK") },
	{ i_TAS, _T("TAS") },
	{ i_BKPT, _T("BKPT") },
	{ i_CALLM, _T("CALLM") },
	{ i_RTM, _T("RTM") },
	{ i_TRAPcc, _T("TRAPcc") },
	{ i_MOVES, _T("MOVES") },
	{ i_FPP, _T("FPP") },
	{ i_FDBcc, _T("FDBcc") },
	{ i_FScc, _T("FScc") },
	{ i_FTRAPcc, _T("FTRAPcc") },
	{ i_FBcc, _T("FBcc") },
	{ i_FBcc, _T("FBcc") },
	{ i_FSAVE, _T("FSAVE") },
	{ i_FRESTORE, _T("FRESTORE") },

	{ i_CINVL, _T("CINVL") },
	{ i_CINVP, _T("CINVP") },
	{ i_CINVA, _T("CINVA") },
	{ i_CPUSHL, _T("CPUSHL") },
	{ i_CPUSHP, _T("CPUSHP") },
	{ i_CPUSHA, _T("CPUSHA") },
	{ i_MOVE16, _T("MOVE16") },

	{ i_MMUOP030, _T("MMUOP030") },
	{ i_PFLUSHN, _T("PFLUSHN") },
	{ i_PFLUSH, _T("PFLUSH") },
	{ i_PFLUSHAN, _T("PFLUSHAN") },
	{ i_PFLUSHA, _T("PFLUSHA") },

	{ i_PLPAR, _T("PLPAR") },
	{ i_PLPAW, _T("PLPAW") },
	{ i_PTESTR, _T("PTESTR") },
	{ i_PTESTW, _T("PTESTW") },

	{ i_LPSTOP, _T("LPSTOP") },
	{ i_ILLG, _T("") },
};

struct instr *table68k;

static amodes mode_from_str (const TCHAR *str)
{
	if (_tcsncmp (str, _T("Dreg"), 4) == 0) return Dreg;
	if (_tcsncmp (str, _T("Areg"), 4) == 0) return Areg;
	if (_tcsncmp (str, _T("Aind"), 4) == 0) return Aind;
	if (_tcsncmp (str, _T("Apdi"), 4) == 0) return Apdi;
	if (_tcsncmp (str, _T("Aipi"), 4) == 0) return Aipi;
	if (_tcsncmp (str, _T("Ad16"), 4) == 0) return Ad16;
	if (_tcsncmp (str, _T("Ad8r"), 4) == 0) return Ad8r;
	if (_tcsncmp (str, _T("absw"), 4) == 0) return absw;
	if (_tcsncmp (str, _T("absl"), 4) == 0) return absl;
	if (_tcsncmp (str, _T("PC16"), 4) == 0) return PC16;
	if (_tcsncmp (str, _T("PC8r"), 4) == 0) return PC8r;
	if (_tcsncmp (str, _T("Immd"), 4) == 0) return imm;
	abort ();
	return (amodes)0;
}

STATIC_INLINE amodes mode_from_mr (int mode, int reg)
{
	switch (mode) {
	case 0: return Dreg;
	case 1: return Areg;
	case 2: return Aind;
	case 3: return Aipi;
	case 4: return Apdi;
	case 5: return Ad16;
	case 6: return Ad8r;
	case 7:
		switch (reg) {
		case 0: return absw;
		case 1: return absl;
		case 2: return PC16;
		case 3: return PC8r;
		case 4: return imm;
		case 5:
		case 6:
		case 7: return am_illg;
		}
	}
	abort ();
	return (amodes)0;
}

static void build_insn (int insn)
{
	int find = -1;
	int variants;
	struct instr_def id;
	const TCHAR *opcstr;
	int i, n;

	int flaglive = 0, flagdead = 0;
  int cflow = 0;

  // Mask of flags set/used
  unsigned char flags_set = 0;
  unsigned char flags_used = 0;

	id = defs68k[insn];

  // Control flow information
  cflow = id.cflow;

  for (i = 0, n = 4; i < 5; i++, n--) {
  	switch (id.flaginfo[i].flagset) {
	    case fa_unset: 
      case fa_isjmp: 
        break;
	    default: 
        flags_set |= (1 << n);
  	}

  	switch (id.flaginfo[i].flaguse) {
	    case fu_unused: 
      case fu_isjmp: 
        break;
	    default: 
        flags_used |= (1 << n);
  	}
  }

	for (i = 0; i < 5; i++) {
		switch (id.flaginfo[i].flagset){
		case fa_unset: break;
	 	case fa_isjmp: break;
		case fa_zero: flagdead |= 1 << i; break;
		case fa_one: flagdead |= 1 << i; break;
		case fa_dontcare: flagdead |= 1 << i; break;
		case fa_unknown: flagdead = -1; goto out1;
		case fa_set: flagdead |= 1 << i; break;
		}
	}

out1:
	for (i = 0; i < 5; i++) {
		switch (id.flaginfo[i].flaguse) {
		case fu_unused: break;
		case fu_isjmp: flaglive |= 1 << i; break;
		case fu_maybecc: flaglive |= 1 << i; break;
		case fu_unknown: flaglive = -1; goto out2;
		case fu_used: flaglive |= 1 << i; break;
		}
	}
out2:

	opcstr = id.opcstr;
	for (variants = 0; variants < (1 << id.n_variable); variants++) {
		int bitcnt[lastbit];
		int bitval[lastbit];
		int bitpos[lastbit];
		int i;
		uae_u16 opc = id.bits;
		uae_u16 msk, vmsk;
		int pos = 0;
		int mnp = 0;
		int bitno = 0;
		int unsized = 1;
		TCHAR mnemonic[64];
		int mnemo;

		wordsizes sz = sz_long;
		int srcgather = 0, dstgather = 0;
		int usesrc = 0, usedst = 0;
		int srctype = 0;
		int srcpos = -1, dstpos = -1;

		amodes srcmode = am_unknown, destmode = am_unknown;
		int srcreg = -1, destreg = -1;

		for (i = 0; i < lastbit; i++)
			bitcnt[i] = bitval[i] = 0;

		vmsk = 1 << id.n_variable;

		for (i = 0, msk = 0x8000; i < 16; i++, msk >>= 1) {
			if (!(msk & id.mask)) {
				int currbit = id.bitpos[bitno++];
				int bit_set;
				vmsk >>= 1;
				bit_set = variants & vmsk ? 1 : 0;
				if (bit_set)
					opc |= msk;
				bitpos[currbit] = 15 - i;
				bitcnt[currbit]++;
				bitval[currbit] <<= 1;
				bitval[currbit] |= bit_set;
			}
		}

		if (bitval[bitj] == 0) bitval[bitj] = 8;
		/* first check whether this one does not match after all */
		if (bitval[bitz] == 3 || bitval[bitC] == 1)
			continue;
		if (bitcnt[bitI] && (bitval[bitI] == 0x00 || bitval[bitI] == 0xff))
			continue;

		if (bitcnt[bitE] && (bitval[bitE] == 0x00))
			continue;

		/* bitI and bitC get copied to biti and bitc */
		if (bitcnt[bitI]) {
			bitval[biti] = bitval[bitI]; bitpos[biti] = bitpos[bitI];
		}
		if (bitcnt[bitC])
			bitval[bitc] = bitval[bitC];

		pos = 0;
		while (opcstr[pos] && !_istspace(opcstr[pos])) {
			if (opcstr[pos] == '.') {
				pos++;
				unsized = 0;
				switch (opcstr[pos]) {

				case 'B': sz = sz_byte; break;
				case 'W': sz = sz_word; break;
				case 'L': sz = sz_long; break;
				case 'z':
					switch (bitval[bitz]) {
					case 0: sz = sz_byte; break;
					case 1: sz = sz_word; break;
					case 2: sz = sz_long; break;
					default: abort();
					}
					break;
				default: abort();
				}
			} else {
				mnemonic[mnp] = opcstr[pos];
				if (mnemonic[mnp] == 'f') {
					find = -1;
					switch (bitval[bitf]) {
					case 0: mnemonic[mnp] = 'R'; break;
					case 1: mnemonic[mnp] = 'L'; break;
					default: abort();
					}
				}
				mnp++;
			}
			pos++;
		}
		mnemonic[mnp] = 0;

		/* now, we have read the mnemonic and the size */
		while (opcstr[pos] && _istspace(opcstr[pos]))
			pos++;

		/* A goto a day keeps the D******a away. */
		if (opcstr[pos] == 0)
			goto endofline;

		/* parse the source address */
		usesrc = 1;
		switch (opcstr[pos++]) {
		case 'D':
			srcmode = Dreg;
			switch (opcstr[pos++]) {
			case 'r': srcreg = bitval[bitr]; srcgather = 1; srcpos = bitpos[bitr]; break;
			case 'R': srcreg = bitval[bitR]; srcgather = 1; srcpos = bitpos[bitR]; break;
			default: abort();
			}
			break;
		case 'A':
			srcmode = Areg;
			switch (opcstr[pos++]) {
	    case 'l': srcmode = absl; break;
			case 'r': srcreg = bitval[bitr]; srcgather = 1; srcpos = bitpos[bitr]; break;
			case 'R': srcreg = bitval[bitR]; srcgather = 1; srcpos = bitpos[bitR]; break;
			default: abort();
			}
			switch (opcstr[pos]) {
			case 'p': srcmode = Apdi; pos++; break;
			case 'P': srcmode = Aipi; pos++; break;
			case 'a': srcmode = Aind; pos++; break;
			}
			break;
		case 'L':
			srcmode = absl;
			break;
		case '#':
			switch (opcstr[pos++]) {
			case 'z': srcmode = imm; break;
			case '0': srcmode = imm0; break;
			case '1': srcmode = imm1; break;
			case '2': srcmode = imm2; break;
			case 'i': srcmode = immi; srcreg = (uae_s32)(uae_s8)bitval[biti];
				if (CPU_EMU_SIZE < 4) {
					/* Used for branch instructions */
					srctype = 1;
					srcgather = 1;
					srcpos = bitpos[biti];
				}
				break;
			case 'j': srcmode = immi; srcreg = bitval[bitj];
				if (CPU_EMU_SIZE < 3) {
					/* 1..8 for ADDQ/SUBQ and rotshi insns */
					srcgather = 1;
					srctype = 3;
					srcpos = bitpos[bitj];
				}
				break;
			case 'J': srcmode = immi; srcreg = bitval[bitJ];
				if (CPU_EMU_SIZE < 5) {
					/* 0..15 */
					srcgather = 1;
					srctype = 2;
					srcpos = bitpos[bitJ];
				}
				break;
			case 'k': srcmode = immi; srcreg = bitval[bitk];
				if (CPU_EMU_SIZE < 3) {
					srcgather = 1;
					srctype = 4;
					srcpos = bitpos[bitk];
				}
				break;
			case 'K': srcmode = immi; srcreg = bitval[bitK];
				if (CPU_EMU_SIZE < 5) {
					/* 0..15 */
					srcgather = 1;
					srctype = 5;
					srcpos = bitpos[bitK];
				}
				break;
      case 'E': srcmode = immi; srcreg = bitval[bitE];
		    if (CPU_EMU_SIZE < 5) {
			    /* 1..255 */
			    srcgather = 1;
			    srctype = 6;
			    srcpos = bitpos[bitE];
    		}
    		break;
      case 'p': srcmode = immi; srcreg = bitval[bitp];
    		if (CPU_EMU_SIZE < 5) {
					/* 0..3 */
					srcgather = 1;
					srctype = 7;
					srcpos = bitpos[bitp];
				}
				break;
			default: abort();
			}
			break;
		case 'd':
			srcreg = bitval[bitD];
			srcmode = mode_from_mr(bitval[bitd],bitval[bitD]);
			if (srcmode == am_illg)
				continue;
			if (CPU_EMU_SIZE < 2 &&
				(srcmode == Areg || srcmode == Dreg || srcmode == Aind
				|| srcmode == Ad16 || srcmode == Ad8r || srcmode == Aipi
				|| srcmode == Apdi))
			{
				srcgather = 1; srcpos = bitpos[bitD];
			}
			if (opcstr[pos] == '[') {
				pos++;
				if (opcstr[pos] == '!') {
					/* exclusion */
					do {
						pos++;
						if (mode_from_str(opcstr+pos) == srcmode)
							goto nomatch;
						pos += 4;
					} while (opcstr[pos] == ',');
					pos++;
				} else {
					if (opcstr[pos+4] == '-') {
						/* replacement */
						if (mode_from_str(opcstr+pos) == srcmode)
							srcmode = mode_from_str(opcstr+pos+5);
						else
							goto nomatch;
						pos += 10;
					} else {
						/* normal */
						while(mode_from_str(opcstr+pos) != srcmode) {
							pos += 4;
							if (opcstr[pos] == ']')
								goto nomatch;
							pos++;
						}
						while(opcstr[pos] != ']') pos++;
						pos++;
						break;
					}
				}
			}
			/* Some addressing modes are invalid as destination */
			if (srcmode == imm || srcmode == PC16 || srcmode == PC8r)
				goto nomatch;
			break;
		case 's':
			srcreg = bitval[bitS];
			srcmode = mode_from_mr(bitval[bits],bitval[bitS]);

			if (srcmode == am_illg)
				continue;
			if (CPU_EMU_SIZE < 2 &&
				(srcmode == Areg || srcmode == Dreg || srcmode == Aind
				|| srcmode == Ad16 || srcmode == Ad8r || srcmode == Aipi
				|| srcmode == Apdi))
			{
				srcgather = 1; srcpos = bitpos[bitS];
			}
			if (opcstr[pos] == '[') {
				pos++;
				if (opcstr[pos] == '!') {
					/* exclusion */
					do {
						pos++;
						if (mode_from_str(opcstr+pos) == srcmode)
							goto nomatch;
						pos += 4;
					} while (opcstr[pos] == ',');
					pos++;
				} else {
					if (opcstr[pos+4] == '-') {
						/* replacement */
						if (mode_from_str(opcstr+pos) == srcmode)
							srcmode = mode_from_str(opcstr+pos+5);
						else
							goto nomatch;
						pos += 10;
					} else {
						/* normal */
						while(mode_from_str(opcstr+pos) != srcmode) {
							pos += 4;
							if (opcstr[pos] == ']')
								goto nomatch;
							pos++;
						}
						while(opcstr[pos] != ']') pos++;
						pos++;
					}
				}
			}
			break;
		default: abort();
		}
		/* safety check - might have changed */
		if (srcmode != Areg && srcmode != Dreg && srcmode != Aind
			&& srcmode != Ad16 && srcmode != Ad8r && srcmode != Aipi
			&& srcmode != Apdi && srcmode != immi)
		{
			srcgather = 0;
		}
		if (srcmode == Areg && sz == sz_byte)
			goto nomatch;

		if (opcstr[pos] != ',')
			goto endofline;
		pos++;

		/* parse the destination address */
		usedst = 1;
		switch (opcstr[pos++]) {
		case 'D':
			destmode = Dreg;
			switch (opcstr[pos++]) {
			case 'r': destreg = bitval[bitr]; dstgather = 1; dstpos = bitpos[bitr]; break;
			case 'R': destreg = bitval[bitR]; dstgather = 1; dstpos = bitpos[bitR]; break;
			default: abort();
			}
			if (dstpos < 0 || dstpos >= 32)
				abort ();
			break;
		case 'A':
			destmode = Areg;
			switch (opcstr[pos++]) {
	    case 'l': destmode = absl; break;
			case 'r': destreg = bitval[bitr]; dstgather = 1; dstpos = bitpos[bitr]; break;
			case 'R': destreg = bitval[bitR]; dstgather = 1; dstpos = bitpos[bitR]; break;
			case 'x': destreg = 0; dstgather = 0; dstpos = 0; break;
			default: abort();
			}
	    if (destmode != absl && (dstpos < 0 || dstpos >= 32))
				abort ();
			switch (opcstr[pos]) {
			case 'p': destmode = Apdi; pos++; break;
			case 'P': destmode = Aipi; pos++; break;
			}
			break;
		case 'L':
			destmode = absl;
			break;
		case '#':
			switch (opcstr[pos++]) {
			case 'z': destmode = imm; break;
			case '0': destmode = imm0; break;
			case '1': destmode = imm1; break;
			case '2': destmode = imm2; break;
			case 'i': destmode = immi; destreg = (uae_s32)(uae_s8)bitval[biti]; break;
			case 'j': destmode = immi; destreg = bitval[bitj]; break;
			case 'J': destmode = immi; destreg = bitval[bitJ]; break;
			case 'k': destmode = immi; destreg = bitval[bitk]; break;
			case 'K': destmode = immi; destreg = bitval[bitK]; break;
			default: abort();
			}
			break;
		case 'd':
			destreg = bitval[bitD];
			destmode = mode_from_mr(bitval[bitd],bitval[bitD]);
			if (destmode == am_illg)
				continue;
			if (CPU_EMU_SIZE < 1 &&
				(destmode == Areg || destmode == Dreg || destmode == Aind
				|| destmode == Ad16 || destmode == Ad8r || destmode == Aipi
				|| destmode == Apdi))
			{
				dstgather = 1; dstpos = bitpos[bitD];
			}

			if (opcstr[pos] == '[') {
				pos++;
				if (opcstr[pos] == '!') {
					/* exclusion */
					do {
						pos++;
						if (mode_from_str(opcstr+pos) == destmode)
							goto nomatch;
						pos += 4;
					} while (opcstr[pos] == ',');
					pos++;
				} else {
					if (opcstr[pos+4] == '-') {
						/* replacement */
						if (mode_from_str(opcstr+pos) == destmode)
							destmode = mode_from_str(opcstr+pos+5);
						else
							goto nomatch;
						pos += 10;
					} else {
						/* normal */
						while(mode_from_str(opcstr+pos) != destmode) {
							pos += 4;
							if (opcstr[pos] == ']')
								goto nomatch;
							pos++;
						}
						while(opcstr[pos] != ']') pos++;
						pos++;
						break;
					}
				}
			}
			/* Some addressing modes are invalid as destination */
			if (destmode == imm || destmode == PC16 || destmode == PC8r)
				goto nomatch;
			break;
		case 's':
			destreg = bitval[bitS];
			destmode = mode_from_mr(bitval[bits],bitval[bitS]);

			if (destmode == am_illg)
				continue;
			if (CPU_EMU_SIZE < 1 &&
				(destmode == Areg || destmode == Dreg || destmode == Aind
				|| destmode == Ad16 || destmode == Ad8r || destmode == Aipi
				|| destmode == Apdi))
			{
				dstgather = 1; dstpos = bitpos[bitS];
			}

			if (opcstr[pos] == '[') {
				pos++;
				if (opcstr[pos] == '!') {
					/* exclusion */
					do {
						pos++;
						if (mode_from_str(opcstr+pos) == destmode)
							goto nomatch;
						pos += 4;
					} while (opcstr[pos] == ',');
					pos++;
				} else {
					if (opcstr[pos+4] == '-') {
						/* replacement */
						if (mode_from_str(opcstr+pos) == destmode)
							destmode = mode_from_str(opcstr+pos+5);
						else
							goto nomatch;
						pos += 10;
					} else {
						/* normal */
						while(mode_from_str(opcstr+pos) != destmode) {
							pos += 4;
							if (opcstr[pos] == ']')
								goto nomatch;
							pos++;
						}
						while(opcstr[pos] != ']') pos++;
						pos++;
					}
				}
			}
			break;
		default: abort();
		}
		/* safety check - might have changed */
		if (destmode != Areg && destmode != Dreg && destmode != Aind
			&& destmode != Ad16 && destmode != Ad8r && destmode != Aipi
			&& destmode != Apdi)
		{
			dstgather = 0;
		}

		if (destmode == Areg && sz == sz_byte)
			goto nomatch;
endofline:
		/* now, we have a match */
		if (table68k[opc].mnemo != i_ILLG)
			;//write_log (_T("Double match: %x: %s\n"), opc, opcstr);
		if (find == -1) {
			for (find = 0;; find++) {
				if (_tcscmp (mnemonic, lookuptab[find].name) == 0) {
					table68k[opc].mnemo = lookuptab[find].mnemo;
					break;
				}
				if (_tcslen (lookuptab[find].name) == 0)
					abort();
			}
		}
		else {
			table68k[opc].mnemo = lookuptab[find].mnemo;
		}
		table68k[opc].cc = bitval[bitc];
		mnemo = table68k[opc].mnemo;
		if (mnemo == i_BTST
			|| mnemo == i_BSET
			|| mnemo == i_BCLR
			|| mnemo == i_BCHG)
		{
			sz = destmode == Dreg ? sz_long : sz_byte;
			unsized = 0;
		}
		if (mnemo == i_JSR || mnemo == i_JMP) {
			unsized = 1;
		}

		table68k[opc].size = sz;
		table68k[opc].unsized = unsized;
		table68k[opc].sduse = id.sduse;
		table68k[opc].sreg = srcreg;
		table68k[opc].dreg = destreg;
		table68k[opc].smode = srcmode;
		table68k[opc].dmode = destmode;
		table68k[opc].spos = srcgather ? srcpos : -1;
		table68k[opc].dpos = dstgather ? dstpos : -1;
		table68k[opc].suse = usesrc;
		table68k[opc].duse = usedst;
		table68k[opc].stype = srctype;
		table68k[opc].plev = id.plevel;
		table68k[opc].clev = id.cpulevel;
		table68k[opc].unimpclev = id.unimpcpulevel;
		table68k[opc].head = id.head;
		table68k[opc].tail = id.tail;
		table68k[opc].clocks = id.clocks;
		table68k[opc].fetchmode = id.fetchmode;

	// Fix flags used information for Scc, Bcc, TRAPcc, DBcc instructions
	if	(	table68k[opc].mnemo == i_Scc
		||	table68k[opc].mnemo == i_Bcc
		||	table68k[opc].mnemo == i_DBcc
		||	table68k[opc].mnemo == i_TRAPcc
		)	{
		switch (table68k[opc].cc) {
		// CC mask:	XNZVC
		// 			 8421
		case 0: flags_used = 0x00; break;	/*  T */
		case 1: flags_used = 0x00; break;	/*  F */
		case 2: flags_used = 0x05; break;	/* HI */
		case 3: flags_used = 0x05; break;	/* LS */
		case 4: flags_used = 0x01; break;	/* CC */
		case 5: flags_used = 0x01; break;	/* CS */
		case 6: flags_used = 0x04; break;	/* NE */
		case 7: flags_used = 0x04; break;	/* EQ */
		case 8: flags_used = 0x02; break;	/* VC */
		case 9: flags_used = 0x02; break;	/* VS */
		case 10:flags_used = 0x08; break;	/* PL */
		case 11:flags_used = 0x08; break;	/* MI */
		case 12:flags_used = 0x0A; break;	/* GE */
		case 13:flags_used = 0x0A; break;	/* LT */
		case 14:flags_used = 0x0E; break;	/* GT */
		case 15:flags_used = 0x0E; break;	/* LE */
		}
	}

#if 1
	/* gb-- flagdead and flaglive would not have correct information */
	table68k[opc].flagdead = flags_set;
	table68k[opc].flaglive = flags_used;
#else
		table68k[opc].flagdead = flagdead;
		table68k[opc].flaglive = flaglive;
#endif
	  table68k[opc].cflow = cflow;
nomatch:
		/* FOO! */;
	}
}


void read_table68k (void)
{
	int i;

	free (table68k);
	table68k = xmalloc (struct instr, 65536);
	for (i = 0; i < 65536; i++) {
		table68k[i].mnemo = i_ILLG;
		table68k[i].handler = -1;
	}
	for (i = 0; i < n_defs68k; i++) {
		build_insn (i);
	}
}

static int imismatch;

static void handle_merges (long int opcode)
{
	uae_u16 smsk;
	uae_u16 dmsk;
	int sbitdst, dstend;
	int srcreg, dstreg;

	if (table68k[opcode].spos == -1) {
		sbitdst = 1; smsk = 0;
	} else {
		switch (table68k[opcode].stype) {
		case 0:
			smsk = 7; sbitdst = 8; break;
		case 1:
			smsk = 255; sbitdst = 256; break;
		case 2:
			smsk = 15; sbitdst = 16; break;
		case 3:
			smsk = 7; sbitdst = 8; break;
		case 4:
			smsk = 7; sbitdst = 8; break;
		case 5:
			smsk = 63; sbitdst = 64; break;
	  case 6:
	    smsk = 255; sbitdst = 256; break;
		case 7:
			smsk = 3; sbitdst = 4; break;
		default:
			smsk = 0; sbitdst = 0;
			abort();
			break;
		}
		smsk <<= table68k[opcode].spos;
	}
	if (table68k[opcode].dpos == -1) {
		dstend = 1; dmsk = 0;
	} else {
		dmsk = 7 << table68k[opcode].dpos;
		dstend = 8;
	}
	for (srcreg=0; srcreg < sbitdst; srcreg++) {
		for (dstreg=0; dstreg < dstend; dstreg++) {
			uae_u16 code = (uae_u16)opcode;

			code = (code & ~smsk) | (srcreg << table68k[opcode].spos);
			code = (code & ~dmsk) | (dstreg << table68k[opcode].dpos);

			/* Check whether this is in fact the same instruction.
			* The instructions should never differ, except for the
			* Bcc.(BW) case. */
			if (table68k[code].mnemo != table68k[opcode].mnemo
				|| table68k[code].size != table68k[opcode].size
				|| table68k[code].suse != table68k[opcode].suse
				|| table68k[code].duse != table68k[opcode].duse)
			{
				imismatch++; continue;
			}
			if (table68k[opcode].suse
				&& (table68k[opcode].spos != table68k[code].spos
				|| table68k[opcode].smode != table68k[code].smode
				|| table68k[opcode].stype != table68k[code].stype))
			{
				imismatch++; continue;
			}
			if (table68k[opcode].duse
				&& (table68k[opcode].dpos != table68k[code].dpos
				|| table68k[opcode].dmode != table68k[code].dmode))
			{
				imismatch++; continue;
			}

			if (code != opcode)
				table68k[code].handler = opcode;
		}
	}
}

void do_merges (void)
{
	long int opcode;
	int nr = 0;
	imismatch = 0;
	for (opcode = 0; opcode < 65536; opcode++) {
		if (table68k[opcode].handler != -1 || table68k[opcode].mnemo == i_ILLG)
			continue;
		nr++;
		handle_merges (opcode);
	}
	nr_cpuop_funcs = nr;
}

int get_no_mismatches (void)
{
	return imismatch;
}
