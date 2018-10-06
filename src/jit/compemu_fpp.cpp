/*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68881 emulation
  *
  * Copyright 1996 Herman ten Brugge
  * Adapted for JIT compilation (c) Bernd Meyer, 2000
  * Modified 2005 Peter Keunecke
 */

#include <math.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "compemu.h"
#include "flags_arm.h"

#if defined(USE_JIT_FPU)

extern void fpp_to_exten(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3);

static const int sz1[8] = { 4, 4, 12, 12, 2, 8, 1, 0 };
static const int sz2[8] = { 4, 4, 12, 12, 2, 8, 2, 0 };

/* return the required floating point precision or -1 for failure, 0=E, 1=S, 2=D */
STATIC_INLINE int comp_fp_get (uae_u32 opcode, uae_u16 extra, int treg)
{
	int reg = opcode & 7;
	int mode = (opcode >> 3) & 7;
	int size = (extra >> 10) & 7;

	if (size == 3 || size == 7) /* 3 = packed decimal, 7 is not defined */
		return -1;
	switch (mode) {
		case 0: /* Dn */
  		switch (size) {
  			case 0: /* Long */
    			fmov_l_rr (treg, reg);
    			return 2;
  			case 1: /* Single */
    			fmov_s_rr (treg, reg);
    			return 1;
  			case 4: /* Word */
    			fmov_w_rr (treg, reg);
    			return 1;
  			case 6: /* Byte */
  			  fmov_b_rr (treg, reg);
    			return 1;
  			default:
    			return -1;
  		}
		case 1: /* An,  invalid mode */
  		return -1;
		case 2: /* (An) */
  		mov_l_rr (S1, reg + 8);
  		break;
		case 3: /* (An)+ */
  		mov_l_rr (S1, reg + 8);
      arm_ADD_l_ri8(reg + 8, (reg == 7 ? sz2[size] : sz1[size]));
  		break;
		case 4: /* -(An) */
		  arm_SUB_l_ri8(reg + 8, (reg == 7 ? sz2[size] : sz1[size]));
  		mov_l_rr (S1, reg + 8);
  		break;
		case 5: /* (d16,An)  */
		{
			uae_u32 off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
			mov_l_rr (S1, reg + 8);
			lea_l_brr (S1, S1, off);
			break;
		}
		case 6: /* (d8,An,Xn) or (bd,An,Xn) or ([bd,An,Xn],od) or ([bd,An],Xn,od) */
		{
			uae_u32 dp = comp_get_iword ((m68k_pc_offset += 2) - 2);
			calc_disp_ea_020 (reg + 8, dp, S1);
			break;
		}
		case 7:
  		switch (reg) {
  			case 0: /* (xxx).W */
  			{
  				uae_u32 off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
  				mov_l_ri (S1, off);
  				break;
  			}
  			case 1: /* (xxx).L */
  			{
  				uae_u32 off = comp_get_ilong ((m68k_pc_offset += 4) - 4);
  				mov_l_ri (S1, off);
  				break;
  			}
  			case 2: /* (d16,PC) */
  			{
  				uae_u32 address = start_pc + ((uae_char*) comp_pc_p - (uae_char*) start_pc_p) +
  					m68k_pc_offset;
  				uae_s32 PC16off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
  				mov_l_ri (S1, address + PC16off);
  				break;
  			}
  			case 3: /* (d8,PC,Xn) or (bd,PC,Xn) or ([bd,PC,Xn],od) or ([bd,PC],Xn,od) */
  			  return -1; /* rarely used, fallback to non-JIT */
  			case 4: /* # < data >; Constants should be converted just once by the JIT */
    			m68k_pc_offset += sz2[size];
    			switch (size) {
    				case 0:
    				{
    					uae_s32 li = comp_get_ilong(m68k_pc_offset - 4);
    					float si = (float)li;
    
    					if (li == (int)si) {
						    //write_log ("converted immediate LONG constant to SINGLE\n");
    						fmov_s_ri(treg, *(uae_u32 *)&si);
    						return 1;
    					}
					    //write_log ("immediate LONG constant\n");
    					fmov_l_ri(treg, *(uae_u32 *)&li);
    					return 2;
    				}
    				case 1:
				      //write_log (_T("immediate SINGLE constant\n"));
      				fmov_s_ri(treg, comp_get_ilong(m68k_pc_offset - 4));
      				return 1;
    				case 2:
            {
				      //write_log (_T("immediate LONG DOUBLE constant\n"));
              uae_u32 wrd1, wrd2, wrd3;
              fpdata tmp;
              wrd3 = comp_get_ilong(m68k_pc_offset - 4);
              wrd2 = comp_get_ilong(m68k_pc_offset - 8);
              wrd1 = comp_get_iword(m68k_pc_offset - 12) << 16;
              fpp_to_exten(&tmp, wrd1, wrd2, wrd3);
              mov_l_ri(S1, ((uae_u32*)&tmp)[0]);
              mov_l_ri(S2, ((uae_u32*)&tmp)[1]);
              fmov_d_rrr (treg, S1, S2);
      				return 0;
            }
    				case 4:
    				{
    					float si = (float)(uae_s16)comp_get_iword(m68k_pc_offset-2);
    
					    //write_log (_T("converted immediate WORD constant %f to SINGLE\n"), si);
    					fmov_s_ri(treg, *(uae_u32 *)&si);
    					return 1;
    				}
    				case 5:
    				{
					    //write_log (_T("immediate DOUBLE constant\n"));
              mov_l_ri(S1, comp_get_ilong(m68k_pc_offset - 4));
              mov_l_ri(S2, comp_get_ilong(m68k_pc_offset - 8));
              fmov_d_rrr (treg, S1, S2);
    					return 2;
    				}
    				case 6:
    				{
    					float si = (float)(uae_s8)comp_get_ibyte(m68k_pc_offset - 2);
    
    					//write_log (_T("converted immediate BYTE constant to SINGLE\n"));
    					fmov_s_ri(treg, *(uae_u32 *)&si);
    					return 1;
    				}
    				default: /* never reached */
      				return -1;
          }
   			default: /* never reached */
  	  		return -1;
     }
  }
  
	switch (size) {
		case 0: /* Long */
  		readlong (S1, S2);
  		fmov_l_rr (treg, S2);
  		return 2;
		case 1: /* Single */
  		readlong (S1, S2);
			fmov_s_rr (treg, S2);
  		return 1;
		case 2: /* Long Double */
		  fp_to_exten_rm (treg, S1);
		  return 0;
		case 4: /* Word */
  		readword (S1, S2);
			fmov_w_rr (treg, S2);
  		return 1;
		case 5: /* Double */
		  fp_to_double_rm (treg, S1);
  		return 2;
		case 6: /* Byte */
  		readbyte (S1, S2);
		  fmov_b_rr (treg, S2);
  		return 1;
		default:
  		return -1;
  }
	return -1;
}

