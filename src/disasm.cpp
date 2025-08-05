
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "debug.h"
#include "memory.h"
#include "newcpu.h"
#include "fpp.h"
#include "debugmem.h"
#include "disasm.h"

int disasm_flags = DISASM_FLAG_LC_MNEMO | DISASM_FLAG_LC_REG | DISASM_FLAG_LC_SIZE | DISASM_FLAG_LC_HEX |
	DISASM_FLAG_CC | DISASM_FLAG_EA | DISASM_FLAG_VAL | DISASM_FLAG_WORDS | DISASM_FLAG_ABSSHORTLONG;
int disasm_min_words = 5;
int disasm_max_words = 16;
TCHAR disasm_hexprefix[3] = { '$', 0 };

static TCHAR disasm_areg, disasm_dreg, disasm_byte, disasm_word, disasm_long;
static TCHAR disasm_pcreg[3], disasm_fpreg[3];
static bool absshort_long = false;

#ifdef DEBUGGER
void disasm_init(void)
{
	_tcscpy(disasm_pcreg, _T("PC"));
	_tcscpy(disasm_fpreg, _T("FP"));
	if (disasm_flags & DISASM_FLAG_LC_REG) {
		_tcscpy(disasm_pcreg, _T("pc"));
		_tcscpy(disasm_fpreg, _T("fp"));
	}
	disasm_areg = (disasm_flags & DISASM_FLAG_LC_REG) ? 'a' : 'A';
	disasm_dreg = (disasm_flags & DISASM_FLAG_LC_REG) ? 'd' : 'D';
	disasm_byte = (disasm_flags & DISASM_FLAG_LC_REG) ? 'b' : 'B';
	disasm_word = (disasm_flags & DISASM_FLAG_LC_REG) ? 'w' : 'W';
	disasm_long = (disasm_flags & DISASM_FLAG_LC_REG) ? 'l' : 'L';
	absshort_long = (disasm_flags & DISASM_FLAG_ABSSHORTLONG) != 0;

}

static void disasm_lc_mnemo(TCHAR *s)
{
	if (!(disasm_flags & DISASM_FLAG_LC_MNEMO) && !(disasm_flags & DISASM_FLAG_LC_SIZE)) {
		return;
	}
	if ((disasm_flags & DISASM_FLAG_LC_MNEMO) && (disasm_flags & DISASM_FLAG_LC_SIZE)) {
		to_lower(s, -1);
		return;
	}
	TCHAR *s2 = _tcschr(s, '.');
	if (s2) {
		if (disasm_flags & DISASM_FLAG_LC_SIZE) {
			to_lower(s2, -1);
		}
		if (disasm_flags & DISASM_FLAG_LC_MNEMO) {
			s2[0] = 0;
			to_lower(s, -1);
			s2[0] = '.';
		}
	} else {
		if (disasm_flags & DISASM_FLAG_LC_MNEMO) {
			to_lower(s, -1);
			return;
		}
	}
}

static const TCHAR *disasm_lc_size(const TCHAR *s)
{
	static TCHAR tmp[32];
	if (disasm_flags & DISASM_FLAG_LC_SIZE) {
		_tcscpy(tmp, s);
		to_lower(tmp, -1);
		return tmp;
	}
	return s;
}

static const TCHAR *disasm_lc_reg(const TCHAR *s)
{
	static TCHAR tmp[32];
	if (disasm_flags & DISASM_FLAG_LC_REG) {
		_tcscpy(tmp, s);
		to_lower(tmp, -1);
		return tmp;
	}
	return s;
}

static const TCHAR *disasm_lc_hex2(const TCHAR *s, bool noprefix)
{
	static TCHAR tmp[32];
	bool copied = false;
	if (disasm_flags & DISASM_FLAG_LC_HEX) {
		const TCHAR *s2 = _tcschr(s, 'X');
		if (s2) {
			_tcscpy(tmp, s);
			copied = true;
			tmp[s2 - s] = 'x';
			for (;;) {
				s2 = _tcschr(tmp, 'X');
				if (!s2) {
					break;
				}
				tmp[s2 - tmp] = 'x';
			}
		}
	}
	if (!noprefix) {
		if (disasm_hexprefix[0] != '$' || disasm_hexprefix[1] != 0 || s[0] != '$') {
			if (!copied) {
				_tcscpy(tmp, s);
				copied = true;
			}
			const TCHAR *s2 = _tcschr(tmp, '%');
			if (s2) {
				int len = uaetcslen(disasm_hexprefix);
				if (s2 > tmp && s2[-1] == '$') {
					len--;
					s2--;
				}
				if (len < 0) {
					memmove(tmp + (s2 - tmp), tmp + (s2 - tmp) - len, (uaetcslen(tmp + (s2 - tmp) - len) + 1) * sizeof(TCHAR));
				} else {
					if (len > 0) {
						memmove(tmp + (s2 - tmp) + len, s2, (uaetcslen(s2) + 1) * sizeof(TCHAR));
					}
					memcpy(tmp + (s2 - tmp), disasm_hexprefix, uaetcslen(disasm_hexprefix) * sizeof(TCHAR));
				}
			}
			return tmp;
		}
	}
	if (copied) {
		return tmp;
	}
	return s;
}

static const TCHAR *disasm_lc_hex(const TCHAR *s)
{
	return disasm_lc_hex2(s, false);
}
static const TCHAR *disasm_lc_nhex(const TCHAR *s)
{
	return disasm_lc_hex2(s, true);
}


struct cpum2c m2cregs[] = {
	{ 0, 31, _T("SFC") },
	{ 1, 31, _T("DFC") },
	{ 2, 30, _T("CACR") },
	{ 3, 24, _T("TC") },
	{ 4, 24, _T("ITT0") },
	{ 5, 24, _T("ITT1") },
	{ 6, 24, _T("DTT0") },
	{ 7, 24, _T("DTT1") },
	{ 8, 16, _T("BUSC") },
	{ 0x800, 31, _T("USP") },
	{ 0x801, 31, _T("VBR") },
	{ 0x802,  6, _T("CAAR") },
	{ 0x803, 15, _T("MSP") },
	{ 0x804, 31, _T("ISP") },
	{ 0x805,  8, _T("MMUS") },
	{ 0x806, 12, _T("URP") },
	{ 0x807, 12, _T("SRP") },
	{ 0x808, 16, _T("PCR") },
    { -1, 0, NULL }
};

const TCHAR *fpsizes[] = {
	_T("L"),
	_T("S"),
	_T("X"),
	_T("P"),
	_T("W"),
	_T("D"),
	_T("B"),
	_T("P")
};
static const wordsizes fpsizeconv[] = {
	sz_long,
	sz_single,
	sz_extended,
	sz_packed,
	sz_word,
	sz_double,
	sz_byte,
	sz_packed
};
static const int datasizes[] = {
	1,
	2,
	4,
	4,
	8,
	12,
	12
};

static void showea_val(TCHAR *buffer, uae_u16 opcode, uaecptr addr, int size)
{
	struct mnemolookup *lookup;
	struct instr *table = &table68k[opcode];

#ifndef CPU_TESTER
#ifdef UAE
	if (addr >= 0xe90000 && addr < 0xf00000)
		goto skip;
	if (addr >= 0xdff000 && addr < 0xe00000)
		goto skip;
#endif
#endif

	if (!(disasm_flags & (DISASM_FLAG_VAL_FORCE | DISASM_FLAG_VAL))) {
		goto skip;
	}

	for (lookup = lookuptab; lookup->mnemo != table->mnemo; lookup++)
		;
	if (!(lookup->flags & 1))
		goto skip;
	buffer += _tcslen(buffer);
	if (debug_safe_addr(addr, datasizes[size])) {
		bool cached = false;
		switch (size)
		{
			case sz_byte:
			{
				uae_u8 v = get_byte_cache_debug(addr, &cached);
				uae_u8 v2 = v;
				if (cached)
					v2 = get_byte_debug(addr);
				if (v != v2) {
					_sntprintf(buffer, sizeof buffer, _T(" [%02x:%02x]"), v, v2);
				} else {
					_sntprintf(buffer, sizeof buffer, _T(" [%s%02x]"), cached ? _T("*") : _T(""), v);
				}
			}
			break;
			case sz_word:
			{
				uae_u16 v = get_word_cache_debug(addr, &cached);
				uae_u16 v2 = v;
				if (cached)
					v2 = get_word_debug(addr);
				if (v != v2) {
					_sntprintf(buffer, sizeof buffer, _T(" [%04x:%04x]"), v, v2);
				} else {
					_sntprintf(buffer, sizeof buffer, _T(" [%s%04x]"), cached ? _T("*") : _T(""), v);
				}
			}
			break;
			case sz_long:
			{
				uae_u32 v = get_long_cache_debug(addr, &cached);
				uae_u32 v2 = v;
				if (cached)
					v2 = get_long_debug(addr);
				if (v != v2) {
					_sntprintf(buffer, sizeof buffer, _T(" [%08x:%08x]"), v, v2);
				} else {
					_sntprintf(buffer, sizeof buffer, _T(" [%s%08x]"), cached ? _T("*") : _T(""), v);
				}
			}
			break;
			case sz_single:
			{
				fpdata fp;
				fpp_to_single(&fp, get_long_debug(addr));
				_sntprintf(buffer, sizeof buffer, _T("[%s]"), fpp_print(&fp, 0));
			}
			break;
			case sz_double:
			{
				fpdata fp;
				fpp_to_double(&fp, get_long_debug(addr), get_long_debug(addr + 4));
				_sntprintf(buffer, sizeof buffer, _T("[%s]"), fpp_print(&fp, 0));
			}
			break;
			case sz_extended:
			{
				fpdata fp;
				fpp_to_exten(&fp, get_long_debug(addr), get_long_debug(addr + 4), get_long_debug(addr + 8));
				_sntprintf(buffer, sizeof buffer, _T("[%s]"), fpp_print(&fp, 0));
				break;
			}
			case sz_packed:
				_sntprintf(buffer, sizeof buffer, _T("[%08x%08x%08x]"), get_long_debug(addr), get_long_debug(addr + 4), get_long_debug(addr + 8));
				break;
		}
	}
skip:
	for (int i = 0; i < size; i++) {
		TCHAR name[256];
		if (debugmem_get_symbol(addr + i, name, sizeof(name) / sizeof(TCHAR))) {
			_sntprintf(buffer + _tcslen(buffer), sizeof buffer + _tcslen(buffer), _T(" %s"), name);
		}
	}
}

