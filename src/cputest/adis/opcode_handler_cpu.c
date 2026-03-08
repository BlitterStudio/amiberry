/*
 * Change history
 * $Log:	opcode_handler_cpu.c,v $
 *
 * Revision 3.1  09/10/15  Matt Hey
 * Fixed mulsl.l, mulul.l, bf width of 32
 *
 * Revision 3.0  93/09/24  17:54:12  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.2  93/07/18  22:56:17  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.1  93/07/08  20:48:55  Martin_Apel
 * Minor mod.: Disabled internal error message in non-debugging version
 * 
 * Revision 2.0  93/07/01  11:54:21  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.14  93/07/01  11:41:49  Martin_Apel
 * Minor mod.: Now checks for zero upper byte on using CCR
 * 
 * Revision 1.13  93/06/16  20:28:56  Martin_Apel
 * Minor mod.: Removed #ifdef FPU_VERSION and #ifdef MACHINE68020
 * Minor mod.: Added variables for 68030 / 040 support
 * 
 * Revision 1.12  93/06/06  13:47:17  Martin_Apel
 * Minor mod.: Replaced first_pass and read_symbols by pass1, pass2, pass3
 * 
 * Revision 1.11  93/06/03  20:28:42  Martin_Apel
 * Minor mod.: Additional linefeed generation for end instructions has been
 *             moved to format_line
 * 
 * Revision 1.10  93/05/27  20:47:51  Martin_Apel
 * Bug fix: Register list was misinterpreted for MOVEM / FMOVEM
 *          instructions.
 * 
 */

#include "defs.h"

/* static char rcsid [] = "$Id: opcode_handler_cpu.c,v 3.1 09/10/15 Martin_Apel Exp $"; */

/**********************************************************************/

uint EA_to_Rn (struct opcode_entry *op)

/* 2 op instruction where src is an EA and dest is a register (op->param)  */
{
if (pass3)
  str_cpy (dest, reg_names [op->param]);
return (decode_ea (src, MODE_NUM (lw(code)), REG_NUM (lw(code)), op->access, 1) + 1);
}

/**********************************************************************/

uint Rn_to_EA (struct opcode_entry *op)

/* 2 op instruction where src is a register (op->param) and dest is an EA  */
{
if (pass3)
  str_cpy (src, reg_names [op->param]);
return (decode_ea (dest, MODE_NUM (lw(code)), REG_NUM (lw(code)), op->access, 1) + 1);
}

/**********************************************************************/

uint op1 (struct opcode_entry *op)

/* handler for 1 op instructions */
{
return (decode_ea (src, MODE_NUM(lw(code)), REG_NUM(lw(code)), op->access, 1) + 1);
}

/**********************************************************************/

uint op1_end (struct opcode_entry *op)

/* handler for a single operand instruction ending an instruction sequence
   i.e. jmp rtm */
{
instr_end = TRUE;
return (decode_ea (src, MODE_NUM(lw(code)), REG_NUM(lw(code)), op->access, 1) + 1);
}

/**********************************************************************/

uint quick (struct opcode_entry *op)

/* handler for addq and subq */
{
if (pass3)
  {
  char *to;

  to = src;
  *to++ = '#';
  *to++ = (UBYTE)(op->param) + '0';
  *to = 0;
  }
return (decode_ea (dest, MODE_NUM (lw(code)), REG_NUM (lw(code)), op->access, 1) + 1);
}

/**********************************************************************/

uint moveq (struct opcode_entry *op)

{
if (pass3)
  {
  char *to;
  to = src;
  *to++ = '#';
  format_x (to, (BYTE)(lw(code)));
  str_cpy (dest, reg_names [op->param]);
  }
return (1);
}

/**********************************************************************/

uint move (struct opcode_entry *op)

{
uint used;

used = decode_ea (src, MODE_NUM (lw(code)), REG_NUM (lw(code)), op->access, 1);
used += decode_ea (dest,((lw(code) >> 6) & 0x7), ((lw(code) >> 9) & 0x7),
                     op->access, used + 1);
return (used + 1);
}

/**********************************************************************/

uint movem (struct opcode_entry *op)

