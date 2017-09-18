/********************************************************************
 * Preferences handling. This is just a convenient place to put it  *
 ********************************************************************/
extern bool have_done_picasso;

bool check_prefs_changed_comp (bool checkonly)
{
  bool changed = 0;

	if (currprefs.fpu_strict != changed_prefs.fpu_strict ||
		currprefs.cachesize != changed_prefs.cachesize)
		changed = 1;

	if (checkonly)
		return changed;

	currprefs.fpu_strict = changed_prefs.fpu_strict;

  if (currprefs.cachesize != changed_prefs.cachesize) {
	  currprefs.cachesize = changed_prefs.cachesize;
	  alloc_cache();
	  changed = 1;
  }

  return changed;
}
