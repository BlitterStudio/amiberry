/*
 * Change history
 * $Log:	defs.h,v $
 * Revision 3.0  93/09/24  17:54:36  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.4  93/07/18  22:57:05  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.3  93/07/11  21:40:10  Martin_Apel
 * Major mod.: Jump table support tested and changed
 * 
 * Revision 2.2  93/07/10  13:02:39  Martin_Apel
 * Major mod.: Added full jump table support. Seems to work quite well
 * 
 * Revision 2.1  93/07/08  20:50:47  Martin_Apel
 * Minor mod.: Added print_flags for debugging purposes
 * 
 * Revision 2.0  93/07/01  11:55:04  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.30  93/07/01  11:46:54  Martin_Apel
 * Minor mod.: Prepared for tabs instead of spaces
 * 
 * Revision 1.29  93/06/19  12:12:20  Martin_Apel
 * Major mod.: Added full 68030/040 support
 * 
 * Revision 1.28  93/06/16  20:31:52  Martin_Apel
 * Minor mod.: Removed #ifdef FPU_VERSION and #ifdef MACHINE68020
 * Minor mod.: Added variables for 68030 / 040 support
 * 
 * Revision 1.27  93/06/06  13:48:45  Martin_Apel
 * Minor mod.: Added preliminary support for jump tables recognition
 * Minor mod.: Replaced first_pass and read_symbols by pass1, pass2, pass3
 * 
 * Revision 1.26  93/06/06  00:16:26  Martin_Apel
 * Major mod.: Added support for library/device disassembly (option -dl)
 * 
 * Revision 1.25  93/06/04  11:57:34  Martin_Apel
 * New feature: Added -ln option for generation of ascending label numbers
 * 
 * Revision 1.24  93/06/03  20:31:01  Martin_Apel
 * New feature: Added -a switch to generate comments for file offsets
 * 
 * Revision 1.23  93/06/03  18:28:15  Martin_Apel
 * Minor mod.: Added new prototypes an variables for hunk-handling routines.
 * Minor mod.: Remove temporary files upon exit (even with CTRL-C).
 * 
 */

#define _CRT_SECURE_NO_WARNINGS

#if 0
#include <exec/types.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef MCH_AMIGA
#ifndef AMIGA
#define AMIGA
#endif
#endif

#ifdef DEBUG
#define PRIVATE
#else
#define PRIVATE static
#endif

typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;

typedef signed char BYTE;
typedef signed short WORD;
typedef signed long LONG;

typedef signed short BOOL;
#define TRUE 1
#define FALSE 0

#if !defined(__GNUC__)
/* Already defined in sys/types.h for GCC */
typedef unsigned int uint;
typedef unsigned short ushort;
#endif
typedef unsigned char uchar;

/* $Id: defs.h,v 3.0 93/09/24 17:54:36 Martin_Apel Exp $ */

struct opcode_entry
  {
  uint (*handler) (struct opcode_entry*);
  UBYTE modes;      /* addressing modes allowed for this opcode */
  UBYTE submodes;   /* addressing submodes of mode 7 allowed for this opcode */
  UWORD chain;      /* if another opcode is possible for this bit pattern, */
                    /* this is the index of another entry in opcode_table. */
                    /* Otherwise it's zero */
  char *mnemonic;
  UWORD param;      /* given to handler () */
  UWORD access;     /* data type of instruction access (ACC_ flags) */
  };

struct opcode_sub_entry
  {
  uint (*handler) (struct opcode_sub_entry*);
  char *mnemonic;
  UWORD param;      /* given to handler () */
  UWORD access;     /* data type of instruction access (ACC_ flags) */
  };

#define TRANSFER 0                /* The handler didn't handle this opcode */
                                  /* Transfer to next in chain */
/* Param values */
/* MOVEM direction */
#define REG_TO_MEM 0
#define MEM_TO_REG 1
/* Bitfield instructions */
#define DATADEST 0
#define DATASRC  1
#define SINGLEOP 2
/* MOVE to or from SR, CCR */
#define TO_CCR   0
#define FROM_CCR 1
#define TO_SR    2
#define FROM_SR  3
/* Extended precision and BCD instructions */
#define NO_ADJ 0
#define ADJUST 1

