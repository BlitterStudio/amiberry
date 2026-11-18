/********************************************************************
 * Preferences handling. This is just a convenient place to put it  *
 ********************************************************************/
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "jit/compemu.h"

extern bool have_done_picasso;

/* Direct access needs memory laid out for it (canbang). Memory laid out for
   indirect access or without the JIT stays so until the next reset
   (init_shm()): run indirect until then, but keep the choice. */
static int comp_trust_effective(int v)
{
	return canbang && changed_prefs.cachesize ? v : 1;
}

bool check_prefs_changed_comp (bool checkonly)
{
	bool changed = 0;
	static bool pending_logged;

	const bool pending = jit_direct_pending(&changed_prefs);
	if (pending && !pending_logged)
		write_log(_T("JIT: \"direct\" access takes effect after a reset, memory is laid out for \"indirect\" access\n"));
	pending_logged = pending;

	special_mem_default = currprefs.comptrustbyte ? (S_READ | S_WRITE | S_N_ADDR) : 0;

	if (currprefs.comptrustbyte != comp_trust_effective(changed_prefs.comptrustbyte) ||
		currprefs.comptrustword != comp_trust_effective(changed_prefs.comptrustword) ||
		currprefs.comptrustlong != comp_trust_effective(changed_prefs.comptrustlong) ||
		currprefs.comptrustnaddr != comp_trust_effective(changed_prefs.comptrustnaddr) ||
		currprefs.compnf != changed_prefs.compnf ||
		currprefs.comp_hardflush != changed_prefs.comp_hardflush ||
		currprefs.comp_constjump != changed_prefs.comp_constjump ||
		currprefs.compfpu != changed_prefs.compfpu ||
		currprefs.fpu_strict != changed_prefs.fpu_strict ||
		currprefs.cachesize != changed_prefs.cachesize)
		changed = 1;

	if (checkonly)
		return changed;

	const bool was_direct = currprefs.cachesize && !currprefs.comptrustbyte;
	currprefs.comptrustbyte = comp_trust_effective(changed_prefs.comptrustbyte);
	currprefs.comptrustword = comp_trust_effective(changed_prefs.comptrustword);
	currprefs.comptrustlong = comp_trust_effective(changed_prefs.comptrustlong);
	currprefs.comptrustnaddr = comp_trust_effective(changed_prefs.comptrustnaddr);
	currprefs.compnf = changed_prefs.compnf;
	currprefs.comp_hardflush = changed_prefs.comp_hardflush;
	currprefs.comp_constjump = changed_prefs.comp_constjump;
	currprefs.compfpu = changed_prefs.compfpu;
	currprefs.fpu_strict = changed_prefs.fpu_strict;

	if (currprefs.cachesize != changed_prefs.cachesize) {
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

		changed = 1;

		if (currprefs.cachesize)
			write_log (_T("JIT: Reverting to \"indirect\" access, because canbang is zero!\n"));
	}

	// A reset may have laid out Direct memory while ROM protection was
	// still skipped for the previous effective Indirect preferences.
	if (!was_direct && currprefs.cachesize && !currprefs.comptrustbyte)
		protect_roms(true);

	if (changed)
		write_log (_T("JIT: cache=%d. b=%d w=%d l=%d fpu=%d nf=%d inline=%d hard=%d\n"),
		currprefs.cachesize,
		currprefs.comptrustbyte, currprefs.comptrustword, currprefs.comptrustlong, 
		currprefs.compfpu, currprefs.compnf, currprefs.comp_constjump, currprefs.comp_hardflush);

	return changed;
}
