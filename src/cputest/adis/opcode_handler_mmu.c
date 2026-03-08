/*
 * Change history
 * $Log:	opcode_handler_mmu.c,v $
 *
 * Revision 3.1  09/10/15 Matt_Hey
 * New feature: Added 68060 plpa60 MMU function
 *
 * Revision 3.0  93/09/24  17:54:17  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.4  93/07/18  22:56:27  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.3  93/07/11  21:39:02  Martin_Apel
 * Bug fix: Changed PTEST function for 68030
 * 
 * Revision 2.2  93/07/08  22:29:17  Martin_Apel
 * 
 * Bug fix: Fixed PFLUSH bug. Mode and mask bits were confused
 * 
 * Revision 2.1  93/07/08  20:49:34  Martin_Apel
 * Bug fixes: Fixed various bugs regarding 68030 opcodes
 * 
 * Revision 2.0  93/07/01  11:54:31  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.3  93/07/01  11:42:59  Martin_Apel
 * 
 * Revision 1.2  93/06/19  12:11:49  Martin_Apel
 * Major mod.: Added full 68030/040 support
 * 
 * Revision 1.1  93/06/16  20:29:09  Martin_Apel
 * Initial revision
 * 
 */

#include "defs.h"

/* static char rcsid [] = "$Id: opcode_handler_mmu.c,v 3.0 93/09/24 17:54:17 Martin_Apel Exp $"; */

/**********************************************************************/

uint plpa60 (struct opcode_entry *op)

{
indirect (src, (REG_NUM (lw(code)) + 8));
return (1);
}

/**********************************************************************/

uint pflush40 (struct opcode_entry *op)

{
if (pass3)
  {
  switch ((lw(code) >> 3) & 0x3)
    {
    case 0: str_cat (opcode, "n");  break;
    case 1:                        break;
    case 2: str_cat (opcode, "an"); break;
    case 3: str_cat (opcode, "a");  break;
    }
  if ((lw(code) & 0x10) == 0)
    indirect (src, (REG_NUM (lw(code)) + 8));
  }
return (1);
}

/**********************************************************************/

uint ptest40 (struct opcode_entry *op)

{
if (pass3)
  {
  if (lw(code) & 0x20)
    str_cat (opcode, "r");
  else
    str_cat (opcode, "w");
  indirect (src, (REG_NUM (lw(code)) + 8));
  }
return (1);
}

/**********************************************************************/

PRIVATE BOOL eval_fc (char *to)

{
if ((lw(code + 1) & 0x0018) == 0x0010)
  immed (to, (ULONG)(lw(code + 1) & 0x7));
else if ((lw(code + 1) & 0x0018) == 0x0008)
  str_cpy (to, reg_names [lw(code + 1) & 0x7]);
else if ((lw(code + 1) & 0x001f) == 0)
  str_cpy (to, special_regs [SFC]);
else if ((lw(code + 1) & 0x001f) == 0x0001)
  str_cpy (to, special_regs [DFC]);
else return (FALSE);
return (TRUE);
}

/**********************************************************************/

uint mmu30 (struct opcode_entry *op)

{
/* Test for PTEST instruction */
if (((lw(code + 1) & 0xfc00) == 0x9c00) && ((lw(code) & 0x003f) != 0))
  return ptest30 (op);
else if (!(lw(code + 1) & 0x8000))
  {
  struct opcode_sub_entry *subop;
  subop = &(mmu_opcode_table [lw(code + 1) >> 10]);
  if (pass3)
    str_cpy (opcode, subop->mnemonic);
  return ((*subop->handler) (subop));
  }
return (TRANSFER);
}

/**********************************************************************/

uint ptest30 (struct opcode_entry *op)

{
if ((lw(code) & 0x003f) == 0)         /* Check for illegal mode */
  return (TRANSFER);

str_cpy (opcode, "ptestwfc");
if (lw(code + 1) & 0x0200)
  opcode [5] = 'R';
if (!eval_fc (dest))     
  return (TRANSFER);
if (lw(code + 1) & 0x0100)
  {
  str_cat (dest, ",");
  str_cat (dest, reg_names [(lw(code + 1) >> 5) & 0x7]);
  }
return (2 + decode_ea (dest, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_UNKNOWN, 2));
}

/**********************************************************************
 mmu_opcode_table (opcodes_mmu.c) instructions using an opcode_sub_entry
**********************************************************************/

uint pfl_or_ld (struct opcode_sub_entry *op)

/* Tests for PLOAD instruction first. Otherwise it's a standard
   PFLUSH instruction */
{
if ((lw(code + 1) & 0x00e0) != 0x0000)
  return (pflush30 (op));

if ((lw(code) & 0x3f) == 0)
  return (TRANSFER);

str_cpy (opcode, "pload");
if (lw(code + 1) & 0x0200)
  str_cat (opcode, "r");
else
  str_cat (opcode, "w");

if (!eval_fc (src))
  return (TRANSFER);
return (2 + decode_ea (dest, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_UNKNOWN, 2));
}

/**********************************************************************/

uint pflush30 (struct opcode_sub_entry *op)

{
switch (op->param)
  {
  case 1: /* PFLUSHA */
          if (lw(code + 1) != 0x2400)
            return (TRANSFER);
          str_cat (opcode, "a");
          return (2);
  case 4: /* PFLUSH FC,MASK */
          /* EA ignored !?! */
          if (!eval_fc (src))
            return (TRANSFER);
          immed (dest, (ULONG)((lw(code + 1) >> 5) & 0x7));
          return (2);

  case 6: /* PFLUSH FC,MASK,EA */
          if (!eval_fc (src))
            return (TRANSFER);
          str_cat (src, ",");
          immed (src + strlen (src), (ULONG)((lw(code + 1) >> 5) & 0x7));
          return (2 + decode_ea (dest, MODE_NUM (lw(code)), REG_NUM (lw(code)),
                                 ACC_UNKNOWN, 2));
  }
return (TRANSFER);
}

/**********************************************************************/

uint pmove30 (struct opcode_sub_entry *op)

{
char *ea,
     *reg;

if ((lw(code) & 0x003f) == 0)
  return (TRANSFER);

if ((lw(code + 1) & 0xff) || ((lw(code + 1) & 0x0010) && op->param == MMUSR))
  return (TRANSFER);

if (lw(code + 1) & 0x0200)
  {
  ea = dest;
  reg = src;
  }
else
  {
  ea = src;
  reg = dest;
  }
if ((lw(code + 1) & 0x0100))
  str_cat (opcode, "fd");

str_cpy (reg, special_regs [op->param]);
return (2 + decode_ea (ea, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_LONG, 2));
}