uaecptr ShowEA_disp(uaecptr *pcp, uaecptr base, TCHAR *buffer, const TCHAR *name, bool pcrel)
{
	uaecptr addr;
	uae_u16 dp;
	int r;
	uae_u32 dispreg;
	uaecptr pc = *pcp;
	TCHAR mult[20];
	TCHAR *p = NULL;

	dp = get_iword_debug(pc);
	pc += 2;

	r = (dp & 0x7000) >> 12; // REGISTER

	dispreg = dp & 0x8000 ? m68k_areg(regs, r) : m68k_dreg(regs, r);
	if (!(dp & 0x800)) { // W/L
		dispreg = (uae_s32)(uae_s16)(dispreg);
	}

	int m = 1 << ((dp >> 9) & 3);
	mult[0] = 0;

	if (currprefs.cpu_model >= 68020) {
		dispreg <<= (dp >> 9) & 3; // SCALE
	}

	if (buffer)
		buffer[0] = 0;
	if ((dp & 0x100) && currprefs.cpu_model >= 68020) {
		TCHAR dr[20];
		uae_s32 outer = 0, disp = 0;

		// Full format extension (68020+)

		if (m > 1 && buffer) {
			_sntprintf(mult, sizeof mult, _T("*%d"), m);
		}

		if (dp & 0x80) { // BS (base register suppress)
			base = 0;
			if (buffer)
				name = NULL;
		}
		if (buffer)
			_sntprintf(dr, sizeof dr, _T("%c%d.%c"), dp & 0x8000 ? disasm_areg : disasm_dreg, (int)r, dp & 0x800 ? disasm_long : disasm_word);
		if (dp & 0x40) { // IS (index suppress)
			dispreg = 0;
			dr[0] = 0;
		}

		if (buffer) {
			_tcscpy(buffer, _T("("));
			p = buffer + _tcslen(buffer);

			if (dp & 3) {
				// indirect
				_sntprintf(p, sizeof p, _T("["));
				p += _tcslen(p);
			} else {
				// (an,dn,word/long)
				if (name) {
					_sntprintf(p, sizeof p, _T("%s,"), name);
					p += _tcslen(p);
				}
				if (dr[0]) {
					_sntprintf(p, sizeof p, _T("%s%s,"), dr, mult);
					p += _tcslen(p);
				}
			}
		}

		if ((dp & 0x30) == 0x20) { // BD SIZE = 2 (WORD)
			disp = (uae_s32)(uae_s16)get_iword_debug(pc);
			if (buffer) {
				_sntprintf(p, sizeof p, disasm_lc_hex(_T("$%04X,")), (uae_s16)disp);
				p += _tcslen(p);
			}
			pc += 2;
			base += disp;
		} else if ((dp & 0x30) == 0x30) { // BD SIZE = 3 (LONG)
			disp = get_ilong_debug(pc);
			if (buffer) {
				_sntprintf(p, sizeof p, disasm_lc_hex(_T("$%08X,")), disp);
				p += _tcslen(p);
			}
			pc += 4;
			base += disp;
		}

		if ((dp & 3) && buffer) {
			if (name) {
				_sntprintf(p, sizeof p, _T("%s,"), name);
				p += _tcslen(p);
			}

			if (!(dp & 0x04)) {
				if (dr[0]) {
					_sntprintf(p, sizeof p, _T("%s%s,"), dr, mult);
					p += _tcslen(p);
				}
			}

			if (p[-1] == ',')
				p--;
			_sntprintf(p, sizeof p, _T("],"));
			p += _tcslen(p);

			if ((dp & 0x04)) {
				if (dr[0]) {
					_sntprintf(p, sizeof p, _T("%s%s,"), dr, mult);
					p += _tcslen(p);
				}
			}

		}

		if ((dp & 0x03) == 0x02) {
			outer = (uae_s32)(uae_s16)get_iword_debug(pc);
			if (buffer) {
				_sntprintf(p, sizeof p, disasm_lc_hex(_T("$%04X,")), (uae_s16)outer);
				p += _tcslen(p);
			}
			pc += 2;
		} else 	if ((dp & 0x03) == 0x03) {
			outer = get_ilong_debug(pc);
			if (buffer) {
				_sntprintf(p, sizeof p, disasm_lc_hex(_T("$%08X,")), outer);
				p += _tcslen(p);
			}
			pc += 4;
		}

		if (buffer) {
			if (p[-1] == ',')
				p--;
			_sntprintf(p, sizeof p, _T(")"));
			p += _tcslen(p);
		}

		if ((dp & 0x4) == 0)
			base += dispreg;
		if (dp & 0x3)
			base = get_long_debug(base);
		if (dp & 0x4)
			base += dispreg;

		addr = base + outer;

		if (buffer) {
			if (disasm_flags & (DISASM_FLAG_VAL_FORCE | DISASM_FLAG_VAL)) {
				_sntprintf(p, sizeof p, disasm_lc_hex(_T(" == $%08X")), addr);
				p += _tcslen(p);
			}
		}

	} else {
		// Brief format extension
		TCHAR regstr[20];
		uae_s8 disp8 = dp & 0xFF;

		if (m > 1 && buffer) {
			if (currprefs.cpu_model < 68020) {
				_sntprintf(mult, sizeof mult, _T("[*%d]"), m);
			} else {
				_sntprintf(mult, sizeof mult, _T("*%d"), m);
			}
		}
		regstr[0] = 0;
		_sntprintf(regstr, sizeof regstr, _T(",%c%d.%c"), dp & 0x8000 ? disasm_areg : disasm_dreg, (int)r, dp & 0x800 ? disasm_long : disasm_word);
		addr = base + (uae_s32)((uae_s8)disp8) + dispreg;
		if (buffer) {
			TCHAR offtxt[16];
			if (disp8 < 0)
				_sntprintf(offtxt, sizeof offtxt, disasm_lc_hex(_T("-$%02X")), -disp8);
			else
				_sntprintf(offtxt, sizeof offtxt, disasm_lc_hex(_T("$%02X")), disp8);
			if (pcrel) {
				if (disasm_flags & (DISASM_FLAG_VAL_FORCE | DISASM_FLAG_VAL)) {
					_sntprintf(buffer, sizeof buffer, _T("(%s,%s%s%s=%08x) == $%08x"), offtxt, name, regstr, mult, (*pcp) += disp8, addr);
				} else {
					_sntprintf(buffer, sizeof buffer, _T("(%s,%s%s%s=$%08x)"), offtxt, name, regstr, mult, (*pcp) += disp8);
				}
			} else {
				if (disasm_flags & (DISASM_FLAG_VAL_FORCE | DISASM_FLAG_VAL)) {
					_sntprintf(buffer, sizeof buffer, _T("(%s,%s%s%s) == $%08x"), offtxt, name, regstr, mult, addr);
				} else {
					_sntprintf(buffer, sizeof buffer, _T("(%s,%s%s%s)"), offtxt, name, regstr, mult);
				}
			}
			if (((dp & 0x0100) || m != 1) && currprefs.cpu_model < 68020) {
				_tcscat(buffer, _T(" (68020+)"));
			}
		}
	}

	*pcp = pc;
	return addr;
}

uaecptr ShowEA(void *f, uaecptr pc, uae_u16 opcode, int reg, amodes mode, wordsizes size, TCHAR *buf, uae_u32 *eaddr, int *actualea, int safemode)
{
	uaecptr addr = pc;
	uae_s16 disp16;
	uae_s32 offset = 0;
	TCHAR buffer[80];

	if (actualea)
		*actualea = 1;
	if ((opcode & 0xf0ff) == 0x60ff && currprefs.cpu_model < 68020) {
		// bcc.l is bcc.s if 68000/68010
		mode = immi;
	}

	buffer[0] = 0;
	switch (mode){
	case Dreg:
		_sntprintf(buffer, sizeof buffer, _T("%c%d"), disasm_dreg, reg);
		if (actualea)
			*actualea = 0;
		break;
	case Areg:
		_sntprintf(buffer, sizeof buffer, _T("%c%d"), disasm_areg, reg);
		if (actualea)
			*actualea = 0;
		break;
	case Aind:
		_sntprintf(buffer, sizeof buffer, _T("(%c%d)"), disasm_areg, reg);
		addr = regs.regs[reg + 8];
		if (disasm_flags & DISASM_FLAG_VAL_FORCE) {
			_sntprintf(buffer + _tcslen(buffer), sizeof buffer, disasm_lc_hex(_T(" == $%08X")), addr);
		}
		showea_val(buffer, opcode, addr, size);
		break;
	case Aipi:
		_sntprintf(buffer, sizeof buffer, _T("(%c%d)+"), disasm_areg, reg);
		addr = regs.regs[reg + 8];
		if (disasm_flags & DISASM_FLAG_VAL_FORCE) {
			_sntprintf(buffer + _tcslen(buffer), sizeof buffer, disasm_lc_hex(_T(" == $%08X")), addr);
		}
		showea_val(buffer, opcode, addr, size);
		break;
	case Apdi:
		_sntprintf(buffer, sizeof buffer, _T("-(%c%d)"), disasm_areg, reg);
		addr = regs.regs[reg + 8] - datasizes[size];
		if (disasm_flags & DISASM_FLAG_VAL_FORCE) {
			_sntprintf(buffer + _tcslen(buffer), sizeof buffer, disasm_lc_hex(_T(" == $%08X")), addr);
		}
		showea_val(buffer, opcode, addr, size);
		break;
	case Ad16:
		{
			TCHAR offtxt[32];
			disp16 = get_iword_debug (pc); pc += 2;
			if (disp16 < 0)
				_sntprintf (offtxt, sizeof offtxt, disasm_lc_hex(_T("-$%04X")), -disp16);
			else
				_sntprintf (offtxt, sizeof offtxt, disasm_lc_hex(_T("$%04X")), disp16);
			addr = m68k_areg (regs, reg) + disp16;
			_sntprintf(buffer, sizeof buffer, _T("(%s,%c%d)"), offtxt, disasm_areg, reg);
			if (disasm_flags & (DISASM_FLAG_VAL_FORCE | DISASM_FLAG_VAL)) {
				_sntprintf(buffer + _tcslen(buffer), sizeof buffer, disasm_lc_hex(_T(" == $%08X")), addr);
			}
			showea_val(buffer, opcode, addr, size);
		}
		break;
	case Ad8r:
		{
			TCHAR name[10];
			_sntprintf(name, sizeof name, _T("%c%d"), disasm_areg, reg);
			addr = ShowEA_disp(&pc, m68k_areg(regs, reg), buffer, name, false);
			showea_val(buffer, opcode, addr, size);
		}
		break;
	case PC16:
		{
			TCHAR offtxt[32];
			disp16 = get_iword_debug (pc); pc += 2;
			if (disp16 < 0)
				_sntprintf(offtxt, sizeof offtxt, disasm_lc_hex(_T("-$%04X")), -disp16);
			else
				_sntprintf(offtxt, sizeof offtxt, disasm_lc_hex(_T("$%04X")), disp16);
			addr += disp16;
			_stprintf(buffer, _T("(%s,%s)"), offtxt, disasm_pcreg);
			if (disasm_flags & (DISASM_FLAG_VAL_FORCE | DISASM_FLAG_VAL)) {
				_stprintf(buffer + _tcslen(buffer), disasm_lc_hex(_T(" == $%08X")), addr);
			}
			showea_val(buffer, opcode, addr, size);
		}
		break;
	case PC8r:
		{
			addr = ShowEA_disp(&pc, addr, buffer, disasm_pcreg, true);
			showea_val(buffer, opcode, addr, size);
		}
		break;
	case absw:
		{
			addr = (uae_s32)(uae_s16)get_iword_debug (pc);
			uae_s16 saddr = (uae_s16)addr;
			if (absshort_long) {
				_sntprintf(buffer, sizeof buffer, disasm_lc_hex(_T("$%08X.%c")), addr, disasm_word);
			} else if (saddr < 0) {
				_sntprintf(buffer, sizeof buffer, disasm_lc_hex(_T("-$%04X.%c")), -saddr, disasm_word);
			} else {
				_sntprintf(buffer, sizeof buffer, disasm_lc_hex(_T("$%04X.%c")), saddr, disasm_word);
			}
			pc += 2;
			showea_val(buffer, opcode, addr, size);
		}
		break;
	case absl:
		addr = get_ilong_debug (pc);
		_sntprintf (buffer, sizeof buffer, disasm_lc_hex(_T("$%08X")), addr);
		pc += 4;
		showea_val(buffer, opcode, addr, size);
		break;
	case imm:
		if (actualea)
			*actualea = 0;
		switch (size){
		case sz_byte:
			_sntprintf (buffer, sizeof buffer, disasm_lc_hex(_T("#$%02X")), (get_iword_debug (pc) & 0xff));
			pc += 2;
			break;
		case sz_word:
			_sntprintf (buffer, sizeof buffer, disasm_lc_hex(_T("#$%04X")), (get_iword_debug (pc) & 0xffff));
			pc += 2;
			break;
		case sz_long:
			_sntprintf(buffer, sizeof buffer, disasm_lc_hex(_T("#$%08X")), (get_ilong_debug(pc)));
			pc += 4;
			break;
		case sz_single:
			{
				fpdata fp;
				fpp_to_single(&fp, get_ilong_debug(pc));
				_sntprintf(buffer, sizeof buffer, _T("#%s"), fpp_print(&fp, 0));
				pc += 4;
			}
			break;
		case sz_double:
			{
				fpdata fp;
				fpp_to_double(&fp, get_ilong_debug(pc), get_ilong_debug(pc + 4));
				_sntprintf(buffer, sizeof buffer, _T("#%s"), fpp_print(&fp, 0));
				pc += 8;
			}
			break;
		case sz_extended:
		{
			fpdata fp;
			fpp_to_exten(&fp, get_ilong_debug(pc), get_ilong_debug(pc + 4), get_ilong_debug(pc + 8));
			_sntprintf(buffer, sizeof buffer, _T("#%s"), fpp_print(&fp, 0));
			pc += 12;
			break;
		}
		case sz_packed:
			_sntprintf(buffer, sizeof buffer, disasm_lc_hex(_T("#$%08X%08X%08X")), get_ilong_debug(pc), get_ilong_debug(pc + 4), get_ilong_debug(pc + 8));
			pc += 12;
			break;
		default:
			break;
		}
		break;
	case imm0:
		offset = (uae_s32)(uae_s8)get_iword_debug (pc);
		_sntprintf (buffer, sizeof buffer, disasm_lc_hex(_T("#$%02X")), (uae_u32)(offset & 0xff));
		addr = pc + 2 + offset;
		if ((opcode & 0xf000) == 0x6000) {
			showea_val(buffer, opcode, addr, 1);
		}
		pc += 2;
		if (actualea)
			*actualea = 0;
		break;
	case imm1:
		offset = (uae_s32)(uae_s16)get_iword_debug (pc);
		buffer[0] = 0;
		_sntprintf (buffer, sizeof buffer, disasm_lc_hex(_T("#$%04X")), (uae_u32)(offset & 0xffff));
		addr = pc + offset;
		if ((opcode & 0xf000) == 0x6000) {
			showea_val(buffer, opcode, addr, 2);
		}
		pc += 2;
		if (actualea)
			*actualea = 0;
		break;
	case imm2:
		offset = (uae_s32)get_ilong_debug (pc);
		_sntprintf (buffer, sizeof buffer, disasm_lc_hex(_T("#$%08X")), (uae_u32)offset);
		addr = pc + offset;
		if ((opcode & 0xf000) == 0x6000) {
			showea_val(buffer, opcode, addr, 4);
		}
		pc += 4;
		if (actualea)
			*actualea = 0;
		break;
	case immi:
		offset = (uae_s32)(uae_s8)(reg & 0xff);
		_sntprintf (buffer, sizeof buffer, disasm_lc_hex(_T("#$%02X")), (uae_u8)offset);
		addr = pc + offset;
		if (actualea)
			*actualea = 0;
		break;
	default:
		break;
	}
	if (buf == NULL)
		f_out (stdout, _T("%s"), buffer);
	else
		_tcscat (buf, buffer);
	if (eaddr)
		*eaddr = addr;
	return pc;
}

