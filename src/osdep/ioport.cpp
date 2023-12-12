#include "sysconfig.h"
#include "sysdeps.h"

#include "ioport.h"
#include "cia.h"
#include "options.h"
#include "vpar.h"

#define para_log write_log

int parallel_direct_write_status (uae_u8 v, uae_u8 dir)
{
#ifdef WITH_VPAR
	if (vpar_enabled()) {
        return vpar_direct_write_status(v, dir);
    }
#endif
	return 0;
}

int parallel_direct_read_status (uae_u8 *vp)
{
#ifdef WITH_VPAR
	if (vpar_enabled()) {
        return vpar_direct_read_status(vp);
    }
#endif
	return 0;
}

int parallel_direct_write_data (uae_u8 v, uae_u8 dir)
{
#ifdef WITH_VPAR
	if (vpar_enabled()) {
        return vpar_direct_write_data(v, dir);
    }
#endif
	return 0;
}

int parallel_direct_read_data (uae_u8 *v)
{
#ifdef WITH_VPAR
	if (vpar_enabled()) {
        return vpar_direct_read_data(v);
    }
#endif
	return 0;
}