{
char *reg_list;
uint used;

if (op->param == REG_TO_MEM)
  {
  /* Transfer registers to memory */
  reg_list = src;
  used = decode_ea (dest, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_DATA, 2);
  }
else
  {
  reg_list = dest;
  used = decode_ea (src, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_DATA, 2);
  }

if (pass3)
  {
  format_reg_list (reg_list, lw(code + 1), (MODE_NUM (lw(code)) != 4), 0);
  if (*reg_list == 0)
    {
    /* some compilers generate empty reg lists for movem instructions */
    *reg_list++ = '#';
    *reg_list++ = '0';
    *reg_list = 0;
    }
  }
return (used + 2);
}

/**********************************************************************/

uint imm_b (struct opcode_entry *op)

/* handler for ori.b andi.b eori.b subi.b addi.b cmpi.b callm */
{
ULONG instr = llu(code);

#if 0
if ((instr & 0xfc0000ff) == 0)  /* if ori.b #0,ea or andi.b #0,ea */
  {
  /* ori.b #0,ea and andi.b #0,ea are probably not code */
  instr_bad = TRUE;
  return (1);
  }
instr &= 0x0000ff00;
if (!((instr == 0) || (instr == 0x0000ff00)))  /* GCC immediate may be $ffxx */
  {
  /* probably not code if upper byte of immediate has trash */
  instr_bad = TRUE;
  return (1);
  }
#endif
if (pass3)
  {
  char *to;
  to = src;
  *to++ = '#';
  format_ux (to, (lw(code + 1) & 0xff));
  }
return (decode_ea (dest, MODE_NUM(lw(code)), REG_NUM(lw(code)), op->access, 2) + 2);
}

/**********************************************************************/

uint imm_w (struct opcode_entry *op)

/* handler for ori.w andi.w eori.w subi.w addi.w cmpi.w */
{
#if 0
if (llu(code) & 0xfc00ffff) == 0)
  {
  /* ori.w #0,ea and andi.w #0,ea are probably not code */
  instr_bad = TRUE;
  return (1);
  }
#endif
if (pass3)
  {
  char *to;
  to = src;
  *to++ = '#';
  format_ux (to, lw(code + 1));
  }
return (decode_ea (dest, MODE_NUM(lw(code)), REG_NUM(lw(code)), op->access, 2) + 2);
}

/**********************************************************************/

uint imm_l (struct opcode_entry *op)

/* handler for ori.l andi.l eori.l subi.l addi.l cmpi.l */
{
ULONG instr_ext = llu(code + 1);

#if 0
if ((instr_ext == 0) && (lw(code) & 0xfc00) == 0)
  {
  /* ori.l #0,ea and andi.l #0,ea are probably not code */
  instr_bad = TRUE;
  return (1);
  }
#endif
if (pass2 && FLAGS_RELOC (current_ref + 2))
  {
  enter_ref (instr_ext, NULL, ACC_UNKNOWN);

  /*  Needed for jump tables?
  if (EVEN (instr_ext) && instr_ext >= first_ref && instr_ext < last_ref)
    last_code_reference = instr_ext;
  */
  }
else if (pass3)
  {
  char *to;

  to = src;
  *to++ = '#';  
  if (FLAGS_RELOC (current_ref + 2))
    {
    /* This reference is relocated and thus needs a label */
    *to++ = '(';
    to = gen_label_ref (to, instr_ext);
    *to++ = ')';
    *to = 0;
    }
  else
    format_ulx (to, instr_ext);
  }
return (decode_ea (dest, MODE_NUM(lw(code)), REG_NUM(lw(code)), op->access, 3) + 3);
}

/**********************************************************************/

uint bit_reg (struct opcode_entry *op)

{
UWORD code_w = lw(code);

if (pass3)
  str_cpy (src, reg_names [op->param]);

if (MODE_NUM (code_w) == 0)  /* data register */
  {
  if (pass3)
    str_cpy (dest, reg_names [REG_NUM (code_w)]);
  return (1);
  }
else
  return (decode_ea (dest, MODE_NUM (code_w), REG_NUM (code_w), ACC_BYTE, 1) + 1);
}

/**********************************************************************/

uint bit_imm (struct opcode_entry *op)

{
UWORD code_w = lw(code);
UWORD data_w = lw(code + 1);

#if 0
if ((data_w & 0xff00) != 0)
  {
  /* only bit #0-255 are valid */
  instr_bad = TRUE;
  return (1);
  }
#endif
if (pass3)
  {
  char *to;
  to = src;
  *to++ = '#';
  format_ux (to, data_w);
  }

if (MODE_NUM (code_w) == 0)  /* data register */
  {
  if (pass3)
    str_cpy (dest, reg_names [REG_NUM (code_w)]);
  return (2);
  }
else
  return (decode_ea (dest, MODE_NUM (code_w), REG_NUM(code_w), ACC_BYTE, 2) + 2);
}