/* return of -1 means failure, >=0 means OK */
STATIC_INLINE int comp_fp_put (uae_u32 opcode, uae_u16 extra)
{
	int reg = opcode & 7;
	int sreg = (extra >> 7) & 7;
	int mode = (opcode >> 3) & 7;
	int size = (extra >> 10) & 7;

	if (size == 3 || size == 7) /* 3 = packed decimal, 7 is not defined */
		return -1;
	switch (mode) {
		case 0: /* Dn */
  		switch (size) {
  			case 0: /* FMOVE.L FPx, Dn */
  			  fmov_to_l_rr(reg, sreg);
    			return 0;
  			case 1: /* FMOVE.S FPx, Dn */
  			  fmov_to_s_rr(reg, sreg);
    			return 0;
  			case 4: /* FMOVE.W FPx, Dn */
  			  fmov_to_w_rr(reg, sreg);
    			return 0;
  			case 6: /* FMOVE.B FPx, Dn */
  			  fmov_to_b_rr(reg, sreg);
			    return 0;
			  default:
			    return -1;
      }
		case 1: /* An, invalid mode */
  		return -1;
		case 2: /* (An) */
		  mov_l_rr (S1, reg + 8);
		  break;
		case 3: /* (An)+ */
		  mov_l_rr (S1, reg + 8);
      arm_ADD_l_ri8(reg + 8, (reg == 7 ? sz2[size] : sz1[size]));
		  break;
		case 4: /* -(An) */
	    arm_SUB_l_ri8(reg + 8, (reg == 7 ? sz2[size] : sz1[size]));
		  mov_l_rr (S1, reg + 8);
		  break;
		case 5: /* (d16,An) */
		{
			uae_u32 off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
			mov_l_rr (S1, reg + 8);
			arm_ADD_l_ri (S1, off);
			break;
		}
		case 6: /* (d8,An,Xn) or (bd,An,Xn) or ([bd,An,Xn],od) or ([bd,An],Xn,od) */
		{
			uae_u32 dp = comp_get_iword ((m68k_pc_offset += 2) - 2);
			calc_disp_ea_020 (reg + 8, dp, S1);
			break;
		}
		case 7:
		  switch (reg) {
			  case 0: /* (xxx).W */
			  {
				  uae_u32 off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
				  mov_l_ri (S1, off);
				  break;
			  }
			  case 1: /* (xxx).L */
			  {
				  uae_u32 off = comp_get_ilong ((m68k_pc_offset += 4) - 4);
				  mov_l_ri (S1, off);
				  break;
			  }
			  default: /* All other modes are not allowed for FPx to <EA> */
			    write_log (_T ("JIT FMOVE FPx,<EA> Mode is not allowed %04x %04x\n"), opcode, extra);
		    return -1;
		  }
  }
	switch (size) {
		case 0: /* Long */
    	fmov_to_l_rr(S2, sreg);
		  writelong_clobber (S1, S2);
		  return 0;
		case 1: /* Single */
      fmov_to_s_rr(S2, sreg);
	    writelong_clobber (S1, S2);
	    return 0;
		case 2:/* Long Double */
		  fp_from_exten_mr (S1, sreg);
		  return 0;
		case 4: /* Word */
		  fmov_to_w_rr(S2, sreg);
		  writeword_clobber (S1, S2);
		  return 0;
		case 5: /* Double */
			fp_from_double_mr(S1, sreg);
		  return 0;
		case 6: /* Byte */
      fmov_to_b_rr(S2, sreg);
		  writebyte (S1, S2);
		  return 0;
		default:
  		return -1;
  }
	return -1;
}