/* Bit masks */
#define EA_MODE 0x038      /* Bits 5 - 3 */
#define EA_REG  0x007      /* Bits 2 - 0 */
#define REG2    0xe00      /* Bits 11 - 9 */
#define OP_MODE 0x1c0      /* Bits 8 - 6 */

/* Shift values */
#define MODE_SHIFT 3
#define REG2_SHIFT 9
#define OP_MODE_SHIFT 6

#define MODE_NUM(x) ((UWORD)(((x) >> 3) & 0x7))
#define REG_NUM(x)  ((UWORD)((x) & EA_REG))

#define USE_SIGN    TRUE
#define NO_SIGN     FALSE
#define USE_LABEL   TRUE
#define NO_LABEL    FALSE
#define USE_COMMENT TRUE
#define NO_COMMENT  FALSE

#define DEF_BUF_SIZE (16L * 1024L)

enum 
  {
  SR, CCR, USP, SFC, DFC, CACR, VBR, CAAR, MSP, ISP, TC, 
  ITT0, ITT1, DTT0, DTT1, MMUSR, URP, SRP, TT0, TT1, CRP,
  BUSCR, PCR
  };

#define PC (ushort)16               /* index into reg_names array */

/* Access types for reference table. */
#define ACC_BYTE     (UWORD)0x0001  /* referenced as byte */
#define ACC_WORD     (UWORD)0x0002  /* referenced as word */
#define ACC_LONG     (UWORD)0x0004  /* referenced as long or fp single (4 bytes) */
#define ACC_DOUBLE   (UWORD)0x0008  /* referenced as fp double (8 bytes) */
#define ACC_EXTEND   (UWORD)0x0010  /* referenced as fp extended (12 bytes) */
#define ACC_SIGNED   (UWORD)0x0020
#define ACC_DATA     (UWORD)0x0040  /* referenced as data but size unknown */
#define ACC_CODE     (UWORD)0x0080  /* referenced as code */
#define ACC_UNKNOWN  (UWORD)0x0100  /* accessed but unknown if code or data */
/* ... */
#define ACC_INACTIVE (UWORD)0x4000  /* This label is temporarily deactivated */
#define ACC_NEW      (UWORD)0x8000  /* ref is new and easily deleted */

#define ACC_SBYTE    ACC_BYTE|ACC_SIGNED  /* shortcut for signed byte */
#define ACC_SWORD    ACC_WORD|ACC_SIGNED  /* shortcut for signed word */
#define ACC_SLONG    ACC_LONG|ACC_SIGNED  /* shortcut for signed long */

/* Flags (UBYTE) */
#define FLAG_BYTE       0x01  /* data size is byte */
#define FLAG_WORD       0x02  /* data size is word */
#define FLAG_LONG       0x04  /* data size is long */
#define FLAG_SIGNED     0x08  /* data is signed */
#define FLAG_RELOC      0x10  /* data is relocatable */
#define FLAG_STRING     0x20  /* data is a string */
#define FLAG_TABLE      0x40  /* data is a table */
#define FLAG_CODE       0x80  /* data is code */

#if 1
#define FLAGS_BYTE(ref) 0
#define FLAGS_WORD(ref) 0
#define FLAGS_LONG(ref) 0
#define FLAGS_SIGNED(ref) 0
#define FLAGS_RELOC(ref) 0
#define FLAGS_CODE(ref) 0
#define FLAGS_STRING(ref) 0
#define FLAGS_BAD(ref) 0
#define FLAGS_ANY(ref) 0
#define FLAGS_NONE(ref) 0
#else
#define FLAGS_BYTE(ref)    (*(flags + (ref) - first_ref) & FLAG_BYTE)
#define FLAGS_WORD(ref)    (*(flags + (ref) - first_ref) & FLAG_WORD)
#define FLAGS_LONG(ref)    (*(flags + (ref) - first_ref) & FLAG_LONG)
#define FLAGS_SIGNED(ref)  (*(flags + (ref) - first_ref) & FLAG_SIGNED)
#define FLAGS_RELOC(ref)   (*(flags + (ref) - first_ref) & FLAG_RELOC)
#define FLAGS_CODE(ref)    (*(flags + (ref) - first_ref) & FLAG_CODE)
#define FLAGS_STRING(ref)  (*(flags + (ref) - first_ref) & FLAG_STRING)
#define FLAGS_BAD(ref)     (*(flags + (ref) - first_ref) &\
                                        (FLAG_CODE | FLAG_RELOC))