/**********************************************************************/

uint srccr (struct opcode_entry *op)

/* ANDI, ORI, EORI to CCR or SR */
{
UWORD size = (lw(code) & 0x00C0) >> 6;  /* size: byte=0 word=1 long=2 */
/* Only allow word size SR or byte size CCR instructions. */
if (size == 0 && (lw(code + 1) & 0xff00))
  return (TRANSFER);
if (pass3)
  {
  str_cpy (opcode, opcode_table [lw(code) >> 6].mnemonic);
  immed (src, (ULONG)(lw(code + 1)));
  str_cpy (dest, special_regs [!size]);
  }
return (2);
}

/**********************************************************************/

uint special (struct opcode_entry *op)

{
/* move usp,     10dxxx
   trap,         00xxxx
   unlk,         011xxx
   link.w        010xxx

   reset,        110000
   nop,          110001
   stop,         110010
   rte,          110011
   rtd,          110100
   rts,          110101
   trapv         110110
   rtr,          110111

   movec,        11101d
*/

char *ptr = NULL;
uint used = 1;

if ((lw(code) & 0x38) == 0x30)
  {
  switch (lw(code) & 0xf)
    {
    case 0: ptr = "reset";
            break;
    case 1: ptr = "nop";
            break;
    case 2: ptr = "stop";
            if (pass3)
              immed (src, lw(code + 1));
            used = 2;
            break;
    case 3: ptr = "rte";
            instr_end = TRUE;
            break;
    case 4: if (!cpu68010)
              return (TRANSFER);
            ptr = "rtd";
            instr_end = TRUE;
            if (pass3)
              {
              src [0] = '#';
              format_ux (src + 1, lw(code + 1));
              }
            used = 2;
            break;
    case 5: ptr = "rts";
            instr_end = TRUE;
            break;
    case 6: ptr = "trapv";
            break;
    case 7: ptr = "rtr";
            instr_end = TRUE;
            break;
    }
  if (pass3)
    str_cpy (opcode, ptr);
  return (used);
  }
else if ((lw(code) & 0x3e) == 0x3a)          /* MOVEC */
  {
  short reg_offset;

  if (!cpu68010)
    return (TRANSFER);
  switch (lw(code + 1) & 0xfff)
    {
    case 0x000: ptr = special_regs [SFC];
                break;
    case 0x001: ptr = special_regs [DFC];
                break;
    case 0x002: ptr = special_regs [CACR];
                break;
    case 0x003: ptr = special_regs [TC];
                break;
    case 0x004: ptr = special_regs [ITT0];
                break;
    case 0x005: ptr = special_regs [ITT1];
                break;
    case 0x006: ptr = special_regs [DTT0];
                break;
    case 0x007: ptr = special_regs [DTT1];
                break;
    case 0x008: ptr = special_regs [BUSCR];
                break;
    case 0x800: ptr = special_regs [USP];
                break;
    case 0x801: ptr = special_regs [VBR];
                break;
    case 0x802: ptr = special_regs [CAAR];
                break;
    case 0x803: ptr = special_regs [MSP];
                break;
    case 0x804: ptr = special_regs [ISP];
                break;
    case 0x805: ptr = special_regs [MMUSR];
                break;
    case 0x806: ptr = special_regs [URP];
                break;
    case 0x807: ptr = special_regs [SRP];
                break;
    case 0x808: ptr = special_regs [PCR];
                break;
    default : return (TRANSFER);
    }

  reg_offset = (lw(code + 1) & 0x8000) ? 8 : 0;
  if (pass3)
    {
    str_cpy (opcode, "movec");
    if (lw(code) & 0x1)
      {
      /* from general register to control register */
      str_cpy (dest, ptr);
      str_cpy (src, reg_names [((lw(code + 1) >> 12) & 0x7) + reg_offset]);
      }
    else
      {
      str_cpy (src, ptr);
      str_cpy (dest, reg_names [((lw(code + 1) >> 12) & 0x7) + reg_offset]);
      }
    }
  return (2);
  }
else if ((lw(code) & 0x30) == 0)
  {
  /* TRAP */
  if (pass3)
    {
    str_cpy (opcode, "trap");
    src [0] = '#';
    format_ux (src + 1, lw(code) & 0xf);
    }
  return (1);
  }
else if ((lw(code) & 0x38) == 0x10)
  {
  /* LINK */
  if (pass3)
    {
    str_cpy (opcode, "link");
    str_cpy (src, reg_names [(lw(code) & 0x7) + 8]);
    dest [0] = '#';
    format_x (dest + 1, (WORD)lw(code + 1));
    }
  return (2);
  }
else if ((lw(code) & 0x38) == 0x18)
  {
  /* UNLK */
  if (pass3)
    {
    str_cpy (opcode, "unlk");
    str_cpy (src, reg_names [(lw(code) & 0x7) + 8]);
    }
  return (1);
  }
else if ((lw(code) & 0x38) == 0x20)
  {
  if (pass3)
    {
    str_cpy (opcode, "move.l");
    str_cpy (dest, special_regs [USP]);
    str_cpy (src, reg_names [(lw(code) & 0x7) + 8]);
    }
  return (1);
  }
else if ((lw(code) & 0x38) == 0x28)
  {
  if (pass3)
    {
    str_cpy (opcode, "move");
    str_cpy (src, special_regs [USP]);
    str_cpy (dest, reg_names [(lw(code) & 0x7) + 8]);
    }
  return (1);
  }
return (TRANSFER);
}

