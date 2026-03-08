/*
 * Change history
 * $Log:	string_recog.h,v $
 * Revision 3.0  93/09/24  17:54:41  Martin_Apel
 * New feature: Added extra 68040 FPU opcodes
 * 
 * Revision 2.1  93/07/18  22:57:12  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 2.0  93/07/01  11:55:11  Martin_Apel
 * *** empty log message ***
 * 
 * Revision 1.4  93/06/03  20:31:25  Martin_Apel
 * 
 * 
 */

/* $Id: string_recog.h,v 3.0 93/09/24 17:54:41 Martin_Apel Exp $ */

#define C_PRINT   0x01  /* printable character including german umlauts */
#define C_NAME    0x02  /* character is valid in a C variable name */
#define C_NORM    0x04  /* common/normal character */
#define C_NUM     0x08  /* character is a decimal number 0-9 */
#define C_HEX     0x10  /* character is a hexadecimal number 0-9,A-F,a-f */
#define C_PATH    0x20  /* character is valid in a path and file name */
#define C_ALL     0x80  /* all valid printable or control characters */

#define IS_PRINTABLE(c) (c_table[c] & C_PRINT)
#define IS_NAME(c)      (c_table[c] & C_NAME)
#define IS_COMMON(c)    (c_table[c] & C_NORM)
#define IS_VALID(c)     (c_table[c] & C_ALL)
#define IS_NUM(c)       (c_table[c] & C_NUM)
#define IS_HEX(c)       (c_table[c] & C_HEX)
#define IS_PATH(c)      (c_table[c] & C_PATH)

