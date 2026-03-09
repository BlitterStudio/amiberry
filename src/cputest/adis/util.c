/*
 * Change history
 * $Log:	util.c,v $
 *
 * Revision 3.1  09/10/15  Matt_Hey
 * New: col_put(), add_str(), str_len(). Many other changes. 
 * Addressing modes changed to new style, lower case
 *
 * Revision 3.0  93/09/24  17:54:29  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.3  93/07/18  22:56:58  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.2  93/07/10  13:02:35  Martin_Apel
 * Major mod.: Added full jump table support. Seems to work quite well
 * 
 * Revision 2.1  93/07/08  22:29:31  Martin_Apel
 * 
 * Minor mod.: Displacements below 4 used with pc indirect indexed are
 *             not entered into the symbol table anymore
 * 
 * Revision 2.0  93/07/01  11:54:56  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.17  93/07/01  11:45:00  Martin_Apel
 * Minor mod.: Removed $-sign from labels
 * Minor mod.: Prepared for tabs instead of spaces
 * 
 * Revision 1.16  93/06/16  20:31:31  Martin_Apel
 * Minor mod.: Removed #ifdef FPU_VERSION and #ifdef MACHINE68020
 * 
 * Revision 1.15  93/06/04  11:56:35  Martin_Apel
 * New feature: Added -ln option for generation of ascending label numbers
 * 
 * Revision 1.14  93/06/03  20:30:17  Martin_Apel
 * Minor mod.: Additional linefeed generation for end instructions has been
 *             moved to format_line
 * New feature: Added -a switch to generate comments for file offsets
 * 
 * Revision 1.13  93/05/27  20:50:48  Martin_Apel
 * Bug fix: Register list was misinterpreted for MOVEM / FMOVEM
 *          instructions.
 * 
 */

#include "defs.h"
#include "string_recog.h"
#include <ctype.h>

/* static char rcsid [] =  "$Id: util.c,v 3.1 09/10/15 Martin_Apel Exp $"; */

/**********************************************************************/
/*      A few routines doing string handling such as formatting       */
/*        a hex value, formatting the register list for MOVEM         */
/*              instructions, different address modes...              */
/*        sprintf would have been much easier but sloooow !!!         */
/**********************************************************************/

const char hex_chars [] = "0123456789abcdef";  /* for format() functions */

/**************************************************************************/

char *str_cpy (char *dest, const char *src)

/* str_cpy() returns new dest (end of str), strcpy() returns old dest */
{
while (*dest++ = *src++);
return (dest - 1);
}

/**************************************************************************/

char *str_cat (char *dest, const char *src)

/* str_cat() returns new dest (end of str), strcat() returns old dest */
{
while (*dest)
  dest++;
while(*dest++ = *src++);
return (dest - 1);
}

/**************************************************************************/

BOOL is_str (char *maybe_string, uint max_len)

{
uchar *tmp_str;
uchar *last_char;

tmp_str = (uchar*) maybe_string;
last_char = tmp_str + max_len;
while (IS_VALID (*tmp_str) && (tmp_str < last_char))
  tmp_str++;
if ((*tmp_str != 0) || ((char*)tmp_str == maybe_string))  /* single 0 char is not a string */
  return (FALSE);
return (TRUE);
}

/**************************************************************************/

int str_len (char *maybe_string, uint max_len)

/* str_len() returns string length or 0 if not a valid string */
{
uchar *tmp_str;
uchar *last_char;

tmp_str = (uchar*) maybe_string;
last_char = tmp_str + max_len;
while (IS_VALID (*tmp_str) && (tmp_str < last_char))
  tmp_str++;

if (*tmp_str == 0)
  return ((char*)tmp_str - maybe_string);
return (0);
}

/**************************************************************************/

void emit (char *string)

/* fputs a string */
{
#ifdef DEBUG
if (out == NULL)
  {
  fprintf (stderr, "ERROR: Attempt to write to closed file in emit()\n");
  ExitADis ();
  }
#endif

if (fputs (string, out) == EOF)
  {
  fprintf (stderr, "ERROR: Writing output file\n");
  ExitADis ();
  }
}

/**************************************************************************/

void col_emit (char *string)

