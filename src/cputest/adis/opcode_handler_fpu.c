/*
 * Change history:
 * $Log:	opcode_handler_fpu.c,v $
 * Revision 3.0  93/09/24  17:54:15  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.1  93/07/18  22:56:24  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.0  93/07/01  11:54:27  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.7  93/06/16  20:29:03  Martin_Apel
 * Minor mod.: Removed #ifdef FPU_VERSION and #ifdef MACHINE68020
 * Minor mod.: Added variables for 68030 / 040 support
 * 
 * Revision 1.6  93/06/06  13:47:37  Martin_Apel
 * Minor mod.: Replaced first_pass and read_symbols by pass1, pass2, pass3
 * 
 * Revision 1.5  93/05/27  20:48:41  Martin_Apel
 * Bug fix: Register list was misinterpreted for MOVEM / FMOVEM
 *          instructions.
 * 
 */

#include "defs.h"

/* static char rcsid [] = "$Id: opcode_handler_fpu.c,v 3.0 93/09/24 17:54:15 Martin_Apel Exp $"; */

/**********************************************************************/

uint fpu (struct opcode_entry *op)

{
uint used;
char *reg_list,
     *ea;
char *tmp = NULL;
uint num_regs;
UWORD instr_word1;
UWORD instr_word2;
UWORD mode;

instr_word1 = *code;
instr_word2 = *(code + 1);
mode = MODE_NUM (instr_word1);
if ((instr_word2 & 0xfc00) == 0x5c00)
  {
  /* fmovecr */
  if (pass3)
    {
    str_cpy (opcode, "fmovecr.x");
    immed (src, (ULONG)(instr_word2 & 0x7f));
    str_cpy (dest, reg_names [17 + ((instr_word2 >> 7) & 0x7)]);
    }
  return (2);
  }
else
  {
  switch (instr_word2 >> 13)
    {
    case 0:
    case 2: /* Use the fpu_opcode_table */
            if ((instr_word2 & 0x0040) && !cpu68040)
              return (TRANSFER);
            struct opcode_sub_entry *subop;
            subop = &(fpu_opcode_table [instr_word2 & 0x7f]);
            return ((*subop->handler) (subop));

    case 3: /* fmove from FPx */
            if (pass3)
              {
              tmp = str_cpy (opcode, "fmove");
              str_cpy (tmp, xfer_size [(instr_word2 >> 10) & 0x7]);
              str_cpy (src, reg_names [17 + ((instr_word2 >> 7) & 0x7)]);
              }
            used = decode_ea (dest, mode, REG_NUM (instr_word1), 
                              sizes [(instr_word2 >> 10) & 0x7], 2);
            if ((instr_word2 & 0x1c00) == 0x0c00)
              {
              /* Packed decimal real with static k-factor */
              if (pass3)
                {
                tmp = dest + strlen (dest);
                *tmp++ = '{';
                tmp = immed (tmp, (ULONG)(instr_word2 & 0x7f));
                *tmp++ = '}';
                *tmp = 0;
                  /* Same as:
                     sprintf (dest + strlen (dest), "{#$%x}", 
                              instr_word2 & 0x7f);
                  */
                }
              }
            else if ((instr_word2 & 0x1c00) == 0x1c00)
              {
              /* Packed decimal real with dynamic k-factor */
              if (pass3)
                {
                tmp = dest + strlen (dest);
                *tmp++ = '{';
                tmp = str_cpy (tmp, reg_names [(instr_word2 >> 4) & 0x7]);
                *tmp++ = '}';
                *tmp = 0;
                /* Same as:
                   sprintf (dest + strlen (dest), "{%s}", 
                            reg_names [(instr_word2 >> 4) & 0x7]);
                */
                }
              }
            return (2 + used);

    case 4: 
    case 5: /* fmove.l and fmovem.l control registers */
            num_regs = 0;
            if (pass3)
              tmp = str_cpy (opcode, "fmove");
            if (instr_word2 & 0x2000)
              {
              ea = dest;
              reg_list = src;
              }
            else
              {
              ea = src;
              reg_list = dest;
              }
            used = decode_ea (ea, mode, REG_NUM (instr_word1), ACC_LONG, 2);
            if (pass3)
              {
              if (instr_word2 & 0x1000)
                {
                reg_list = str_cpy (reg_list, "fpcr");
                num_regs++;
                }
              if (instr_word2 & 0x0800)
                {
                if (num_regs != 0)
                  *reg_list++ = '/';
                reg_list = str_cpy (reg_list, "fpsr");
                num_regs++;
                }
              if (instr_word2 & 0x0400)
                {
                if (num_regs != 0)
                  *reg_list++ = '/';
                str_cpy (reg_list, "fpiar");
                num_regs++;
                }
              if (num_regs > 1)
                *tmp++ = 'm';  /* for fmovem.l */
              *tmp++ = '.';
              *tmp++ = 'l';
              *tmp = 0;
              }
            return (2 + used);

    case 6:
    case 7: /* fmovem.x */
            if (mode < 2 || (instr_word1 & 0x3f) == 0x3c)  /* dn, an || immediate */
              return (TRANSFER);
            if (instr_word2 & 0x2000)
              {
              /* registers to memory */
              if (mode == 3 || (instr_word1 & 0x3f) >= 0x3a)  /* (an)+ || pc modes */
                return (TRANSFER);
              reg_list = src;
              ea = dest;
              }
            else
              {
              /* memory to registers */
              if (mode == 4)  /* -(an) */
                return (TRANSFER);
              reg_list = dest;
              ea = src;
              }

            if (pass3)
              {
              str_cpy (opcode, "fmovem.x");
              if (instr_word2 & 0x0800)
                {
                /* Dynamic register list in data register */
                str_cpy (reg_list, reg_names [(instr_word2 >> 4) & 0x7]);
                }
              else
                {
                /* Test for predecrement mode */
                if (mode == 4)
                  format_reg_list (reg_list, (instr_word2 & 0xff),
                                   TRUE, 17);
                else
                  format_reg_list (reg_list, (instr_word2 << 8),
                                   FALSE, 17);
                }
              }
            return (decode_ea (ea, mode, REG_NUM (instr_word1), ACC_EXTEND, 2) + 2);
    }
  }
return (TRANSFER);
}

