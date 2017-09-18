
/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Decruncher reinitialization
 *
 */

#include <string.h>

#include "cdata.h"
#include "u_init.h"
#include "u_quick.h"
#include "u_medium.h"
#include "u_deep.h"
#include "u_heavy.h"

void Init_Decrunchers(void){
	dms_quick_text_loc = 251;
	dms_medium_text_loc = 0x3fbe;
	dms_heavy_text_loc = 0;
	dms_deep_text_loc = 0x3fc4;
	dms_init_deep_tabs = 1;
	memset(dms_text,0,0x3fc8);
	dms_lastlen = 0;
	dms_np = 0;
}