static const TCHAR *ccnames[] =
{
	_T("T"), _T("F"), _T("HI"),_T("LS"),_T("CC"),_T("CS"),_T("NE"),_T("EQ"),
	_T("VC"),_T("VS"),_T("PL"),_T("MI"),_T("GE"),_T("LT"),_T("GT"),_T("LE")
};
static const TCHAR *fpccnames[] =
{
	_T("F"),
	_T("EQ"),
	_T("OGT"),
	_T("OGE"),
	_T("OLT"),
	_T("OLE"),
	_T("OGL"),
	_T("OR"),
	_T("UN"),
	_T("UEQ"),
	_T("UGT"),
	_T("UGE"),
	_T("ULT"),
	_T("ULE"),
	_T("NE"),
	_T("T"),
	_T("SF"),
	_T("SEQ"),
	_T("GT"),
	_T("GE"),
	_T("LT"),
	_T("LE"),
	_T("GL"),
	_T("GLE"),
	_T("NGLE"),
	_T("NGL"),
	_T("NLE"),
	_T("NLT"),
	_T("NGE"),
	_T("NGT"),
	_T("SNE"),
	_T("ST")
};
const TCHAR *fpuopcodes[] =
{
	_T("FMOVE"),
	_T("FINT"),
	_T("FSINH"),
	_T("FINTRZ"),
	_T("FSQRT"),
	NULL,
	_T("FLOGNP1"),
	NULL,
	_T("FETOXM1"),
	_T("FTANH"),
	_T("FATAN"),
	NULL,
	_T("FASIN"),
	_T("FATANH"),
	_T("FSIN"),
	_T("FTAN"),
	_T("FETOX"),	// 0x10
	_T("FTWOTOX"),
	_T("FTENTOX"),
	NULL,
	_T("FLOGN"),
	_T("FLOG10"),
	_T("FLOG2"),
	NULL,
	_T("FABS"),
	_T("FCOSH"),
	_T("FNEG"),
	NULL,
	_T("FACOS"),
	_T("FCOS"),
	_T("FGETEXP"),
	_T("FGETMAN"),
	_T("FDIV"),		// 0x20
	_T("FMOD"),
	_T("FADD"),
	_T("FMUL"),
	_T("FSGLDIV"),
	_T("FREM"),
	_T("FSCALE"),
	_T("FSGLMUL"),
	_T("FSUB"),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	_T("FSINCOS"),	// 0x30
	_T("FSINCOS"),
	_T("FSINCOS"),
	_T("FSINCOS"),
	_T("FSINCOS"),
	_T("FSINCOS"),
	_T("FSINCOS"),
	_T("FSINCOS"),
	_T("FCMP"),
	NULL,
	_T("FTST"),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	_T("FSMOVE"),	// 0x40
	_T("FSSQRT"),
	NULL,
	NULL,
	_T("FDMOVE"),
	_T("FDSQRT"),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	// 0x50
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	_T("FSABS"),
	NULL,
	_T("FSNEG"),
	NULL,
	_T("FDABS"),
	NULL,
	_T("FDNEG"),
	NULL,
	_T("FSDIV"), // 0x60
	NULL,
	_T("FSADD"),
	_T("FSMUL"),
	_T("FDDIV"),
	NULL,
	_T("FDADD"),
	_T("FDMUL"),
	_T("FSSUB"),
	NULL,
	NULL,
	NULL,
	_T("FDSUB"),
	NULL,
	NULL,
	NULL,
	NULL,  // 0x70
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	_T("ILLG") // 0x80
};

static const TCHAR *movemregs[] =
{
	_T("D0"),
	_T("D1"),
	_T("D2"),
	_T("D3"),
	_T("D4"),
	_T("D5"),
	_T("D6"),
	_T("D7"),
	_T("A0"),
	_T("A1"),
	_T("A2"),
	_T("A3"),
	_T("A4"),
	_T("A5"),
	_T("A6"),
	_T("A7"),
	_T("FP0"),
	_T("FP1"),
	_T("FP2"),
	_T("FP3"),
	_T("FP4"),
	_T("FP5"),
	_T("FP6"),
	_T("FP7"),
	_T("FPCR"),
	_T("FPSR"),
	_T("FPIAR")
};

static void addmovemreg(TCHAR *out, int *prevreg, int *lastreg, int *first, int reg, int fpmode)
{
	TCHAR s[10];
	TCHAR *p = out + _tcslen (out);
	if (*prevreg < 0) {
		*prevreg = reg;
		*lastreg = reg;
		return;
	}
	if (reg < 0 || fpmode == 2 || (*prevreg) + 1 != reg || (reg & 8) != ((*prevreg & 8))) {
		_tcscpy(s, movemregs[*lastreg]);
		if (disasm_flags & DISASM_FLAG_LC_REG) {
			to_lower(s, -1);
		}
		_sntprintf (p, sizeof p, _T("%s%s"), (*first) ? _T("") : _T("/"), s);
		p = p + _tcslen (p);
		if (*lastreg != *prevreg) {
			_tcscpy(s, movemregs[*prevreg]);
			if (disasm_flags & DISASM_FLAG_LC_REG) {
				to_lower(s, -1);
			}
			if ((*lastreg) + 2 == reg) {
				_sntprintf(p, sizeof p, _T("/%s"), s);
			} else if ((*lastreg) != (*prevreg)) {
				_sntprintf(p, sizeof p, _T("-%s"), s);
			}
		}
		*lastreg = reg;
		*first = 0;
	}
	*prevreg = reg;
}

static bool movemout(TCHAR *out, uae_u16 mask, int mode, int fpmode, bool dst)
{
	unsigned int dmask, amask;
	int prevreg = -1, lastreg = -1, first = 1;

	if (mode == Apdi && !fpmode) {
		uae_u8 dmask2;
		uae_u8 amask2;
		
		amask2 = mask & 0xff;
		dmask2 = (mask >> 8) & 0xff;
		dmask = 0;
		amask = 0;
		for (int i = 0; i < 8; i++) {
			if (dmask2 & (1 << i))
				dmask |= 1 << (7 - i);
			if (amask2 & (1 << i))
				amask |= 1 << (7 - i);
		}
	} else {
		dmask = mask & 0xff;
		amask = (mask >> 8) & 0xff;
		if (fpmode == 1 && mode != Apdi) {
			uae_u8 dmask2 = dmask;
			dmask = 0;
			for (int i = 0; i < 8; i++) {
				if (dmask2 & (1 << i))
					dmask |= 1 << (7 - i);
			}
		}
	}
	bool dataout = dmask != 0 || amask != 0;
	if (dst && dataout)
		_tcscat(out, _T(","));
	if (fpmode) {
		while (dmask) { addmovemreg(out, &prevreg, &lastreg, &first, movem_index1[dmask] + (fpmode == 2 ? 24 : 16), fpmode); dmask = movem_next[dmask]; }
	} else {
		while (dmask) { addmovemreg (out, &prevreg, &lastreg, &first, movem_index1[dmask], fpmode); dmask = movem_next[dmask]; }
		while (amask) { addmovemreg (out, &prevreg, &lastreg, &first, movem_index1[amask] + 8, fpmode); amask = movem_next[amask]; }
	}
	addmovemreg(out, &prevreg, &lastreg, &first, -1, fpmode);
	return dataout;
}

static void disasm_size(TCHAR *instrname, struct instr *dp)
{
	int size = dp->size;
	if (dp->unsized) {
		_tcscat(instrname, _T(" "));
		return;
	}
	int m = dp->mnemo;
	if (dp->suse && dp->smode == immi &&
		(m == i_MOVE || m == i_ADD || m == i_ADDA || m == i_SUB || m == i_SUBA)) {
		_tcscat(instrname, disasm_lc_size(_T("Q")));
		if (m == i_MOVE)
			size = -1;
	}
	// EXT.B -> EXTB.L
	if (m == i_EXT && dp->size == sz_byte) {
		_tcscat(instrname, disasm_lc_size(_T("B")));
		size = sz_long;
	}

	switch (size)
	{
	case sz_byte:
		_tcscat(instrname, disasm_lc_size(_T(".B ")));
		break;
	case sz_word:
		_tcscat(instrname, disasm_lc_size(_T(".W ")));
		break;
	case sz_long:
		_tcscat(instrname, disasm_lc_size(_T(".L ")));
		break;
	default:
		_tcscat(instrname, disasm_lc_size(_T(" ")));
		break;
	}
}