/* fputs leading OPCODE_COL blanks followed by string */
{

#ifdef DEBUG
if (out == NULL)
  {
  fprintf (stderr, "ERROR: Attempt to write to closed file in col_emit()\n");
  ExitADis ();
  }
#endif

if (fputs (spaces, out) == EOF)
  {
  fprintf (stderr, "ERROR: Writing output file\n");
  ExitADis ();
  }
if (fputs (string, out) == EOF)
  {
  fprintf (stderr, "ERROR: Writing output file\n");
  ExitADis ();
  }
}

/**************************************************************************/

char *format_line (char *to, BOOL commented, int instr_words)

/* format opcode, src and dest to to */
{
char *tmp_str;
int i;
char reg_space_char = space_char;

tmp_str = to;
i = OPCODE_COL;
do
  {
  *to++ = reg_space_char;
  i--;
  }
while (i > 0);
to = str_cpy (to, opcode);

if (!(src [0] == 0 && dest [0] == 0))
  {
  i = PARAM_COL - (to - tmp_str);
  do
    {
      *to++ = reg_space_char;
      i--;
    }
  while (i > 0);
  if (dest [0] == 0)
    to = str_cpy (to, src);
  else if (src [0] == 0)
    to = str_cpy (to, dest);
  else
    {
    to = str_cpy (to, src);
    *to++ = ',';
    to = str_cpy (to, dest);
    }
  }

if (commented && comment)
  {
  ULONG offset;

  i = COMMENT_COL - (to - tmp_str);
  do
    {
    *to++ = reg_space_char;
    i--;
    }
  while (i > 0);
  *to++ = ';';

  if ((comment & 0x8) == 8)   /* btst #3 */
    {
    ULONG flags_l;

    *to++ = ' ';
    flags_l = *(flags + (current_ref - first_ref));
    if (flags_l & FLAG_BYTE)
      *to++ = 'b';
    else
      *to++ = '-';
    if (flags_l & FLAG_WORD)
      *to++ = 'w';
    else
      *to++ = '-';
    if (flags_l & FLAG_LONG)
      *to++ = 'l';
    else
      *to++ = '-';
    if (flags_l & FLAG_SIGNED)
      *to++ = 's';
    else
      *to++ = '-';
    if (flags_l & FLAG_RELOC)
      *to++ = 'R';
    else
      *to++ = '-';
    if (flags_l & FLAG_STRING)
      *to++ = 'S';
    else
      *to++ = '-';
    if (flags_l & FLAG_TABLE)
      *to++ = 'T';
    else
      *to++ = '-';
    if (flags_l & FLAG_CODE)
      *to++ = 'C';
    else
      *to++ = '-';
    }
  if (!(comment & 0x06))  /* comment=0 or comment=1 */
    {
    /* relocation offset */
    offset = current_ref;
    }
  else if ((comment & 0x2) == 2)  /* btst #1 */
    {
    /* hunk offset */
    offset = current_ref - first_ref;
    }
  else
    {
    /* file offset */
    offset = current_ref - first_ref + *(hunk_offset + current_hunk);
    }
  *to++ = ' ';
  to = format_ulx_no_dollar (to, offset);
  if (EVEN (comment))
    {
    if (instr_words)
      {
      *to++ = ' ';
      *to++ = ':';
      for (i = 0; i < instr_words; i++)
        {
        *to++ = ' ';
        to = format_ulx_digits (to, lw(code + i), 4L);
        }
      }
    }
  }
#if 0
*to++ = 0x0a;  /* lf=0x0a */
if (instr_end)  /* extra LF after routines looks nicer */
  *to++ = 0x0a;  /* lf=0x0a */
#endif
*to = 0;
return (to);
}

/**************************************************************************/

char *format_ulx (char *to, ULONG val)

/* Converts a ULONG (or UWORD) to a hex string. Returns ptr to EOS. */
{

if (val <= 9)
  {
  *to++ = (char)(val + '0');
  *to = 0;
  return (to);
  }
else
  {
  char stack [8],
       *sp;

  sp = stack;
  do
    {
    *sp++ = val & 0xf;       /* MOD 16 */
    val = val >> 4;          /* DIV 16 */
    }
  while (val != 0);
  *to++ = '$';
  do
    *to++ = hex_chars [*(--sp)];
  while (sp != stack);
  *to = 0;
  return (to);
  }
}

/**************************************************************************/

char *format_lx (char *to, ULONG val)

