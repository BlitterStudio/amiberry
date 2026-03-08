/*
 * Change history
 * $Log:	opcodes.c,v $
 *
 * Revision 3.1  09/10/15  Matt_Hey
 * New feature: Added 68060 MMU opcodes for plpaw & plpar
 * Minor mod.: pea (and lea) now enter_ref() ACC_UNKNOWN instead of ACC_LONG
 *             as nothing is known about data length or type at that offset
 *
 * Revision 3.0  93/09/24  17:54:19  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.2  93/07/18  22:56:30  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.1  93/07/08  20:49:58  Martin_Apel
 * Bug fix: Enabled addressing mode 0 for mmu30 instruction
 * 
 * Revision 2.0  93/07/01  11:54:33  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.13  93/07/01  11:43:07  Martin_Apel
 * Bug fix: TST.x Ax for 68020 enabled (Error in 68020 manual)
 * 
 * Revision 1.12  93/06/19  12:11:54  Martin_Apel
 * Major mod.: Added full 68030/040 support
 * 
 * Revision 1.11  93/06/16  20:29:10  Martin_Apel
 * Minor mod.: Removed #ifdef FPU_VERSION and #ifdef MACHINE68020
 * Minor mod.: Added variables for 68030 / 040 support
 * 
 * Revision 1.10  93/06/03  20:29:25  Martin_Apel
 * 
 * 
 */

/* The opcode table is an opcode_entry array (defined in defs.h) that is
   indexed by the first 10 bits of each instruction encoding. The handler
   of that index is then called. See disasm_instr () in disasm_code.c for use. */

#include "defs.h"

/* static char rcsid [] = "$Id: opcodes.c,v 3.0 93/09/24 17:54:19 Martin_Apel Exp $"; */

/* If the opcode_table is 16 byte aligned, then only one cache line is needed
   per entry instead of 2. Unfortunatly, __attribute__((aligned(16)) doesn't
   seem to work with GCC 3.4.0. The Amiga hunk loader can only guarentee 4 byte
   alignment (8 with TLSFMem?). Memory could be dynamically allocated, manually
   aligned, and copied but that needs more memory and effort. */

