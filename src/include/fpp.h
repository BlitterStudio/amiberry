
/* single   : S  8*E 23*F */
/* double   : S 11*E 52*F */
/* extended : S 15*E 64*F */
/* E = 0 & F = 0 -> 0 */
/* E = MAX & F = 0 -> Infin */
/* E = MAX & F # 0 -> NotANumber */
/* E = biased by 127 (single) ,1023 (double) ,16383 (extended) */

#pragma once
#define FPSR_BSUN       0x00008000
#define FPSR_SNAN       0x00004000
#define FPSR_OPERR      0x00002000
#define FPSR_OVFL       0x00001000
#define FPSR_UNFL       0x00000800
#define FPSR_DZ         0x00000400
#define FPSR_INEX2      0x00000200
#define FPSR_INEX1      0x00000100

#if defined(CPU_i386) || defined(CPU_x86_64)
extern void init_fpucw_x87(void);
#endif

#define PREC_NORMAL 0
#define PREC_FLOAT 1
#define PREC_DOUBLE 2
#define PREC_EXTENDED 3
