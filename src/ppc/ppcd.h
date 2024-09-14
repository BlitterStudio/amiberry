#pragma once

/*
 * General Data Types.
*/

typedef signed   char       s8;
typedef signed   short      s16;
typedef signed   int        s32;
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef float               f32;
typedef double              f64;

typedef unsigned long long  u64;
typedef signed   long long  s64;

#ifndef __cplusplus
typedef enum { false = 0, true } bool;
#endif

#define FASTCALL    __fastcall
#define INLINE      __inline

// See some documentation in CPP file.

// Instruction class
#define PPC_DISA_OTHER      0x0000  // No additional information
#define PPC_DISA_64         0x0001  // 64-bit architecture only
#define PPC_DISA_INTEGER    0x0002  // Integer-type instruction
#define PPC_DISA_BRANCH     0x0004  // Branch instruction
#define PPC_DISA_LDST       0x0008  // Load-store instruction
#define PPC_DISA_STRING     0x0010  // Load-store string/multiple
#define PPC_DISA_FPU        0x0020  // Floating-point instruction
#define PPC_DISA_OEA        0x0040  // Supervisor level
#define PPC_DISA_OPTIONAL   0x0200  // Optional
#define PPC_DISA_BRIDGE     0x0400  // Optional 64-bit bridge
#define PPC_DISA_SPECIFIC   0x0800  // Implementation-specific
#define PPC_DISA_ILLEGAL    0x1000  // Illegal
#define PPC_DISA_SIMPLIFIED 0x8000  // Simplified mnemonic is used

typedef struct PPCD_CB
{
    u64     pc;                     // Program counter (input)
    u32     instr;                  // Instruction (input)
    char    mnemonic[16];           // Instruction mnemonic.
    char    operands[64];           // Instruction operands.
    u32     immed;                  // Immediate value (displacement for load/store, immediate operand for arithm./logic).
    int     r[4];                   // Index value for operand registers and immediates.
    u64     target;                 // Target address for branch instructions / Mask for RLWINM-like instructions
    int     iclass;                 // One or combination of PPC_DISA_* flags.
} PPCD_CB;

void    PPCDisasm(PPCD_CB *disa);
char*   PPCDisasmSimple(u64 pc, u32 instr);