#define FLAGS_ANY(ref)     (*(flags + (ref) - first_ref))
#define FLAGS_NONE(ref)    (!FLAGS_ANY(ref))

#define FLAGS_BYTE_P(ptr)      (*(ptr) & FLAG_BYTE)
#define FLAGS_WORD_P(ptr)      (*(ptr) & FLAG_WORD)
#define FLAGS_LONG_P(ptr)      (*(ptr) & FLAG_LONG)
#define FLAGS_SIGNED_P(ptr)    (*(ptr) & FLAG_SIGNED)
#define FLAGS_RELOC_P(ptr)     (*(ptr) & FLAG_RELOC)
#define FLAGS_CODE_P(ptr)      (*(ptr) & FLAG_CODE)
#define FLAGS_STRING_P(ptr)    (*(ptr) & FLAG_STRING)
#define FLAGS_BAD_P(ptr)       (*(ptr) & (FLAG_CODE | FLAG_RELOC))
#define FLAGS_ANY_P(ptr)       (*(ptr))
#endif

/* Marker for jump-tables */
#ifdef DEBUG
#define UNSET 0xffffffffL
#else
#define UNSET 0L
#endif

#define ODD(x) ((x) & 1)
#define EVEN(x) (!ODD(x))

/* More efficient use of disassembler pass variables than 3 booleans */
#define set_pass1 (pass = 0)
#define set_pass2 (pass = 1)
#define set_pass3 (pass = -1)
#define pass1 (pass == 0)
#define pass2 (pass > 0)
#define pass3 (pass < 0)

extern UBYTE *hunk_address;
extern UBYTE *flags;  /* UBYTE array of flags describing data. See Flags above */
extern UWORD *code;

/* Refs are total offsets of the program starting with lowest numbered hunk.
   hunk_offset = current_ref - first_ref;
   data_address = *hunk_address + current_ref - first_ref;  */

extern ULONG current_ref;  /* like Program Counter (PC) but with refs */
extern ULONG first_ref;    /* start ref of current hunk */
extern ULONG last_ref;     /* last ref of current hunk */
extern ULONG total_size;   /* total program size in bytes */
extern ULONG current_hunk; /* hunk number of current hunk */
/* The following point to a table (array) of hunk attributes indexed by hunk number */
extern ULONG *hunk_start,
             *hunk_end,
             *hunk_offset,    /* hunk f_offset */
             *hunk_memtype;   /* hunk memory type */

extern char *conditions [];
extern char *reg_names []; 
extern char *special_regs [];
extern char opcode [];
extern char src [];
extern char dest [];
extern char spaces [];

extern ULONG f_offset;     /* file offset */
extern FILE *in,
            *out,
            *tmp_f;
extern char *in_buf,
            *out_buf;
extern char *input_filename;
extern char *output_filename;
extern char *fd_filename;

extern int buf_size;
extern long a4_offset;
extern BOOL try_small;
extern BOOL instr_bad;
extern BOOL instr_end;
extern char pass;
extern char space_char;
extern char comment;
extern BOOL optimize;
extern BOOL branch_sizes;
extern BOOL verbose;
extern BOOL print_illegal_instr_offset;
extern BOOL single_file;
extern BOOL ascending_label_numbers;
extern BOOL disasm_as_lib;
extern BOOL old_style;
extern BOOL cpu68010,
            cpu68020,
            cpu68030,
            cpu68040,
            cpu68060,
            cpuCF,
            fpu68881;

extern BOOL ROMTagFound;
extern UBYTE ROMTagType;
extern ULONG ROMTagRef;
extern ULONG InitTableRef;
extern ULONG FuncTableRef;
extern ULONG DataTableRef;

extern short cpu68020_disabled [];
extern short cpu68030_disabled [];
extern short cpu68040_disabled [];
extern short cpu68060_disabled [];
extern short fpu68881_disabled [];