/**********************************************************************/

uint fbranch (struct opcode_entry *op)

{
LONG offset;
ULONG ref;
uint used;

if (pass3)
  str_cat (opcode, fpu_conditions [*code & 0x1f]);
if (!op->param)
  {
  offset = (WORD)*(code + 1);
  used = 2;
  }
else
  {
  offset = *(LONG*)(code + 1);
  used = 3;
  }

if (offset == 0 && pass3)
  str_cpy (opcode, "fnop");
else if (offset != 0)
  {
  ref = current_ref + 2 + offset;
  if (ref < first_ref || ref > last_ref)
    return (TRANSFER);
  if (pass3)
    {
    gen_label_ref (src, ref);
    if (branch_sizes)
      {
      if (used == 2)
        str_cat (opcode, ".w");
      else
        str_cat (opcode, ".l");
      }
    }
  else
    enter_ref (ref, NULL, ACC_CODE);
  }
return (used);
}

/**********************************************************************/

uint fdbranch (struct opcode_entry *op)

{
ULONG ref;

if ((*(code + 1) & 0xffe0) != 0)
  return (TRANSFER);
if ((*code & 0x0038) != 0x8)
  return (TRANSFER);
if (pass3)
  {
  str_cat (opcode, fpu_conditions [*(code + 1)]);
  str_cpy (src, reg_names [*code & 0x7]);
  }
ref = current_ref + 4 + (WORD)*(code + 2);
if (ref < first_ref || ref > last_ref)
  return (TRANSFER);
if (pass3)
  gen_label_ref (dest, ref);
else
  enter_ref (ref, NULL, ACC_CODE);
return (3);
}

/**********************************************************************/

uint fscc (struct opcode_entry *op)