static void asm_add_extensions(uae_u16 *data, int *dcntp, int mode, uae_u32 v, int extcnt, uae_u16 *ext, uaecptr pc, int size)
{
	int dcnt = *dcntp;
	if (mode < 0)
		return;
	if (mode == Ad16) {
		data[dcnt++] = v;
	}
	if (mode == PC16) {
		data[dcnt++] = v - (pc + 2);
	}
	if (mode == Ad8r || mode == PC8r) {
		for (int i = 0; i < extcnt; i++) {
			data[dcnt++] = ext[i];
		}
	}
	if (mode == absw) {
		data[dcnt++] = (uae_u16)v;
	}
	if (mode == absl) {
		data[dcnt++] = (uae_u16)(v >> 16);
		data[dcnt++] = (uae_u16)v;
	}
	if ((mode == imm && size == 0) || mode == imm0) {
		data[dcnt++] = (uae_u8)v;
	}
	if ((mode == imm && size == 1) || mode == imm1) {
		data[dcnt++] = (uae_u16)v;
	}
	if ((mode == imm && size == 2) || mode == imm2) {
		data[dcnt++] = (uae_u16)(v >> 16);
		data[dcnt++] = (uae_u16)v;
	}
	*dcntp = dcnt;
}

static int asm_isdreg(const TCHAR *s)
{
	if (s[0] == 'D' && s[1] >= '0' && s[1] <= '7')
		return s[1] - '0';
	return -1;
}
static int asm_isareg(const TCHAR *s)
{
	if (s[0] == 'A' && s[1] >= '0' && s[1] <= '7')
		return s[1] - '0';
	if (s[0] == 'S' && s[1] == 'P')
		return 7;
	return -1;
}
static int asm_ispc(const TCHAR *s)
{
	if (s[0] == 'P' && s[1] == 'C')
		return 1;
	return 0;
}

static uae_u32 asmgetval(const TCHAR *s)
{
	TCHAR *endptr;
	if (s[0] == '$') {
		s++;
	}
	if (s[0] == '-') {
		return _tcstol(s, &endptr, 16);
	}
	return _tcstoul(s, &endptr, 16);
}

static int asm_parse_mode020(TCHAR *s, uae_u8 *reg, uae_u32 *v, int *extcnt, uae_u16 *ext)
{
	return -1;
}

static int asm_parse_mode(TCHAR *s, uae_u8 *reg, uae_u32 *v, int *extcnt, uae_u16 *ext)
{
	TCHAR *ss = s;
	*reg = -1;
	*v = 0;
	*ext = 0;
	*extcnt = 0;
	if (s[0] == 0)
		return -1;
	// Dn
	if (asm_isdreg(s) >= 0 && s[2] == 0) {
		*reg = asm_isdreg(s);
		return Dreg;
	}
	// An
	if (asm_isareg(s) >= 0 && s[2] == 0) {
		*reg = asm_isareg(s);
		return Areg;
	}
	// (An) and (An)+
	if (s[0] == '(' && asm_isareg(s + 1) >= 0 && s[3] == ')') {
		*reg = asm_isareg(s + 1);
		if (s[4] == '+' && s[5] == 0)
			return Aipi;
		if (s[4] == 0)
			return Aind;
		return -1;
	}
	// -(An)
	if (s[0] == '-' && s[1] == '(' && asm_isareg(s + 2) >= 0 && s[4] == ')' && s[5] == 0) {
		*reg = asm_isareg(s + 2);
		return Apdi;
	}
	// Immediate
	if (s[0] == '#') {
		if (s[1] == '!') {
			*v = _tstol(s + 2);
		} else {
			*v = asmgetval(s + 1);
		}
		return imm;
	}
	// Value
	if (s[0] == '!') {
		*v = _tstol(s + 1);
	} else {
		*v = asmgetval(s);
	}
	int dots = 0;
	int fullext = 0;
	for (int i = 0; i < _tcslen(s); i++) {
		if (s[i] == ',') {
			dots++;
		} else if (s[i] == '[') {
			if (fullext > 0)
				fullext = -1;
			else
				fullext = 1;
		} else if (s[i] == ']') {
			if (fullext != 1)
				fullext = -1;
			else
				fullext = 2;
		}
	}
	if (fullext < 0 || fullext == 1)
		return -1;
	if (fullext == 2) {
		return asm_parse_mode020(s, reg, v, extcnt, ext);
	}
	while (*s != 0) {
		// d16(An)
		if (dots == 0 && s[0] == '(' && asm_isareg(s + 1) >= 0 && s[3] == ')' && s[4] == 0) {
			*reg = asm_isareg(s + 1);
			return Ad16;
		}
		// d16(PC)
		if (dots == 0 && s[0] == '(' && asm_ispc(s + 1) && s[3] == ')' && s[4] == 0) {
			*reg = 2;
			return PC16;
		}
		// (d16,An) / (d16,PC)
		if (dots == 1 && s[0] == '(' && !asm_ispc(s + 1) && asm_isareg(s + 1) < 0 && asm_isdreg(s + 1) < 0) {
			TCHAR *startptr, *endptr;
			if (s[1] == '!') {
				startptr = s + 2;
				*v = _tcstol(startptr, &endptr, 10);
			} else {
				if (s[1] == '$')
					startptr = s + 2;
				else
					startptr = s + 1;
				*v = _tcstol(startptr, &endptr, 16);
			}
			if (endptr == startptr || endptr[0] != ',')
				return -1;
			if (asm_ispc(endptr + 1) && endptr[3] == ')') {
				*reg = 2;
				return PC16;
			}
			if (asm_isareg(endptr + 1) >= 0 && endptr[3] == ')') {
				*reg = asm_isareg(endptr + 1);
				return Ad16;
			}
			return -1;
		}
		// Ad8r PC8r
		if (s[0] == '(') {
			TCHAR *s2 = s;
			if (!asm_ispc(s + 1) && asm_isareg(s + 1) < 0 && asm_isdreg(s + 1) < 0) {
				if (dots != 2)
					return -1;
				TCHAR *startptr, *endptr;
				if (s[1] == '!') {
					startptr = s + 2;
					*v = _tcstol(startptr, &endptr, 10);
				} else {
					if (s[1] == '$')
						startptr = s + 2;
					else
						startptr = s + 1;
					*v = _tcstol(startptr, &endptr, 16);
				}
				if (endptr == startptr || endptr[0] != ',')
					return -1;
				s2 = endptr + 1;
			} else if (((asm_isareg(s + 1) >= 0 || asm_ispc(s + 1)) && s[3] == ',') || (asm_isdreg(s + 4) >= 0 || asm_isareg(s + 4) >= 0)) {
				if (dots != 1)
					return -1;
				s2 = s + 1;
			} else {
				return -1;
			}
			uae_u8 reg2;
			bool ispc = asm_ispc(s2);
			if (ispc) {
				*reg = 3;
			} else {
				*reg = asm_isareg(s2);
			}
			*extcnt = 1;
			s2 += 2;
			if (*s2 != ',')
				return -1;
			s2++;
			if (asm_isdreg(s2) >= 0) {
				reg2 = asm_isdreg(s2);
			} else {
				reg2 = asm_isareg(s2);
				*ext |= 1 << 15;
			}
			s2 += 2;
			*ext |= reg2 << 12;
			*ext |= (*v) & 0xff;
			if (s2[0] == '.' && s2[1] == 'W') {
				s2 += 2;
			} else if (s2[0] == '.' && s2[1] == 'L') {
				*ext |= 1 << 11;
				s2 += 2;
			}
			if (s2[0] == '*') {
				TCHAR scale = s2[1];
				if (scale == '2')
					*ext |= 1 << 9;
				else if (scale == '4')
					*ext |= 2 << 9;
				else if (scale == '8')
					*ext |= 3 << 9;
				else
					return -1;
				s2 += 2;
			}
			if (s2[0] == ')' && s2[1] == 0) {
				return ispc ? PC8r : Ad8r;
			}
			return -1;
		}
		s++;
	}
	// abs.w
	if (s - ss > 2 && s[-2] == '.' && s[-1] == 'W') {
		*reg = 0;
		return absw;
	}
	// abs.l
	*reg = 1;
	return absl;
}

static TCHAR *asm_parse_parm(TCHAR *parm, TCHAR *out)
{
	TCHAR *p = parm;
	bool quote = false;

	for (;;) {
		if (*p == '(') {
			quote = true;
		}
		if (*p == ')') {
			if (!quote)
				return NULL;
			quote = false;
		}
		if ((*p == ',' || *p == 0) && !quote) {
			TCHAR c = *p;
			p[0] = 0;
			_tcscpy(out, parm);
			my_trim(out);
			if (c)
				p++;
			return p;
		}
		p++;
	}
}

static bool m68k_asm_parse_movec(TCHAR *s, TCHAR *d)
{
	for (int i = 0; m2cregs[i].regname; i++) {
		if (!_tcscmp(s, m2cregs[i].regname)) {
			uae_u16 v = m2cregs[i].regno;
			if (asm_isareg(d) >= 0)
				v |= 0x8000 | (asm_isareg(d) << 12);
			else if (asm_isdreg(d) >= 0)
				v |= (asm_isdreg(d) << 12);
			else
				return false;
			_sntprintf(s, sizeof s, _T("#%X"), v);
			return true;
		}
	}
	return false;
}

static bool m68k_asm_parse_movem(TCHAR *s, int dir)
{
	TCHAR *d = s;
	uae_u16 regmask = 0;
	uae_u16 mask = dir ? 0x8000 : 0x0001;
	bool ret = false;
	while(*s) {
		int dreg = asm_isdreg(s);
		int areg = asm_isareg(s);
		if (dreg < 0 && areg < 0)
			break;
		int reg = dreg >= 0 ? dreg : areg + 8;
		regmask |= dir ? (mask >> reg) : (mask << reg);
		s += 2;
		if (*s == 0) {
			ret = true;
			break;
		} else if (*s == '/') {
			s++;
			continue;
		} else if (*s == '-') {
			s++;
			int dreg2 = asm_isdreg(s);
			int areg2 = asm_isareg(s);
			if (dreg2 < 0 && areg2 < 0)
				break;
			int reg2 = dreg2 >= 0 ? dreg2 : areg2 + 8;
			if (reg2 < reg)
				break;
			while (reg2 >= reg) {
				regmask |= dir ? (mask >> reg) : (mask << reg);
				reg++;
			}
			s += 2;
			if (*s == 0) {
				ret = true;
				break;
			}
		} else {
			break;
		}
	}
	if (ret)
		_sntprintf(d, sizeof d, _T("#%X"), regmask);
	return ret;
}