/**********************************************************************/

uint illegal (struct opcode_entry *op)

/* The official illegal instruction */
{
return (1);
}

/**********************************************************************/

uint invalid (struct opcode_entry *op)

/* unimplemented instruction */
{
src [0] = 0;
dest [0] = 0;
instr_bad = TRUE;
return (1);
}

/**********************************************************************/

uint invalid2 (struct opcode_sub_entry *op)

/* unimplemented instruction in the mmu & fpu sub tables */
{
src [0] = 0;
dest [0] = 0;
instr_bad = TRUE;
return (1);
}

/**********************************************************************/

uint branch (struct opcode_entry *op)

{
uint used;
ULONG ref;
LONG offset = (BYTE)(lw(code) & 0xff);

if (offset == 0)
  {
  /* word displacement */
  offset = (WORD)lw(code + 1);
  used = 2;
  }
else if (offset == -1 && cpu68020)
  {
  /* long displacement */
  if (!cpu68020)
    return (TRANSFER);
  offset = lls(code + 1);
  used = 3;
  }
else
  {
  /* byte displacement */
  used = 1;
  }

ref = offset + current_ref + 2;
//if (ODD (offset) || ref < first_ref || ref >= last_ref)
//  return (TRANSFER);

if (pass3)
  {
  gen_label_ref (src, ref);
  if (branch_sizes)
    {
    if (used == 1)
      str_cat (opcode, ".b");
    else if (used == 2)
      str_cat (opcode, ".w");
    else
      str_cat (opcode, ".l");
    }
  }
else
  {
  enter_ref (ref, NULL, ACC_CODE);
  if ((lw(code) & 0xff00) == 0x6000)  /* bra */
    instr_end = TRUE;  /* NOT on pass3 as it looks better without blank line */
  }
return (used);
}

/**********************************************************************/

uint dbranch (struct opcode_entry *op)

{
ULONG ref;

//if (lw(code + 1) & 0x1)  /* branch to an odd address */
//  return (TRANSFER);

if (pass3)
  str_cpy (src, reg_names [lw(code) & 7]);
ref = current_ref + 2 + (short)(lw(code + 1));
if (ref < first_ref || ref > last_ref)
  return (TRANSFER);
if (pass3)
  gen_label_ref (dest, ref);
else
  enter_ref (ref, NULL, ACC_CODE);

return (2);
}

/**********************************************************************/

uint shiftreg (struct opcode_entry *op)

