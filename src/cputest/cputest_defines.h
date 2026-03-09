
#define DATA_VERSION 24

#define CT_FPREG 0
#define CT_DREG 0
#define CT_AREG 8
#define CT_SSP 16
#define CT_MSP 17
#define CT_SR 18
#define CT_PC 19
#define CT_FPIAR 20
#define CT_FPSR 21
#define CT_FPCR 22
#define CT_EDATA 23
#define CT_CYCLES 25
#define CT_ENDPC 26
#define CT_BRANCHTARGET 27
#define CT_SRCADDR 28
#define CT_DSTADDR 29
#define CT_MEMWRITE 30
#define CT_MEMWRITES 31
#define CT_DATA_MASK 31
#define CT_EXCEPTION_MASK 63

#define CT_EDATA_IRQ_CYCLES 1

#define CT_SIZE_BYTE (0 << 5)
#define CT_SIZE_WORD (1 << 5)
#define CT_SIZE_LONG (2 << 5)
#define CT_SIZE_FPU (3 << 5) // CT_DREG -> CT_FPREG
#define CT_SIZE_MASK (3 << 5)

// if MEMWRITE or PC
#define CT_RELATIVE_START_WORD (0 << 5) // word
#define CT_ABSOLUTE_WORD (1 << 5)
#define CT_ABSOLUTE_LONG (2 << 5)
// if MEMWRITES
#define CT_PC_BYTES (3 << 5)
// if PC
#define CT_RELATIVE_START_BYTE (3 << 5)

#define CT_END 0x80
#define CT_END_FINISH 0xff
#define CT_END_INIT (0x80 | 0x40)
#define CT_END_SKIP (0x80 | 0x40 | 0x01)
#define CT_SKIP_REGS (0x80 | 0x40 | 0x02)
#define CT_EMPTY CT_END_INIT
#define CT_OVERRIDE_REG (0x80 | 0x40 | 0x10)
#define CT_BRANCHED 0x40

#define OPCODE_AREA 48
#define BRANCHTARGET_AREA 4
#define LM_BUFFER 128

// MOVEA.L A0,A0
// not NOP because on 68040 NOP generates T0 trace.
#define NOP_OPCODE 0x2048
#define REAL_NOP_OPCODE 0x4e71
#define ILLG_OPCODE 0x4afc
#define LM_OPCODE 0x42db

#define IPL_TEST_NOP1 0x4e71
#define IPL_TEST_NOP2 0x4e71

#define IPL_TRIGGER_ADDR 0xdff030
#define IPL_TRIGGER_ADDR_SIZE 2
#define IPL_TRIGGER_DATA (0x100 | 0xCA)
#define IPL_TRIGGER_SERPER 10
#define INTERRUPT_CYCLES ((((IPL_TRIGGER_SERPER + 1) * 9) + (((IPL_TRIGGER_SERPER + 1) - 1) / 2) + 1 + 3 + 9 + 0) * 2)
#define IPL_TRIGGER_INTMASK 0x0800
#define IPL_TEST_IPL_LEVEL 5
#define IPL_BLTSIZE 0xdff058