int m68k_asm(TCHAR *sline, uae_u16 *out, uaecptr pc)
{
	TCHAR *p;
	const TCHAR *cp1;
	TCHAR ins[256], parms[256];
	TCHAR line[256];
	TCHAR srcea[256], dstea[256];
	uae_u16 data[16], sexts[8], dexts[8];
	int sextcnt, dextcnt;
	int dcnt = 0;
	int cc = -1;
	int quick = 0;
	bool immrelpc = false;

	if (uaetcslen(sline) > 100)
		return -1;

	srcea[0] = dstea[0] = 0;
	parms[0] = 0;

	// strip all white space except first space
	p = line;
	bool firstsp = true;
	for (int i = 0; sline[i]; i++) {
		TCHAR c = sline[i];
		if (c == 32 && firstsp) {
			firstsp = false;
			*p++ = 32;
		}
		if (c <= 32)
			continue;
		*p++ = c;
	}
	*p = 0;

	to_upper(line, uaetcslen(line));

	p = line;
	while (*p && *p != ' ')
		p++;
	if (*p == ' ') {
		*p = 0;
		_tcscpy(parms, p + 1);
		my_trim(parms);
	}
	_tcscpy(ins, line);
	
	if (uaetcslen(ins) == 0)
		return 0;

	int size = 1;
	int inssize = -1;
	cp1 = _tcschr(line, '.');
	if (cp1) {
		size = cp1[1];
		if (size == 'W')
			size = 1;
		else if (size == 'L')
			size = 2;
		else if (size == 'B')
			size = 0;
		else
			return 0;
		inssize = size;
		line[cp1 - line] = 0;
		_tcscpy(ins, line);
	}

	TCHAR *parmp = parms;
	parmp = asm_parse_parm(parmp, srcea);
	if (!parmp)
		return 0;
	if (srcea[0]) {
		parmp = asm_parse_parm(parmp, dstea);
		if (!parmp)
			return 0;
	}

	int smode = -1;
	int dmode = -1;
	uae_u8 sreg = -1;
	uae_u8 dreg = -1;
	uae_u32 sval = 0;
	uae_u32 dval = 0;
	int ssize = -1;
	int dsize = -1;
	struct mnemolookup *lookup;

	dmode = asm_parse_mode(dstea, &dreg, &dval, &dextcnt, dexts);


	// Common alias
	if (!_tcscmp(ins, _T("BRA"))) {
		_tcscpy(ins, _T("BT"));
	} else if (!_tcscmp(ins, _T("BSR"))) {
		immrelpc = true;
	} else if (!_tcscmp(ins, _T("MOVEM"))) {
		_tcscpy(ins, _T("MVMLE"));
		if (!m68k_asm_parse_movem(srcea, dmode == Apdi)) {
			TCHAR tmp[256];
			_tcscpy(ins, _T("MVMEL"));
			_tcscpy(tmp, srcea);
			_tcscpy(srcea, dstea);
			_tcscpy(dstea, tmp);
			if (!m68k_asm_parse_movem(srcea, 0))
				return -1;
			dmode = asm_parse_mode(dstea, &dreg, &dval, &dextcnt, dexts);
		}
	} else if (!_tcscmp(ins, _T("MOVEC"))) {
		if (dmode == Dreg || dmode == Areg) {
			_tcscpy(ins, _T("MOVEC2"));
			if (!m68k_asm_parse_movec(srcea, dstea))
				return -1;
		} else {
			TCHAR tmp[256];
			_tcscpy(ins, _T("MOVE2C"));
			_tcscpy(tmp, srcea);
			_tcscpy(srcea, dstea);
			dstea[0] = 0;
			if (!m68k_asm_parse_movec(srcea, tmp))
				return -1;
		}
		dmode = -1;
	}
	
	if (dmode == Areg) {
		int l = uaetcslen(ins);
		if (l <= 2)
			return -1;
		TCHAR last = ins[l- 1];
		if (last == 'Q') {
			last = ins[l - 2];
			if (last != 'A') {
				ins[l - 1] = 'A';
				ins[l] = 'Q';
				ins[l + 1] = 0;
			}
		} else if (last != 'A') {
			TCHAR insa[256];
			_tcscpy(insa, ins);
			_tcscat(insa, _T("A"));
			for (lookup = lookuptab; lookup->name; lookup++) {
				if (!_tcscmp(insa, lookup->name)) {
					_tcscpy(ins, insa);
					break;
				}
			}
		}
	}

	bool fp = ins[0] == 'F';
	int tsize = size;

	if (ins[uaetcslen(ins) - 1] == 'Q' && uaetcslen(ins) > 3 && !fp) {
		quick = 1;
		ins[uaetcslen(ins) - 1] = 0;
		if (inssize < 0)
			tsize = 2;
	}

	for (lookup = lookuptab; lookup->name; lookup++) {
		if (!_tcscmp(ins, lookup->name))
			break;
	}
	if (!lookup->name) {
		// Check cc variants
		for (lookup = lookuptab; lookup->name; lookup++) {
			const TCHAR *ccp = _tcsstr(lookup->name, _T("cc"));
			if (ccp) {
				TCHAR tmp[256];
				for (int i = 0; i < (fp ? 32 : 16); i++) {
					const TCHAR *ccname = fp ? fpccnames[i] : ccnames[i];
					_tcscpy(tmp, lookup->name);
					_tcscpy(tmp + (ccp - lookup->name), ccname);
					if (tmp[_tcslen(tmp) - 1] == ' ')
						tmp[_tcslen(tmp) - 1] = 0;
					if (!_tcscmp(tmp, ins)) {
						_tcscpy(ins, lookup->name);
						cc = i;
						if (lookup->mnemo == i_DBcc || lookup->mnemo == i_Bcc) {
							// Bcc.B uses same encoding mode as MOVEQ
							immrelpc = true;
						}
						if (size == 0) {
							quick = 2;
						}
						break;
					}
				}
			}
			if (cc >= 0)
				break;
		}
	}

	if (!lookup->name)
		return 0;

	int mnemo = lookup->mnemo;

	int found = 0;
	int sizemask = 0;
	int unsized = 0;

	for (int round = 0; round < 9; round++) {

		if (!found && round == 8)
			return 0;

		if (round == 3) {
			bool isimm = srcea[0] == '#';
			if (immrelpc && !isimm) {
				TCHAR tmp[256];
				_tcscpy(tmp, srcea);
				srcea[0] = '#';
				_tcscpy(srcea + 1, tmp);
			}
			smode = asm_parse_mode(srcea, &sreg, &sval, &sextcnt, sexts);
			if (immrelpc && !isimm) {
				sval = sval - (pc + 2);
			}
			if (quick) {
				smode = immi;
				sreg = sval & 0xff;
			}
		}

		if (round == 1) {
			if (!quick && (sizemask == 1 || sizemask == 2 || sizemask == 4)) {
				tsize = 0;
				if (sizemask == 2)
					tsize = 1;
				else if (sizemask == 4)
					tsize = 2;
			} else {
				continue;
			}
		}
		if (round == 2 && !found) {
			unsized = 1;
		}

		if (round == 4 && smode == imm) {
			smode = imm0;
		} else if (round == 5 && smode == imm0) {
			smode = imm1;
		} else if (round == 6 && smode == imm1) {
			smode = imm2;
		} else if (round == 7 && smode == imm2) {
			smode = immi;
			sreg = sval & 0xff;
		} else if (round == 4) {
			round += 5 - 1;
		}

		for (int opcode = 0; opcode < 65536; opcode++) {
			struct instr *table = &table68k[opcode];
			if (table->mnemo != mnemo)
				continue;
			if (cc >= 0 && table->cc != cc)
				continue;

#if 0
			if (round == 0) {
				console_out_f(_T("%s OP=%04x S=%d SR=%d SM=%d SU=%d SP=%d DR=%d DM=%d DU=%d DP=%d SDU=%d\n"), lookup->name, opcode, table->size,
					table->sreg, table->smode, table->suse, table->spos,
					table->dreg, table->dmode, table->duse, table->dpos,
					table->sduse);
			}
#endif

			if (table->duse && !(table->dmode == dmode || (dmode >= imm && dmode <= imm2 && table->dmode >= imm && table->dmode <= imm2)))
				continue;
			if (round == 0) {
				sizemask |= 1 << table->size;
			}
			if (unsized > 0 && !table->unsized) {
				continue;
			}

			found++;

			if (round >= 3) {

				if (
					((table->size == tsize || table->unsized)) &&
					((!table->suse && smode < 0) || (table->suse && table->smode == smode)) &&
					((!table->duse && dmode < 0) || (table->duse && (table->dmode == dmode || (dmode == imm && (table->dmode >= imm && table->dmode <= imm2))))) &&
					((table->sreg == sreg || (table->smode >= absw && table->smode != immi))) &&
					((table->dreg == dreg || table->dmode >= absw))
					)
				{
					if (inssize >= 0 && tsize != inssize)
						continue;


					data[dcnt++] = opcode;
					asm_add_extensions(data, &dcnt, smode, sval, sextcnt, sexts, pc, tsize);
					if (smode >= 0)
						asm_add_extensions(data, &dcnt, dmode, dval, dextcnt, dexts, pc, tsize);
					for (int i = 0; i < dcnt; i++) {
						out[i] = data[i];
					}
					return dcnt;
				}

			}
		}
	}

	return 0;
}

static void resolve_if_jmp(TCHAR *s, uae_u32 addr)
{
	uae_u16 opcode = get_word_debug(addr);
	if (opcode == 0x4ef9) { // JMP x.l
		TCHAR *p = s + _tcslen(s);
		uae_u32 addr2 = get_long_debug(addr + 2);
		if (disasm_flags & (DISASM_FLAG_VAL_FORCE | DISASM_FLAG_VAL)) {
			_sntprintf(p, sizeof p, disasm_lc_hex(_T(" == $%08X ")), addr2);
			
		}
		showea_val(p + _tcslen(p), opcode, addr2, 4);
		TCHAR txt[256];
		bool ext;
		if (debugmem_get_segment(addr2, NULL, &ext, NULL, txt)) {
			if (ext) {
				_tcscat(p, _T(" "));
				_tcscat(p, txt);
			}
		}
	}
}

static bool mmu_op30_helper_get_fc(uae_u16 extra, TCHAR *out)
{
	switch (extra & 0x0018) {
	case 0x0010:
		_sntprintf(out, sizeof out, _T("#%d"), extra & 7);
		return true;
	case 0x0008:
		_sntprintf(out, sizeof out, _T("%c%d"), disasm_dreg, extra & 7);
		return true;
	case 0x0000:
		if (extra & 1) {
			_tcscpy(out, disasm_lc_reg(_T("DFC")));
		} else {
			_tcscpy(out, disasm_lc_reg(_T("SFC")));
		}
		return true;
	default:
		return false;
	}
}

static bool mmu_op30_invea(uae_u32 opcode)
{
	int eamode = (opcode >> 3) & 7;
	int rreg = opcode & 7;

	// Dn, An, (An)+, -(An), immediate and PC-relative not allowed
	if (eamode == 0 || eamode == 1 || eamode == 3 || eamode == 4 || (eamode == 7 && rreg > 1))
		return true;
	return false;
}