/* Converts a long to a hex string. Returns ptr to EOS. */
{

if ((LONG)val < 0)
  {
  val = 0-val;
  *to++ = '-';
  }

if (val <= 9)
  {
  *to++ = (char)(val + '0');
  *to = 0;
  return (to);
  }
else
  {
  char stack [8],
       *sp;

  sp = stack;
  do
    {
    *sp++ = val & 0xf;       /* MOD 16 */
    val = val >> 4;          /* DIV 16 */
    }
  while (val != 0);
  *to++ = '$';
  do
    *to++ = hex_chars [*(--sp)];
  while (sp != stack);
  *to = 0;
  return (to);
  }
}

/**************************************************************************/

char *format_ulx_no_dollar (char *to, ULONG val)

/* Converts a ULONG (or UWORD) to a hex string. Returns ptr to EOS.
   The same as format_ulx but without the dollar sign. */
{
char stack [8],
     *sp;

sp = stack;
do
  {
  *sp++ = val & 0xf;       /* MOD 16 */
  val = val >> 4;          /* DIV 16 */
  }
while (val != 0);
do
  *to++ = hex_chars [*(--sp)];
while (sp != stack);

*to = 0;
return (to);
}

/**************************************************************************/

/* Currently unused
char *format_lx_no_dollar (char *to, ULONG val)

// Converts a long to a hex string. Returns ptr to EOS.
// The same as format_lx but without the dollar sign
{
char stack [8],
     *sp;

if ((LONG)val < 0)
  {
  val = -val;
  *to++ = '-';
  }

sp = stack;
do
  {
  *sp++ = val & 0xf;       // MOD 16
  val = val >> 4;          // DIV 16
  }
while (val != 0);
do
  *to++ = hex_chars [*(--sp)];
while (sp != stack);

*to = 0;
return (to);
}
*/

/**************************************************************************/

char *format_ulx_digits (char *to, ULONG val, uint digits)

/* Convert val to a hex string of (up to 8) digits. Returns ptr to EOS.
   No dollar sign. Leading zeros included.
     digits = 4 for a UWORD
     digits = 8 for a ULONG
*/
{
char stack [32],
     *sp;

sp = stack;
do
  {
  *sp++ = val & 0xf;       /* MOD 16 */
  val = val >> 4;          /* DIV 16 */
  digits--;
  }
while (digits != 0);
do
  *to++ = hex_chars [*(--sp)];
while (sp != stack);

*to = 0;
return (to);
}

/**************************************************************************/

void format_reg_list (char *to, ushort list, BOOL incr_list, ushort reglist_offset)

{
int i;
long mylist;                 /* list in order a7-a0/d7-d0 */
short last_set;
BOOL regs_used;


if (!incr_list)
  {
  mylist = 0;
  for (i = 0; i < 16; i++)
    {
    if (list & (1 << i))
      mylist |= 1 << (15 - i);
    }
  }
else
  mylist = list;

last_set = -1;
regs_used = FALSE;
for (i = 0; i < 17; i++)
  {
  if ((mylist & (1 << i)) && last_set == -1)
    {
    if (regs_used)
      *to++ = '/';
    to = str_cpy (to, reg_names [i + reglist_offset]);
    last_set = i;
    regs_used = TRUE;
    }
  else if (!(mylist & (1 << i)))
    {
    if (last_set == i - 1)
      last_set = -1;
    else if (last_set != -1)
      {
      *to++ = '-';
      to = str_cpy (to, reg_names [i - 1 + reglist_offset]);
      last_set = -1;
      }
    }
  if (i == 7 && (mylist & 0x180) == 0x180)
    {
    if (last_set != 7) 
      /* d7 and a0 both used and still in a list */
      {
      *to++ = '-';
      to = str_cpy (to, reg_names [7 + reglist_offset]);
      }
    last_set = -1;
    }      
  }
}

/**************************************************************************/

char *immed (char *to, ULONG val)

{
/* e.g. #$17 */

*to++ = '#';
return (format_ulx (to, val));
}

/**************************************************************************/

void indirect (char *to, ushort reg_num)

{
/* e.g. (a0) */

*to++ = '(';
to = str_cpy (to, reg_names [reg_num]);
*to++ = ')';
*to = 0;
}

/**************************************************************************/

void pre_dec (char *to, ushort reg_num)

{
/* e.g. -(a0) */

*to++ = '-';
*to++ = '(';
to = str_cpy (to, reg_names [reg_num]);
*to++ = ')';
*to = 0;
}