/* return -1 for failure, or register number for success */
STATIC_INLINE int comp_fp_adr (uae_u32 opcode)
{
	uae_s32 off;
	int mode = (opcode >> 3) & 7;
	int reg = opcode & 7;

	switch (mode) {
		case 2:
		case 3:
		case 4:
		  mov_l_rr (S1, 8 + reg);
		  return S1;
		case 5:
		  off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
      mov_l_rr (S1, 8 + reg);
		  arm_ADD_l_ri (S1, off);
		  return S1;
		case 7:
		  switch (reg) {
			  case 0:
			    off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
			    mov_l_ri (S1, off);
			    return S1;
			  case 1:
			    off = comp_get_ilong ((m68k_pc_offset += 4) - 4);
			    mov_l_ri (S1, off);
			    return S1;
		  }
		default:
  		return -1;
	}
}

void comp_fdbcc_opp (uae_u32 opcode, uae_u16 extra)
{
	FAIL (1);
	return;
}

void comp_fscc_opp (uae_u32 opcode, uae_u16 extra)
{
	int reg;

	if (!currprefs.compfpu) {
		FAIL (1);
		return;
	}

	if (extra & 0x20) {  /* only cc from 00 to 1f are defined */
	  FAIL (1);
	  return;
	}
	if ((opcode & 0x38) != 0) { /* We can only do to integer register */
		FAIL (1);
		return;
	}

	fflags_into_flags ();
	reg = (opcode & 7);

	if (!(opcode & 0x38)) {
	  switch (extra & 0x0f) { /* according to fpp.c, the 0x10 bit is ignored */
		  case 0: fp_fscc_ri(reg, NATIVE_CC_F_NEVER); break;
		  case 1: fp_fscc_ri(reg, NATIVE_CC_EQ); break;
		  case 2: fp_fscc_ri(reg, NATIVE_CC_F_OGT); break;
		  case 3: fp_fscc_ri(reg, NATIVE_CC_F_OGE); break;
		  case 4: fp_fscc_ri(reg, NATIVE_CC_F_OLT); break;
		  case 5: fp_fscc_ri(reg, NATIVE_CC_F_OLE); break;
		  case 6: fp_fscc_ri(reg, NATIVE_CC_F_OGL); break;
		  case 7: fp_fscc_ri(reg, NATIVE_CC_F_OR); break;
		  case 8: fp_fscc_ri(reg, NATIVE_CC_F_UN); break;
		  case 9: fp_fscc_ri(reg, NATIVE_CC_F_UEQ); break;
		  case 10: fp_fscc_ri(reg, NATIVE_CC_F_UGT); break;
		  case 11: fp_fscc_ri(reg, NATIVE_CC_F_UGE); break;
		  case 12: fp_fscc_ri(reg, NATIVE_CC_F_ULT); break;
		  case 13: fp_fscc_ri(reg, NATIVE_CC_F_ULE); break;
		  case 14: fp_fscc_ri(reg, NATIVE_CC_NE); break;
		  case 15: fp_fscc_ri(reg, NATIVE_CC_AL); break;
	  }
  }
}