struct opcode_entry opcode_table [] =
{
/* *handler , modes, submodes, chain, *mnemonic , param, access */

{ imm_b    , 0xfd, 0x03, 1026, "ori.b"   , 0, ACC_BYTE     }, /* 0000 000 000 */
{ imm_w    , 0xfd, 0x03, 1026, "ori.w"   , 0, ACC_WORD     }, /* 0000 000 001 */
{ imm_l    , 0xfd, 0x03, 1024, "ori.l"   , 0, ACC_LONG     }, /* 0000 000 010 */
{ chkcmp2  , 0xe4, 0x0f, 1056, "c  2.b"  , 0, ACC_BYTE     }, /* 0000 000 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 0, 0            }, /* 0000 000 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 0, 0            }, /* 0000 000 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 0, 0            }, /* 0000 000 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 0, 0            }, /* 0000 000 111 */
{ imm_b    , 0xfd, 0x03, 1026, "andi.b"  , 0, ACC_BYTE     }, /* 0000 001 000 */
{ imm_w    , 0xfd, 0x03, 1026, "andi.w"  , 0, ACC_WORD     }, /* 0000 001 001 */
{ imm_l    , 0xfd, 0x03, 1024, "andi.l"  , 0, ACC_LONG     }, /* 0000 001 010 */
{ chkcmp2  , 0xe4, 0x0f, 1057, "c  2.w"  , 0, ACC_WORD     }, /* 0000 001 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 1, 0            }, /* 0000 001 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 1, 0            }, /* 0000 001 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 1, 0            }, /* 0000 001 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 1, 0            }, /* 0000 001 111 */
{ imm_b    , 0xfd, 0x03, 1024, "subi.b"  , 0, ACC_BYTE     }, /* 0000 010 000 */
{ imm_w    , 0xfd, 0x03, 1024, "subi.w"  , 0, ACC_WORD     }, /* 0000 010 001 */
{ imm_l    , 0xfd, 0x03, 1024, "subi.l"  , 0, ACC_LONG     }, /* 0000 010 010 */
{ chkcmp2  , 0xe4, 0x0f, 1058, "c  2.l"  , 0, ACC_LONG     }, /* 0000 010 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 2, 0            }, /* 0000 010 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 2, 0            }, /* 0000 010 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 2, 0            }, /* 0000 010 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 2, 0            }, /* 0000 010 111 */
{ imm_b    , 0xfd, 0x03, 1024, "addi.b"  , 0, ACC_BYTE     }, /* 0000 011 000 */
{ imm_w    , 0xfd, 0x03, 1024, "addi.w"  , 0, ACC_WORD     }, /* 0000 011 001 */
{ imm_l    , 0xfd, 0x03, 1024, "addi.l"  , 0, ACC_LONG     }, /* 0000 011 010 */
{ imm_b    , 0xe4, 0x0f, 1027, "callm"   , 0, 0            }, /* 0000 011 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 3, 0            }, /* 0000 011 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 3, 0            }, /* 0000 011 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 3, 0            }, /* 0000 011 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 3, 0            }, /* 0000 011 111 */
{ bit_imm  , 0xfd, 0x0f, 1024, "btst"    , 0, 0            }, /* 0000 100 000 */
{ bit_imm  , 0xfd, 0x03, 1024, "bchg"    , 0, 0            }, /* 0000 100 001 */
{ bit_imm  , 0xfd, 0x03, 1024, "bclr"    , 0, 0            }, /* 0000 100 010 */
{ bit_imm  , 0xfd, 0x03, 1024, "bset"    , 0, 0            }, /* 0000 100 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 4, 0            }, /* 0000 100 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 4, 0            }, /* 0000 100 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 4, 0            }, /* 0000 100 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 4, 0            }, /* 0000 100 111 */
{ imm_b    , 0xfd, 0x03, 1026, "eori.b"  , 0, ACC_BYTE     }, /* 0000 101 000 */
{ imm_w    , 0xfd, 0x03, 1026, "eori.w"  , 0, ACC_WORD     }, /* 0000 101 001 */
{ imm_l    , 0xfd, 0x03, 1024, "eori.l"  , 0, ACC_LONG     }, /* 0000 101 010 */
{ cas      , 0xfc, 0x03, 1040, "cas.b"   , 0, ACC_BYTE     }, /* 0000 101 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 5, 0            }, /* 0000 101 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 5, 0            }, /* 0000 101 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 5, 0            }, /* 0000 101 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 5, 0            }, /* 0000 101 111 */
{ imm_b    , 0xfd, 0x0f, 1024, "cmpi.b"  , 0, ACC_BYTE     }, /* 0000 110 000 */
{ imm_w    , 0xfd, 0x0f, 1024, "cmpi.w"  , 0, ACC_WORD     }, /* 0000 110 001 */
{ imm_l    , 0xfd, 0x0f, 1024, "cmpi.l"  , 0, ACC_LONG     }, /* 0000 110 010 */
{ cas      , 0xfc, 0x03, 1041, "cas.w"   , 0, ACC_WORD     }, /* 0000 110 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 6, 0            }, /* 0000 110 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 6, 0            }, /* 0000 110 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 6, 0            }, /* 0000 110 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 6, 0            }, /* 0000 110 111 */
{ moves    , 0xfc, 0x03, 1024, "moves.b" , 0, ACC_BYTE     }, /* 0000 111 000 */
{ moves    , 0xfc, 0x03, 1024, "moves.w" , 0, ACC_WORD     }, /* 0000 111 001 */
{ moves    , 0xfc, 0x03, 1024, "moves.l" , 0, ACC_LONG     }, /* 0000 111 010 */
{ cas      , 0xfc, 0x03, 1042, "cas.l"   , 0, ACC_LONG     }, /* 0000 111 011 */
{ bit_reg  , 0xfd, 0x1f, 1037, "btst"    , 7, 0            }, /* 0000 111 100 */
{ bit_reg  , 0xfd, 0x03, 1037, "bchg"    , 7, 0            }, /* 0000 111 101 */
{ bit_reg  , 0xfd, 0x03, 1037, "bclr"    , 7, 0            }, /* 0000 111 110 */
{ bit_reg  , 0xfd, 0x03, 1037, "bset"    , 7, 0            }, /* 0000 111 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 000 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0001 000 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 000 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 000 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 000 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 000 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 000 110 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 000 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 1, ACC_BYTE     }, /* 0001 001 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 1, 0            }, /* 0001 001 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 001 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 001 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 001 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 001 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 001 110 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 001 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 2, ACC_BYTE     }, /* 0001 010 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 2, 0            }, /* 0001 010 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 010 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 010 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 010 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 010 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 010 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 2, 0            }, /* 0001 010 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 3, ACC_BYTE     }, /* 0001 011 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 3, 0            }, /* 0001 011 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 011 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 011 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 011 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 011 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 011 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 3, 0            }, /* 0001 011 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 4, ACC_BYTE     }, /* 0001 100 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 4, 0            }, /* 0001 100 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 100 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 100 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 100 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 100 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 100 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 4, 0            }, /* 0001 100 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 5, ACC_BYTE     }, /* 0001 101 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 5, 0            }, /* 0001 101 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 101 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 101 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 101 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 101 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 101 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 5, 0            }, /* 0001 101 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 6, ACC_BYTE     }, /* 0001 110 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 6, 0            }, /* 0001 110 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 110 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 110 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 110 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 110 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 110 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 6, 0            }, /* 0001 110 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.b"  , 7, ACC_BYTE     }, /* 0001 111 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 0001 111 001 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 111 010 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 111 011 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 111 100 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 111 101 */
{ move     , 0xff, 0x1f, 1024, "move.b"  , 0, ACC_BYTE     }, /* 0001 111 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 0001 111 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 000 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" , 8, ACC_LONG     }, /* 0010 000 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 000 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 000 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 000 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 000 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 000 110 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 000 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 1, ACC_LONG     }, /* 0010 001 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" , 9, ACC_LONG     }, /* 0010 001 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 001 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 001 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 001 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 001 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 001 110 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 001 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 2, ACC_LONG     }, /* 0010 010 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" ,10, ACC_LONG     }, /* 0010 010 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 010 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 010 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 010 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 010 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 010 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 2, 0            }, /* 0010 010 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 3, ACC_LONG     }, /* 0010 011 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" ,11, ACC_LONG     }, /* 0010 011 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 011 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 011 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 011 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 011 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 011 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 3, 0            }, /* 0010 011 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 4, ACC_LONG     }, /* 0010 100 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" ,12, ACC_LONG     }, /* 0010 100 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 100 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 100 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 100 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 100 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 100 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 4, 0            }, /* 0010 100 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 5, ACC_LONG     }, /* 0010 101 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" ,13, ACC_LONG     }, /* 0010 101 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 101 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 101 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 101 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 101 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 101 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 5, 0            }, /* 0010 101 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 6, ACC_LONG     }, /* 0010 110 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" ,14, ACC_LONG     }, /* 0010 110 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 110 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 110 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 110 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 110 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 110 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 6, 0            }, /* 0010 110 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.l"  , 7, ACC_LONG     }, /* 0010 111 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.l" ,15, ACC_LONG     }, /* 0010 111 001 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 111 010 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 111 011 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 111 100 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 111 101 */
{ move     , 0xff, 0x1f, 1024, "move.l"  , 0, ACC_LONG     }, /* 0010 111 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 0010 111 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 000 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" , 8, ACC_WORD     }, /* 0011 000 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 000 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 000 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 000 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 000 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 000 110 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 000 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 1, ACC_WORD     }, /* 0011 001 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" , 9, ACC_WORD     }, /* 0011 001 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 001 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 001 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 001 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 001 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 001 110 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 001 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 2, ACC_WORD     }, /* 0011 010 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" ,10, ACC_WORD     }, /* 0011 010 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 010 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 010 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 010 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 010 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 010 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 2, 0            }, /* 0011 010 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 3, ACC_WORD     }, /* 0011 011 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" ,11, ACC_WORD     }, /* 0011 011 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 011 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 011 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 011 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 011 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 011 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 3, 0            }, /* 0011 011 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 4, ACC_WORD     }, /* 0011 100 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" ,12, ACC_WORD     }, /* 0011 100 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 100 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 100 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 100 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 100 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 100 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 4, 0            }, /* 0011 100 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 5, ACC_WORD     }, /* 0011 101 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" ,13, ACC_WORD     }, /* 0011 101 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 101 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 101 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 101 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 101 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 101 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 5, 0            }, /* 0011 101 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 6, ACC_WORD     }, /* 0011 110 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" ,14, ACC_WORD     }, /* 0011 110 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 110 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 110 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 110 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 110 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 110 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 6, 0            }, /* 0011 110 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "move.w"  , 7, ACC_WORD     }, /* 0011 111 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "movea.w" ,15, ACC_WORD     }, /* 0011 111 001 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 111 010 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 111 011 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 111 100 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 111 101 */
{ move     , 0xff, 0x1f, 1024, "move.w"  , 0, ACC_WORD     }, /* 0011 111 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 0011 111 111 */
{ op1      , 0xfd, 0x03, 1024, "negx.b"  , 0, ACC_BYTE     }, /* 0100 000 000 */
{ op1      , 0xfd, 0x03, 1024, "negx.w"  , 0, ACC_WORD     }, /* 0100 000 001 */
{ op1      , 0xfd, 0x03, 1024, "negx.l"  , 0, ACC_LONG     }, /* 0100 000 010 */
{ movesrccr, 0xfd, 0x03, 1024, "move.w", FROM_SR, ACC_WORD }, /* 0100 000 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 0, ACC_LONG     }, /* 0100 000 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 000 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 0, ACC_WORD     }, /* 0100 000 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1024, "lea"     , 8, ACC_UNKNOWN  }, /* 0100 000 111 */
{ op1      , 0xfd, 0x03, 1024, "clr.b"   , 0, ACC_BYTE     }, /* 0100 001 000 */
{ op1      , 0xfd, 0x03, 1024, "clr.w"   , 0, ACC_WORD     }, /* 0100 001 001 */
{ op1      , 0xfd, 0x03, 1024, "clr.l"   , 0, ACC_LONG     }, /* 0100 001 010 */
{ movesrccr, 0xfd, 0x03, 1024, "move.w", FROM_CCR, ACC_WORD}, /* 0100 001 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 1, ACC_LONG     }, /* 0100 001 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 001 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 1, ACC_WORD     }, /* 0100 001 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1024, "lea"     , 9, ACC_UNKNOWN  }, /* 0100 001 111 */
{ op1      , 0xfd, 0x03, 1024, "neg.b"   , 0, ACC_BYTE     }, /* 0100 010 000 */
{ op1      , 0xfd, 0x03, 1024, "neg.w"   , 0, ACC_WORD     }, /* 0100 010 001 */
{ op1      , 0xfd, 0x03, 1024, "neg.l"   , 0, ACC_LONG     }, /* 0100 010 010 */
{ movesrccr, 0xfd, 0x1f, 1024, "move.w", TO_CCR, ACC_WORD  }, /* 0100 010 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 2, ACC_LONG     }, /* 0100 010 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 010 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 2, ACC_WORD     }, /* 0100 010 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1024, "lea"     ,10, ACC_UNKNOWN  }, /* 0100 010 111 */
{ op1      , 0xfd, 0x03, 1024, "not.b"   , 0, ACC_BYTE     }, /* 0100 011 000 */
{ op1      , 0xfd, 0x03, 1024, "not.w"   , 0, ACC_WORD     }, /* 0100 011 001 */
{ op1      , 0xfd, 0x03, 1024, "not.l"   , 0, ACC_LONG     }, /* 0100 011 010 */
{ movesrccr, 0xfd, 0x1f, 1024, "move.w", TO_SR, ACC_WORD   }, /* 0100 011 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 3, ACC_LONG     }, /* 0100 011 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 011 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 3, ACC_WORD     }, /* 0100 011 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1024, "lea"     ,11, ACC_UNKNOWN  }, /* 0100 011 111 */
{ op1      , 0xfd, 0x03, 1055, "nbcd.b"  , 0, ACC_BYTE     }, /* 0100 100 000 */
{ op1      , 0xe4, 0x0f, 1029, "pea"     , 0, ACC_UNKNOWN  }, /* 0100 100 001 */
{ movem    , 0xf4, 0x03, 1030, "movem.w", REG_TO_MEM, 0    }, /* 0100 100 010 */
{ movem    , 0xf4, 0x03, 1050, "movem.l", REG_TO_MEM, 0    }, /* 0100 100 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 4, ACC_LONG     }, /* 0100 100 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 100 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 4, ACC_WORD     }, /* 0100 100 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1051, "lea"     ,12, ACC_UNKNOWN  }, /* 0100 100 111 */
{ op1      , 0xff, 0x1f, 1024, "tst.b"   , 0, ACC_BYTE     }, /* 0100 101 000 */
{ op1      , 0xff, 0x1f, 1024, "tst.w"   , 0, ACC_WORD     }, /* 0100 101 001 */
{ op1      , 0xff, 0x1f, 1024, "tst.l"   , 0, ACC_LONG     }, /* 0100 101 010 */
{ op1      , 0xfd, 0x03, 1025, "tas"     , 0, ACC_BYTE     }, /* 0100 101 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 5, ACC_LONG     }, /* 0100 101 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 101 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 5, ACC_WORD     }, /* 0100 101 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1024, "lea"     ,13, ACC_UNKNOWN  }, /* 0100 101 111 */
{ muldivl  , 0xfd, 0x1f, 1024, "mul "    , 0, ACC_LONG     }, /* 0100 110 000 */
{ muldivl  , 0xfd, 0x1f, 1024, "div "    , 0, ACC_LONG     }, /* 0100 110 001 */
{ movem    , 0xec, 0x0f, 1059, "movem.w" , MEM_TO_REG, 0   }, /* 0100 110 010 */
{ movem    , 0xec, 0x0f, 1024, "movem.l" , MEM_TO_REG, 0   }, /* 0100 110 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 6, ACC_LONG     }, /* 0100 110 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 110 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 6, ACC_WORD     }, /* 0100 110 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1024, "lea"     ,14, ACC_UNKNOWN  }, /* 0100 110 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 111 000 */
{ special  , 0xff, 0x0c, 1024, 0         , 0, 0            }, /* 0100 111 001 */ 
{ op1      , 0xe4, 0x0f, 1024, "jsr"     , 0, ACC_CODE     }, /* 0100 111 010 */
{ op1_end  , 0xe4, 0x0f, 1024, "jmp"     , 0, ACC_CODE     }, /* 0100 111 011 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.l"   , 7, ACC_LONG     }, /* 0100 111 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 0100 111 101 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "chk.w"   , 7, ACC_WORD     }, /* 0100 111 110 */
{ EA_to_Rn , 0xe4, 0x0f, 1024, "lea"     ,15, ACC_UNKNOWN  }, /* 0100 111 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 8, ACC_BYTE     }, /* 0101 000 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 8, ACC_WORD     }, /* 0101 000 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 8, ACC_LONG     }, /* 0101 000 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbt"     , 0, 0            }, /* 0101 000 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 8, ACC_BYTE     }, /* 0101 000 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 8, ACC_WORD     }, /* 0101 000 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 8, ACC_LONG     }, /* 0101 000 110 */
{ dbranch  , 0x02, 0xff, 1046, "dbra"    , 0, 0            }, /* 0101 000 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 1, ACC_BYTE     }, /* 0101 001 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 1, ACC_WORD     }, /* 0101 001 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 1, ACC_LONG     }, /* 0101 001 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbhi"    , 1, 0            }, /* 0101 001 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 1, ACC_BYTE     }, /* 0101 001 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 1, ACC_WORD     }, /* 0101 001 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 1, ACC_LONG     }, /* 0101 001 110 */
{ dbranch  , 0x02, 0xff, 1046, "dbls"    , 1, 0            }, /* 0101 001 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 2, ACC_BYTE     }, /* 0101 010 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 2, ACC_WORD     }, /* 0101 010 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 2, ACC_LONG     }, /* 0101 010 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbcc"    , 2, 0            }, /* 0101 010 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 2, ACC_BYTE     }, /* 0101 010 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 2, ACC_WORD     }, /* 0101 010 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 2, ACC_LONG     }, /* 0101 010 110 */
{ dbranch  , 0x02, 0xff, 1046, "dbcs"    , 2, 0            }, /* 0101 010 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 3, ACC_BYTE     }, /* 0101 011 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 3, ACC_WORD     }, /* 0101 011 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 3, ACC_LONG     }, /* 0101 011 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbne"    , 3, 0            }, /* 0101 011 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 3, ACC_BYTE     }, /* 0101 011 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 3, ACC_WORD     }, /* 0101 011 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 3, ACC_LONG     }, /* 0101 011 110 */
{ dbranch  , 0x02, 0xff, 1046, "dbeq"    , 3, 0            }, /* 0101 011 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 4, ACC_BYTE     }, /* 0101 100 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 4, ACC_WORD     }, /* 0101 100 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 4, ACC_LONG     }, /* 0101 100 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbvc"    , 4, 0            }, /* 0101 100 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 4, ACC_BYTE     }, /* 0101 100 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 4, ACC_WORD     }, /* 0101 100 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 4, ACC_LONG     }, /* 0101 100 110 */
{ dbranch  , 0x02, 0xff, 1046, "dbvs"    , 4, 0            }, /* 0101 100 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 5, ACC_BYTE     }, /* 0101 101 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 5, ACC_WORD     }, /* 0101 101 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 5, ACC_LONG     }, /* 0101 101 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbpl"    , 5, 0            }, /* 0101 101 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 5, ACC_BYTE     }, /* 0101 101 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 5, ACC_WORD     }, /* 0101 101 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 5, ACC_LONG     }, /* 0101 101 110 */
{ dbranch  , 0x02, 0xff, 1046, "dbmi"    , 5, 0            }, /* 0101 101 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 6, ACC_BYTE     }, /* 0101 110 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 6, ACC_WORD     }, /* 0101 110 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 6, ACC_LONG     }, /* 0101 110 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbge"    , 6, 0            }, /* 0101 110 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 6, ACC_BYTE     }, /* 0101 110 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 6, ACC_WORD     }, /* 0101 110 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 6, ACC_LONG     }, /* 0101 110 110 */
{ dbranch  , 0x02, 0xff, 1046, "dble"    , 6, 0            }, /* 0101 110 111 */
{ quick    , 0xff, 0x03, 1024, "addq.b"  , 7, ACC_BYTE     }, /* 0101 111 000 */
{ quick    , 0xff, 0x03, 1024, "addq.w"  , 7, ACC_WORD     }, /* 0101 111 001 */
{ quick    , 0xff, 0x03, 1024, "addq.l"  , 7, ACC_LONG     }, /* 0101 111 010 */
{ dbranch  , 0x02, 0xff, 1046, "dbgt"    , 7, 0            }, /* 0101 111 011 */
{ quick    , 0xff, 0x03, 1024, "subq.b"  , 7, ACC_BYTE     }, /* 0101 111 100 */
{ quick    , 0xff, 0x03, 1024, "subq.w"  , 7, ACC_WORD     }, /* 0101 111 101 */
{ quick    , 0xff, 0x03, 1024, "subq.l"  , 7, ACC_LONG     }, /* 0101 111 110 */
{ dbranch  , 0x02, 0xff, 1046, "dble"    , 7, 0            }, /* 0101 111 111 */
{ branch   , 0xff, 0xff, 1024, "bra"     , 0, 0            }, /* 0110 000 000 */
{ branch   , 0xff, 0xff, 1024, "bra"     , 0, 0            }, /* 0110 000 001 */
{ branch   , 0xff, 0xff, 1024, "bra"     , 0, 0            }, /* 0110 000 010 */
{ branch   , 0xff, 0xff, 1024, "bra"     , 0, 0            }, /* 0110 000 011 */
{ branch   , 0xff, 0xff, 1024, "bsr"     , 0, 0            }, /* 0110 000 100 */
{ branch   , 0xff, 0xff, 1024, "bsr"     , 0, 0            }, /* 0110 000 101 */
{ branch   , 0xff, 0xff, 1024, "bsr"     , 0, 0            }, /* 0110 000 110 */
{ branch   , 0xff, 0xff, 1024, "bsr"     , 0, 0            }, /* 0110 000 111 */
{ branch   , 0xff, 0xff, 1024, "bhi"     , 0, 0            }, /* 0110 001 000 */
{ branch   , 0xff, 0xff, 1024, "bhi"     , 0, 0            }, /* 0110 001 001 */
{ branch   , 0xff, 0xff, 1024, "bhi"     , 0, 0            }, /* 0110 001 010 */
{ branch   , 0xff, 0xff, 1024, "bhi"     , 0, 0            }, /* 0110 001 011 */
{ branch   , 0xff, 0xff, 1024, "bls"     , 0, 0            }, /* 0110 001 100 */
{ branch   , 0xff, 0xff, 1024, "bls"     , 0, 0            }, /* 0110 001 101 */
{ branch   , 0xff, 0xff, 1024, "bls"     , 0, 0            }, /* 0110 001 110 */
{ branch   , 0xff, 0xff, 1024, "bls"     , 0, 0            }, /* 0110 001 111 */
{ branch   , 0xff, 0xff, 1024, "bcc"     , 0, 0            }, /* 0110 010 000 */
{ branch   , 0xff, 0xff, 1024, "bcc"     , 0, 0            }, /* 0110 010 001 */
{ branch   , 0xff, 0xff, 1024, "bcc"     , 0, 0            }, /* 0110 010 010 */
{ branch   , 0xff, 0xff, 1024, "bcc"     , 0, 0            }, /* 0110 010 011 */
{ branch   , 0xff, 0xff, 1024, "bcs"     , 0, 0            }, /* 0110 010 100 */
{ branch   , 0xff, 0xff, 1024, "bcs"     , 0, 0            }, /* 0110 010 101 */
{ branch   , 0xff, 0xff, 1024, "bcs"     , 0, 0            }, /* 0110 010 110 */
{ branch   , 0xff, 0xff, 1024, "bcs"     , 0, 0            }, /* 0110 010 111 */
{ branch   , 0xff, 0xff, 1024, "bne"     , 0, 0            }, /* 0110 011 000 */
{ branch   , 0xff, 0xff, 1024, "bne"     , 0, 0            }, /* 0110 011 001 */
{ branch   , 0xff, 0xff, 1024, "bne"     , 0, 0            }, /* 0110 011 010 */
{ branch   , 0xff, 0xff, 1024, "bne"     , 0, 0            }, /* 0110 011 011 */
{ branch   , 0xff, 0xff, 1024, "beq"     , 0, 0            }, /* 0110 011 100 */
{ branch   , 0xff, 0xff, 1024, "beq"     , 0, 0            }, /* 0110 011 101 */
{ branch   , 0xff, 0xff, 1024, "beq"     , 0, 0            }, /* 0110 011 110 */
{ branch   , 0xff, 0xff, 1024, "beq"     , 0, 0            }, /* 0110 011 111 */
{ branch   , 0xff, 0xff, 1024, "bvc"     , 0, 0            }, /* 0110 100 000 */
{ branch   , 0xff, 0xff, 1024, "bvc"     , 0, 0            }, /* 0110 100 001 */
{ branch   , 0xff, 0xff, 1024, "bvc"     , 0, 0            }, /* 0110 100 010 */
{ branch   , 0xff, 0xff, 1024, "bvc"     , 0, 0            }, /* 0110 100 011 */
{ branch   , 0xff, 0xff, 1024, "bvs"     , 0, 0            }, /* 0110 100 100 */
{ branch   , 0xff, 0xff, 1024, "bvs"     , 0, 0            }, /* 0110 100 101 */
{ branch   , 0xff, 0xff, 1024, "bvs"     , 0, 0            }, /* 0110 100 110 */
{ branch   , 0xff, 0xff, 1024, "bvs"     , 0, 0            }, /* 0110 100 111 */
{ branch   , 0xff, 0xff, 1024, "bpl"     , 0, 0            }, /* 0110 101 000 */
{ branch   , 0xff, 0xff, 1024, "bpl"     , 0, 0            }, /* 0110 101 001 */
{ branch   , 0xff, 0xff, 1024, "bpl"     , 0, 0            }, /* 0110 101 010 */
{ branch   , 0xff, 0xff, 1024, "bpl"     , 0, 0            }, /* 0110 101 011 */
{ branch   , 0xff, 0xff, 1024, "bmi"     , 0, 0            }, /* 0110 101 100 */
{ branch   , 0xff, 0xff, 1024, "bmi"     , 0, 0            }, /* 0110 101 101 */
{ branch   , 0xff, 0xff, 1024, "bmi"     , 0, 0            }, /* 0110 101 110 */
{ branch   , 0xff, 0xff, 1024, "bmi"     , 0, 0            }, /* 0110 101 111 */
{ branch   , 0xff, 0xff, 1024, "bge"     , 0, 0            }, /* 0110 110 000 */
{ branch   , 0xff, 0xff, 1024, "bge"     , 0, 0            }, /* 0110 110 001 */
{ branch   , 0xff, 0xff, 1024, "bge"     , 0, 0            }, /* 0110 110 010 */
{ branch   , 0xff, 0xff, 1024, "bge"     , 0, 0            }, /* 0110 110 011 */
{ branch   , 0xff, 0xff, 1024, "blt"     , 0, 0            }, /* 0110 110 100 */
{ branch   , 0xff, 0xff, 1024, "blt"     , 0, 0            }, /* 0110 110 101 */
{ branch   , 0xff, 0xff, 1024, "blt"     , 0, 0            }, /* 0110 110 110 */
{ branch   , 0xff, 0xff, 1024, "blt"     , 0, 0            }, /* 0110 110 111 */
{ branch   , 0xff, 0xff, 1024, "bgt"     , 0, 0            }, /* 0110 111 000 */
{ branch   , 0xff, 0xff, 1024, "bgt"     , 0, 0            }, /* 0110 111 001 */
{ branch   , 0xff, 0xff, 1024, "bgt"     , 0, 0            }, /* 0110 111 010 */
{ branch   , 0xff, 0xff, 1024, "bgt"     , 0, 0            }, /* 0110 111 011 */
{ branch   , 0xff, 0xff, 1024, "ble"     , 0, 0            }, /* 0110 111 100 */
{ branch   , 0xff, 0xff, 1024, "ble"     , 0, 0            }, /* 0110 111 101 */
{ branch   , 0xff, 0xff, 1024, "ble"     , 0, 0            }, /* 0110 111 110 */
{ branch   , 0xff, 0xff, 1024, "ble"     , 0, 0            }, /* 0110 111 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 0, 0            }, /* 0111 000 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 0, 0            }, /* 0111 000 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 0, 0            }, /* 0111 000 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 0, 0            }, /* 0111 000 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 0, ACC_SBYTE    }, /* 0111 000 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 0, ACC_SWORD    }, /* 0111 000 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 0, ACC_BYTE     }, /* 0111 000 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 0, ACC_WORD     }, /* 0111 000 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 1, 0            }, /* 0111 001 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 1, 0            }, /* 0111 001 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 1, 0            }, /* 0111 001 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 1, 0            }, /* 0111 001 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 1, ACC_SBYTE    }, /* 0111 001 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 1, ACC_SWORD    }, /* 0111 001 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 1, ACC_BYTE     }, /* 0111 001 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 1, ACC_WORD     }, /* 0111 001 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 2, 0            }, /* 0111 010 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 2, 0            }, /* 0111 010 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 2, 0            }, /* 0111 010 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 2, 0            }, /* 0111 010 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 2, ACC_SBYTE    }, /* 0111 010 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 2, ACC_SWORD    }, /* 0111 010 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 2, ACC_BYTE     }, /* 0111 010 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 2, ACC_WORD     }, /* 0111 010 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 3, 0            }, /* 0111 011 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 3, 0            }, /* 0111 011 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 3, 0            }, /* 0111 011 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 3, 0            }, /* 0111 011 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 3, ACC_SBYTE    }, /* 0111 011 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 3, ACC_SWORD    }, /* 0111 011 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 3, ACC_BYTE     }, /* 0111 011 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 3, ACC_WORD     }, /* 0111 011 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 4, 0            }, /* 0111 100 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 4, 0            }, /* 0111 100 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 4, 0            }, /* 0111 100 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 4, 0            }, /* 0111 100 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 4, ACC_SBYTE    }, /* 0111 100 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 4, ACC_SWORD    }, /* 0111 100 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 4, ACC_BYTE     }, /* 0111 100 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 4, ACC_WORD     }, /* 0111 100 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 5, 0            }, /* 0111 101 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 5, 0            }, /* 0111 101 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 5, 0            }, /* 0111 101 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 5, 0            }, /* 0111 101 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 5, ACC_SBYTE    }, /* 0111 101 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 5, ACC_SWORD    }, /* 0111 101 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 5, ACC_BYTE     }, /* 0111 101 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 5, ACC_WORD     }, /* 0111 101 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 6, 0            }, /* 0111 110 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 6, 0            }, /* 0111 110 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 6, 0            }, /* 0111 110 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 6, 0            }, /* 0111 110 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 6, ACC_SBYTE    }, /* 0111 110 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 6, ACC_SWORD    }, /* 0111 110 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 6, ACC_BYTE     }, /* 0111 110 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 6, ACC_WORD     }, /* 0111 110 111 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 7, 0            }, /* 0111 111 000 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 7, 0            }, /* 0111 111 001 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 7, 0            }, /* 0111 111 010 */
{ moveq    , 0xff, 0xff, 1024, "moveq"   , 7, 0            }, /* 0111 111 011 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.b"   , 7, ACC_SBYTE    }, /* 0111 111 100 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvs.w"   , 7, ACC_SWORD    }, /* 0111 111 101 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.b"   , 7, ACC_BYTE     }, /* 0111 111 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "mvz.w"   , 7, ACC_WORD     }, /* 0111 111 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 0, ACC_BYTE     }, /* 1000 000 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 0, ACC_WORD     }, /* 1000 000 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 0, ACC_LONG     }, /* 1000 000 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 0, ACC_WORD     }, /* 1000 000 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 0, ACC_BYTE     }, /* 1000 000 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 0, ACC_WORD     }, /* 1000 000 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 0, ACC_LONG     }, /* 1000 000 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 0, ACC_SWORD    }, /* 1000 000 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 1, ACC_BYTE     }, /* 1000 001 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 1, ACC_WORD     }, /* 1000 001 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 1, ACC_LONG     }, /* 1000 001 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 1, ACC_WORD     }, /* 1000 001 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 1, ACC_BYTE     }, /* 1000 001 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 1, ACC_WORD     }, /* 1000 001 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 1, ACC_LONG     }, /* 1000 001 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 1, ACC_SWORD    }, /* 1000 001 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 2, ACC_BYTE     }, /* 1000 010 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 2, ACC_WORD     }, /* 1000 010 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 2, ACC_LONG     }, /* 1000 010 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 2, ACC_WORD     }, /* 1000 010 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 2, ACC_BYTE     }, /* 1000 010 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 2, ACC_WORD     }, /* 1000 010 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 2, ACC_LONG     }, /* 1000 010 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 2, ACC_SWORD    }, /* 1000 010 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 3, ACC_BYTE     }, /* 1000 011 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 3, ACC_WORD     }, /* 1000 011 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 3, ACC_LONG     }, /* 1000 011 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 3, ACC_WORD     }, /* 1000 011 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 3, ACC_BYTE     }, /* 1000 011 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 3, ACC_WORD     }, /* 1000 011 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 3, ACC_LONG     }, /* 1000 011 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 3, ACC_SWORD    }, /* 1000 011 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 4, ACC_BYTE     }, /* 1000 100 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 4, ACC_WORD     }, /* 1000 100 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 4, ACC_LONG     }, /* 1000 100 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 4, ACC_WORD     }, /* 1000 100 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 4, ACC_BYTE     }, /* 1000 100 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 4, ACC_WORD     }, /* 1000 100 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 4, ACC_LONG     }, /* 1000 100 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 4, ACC_SWORD    }, /* 1000 100 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 5, ACC_BYTE     }, /* 1000 101 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 5, ACC_WORD     }, /* 1000 101 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 5, ACC_LONG     }, /* 1000 101 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 5, ACC_WORD     }, /* 1000 101 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 5, ACC_BYTE     }, /* 1000 101 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 5, ACC_WORD     }, /* 1000 101 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 5, ACC_LONG     }, /* 1000 101 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 5, ACC_SWORD    }, /* 1000 101 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 6, ACC_BYTE     }, /* 1000 110 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 6, ACC_WORD     }, /* 1000 110 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 6, ACC_LONG     }, /* 1000 110 00 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 6, ACC_WORD     }, /* 1000 110 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 6, ACC_BYTE     }, /* 1000 110 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 6, ACC_WORD     }, /* 1000 110 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 6, ACC_LONG     }, /* 1000 110 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 6, ACC_SWORD    }, /* 1000 110 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.b"    , 7, ACC_BYTE     }, /* 1000 111 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.w"    , 7, ACC_WORD     }, /* 1000 111 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "or.l"    , 7, ACC_LONG     }, /* 1000 111 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divu.w"  , 7, ACC_WORD     }, /* 1000 111 011 */
{ Rn_to_EA , 0xfc, 0x03, 1031, "or.b"    , 7, ACC_BYTE     }, /* 1000 111 100 */
{ Rn_to_EA , 0xfc, 0x03, 1032, "or.w"    , 7, ACC_WORD     }, /* 1000 111 101 */
{ Rn_to_EA , 0xfc, 0x03, 1028, "or.l"    , 7, ACC_LONG     }, /* 1000 111 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "divs.w"  , 7, ACC_SWORD    }, /* 1000 111 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 0, ACC_BYTE     }, /* 1001 000 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 0, ACC_WORD     }, /* 1001 000 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 0, ACC_LONG     }, /* 1001 000 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  , 8, ACC_WORD     }, /* 1001 000 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 0, ACC_BYTE     }, /* 1001 000 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 0, ACC_WORD     }, /* 1001 000 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 0, ACC_LONG     }, /* 1001 000 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  , 8, ACC_LONG     }, /* 1001 000 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 1, ACC_BYTE     }, /* 1001 001 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 1, ACC_WORD     }, /* 1001 001 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 1, ACC_LONG     }, /* 1001 001 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  , 9, ACC_WORD     }, /* 1001 001 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 1, ACC_BYTE     }, /* 1001 001 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 1, ACC_WORD     }, /* 1001 001 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 1, ACC_LONG     }, /* 1001 001 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  , 9, ACC_LONG     }, /* 1001 001 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 2, ACC_BYTE     }, /* 1001 010 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 2, ACC_WORD     }, /* 1001 010 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 2, ACC_LONG     }, /* 1001 010 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  ,10, ACC_WORD     }, /* 1001 010 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 2, ACC_BYTE     }, /* 1001 010 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 2, ACC_WORD     }, /* 1001 010 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 2, ACC_LONG     }, /* 1001 010 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  ,10, ACC_LONG     }, /* 1001 010 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 3, ACC_BYTE     }, /* 1001 011 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 3, ACC_WORD     }, /* 1001 011 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 3, ACC_LONG     }, /* 1001 011 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  ,11, ACC_WORD     }, /* 1001 011 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 3, ACC_BYTE     }, /* 1001 011 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 3, ACC_WORD     }, /* 1001 011 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 3, ACC_LONG     }, /* 1001 011 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  ,11, ACC_LONG     }, /* 1001 011 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 4, ACC_BYTE     }, /* 1001 100 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 4, ACC_WORD     }, /* 1001 100 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 4, ACC_LONG     }, /* 1001 100 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  ,12, ACC_WORD     }, /* 1001 100 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 4, ACC_BYTE     }, /* 1001 100 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 4, ACC_WORD     }, /* 1001 100 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 4, ACC_LONG     }, /* 1001 100 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  ,12, ACC_LONG     }, /* 1001 100 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 5, ACC_BYTE     }, /* 1001 101 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 5, ACC_WORD     }, /* 1001 101 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 5, ACC_LONG     }, /* 1001 101 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  ,13, ACC_WORD     }, /* 1001 101 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 5, ACC_BYTE     }, /* 1001 101 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 5, ACC_WORD     }, /* 1001 101 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 5, ACC_LONG     }, /* 1001 101 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  ,13, ACC_LONG     }, /* 1001 101 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 6, ACC_BYTE     }, /* 1001 110 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 6, ACC_WORD     }, /* 1001 110 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 6, ACC_LONG     }, /* 1001 110 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  ,14, ACC_WORD     }, /* 1001 110 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 6, ACC_BYTE     }, /* 1001 110 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 6, ACC_WORD     }, /* 1001 110 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 6, ACC_LONG     }, /* 1001 110 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  ,14, ACC_LONG     }, /* 1001 110 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.b"   , 7, ACC_BYTE     }, /* 1001 111 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.w"   , 7, ACC_WORD     }, /* 1001 111 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "sub.l"   , 7, ACC_LONG     }, /* 1001 111 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.w"  ,15, ACC_WORD     }, /* 1001 111 011 */
{ Rn_to_EA , 0xfc, 0x03, 1052, "sub.b"   , 7, ACC_BYTE     }, /* 1001 111 100 */
{ Rn_to_EA , 0xfc, 0x03, 1053, "sub.w"   , 7, ACC_WORD     }, /* 1001 111 101 */
{ Rn_to_EA , 0xfc, 0x03, 1054, "sub.l"   , 7, ACC_LONG     }, /* 1001 111 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "suba.l"  ,15, ACC_LONG     }, /* 1001 111 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 000 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 001 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 010 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 011 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 100 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 101 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 110 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1010 111 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 0, ACC_BYTE     }, /* 1011 000 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 0, ACC_WORD     }, /* 1011 000 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 0, ACC_LONG     }, /* 1011 000 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  , 8, ACC_WORD     }, /* 1011 000 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 0, ACC_BYTE     }, /* 1011 000 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 0, ACC_WORD     }, /* 1011 000 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 0, ACC_LONG     }, /* 1011 000 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  , 8, ACC_LONG     }, /* 1011 000 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 1, ACC_BYTE     }, /* 1011 001 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 1, ACC_WORD     }, /* 1011 001 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 1, ACC_LONG     }, /* 1011 001 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  , 9, ACC_WORD     }, /* 1011 001 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 1, ACC_BYTE     }, /* 1011 001 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 1, ACC_WORD     }, /* 1011 001 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 1, ACC_LONG     }, /* 1011 001 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  , 9, ACC_LONG     }, /* 1011 001 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 2, ACC_BYTE     }, /* 1011 010 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 2, ACC_WORD     }, /* 1011 010 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 2, ACC_LONG     }, /* 1011 010 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  ,10, ACC_WORD     }, /* 1011 010 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 2, ACC_BYTE     }, /* 1011 010 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 2, ACC_WORD     }, /* 1011 010 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 2, ACC_LONG     }, /* 1011 010 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  ,10, ACC_LONG     }, /* 1011 010 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 3, ACC_BYTE     }, /* 1011 011 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 3, ACC_WORD     }, /* 1011 011 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 3, ACC_LONG     }, /* 1011 011 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  ,11, ACC_WORD     }, /* 1011 011 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 3, ACC_BYTE     }, /* 1011 011 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 3, ACC_WORD     }, /* 1011 011 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 3, ACC_LONG     }, /* 1011 011 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  ,11, ACC_LONG     }, /* 1011 011 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 4, ACC_BYTE     }, /* 1011 100 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 4, ACC_WORD     }, /* 1011 100 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 4, ACC_LONG     }, /* 1011 100 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  ,12, ACC_WORD     }, /* 1011 100 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 4, ACC_BYTE     }, /* 1011 100 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 4, ACC_WORD     }, /* 1011 100 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 4, ACC_LONG     }, /* 1011 100 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  ,12, ACC_LONG     }, /* 1011 100 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 5, ACC_BYTE     }, /* 1011 101 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 5, ACC_WORD     }, /* 1011 101 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 5, ACC_LONG     }, /* 1011 101 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  ,13, ACC_WORD     }, /* 1011 101 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 5, ACC_BYTE     }, /* 1011 101 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 5, ACC_WORD     }, /* 1011 101 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 5, ACC_LONG     }, /* 1011 101 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  ,13, ACC_LONG     }, /* 1011 101 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 6, ACC_BYTE     }, /* 1011 110 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 6, ACC_WORD     }, /* 1011 110 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 6, ACC_LONG     }, /* 1011 110 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  ,14, ACC_WORD     }, /* 1011 110 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 6, ACC_BYTE     }, /* 1011 110 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 6, ACC_WORD     }, /* 1011 110 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 6, ACC_LONG     }, /* 1011 110 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  ,14, ACC_LONG     }, /* 1011 110 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.b"   , 7, ACC_BYTE     }, /* 1011 111 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.w"   , 7, ACC_WORD     }, /* 1011 111 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmp.l"   , 7, ACC_LONG     }, /* 1011 111 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.w"  ,15, ACC_WORD     }, /* 1011 111 011 */
{ Rn_to_EA , 0xfd, 0x03, 1034, "eor.b"   , 7, ACC_BYTE     }, /* 1011 111 100 */
{ Rn_to_EA , 0xfd, 0x03, 1043, "eor.w"   , 7, ACC_WORD     }, /* 1011 111 101 */
{ Rn_to_EA , 0xfd, 0x03, 1044, "eor.l"   , 7, ACC_LONG     }, /* 1011 111 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "cmpa.l"  ,15, ACC_LONG     }, /* 1011 111 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 0, ACC_BYTE     }, /* 1100 000 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 0, ACC_WORD     }, /* 1100 000 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 0, ACC_LONG     }, /* 1100 000 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 0, ACC_WORD     }, /* 1100 000 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 0, ACC_BYTE     }, /* 1100 000 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 0, ACC_WORD     }, /* 1100 000 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 0, ACC_LONG     }, /* 1100 000 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 0, ACC_SWORD    }, /* 1100 000 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 1, ACC_BYTE     }, /* 1100 001 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 1, ACC_WORD     }, /* 1100 001 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 1, ACC_LONG     }, /* 1100 001 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 1, ACC_WORD     }, /* 1100 001 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 1, ACC_BYTE     }, /* 1100 001 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 1, ACC_WORD     }, /* 1100 001 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 1, ACC_LONG     }, /* 1100 001 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 1, ACC_SWORD    }, /* 1100 001 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 2, ACC_BYTE     }, /* 1100 010 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 2, ACC_WORD     }, /* 1100 010 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 2, ACC_LONG     }, /* 1100 010 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 2, ACC_WORD     }, /* 1100 010 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 2, ACC_BYTE     }, /* 1100 010 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 2, ACC_WORD     }, /* 1100 010 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 2, ACC_LONG     }, /* 1100 010 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 2, ACC_SWORD    }, /* 1100 010 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 3, ACC_BYTE     }, /* 1100 011 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 3, ACC_WORD     }, /* 1100 011 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 3, ACC_LONG     }, /* 1100 011 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 3, ACC_WORD     }, /* 1100 011 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 3, ACC_BYTE     }, /* 1100 011 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 3, ACC_WORD     }, /* 1100 011 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 3, ACC_LONG     }, /* 1100 011 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 3, ACC_SWORD    }, /* 1100 011 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 4, ACC_BYTE     }, /* 1100 100 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 4, ACC_WORD     }, /* 1100 100 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 4, ACC_LONG     }, /* 1100 100 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 4, ACC_WORD     }, /* 1100 100 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 4, ACC_BYTE     }, /* 1100 100 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 4, ACC_WORD     }, /* 1100 100 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 4, ACC_LONG     }, /* 1100 100 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 4, ACC_SWORD    }, /* 1100 100 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 5, ACC_BYTE     }, /* 1100 101 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 5, ACC_WORD     }, /* 1100 101 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 5, ACC_LONG     }, /* 1100 101 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 5, ACC_WORD     }, /* 1100 101 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 5, ACC_BYTE     }, /* 1100 101 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 5, ACC_WORD     }, /* 1100 101 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 5, ACC_LONG     }, /* 1100 101 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 5, ACC_SWORD    }, /* 1100 101 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 6, ACC_BYTE     }, /* 1100 110 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 6, ACC_WORD     }, /* 1100 110 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 6, ACC_LONG     }, /* 1100 110 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 6, ACC_WORD     }, /* 1100 110 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 6, ACC_BYTE     }, /* 1100 110 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 6, ACC_WORD     }, /* 1100 110 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 6, ACC_LONG     }, /* 1100 110 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 6, ACC_SWORD    }, /* 1100 110 111 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.b"   , 7, ACC_BYTE     }, /* 1100 111 000 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.w"   , 7, ACC_WORD     }, /* 1100 111 001 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "and.l"   , 7, ACC_LONG     }, /* 1100 111 010 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "mulu.w"  , 7, ACC_WORD     }, /* 1100 111 011 */
{ Rn_to_EA , 0xfc, 0x03, 1035, "and.b"   , 7, ACC_BYTE     }, /* 1100 111 100 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.w"   , 7, ACC_WORD     }, /* 1100 111 101 */
{ Rn_to_EA , 0xfc, 0x03, 1036, "and.l"   , 7, ACC_LONG     }, /* 1100 111 110 */
{ EA_to_Rn , 0xfd, 0x1f, 1024, "muls.w"  , 7, ACC_SWORD    }, /* 1100 111 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 0, ACC_BYTE     }, /* 1101 000 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 0, ACC_WORD     }, /* 1101 000 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 0, ACC_LONG     }, /* 1101 000 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  , 8, ACC_WORD     }, /* 1101 000 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 0, ACC_BYTE     }, /* 1101 000 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 0, ACC_WORD     }, /* 1101 000 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 0, ACC_LONG     }, /* 1101 000 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  , 8, ACC_LONG     }, /* 1101 000 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 1, ACC_BYTE     }, /* 1101 001 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 1, ACC_WORD     }, /* 1101 001 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 1, ACC_LONG     }, /* 1101 001 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  , 9, ACC_WORD     }, /* 1101 001 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 1, ACC_BYTE     }, /* 1101 001 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 1, ACC_WORD     }, /* 1101 001 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 1, ACC_LONG     }, /* 1101 001 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  , 9, ACC_LONG     }, /* 1101 001 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 2, ACC_BYTE     }, /* 1101 010 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 2, ACC_WORD     }, /* 1101 010 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 2, ACC_LONG     }, /* 1101 010 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  ,10, ACC_WORD     }, /* 1101 010 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 2, ACC_BYTE     }, /* 1101 010 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 2, ACC_WORD     }, /* 1101 010 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 2, ACC_LONG     }, /* 1101 010 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  ,10, ACC_LONG     }, /* 1101 010 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 3, ACC_BYTE     }, /* 1101 011 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 3, ACC_WORD     }, /* 1101 011 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 3, ACC_LONG     }, /* 1101 011 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  ,11, ACC_WORD     }, /* 1101 011 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 3, ACC_BYTE     }, /* 1101 011 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 3, ACC_WORD     }, /* 1101 011 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 3, ACC_LONG     }, /* 1101 011 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  ,11, ACC_LONG     }, /* 1101 011 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 4, ACC_BYTE     }, /* 1101 100 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 4, ACC_WORD     }, /* 1101 100 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 4, ACC_LONG     }, /* 1101 100 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  ,12, ACC_WORD     }, /* 1101 100 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 4, ACC_BYTE     }, /* 1101 100 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 4, ACC_WORD     }, /* 1101 100 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 4, ACC_LONG     }, /* 1101 100 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  ,12, ACC_LONG     }, /* 1101 100 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 5, ACC_BYTE     }, /* 1101 101 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 5, ACC_WORD     }, /* 1101 101 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 5, ACC_LONG     }, /* 1101 101 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  ,13, ACC_WORD     }, /* 1101 101 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 5, ACC_BYTE     }, /* 1101 101 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 5, ACC_WORD     }, /* 1101 101 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 5, ACC_LONG     }, /* 1101 101 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  ,13, ACC_LONG     }, /* 1101 101 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 6, ACC_BYTE     }, /* 1101 110 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 6, ACC_WORD     }, /* 1101 110 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 6, ACC_LONG     }, /* 1101 110 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  ,14, ACC_WORD     }, /* 1101 110 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 6, ACC_BYTE     }, /* 1101 110 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 6, ACC_WORD     }, /* 1101 110 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 6, ACC_LONG     }, /* 1101 110 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  ,14, ACC_LONG     }, /* 1101 110 111 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.b"   , 7, ACC_BYTE     }, /* 1101 111 000 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.w"   , 7, ACC_WORD     }, /* 1101 111 001 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "add.l"   , 7, ACC_LONG     }, /* 1101 111 010 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.w"  ,15, ACC_WORD     }, /* 1101 111 011 */
{ Rn_to_EA , 0xfc, 0x03, 1033, "add.b"   , 7, ACC_BYTE     }, /* 1101 111 100 */
{ Rn_to_EA , 0xfc, 0x03, 1038, "add.w"   , 7, ACC_WORD     }, /* 1101 111 101 */
{ Rn_to_EA , 0xfc, 0x03, 1039, "add.l"   , 7, ACC_LONG     }, /* 1101 111 110 */
{ EA_to_Rn , 0xff, 0x1f, 1024, "adda.l"  ,15, ACC_LONG     }, /* 1101 111 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 8, 0            }, /* 1110 000 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 8, 0            }, /* 1110 000 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 8, 0            }, /* 1110 000 010 */
{ op1      , 0xfc, 0x03, 1024, "asr.w"   , 0, ACC_WORD     }, /* 1110 000 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 8, 0            }, /* 1110 000 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 8, 0            }, /* 1110 000 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 8, 0            }, /* 1110 000 110 */
{ op1      , 0xfc, 0x03, 1024, "asl.w"   , 0, ACC_WORD     }, /* 1110 000 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 1, 0            }, /* 1110 001 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 1, 0            }, /* 1110 001 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 1, 0            }, /* 1110 001 010 */
{ op1      , 0xfc, 0x03, 1024, "lsr.w"   , 0, ACC_WORD     }, /* 1110 001 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 1, 0            }, /* 1110 001 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 1, 0            }, /* 1110 001 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 1, 0            }, /* 1110 001 110 */
{ op1      , 0xfc, 0x03, 1024, "lsl.w"   , 0, ACC_WORD     }, /* 1110 001 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 2, 0            }, /* 1110 010 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 2, 0            }, /* 1110 010 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 2, 0            }, /* 1110 010 010 */
{ op1      , 0xfc, 0x03, 1024, "roxr.w"  , 0, ACC_WORD     }, /* 1110 010 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 2, 0            }, /* 1110 010 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 2, 0            }, /* 1110 010 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 2, 0            }, /* 1110 010 110 */
{ op1      , 0xfc, 0x03, 1024, "roxl.w"  , 0, ACC_WORD     }, /* 1110 010 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 3, 0            }, /* 1110 011 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 3, 0            }, /* 1110 011 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 3, 0            }, /* 1110 011 010 */
{ op1      , 0xfc, 0x03, 1024, "ror.w"   , 0, ACC_WORD     }, /* 1110 011 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 3, 0            }, /* 1110 011 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 3, 0            }, /* 1110 011 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 3, 0            }, /* 1110 011 110 */
{ op1      , 0xfc, 0x03, 1024, "rol.w"   , 0, ACC_WORD     }, /* 1110 011 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 4, 0            }, /* 1110 100 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 4, 0            }, /* 1110 100 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 4, 0            }, /* 1110 100 010 */
{ bitfield , 0xe5, 0x0f, 1024, "bftst"   , SINGLEOP, 0     }, /* 1110 100 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 4, 0            }, /* 1110 100 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 4, 0            }, /* 1110 100 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 4, 0            }, /* 1110 100 110 */
{ bitfield , 0xe5, 0x0f, 1024, "bfextu"  , DATADEST, 0     }, /* 1110 100 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 5, 0            }, /* 1110 101 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 5, 0            }, /* 1110 101 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 5, 0            }, /* 1110 101 010 */
{ bitfield , 0xe5, 0x03, 1024, "bfchg"   , SINGLEOP, 0     }, /* 1110 101 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 5, 0            }, /* 1110 101 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 5, 0            }, /* 1110 101 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 5, 0            }, /* 1110 101 110 */
{ bitfield , 0xe5, 0x0f, 1024, "bfexts"  , DATADEST, 0     }, /* 1110 101 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 6, 0            }, /* 1110 110 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 6, 0            }, /* 1110 110 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 6, 0            }, /* 1110 110 010 */
{ bitfield , 0xe5, 0x03, 1024, "bfclr"   , SINGLEOP, 0     }, /* 1110 110 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 6, 0            }, /* 1110 110 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 6, 0            }, /* 1110 110 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 6, 0            }, /* 1110 110 110 */
{ bitfield , 0xe5, 0x0f, 1024, "bfffo"   , DATADEST, 0     }, /* 1110 110 111 */
{ shiftreg , 0xff, 0xff, 1024, "r.b"     , 7, 0            }, /* 1110 111 000 */
{ shiftreg , 0xff, 0xff, 1024, "r.w"     , 7, 0            }, /* 1110 111 001 */
{ shiftreg , 0xff, 0xff, 1024, "r.l"     , 7, 0            }, /* 1110 111 010 */
{ bitfield , 0xe5, 0x03, 1024, "bfset"   , SINGLEOP, 0     }, /* 1110 111 011 */
{ shiftreg , 0xff, 0xff, 1024, "l.b"     , 7, 0            }, /* 1110 111 100 */
{ shiftreg , 0xff, 0xff, 1024, "l.w"     , 7, 0            }, /* 1110 111 101 */
{ shiftreg , 0xff, 0xff, 1024, "l.l"     , 7, 0            }, /* 1110 111 110 */
{ bitfield , 0xe5, 0x03, 1024, "bfins"   , DATASRC, 0      }, /* 1110 111 111 */

{ mmu30    , 0xe5, 0x03, 1024, 0         , 7, 0            }, /* 1111 000 000 */   
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 000 001 */   
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 000 010 */   
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 000 011 */   
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 000 100 */   
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 000 101 */   
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 000 110 */   
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 000 111 */   
/* FPU opcode encodings */
{ fpu      , 0xff, 0xff, 1024, 0         , 1, 0            }, /* 1111 001 000 */
{ fscc     , 0xfd, 0x03, 1047, "fs"      , 1, 0            }, /* 1111 001 001 */
{ fbranch  , 0xff, 0x00, 1024, "fb"      , 0, ACC_CODE     }, /* 1111 001 010 */
{ fbranch  , 0xff, 0x00, 1024, "fb"      , 1, ACC_CODE     }, /* 1111 001 011 */
{ op1      , 0xf4, 0x03, 1024, "fsave"   , 0, ACC_LONG     }, /* 1111 001 100 */
{ op1      , 0xec, 0x0f, 1024, "frestore", 0, ACC_LONG     }, /* 1111 001 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1111 001 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 0, 0            }, /* 1111 001 111 */

/* Opcodes for user-definable coprocessors */
{ cache    , 0xee, 0xff, 1024, 0         , 0, 0            }, /* 1111 010 000 */
{ cache    , 0xee, 0xff, 1024, 0         , 1, 0            }, /* 1111 010 001 */
{ cache    , 0xee, 0xff, 1024, 0         , 2, 0            }, /* 1111 010 010 */
{ cache    , 0xee, 0xff, 1024, 0         , 3, 0            }, /* 1111 010 011 */
{ pflush40 , 0x0f, 0xff, 1024, "pflush"  , 0, 0            }, /* 1111 010 100 */
{ ptest40  , 0x22, 0xff, 1024, "ptest"   , 7, 0            }, /* 1111 010 101 */
{ plpa60   , 0x02, 0x00, 1024, "plpaw"   , 7, 0            }, /* 1111 010 110 */
{ plpa60   , 0x02, 0x00, 1024, "plpar"   , 7, 0            }, /* 1111 010 111 */
{ move16   , 0x1f, 0xff, 1024, "move16"  , 7, 0            }, /* 1111 011 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 011 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 011 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 011 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 011 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 011 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 011 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 011 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 100 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 101 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 110 111 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 000 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 001 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 010 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 011 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 100 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 101 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 110 */
{ invalid  , 0xff, 0xff, 1024, 0         , 7, 0            }, /* 1111 111 111 */

/* Here are the chained routines for handling doubly-defined opcodes */

{ invalid  , 0xff, 0xff,    0, 0         , 0, 0            }, /* 1024 */
{ illegal  , 0x80, 0x10, 1024, "illegal" , 0, 0            }, /* 1025 */
{ srccr    , 0x80, 0x10, 1024, 0         , 0, 0            }, /* 1026 */
{ op1_end  , 0x03, 0x00, 1024, "rtm"     , 0, 0            }, /* 1027 */
{ op2_bcdx , 0x03, 0x00, 1024, "unpk"    , ADJUST, 0       }, /* 1028 */
{ op1      , 0x01, 0x00, 1049, "swap"    , 0, ACC_LONG     }, /* 1029 */
{ op1      , 0x01, 0x00, 1024, "ext.w"   , 0, ACC_WORD     }, /* 1030 */
{ op2_bcdx , 0x03, 0x00, 1024, "sbcd"    , NO_ADJ, 0       }, /* 1031 */
{ op2_bcdx , 0x03, 0x00, 1024, "pack"    , ADJUST, 0       }, /* 1032 */
{ op2_bcdx , 0x03, 0x00, 1024, "addx.b"  , NO_ADJ, 0       }, /* 1033 */
{ cmpm     , 0x02, 0xff, 1024, "cmpm.b"  , 0, 0            }, /* 1034 */
{ op2_bcdx , 0x03, 0x00, 1024, "abcd.b"  , NO_ADJ, 0       }, /* 1035 */
{ exg      , 0x03, 0x00, 1024, "exg"     , 0, 0            }, /* 1036 */
{ movep    , 0x02, 0x00, 1024, "movep"   , 0, 0            }, /* 1037 */
{ op2_bcdx , 0x03, 0x00, 1024, "addx.w"  , NO_ADJ, 0       }, /* 1038 */
{ op2_bcdx , 0x03, 0x00, 1024, "addx.l"  , NO_ADJ, 0       }, /* 1039 */
{ cas2     , 0x80, 0x10, 1024, "cas2.b"  , 0, ACC_BYTE     }, /* 1040 */
{ cas2     , 0x80, 0x10, 1024, "cas2.w"  , 0, ACC_WORD     }, /* 1041 */
{ cas2     , 0x80, 0x10, 1024, "cas2.l"  , 0, ACC_LONG     }, /* 1042 */
{ cmpm     , 0x02, 0xff, 1024, "cmpm.w"  , 0, 0            }, /* 1043 */
{ cmpm     , 0x02, 0xff, 1024, "cmpm.l"  , 0, 0            }, /* 1044 */
{ trapcc   , 0x80, 0x1c, 1024, "trap"    , 0, 0            }, /* 1045 */
{ scc      , 0xfd, 0x03, 1045, "s"       , 0, ACC_BYTE     }, /* 1046 */
{ fdbranch , 0x02, 0x00, 1048, "fdb"     , 0, 0            }, /* 1047 */
{ ftrapcc  , 0x80, 0x1c, 1024, "ftrap"   , 0, 0            }, /* 1048 */
{ bkpt     , 0x02, 0xff, 1024, "bkpt"    , 0, 0            }, /* 1049 */    
{ op1      , 0x01, 0x00, 1024, "ext.l"   , 0, ACC_LONG     }, /* 1050 */
{ op1      , 0x01, 0x00, 1024, "extb.l"  , 0, ACC_LONG     }, /* 1051 */
{ op2_bcdx , 0x03, 0x00, 1024, "subx.b"  , NO_ADJ, 0       }, /* 1052 */
{ op2_bcdx , 0x03, 0x00, 1024, "subx.w"  , NO_ADJ, 0       }, /* 1053 */
{ op2_bcdx , 0x03, 0x00, 1024, "subx.l"  , NO_ADJ, 0       }, /* 1054 */
{ link_l   , 0x02, 0x00, 1024, "link.l"  , 0, 0            }  /* 1055 */
};