/**************************************************************************/

void post_inc (char *to, ushort reg_num)

{
/* e.g. (a0)+ */

*to++ = '(';
to = str_cpy (to, reg_names [reg_num]);
*to++ = ')';
*to++ = '+';
*to = 0;
}

/**************************************************************************/

void disp_an (char *to, ushort reg_num, WORD disp)

{
/* e.g. 4(a0) or (4,a0) */

if (old_style)
  {
  to = format_x (to, disp);
  *to++ = '(';
  }
else
  {
  *to++ = '(';
  to = format_x (to, disp);
  *to++ = ',';
  }
to = str_cpy (to, reg_names [reg_num]);
*to++ = ')';
*to = 0;
}

/**************************************************************************/

void disp_an_indexed (char *to, ushort an, BYTE disp, ushort index_reg, 
                             ushort scale, ushort size)

{
/* e.g. 4(a0,d0.w*4) or (4,a0,d0.w*4) */
/* address register indirect with index (8 bit displacement) */
/* the displacement, address register, and index register are mandantory */

if (!old_style)
  *to++ = '(';

if (!(disp == 0 && optimize))
  to = format_x (to, (WORD)disp);

if (old_style)
  *to++ = '(';
else if (!(disp == 0 && optimize))
  *to++ = ',';

to = str_cpy (to, reg_names [an]);
*to++ = ',';
to = str_cpy (to, reg_names [index_reg]);
*to++ = '.';
if (size == 4)
  *to++ = 'l';
else
  *to++ = 'w';
if (scale != 1)
  {
  *to++ = '*';
  *to++ = scale + '0';
  }
*to++ = ')';
*to = 0;
}

/**************************************************************************/

void disp_pc_indexed (char *to, ULONG ref, BYTE disp, ushort index_reg, 
                             ushort scale, ushort size)

{
/* e.g. 4(pc,d0.w*4) or (4,pc,d0.w*4) */
/* pc indirect with index (8 bit displacement) */
/* the displacement, pc, and index register are mandantory */

if (!old_style)
  *to++ = '(';

if (!((lw(code) & 0xffc0) == 0x4ec0))  /* not jmp */
  to = gen_label_ref (to, ref);
else if (!(disp == 0 && optimize))
  to = format_x (to, (WORD)disp);

if (old_style)
  *to++ = '(';
else if (!(disp == 0 && optimize))
  *to++ = ',';

to = str_cpy (to, reg_names [PC]);
*to++ = ',';
to = str_cpy (to, reg_names [index_reg]);
*to++ = '.';
if (size == 4)
  *to++ = 'l';
else
  *to++ = 'w';
if (scale != 1)
  {
  *to++ = '*';
  *to++ = scale + '0';
  }
*to++ = ')';
*to = 0;
}

/**************************************************************************/

uint full_extension (char *to, UWORD *instr_ext, ushort mode, ushort reg)