static const UBYTE c_table [256] =
  {
  0,                                                          /* $0 EOS */
  0,                                                          /* $1 */
  0,                                                          /* $2 */
  0,                                                          /* $3 */
  0,                                                          /* $4 */
  0,                                                          /* $5 */
  0,                                                          /* $6 */
  C_ALL,                                                      /* $7 bell */
  C_ALL,                                                      /* $8 BS */
  C_ALL | C_NORM,                                             /* $9 TAB */
  C_ALL | C_NORM,                                             /* $A LF */
  0,                                                          /* $B */
  C_ALL,                                                      /* $C FF */
  C_ALL | C_NORM,                                             /* $D CR */
  0,                                                          /* $E */
  0,                                                          /* $F */

  0,                                                          /* $10 */
  0,                                                          /* $11 */
  0,                                                          /* $12 */
  0,                                                          /* $13 */
  0,                                                          /* $14 */
  0,                                                          /* $15 */
  0,                                                          /* $16 */
  0,                                                          /* $17 */
  0,                                                          /* $18 */
  0,                                                          /* $19 */
  0,                                                          /* $1A */
  C_ALL,                                                      /* $1B ESC */
  0,                                                          /* $1C */
  0,                                                          /* $1D */
  0,                                                          /* $1E */
  0,                                                          /* $1F */

  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $20 SPACE */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $21 ! */
  C_ALL | C_PRINT | C_NORM,                                   /* $22 " */
  C_ALL | C_PRINT | C_NORM,                                   /* $23 # */
  C_ALL | C_PRINT | C_NORM,                                   /* $24 $ */
  C_ALL | C_PRINT | C_NORM,                                   /* $25 % */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $26 & */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $27 ' */
  C_ALL | C_PRINT | C_NORM,                                   /* $28 ( */
  C_ALL | C_PRINT | C_NORM,                                   /* $29 ) */
  C_ALL | C_PRINT | C_NORM,                                   /* $2A * */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $2B + */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $2C , */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $2D - */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $2E . */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $2F / */

  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $30 0 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $31 1 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $32 2 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $33 3 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $34 4 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $35 5 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $36 6 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $37 7 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $38 8 */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_NUM | C_HEX | C_PATH, /* $39 9 */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $3A : */
  C_ALL | C_PRINT | C_NORM,                                   /* $3B ; */
  C_ALL | C_PRINT | C_NORM,                                   /* $3C < */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $3D = */
  C_ALL | C_PRINT | C_NORM,                                   /* $3E > */
  C_ALL | C_PRINT | C_NORM,                                   /* $3F ? */

  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $40 @ */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $41 A */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $42 B */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $43 C */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $44 D */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $45 E */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $46 F */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $47 G */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $48 H */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $49 I */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $4A J */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $4B K */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $4C L */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $4D M */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $4E N */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $4F O */

  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $50 P */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $51 Q */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $52 R */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $53 S */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $54 T */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $55 U */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $56 V */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $57 W */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $58 X */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $59 Y */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $5A Z */
  C_ALL | C_PRINT | C_NORM,                                   /* $5B [ */
  C_ALL | C_PRINT | C_NORM,                                   /* $5C \ */
  C_ALL | C_PRINT | C_NORM,                                   /* $5D ] */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $5E ^ */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $5F _ */

  C_ALL | C_PRINT | C_NORM,                                   /* $60 ` */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $61 a */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $62 b */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $63 c */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $64 d */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $65 e */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_HEX | C_PATH,         /* $66 f */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $67 g */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $68 h */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $69 i */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $6A j */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $6B k */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $6C l */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $6D m */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $6E n */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $6F o */

  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $70 p */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $71 q */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $72 r */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $73 s */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $74 t */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $75 u */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $76 v */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $77 w */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $78 x */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $79 y */
  C_ALL | C_PRINT | C_NAME | C_NORM | C_PATH,                 /* $7A z */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $7B { */
  C_ALL | C_PRINT | C_NORM,                                   /* $7C | */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $7D } */
  C_ALL | C_PRINT | C_NORM,                                   /* $7E ~ */
  C_ALL,                                                      /* $7F DEL */

  0,                                                          /* $80 */
  0,                                                          /* $81 */
  0,                                                          /* $82 */
  0,                                                          /* $83 */
  0,                                                          /* $84 */
  0,                                                          /* $85 */
  0,                                                          /* $86 */
  0,                                                          /* $87 */
  0,                                                          /* $88 */
  0,                                                          /* $89 */
  0,                                                          /* $8A */
  0,                                                          /* $8B */
  0,                                                          /* $8C */
  0,                                                          /* $8D */
  0,                                                          /* $8E */
  0,                                                          /* $8F */

  0,                                                          /* $90 */
  0,                                                          /* $91 */
  0,                                                          /* $92 */
  0,                                                          /* $93 */
  0,                                                          /* $94 */
  0,                                                          /* $95 */
  0,                                                          /* $96 */
  0,                                                          /* $97 */
  0,                                                          /* $98 */
  0,                                                          /* $99 */
  0,                                                          /* $9A */
  0,                                                          /* $9B */
  0,                                                          /* $9C */
  0,                                                          /* $9D */
  0,                                                          /* $9E */
  0,                                                          /* $9F */

  0,                                                          /* $A0 */
  0,                                                          /* $A1 */
  0,                                                          /* $A2 */
  0,                                                          /* $A3 */
  0,                                                          /* $A4 */
  0,                                                          /* $A5 */
  0,                                                          /* $A6 */
  0,                                                          /* $A7 */
  0,                                                          /* $A8 */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $A9 © */
  0,                                                          /* $AA */
  0,                                                          /* $AB */
  0,                                                          /* $AC */
  0,                                                          /* $AD */
  0,                                                          /* $AE */
  0,                                                          /* $AF */

  0,                                                          /* $B0 */
  0,                                                          /* $B1 */
  0,                                                          /* $B2 */
  0,                                                          /* $B3 */
  0,                                                          /* $B4 */
  0,                                                          /* $B5 */
  0,                                                          /* $B6 */
  0,                                                          /* $B7 */
  0,                                                          /* $B8 */
  0,                                                          /* $B9 */
  0,                                                          /* $BA */
  0,                                                          /* $BB */
  0,                                                          /* $BC */
  0,                                                          /* $BD */
  0,                                                          /* $BE */
  0,                                                          /* $BF */

  0,                                                          /* $C0 */
  0,                                                          /* $C1 */
  0,                                                          /* $C2 */
  0,                                                          /* $C3 */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $C4 Ä */
  0,                                                          /* $C5 */
  0,                                                          /* $C6 */
  0,                                                          /* $C7 */
  0,                                                          /* $C8 */
  0,                                                          /* $C9 */
  0,                                                          /* $CA */
  0,                                                          /* $CB */
  0,                                                          /* $CC */
  0,                                                          /* $CD */
  0,                                                          /* $CE */
  0,                                                          /* $CF */

  0,                                                          /* $D0 */
  0,                                                          /* $D1 */
  0,                                                          /* $D2 */
  0,                                                          /* $D3 */
  0,                                                          /* $D4 */
  0,                                                          /* $D5 */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $D6 Ö */
  0,                                                          /* $D7 */
  0,                                                          /* $D8 */
  0,                                                          /* $D9 */
  0,                                                          /* $DA */
  0,                                                          /* $DB */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $DC Ü */
  0,                                                          /* $DD */
  0,                                                          /* $DE */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $DF ß */

  0,                                                          /* $E0 */
  0,                                                          /* $E1 */
  0,                                                          /* $E2 */
  0,                                                          /* $E3 */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $E4 ä */
  0,                                                          /* $E5 */
  0,                                                          /* $E6 */
  0,                                                          /* $E7 */
  0,                                                          /* $E8 */
  0,                                                          /* $E9 */
  0,                                                          /* $EA */
  0,                                                          /* $EB */
  0,                                                          /* $EC */
  0,                                                          /* $ED */
  0,                                                          /* $EE */
  0,                                                          /* $EF */

  0,                                                          /* $F0 */
  0,                                                          /* $F1 */
  0,                                                          /* $F2 */
  0,                                                          /* $F3 */
  0,                                                          /* $F4 */
  0,                                                          /* $F5 */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $F6 ö */
  0,                                                          /* $F7 */
  0,                                                          /* $F8 */
  0,                                                          /* $F9 */
  0,                                                          /* $FA */
  0,                                                          /* $FB */
  C_ALL | C_PRINT | C_NORM | C_PATH,                          /* $FC ü */
  0,                                                          /* $FD */
  0,                                                          /* $FE */
  0                                                           /* $FF */
  };