static uaecptr disasm_mmu030(uaecptr pc, uae_u16 opcode, uae_u16 extra, struct instr *dp, TCHAR *instrname, uae_u32 *seaddr2, int *actualea, int safemode)
{
	int type = extra >> 13;

	_tcscpy(instrname, _T("F-LINE (MMU 68030)"));
	pc += 2;

	switch (type)
	{
	case 0:
	case 2:
	case 3:
	{
		// PMOVE
		int preg = (extra >> 10) & 31;
		int rw = (extra >> 9) & 1;
		int fd = (extra >> 8) & 1;
		int unused = (extra & 0xff);
		const TCHAR *r = NULL;

		if (mmu_op30_invea(opcode))
			break;
		if (unused)
			break;
		if (rw && fd)
			break;
		switch (preg)
		{
		case 0x10:
			r = _T("TC");
			break;
		case 0x12:
			r = _T("SRP");
			break;
		case 0x13:
			r = _T("CRP");
			break;
		case 0x18:
			r = _T("MMUSR");
			break;
		case 0x02:
			r = _T("TT0");
			break;
		case 0x03:
			r = _T("TT1");
			break;
		}
		if (!r)
			break;

		_tcscpy(instrname, _T("PMOVE"));
		if (fd)
			_tcscat(instrname, _T("FD"));
		_tcscat(instrname, _T(" "));
		disasm_lc_mnemo(instrname);

		if (!rw) {
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, seaddr2, actualea, safemode);
			_tcscat(instrname, _T(","));
		}
		_tcscat(instrname, disasm_lc_reg(r));
		if (rw) {
			_tcscat(instrname, _T(","));
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, seaddr2, actualea, safemode);
		}
		break;
	}
	case 1:
	{
		// PLOAD/PFLUSH
		uae_u16 mode = (extra >> 8) & 31;
		int unused = (extra & (0x100 | 0x80 | 0x40 | 0x20));
		uae_u16 fc_mask = (extra & 0x00E0) >> 5;
		uae_u16 fc_bits = extra & 0x7f;
		TCHAR fc[10];

		if (unused)
			break;

		switch (mode) {
		case 0x00: // PLOAD W
		case 0x02: // PLOAD R
			if (mmu_op30_invea(opcode))
				break;
			if (!mmu_op30_helper_get_fc(extra, fc))
				break;
			_sntprintf(instrname, sizeof instrname, _T("PLOAD%c"), mode == 0 ? 'W' : 'R');
			disasm_lc_mnemo(instrname);
			_sntprintf(instrname + _tcslen(instrname), sizeof instrname, _T(" %s,"), fc);
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, seaddr2, actualea, safemode);
			break;
		case 0x04: // PFLUSHA
			if (fc_bits)
				break;
			_tcscpy(instrname, _T("PFLUSHA"));
			disasm_lc_mnemo(instrname);
			break;
		case 0x10: // FC
			if (!mmu_op30_helper_get_fc(extra, fc))
				break;
			_tcscpy(instrname, _T("PFLUSH"));
			disasm_lc_mnemo(instrname);
			_sntprintf(instrname + _tcslen(instrname), sizeof instrname, _T(" %s,%d"), fc, fc_mask);
			break;
		case 0x18: // FC + EA
			if (mmu_op30_invea(opcode))
				break;
			if (!mmu_op30_helper_get_fc(extra, fc))
				break;
			_tcscpy(instrname, _T("PFLUSH"));
			disasm_lc_mnemo(instrname);
			_sntprintf(instrname + _tcslen(instrname), sizeof instrname, _T(" %s,%d"), fc, fc_mask);
			_tcscat(instrname, _T(","));
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, seaddr2, actualea, safemode);
			break;
		}
		break;
	}
	case 4:
	{
		// PTEST
		int level = (extra & 0x1C00) >> 10;
		int rw = (extra >> 9) & 1;
		int a = (extra >> 8) & 1;
		int areg = (extra & 0xE0) >> 5;
		TCHAR fc[10];

		if (mmu_op30_invea(opcode))
			break;
		if (!mmu_op30_helper_get_fc(extra, fc))
			break;
		if (!level && a)
			break;
		_sntprintf(instrname, sizeof instrname, _T("PTEST%c"), rw ? 'R' : 'W');
		disasm_lc_mnemo(instrname);
		_sntprintf(instrname + _tcslen(instrname), sizeof instrname, _T(" %s,"), fc);
		pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, seaddr2, actualea, safemode);
		_sntprintf(instrname + _tcslen(instrname), sizeof instrname, _T(",#%d"), level);
		if (a)
			_sntprintf(instrname + _tcslen(instrname), sizeof instrname, _T(",%c%d"), disasm_areg, areg);
		break;
	}
	default:
		disasm_lc_mnemo(instrname);
		break;
	}
	return pc;
}

static uae_u16 get_disasm_word(uaecptr pc, uae_u16 *bufpc, int bufpcsizep, int offset)
{
	offset /= 2;
	if (bufpc) {
		if (bufpcsizep > offset) {
			return bufpc[offset];
		}
		return 0;
	} else {
		return get_word_debug(pc + offset * 2);
	}
}
static void add_disasm_word(uaecptr *pcp, uae_u16 **bufpcp, int *bufpcsizep, int add)
{
	if (*bufpcp) {
		*bufpcp += add;
		*bufpcsizep -= add;
	} else {
		*pcp += add;
	}
}