void comp_ftrapcc_opp (uae_u32 opcode, uaecptr oldpc)
{
	FAIL (1);
	return;
}

void comp_fbcc_opp (uae_u32 opcode)
{
	uae_u32 start_68k_offset = m68k_pc_offset;
	uae_u32 off, v1, v2;
	int cc;

	if (!currprefs.compfpu) {
		FAIL (1);
		return;
	}

	if (opcode & 0x20) {  /* only cc from 00 to 1f are defined */
		FAIL (1);
		return;
	}
	if (!(opcode & 0x40)) {
		off = (uae_s32) (uae_s16) comp_get_iword ((m68k_pc_offset += 2) - 2);
	}
	else {
		off = comp_get_ilong ((m68k_pc_offset += 4) - 4);
	}

	/* according to fpp.c, the 0x10 bit is ignored
	   (it handles exception handling, which we don't
	   do, anyway ;-) */
	cc = opcode & 0x0f;
  if(cc == 0)
    return; /* jump never */

	/* Note, "off" will sometimes be (unsigned) "negative", so the following
         * uintptr can be > 0xffffffff, but the result will be correct due to
         * wraparound when truncated to 32 bit in the call to mov_l_ri. */
	mov_l_ri(S1, (uintptr)
		(comp_pc_p + off - (m68k_pc_offset - start_68k_offset)));
	mov_l_ri(PC_P, (uintptr) comp_pc_p);

	/* Now they are both constant. Might as well fold in m68k_pc_offset */
	arm_ADD_l_ri (S1, m68k_pc_offset);
	arm_ADD_l_ri (PC_P, m68k_pc_offset);
	m68k_pc_offset = 0;

	v1 = get_const (PC_P);
	v2 = get_const (S1);
	fflags_into_flags ();

	switch (cc) {
		case 1: register_branch (v1, v2, NATIVE_CC_EQ); break;
		case 2: register_branch (v1, v2, NATIVE_CC_F_OGT); break;
		case 3: register_branch (v1, v2, NATIVE_CC_F_OGE); break;
		case 4: register_branch (v1, v2, NATIVE_CC_F_OLT); break;
		case 5: register_branch (v1, v2, NATIVE_CC_F_OLE); break;
		case 6: register_branch (v1, v2, NATIVE_CC_F_OGL); break;
		case 7: register_branch (v1, v2, NATIVE_CC_F_OR); break;
		case 8: register_branch (v1, v2, NATIVE_CC_F_UN); break;
		case 9: register_branch (v1, v2, NATIVE_CC_F_UEQ); break;
		case 10: register_branch (v1, v2, NATIVE_CC_F_UGT); break;
		case 11: register_branch (v1, v2, NATIVE_CC_F_UGE); break;
		case 12: register_branch (v1, v2, NATIVE_CC_F_ULT); break;
		case 13: register_branch (v1, v2, NATIVE_CC_F_ULE); break;
		case 14: register_branch (v1, v2, NATIVE_CC_NE); break;
		case 15: register_branch (v2, v2, NATIVE_CC_AL); break;
	}
}

void comp_fsave_opp (uae_u32 opcode)
{
	FAIL (1);
	return;
}

void comp_frestore_opp (uae_u32 opcode)
{
	FAIL (1);
	return;
}

