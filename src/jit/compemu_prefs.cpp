/********************************************************************
 * Preferences handling. This is just a convenient place to put it  *
 ********************************************************************/
extern bool have_done_picasso;

bool check_prefs_changed_comp (bool checkonly)
{
	bool changed = 0;
	static int cachesize_prev, comptrust_prev;
	static bool canbang_prev;

	special_mem_default = currprefs.comptrustbyte ? (S_READ | S_WRITE | S_N_ADDR) : 0;

	if (currprefs.comptrustbyte != changed_prefs.comptrustbyte ||
		currprefs.comptrustword != changed_prefs.comptrustword ||
		currprefs.comptrustlong != changed_prefs.comptrustlong ||
		currprefs.comptrustnaddr!= changed_prefs.comptrustnaddr ||
		currprefs.compnf != changed_prefs.compnf ||
		currprefs.comp_hardflush != changed_prefs.comp_hardflush ||
		currprefs.comp_constjump != changed_prefs.comp_constjump ||
		currprefs.compfpu != changed_prefs.compfpu ||
		currprefs.fpu_strict != changed_prefs.fpu_strict ||
		currprefs.cachesize != changed_prefs.cachesize)
		changed = 1;

	if (checkonly)
		return changed;

	currprefs.comptrustbyte = changed_prefs.comptrustbyte;
	currprefs.comptrustword = changed_prefs.comptrustword;
	currprefs.comptrustlong = changed_prefs.comptrustlong;
	currprefs.comptrustnaddr= changed_prefs.comptrustnaddr;
	currprefs.compnf = changed_prefs.compnf;
	currprefs.comp_hardflush = changed_prefs.comp_hardflush;
	currprefs.comp_constjump = changed_prefs.comp_constjump;
	currprefs.compfpu = changed_prefs.compfpu;
	currprefs.fpu_strict = changed_prefs.fpu_strict;

	if (currprefs.cachesize != changed_prefs.cachesize) {
		if (currprefs.cachesize && !changed_prefs.cachesize) {
			cachesize_prev = currprefs.cachesize;
			comptrust_prev = currprefs.comptrustbyte;
			canbang_prev = canbang;
		} else if (!currprefs.cachesize && changed_prefs.cachesize == cachesize_prev) {
			changed_prefs.comptrustbyte = currprefs.comptrustbyte = comptrust_prev;
			changed_prefs.comptrustword = currprefs.comptrustword = comptrust_prev;
			changed_prefs.comptrustlong = currprefs.comptrustlong = comptrust_prev;
			changed_prefs.comptrustnaddr = currprefs.comptrustnaddr = comptrust_prev;
		}
		currprefs.cachesize = changed_prefs.cachesize;
		alloc_cache();
		changed = 1;
	}

	// Turn off illegal-mem logging when using JIT...
	if(currprefs.cachesize)
		currprefs.illegal_mem = changed_prefs.illegal_mem;// = 0;

	if ((!canbang || !currprefs.cachesize) && currprefs.comptrustbyte != 1) {
		// Set all of these to indirect when canbang == 0
		currprefs.comptrustbyte = 1;
		currprefs.comptrustword = 1;
		currprefs.comptrustlong = 1;
		currprefs.comptrustnaddr= 1;

		changed_prefs.comptrustbyte = 1;
		changed_prefs.comptrustword = 1;
		changed_prefs.comptrustlong = 1;
		changed_prefs.comptrustnaddr= 1;

		changed = 1;

		if (currprefs.cachesize)
			write_log (_T("JIT: Reverting to \"indirect\" access, because canbang is zero!\n"));
	}

	if (changed)
		write_log (_T("JIT: cache=%d. b=%d w=%d l=%d fpu=%d nf=%d inline=%d hard=%d\n"),
		currprefs.cachesize,
		currprefs.comptrustbyte, currprefs.comptrustword, currprefs.comptrustlong, 
		currprefs.compfpu, currprefs.compnf, currprefs.comp_constjump, currprefs.comp_hardflush);

	return changed;
}
