/*
 * Change history
 * $Log:	decode_ea.c,v $
 * Revision 3.1  09/12/11 Matt Hey
 * Minor mod.: enter_ref() for immediate and PC memory indirect with index
 *             relocations is now ACC_UNKNOWN instead of access
 *
 * Revision 3.0  93/09/24  17:53:50  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.5  93/07/18  22:55:43  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.4  93/07/11  21:38:04  Martin_Apel
 * Major mod.: Jump table support tested and changed
 * 
 * Revision 2.3  93/07/10  13:01:56  Martin_Apel
 * Major mod.: Added full jump table support. Seems to work quite well
 * 
 * Revision 2.2  93/07/08  22:27:46  Martin_Apel
 * 
 * Minor mod.: Displacements below 4 used with pc indirect indexed are
 *             not entered into the symbol table anymore
 * 
 * Revision 2.1  93/07/08  20:47:04  Martin_Apel
 * Bug fix: Extended precision reals were printed wrong
 * 
 * Revision 2.0  93/07/01  11:53:45  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.12  93/07/01  11:38:28  Martin_Apel
 * Minor mod.: PC relative addressing is now marked as such
 * 
 * Revision 1.11  93/06/16  20:26:17  Martin_Apel
 * Minor mod.: Added jump table support. UNTESTED !!!
 * 
 * Revision 1.10  93/06/06  13:45:34  Martin_Apel
 * Minor mod.: Replaced first_pass and read_symbols by pass1, pass2, pass3
 * 
 * Revision 1.9  93/06/03  18:22:12  Martin_Apel
 * Minor mod.: Addressing relative to hunk end changed
 * 
 */

#include "defs.h"

/* static char rcsid [] = "$Id: decode_ea.c,v 3.0 93/09/24 17:53:50 Martin_Apel Exp $"; */

uint decode_ea (char *to, ushort mode, ushort reg, UWORD access, ushort first_ext)