static uae_u32 dhex_pi[]    ={0x54442D18, 0x400921FB};
static uae_u32 dhex_exp_1[] ={0x8B145769, 0x4005BF0A};
static uae_u32 dhex_l2_e[]  ={0x652B82FE, 0x3FF71547};
static uae_u32 dhex_ln_2[]  ={0xFEFA39EF, 0x3FE62E42};
static uae_u32 dhex_ln_10[] ={0xBBB55516, 0x40026BB1};
static uae_u32 dhex_l10_2[] ={0x509F79FF, 0x3FD34413};
static uae_u32 dhex_l10_e[] ={0x1526E50E, 0x3FDBCB7B};
static uae_u32 dhex_1e16[]  ={0x37E08000, 0x4341C379};
static uae_u32 dhex_1e32[]  ={0xB5056E17, 0x4693B8B5};
static uae_u32 dhex_1e64[]  ={0xE93FF9F5, 0x4D384F03};
static uae_u32 dhex_1e128[] ={0xF9301D32, 0x5A827748};
static uae_u32 dhex_1e256[] ={0x7F73BF3C, 0x75154FDD};
static uae_u32 dhex_inf[]   ={0x00000000, 0x7ff00000};
static uae_u32 dhex_nan[]   ={0xffffffff, 0x7fffffff};
extern double fp_1e8;