extern int OPCODE_COL;
extern int PARAM_COL;
extern int COMMENT_COL;
extern int MAX_STR_COL;


/* Prototypes */

/* analyze.c */
void analyze_code (UBYTE *seg);

/* decode_ea.c */
uint decode_ea (char *to, ushort mode, ushort reg, UWORD access, ushort first_ext);

/* disasm_code.c */
uint disasm_instr (UWORD *instr, char*, int);
void disasm_code (UBYTE *seg, ULONG seg_size);

/* disasm_data.c */
void disasm_data (UBYTE *seg, ULONG seg_size);
void disasm_bss (void);

/* main.c */
int main (int argc, char *argv []);
void parse_args (int argc, char *argv []);
#ifdef DEBUG
void usage (void);
#endif
void open_files (void);
void close_files (void);
void open_output_file (void);
void close_output_file (void);
void ExitADis (void);
#ifdef DEBUG
void check_consistency (void);
#endif

/* util.c */
char *str_cpy (char *dest, const char *src);
char *str_cat (char *dest, const char *src);
BOOL is_str (char *maybe_string, uint max_len);
int str_len (char *maybe_string, uint max_len);
void emit (char *string);
void col_emit (char *string);

char *format_ulx (char *to, ULONG val);
char *format_lx (char *to, ULONG val);
char *format_ulx_no_dollar (char *to, ULONG val);
char *format_ulx_digits (char *to, ULONG val, uint digits);
/* Processing .l is as fast or faster than .w for 68020+. */
/* format_ulx () can handle UWORD values thus define below. */
#define format_ux(to,val) format_ulx(to,val)
/* format_lx () can handle WORD (and BYTE) values thus define below. */
#define format_x(to,val) format_lx(to,val)
/* format_ulx_no_dollar () can handle UWORD values thus define below. */
#define format_ux_no_dollar(to,val) format_ulx_no_dollar(to,val)

void format_reg_list (char *to, UWORD list, BOOL to_mem, ushort reglist_offset);
char *immed (char *to, ULONG val);
void pre_dec (char *to, ushort reg_num);
void post_inc (char *to, ushort reg_num);
void indirect (char *to, ushort reg_num);
void disp_an (char *to, ushort reg_num, WORD disp);
void disp_an_indexed (char *to, ushort an, BYTE disp, ushort index_reg, 
                             ushort scale, ushort size);
void disp_pc_indexed (char *to, ULONG ref, BYTE disp, ushort index_reg, 
                             ushort scale, ushort size);
uint full_extension (char *to, UWORD *instr, ushort mode, ushort reg);
char *gen_xref (char *to, ULONG address);
char *gen_label_ref (char *to, ULONG ref);
char *gen_label (char *to, ULONG ref);
char *format_line (char *to, BOOL commented, int instr_words);
void assign_label_names (void);

/* hunks.c */
BOOL readfile (void);
void read_hunk_header (void);
void read_hunk_unit (void);
BOOL read_hunk_code (void);
BOOL read_hunk_data (void);
BOOL read_hunk_bss (void);
void process_hunks (UBYTE *seg);
BOOL read_hunk_symbol (void);
BOOL read_hunk_reloc16 (UBYTE *seg);
BOOL read_hunk_reloc32 (UBYTE *seg);

/* refs.c */
BOOL init_ref_table (ULONG size);
void kill_ref_table (void);
void enter_ref (ULONG ref, char *name, UWORD access_type);
ULONG ext_enter_ref (ULONG ref, ULONG hunk, char *name, UWORD access_type);
BOOL find_ref (ULONG ref, char **name, UWORD *access_type);
BOOL find_active_ref (ULONG ref, char **name, UWORD *access_type);
ULONG next_ref (ULONG from, ULONG to, UWORD *access_type);
ULONG next_active_ref (ULONG from, ULONG to, UWORD *access_type);
void deactivate_refs (ULONG from, ULONG to);
void save_new_refs (void);
void delete_new_refs (void);

/* mem.c */
void *alloc_mem (ULONG size);
void free_mem (void *buffer);

/* user_defined.c */
void predefine_label (ULONG address, UWORD access);
void add_predefined_labels (void);