uae_u32 m68k_disasm_2(TCHAR *buf, int bufsize, uaecptr pc, uae_u16 *bufpc, int bufpcsize, uaecptr *nextpc, int cnt, uae_u32 *seaddr, uae_u32 *deaddr, uaecptr lastpc, int safemode)
{
	uae_u32 seaddr2;
	uae_u32 deaddr2;
	int actualea_src = 0;
	int actualea_dst = 0;

	if (!table68k)
		return 0;
	while (cnt-- > 0) {
		TCHAR instrname[256], *ccpt;
		TCHAR segout[256], segname[256];
		int i;
		uae_u32 opcode;
		uae_u16 extra;
		struct mnemolookup *lookup;
		struct instr *dp;
		uaecptr oldpc;
		uaecptr m68kpc_illg = 0;
		int illegal = 0;
		int segid, lastsegid;
		TCHAR *symbolpos;
		bool skip = false;

		seaddr2 = deaddr2 = 0xffffffff;
		oldpc = pc;
		opcode = get_disasm_word(pc, bufpc, bufpcsize, 0);
		extra = get_disasm_word(pc, bufpc, bufpcsize, 2);
		if (cpufunctbl[opcode] == op_illg_1 || cpufunctbl[opcode] == op_unimpl_1) {
			m68kpc_illg = pc + 2;
			illegal = 1;
		}

		dp = table68k + opcode;
		if (dp->mnemo == i_ILLG) {
			illegal = 0;
			opcode = 0x4AFC;
			dp = table68k + opcode;
		}
		for (lookup = lookuptab;lookup->mnemo != dp->mnemo; lookup++)
			;

		lastsegid = -1;
		bool exact = false;
		segid = 0;
		if (!bufpc) {
			if (lastpc != 0xffffffff) {
				lastsegid = debugmem_get_segment(lastpc, NULL, NULL, NULL, NULL);
			}
			segid = debugmem_get_segment(pc, &exact, NULL, segout, segname);
			if (segid && (lastsegid != -1 || exact) && (segid != lastsegid || pc == lastpc || exact)) {
				buf = buf_out(buf, &bufsize, _T("%s\n"), segname);
			}
		}
		symbolpos = buf;

		buf = buf_out (buf, &bufsize, disasm_lc_nhex(_T("%08X ")), pc);

		if (segid) {
			buf = buf_out(buf, &bufsize, _T("%s "), segout);
		}

		add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
		
		if (lookup->friendlyname)
			_tcscpy(instrname, lookup->friendlyname);
		else
			_tcscpy(instrname, lookup->name);
		ccpt = _tcsstr(instrname, _T("cc"));
		if (ccpt != 0) {
			if ((opcode & 0xf000) == 0xf000) {
				if (lookup->mnemo == i_FBcc) {
					_tcscpy(ccpt, fpccnames[opcode & 0x1f]);
				} else {
					_tcscpy(ccpt, fpccnames[extra & 0x1f]);
				}
			} else {
				_tcscpy(ccpt, ccnames[dp->cc]);
				if (dp->mnemo == i_Bcc && dp->cc == 0) {
					_tcscpy(ccpt, _T("RA")); // BT -> BRA
				}
			}
		}
		disasm_lc_mnemo(instrname);
		disasm_size(instrname, dp);

		if (lookup->mnemo == i_MOVEC2 || lookup->mnemo == i_MOVE2C) {
			uae_u16 imm = extra;
			uae_u16 creg = imm & 0x0fff;
			uae_u16 r = imm >> 12;
			TCHAR regs[16];
			const TCHAR *cname = _T("?");
			int j;
			for (j = 0; m2cregs[j].regname; j++) {
				if (m2cregs[j].regno == creg)
					break;
			}
			_sntprintf(regs, sizeof regs, _T("%c%d"), r >= 8 ? disasm_areg : disasm_dreg, r >= 8 ? r - 8 : r);
			if (m2cregs[j].regname)
				cname = m2cregs[j].regname;
			if (lookup->mnemo == i_MOVE2C) {
				_tcscat(instrname, regs);
				_tcscat(instrname, _T(","));
				_tcscat(instrname, cname);
			} else {
				_tcscat(instrname, cname);
				_tcscat(instrname, _T(","));
				_tcscat(instrname, regs);
			}
			int lvl = (currprefs.cpu_model - 68000) / 10;
			if (lvl == 6)
				lvl = 5;
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			if (lvl < 1 || !(m2cregs[j].flags & (1 << (lvl - 1))))
				illegal = -1;
		} else if (lookup->mnemo == i_CHK2) {
			TCHAR *p;
			if (!(extra & 0x0800)) {
				instrname[1] = 'M';
				instrname[2] = 'P';
				disasm_lc_mnemo(instrname);
			}
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			p = instrname + _tcslen(instrname);
			_sntprintf(p, sizeof p, _T(",%c%d"), (extra & 0x8000) ? disasm_areg : disasm_dreg, (extra >> 12) & 7);
		} else if (lookup->mnemo == i_CAS) {
			TCHAR *p = instrname + _tcslen(instrname);
			_sntprintf(p, sizeof p, _T("%c%d,%c%d,"), disasm_dreg, extra & 7, disasm_dreg, (extra >> 6) & 7);
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &deaddr2, &actualea_dst, safemode);
		} else if (lookup->mnemo == i_CAS2) {
			TCHAR *p = instrname + _tcslen(instrname);
			uae_u16 extra2 = get_word_debug(pc + 2);
			_sntprintf(p, sizeof p, _T("%c%d:%c%d,%c%d,%c%d,(%c%d):(%c%d)"),
				disasm_dreg, extra & 7, disasm_dreg, extra2 & 7, disasm_dreg, (extra >> 6) & 7, disasm_dreg, (extra2 >> 6) & 7,
				(extra & 0x8000) ? disasm_areg : disasm_dreg, (extra >> 12) & 7,
				(extra2 & 0x8000) ? disasm_areg : disasm_dreg, (extra2 >> 12) & 7);
			add_disasm_word(&pc, &bufpc, &bufpcsize, 4);
		} else if (lookup->mnemo == i_ORSR || lookup->mnemo == i_ANDSR || lookup->mnemo == i_EORSR) {
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			_tcscat(instrname, dp->size == sz_byte ? disasm_lc_reg(_T(",CCR")) : disasm_lc_reg(_T(",SR")));
		} else if (lookup->mnemo == i_MVR2USP) {
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			_tcscat(instrname, disasm_lc_reg(_T(",USP")));
		} else if (lookup->mnemo == i_MVUSP2R) {
			_tcscat(instrname, disasm_lc_reg(_T("USP,")));
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
		} else if (lookup->mnemo == i_MV2SR) {
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			_tcscat(instrname, dp->size == sz_byte ? disasm_lc_reg(_T(",CCR")) : disasm_lc_reg(_T(",SR")));
		} else if (lookup->mnemo == i_MVSR2) {		
			_tcscat(instrname, dp->size == sz_byte ? disasm_lc_reg(_T("CCR,")) : disasm_lc_reg(_T("SR,")));
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
		} else if (lookup->mnemo == i_MVMEL) {
			uae_u16 mask = extra;
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			pc = ShowEA (NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			movemout (instrname, mask, dp->dmode, 0, true);
		} else if (lookup->mnemo == i_MVMLE) {
			uae_u16 mask = extra;
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			if (movemout(instrname, mask, dp->dmode, 0, false))
				_tcscat(instrname, _T(","));
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &deaddr2, &actualea_dst, safemode);
		} else if (lookup->mnemo == i_MULL) {
			extra = get_disasm_word(pc, bufpc, bufpcsize, 0);
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			if (extra & 0x0800) // signed/unsigned
				instrname[3] = 'S';
			else
				instrname[3] = 'U';
			disasm_lc_mnemo(instrname);
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			TCHAR *p = instrname + _tcslen(instrname);
			if (extra & 0x0400)
				_stprintf(p, _T(",%c%d:%c%d"), disasm_dreg, extra & 7, disasm_dreg, (extra >> 12) & 7);
			else
				_stprintf(p, _T(",%c%d"), disasm_dreg, (extra >> 12) & 7);
		} else if (lookup->mnemo == i_DIVL) {
			extra = get_disasm_word(pc, bufpc, bufpcsize, 0);
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			if (extra & 0x0800) // signed/unsigned
				instrname[3] = 'S';
			else
				instrname[3] = 'U';
			if (!(extra & 0x0400)) {
				// DIVS.L/DIVU.L->DIVSL.L/DIVUL.L
				instrname[8] = 0;
				instrname[7] = ' ';
				instrname[6] = instrname[5];
				instrname[5] = instrname[4];
				instrname[4] = 'L';
			}
			disasm_lc_mnemo(instrname);
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			TCHAR* p = instrname + _tcslen(instrname);
			_sntprintf(p, sizeof p, _T(",%c%d:%c%d"), disasm_dreg, extra & 7, disasm_dreg, (extra >> 12) & 7);
		} else if (lookup->mnemo == i_MOVES) {
			TCHAR *p;
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			if (!(extra & 0x0800)) {
				pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &deaddr2, &actualea_dst, safemode);
				p = instrname + _tcslen(instrname);
				_sntprintf(p, sizeof p, _T(",%c%d"), (extra & 0x8000) ? disasm_areg : disasm_dreg, (extra >> 12) & 7);
			} else {
				p = instrname + _tcslen(instrname);
				_sntprintf(p, sizeof p, _T("%c%d,"), (extra & 0x8000) ? disasm_areg : disasm_dreg, (extra >> 12) & 7);
				pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			}
		} else if (lookup->mnemo == i_BFEXTS || lookup->mnemo == i_BFEXTU ||
				   lookup->mnemo == i_BFCHG || lookup->mnemo == i_BFCLR ||
				   lookup->mnemo == i_BFFFO || lookup->mnemo == i_BFINS ||
				   lookup->mnemo == i_BFSET || lookup->mnemo == i_BFTST) {
			TCHAR *p;
			int reg = -1;

			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			p = instrname + _tcslen(instrname);
			if (lookup->mnemo == i_BFEXTS || lookup->mnemo == i_BFEXTU || lookup->mnemo == i_BFFFO || lookup->mnemo == i_BFINS)
				reg = (extra >> 12) & 7;
			if (lookup->mnemo == i_BFINS)
				_sntprintf(p, sizeof p, _T("%c%d,"), disasm_dreg, reg);
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			_tcscat(instrname, _T(" {"));
			p = instrname + _tcslen(instrname);
			if (extra & 0x0800)
				_sntprintf(p, sizeof p, _T("%c%d"), disasm_dreg, (extra >> 6) & 7);
			else
				_sntprintf(p, sizeof p, _T("%d"), (extra >> 6) & 31);
			_tcscat(instrname, _T(":"));
			p = instrname + _tcslen(instrname);
			if (extra & 0x0020)
				_sntprintf(p, sizeof p, _T("%c%d"), disasm_dreg, extra & 7);
			else
				_sntprintf(p, sizeof p, _T("%d"), extra  & 31);
			_tcscat(instrname, _T("}"));
			p = instrname + _tcslen(instrname);
			if (lookup->mnemo == i_BFFFO || lookup->mnemo == i_BFEXTS || lookup->mnemo == i_BFEXTU)
				_sntprintf(p, sizeof p, _T(",%c%d"), disasm_dreg, reg);
		} else if (lookup->mnemo == i_CPUSHA || lookup->mnemo == i_CPUSHL || lookup->mnemo == i_CPUSHP ||
			lookup->mnemo == i_CINVA || lookup->mnemo == i_CINVL || lookup->mnemo == i_CINVP) {
			if ((opcode & 0xc0) == 0xc0)
				_tcscat(instrname, disasm_lc_reg(_T("BC")));
			else if (opcode & 0x80)
				_tcscat(instrname, disasm_lc_reg(_T("IC")));
			else if (opcode & 0x40)
				_tcscat(instrname, disasm_lc_reg(_T("DC")));
			else
				_tcscat(instrname, _T("?"));
			if (lookup->mnemo == i_CPUSHL || lookup->mnemo == i_CPUSHP || lookup->mnemo == i_CINVL || lookup->mnemo == i_CINVP) {
				TCHAR *p = instrname + _tcslen(instrname);
				_sntprintf(p, sizeof p, _T(",(%c%d)"), disasm_areg, opcode & 7);
			}
		} else if (lookup->mnemo == i_MOVE16) {
			TCHAR *p = instrname + _tcslen(instrname);
			if (opcode & 0x20) {
				_sntprintf(p, sizeof p, _T("(%c%d)+,(%c%d)+"), disasm_areg, opcode & 7, disasm_areg, (extra >> 12) & 7);
				add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			} else {
				uae_u32 addr = get_long_debug(pc);
				int ay = opcode & 7;
				pc += 4;
				switch ((opcode >> 3) & 3)
				{
				case 0:
					_sntprintf(p, sizeof p, _T("(%c%d)+,$%08x"), disasm_areg, ay, addr);
					break;
				case 1:
					_sntprintf(p, sizeof p, _T("$%08x,(%c%d)+"), addr, disasm_areg, ay);
					break;
				case 2:
					_sntprintf(p, sizeof p, _T("(%c%d),$%08x"), disasm_areg, ay, addr);
					break;
				case 3:
					_sntprintf(p, sizeof p, _T("$%08x,(%c%d)"), addr, disasm_areg, ay);
					break;
				}
			}
		} else if (lookup->mnemo == i_PACK || lookup->mnemo == i_UNPK) {
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			_tcscat(instrname, _T(","));
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &deaddr2, &actualea_dst, safemode);
			extra = get_word_debug(pc);
			_sntprintf(instrname + _tcslen(instrname), sizeof instrname, disasm_lc_hex(_T(",#$%04X")), extra);
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
		} else if (lookup->mnemo == i_LPSTOP) {
			if (extra == 0x01c0) {
				uae_u16 extra2 = get_word_debug(pc + 2);
				_tcscpy(instrname, _T("LPSTOP"));
				disasm_lc_mnemo(instrname);
				_sntprintf(instrname + _tcslen(instrname), sizeof instrname, disasm_lc_hex(_T(" #$%04X")), extra2);
				add_disasm_word(&pc, &bufpc, &bufpcsize, 4);
			} else {
				_tcscpy(instrname, _T("ILLG"));
				disasm_lc_mnemo(instrname);
				_sntprintf(instrname + _tcslen(instrname), sizeof instrname, disasm_lc_hex(_T(" #$%04X")), extra);
				add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			}
		} else if (lookup->mnemo == i_CALLM) {
			TCHAR *p = instrname + _tcslen(instrname);
			_sntprintf(p, sizeof p, _T("#%d,"), extra & 255);
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
		} else if (lookup->mnemo == i_FDBcc) {
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			_tcscat(instrname, _T(","));
			pc = ShowEA(NULL, pc, opcode, 0, imm1, sz_word, instrname, &deaddr2, &actualea_dst, safemode);
		} else if (lookup->mnemo == i_FTRAPcc) {
			pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &seaddr2, &actualea_src, safemode);
			int mode = opcode & 7;
			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			if (mode == 2) {
				pc = ShowEA(NULL, pc, opcode, 0, imm1, sz_word, instrname, NULL, NULL, safemode);
			} else if (mode == 3) {
				pc = ShowEA(NULL, pc, opcode, 0, imm2, sz_long, instrname, NULL, NULL, safemode);
			}
		} else if (lookup->mnemo == i_FPP) {
			TCHAR *p;
			int ins = extra & 0x7f;
			int size = (extra >> 10) & 7;

			add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			if ((extra & 0xfc00) == 0x5c00) { // FMOVECR (=i_FPP with source specifier = 7)
				fpdata fp;
				fpu_get_constant(&fp, extra & 0x7f);
				_tcscpy(instrname, _T("FMOVECR.X"));
				disasm_lc_mnemo(instrname);
				_sntprintf(instrname + _tcslen(instrname), sizeof instrname, _T(" #0x%02x [%s],%s%d"), extra & 0x7f, fpp_print(&fp, 0), disasm_fpreg, (extra >> 7) & 7);
			} else if ((extra & 0x8000) == 0x8000) { // FMOVEM or FMOVE control register
				int dr = (extra >> 13) & 1;
				int mode;
				int dreg = (extra >> 4) & 7;
				int regmask, fpmode;
				
				if (extra & 0x4000) {
					mode = (extra >> 11) & 3;
					regmask = extra & 0xff;  // FMOVEM FPx
					fpmode = 1;
					_tcscpy(instrname, _T("FMOVEM.X "));
					disasm_lc_mnemo(instrname);
				} else {
					mode = 0;
					regmask = (extra >> 10) & 7;  // FMOVEM or FMOVE control
					fpmode = 2;
					_tcscpy(instrname, _T("FMOVEM.L "));
					if (regmask == 1 || regmask == 2 || regmask == 4)
						_tcscpy(instrname, _T("FMOVE.L "));
					disasm_lc_mnemo(instrname);
					int msk = regmask & 2;
					if (regmask & 1) {
						msk |= 4;
					}
					if (regmask & 4) {
						msk |= 1;
					}
					regmask = msk;
				}
				p = instrname + _tcslen(instrname);
				if (dr) {
					if (mode & 1)
						_sntprintf(p, sizeof p, _T("%c%d"), disasm_dreg, dreg);
					else
						movemout(p, regmask, dp->dmode, fpmode, false);
					_tcscat(instrname, _T(","));
					pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &deaddr2, &actualea_dst, safemode);
				} else {
					if ((opcode & 0x3f) == 0x3c && !(extra & 0x2000)) {
						// FMOVEM #xxx,control registers (strange one, can have up to 3 long word immediates)
						bool entry = false;
						if (!(extra & (0x400 | 0x800 | 0x1000)))
							extra |= 0x400;
						for (int i = 0; i < 3; i++) {
							if (extra & (0x1000 >> i)) {
								if (entry)
									_tcscat(p, _T("/"));
								entry = true;
								p = instrname + _tcslen(instrname);
								_sntprintf(p, sizeof p, disasm_lc_hex(_T("#$%08X")), get_ilong_debug(pc));
								add_disasm_word(&pc, &bufpc, &bufpcsize, 4);
							}
						}
					} else {
						pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &deaddr2, &actualea_dst, safemode);
					}
					p = instrname + _tcslen(instrname);
					if (mode & 1)
						_sntprintf(p, sizeof p, _T(",%c%d"), disasm_dreg, dreg);
					else
						movemout(p, regmask, dp->dmode, fpmode, true);
				}
			} else {
				if (fpuopcodes[ins]) {
					_tcscpy(instrname, fpuopcodes[ins]);
				} else {
					_sntprintf(instrname, sizeof instrname, _T("%s?%02X"), disasm_lc_reg(_T("F")), ins);
				}
				disasm_lc_mnemo(instrname);
				if ((extra & (0x8000 | 0x4000 | 0x2000)) == (0x4000 | 0x2000)) { // FMOVE to memory/data register
					int kfactor = extra & 0x7f;
					_tcscpy(instrname, _T("FMOVE."));
					_tcscat(instrname, fpsizes[size]);
					disasm_lc_mnemo(instrname);
					_tcscat(instrname, _T(" "));
					p = instrname + _tcslen(instrname);
					_sntprintf(p, sizeof p, _T("%s%d,"), disasm_fpreg, (extra >> 7) & 7);
					pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, fpsizeconv[size], instrname, &deaddr2, &actualea_dst, safemode);
					p = instrname + _tcslen(instrname);
					if (size == 7) {
						_sntprintf(p, sizeof p, _T(" {%c%d}"), disasm_dreg, (kfactor >> 4));
					} else if (kfactor) {
						if (kfactor & 0x40)
							kfactor |= ~0x3f;
						_sntprintf(p, sizeof p, _T(" {%d}"), kfactor);
					}
				} else if ((extra & (0x8000 | 0x2000)) == 0) {
					if (extra & 0x4000) { // source is EA
						_tcscat(instrname, _T("."));
						_tcscat(instrname, disasm_lc_size(fpsizes[size]));
						_tcscat(instrname, _T(" "));
						pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, fpsizeconv[size], instrname, &seaddr2, &actualea_src, safemode);
					} else { // source is FPx
						p = instrname + _tcslen(instrname);
						_tcscat(p, disasm_lc_reg(_T(".X")));
						p = instrname + _tcslen(instrname);
						_sntprintf(p, sizeof p, _T(" %s%d"), disasm_fpreg, (extra >> 10) & 7);
					}
					p = instrname + _tcslen(instrname);
					if (ins >= 0x30 && ins < 0x38) { // FSINCOS
						p = instrname + _tcslen(instrname);
						_sntprintf(p, sizeof p, _T(",%s%d"), disasm_fpreg, extra & 7);
						p = instrname + _tcslen(instrname);
						_sntprintf(p, sizeof p, _T(",%s%d"), disasm_fpreg, (extra >> 7) & 7);
					} else {
						if ((extra & 0x4000) || (((extra >> 7) & 7) != ((extra >> 10) & 7))) {
							_sntprintf(p, sizeof p, _T(",%s%d"), disasm_fpreg, (extra >> 7) & 7);
						}
					}
				}
				if (ins >= 0x40 && currprefs.fpu_model >= 68881 && fpuopcodes[ins]) {
					_tcscat(instrname, _T(" (68040+)"));
				}
			}
		} else if (lookup->mnemo == i_MMUOP030) {
			pc = disasm_mmu030(pc, opcode, extra, dp, instrname, &seaddr2, &actualea_src, safemode);
		} else if ((opcode & 0xf000) == 0xa000) {
			_tcscpy(instrname, _T("A-LINE"));
			disasm_lc_mnemo(instrname);
		} else {
			if (lookup->mnemo == i_FBcc && (opcode & 0x1f) == 0 && extra == 0) {
				_tcscpy(instrname, _T("FNOP"));
				disasm_lc_mnemo(instrname);
				add_disasm_word(&pc, &bufpc, &bufpcsize, 2);
			} else {
				if (dp->suse) {
					pc = ShowEA(NULL, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, &seaddr2, &actualea_src, safemode);

					// JSR x(a6) / JMP x(a6)
					if (opcode == 0x4ea8 + 6 || opcode == 0x4ee8 + 6) {
						TCHAR sname[256];
						if (debugger_get_library_symbol(m68k_areg(regs, 6), 0xffff0000 | extra, sname)) {
							TCHAR *p = instrname + _tcslen(instrname);
							_sntprintf(p, sizeof p, _T(" %s"), sname);
							resolve_if_jmp(instrname, m68k_areg(regs, 6) + (uae_s16)extra);
						}
					}
					// show target address if JSR x(pc) + JMP xxxx combination
					if (opcode == 0x4eba && seaddr2 && instrname[0]) { // JSR x(pc)
						resolve_if_jmp(instrname, seaddr2);
					}
				}
				if (dp->suse && dp->duse)
					_tcscat(instrname, _T(","));
				if (dp->duse) {
					pc = ShowEA(NULL, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, &deaddr2, &actualea_dst, safemode);
				}
				if (lookup->mnemo == i_RTS || lookup->mnemo == i_RTD || lookup->mnemo == i_RTR || lookup->mnemo == i_RTE) {
					uaecptr a = regs.regs[15];
					TCHAR eas[100];
					eas[0] = 0;
					if (lookup->mnemo == i_RTE || lookup->mnemo == i_RTR) {
						a += 2;
					}
					if (disasm_flags & DISASM_FLAG_EA) {
						_sntprintf(eas, sizeof eas, disasm_lc_hex(_T(" == $%08X")), get_ilong_debug(a));
					}
					_tcscat(instrname, eas);
				}
			}
		}

		if (disasm_flags & DISASM_FLAG_WORDS) {
			for (i = 0; i < (pc - oldpc) / 2 && i < disasm_max_words; i++) {
				buf = buf_out(buf, &bufsize, disasm_lc_nhex(_T("%04X ")), get_word_debug(oldpc + i * 2));
			}
			while (i++ < disasm_min_words) {
				buf = buf_out(buf, &bufsize, _T("     "));
			}
		}

		if (illegal)
			buf = buf_out (buf, &bufsize, _T("[ "));
		buf = buf_out (buf, &bufsize, instrname);
		if (illegal)
			buf = buf_out (buf, &bufsize, _T(" ]"));

		if (ccpt != 0) {
			uaecptr addr2 = deaddr2 != 0xffffffff ? deaddr2 : seaddr2;
			if (deaddr)
				*deaddr = pc;
			if ((opcode & 0xf000) == 0xf000) {
				if (currprefs.fpu_model) {
					if (disasm_flags & DISASM_FLAG_EA) {
						buf = buf_out(buf, &bufsize, disasm_lc_hex(_T(" == $%08X")), addr2);
					}
					if (disasm_flags & DISASM_FLAG_CC) {
						if (fpp_cond(dp->cc)) {
							buf = buf_out(buf, &bufsize, _T(" (T)"));
						} else {
							buf = buf_out(buf, &bufsize, _T(" (F)"));
						}
					}
				}
			} else {
				if (dp->mnemo == i_Bcc || dp->mnemo == i_DBcc) {
					if (disasm_flags & DISASM_FLAG_EA) {
						buf = buf_out(buf, &bufsize, disasm_lc_hex(_T(" == $%08X")), addr2);
					}
					if (disasm_flags & DISASM_FLAG_CC) {
						if (cctrue(dp->cc)) {
							buf = buf_out(buf, &bufsize, _T(" (T)"));
						} else {
							buf = buf_out(buf, &bufsize, _T(" (F)"));
						}
					}
				} else {
					if (disasm_flags & DISASM_FLAG_CC) {
						if (cctrue(dp->cc)) {
							buf = buf_out(buf, &bufsize, _T(" (T)"));
						} else {
							buf = buf_out(buf, &bufsize, _T(" (F)"));
						}
					}
				}
			}
		} else if ((opcode & 0xff00) == 0x6100) { /* BSR */
			if (deaddr)
				*deaddr = pc;
			if (disasm_flags & DISASM_FLAG_EA) {
				buf = buf_out(buf, &bufsize, disasm_lc_hex(_T(" == $%08X")), seaddr2);
			}
		}
		buf = buf_out (buf, &bufsize, _T("\n"));

		for (uaecptr segpc = oldpc; segpc < pc; segpc++) {
			TCHAR segout[256];
			if (debugmem_get_symbol(segpc, segout, sizeof(segout) / sizeof(TCHAR))) {
				_tcscat(segout, _T(":\n"));
				if (bufsize > uaetcslen(segout)) {
					memmove(symbolpos + uaetcslen(segout), symbolpos, (uaetcslen(symbolpos) + 1) * sizeof(TCHAR));
					memcpy(symbolpos, segout, uaetcslen(segout) * sizeof(TCHAR));
					bufsize -= uaetcslen(segout);
					buf += uaetcslen(segout);
					symbolpos += uaetcslen(segout);
				}
			}
		}

		int srcline = -1;
		for (uaecptr segpc = oldpc; segpc < pc; segpc++) {
			TCHAR sourceout[256];
			int line = debugmem_get_sourceline(segpc, sourceout, sizeof(sourceout) / sizeof(TCHAR));
			if (line < 0)
				break;
			if (srcline != line) {
				if (srcline < 0)
					buf = buf_out(buf, &bufsize, _T("\n"));
				buf = buf_out(buf, &bufsize, sourceout);
				srcline = line;
			}
		}
		if (srcline >= 0) {
			buf = buf_out(buf, &bufsize, _T("\n"));
		}

		if (illegal > 0)
			pc = m68kpc_illg;
	}
	if (nextpc)
		*nextpc = pc;
	if (seaddr)
		*seaddr = seaddr2;
	if (deaddr)
		*deaddr = deaddr2;
	return (actualea_src ? 1 : 0) | (actualea_dst ? 2 : 0);
}