/* returns the number of extension words used for ea */
{
static ULONG maybe_jmptable_ref = UNSET;  /* for jump table disassembly */
ULONG ref;
UWORD first_ext_w;

first_ext_w = lw(code + first_ext);
switch (mode)
  {
  /********************** data register direct ************************/

  case 0:
    if (pass3)
      str_cpy (to, reg_names [reg]);
    return (0);

  /********************* address register direct **********************/

  case 1:
    if (access & ACC_BYTE)
      {
      instr_bad = TRUE;
      return (0);
      }
    if (pass3)
      str_cpy (to, reg_names [reg + 8]);
    return (0);

  /********************** address register indirect *******************/

  case 2:
    if (pass3)
      indirect (to, (ushort)(reg + 8));
    else if ((lw(code) & 0xfff8) == 0x4ed0)  /* jmp (An) */
      {
      if (!FLAGS_RELOC (maybe_jmptable_ref))
        enter_jmptab (maybe_jmptable_ref, 0);  /* jmp table with word entries */
      else
        {
        /* Probably a jmp table of long relocs. The refs/labels pointed to
           by the jmp table entries should already be entered but may not
           be marked as code. */

        while ((*(ULONG*)(flags + maybe_jmptable_ref - first_ref) &
              ((FLAG_RELOC << 24) | (FLAG_RELOC << 16) |
               (FLAG_RELOC << 8) | FLAG_RELOC)) ==
              ((FLAG_RELOC << 24) | (FLAG_RELOC << 16) |
               (FLAG_RELOC << 8) | FLAG_RELOC))
          {
          ref = *(ULONG*)(hunk_address + maybe_jmptable_ref - first_ref);
          if (ODD (ref) || ref < first_ref || ref >= last_ref)
            break;
          enter_ref (ref, NULL, ACC_CODE);  /* mark ref/label as code */
          maybe_jmptable_ref += 4;
          }
        }
      }
    return (0);

  /************ address register indirect post-increment **************/

  case 3:
    if (pass3)
      post_inc (to, (ushort)(reg + 8));
    return (0);

  /************* address register indirect pre-decrement **************/

  case 4:
    if (pass3)
      pre_dec (to, (ushort)(reg + 8));
    return (0);

  /************* address register indirect with displacement **********/

  case 5:
    if (try_small && reg == 4)
      {
      ref = (WORD)first_ext_w + a4_offset;
      if (pass3)
        gen_label_ref (to, ref);
      else
        enter_ref (ref, NULL, access);
      }
    else if (pass3)
      disp_an (to, (ushort)(reg + 8), (ushort)first_ext_w);
    return (1);

  /****************** address register indirect indexed ***************/

  case 6:
    if ((first_ext_w & 0x100) && !cpu68020)
      first_ext_w &= ~0x100;
    if (first_ext_w & 0x100)
      {
      if (cpu68020)
        return (full_extension (to, code + first_ext, mode, reg));
#if 0
      else
        {
        instr_bad = TRUE;
        return (0);
        }
#endif
    }
    else
      {
      UWORD size = (first_ext_w >> 9) & 0x3;
      if (size != 0 && !cpu68020)
          size = 0;
#if 0
      if (!cpu68020 && size != 0)  /* Only size of 1 allowed for 68000 */
        {
        instr_bad = TRUE;
        return (0);
        }
#endif
	  if (pass3)
        {
        /* To get the sign right */
        disp_an_indexed (to, (ushort)(reg + 8),
                         (BYTE)first_ext_w,
                         (ushort)(first_ext_w >> 12),
                         (ushort)(1 << size),
                         (first_ext_w & 0x0800) ? ACC_LONG : ACC_WORD);
        }
      return (1);
      }

  /************************* Mode 7 with submodes *********************/

  case 7:
    switch (reg)
      {
      /*********************** absolute short *********************/

      case 0:
        if (pass3)
          {
          to = format_ulx (to, (WORD)first_ext_w);
          *to++ = '.';
          *to++ = 'w';
          *to = 0;
          }
        return (1);

      /*********************** absolute long **********************/

      case 1:
        ref = llu(code + first_ext);
        if (FLAGS_RELOC (current_ref + first_ext * 2))
          {
          if (pass3)
            /* This reference is relocated and thus needs a label */
            gen_label_ref (to, ref);
          else
            {
            enter_ref (ref, NULL, access);
            /*
            if (EVEN (ref) && ref >= first_ref && ref < last_ref)
              maybe_jmptable_ref = ref;
            */
            }
          }
        else if (pass3)
          {
          to = format_ulx (to, ref);
          if (!optimize)
            {
            *to++ = '.';
            *to++ = 'l';
            *to = 0;
            }
          }
        return (2);

      /************************** d16(PC) *************************/

      case 2:
/* It's possible, that a program accesses its hunk structure PC relative,
   thus it generates a reference to a location outside valid code addresses */

        /* A neg displacement can make ref neg if out of bounds. */
        ref = current_ref + ((ULONG)first_ext << 1) + (WORD)first_ext_w;
        if (pass3)
          {
          if (!old_style)
            *to++ = '(';
          if ((LONG)ref < (LONG)first_ref)
            {
            to = gen_label_ref (to, first_ref);
            *to++ = '-';
            to = format_lx (to, (LONG)first_ref - (LONG)ref);
            }
          else if (ref >= last_ref)
            {
            to = gen_label_ref (to, last_ref - 2);
            *to++ = '+';
            to = format_lx (to, ref - last_ref + 2);
            }
          else
            to = gen_label_ref (to, ref);
          if (old_style)
            *to++ = '(';
          else
            *to++ = ',';
          to = str_cpy (to, reg_names [PC]);
          *to++ =')';
          *to = 0;
          }
        else if ((LONG)ref >= (LONG)first_ref && (LONG)ref < (LONG)last_ref)
          {
          enter_ref (ref, NULL, access);
          if (EVEN (ref))
            maybe_jmptable_ref = ref;
          }
        return (1);

      /***************** PC memory indirect with index ************/

      case 3:
          if ((first_ext_w & 0x100) && !cpu68020)
              first_ext_w &= ~0x100;
          if (first_ext_w & 0x0100)
          {
          /* long extension word */
          if (cpu68020)
            return (full_extension (to, code + first_ext, mode, reg));
#if 0
		  else
            {
            instr_bad = TRUE;
            return (0);
            }
#endif
		}
        else
          {
          UWORD size = (first_ext_w >> 9) & 0x3;
          if (size != 0 && !cpu68020)
              size = 0;
#if 0
          if (!cpu68020 && size != 0)  /* Only size of 1 allowed for 68000 */
            {
            instr_bad = TRUE;
            return (0);
            }
#endif
		  /* A neg displacement can make ref neg if out of bounds. */
          ref = current_ref + ((ULONG)first_ext << 1) + (BYTE)first_ext_w;
          if (pass3)
            disp_pc_indexed (to, ref, (BYTE)first_ext_w,
                            (ushort)(first_ext_w >> 12),
                            (ushort)(1 << size),
                            (first_ext_w & 0x0800) ? ACC_LONG : ACC_WORD);
          else if ((LONG)ref >= (LONG)first_ref && (LONG)ref < (LONG)last_ref)
            {
            if (lw(code) == 0x4efb)  /* Check for JMP (d8,PC,Xn) */
              {
              enter_jmptab (maybe_jmptable_ref, (ULONG)((BYTE)first_ext_w) + 2);
              }
            else
              {
              enter_ref (ref, NULL, access);
              if (EVEN (ref))
                maybe_jmptable_ref = ref;
              }
            }
          return (1);
          }

      /************************ immediate *************************/

      case 4:
        if (access & ACC_BYTE)
          {
          if (pass3)
            {
            *to++ = '#';
            if (access & ACC_SIGNED)
              format_x (to, (BYTE)first_ext_w);
            else
              format_ux (to, (UBYTE)first_ext_w);
            }
          return (1);
          }
        else if (access & ACC_WORD)
          {
          if (pass3)
            {
            *to++ = '#';
            if (access & ACC_SIGNED)
              format_x (to, (WORD)first_ext_w);
            else
              format_ux (to, (UWORD)first_ext_w);
            }
          return (1);
          }
        else if (access & ACC_LONG)
          {
          ref = llu(code + first_ext);
          if (FLAGS_RELOC (current_ref + first_ext * 2))
            {
            if (pass2)
              {
              enter_ref (ref, NULL, ACC_UNKNOWN);  /* data type and size is unknown */
              if (EVEN (ref) && ref >= first_ref && ref < last_ref)
                maybe_jmptable_ref = ref;
              }
            else
              {
              /* This reference is relocated and thus needs a label */
              *to++ = '#';
              *to++ = '(';
              to = gen_label_ref (to, ref);
              *to++ = ')';
              *to++ = 0;
              }
            }
          else if (pass3)
            {
            *to++ = '#';
            if (access & ACC_SIGNED)
              format_lx (to, (LONG)ref);
            else
              format_ulx (to, ref);
            }
          return (2);
          }
        else if (access & ACC_DOUBLE)
          {
          if (pass3)
            {
            sprintf (to, "#$%lx%08lx", llu(code + first_ext),
                     llu(code + first_ext + 2));
            }
          return (4);
          }
        else if (access & ACC_EXTEND)
          {
          if (pass3)
            {
            sprintf (to, "#$%lx%08lx%08lx", llu(code + first_ext),
                     llu(code + first_ext + 2), 
                     llu(code + first_ext + 4));
            }
          return (6);
          }
#ifdef DEBUG
        else
          {
          fprintf (stderr, "WARNING: Unknown immediate addressing size at %lx\n", current_ref);
          }
#endif
        break;
      default:
        /* Should not occur, as it should be caught by the submode test in disasm.c */
        instr_bad = TRUE;
#ifdef DEBUG
        fprintf (stderr, "WARNING: Illegal submode at %lx\n", current_ref);
#endif
      }
    break;    
  }
return (0);
}
