
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

extern USHORT lastlen, np;

void Init_Decrunchers(void){
	quick_text_loc = 251;
	medium_text_loc = 0x3fbe;
	heavy_text_loc = 0;
	deep_text_loc = 0x3fc4;
	init_deep_tabs = 1;
	memset(text,0,0x3fc8);
	lastlen = 0;
	np = 0;
}