{
if (pass3)
  {
  opcode [1] = 's';
  opcode [2] = 0;
  switch ((lw(code) >> 3) & 0x3)
    {
    case 0:  /* Arithmetic shift */
             opcode [0] = 'a';
             break;
    case 1:  /* Logical shift */
             opcode [0] = 'l';
             break;
    case 2:  /* Rotate with extend */
             str_cpy (opcode, "rox");
             break;
    case 3:  /* Rotate */
             opcode [0] = 'r';
             opcode [1] = 'o';
             break;
    }
  str_cat (opcode, op->mnemonic);

  if (lw(code) & 0x20)
    {
    /* shift count is in register */
    str_cpy (src, reg_names [op->param & 0x7]);
    /* This is because param gives a shift count of 8, if this bitfield is 0 */
    }
  else
    {
    /* shift count specified as immediate */
    src [0] = '#';
    format_ux (src + 1, (UWORD)op->param);
    }

  str_cpy (dest, reg_names [REG_NUM (lw(code))]);
  }
return (1);
}

/**********************************************************************/

uint op2_bcdx (struct opcode_entry *op)

/* For opcodes such as ADDX, SUBX, ABCD, SBCD, PACK, UNPK with only 
   -(Ax) and Dn addressing allowed */
{
char *tmp;

if (pass3)
  {
  if (lw(code) & 0x8)
    {
    /* -(Ax) addressing */
    pre_dec (src, (ushort)(REG_NUM (lw(code)) + 8));
    pre_dec (dest, (ushort)(((lw(code) >> 9) & 0x7) + 8));
    }
  else
    {
    /* Dn addressing */
    str_cpy (src, reg_names [REG_NUM (lw(code))]);
    str_cpy (dest, reg_names [((lw(code) >> 9) & 0x7)]);
    }
  }

if (op->param == NO_ADJ)
  return (1);
else
  {
  if (pass3)
    {
    tmp = dest + strlen (dest);
    *tmp++ = ',';
    immed (tmp, lw(code + 1));
    }
  return (2);
  }
}

/**********************************************************************/

uint muldivl (struct opcode_entry *op)

{
UWORD instr1w = lw(code);
UWORD instr2w = lw(code + 1);
UWORD access = ACC_LONG;

if ((instr2w & 0x800) != 0)
  access |= ACC_SIGNED;

if (pass3)
  {
  char *tmp_o;
  char *tmp_d;
  UWORD dr, dq;

  tmp_o = opcode + 3;

  if (access & ACC_SIGNED)
    *tmp_o++ = 's';
  else
    *tmp_o++ = 'u';

  dr = instr2w & 0x7;
  dq = (instr2w >> 12) & 0x7;

  tmp_d = dest;

  if (instr2w & 0x400)
    {
    /* 64-bit operation, size = 1 */
    tmp_d = str_cpy (tmp_d, reg_names [dr]);
    *tmp_d++ = ':';
    }
  else if ((instr1w & 0x40) && (dr != dq))
    {
    /* divl */
    *tmp_o++ = 'l';
    tmp_d = str_cpy (tmp_d, reg_names [dr]);
    *tmp_d++ = ':';
    }
  str_cpy (tmp_d, reg_names [dq]);
  *tmp_o++ = '.';
  *tmp_o++ = 'l';
  *tmp_o = 0;
  }
return (decode_ea (src, MODE_NUM (instr1w), REG_NUM (instr1w), access, 2) + 2);
}

/**********************************************************************/
    
uint bitfield (struct opcode_entry *op)

{
uint used;
UWORD offset, width;
char *ptr_ea = NULL, *ptr_dn = NULL;

offset = (lw(code + 1) >> 6) & 0x1f;
width = lw(code + 1) & 0x1f;

switch (op->param)
  {
  case SINGLEOP: ptr_ea = src;
                 break;
  case DATADEST: ptr_ea = src;
                 ptr_dn = dest;
                 break;
  case DATASRC : ptr_ea = dest;
                 ptr_dn = src;
                 break;
  }

used = decode_ea (ptr_ea, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_DATA, 2);

if (pass3)
  {
  ptr_ea = str_cat (ptr_ea, "{");
  if (op->param != SINGLEOP)
    str_cpy (ptr_dn, reg_names [(lw(code + 1) >> 12) & 0x7]);
  }

if (lw(code + 1) & 0x800)
  {
  /* Offset specified in register */
    offset &= 7;
//  if (offset > 7)
//    return (TRANSFER);
  if (pass3)
    ptr_ea = str_cpy (ptr_ea, reg_names [offset]);
  }
else if (pass3)
  {
  /* Offset specified as immediate */
  ptr_ea = format_ux (ptr_ea, offset);
  }

if (pass3)
  *ptr_ea++ = ':';

if (lw(code + 1) & 0x20)
  {
  /* Width specified in register */
  width &= 7;
//  if (width > 7)
//    return (TRANSFER);
  if (pass3)
    ptr_ea = str_cpy (ptr_ea, reg_names [width]);
  }
else if (pass3)
  {
  /* Width specified as immediate */
  if (!width)
    width = 0x20;
  ptr_ea = format_ux (ptr_ea, width);
  }

if (pass3)
  {
  *ptr_ea++ = '}';
  *ptr_ea = 0;
  }

return (2 + used);
}

