#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "savestate.h"
#include "blitter.h"
#include "blitfunc.h"

blitter_func * const blitfunc_dofast[256] = {
blitdofast_0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_2a, 0, 0, 0, 0, 0, 
blitdofast_30, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_3a, 0, blitdofast_3c, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_4a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_6a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_8a, 0, blitdofast_8c, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_9a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
blitdofast_a8, 0, blitdofast_aa, 0, 0, 0, 0, 0, 
0, blitdofast_b1, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_ca, 0, blitdofast_cc, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
blitdofast_d8, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_e2, 0, 0, 0, 0, 0, 
0, 0, blitdofast_ea, 0, 0, 0, 0, 0, 
blitdofast_f0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_fa, 0, blitdofast_fc, 0, 0, 0
};

blitter_func * const blitfunc_dofast_desc[256] = {
blitdofast_desc_0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_2a, 0, 0, 0, 0, 0, 
blitdofast_desc_30, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_3a, 0, blitdofast_desc_3c, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_4a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_6a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_8a, 0, blitdofast_desc_8c, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_9a, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
blitdofast_desc_a8, 0, blitdofast_desc_aa, 0, 0, 0, 0, 0, 
0, blitdofast_desc_b1, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_ca, 0, blitdofast_desc_cc, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
blitdofast_desc_d8, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_e2, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_ea, 0, 0, 0, 0, 0, 
blitdofast_desc_f0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, blitdofast_desc_fa, 0, blitdofast_desc_fc, 0, 0, 0
};