{
/* address register indirect with index (base displacement) */
/* the displacement, address register and index register are all optional */

/*  15  14 13 12   11   10 9  8   7   6    5 4    3  2 1 0 */
/* D/A  Register  W/L  Scale  1  BS  IS  BD size  0  I/IS  */

/* D/A is Index register type %0=Dn, %1=An */
/* W/L is Word/Longword index size %0=sign extended word, %1=longword */ 
/* Scale is Scale factor %00=1, %01=2, %10=4, %11=8 */
/* BS is Base Register Suppress %0=add base register, %1=suppress */
/* IS is Index Suppress %0=evaluate & add index operand, %1=suppress */
/* BD size is Base Displacement size %00=reserved, %01=null, %10=word, %11=long */
/* I/IS is Index/Indirect Select
/*
select = (no_index_reg >> 3) | (*(instr_ext) & 0x0007);
switch (select)
  {
  case 0x0: // %0000 No memory indirect action
  case 0x1: // %0001 Indirect Preindexed with null od
  case 0x2: // %0010 Indirect Preindexed with word od
  case 0x3: // %0011 Indirect Preindexed with long od
  case 0x4: // %0100 Reserved
  case 0x5: // %0101 Indirect Postindexed with null od
  case 0x6: // %0110 Indirect Postindexed with word od
  case 0x7: // %0111 Indirect Postindexed with long od

  // index register suppressed
  case 0x8: // %1000 No memory indirect action
  case 0x9: // %1001 Memory indirect with null od
  case 0xa: // %1010 Memory indirect with word od
  case 0xb: // %1011 Memory indirect with long od
  case 0xc: // %1100 Reserved
  case 0xd: // %1101 Reserved
  case 0xe: // %1110 Reserved
  case 0xf: // %1111 Reserved
  }
*/

int bd_size,
    od_size;
UWORD full_ext_w,
      scale,
      post_indexed,
      no_index_reg,
      no_base_reg;
UWORD *next_instr_ext;

full_ext_w = lw(instr_ext);
next_instr_ext = instr_ext + 1;

no_index_reg = (full_ext_w & 0x0040);  /*  %1000000, bit #6 */

/* check for validity of instr word */
if ((full_ext_w & 0x0008) || !(full_ext_w & 0x0030) ||
    (full_ext_w & 0x0007) == 4 || ((full_ext_w & 0x0007) > 4 && no_index_reg))
  {
  instr_bad = TRUE;
  return (0);
  }

no_base_reg = (full_ext_w & 0x0080);   /* %10000000, bit #7 */
post_indexed = (full_ext_w & 0x0004);  /*      %100, bit #2 */
bd_size = ((full_ext_w >> 4) & 0x0003) - 1; /* -1=invalid 0=NULL, 1=word, 2=longword */
od_size = (full_ext_w & 0x0003) - 1;  /* -1=invalid, 0=NULL, 1=word, 2=longword */

if (!(no_index_reg && no_base_reg && od_size < 0))
  {
  *to++ = '(';
  if (od_size >= 0)
    *to++ = '[';
  }

if (bd_size == 1)  /* .w? */
  {
  if (mode == 7 && !no_base_reg)  /* pc relative && pc not suppressed? */
    {
    ULONG b_offset = current_ref + ((instr_ext - code)  << 1);
    if (pass2)
      enter_ref (b_offset + (WORD)lw(next_instr_ext++), NULL, ACC_UNKNOWN);
    else
      to = gen_label_ref (to, b_offset + (WORD)lw(next_instr_ext++));
    }
  else
    {
    to = format_x (to, lw(next_instr_ext++));
    if (!optimize)
      {
      *to++ = '.';
      *to++ = 'w';
      }
    }
  }
else if (bd_size == 2)  /* .l? */
  {
  ULONG b_offset = current_ref + (((UWORD*)instr_ext - code) << 1);
  if (FLAGS_RELOC (b_offset + 2))  /* relocated absolute (to a label) */
    {
    if (pass2)
      enter_ref (lls(next_instr_ext), NULL, ACC_UNKNOWN);
    else
      to = gen_label_ref (to, llu(next_instr_ext));
    }
  else if (mode == 7 && !no_base_reg)  /* pc relative && pc not suppressed? */
    {
    if (pass2)
      enter_ref (b_offset + lls(next_instr_ext), NULL, ACC_UNKNOWN);
    else
      to = gen_label_ref (to, b_offset + lls(next_instr_ext));
    }
  else
    {
    to = format_lx (to, llu(next_instr_ext));
    if (!optimize)
      {
      *to++ = '.';
      *to++ = 'l';
      }
    }
  next_instr_ext += 2;
  }
else if (bd_size == 0 && no_base_reg && no_index_reg)  /* absolute with NULL bd */
  *to++ = '0';

if (!no_base_reg) /* base register? */
  {
  if (bd_size > 0)
    *to++ = ',';
  if (mode == 7)  /* pc relative? */
    reg = 8;
  to = str_cpy (to, reg_names [reg + 8]);  
  }

if (!no_index_reg)
  {
  if (post_indexed)
    *to++ = ']';
  if (bd_size != 0 || !no_base_reg)
    *to++ = ',';
  to = str_cpy (to, reg_names [full_ext_w >> 12]);
  *to++ = '.';
  if (full_ext_w & 0x0800)
    *to++ = 'l';
  else
    *to++ = 'w';
  scale = 1 << ((full_ext_w & 0x0600) >> 9);
  if (scale != 1)
    {
    *to++ = '*';
    *to++ = scale + '0';
    }
  }

if (!post_indexed && od_size >= 0)
  *to++ = ']';

if (od_size == 1)  /* .w? */
  {
  *to++ = ',';
  to = format_x (to, lw(next_instr_ext++));
  if (!optimize)
    {
    *to++ = '.';
    *to++ = 'w';
    }
  }
else if (od_size == 2)  /* .l? */
  {
  *to++ = ',';
  to = format_lx (to, llu(next_instr_ext));
  if (!optimize)
    {
    *to++ = '.';
    *to++ = 'l';
    }
  next_instr_ext += 2;
  }

if (!(no_index_reg && no_base_reg && od_size < 0))
  *to++ = ')';
*to = 0;

return ((uint)((UWORD*)next_instr_ext - instr_ext));
}