/**********************************************************************/

uint scc (struct opcode_entry *op)

{
if (pass3)
  str_cat (opcode, conditions [(lw(code) >> 8) & 0xf]);
return (decode_ea (src, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_BYTE, 1) + 1);
}

/**********************************************************************/

uint exg (struct opcode_entry *op)

{
UWORD rx = (lw(code) >> REG2_SHIFT) & 0x7;

if (((lw(code) >> 3) & 0x1f) == 0x8)
  {
  /* exchange two data registers */
  if (pass3)
    {
    str_cpy (src, reg_names [rx]);
    str_cpy (dest, reg_names [lw(code) & 0x7]);
    }
  }
else if (((lw(code) >> 3) & 0x1f) == 0x9)
  {
  /* exchange two address registers */
  if (pass3)
    {
    str_cpy (src, reg_names [rx + 8]);
    str_cpy (dest, reg_names [(lw(code) & 0x7) + 8]);
    }
  }
else if (((lw(code) >> 3) & 0x1f) == 0x11)
  {
  /* exchange an address and a data register */
  if (pass3)
    {
    str_cpy (src, reg_names [rx]);
    str_cpy (dest, reg_names [(lw(code) & 0x7) + 8]);
    }
  }
else
  return (TRANSFER);
return (1);
}

/**********************************************************************/

uint trapcc (struct opcode_entry *op)

{
if (pass3)
  str_cat (opcode, conditions [(lw(code) >> 8) & 0xf]);
if ((lw(code) & 0x7) == 2)
  {
  if (pass3)
    immed (src, lw(code + 1));
  return (2);
  }
if ((lw(code) & 0x7) == 3)
  {
  if (pass3)
    immed (src, llu(code + 1));
  return (3);
  }
return (1);
}

/**********************************************************************/

uint chkcmp2 (struct opcode_entry *op)

{
if (pass3)
  {
  if (lw(code + 1) & 0x800)
    {
    /* CHK2 */
    opcode [1] = 'h';
    opcode [2] = 'k';
    }
  else
    {
    /* CMP2 */
    opcode [1] = 'm';
    opcode [2] = 'p';
    }
  if (pass3)
    str_cpy (dest, reg_names [lw(code + 1) >> 12]);
  }
return (decode_ea (src, MODE_NUM (lw(code)), REG_NUM (lw(code)), op->access, 2) + 2);
}

/**********************************************************************/

uint cas (struct opcode_entry *op)

{
char *tmp_s;

if (pass3)
  {
  tmp_s = str_cpy (src, reg_names [lw(code + 1) & 0x7]);
  *tmp_s++ = ',';
  str_cpy (tmp_s, reg_names [(lw(code + 1) >> 6) & 0x7]);
  }
return (decode_ea (dest, MODE_NUM (lw(code)), REG_NUM (lw(code)), op->access, 2) + 2);
}

/**********************************************************************/

uint cas2 (struct opcode_entry *op)

{
if (pass3)
  {
  sprintf (src, "%s:%s,%s:%s", reg_names [lw(code + 1) & 0x7],
                               reg_names [lw(code + 2) & 0x7],
                               reg_names [(lw(code + 1) >> 6) & 0x7],
                               reg_names [(lw(code + 2) >> 6) & 0x7]);

  sprintf (dest, "(%s):(%s)",  reg_names [lw(code + 1) >> 12],
                               reg_names [lw(code + 2) >> 12]);
  }
return (3);
}

/**********************************************************************/

uint moves (struct opcode_entry *op)

{
char *reg, *ea;

if (lw(code + 1) & 0x800)
  {
  reg = src;
  ea = dest;
  }
else
  {
  reg = dest;
  ea = src;
  }

if (pass3)
  str_cpy (reg, reg_names [(lw(code + 1) >> 12) & 0xf]);
return (decode_ea (ea, MODE_NUM (lw(code)), REG_NUM (lw(code)), op->access, 2) + 2);
}

