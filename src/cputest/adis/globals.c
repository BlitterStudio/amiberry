/*
 * Change history
 * $Log:	globals.c,v $
 *
 * Revision 3.1  09/10/19 Matt_Hey
 * New feature: Added cpu68060 support
 *
 * Revision 3.0  93/09/24  17:54:00  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.1  93/07/18  22:55:57  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.0  93/07/01  11:54:00  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.15  93/07/01  11:39:52  Martin_Apel
 * Minor mod.: Prepared for tabs instead of spaces.
 * Minor mod.: Added 68030 opcodes to disable list
 * 
 * Revision 1.14  93/06/19  12:08:38  Martin_Apel
 * Major mod.: Added full 68030/040 support
 * 
 * Revision 1.13  93/06/16  20:28:04  Martin_Apel
 * Minor mod.: Removed #ifdef FPU_VERSION and #ifdef MACHINE68020
 * Minor mod.: Added variables for 68030 / 040 support
 * 
 * Revision 1.12  93/06/06  13:46:34  Martin_Apel
 * Minor mod.: Replaced first_pass and read_symbols by pass1, pass2, pass3
 * 
 * Revision 1.11  93/06/06  00:11:41  Martin_Apel
 * Major mod.: Added support for library/device disassembly (option -dl)
 * 
 * Revision 1.10  93/06/04  11:50:40  Martin_Apel
 * New feature: Added -ln option for generation of ascending label numbers
 * 
 * Revision 1.9  93/06/03  20:27:16  Martin_Apel
 * New feature: Added -a switch to generate comments for file offsets
 * 
 * Revision 1.8  93/06/03  18:31:10  Martin_Apel
 * Minor mod.: Added hunk_end array
 * Minor mod.: Remove temporary files upon exit (even with CTRL-C)
 * 
 */

#include "defs.h"

/* static char rcsid [] = "$Id: globals.c,v 3.0 93/09/24 17:54:00 Martin_Apel Exp $"; */

UBYTE *hunk_address;
UBYTE *flags;
UWORD *code;
ULONG current_ref;
ULONG first_ref;
ULONG last_ref;
ULONG total_size;
ULONG current_hunk;
ULONG *hunk_start = NULL,
      *hunk_end = NULL,
      *hunk_offset = NULL,
      *hunk_memtype = NULL;

ULONG f_offset;
FILE *in = NULL,
     *out = NULL,
     *tmp_f = NULL;
char *in_buf = NULL,
     *out_buf = NULL;
char *input_filename = NULL;
char *output_filename = NULL;
char *fd_filename = NULL;

/* Variables for options */

int OPCODE_COL = 3;               /* def column instructions */
int PARAM_COL = 12;               /* def column paramaters */
int COMMENT_COL = 40;             /* def column comments */
int MAX_STR_COL = 77;             /* max column for dc.b string */

int buf_size = DEF_BUF_SIZE;       /* -f */

/* Try to resolve addressing used by many compilers in small mode. */
/* (Addressing relative to A4) */
long a4_offset = -1;
BOOL try_small = FALSE;             /* -b */

BOOL single_file = TRUE;                                   
BOOL print_illegal_instr_offset = FALSE;         /* -i */
BOOL old_style = FALSE;
BOOL cpu68010 = TRUE,
     cpu68020 = TRUE,              /* -m2 */
     cpu68030 = TRUE,              /* -m3 */
     cpu68040 = TRUE,              /* -m4 */
     cpu68060 = TRUE,              /* -m6 */
     fpu68881 = TRUE;              /* -m8 */
BOOL disasm_as_lib = FALSE;         /* -l */
BOOL ascending_label_numbers = FALSE;   /* -dn */
BOOL verbose = FALSE;
BOOL optimize = FALSE;
BOOL branch_sizes = TRUE;
BOOL instr_bad;
BOOL instr_end;
char pass;
char space_char = ' ';  /* space or tab for column seperator */
char comment = 0;                   /* -a */

ULONG InitTableRef = 0;
ULONG FuncTableRef = 0;
ULONG DataTableRef = 0;
ULONG ROMTagRef = 0;
BOOL  ROMTagFound = FALSE;
UBYTE ROMTagType;

