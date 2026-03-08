/*
 * Change history
 * $Log:	fpu_opcodes.c,v $
 * Revision 3.0  93/09/24  17:53:57  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.1  93/07/18  22:55:54  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.0  93/07/01  11:53:57  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.6  93/06/16  20:27:22  Martin_Apel
 * Minor mod.: Removed #ifdef FPU_VERSION and #ifdef MACHINE68020
 * 
 * Revision 1.5  93/06/03  20:27:05  Martin_Apel
 * 
 * 
 */

#include "defs.h"

/* static char rcsid [] = "$Id: fpu_opcodes.c,v 3.0 93/09/24 17:53:57 Martin_Apel Exp $"; */

struct opcode_sub_entry fpu_opcode_table [] =
{
/* *handler,  *mnemonic, param, access */

{ std_fpu  , "fmove"  ,       0, 0 },        /* 000 0000 */
{ std_fpu  , "fint"   , SNG_ALL, 0 },        /* 000 0001 */
{ std_fpu  , "fsinh"  , SNG_ALL, 0 },        /* 000 0010 */
{ std_fpu  , "fintrz" , SNG_ALL, 0 },        /* 000 0011 */
{ std_fpu  , "fsqrt"  , SNG_ALL, 0 },        /* 000 0100 */
{ invalid2 ,  0       ,       0, 0 },        /* 000 0101 */
{ std_fpu  , "flognp1", SNG_ALL, 0 },        /* 000 0110 */
{ invalid2 ,  0       ,       0, 0 },        /* 000 0111 */
{ std_fpu  , "fetoxm1", SNG_ALL, 0 },        /* 000 1000 */
{ std_fpu  , "ftanh"  , SNG_ALL, 0 },        /* 000 1001 */
{ std_fpu  , "fatan"  , SNG_ALL, 0 },        /* 000 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 000 1011 */
{ std_fpu  , "fasin"  , SNG_ALL, 0 },        /* 000 1100 */
{ std_fpu  , "fatanh" , SNG_ALL, 0 },        /* 000 1101 */
{ std_fpu  , "fsin"   , SNG_ALL, 0 },        /* 000 1110 */
{ std_fpu  , "ftan"   , SNG_ALL, 0 },        /* 000 1111 */
{ std_fpu  , "fetox"  , SNG_ALL, 0 },        /* 001 0000 */
{ std_fpu  , "ftwotox", SNG_ALL, 0 },        /* 001 0001 */
{ std_fpu  , "ftentox", SNG_ALL, 0 },        /* 001 0010 */
{ invalid2 ,  0       ,       0, 0 },        /* 001 0011 */
{ std_fpu  , "flogn"  , SNG_ALL, 0 },        /* 001 0100 */
{ std_fpu  , "flog10" , SNG_ALL, 0 },        /* 001 0101 */
{ std_fpu  , "flog2"  , SNG_ALL, 0 },        /* 001 0110 */
{ invalid2 ,  0       ,       0, 0 },        /* 001 0111 */
{ std_fpu  , "fabs"   , SNG_ALL, 0 },        /* 001 1000 */
{ std_fpu  , "fcosh"  , SNG_ALL, 0 },        /* 001 1001 */
{ std_fpu  , "fneg"   , SNG_ALL, 0 },        /* 001 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 001 1011 */
{ std_fpu  , "facos"  , SNG_ALL, 0 },        /* 001 1100 */
{ std_fpu  , "fcos"   , SNG_ALL, 0 },        /* 001 1101 */
{ std_fpu  , "fgetexp", SNG_ALL, 0 },        /* 001 1110 */
{ std_fpu  , "fgetman", SNG_ALL, 0 },        /* 001 1111 */
{ std_fpu  , "fdiv"   ,       0, 0 },        /* 010 0000 */
{ std_fpu  , "fmod"   ,       0, 0 },        /* 010 0001 */
{ std_fpu  , "fadd"   ,       0, 0 },        /* 010 0010 */
{ std_fpu  , "fmul"   ,       0, 0 },        /* 010 0011 */
{ std_fpu  , "fsgldiv",       0, 0 },        /* 010 0100 */
{ std_fpu  , "frem"   ,       0, 0 },        /* 010 0101 */
{ std_fpu  , "fscale" ,       0, 0 },        /* 010 0110 */
{ std_fpu  , "fsglmul",       0, 0 },        /* 010 0111 */
{ std_fpu  , "fsub"   ,       0, 0 },        /* 010 1000 */
{ invalid2 ,  0       ,       0, 0 },        /* 010 1001 */
{ invalid2 ,  0       ,       0, 0 },        /* 010 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 010 1011 */
{ invalid2 ,  0       ,       0, 0 },        /* 010 1100 */
{ invalid2 ,  0       ,       0, 0 },        /* 010 1101 */
{ invalid2 ,  0       ,       0, 0 },        /* 010 1110 */
{ invalid2 ,  0       ,       0, 0 },        /* 010 1111 */
{ fsincos  , "fsincos",       0, 0 },        /* 011 0000 */
{ fsincos  , "fsincos",       1, 0 },        /* 011 0001 */
{ fsincos  , "fsincos",       2, 0 },        /* 011 0010 */
{ fsincos  , "fsincos",       3, 0 },        /* 011 0011 */
{ fsincos  , "fsincos",       4, 0 },        /* 011 0100 */
{ fsincos  , "fsincos",       5, 0 },        /* 011 0101 */
{ fsincos  , "fsincos",       6, 0 },        /* 011 0110 */
{ fsincos  , "fsincos",       7, 0 },        /* 011 0111 */
{ std_fpu  , "fcmp"   ,       0, 0 },        /* 011 1000 */
{ invalid2 ,  0       ,       0, 0 },        /* 011 1001 */
{ ftst     , "ftst"   ,       0, 0 },        /* 011 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 011 1011 */
{ invalid2 ,  0       ,       0, 0 },        /* 011 1100 */
{ invalid2 ,  0       ,       0, 0 },        /* 011 1101 */
{ invalid2 ,  0       ,       0, 0 },        /* 011 1110 */
{ invalid2 ,  0       ,       0, 0 },        /* 011 1111 */

{ std_fpu  , "fsmove" ,       0, 0 },        /* 100 0000 */
{ std_fpu  , "fssqrt" , SNG_ALL, 0 },        /* 100 0001 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 0010 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 0011 */
{ std_fpu  , "fdmove" ,       0, 0 },        /* 100 0100 */
{ std_fpu  , "fdsqrt" , SNG_ALL, 0 },        /* 100 0101 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 0110 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 0111 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1000 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1001 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1011 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1100 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1101 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1110 */
{ invalid2 ,  0       ,       0, 0 },        /* 100 1111 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0000 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0001 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0010 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0011 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0100 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0101 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0110 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 0111 */
{ std_fpu  , "fsabs"  , SNG_ALL, 0 },        /* 101 1000 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 1001 */
{ std_fpu  , "fsneg"  , SNG_ALL, 0 },        /* 101 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 1011 */
{ std_fpu  , "fdabs"  , SNG_ALL, 0 },        /* 101 1100 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 1101 */
{ std_fpu  , "fdneg"  , SNG_ALL, 0 },        /* 101 1110 */
{ invalid2 ,  0       ,       0, 0 },        /* 101 1111 */
{ std_fpu  , "fsdiv"  ,       0, 0 },        /* 110 0000 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 0001 */
{ std_fpu  , "fsadd"  ,       0, 0 },        /* 110 0010 */
{ std_fpu  , "fsmul"  ,       0, 0 },        /* 110 0011 */
{ std_fpu  , "fddiv"  ,       0, 0 },        /* 110 0100 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 0101 */
{ std_fpu  , "fdadd"  ,       0, 0 },        /* 110 0110 */
{ std_fpu  , "fdmul"  ,       0, 0 },        /* 110 0111 */
{ std_fpu  , "fssub"  ,       0, 0 },        /* 110 1000 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 1001 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 1011 */
{ std_fpu  , "fdsub"  ,       0, 0 },        /* 110 1100 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 1101 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 1110 */
{ invalid2 ,  0       ,       0, 0 },        /* 110 1111 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0000 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0001 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0010 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0011 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0100 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0101 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0110 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 0111 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 1000 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 1001 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 1010 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 1011 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 1100 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 1101 */
{ invalid2 ,  0       ,       0, 0 },        /* 111 1110 */
{ invalid2 ,  0       ,       0, 0 }         /* 111 1111 */
};

char *xfer_size [] = { ".l",
                       ".s",
                       ".x",
                       ".p",
                       ".w",
                       ".d",
                       ".b",
                       ".p",  };

UWORD sizes [] = { ACC_LONG   | ACC_DATA,
                   ACC_LONG   | ACC_DATA,
                   ACC_EXTEND | ACC_DATA,
                   ACC_EXTEND | ACC_DATA,
                   ACC_WORD   | ACC_DATA,
                   ACC_DOUBLE | ACC_DATA,
                   ACC_BYTE   | ACC_DATA,
                   ACC_EXTEND | ACC_DATA };

char *fpu_conditions [] = {
  "f",
  "eq",
  "ogt",
  "oge",
  "olt",
  "ole",
  "ogl",
  "or",
  "un",        /* 8 */
  "ueq",
  "ugt",
  "uge",
  "ult",
  "ule",
  "ne",
  "t",
  "sf",        /* 16 */
  "seq",
  "gt",
  "ge",
  "lt",
  "le",
  "gl",
  "gle",
  "ngle",      /* 24 */
  "ngl",
  "nle",
  "nlt",
  "nge",
  "ngt",
  "sne",
  "st"
};