/*************************************************************
Disasm the m68kcode at the given address into instrname
and instrcode
*************************************************************/
void sm68k_disasm (TCHAR *instrname, TCHAR *instrcode, uaecptr addr, uaecptr *nextpc, uaecptr lastpc)
{
	TCHAR *ccpt;
	uae_u32 opcode;
	struct mnemolookup *lookup;
	struct instr *dp;
	uaecptr pc, oldpc;

	pc = oldpc = addr;
	opcode = get_word_debug (pc);
	if (cpufunctbl[opcode] == op_illg_1) {
		opcode = 0x4AFC;
	}
	dp = table68k + opcode;
	for (lookup = lookuptab;lookup->mnemo != dp->mnemo; lookup++);

	pc += 2;

	_tcscpy (instrname, lookup->name);
	ccpt = _tcsstr (instrname, _T("cc"));
	if (ccpt != 0) {
		_tcsncpy (ccpt, ccnames[dp->cc], 2);
	}
	switch (dp->size){
	case sz_byte: _tcscat (instrname, _T(".B ")); break;
	case sz_word: _tcscat (instrname, _T(".W ")); break;
	case sz_long: _tcscat (instrname, _T(".L ")); break;
	default: _tcscat (instrname, _T("   ")); break;
	}

	if (dp->suse) {
		pc = ShowEA (0, pc, opcode, dp->sreg, dp->smode, dp->size, instrname, NULL, NULL, 0);
	}
	if (dp->suse && dp->duse)
		_tcscat (instrname, _T(","));
	if (dp->duse) {
		pc = ShowEA (0, pc, opcode, dp->dreg, dp->dmode, dp->size, instrname, NULL, NULL, 0);
	}
	if (instrcode)
	{
		int i;
		for (i = 0; i < (pc - oldpc) / 2; i++)
		{
			_sntprintf (instrcode, sizeof instrcode, _T("%04x "), get_iword_debug (oldpc + i * 2));
			instrcode += _tcslen (instrcode);
		}
	}
	if (nextpc)
		*nextpc = pc;
}
#endif