/**************************************************************************/

char *gen_xref (char *to, ULONG address)

{
to = str_cpy (to, "XREF ");
to = gen_label_ref (to, address);
*to++ = 0x0a;  /* lf=$0a */
*to = 0;
return (to);
}

/**************************************************************************/

char *gen_label_ref (char *to, ULONG ref)

/* generates a label reference and returns new dest (end of str) */
{
char *label;
UWORD access;

if (find_ref (ref, &label, &access) && label != 0)
  {
  to = str_cpy (to, label);
  return (to);
  }
to = str_cpy (to, "lab_");
to = format_ux_no_dollar (to, ref);
return (to);
}

/**************************************************************************/

char *gen_label (char *to, ULONG ref)

/* generates a label if needed and returns new dest (end of str) */
{
char *label;
UWORD access;

if (find_ref (ref, &label, &access))
  {
  if (label == NULL)
    {
    to = str_cpy (to, "lab_");
    to = format_ux_no_dollar (to, ref);
    }
  else
    to = str_cpy (to, label);
  *to++ = ':';
  *to++ = 0x0a;  /* lf=$0a */
  }
*to = 0;
return (to);
}

/**************************************************************************/

void assign_label_names (void)

/**********************************************************************/
/*   Assigns each label an ascending number instead of its address    */
/**********************************************************************/
{
uint label_count = 1;
ULONG ref = 0;
UWORD access;
char *old_name;
char label_name [16];

while ((ref = next_ref (ref, total_size, &access)) != total_size)
  {
  find_ref (ref, &old_name, &access);
  if (old_name == NULL)
    {
    format_ux_no_dollar (str_cpy (label_name, "lab_"), label_count++);
    enter_ref (ref, label_name, access);
    }
  }
}


void ExitADis(void)
{
}
void enter_ref(ULONG ref, char *name, UWORD access_type)
{
}
BOOL find_ref(ULONG ref, char **name, UWORD *access_type)
{
	return FALSE;
}
void enter_jmptab(ULONG start, ULONG offset)
{
}
ULONG next_ref(ULONG from, ULONG to, UWORD *access_type)
{
	return 0;
}

uint disasm_instr(UWORD *instr, char *out, int cpu_lvl)

/**********************************************************************/
/*    Returns number of 16 bit words disassembled                     */
/**********************************************************************/

{
	struct opcode_entry *op;
	uint size = 0;

    first_ref = 0;
    last_ref = 0xffffffff;
    current_ref = (ULONG)instr;
    if (cpu_lvl < 5) {
        cpu68060 = 0;
    }
    if (cpu_lvl < 4) {
        cpu68040 = 0;
    }
    if (cpu_lvl < 3) {
        cpu68030 = 0;
    }
    if (cpu_lvl < 2) {
        cpu68020 = 0;
        fpu68881 = 0;
    }
    if (cpu_lvl < 1) {
    		cpu68010 = 0;
		}

	set_pass3;

	opcode[0] = src[0] = dest[0] = 0;
	instr_bad = FALSE; /* global variable TRUE when unknown or corrupt instr is found */
	instr_end = FALSE; /* global variable TRUE at rts, bra, jmp, etc. */
	code = instr;      /* global variable */

	for (op = &(opcode_table[(lw(instr)) >> 6]);
		size == 0;
		op = &(opcode_table[op->chain]))
	{
		/* Test for validity of ea_mode */
		if (op->modes & (1 << MODE_NUM(lw(instr))))
		{
			if ((MODE_NUM(lw(instr)) == 7) && !(op->submodes & (1 << REG_NUM(lw(instr)))))
				continue;

			if (pass3 && op->mnemonic != 0)
			{
				str_cpy(opcode, op->mnemonic);
			}
			size = (*op->handler) (op);
		}
	}
	format_line(out, FALSE, 0);
	return (size);
}