{
if ((*(code + 1) & 0xffe0) != 0)
  return (TRANSFER);

if (pass3)
  str_cat (opcode, fpu_conditions [*(code + 1)]);
return (decode_ea (src, MODE_NUM (*code), REG_NUM (*code), ACC_BYTE, 2) + 2);
}

/**********************************************************************/

uint ftrapcc (struct opcode_entry *op)

{
if ((*(code + 1) & 0xffe0) != 0)
  return (TRANSFER);
if (pass3)
  str_cat (opcode, fpu_conditions [*(code + 1)]);
if ((*code & 0x7) == 2)
  {
  if (pass3)
    immed (src, (ULONG)*(code + 2));
  return (3);
  }
if ((*code & 0x7) == 3)
  {
  if (pass3)
    immed (src, *(ULONG*)(code + 2));
  return (4);
  }

return (2);
}

/**********************************************************************
 fpu_opcode_table (opcodes_fpu.c) instructions using an opcode_sub_entry
**********************************************************************/

uint std_fpu (struct opcode_sub_entry *op)

{
char *tmp_op = NULL;

if (pass3)
  {
  tmp_op = str_cpy (opcode, op->mnemonic);
  str_cpy (dest, reg_names [17 + ((*(code + 1) >> 7) & 0x7)]);
  }
if (*(code + 1) & 0x4000)          /* R/M - Bit */
  {
  if (pass3)
    str_cpy (tmp_op, xfer_size [(*(code + 1) >> 10) & 0x7]);
  return (decode_ea (src, MODE_NUM (*code), REG_NUM (*code), 
                     sizes [(*(code + 1) >> 10) & 0x7], 2) + 2);
  }
else if (pass3)
  {
  str_cpy (tmp_op, xfer_size [2]);       /* ".X" */
  str_cpy (src, reg_names [17 + ((*(code + 1) >> 10) & 0x7)]);
  }

if ((src [2] == dest [2]) && (op->param == SNG_ALL))      /* single operand */
  dest [0] = 0;
return (2);
}

/**********************************************************************/

uint ftst (struct opcode_sub_entry *op)

/* ftst is unusual because it only allows 1 opcode */
{
char *tmp_op = NULL;

if (pass3)
  tmp_op = str_cpy (opcode, op->mnemonic);
if (*(code + 1) & 0x4000)          /* R/M - Bit */
  {
  if (pass3)
    str_cpy (tmp_op, xfer_size [(*(code + 1) >> 10) & 0x7]);
  return (decode_ea (src, MODE_NUM (*code), REG_NUM (*code), 
                     sizes [(*(code + 1) >> 10) & 0x7], 2) + 2);
  }
else if (pass3)
  {
  str_cpy (tmp_op, xfer_size [2]);       /* ".X" */
  str_cpy (src, reg_names [17 + ((*(code + 1) >> 10) & 0x7)]);
  }
return (2);
}

/**********************************************************************/

uint fsincos (struct opcode_sub_entry *op)

{
char *tmp_op = NULL;
char *tmp_dest;

if (pass3)
  {
  tmp_op = str_cpy (opcode, op->mnemonic);
  tmp_dest = str_cpy (dest, reg_names [17 + op->param]);
  *tmp_dest++ = ':';
  str_cpy (tmp_dest, reg_names [17 + ((*(code + 1) >> 7) & 0x7)]);
  }
if (*(code + 1) & 0x4000)          /* R/M-Bit */
  {
  if (pass3)
    str_cpy (tmp_op, xfer_size [(*(code + 1) >> 10) & 0x7]);
  return (decode_ea (src, MODE_NUM (*code), REG_NUM (*code), 
                     sizes [(*(code + 1) >> 10) & 0x7], 2) + 2);
  }
else
  {
  if (pass3)
    {
    str_cpy (tmp_op, xfer_size [2]);       /* ".X" */
    str_cpy (src, reg_names [17 + ((*(code + 1) >> 10) & 0x7)]);
    }
  return (2);
  }
}