/**********************************************************************/

uint movesrccr (struct opcode_entry *op)

{
char *ea = NULL;

if (pass3)
  {
  switch (op->param)
    {
    case FROM_CCR: str_cpy (src, special_regs [CCR]);
                   ea = dest;
                   break;
    case TO_CCR  : str_cpy (dest, special_regs [CCR]);
                   ea = src;
                   break;
    case FROM_SR : str_cpy (src, special_regs [SR]);
                   ea = dest;
                   break;
    case TO_SR   : str_cpy (dest, special_regs [SR]);
                   ea = src;
                   break;
    }
  }
return (decode_ea (ea, MODE_NUM (lw(code)), REG_NUM (lw(code)), ACC_WORD, 1) + 1);
}

/**********************************************************************/

uint cmpm (struct opcode_entry *op)

{
if (pass3)
  {
  post_inc (src, (lw(code) & 0xf));
  post_inc (dest, ((lw(code) >> 9) & 0xf));
  }
return (1);
}

/**********************************************************************/

uint movep (struct opcode_entry *op)

{
char *dn, *an;

if (pass3)
  {
  if (lw(code) & 0x40)
    str_cat (opcode, ".l");
  else
    str_cat (opcode, ".w");
  }
if (lw(code) & 0x80)
  {
  /* Transfer to memory */
  an = dest;
  dn = src;
  }
else
  {
  /* Transfer from memory */
  an = src;
  dn = dest;
  }

if (pass3)
  {
  str_cpy (dn, reg_names [(lw(code) >> 9) & 0x7]);
  disp_an (an, ((lw(code) & 0x7) + 8), lw(code + 1));
  }
return (2);
}

/**********************************************************************/

uint bkpt (struct opcode_entry *op)

{
if (pass3)
  {
  src [0] = '#';
  format_ux (src + 1, (UWORD)(lw(code) & 0x7));
  }
return (1);
}

/**********************************************************************/

uint link_l (struct opcode_entry *op)

{
if (pass3)
  {
  dest [0] = '#';
  format_ulx (dest + 1, llu(code + 1));
  str_cpy (src, reg_names [REG_NUM (lw(code)) + 8]);
  }
return (3);
}

/**********************************************************************/

uint move16 (struct opcode_entry *op)

{
char *tmp;

if ((lw(code) & 0x20) && ((lw(code + 1) & 0x8fff) == 0x8000))
  {  /* post increment mode for src and dest */
  if (pass3)
    {
    post_inc (src, (lw(code) & 0x7) + 8);
    post_inc (dest, ((lw(code + 1) >> 12) & 0x7) + 8);
    }
  return (2);
  }
else if ((lw(code) & 0x20) == 0)
  {
  if (pass3)
    {
    if (lw(code) & 0x8)    
      {
      tmp = dest;
      decode_ea (src, 7, 1, ACC_LONG, 1);
      }
    else
      {
      tmp = src;
      decode_ea (dest, 7, 1, ACC_LONG, 1);
      }
    if (lw(code) & 0x10)
      indirect (tmp, (lw(code) & 0x7) + 8);
    else
      post_inc (tmp, (lw(code) & 0x7) + 8);
    }
  return (3);
  }
return (TRANSFER);
}

/**********************************************************************/

uint cache (struct opcode_entry *op)

{
char *tmp_o;

if (pass3)
  {
  if (lw(code) & 0x20)
    tmp_o = str_cpy (opcode, "cpush");
  else
    tmp_o = str_cpy (opcode, "cinv");
  switch ((lw(code) >> 3) & 0x3)
    {
    case 1: *tmp_o++ = 'l'; break;
    case 2: *tmp_o++ = 'p'; break;
    case 3: *tmp_o++ = 'a'; break;
    }
  *tmp_o = 0;

  switch (op->param)          /* which cache to modify */
    {
    case 0: str_cpy (src, "nc"); break;
    case 1: str_cpy (src, "dc"); break;
    case 2: str_cpy (src, "ic"); break;
    case 3: str_cpy (src, "bc"); break;
    }

  if (((lw(code) >> 3) & 0x3) != 0x3)      /* not all --> page or line */
    indirect (dest, (REG_NUM (lw(code)) + 8));
  }
return (1);
}