void comp_fpp_opp (uae_u32 opcode, uae_u16 extra)
{
	int reg;
	int sreg, prec = 0;
	int	dreg = (extra >> 7) & 7;
	int source = (extra >> 13) & 7;
	int	opmode = extra & 0x7f;

   if (special_mem) {
    FAIL(1);
    return;
  } 

	if (!currprefs.compfpu) {
		FAIL (1);
		return;
	}
	switch (source) {
		case 3: /* FMOVE FPx, <EA> */
		  if (comp_fp_put (opcode, extra) < 0)
			  FAIL (1);
		  return;
		case 4: /* FMOVE.L  <EA>, ControlReg */
		  if (!(opcode & 0x30)) { /* Dn or An */
			  if (extra & 0x1000) { /* FPCR */
				  mov_l_mr (uae_p32(&regs.fpcr), opcode & 15);
				  return;
			  }
			  if (extra & 0x0800) { /* FPSR */
				  FAIL (1);
				  return;
				  // set_fpsr(m68k_dreg (regs, opcode & 15));
			  }
			  if (extra & 0x0400) { /* FPIAR */
				  mov_l_mr (uae_p32(&regs.fpiar), opcode & 15); return;
			  }
		  }
		  else if ((opcode & 0x3f) == 0x3c) {
			  if (extra & 0x1000) { /* FPCR */
				  uae_u32 val = comp_get_ilong ((m68k_pc_offset += 4) - 4);
				  mov_l_mi (uae_p32(&regs.fpcr), val);
	        switch(val & 0x30) {
	          case 0x00:
	            // round to nearest
	            roundingmode(0x00000000);
	            break;
	          case 0x10:
	            // round toward minus infinity
	            roundingmode(0x00800000);
	            break;
	          case 0x01:
	            // round toward plus infinity
	            roundingmode(0x00400000);
	            break;
	          case 0x11:
	          default:
	            // round towards zero
	            roundingmode(0x00c00000);
	            break;
	        }
				  return;
			  }
			  if (extra & 0x0800) { /* FPSR */
				  FAIL (1);
				  return;
			  }
			  if (extra & 0x0400) { /* FPIAR */
				  uae_u32 val = comp_get_ilong ((m68k_pc_offset += 4) - 4);
				  mov_l_mi (uae_p32(&regs.fpiar), val);
				  return;
			  }
		  }
		  FAIL (1);
		  return;
		case 5: /* FMOVE.L  ControlReg, <EA> */
		  if (!(opcode & 0x30)) { /* Dn or An */
			  if (extra & 0x1000) { /* FPCR */
				  mov_l_rm (opcode & 15, uae_p32(&regs.fpcr)); return;
			  }
			  if (extra & 0x0800) { /* FPSR */
				  FAIL (1);
				  return;
			  }
			  if (extra & 0x0400) { /* FPIAR */
				  mov_l_rm (opcode & 15, uae_p32(&regs.fpiar)); return;
			  }
		  }
		  FAIL (1);
		  return;
		case 6:
		case 7:
		{
			uae_u32 list = 0;
			int incr = 0;
			if (extra & 0x2000) {
				int ad;

				/* FMOVEM FPP->memory */
				switch ((extra >> 11) & 3) { /* Get out early if failure */
					case 0:
					case 2:
					  break;
					case 1:
					case 3:
					default:
					  FAIL (1); 
            return;
				}
				ad = comp_fp_adr (opcode);
				if (ad < 0) {
					m68k_setpc (m68k_getpc () - 4);
					op_illg (opcode);
					return;
				}
				switch ((extra >> 11) & 3) {
					case 0:	/* static pred */
					  list = extra & 0xff;
					  incr = -1;
					  break;
					case 2:	/* static postinc */
					  list = extra & 0xff;
					  incr = 1;
					  break;
					case 1:	/* dynamic pred */
					case 3:	/* dynamic postinc */
  					abort ();
				}
				if (incr < 0) { /* Predecrement */
					for (reg = 7; reg >= 0; reg--) {
						if (list & 0x80) {
							sub_l_ri (ad, 12);
							fp_from_exten_mr (ad, reg);
						}
						list <<= 1;
					}
				} else { /* Postincrement */
					for (reg = 0; reg <= 7; reg++) {
						if (list & 0x80) {
							fp_from_exten_mr (ad, reg);
							arm_ADD_l_ri (ad, 12);
						}
						list <<= 1;
					}
				}
				if ((opcode & 0x38) == 0x18)
					mov_l_rr ((opcode & 7) + 8, ad);
				if ((opcode & 0x38) == 0x20)
					mov_l_rr ((opcode & 7) + 8, ad);
			} else {
				/* FMOVEM memory->FPP */

				int ad;
				switch ((extra >> 11) & 3) { /* Get out early if failure */
					case 0:
					case 2:
					  break;
					case 1:
					case 3:
					default:
					  FAIL (1); 
            return;
				}
				ad = comp_fp_adr (opcode);
				if (ad < 0) {
					m68k_setpc (m68k_getpc () - 4);
					op_illg (opcode);
					return;
				}
				switch ((extra >> 11) & 3) {
					case 0:	/* static pred */
					  list = extra & 0xff;
					  incr = -1;
					  break;
					case 2:	/* static postinc */
					  list = extra & 0xff;
					  incr = 1;
					  break;
					case 1:	/* dynamic pred */
					case 3:	/* dynamic postinc */
  					abort ();
				}

				if (incr < 0) {
					// not reached
					for (reg = 7; reg >= 0; reg--) {
						if (list & 0x80) {
							sub_l_ri (ad, 12);
              fp_to_exten_rm(reg, ad);
						}
						list <<= 1;
					}
				} else {
					for (reg = 0; reg <= 7; reg++) {
						if (list & 0x80) {
              fp_to_exten_rm(reg, ad);
							arm_ADD_l_ri (ad, 12);
						}
						list <<= 1;
					}
				}
				if ((opcode & 0x38) == 0x18)
					mov_l_rr ((opcode & 7) + 8, ad);
				if ((opcode & 0x38) == 0x20)
					mov_l_rr ((opcode & 7) + 8, ad);
			}
		}
    return;
		case 2: /* from <EA> to FPx */
		  dont_care_fflags ();
		  if ((extra & 0xfc00) == 0x5c00) { /* FMOVECR */
			  //write_log (_T("JIT FMOVECR %x\n"), opmode);
			  switch (opmode) {
				  case 0x00:
				    fmov_d_rm (dreg, uae_p32(&dhex_pi));
				    break;
				  case 0x0b:
				    fmov_d_rm (dreg, uae_p32(&dhex_l10_2));
				    break;
				  case 0x0c:
				    fmov_d_rm (dreg, uae_p32(&dhex_exp_1));
				    break;
				  case 0x0d:
				    fmov_d_rm (dreg, uae_p32(&dhex_l2_e));
				    break;
				  case 0x0e:
				    fmov_d_rm (dreg, uae_p32(&dhex_l10_e));
				    break;
				  case 0x0f:
            fmov_d_ri_0 (dreg);
				    break;
				  case 0x30:
				    fmov_d_rm (dreg, uae_p32(&dhex_ln_2));
				    break; 
				  case 0x31:
				    fmov_d_rm (dreg, uae_p32(&dhex_ln_10));
				    break;
				  case 0x32:
            fmov_d_ri_1 (dreg);
				    break;
				  case 0x33:
            fmov_d_ri_10 (dreg);
				    break;
				  case 0x34:
				    fmov_d_ri_100 (dreg);
				    break;
				  case 0x35:
				    fmov_l_ri (dreg, 10000);
				    break;
				  case 0x36:
				    fmov_rm (dreg, uae_p32(&fp_1e8));
				    break;
				  case 0x37:
				    fmov_d_rm (dreg, uae_p32(&dhex_1e16));
				    break;
				  case 0x38:
				    fmov_d_rm (dreg, uae_p32(&dhex_1e32));
				    break;
				  case 0x39:
				    fmov_d_rm (dreg, uae_p32(&dhex_1e64));
				    break;
				  case 0x3a:
				    fmov_d_rm (dreg, uae_p32(&dhex_1e128));
				    break;
				  case 0x3b:
				    fmov_d_rm (dreg, uae_p32(&dhex_1e256));
				    break;
				  default:
				    FAIL (1);
				    return;
			  }
			  fmov_rr (FP_RESULT, dreg);
			  return;
      }
		  if (opmode & 0x20) /* two operands, so we need a scratch reg */
			  sreg = FS1;
		  else /* one operand only, thus we can load the argument into dreg */
			  sreg = dreg;
		  if(opmode >= 0x30 && opmode <= 0x37) {
				// get out early for unsupported ops
			  FAIL (1);
			  return;
		  }
		  if ((prec = comp_fp_get (opcode, extra, sreg)) < 0) {
			  FAIL (1);
			  return;
		  }
		  if (!opmode) { /* FMOVE  <EA>,FPx */
			  fmov_rr (FP_RESULT, dreg);
			  return;
		  }
		  /* no break here for <EA> to dreg */
		case 0: /* directly from sreg to dreg */
		  if (!source) { /* no <EA> */
			  dont_care_fflags ();
			  sreg = (extra >> 10) & 7;
		  }
		  switch (opmode) {
			  case 0x00: /* FMOVE */
			    fmov_rr (dreg, sreg);
			    break;
			  case 0x01: /* FINT */
			    frndint_rr (dreg, sreg);
			    break;
			  case 0x02: /* FSINH */
			    ffunc_rr (sinh, dreg, sreg);
			    break;
			  case 0x03: /* FINTRZ */
			    frndintz_rr (dreg, sreg);
			    break;
		    case 0x04: /* FSQRT */
		      fsqrt_rr (dreg, sreg);
		      break;
		    case 0x06: /* FLOGNP1 */
		      ffunc_rr (log1p, dreg, sreg);
		      break;
		    case 0x08: /* FETOXM1 */
		      ffunc_rr (expm1, dreg, sreg);
		      break;
		    case 0x09: /* FTANH */
		      ffunc_rr (tanh, dreg, sreg);
		      break;
		    case 0x0a: /* FATAN */
		      ffunc_rr (atan, dreg, sreg);
		      break;
		    case 0x0c: /* FASIN */
		      ffunc_rr (asin, dreg, sreg);
		      break;
		    case 0x0d: /* FATANH */
		      ffunc_rr (atanh, dreg, sreg);
		      break;
		    case 0x0e: /* FSIN */
		      ffunc_rr (sin, dreg, sreg);
		      break;
		    case 0x0f: /* FTAN */
		      ffunc_rr (tan, dreg, sreg);
		      break;
		    case 0x10: /* FETOX */
		      ffunc_rr (exp, dreg, sreg);
		      break;
		    case 0x11: /* FTWOTOX */
		      fpowx_rr (2, dreg, sreg);
		      break;
		    case 0x12: /* FTENTOX */
		      fpowx_rr (10, dreg, sreg);
		      break;
			  case 0x14: /* FLOGN */
			    ffunc_rr (log, dreg, sreg);
			    break;
			  case 0x15: /* FLOG10 */
			    ffunc_rr (log10, dreg, sreg);
			    break;
			  case 0x16: /* FLOG2 */
			    ffunc_rr (log2, dreg, sreg);
			    break;
			  case 0x18: /* FABS */
			    fabs_rr (dreg, sreg);
			    break;
			  case 0x19: /* FCOSH */
			    ffunc_rr (cosh, dreg, sreg);
			    break;
			  case 0x1a: /* FNEG */
			    fneg_rr (dreg, sreg);
			    break;
  			case 0x1c: /* FACOS */
			    ffunc_rr (acos, dreg, sreg);
			    break;
			  case 0x1d: /* FCOS */
			    ffunc_rr (cos, dreg, sreg);
			    break;
			  case 0x20: /* FDIV */
			    fdiv_rr (dreg, sreg);
			    break;
			  case 0x21: /* FMOD */
			    fmod_rr (dreg, sreg);
			    break;
			  case 0x22: /* FADD */
			    fadd_rr (dreg, sreg);
			    break;
			  case 0x23: /* FMUL */
			    fmul_rr (dreg, sreg);
			    break;
			  case 0x24: /* FSGLDIV */
			    fsgldiv_rr (dreg, sreg);
			    break;
			  case 0x60: /* FSDIV */
			    fdiv_rr (dreg, sreg);
			    if (!currprefs.fpu_strict) /* faster, but less strict rounding */
				    break;
			    fcuts_r (dreg);
			    break;
			  case 0x25: /* FREM */
			    frem1_rr (dreg, sreg);
			    break;
        case 0x27: /* FSGLMUL */
			    fsglmul_rr (dreg, sreg);
			    break;
			  case 0x63: /* FSMUL */
			    fmul_rr (dreg, sreg);
			    if (!currprefs.fpu_strict) /* faster, but less strict rounding */
				    break;
			    fcuts_r (dreg);
			    break;
			  case 0x28: /* FSUB */
			    fsub_rr (dreg, sreg);
			    break;
			  case 0x30: /* FSINCOS */
			  case 0x31:
			  case 0x32:
			  case 0x33:
			  case 0x34:
			  case 0x35:
			  case 0x36:
			  case 0x37:
				  FAIL (1);
				  return;
			  case 0x38: /* FCMP */
			    fmov_rr (FP_RESULT, dreg);
			    fsub_rr (FP_RESULT, sreg);
			    return;
			  case 0x3a: /* FTST */
			    fmov_rr (FP_RESULT, sreg);
			    return;
		    case 0x40: /* FSMOVE */
			    if (prec == 1 || !currprefs.fpu_strict) {
				    if (sreg != dreg) /* no <EA> */
					    fmov_rr (dreg, sreg);
			    }
			    else {
  			    fmovs_rr (dreg, sreg);
			    }
          break;
			  case 0x44: /* FDMOVE */
          if (sreg != dreg) /* no <EA> */
  			    fmov_rr (dreg, sreg);
          break;
			  case 0x41: /* FSSQRT */
			    fsqrt_rr (dreg, sreg);
			    if (!currprefs.fpu_strict) /* faster, but less strict rounding */
				    break;
			    fcuts_r (dreg);
			    break;
			  case 0x45: /* FDSQRT */
          fsqrt_rr (dreg, sreg);
          break;
			  case 0x58: /* FSABS */
			    fabs_rr (dreg, sreg);
			    if (prec != 1 && currprefs.fpu_strict)
			      fcuts_r (dreg);
			    break;
			  case 0x5a: /* FSNEG */
			    fneg_rr (dreg, sreg);
			    if (prec != 1 && currprefs.fpu_strict)
			      fcuts_r (dreg);
			    break;
			  case 0x5c: /* FDABS */
			    fabs_rr (dreg, sreg);
			    break;
			  case 0x5e: /* FDNEG */
			    fneg_rr (dreg, sreg);
			    break;
			  case 0x62: /* FSADD */
			    fadd_rr (dreg, sreg);
			    if (!currprefs.fpu_strict) /* faster, but less strict rounding */
				    break;
			    fcuts_r (dreg);
			    break;
			  case 0x64: /* FDDIV */
			    fdiv_rr (dreg, sreg);
			    break;
			  case 0x66: /* FDADD */
			    fadd_rr (dreg, sreg);
			    break;
			  case 0x67: /* FDMUL */
			    fmul_rr (dreg, sreg);
			    break;
			  case 0x68: /* FSSUB */
			    fsub_rr (dreg, sreg);
			    if (!currprefs.fpu_strict) /* faster, but less strict rounding */
				    break;
			    fcuts_r (dreg);
			    break;
			  case 0x6c: /* FDSUB */
			    fsub_rr (dreg, sreg);
			    break;
		    default:
			    FAIL (1);
			    return;
      }
		  fmov_rr (FP_RESULT, dreg);
		  return;
		default:
		  write_log (_T ("Unsupported JIT-FPU instruction: 0x%04x %04x\n"), opcode, extra);
		  FAIL (1);
		  return;
  }
}
#endif