#ifdef DEBUG
void print_ref_table (ULONG from, ULONG to);
void print_active_table (ULONG from, ULONG to);
void check_active_table (void);
#endif

/* opcodes.c */
extern struct opcode_entry opcode_table [];

/* opcode_handler.c */

uint EA_to_Rn (struct opcode_entry *op);
uint Rn_to_EA (struct opcode_entry *op);
uint op1 (struct opcode_entry *op);
uint op1_end (struct opcode_entry *op);
uint imm_b (struct opcode_entry *op);
uint imm_w (struct opcode_entry *op);
uint imm_l (struct opcode_entry *op);
uint quick (struct opcode_entry *op);
uint bit_reg (struct opcode_entry *op);
uint bit_imm (struct opcode_entry *op);
uint move (struct opcode_entry *op);
uint movem (struct opcode_entry *op);
uint moveq (struct opcode_entry *op);
uint srccr (struct opcode_entry *op);
uint special (struct opcode_entry *op);
uint illegal (struct opcode_entry *op);
uint invalid (struct opcode_entry *op);
uint branch (struct opcode_entry *op);
uint dbranch (struct opcode_entry *op);
uint shiftreg (struct opcode_entry *op);
uint op2_bcdx (struct opcode_entry *op);
uint scc (struct opcode_entry *op);
uint exg (struct opcode_entry *op);
uint movesrccr (struct opcode_entry *op);
uint cmpm (struct opcode_entry *op);
uint movep (struct opcode_entry *op);
uint bkpt (struct opcode_entry *op);
uint muldivl (struct opcode_entry *op);
uint bitfield (struct opcode_entry *op);
uint trapcc (struct opcode_entry *op);
uint chkcmp2 (struct opcode_entry *op);
uint cas (struct opcode_entry *op);
uint cas2 (struct opcode_entry *op);
uint moves (struct opcode_entry *op);
uint link_l (struct opcode_entry *op);
uint move16 (struct opcode_entry *op);
uint cache (struct opcode_entry *op);
uint mov3q (struct opcode_entry *op); /* ColdFire */
uint invalid2 (struct opcode_sub_entry *op); /* for mmu & fpu subtables */

#define SNG_ALL 1        /* Single operand syntax allowed */

/* opcodes_fpu.c */
extern struct opcode_sub_entry fpu_opcode_table [];
extern char *xfer_size [];
extern UWORD sizes [];
extern char *fpu_conditions [];

/* opcode_handler_fpu.c */
uint fpu (struct opcode_entry *op);
uint fscc (struct opcode_entry *op);
uint fbranch (struct opcode_entry *op);
uint ftrapcc (struct opcode_entry *op);
uint fdbranch (struct opcode_entry *op);

uint std_fpu (struct opcode_sub_entry *op);
uint fsincos (struct opcode_sub_entry *op);
uint ftst (struct opcode_sub_entry *op);


/* opcodes_mmu.c */
extern struct opcode_sub_entry mmu_opcode_table [];

/* opcode_handler_mmu.c */
uint plpa60 (struct opcode_entry *op);
uint pflush40 (struct opcode_entry *op);
uint ptest40 (struct opcode_entry *op);
uint mmu30 (struct opcode_entry *op);
uint ptest30 (struct opcode_entry *op);

uint pfl_or_ld (struct opcode_sub_entry *op);
uint pflush30 (struct opcode_sub_entry *op);
uint pmove30 (struct opcode_sub_entry *op);

/* amiga.c */
BOOL user_aborted_analysis (void);
void delete_tmp_files (void);
#ifdef AMIGA
void use_host_cpu_type (void);
ULONG req_off (void);
void req_on (ULONG oldwindowptr);
#endif

/* version.c */
void print_version (void);

/* jmptab.c */
void free_jmptab_list (void);
void enter_jmptab (ULONG start, ULONG offset);
BOOL next_code_ref_from_jmptab (UBYTE *seg);
/* BOOL invalidate_last_jmptab_entry (void); */
BOOL find_and_format_jmptab (char *to, WORD current_w);
#ifdef DEBUG
void print_jmptab_list (void);
#endif

/* library.c */
void enter_library_refs (UBYTE *seg);

UWORD lw(UWORD*);
ULONG llu(UWORD*);
LONG lls(UWORD*);