char spaces [44];  /* storage for leading spaces before instructions */
char opcode [20];  /* storage for instruction name (mnemonic) */
char src [200];    /* storage for source op */
char dest [200];   /* storage for destination op */

char *conditions [] =
  {
  "t",
  "f",
  "hi",
  "ls",
  "cc",
  "cs",
  "ne",
  "eq",
  "vc",
  "vs",
  "pl",
  "mi",
  "ge",
  "lt",
  "gt",
  "le"
  };

char *reg_names [] =
  {
  "d0",
  "d1",
  "d2",
  "d3",
  "d4",
  "d5",
  "d6",
  "d7",
  "a0",
  "a1",
  "a2",
  "a3",
  "a4",
  "a5",
  "a6",
  "sp",
  "pc",       /* Start at offset 17 */
  "fp0",
  "fp1",
  "fp2",
  "fp3",
  "fp4",
  "fp5",
  "fp6",
  "fp7"
  };

char *special_regs [] =
  {
  "sr",        /* 0 */
  "ccr",       /* 1 */
  "usp",       /* 2 */
  "sfc",       /* 3 */
  "dfc",       /* 4 */
  "cacr",      /* 5 */
  "vbr",       /* 6 */
  "caar",      /* 7 */
  "msp",       /* 8 */
  "isp",       /* 9 */
  "tc",        /* 10 */
  "itt0",      /* 11 */
  "itt1",      /* 12 */
  "dtt0",      /* 13 */
  "dtt1",      /* 14 */
  "mmusr",     /* 15 */
  "urp",       /* 16 */
  "srp",       /* 17 */
  "tt0",       /* 18 */
  "tt1",       /* 19 */
  "crp"        /* 20 */
  "buscr"      /* 21 */
  "pcr"        /* 22 */
  };

/**********************************************************************/
/*      When the program is compiled for use on a 68020 system,       */
/*        68020 instructions disassembly can be switched off          */
/*                     via a command line switch.                     */
/*      Then those following instructions are marked as illegal       */
/**********************************************************************/

short cpu68020_disabled [] =
  {
    3, /* CHK2, CMP2 */
   11, /* CHK2, CMP2 */
   19, /* CHK2, CMP2 */
   27, /* CALLM */
   43, /* CAS */
   51, /* CAS */
   56, /* MOVES */
   57, /* MOVES */
   58, /* MOVES */
   59, /* CAS */
  260, /* CHK.L */
  268, /* CHK.L */
  276, /* CHK.L */
  284, /* CHK.L */
  292, /* CHK.L */
  300, /* CHK.L */
  304, /* MUL .L */
  305, /* DIV .L */
  308, /* CHK.L */
  316, /* CHK.L */
  931, /* BFTST */
  935, /* BFEXTU */
  939, /* BFCHG */
  943, /* BFEXTS */
  947, /* BFCLR */
  951, /* BFFFO */
  955, /* BFSET */
  959, /* BFINS */
 1027, /* RTM */
 1028, /* UNPK */
 1032, /* PACK */
 1040, /* CAS2 */
 1041, /* CAS2 */
 1042, /* CAS2 */
 1045, /* TRAPcc */
 1049, /* BKPT */
 1051, /* EXTB.L */
    0
  };

short fpu68881_disabled [] =
  {
  968, /* General fpu opcode */
  969, /* FScc */
  970, /* FBcc */
  971, /* FBcc */
  972, /* FSAVE */
  973, /* FRESTORE */
 1047, /* FDBcc */
 1048, /* FTRAPcc */
    0
  };

short cpu68030_disabled [] =
  {
  960, /* MMU instructions */
  0
  };

short cpu68040_disabled [] =
  {
  976, /* CPUSH, CINV */
  977, /* CPUSH, CINV */
  978, /* CPUSH, CINV */
  979, /* CPUSH, CINV */
  980, /* PFLUSH */
  981, /* PTEST */
  984, /* MOVE16 */
    0
  };

short cpu68060_disabled [] =
  {
  982, /* PLPAW */
  983, /* PLPAR */
    0
  };
