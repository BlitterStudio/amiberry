#ifdef CPUEMU_90
#include "cputest.h"
const struct cputbl CPUFUNC(op_smalltbl_90_test)[] = {
{ NULL, op_0000_90_test_ff, 0x0000, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0010_90_test_ff, 0x0010, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0018_90_test_ff, 0x0018, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0020_90_test_ff, 0x0020, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0028_90_test_ff, 0x0028, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0030_90_test_ff, 0x0030, 6, { 4, 0 }, 0 }, /* OR */
{ NULL, op_0038_90_test_ff, 0x0038, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0039_90_test_ff, 0x0039, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_003c_90_test_ff, 0x003c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0040_90_test_ff, 0x0040, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0050_90_test_ff, 0x0050, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0058_90_test_ff, 0x0058, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0060_90_test_ff, 0x0060, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0068_90_test_ff, 0x0068, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0070_90_test_ff, 0x0070, 6, { 4, 0 }, 0 }, /* OR */
{ NULL, op_0078_90_test_ff, 0x0078, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0079_90_test_ff, 0x0079, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_007c_90_test_ff, 0x007c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0080_90_test_ff, 0x0080, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0090_90_test_ff, 0x0090, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0098_90_test_ff, 0x0098, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a0_90_test_ff, 0x00a0, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a8_90_test_ff, 0x00a8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b0_90_test_ff, 0x00b0, 8, { 4, 0 }, 0 }, /* OR */
{ NULL, op_00b8_90_test_ff, 0x00b8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b9_90_test_ff, 0x00b9, 10, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0100_90_test_ff, 0x0100, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0108_90_test_ff, 0x0108, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0110_90_test_ff, 0x0110, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0118_90_test_ff, 0x0118, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0120_90_test_ff, 0x0120, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0128_90_test_ff, 0x0128, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0130_90_test_ff, 0x0130, 4, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_0138_90_test_ff, 0x0138, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0139_90_test_ff, 0x0139, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013a_90_test_ff, 0x013a, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013b_90_test_ff, 0x013b, 4, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_013c_90_test_ff, 0x013c, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0140_90_test_ff, 0x0140, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0148_90_test_ff, 0x0148, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0150_90_test_ff, 0x0150, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0158_90_test_ff, 0x0158, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0160_90_test_ff, 0x0160, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0168_90_test_ff, 0x0168, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0170_90_test_ff, 0x0170, 4, { 4, 0 }, 0 }, /* BCHG */
{ NULL, op_0178_90_test_ff, 0x0178, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0179_90_test_ff, 0x0179, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0180_90_test_ff, 0x0180, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0188_90_test_ff, 0x0188, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_0190_90_test_ff, 0x0190, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0198_90_test_ff, 0x0198, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a0_90_test_ff, 0x01a0, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a8_90_test_ff, 0x01a8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b0_90_test_ff, 0x01b0, 4, { 4, 0 }, 0 }, /* BCLR */
{ NULL, op_01b8_90_test_ff, 0x01b8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b9_90_test_ff, 0x01b9, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01c0_90_test_ff, 0x01c0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01c8_90_test_ff, 0x01c8, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_01d0_90_test_ff, 0x01d0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01d8_90_test_ff, 0x01d8, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e0_90_test_ff, 0x01e0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e8_90_test_ff, 0x01e8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f0_90_test_ff, 0x01f0, 4, { 4, 0 }, 0 }, /* BSET */
{ NULL, op_01f8_90_test_ff, 0x01f8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f9_90_test_ff, 0x01f9, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0200_90_test_ff, 0x0200, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0210_90_test_ff, 0x0210, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0218_90_test_ff, 0x0218, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0220_90_test_ff, 0x0220, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0228_90_test_ff, 0x0228, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0230_90_test_ff, 0x0230, 6, { 4, 0 }, 0 }, /* AND */
{ NULL, op_0238_90_test_ff, 0x0238, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0239_90_test_ff, 0x0239, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_023c_90_test_ff, 0x023c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0240_90_test_ff, 0x0240, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0250_90_test_ff, 0x0250, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0258_90_test_ff, 0x0258, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0260_90_test_ff, 0x0260, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0268_90_test_ff, 0x0268, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0270_90_test_ff, 0x0270, 6, { 4, 0 }, 0 }, /* AND */
{ NULL, op_0278_90_test_ff, 0x0278, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0279_90_test_ff, 0x0279, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_027c_90_test_ff, 0x027c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0280_90_test_ff, 0x0280, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0290_90_test_ff, 0x0290, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0298_90_test_ff, 0x0298, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a0_90_test_ff, 0x02a0, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a8_90_test_ff, 0x02a8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b0_90_test_ff, 0x02b0, 8, { 4, 0 }, 0 }, /* AND */
{ NULL, op_02b8_90_test_ff, 0x02b8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b9_90_test_ff, 0x02b9, 10, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0400_90_test_ff, 0x0400, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0410_90_test_ff, 0x0410, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0418_90_test_ff, 0x0418, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0420_90_test_ff, 0x0420, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0428_90_test_ff, 0x0428, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0430_90_test_ff, 0x0430, 6, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_0438_90_test_ff, 0x0438, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0439_90_test_ff, 0x0439, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0440_90_test_ff, 0x0440, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0450_90_test_ff, 0x0450, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0458_90_test_ff, 0x0458, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0460_90_test_ff, 0x0460, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0468_90_test_ff, 0x0468, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0470_90_test_ff, 0x0470, 6, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_0478_90_test_ff, 0x0478, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0479_90_test_ff, 0x0479, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0480_90_test_ff, 0x0480, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0490_90_test_ff, 0x0490, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0498_90_test_ff, 0x0498, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a0_90_test_ff, 0x04a0, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a8_90_test_ff, 0x04a8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b0_90_test_ff, 0x04b0, 8, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_04b8_90_test_ff, 0x04b8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b9_90_test_ff, 0x04b9, 10, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0600_90_test_ff, 0x0600, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0610_90_test_ff, 0x0610, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0618_90_test_ff, 0x0618, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0620_90_test_ff, 0x0620, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0628_90_test_ff, 0x0628, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0630_90_test_ff, 0x0630, 6, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_0638_90_test_ff, 0x0638, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0639_90_test_ff, 0x0639, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0640_90_test_ff, 0x0640, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0650_90_test_ff, 0x0650, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0658_90_test_ff, 0x0658, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0660_90_test_ff, 0x0660, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0668_90_test_ff, 0x0668, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0670_90_test_ff, 0x0670, 6, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_0678_90_test_ff, 0x0678, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0679_90_test_ff, 0x0679, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0680_90_test_ff, 0x0680, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0690_90_test_ff, 0x0690, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0698_90_test_ff, 0x0698, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a0_90_test_ff, 0x06a0, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a8_90_test_ff, 0x06a8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b0_90_test_ff, 0x06b0, 8, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_06b8_90_test_ff, 0x06b8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b9_90_test_ff, 0x06b9, 10, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0800_90_test_ff, 0x0800, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0810_90_test_ff, 0x0810, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0818_90_test_ff, 0x0818, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0820_90_test_ff, 0x0820, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0828_90_test_ff, 0x0828, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0830_90_test_ff, 0x0830, 6, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_0838_90_test_ff, 0x0838, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0839_90_test_ff, 0x0839, 8, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083a_90_test_ff, 0x083a, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083b_90_test_ff, 0x083b, 6, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_0840_90_test_ff, 0x0840, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0850_90_test_ff, 0x0850, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0858_90_test_ff, 0x0858, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0860_90_test_ff, 0x0860, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0868_90_test_ff, 0x0868, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0870_90_test_ff, 0x0870, 6, { 4, 0 }, 0 }, /* BCHG */
{ NULL, op_0878_90_test_ff, 0x0878, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0879_90_test_ff, 0x0879, 8, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0880_90_test_ff, 0x0880, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0890_90_test_ff, 0x0890, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0898_90_test_ff, 0x0898, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a0_90_test_ff, 0x08a0, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a8_90_test_ff, 0x08a8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b0_90_test_ff, 0x08b0, 6, { 4, 0 }, 0 }, /* BCLR */
{ NULL, op_08b8_90_test_ff, 0x08b8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b9_90_test_ff, 0x08b9, 8, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08c0_90_test_ff, 0x08c0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d0_90_test_ff, 0x08d0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d8_90_test_ff, 0x08d8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e0_90_test_ff, 0x08e0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e8_90_test_ff, 0x08e8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f0_90_test_ff, 0x08f0, 6, { 4, 0 }, 0 }, /* BSET */
{ NULL, op_08f8_90_test_ff, 0x08f8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f9_90_test_ff, 0x08f9, 8, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0a00_90_test_ff, 0x0a00, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a10_90_test_ff, 0x0a10, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a18_90_test_ff, 0x0a18, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a20_90_test_ff, 0x0a20, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a28_90_test_ff, 0x0a28, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a30_90_test_ff, 0x0a30, 6, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_0a38_90_test_ff, 0x0a38, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a39_90_test_ff, 0x0a39, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a3c_90_test_ff, 0x0a3c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a40_90_test_ff, 0x0a40, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a50_90_test_ff, 0x0a50, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a58_90_test_ff, 0x0a58, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a60_90_test_ff, 0x0a60, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a68_90_test_ff, 0x0a68, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a70_90_test_ff, 0x0a70, 6, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_0a78_90_test_ff, 0x0a78, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a79_90_test_ff, 0x0a79, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a7c_90_test_ff, 0x0a7c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a80_90_test_ff, 0x0a80, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a90_90_test_ff, 0x0a90, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a98_90_test_ff, 0x0a98, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa0_90_test_ff, 0x0aa0, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa8_90_test_ff, 0x0aa8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab0_90_test_ff, 0x0ab0, 8, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_0ab8_90_test_ff, 0x0ab8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab9_90_test_ff, 0x0ab9, 10, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0c00_90_test_ff, 0x0c00, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c10_90_test_ff, 0x0c10, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c18_90_test_ff, 0x0c18, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c20_90_test_ff, 0x0c20, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c28_90_test_ff, 0x0c28, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c30_90_test_ff, 0x0c30, 6, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_0c38_90_test_ff, 0x0c38, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c39_90_test_ff, 0x0c39, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c40_90_test_ff, 0x0c40, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c50_90_test_ff, 0x0c50, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c58_90_test_ff, 0x0c58, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c60_90_test_ff, 0x0c60, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c68_90_test_ff, 0x0c68, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c70_90_test_ff, 0x0c70, 6, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_0c78_90_test_ff, 0x0c78, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c79_90_test_ff, 0x0c79, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c80_90_test_ff, 0x0c80, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c90_90_test_ff, 0x0c90, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c98_90_test_ff, 0x0c98, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca0_90_test_ff, 0x0ca0, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca8_90_test_ff, 0x0ca8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb0_90_test_ff, 0x0cb0, 8, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_0cb8_90_test_ff, 0x0cb8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb9_90_test_ff, 0x0cb9, 10, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_1000_90_test_ff, 0x1000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1010_90_test_ff, 0x1010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1018_90_test_ff, 0x1018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1020_90_test_ff, 0x1020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1028_90_test_ff, 0x1028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1030_90_test_ff, 0x1030, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1038_90_test_ff, 0x1038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1039_90_test_ff, 0x1039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103a_90_test_ff, 0x103a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103b_90_test_ff, 0x103b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_103c_90_test_ff, 0x103c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1080_90_test_ff, 0x1080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1090_90_test_ff, 0x1090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1098_90_test_ff, 0x1098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a0_90_test_ff, 0x10a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a8_90_test_ff, 0x10a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b0_90_test_ff, 0x10b0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10b8_90_test_ff, 0x10b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b9_90_test_ff, 0x10b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10ba_90_test_ff, 0x10ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10bb_90_test_ff, 0x10bb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10bc_90_test_ff, 0x10bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10c0_90_test_ff, 0x10c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d0_90_test_ff, 0x10d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d8_90_test_ff, 0x10d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e0_90_test_ff, 0x10e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e8_90_test_ff, 0x10e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f0_90_test_ff, 0x10f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10f8_90_test_ff, 0x10f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f9_90_test_ff, 0x10f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fa_90_test_ff, 0x10fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fb_90_test_ff, 0x10fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10fc_90_test_ff, 0x10fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1100_90_test_ff, 0x1100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1110_90_test_ff, 0x1110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1118_90_test_ff, 0x1118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1120_90_test_ff, 0x1120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1128_90_test_ff, 0x1128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1130_90_test_ff, 0x1130, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1138_90_test_ff, 0x1138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1139_90_test_ff, 0x1139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113a_90_test_ff, 0x113a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113b_90_test_ff, 0x113b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_113c_90_test_ff, 0x113c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1140_90_test_ff, 0x1140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1150_90_test_ff, 0x1150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1158_90_test_ff, 0x1158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1160_90_test_ff, 0x1160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1168_90_test_ff, 0x1168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1170_90_test_ff, 0x1170, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_1178_90_test_ff, 0x1178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1179_90_test_ff, 0x1179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117a_90_test_ff, 0x117a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117b_90_test_ff, 0x117b, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_117c_90_test_ff, 0x117c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1180_90_test_ff, 0x1180, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1190_90_test_ff, 0x1190, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1198_90_test_ff, 0x1198, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11a0_90_test_ff, 0x11a0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11a8_90_test_ff, 0x11a8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11b0_90_test_ff, 0x11b0, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_11b8_90_test_ff, 0x11b8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11b9_90_test_ff, 0x11b9, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11ba_90_test_ff, 0x11ba, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11bb_90_test_ff, 0x11bb, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_11bc_90_test_ff, 0x11bc, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11c0_90_test_ff, 0x11c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d0_90_test_ff, 0x11d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d8_90_test_ff, 0x11d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e0_90_test_ff, 0x11e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e8_90_test_ff, 0x11e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f0_90_test_ff, 0x11f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_11f8_90_test_ff, 0x11f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f9_90_test_ff, 0x11f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fa_90_test_ff, 0x11fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fb_90_test_ff, 0x11fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_11fc_90_test_ff, 0x11fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13c0_90_test_ff, 0x13c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d0_90_test_ff, 0x13d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d8_90_test_ff, 0x13d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e0_90_test_ff, 0x13e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e8_90_test_ff, 0x13e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f0_90_test_ff, 0x13f0, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_13f8_90_test_ff, 0x13f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f9_90_test_ff, 0x13f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fa_90_test_ff, 0x13fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fb_90_test_ff, 0x13fb, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_13fc_90_test_ff, 0x13fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2000_90_test_ff, 0x2000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2008_90_test_ff, 0x2008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2010_90_test_ff, 0x2010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2018_90_test_ff, 0x2018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2020_90_test_ff, 0x2020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2028_90_test_ff, 0x2028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2030_90_test_ff, 0x2030, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2038_90_test_ff, 0x2038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2039_90_test_ff, 0x2039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203a_90_test_ff, 0x203a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203b_90_test_ff, 0x203b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_203c_90_test_ff, 0x203c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2040_90_test_ff, 0x2040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2048_90_test_ff, 0x2048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2050_90_test_ff, 0x2050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2058_90_test_ff, 0x2058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2060_90_test_ff, 0x2060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2068_90_test_ff, 0x2068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2070_90_test_ff, 0x2070, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_2078_90_test_ff, 0x2078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2079_90_test_ff, 0x2079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207a_90_test_ff, 0x207a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207b_90_test_ff, 0x207b, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_207c_90_test_ff, 0x207c, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2080_90_test_ff, 0x2080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2088_90_test_ff, 0x2088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2090_90_test_ff, 0x2090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2098_90_test_ff, 0x2098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a0_90_test_ff, 0x20a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a8_90_test_ff, 0x20a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b0_90_test_ff, 0x20b0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20b8_90_test_ff, 0x20b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b9_90_test_ff, 0x20b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20ba_90_test_ff, 0x20ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20bb_90_test_ff, 0x20bb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20bc_90_test_ff, 0x20bc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c0_90_test_ff, 0x20c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c8_90_test_ff, 0x20c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d0_90_test_ff, 0x20d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d8_90_test_ff, 0x20d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e0_90_test_ff, 0x20e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e8_90_test_ff, 0x20e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f0_90_test_ff, 0x20f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20f8_90_test_ff, 0x20f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f9_90_test_ff, 0x20f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fa_90_test_ff, 0x20fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fb_90_test_ff, 0x20fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20fc_90_test_ff, 0x20fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2100_90_test_ff, 0x2100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2108_90_test_ff, 0x2108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2110_90_test_ff, 0x2110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2118_90_test_ff, 0x2118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2120_90_test_ff, 0x2120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2128_90_test_ff, 0x2128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2130_90_test_ff, 0x2130, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2138_90_test_ff, 0x2138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2139_90_test_ff, 0x2139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213a_90_test_ff, 0x213a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213b_90_test_ff, 0x213b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_213c_90_test_ff, 0x213c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2140_90_test_ff, 0x2140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2148_90_test_ff, 0x2148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2150_90_test_ff, 0x2150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2158_90_test_ff, 0x2158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2160_90_test_ff, 0x2160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2168_90_test_ff, 0x2168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2170_90_test_ff, 0x2170, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_2178_90_test_ff, 0x2178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2179_90_test_ff, 0x2179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217a_90_test_ff, 0x217a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217b_90_test_ff, 0x217b, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_217c_90_test_ff, 0x217c, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2180_90_test_ff, 0x2180, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2188_90_test_ff, 0x2188, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2190_90_test_ff, 0x2190, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2198_90_test_ff, 0x2198, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21a0_90_test_ff, 0x21a0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21a8_90_test_ff, 0x21a8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21b0_90_test_ff, 0x21b0, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_21b8_90_test_ff, 0x21b8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21b9_90_test_ff, 0x21b9, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21ba_90_test_ff, 0x21ba, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21bb_90_test_ff, 0x21bb, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_21bc_90_test_ff, 0x21bc, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21c0_90_test_ff, 0x21c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21c8_90_test_ff, 0x21c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d0_90_test_ff, 0x21d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d8_90_test_ff, 0x21d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e0_90_test_ff, 0x21e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e8_90_test_ff, 0x21e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f0_90_test_ff, 0x21f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_21f8_90_test_ff, 0x21f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f9_90_test_ff, 0x21f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fa_90_test_ff, 0x21fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fb_90_test_ff, 0x21fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_21fc_90_test_ff, 0x21fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c0_90_test_ff, 0x23c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c8_90_test_ff, 0x23c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d0_90_test_ff, 0x23d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d8_90_test_ff, 0x23d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e0_90_test_ff, 0x23e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e8_90_test_ff, 0x23e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f0_90_test_ff, 0x23f0, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_23f8_90_test_ff, 0x23f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f9_90_test_ff, 0x23f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fa_90_test_ff, 0x23fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fb_90_test_ff, 0x23fb, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_23fc_90_test_ff, 0x23fc, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3000_90_test_ff, 0x3000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3008_90_test_ff, 0x3008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3010_90_test_ff, 0x3010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3018_90_test_ff, 0x3018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3020_90_test_ff, 0x3020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3028_90_test_ff, 0x3028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3030_90_test_ff, 0x3030, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3038_90_test_ff, 0x3038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3039_90_test_ff, 0x3039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303a_90_test_ff, 0x303a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303b_90_test_ff, 0x303b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_303c_90_test_ff, 0x303c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3040_90_test_ff, 0x3040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3048_90_test_ff, 0x3048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3050_90_test_ff, 0x3050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3058_90_test_ff, 0x3058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3060_90_test_ff, 0x3060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3068_90_test_ff, 0x3068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3070_90_test_ff, 0x3070, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_3078_90_test_ff, 0x3078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3079_90_test_ff, 0x3079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307a_90_test_ff, 0x307a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307b_90_test_ff, 0x307b, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_307c_90_test_ff, 0x307c, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3080_90_test_ff, 0x3080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3088_90_test_ff, 0x3088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3090_90_test_ff, 0x3090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3098_90_test_ff, 0x3098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a0_90_test_ff, 0x30a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a8_90_test_ff, 0x30a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b0_90_test_ff, 0x30b0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30b8_90_test_ff, 0x30b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b9_90_test_ff, 0x30b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30ba_90_test_ff, 0x30ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30bb_90_test_ff, 0x30bb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30bc_90_test_ff, 0x30bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c0_90_test_ff, 0x30c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c8_90_test_ff, 0x30c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d0_90_test_ff, 0x30d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d8_90_test_ff, 0x30d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e0_90_test_ff, 0x30e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e8_90_test_ff, 0x30e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f0_90_test_ff, 0x30f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30f8_90_test_ff, 0x30f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f9_90_test_ff, 0x30f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fa_90_test_ff, 0x30fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fb_90_test_ff, 0x30fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30fc_90_test_ff, 0x30fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3100_90_test_ff, 0x3100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3108_90_test_ff, 0x3108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3110_90_test_ff, 0x3110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3118_90_test_ff, 0x3118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3120_90_test_ff, 0x3120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3128_90_test_ff, 0x3128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3130_90_test_ff, 0x3130, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3138_90_test_ff, 0x3138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3139_90_test_ff, 0x3139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313a_90_test_ff, 0x313a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313b_90_test_ff, 0x313b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_313c_90_test_ff, 0x313c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3140_90_test_ff, 0x3140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3148_90_test_ff, 0x3148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3150_90_test_ff, 0x3150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3158_90_test_ff, 0x3158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3160_90_test_ff, 0x3160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3168_90_test_ff, 0x3168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3170_90_test_ff, 0x3170, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_3178_90_test_ff, 0x3178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3179_90_test_ff, 0x3179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317a_90_test_ff, 0x317a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317b_90_test_ff, 0x317b, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_317c_90_test_ff, 0x317c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3180_90_test_ff, 0x3180, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3188_90_test_ff, 0x3188, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3190_90_test_ff, 0x3190, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3198_90_test_ff, 0x3198, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31a0_90_test_ff, 0x31a0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31a8_90_test_ff, 0x31a8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31b0_90_test_ff, 0x31b0, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_31b8_90_test_ff, 0x31b8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31b9_90_test_ff, 0x31b9, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31ba_90_test_ff, 0x31ba, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31bb_90_test_ff, 0x31bb, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_31bc_90_test_ff, 0x31bc, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31c0_90_test_ff, 0x31c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31c8_90_test_ff, 0x31c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d0_90_test_ff, 0x31d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d8_90_test_ff, 0x31d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e0_90_test_ff, 0x31e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e8_90_test_ff, 0x31e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f0_90_test_ff, 0x31f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_31f8_90_test_ff, 0x31f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f9_90_test_ff, 0x31f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fa_90_test_ff, 0x31fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fb_90_test_ff, 0x31fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_31fc_90_test_ff, 0x31fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c0_90_test_ff, 0x33c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c8_90_test_ff, 0x33c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d0_90_test_ff, 0x33d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d8_90_test_ff, 0x33d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e0_90_test_ff, 0x33e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e8_90_test_ff, 0x33e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f0_90_test_ff, 0x33f0, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_33f8_90_test_ff, 0x33f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f9_90_test_ff, 0x33f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fa_90_test_ff, 0x33fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fb_90_test_ff, 0x33fb, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_33fc_90_test_ff, 0x33fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_4000_90_test_ff, 0x4000, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4010_90_test_ff, 0x4010, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4018_90_test_ff, 0x4018, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4020_90_test_ff, 0x4020, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4028_90_test_ff, 0x4028, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4030_90_test_ff, 0x4030, 4, { 4, 0 }, 0 }, /* NEGX */
{ NULL, op_4038_90_test_ff, 0x4038, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4039_90_test_ff, 0x4039, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4040_90_test_ff, 0x4040, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4050_90_test_ff, 0x4050, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4058_90_test_ff, 0x4058, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4060_90_test_ff, 0x4060, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4068_90_test_ff, 0x4068, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4070_90_test_ff, 0x4070, 4, { 4, 0 }, 0 }, /* NEGX */
{ NULL, op_4078_90_test_ff, 0x4078, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4079_90_test_ff, 0x4079, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4080_90_test_ff, 0x4080, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4090_90_test_ff, 0x4090, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4098_90_test_ff, 0x4098, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a0_90_test_ff, 0x40a0, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a8_90_test_ff, 0x40a8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b0_90_test_ff, 0x40b0, 4, { 4, 0 }, 0 }, /* NEGX */
{ NULL, op_40b8_90_test_ff, 0x40b8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b9_90_test_ff, 0x40b9, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40c0_90_test_ff, 0x40c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d0_90_test_ff, 0x40d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d8_90_test_ff, 0x40d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e0_90_test_ff, 0x40e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e8_90_test_ff, 0x40e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f0_90_test_ff, 0x40f0, 4, { 4, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f8_90_test_ff, 0x40f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f9_90_test_ff, 0x40f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_4180_90_test_ff, 0x4180, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4190_90_test_ff, 0x4190, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4198_90_test_ff, 0x4198, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a0_90_test_ff, 0x41a0, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a8_90_test_ff, 0x41a8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b0_90_test_ff, 0x41b0, 4, { 4, 0 }, 0 }, /* CHK */
{ NULL, op_41b8_90_test_ff, 0x41b8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b9_90_test_ff, 0x41b9, 6, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41ba_90_test_ff, 0x41ba, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41bb_90_test_ff, 0x41bb, 4, { 4, 0 }, 0 }, /* CHK */
{ NULL, op_41bc_90_test_ff, 0x41bc, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41d0_90_test_ff, 0x41d0, 2, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41e8_90_test_ff, 0x41e8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f0_90_test_ff, 0x41f0, 4, { 4, 0 }, 0 }, /* LEA */
{ NULL, op_41f8_90_test_ff, 0x41f8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f9_90_test_ff, 0x41f9, 6, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fa_90_test_ff, 0x41fa, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fb_90_test_ff, 0x41fb, 4, { 4, 0 }, 0 }, /* LEA */
{ NULL, op_4200_90_test_ff, 0x4200, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4210_90_test_ff, 0x4210, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4218_90_test_ff, 0x4218, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4220_90_test_ff, 0x4220, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4228_90_test_ff, 0x4228, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4230_90_test_ff, 0x4230, 4, { 4, 0 }, 0 }, /* CLR */
{ NULL, op_4238_90_test_ff, 0x4238, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4239_90_test_ff, 0x4239, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4240_90_test_ff, 0x4240, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4250_90_test_ff, 0x4250, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4258_90_test_ff, 0x4258, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4260_90_test_ff, 0x4260, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4268_90_test_ff, 0x4268, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4270_90_test_ff, 0x4270, 4, { 4, 0 }, 0 }, /* CLR */
{ NULL, op_4278_90_test_ff, 0x4278, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4279_90_test_ff, 0x4279, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4280_90_test_ff, 0x4280, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4290_90_test_ff, 0x4290, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4298_90_test_ff, 0x4298, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a0_90_test_ff, 0x42a0, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a8_90_test_ff, 0x42a8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b0_90_test_ff, 0x42b0, 4, { 4, 0 }, 0 }, /* CLR */
{ NULL, op_42b8_90_test_ff, 0x42b8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b9_90_test_ff, 0x42b9, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4400_90_test_ff, 0x4400, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4410_90_test_ff, 0x4410, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4418_90_test_ff, 0x4418, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4420_90_test_ff, 0x4420, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4428_90_test_ff, 0x4428, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4430_90_test_ff, 0x4430, 4, { 4, 0 }, 0 }, /* NEG */
{ NULL, op_4438_90_test_ff, 0x4438, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4439_90_test_ff, 0x4439, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4440_90_test_ff, 0x4440, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4450_90_test_ff, 0x4450, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4458_90_test_ff, 0x4458, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4460_90_test_ff, 0x4460, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4468_90_test_ff, 0x4468, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4470_90_test_ff, 0x4470, 4, { 4, 0 }, 0 }, /* NEG */
{ NULL, op_4478_90_test_ff, 0x4478, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4479_90_test_ff, 0x4479, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4480_90_test_ff, 0x4480, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4490_90_test_ff, 0x4490, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4498_90_test_ff, 0x4498, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a0_90_test_ff, 0x44a0, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a8_90_test_ff, 0x44a8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b0_90_test_ff, 0x44b0, 4, { 4, 0 }, 0 }, /* NEG */
{ NULL, op_44b8_90_test_ff, 0x44b8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b9_90_test_ff, 0x44b9, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44c0_90_test_ff, 0x44c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d0_90_test_ff, 0x44d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d8_90_test_ff, 0x44d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e0_90_test_ff, 0x44e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e8_90_test_ff, 0x44e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f0_90_test_ff, 0x44f0, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f8_90_test_ff, 0x44f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f9_90_test_ff, 0x44f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fa_90_test_ff, 0x44fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fb_90_test_ff, 0x44fb, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fc_90_test_ff, 0x44fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4600_90_test_ff, 0x4600, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4610_90_test_ff, 0x4610, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4618_90_test_ff, 0x4618, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4620_90_test_ff, 0x4620, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4628_90_test_ff, 0x4628, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4630_90_test_ff, 0x4630, 4, { 4, 0 }, 0 }, /* NOT */
{ NULL, op_4638_90_test_ff, 0x4638, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4639_90_test_ff, 0x4639, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4640_90_test_ff, 0x4640, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4650_90_test_ff, 0x4650, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4658_90_test_ff, 0x4658, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4660_90_test_ff, 0x4660, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4668_90_test_ff, 0x4668, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4670_90_test_ff, 0x4670, 4, { 4, 0 }, 0 }, /* NOT */
{ NULL, op_4678_90_test_ff, 0x4678, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4679_90_test_ff, 0x4679, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4680_90_test_ff, 0x4680, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4690_90_test_ff, 0x4690, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4698_90_test_ff, 0x4698, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a0_90_test_ff, 0x46a0, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a8_90_test_ff, 0x46a8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b0_90_test_ff, 0x46b0, 4, { 4, 0 }, 0 }, /* NOT */
{ NULL, op_46b8_90_test_ff, 0x46b8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b9_90_test_ff, 0x46b9, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46c0_90_test_ff, 0x46c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d0_90_test_ff, 0x46d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d8_90_test_ff, 0x46d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e0_90_test_ff, 0x46e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e8_90_test_ff, 0x46e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f0_90_test_ff, 0x46f0, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f8_90_test_ff, 0x46f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f9_90_test_ff, 0x46f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fa_90_test_ff, 0x46fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fb_90_test_ff, 0x46fb, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fc_90_test_ff, 0x46fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4800_90_test_ff, 0x4800, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4810_90_test_ff, 0x4810, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4818_90_test_ff, 0x4818, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4820_90_test_ff, 0x4820, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4828_90_test_ff, 0x4828, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4830_90_test_ff, 0x4830, 4, { 4, 0 }, 0 }, /* NBCD */
{ NULL, op_4838_90_test_ff, 0x4838, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4839_90_test_ff, 0x4839, 6, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4840_90_test_ff, 0x4840, 2, { 0, 0 }, 0 }, /* SWAP */
{ NULL, op_4850_90_test_ff, 0x4850, 2, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4868_90_test_ff, 0x4868, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4870_90_test_ff, 0x4870, 4, { 4, 0 }, 0 }, /* PEA */
{ NULL, op_4878_90_test_ff, 0x4878, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4879_90_test_ff, 0x4879, 6, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487a_90_test_ff, 0x487a, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487b_90_test_ff, 0x487b, 4, { 4, 0 }, 0 }, /* PEA */
{ NULL, op_4880_90_test_ff, 0x4880, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_4890_90_test_ff, 0x4890, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a0_90_test_ff, 0x48a0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a8_90_test_ff, 0x48a8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b0_90_test_ff, 0x48b0, 6, { 4, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b8_90_test_ff, 0x48b8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b9_90_test_ff, 0x48b9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48c0_90_test_ff, 0x48c0, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_48d0_90_test_ff, 0x48d0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e0_90_test_ff, 0x48e0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e8_90_test_ff, 0x48e8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f0_90_test_ff, 0x48f0, 6, { 4, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f8_90_test_ff, 0x48f8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f9_90_test_ff, 0x48f9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_4a00_90_test_ff, 0x4a00, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a10_90_test_ff, 0x4a10, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a18_90_test_ff, 0x4a18, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a20_90_test_ff, 0x4a20, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a28_90_test_ff, 0x4a28, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a30_90_test_ff, 0x4a30, 4, { 4, 0 }, 0 }, /* TST */
{ NULL, op_4a38_90_test_ff, 0x4a38, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a39_90_test_ff, 0x4a39, 6, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a40_90_test_ff, 0x4a40, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a50_90_test_ff, 0x4a50, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a58_90_test_ff, 0x4a58, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a60_90_test_ff, 0x4a60, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a68_90_test_ff, 0x4a68, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a70_90_test_ff, 0x4a70, 4, { 4, 0 }, 0 }, /* TST */
{ NULL, op_4a78_90_test_ff, 0x4a78, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a79_90_test_ff, 0x4a79, 6, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a80_90_test_ff, 0x4a80, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a90_90_test_ff, 0x4a90, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a98_90_test_ff, 0x4a98, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa0_90_test_ff, 0x4aa0, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa8_90_test_ff, 0x4aa8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab0_90_test_ff, 0x4ab0, 4, { 4, 0 }, 0 }, /* TST */
{ NULL, op_4ab8_90_test_ff, 0x4ab8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab9_90_test_ff, 0x4ab9, 6, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ac0_90_test_ff, 0x4ac0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad0_90_test_ff, 0x4ad0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad8_90_test_ff, 0x4ad8, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae0_90_test_ff, 0x4ae0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae8_90_test_ff, 0x4ae8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af0_90_test_ff, 0x4af0, 4, { 4, 0 }, 0 }, /* TAS */
{ NULL, op_4af8_90_test_ff, 0x4af8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af9_90_test_ff, 0x4af9, 6, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4c90_90_test_ff, 0x4c90, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4c98_90_test_ff, 0x4c98, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ca8_90_test_ff, 0x4ca8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb0_90_test_ff, 0x4cb0, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb8_90_test_ff, 0x4cb8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb9_90_test_ff, 0x4cb9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cba_90_test_ff, 0x4cba, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cbb_90_test_ff, 0x4cbb, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd0_90_test_ff, 0x4cd0, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd8_90_test_ff, 0x4cd8, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ce8_90_test_ff, 0x4ce8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf0_90_test_ff, 0x4cf0, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf8_90_test_ff, 0x4cf8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf9_90_test_ff, 0x4cf9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfa_90_test_ff, 0x4cfa, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfb_90_test_ff, 0x4cfb, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4e40_90_test_ff, 0x4e40, 2, { 0, 0 }, 0 }, /* TRAP */
{ NULL, op_4e50_90_test_ff, 0x4e50, 4, { 0, 0 }, 0 }, /* LINK */
{ NULL, op_4e58_90_test_ff, 0x4e58, 2, { 0, 0 }, 0 }, /* UNLK */
{ NULL, op_4e60_90_test_ff, 0x4e60, 2, { 0, 0 }, 0 }, /* MVR2USP */
{ NULL, op_4e68_90_test_ff, 0x4e68, 2, { 0, 0 }, 0 }, /* MVUSP2R */
{ NULL, op_4e70_90_test_ff, 0x4e70, 2, { 0, 0 }, 0 }, /* RESET */
{ NULL, op_4e71_90_test_ff, 0x4e71, 2, { 0, 0 }, 0 }, /* NOP */
{ NULL, op_4e72_90_test_ff, 0x4e72, 0, { 0, 0 }, 0 }, /* STOP */
{ NULL, op_4e73_90_test_ff, 0x4e73, 2, { 0, 0 }, 1 }, /* RTE */
{ NULL, op_4e75_90_test_ff, 0x4e75, 2, { 0, 0 }, 1 }, /* RTS */
{ NULL, op_4e76_90_test_ff, 0x4e76, 2, { 0, 0 }, 0 }, /* TRAPV */
{ NULL, op_4e77_90_test_ff, 0x4e77, 2, { 0, 0 }, 1 }, /* RTR */
{ NULL, op_4e90_90_test_ff, 0x4e90, 2, { 0, 0 }, 1 }, /* JSR */
{ NULL, op_4ea8_90_test_ff, 0x4ea8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb0_90_test_ff, 0x4eb0, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4eb8_90_test_ff, 0x4eb8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb9_90_test_ff, 0x4eb9, 6, { 0, 0 }, 3 }, /* JSR */
{ NULL, op_4eba_90_test_ff, 0x4eba, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4ebb_90_test_ff, 0x4ebb, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4ed0_90_test_ff, 0x4ed0, 2, { 0, 0 }, 1 }, /* JMP */
{ NULL, op_4ee8_90_test_ff, 0x4ee8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef0_90_test_ff, 0x4ef0, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_4ef8_90_test_ff, 0x4ef8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef9_90_test_ff, 0x4ef9, 6, { 0, 0 }, 3 }, /* JMP */
{ NULL, op_4efa_90_test_ff, 0x4efa, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4efb_90_test_ff, 0x4efb, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_5000_90_test_ff, 0x5000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5010_90_test_ff, 0x5010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5018_90_test_ff, 0x5018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5020_90_test_ff, 0x5020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5028_90_test_ff, 0x5028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5030_90_test_ff, 0x5030, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_5038_90_test_ff, 0x5038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5039_90_test_ff, 0x5039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5040_90_test_ff, 0x5040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5048_90_test_ff, 0x5048, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5050_90_test_ff, 0x5050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5058_90_test_ff, 0x5058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5060_90_test_ff, 0x5060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5068_90_test_ff, 0x5068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5070_90_test_ff, 0x5070, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_5078_90_test_ff, 0x5078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5079_90_test_ff, 0x5079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5080_90_test_ff, 0x5080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5088_90_test_ff, 0x5088, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5090_90_test_ff, 0x5090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5098_90_test_ff, 0x5098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a0_90_test_ff, 0x50a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a8_90_test_ff, 0x50a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b0_90_test_ff, 0x50b0, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_50b8_90_test_ff, 0x50b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b9_90_test_ff, 0x50b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50c0_90_test_ff, 0x50c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50c8_90_test_ff, 0x50c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_50d0_90_test_ff, 0x50d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50d8_90_test_ff, 0x50d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e0_90_test_ff, 0x50e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e8_90_test_ff, 0x50e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f0_90_test_ff, 0x50f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_50f8_90_test_ff, 0x50f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f9_90_test_ff, 0x50f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5100_90_test_ff, 0x5100, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5110_90_test_ff, 0x5110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5118_90_test_ff, 0x5118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5120_90_test_ff, 0x5120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5128_90_test_ff, 0x5128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5130_90_test_ff, 0x5130, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_5138_90_test_ff, 0x5138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5139_90_test_ff, 0x5139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5140_90_test_ff, 0x5140, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5148_90_test_ff, 0x5148, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5150_90_test_ff, 0x5150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5158_90_test_ff, 0x5158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5160_90_test_ff, 0x5160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5168_90_test_ff, 0x5168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5170_90_test_ff, 0x5170, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_5178_90_test_ff, 0x5178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5179_90_test_ff, 0x5179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5180_90_test_ff, 0x5180, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5188_90_test_ff, 0x5188, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5190_90_test_ff, 0x5190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5198_90_test_ff, 0x5198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a0_90_test_ff, 0x51a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a8_90_test_ff, 0x51a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b0_90_test_ff, 0x51b0, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_51b8_90_test_ff, 0x51b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b9_90_test_ff, 0x51b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51c0_90_test_ff, 0x51c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51c8_90_test_ff, 0x51c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_51d0_90_test_ff, 0x51d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51d8_90_test_ff, 0x51d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e0_90_test_ff, 0x51e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e8_90_test_ff, 0x51e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f0_90_test_ff, 0x51f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_51f8_90_test_ff, 0x51f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f9_90_test_ff, 0x51f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c0_90_test_ff, 0x52c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c8_90_test_ff, 0x52c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_52d0_90_test_ff, 0x52d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52d8_90_test_ff, 0x52d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e0_90_test_ff, 0x52e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e8_90_test_ff, 0x52e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f0_90_test_ff, 0x52f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_52f8_90_test_ff, 0x52f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f9_90_test_ff, 0x52f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c0_90_test_ff, 0x53c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c8_90_test_ff, 0x53c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_53d0_90_test_ff, 0x53d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53d8_90_test_ff, 0x53d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e0_90_test_ff, 0x53e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e8_90_test_ff, 0x53e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f0_90_test_ff, 0x53f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_53f8_90_test_ff, 0x53f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f9_90_test_ff, 0x53f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c0_90_test_ff, 0x54c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c8_90_test_ff, 0x54c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_54d0_90_test_ff, 0x54d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54d8_90_test_ff, 0x54d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e0_90_test_ff, 0x54e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e8_90_test_ff, 0x54e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f0_90_test_ff, 0x54f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_54f8_90_test_ff, 0x54f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f9_90_test_ff, 0x54f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c0_90_test_ff, 0x55c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c8_90_test_ff, 0x55c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_55d0_90_test_ff, 0x55d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55d8_90_test_ff, 0x55d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e0_90_test_ff, 0x55e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e8_90_test_ff, 0x55e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f0_90_test_ff, 0x55f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_55f8_90_test_ff, 0x55f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f9_90_test_ff, 0x55f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c0_90_test_ff, 0x56c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c8_90_test_ff, 0x56c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_56d0_90_test_ff, 0x56d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56d8_90_test_ff, 0x56d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e0_90_test_ff, 0x56e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e8_90_test_ff, 0x56e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f0_90_test_ff, 0x56f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_56f8_90_test_ff, 0x56f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f9_90_test_ff, 0x56f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c0_90_test_ff, 0x57c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c8_90_test_ff, 0x57c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_57d0_90_test_ff, 0x57d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57d8_90_test_ff, 0x57d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e0_90_test_ff, 0x57e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e8_90_test_ff, 0x57e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f0_90_test_ff, 0x57f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_57f8_90_test_ff, 0x57f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f9_90_test_ff, 0x57f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c0_90_test_ff, 0x58c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c8_90_test_ff, 0x58c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_58d0_90_test_ff, 0x58d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58d8_90_test_ff, 0x58d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e0_90_test_ff, 0x58e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e8_90_test_ff, 0x58e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f0_90_test_ff, 0x58f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_58f8_90_test_ff, 0x58f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f9_90_test_ff, 0x58f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c0_90_test_ff, 0x59c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c8_90_test_ff, 0x59c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_59d0_90_test_ff, 0x59d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59d8_90_test_ff, 0x59d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e0_90_test_ff, 0x59e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e8_90_test_ff, 0x59e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f0_90_test_ff, 0x59f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_59f8_90_test_ff, 0x59f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f9_90_test_ff, 0x59f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac0_90_test_ff, 0x5ac0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac8_90_test_ff, 0x5ac8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ad0_90_test_ff, 0x5ad0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ad8_90_test_ff, 0x5ad8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae0_90_test_ff, 0x5ae0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae8_90_test_ff, 0x5ae8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af0_90_test_ff, 0x5af0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5af8_90_test_ff, 0x5af8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af9_90_test_ff, 0x5af9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc0_90_test_ff, 0x5bc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc8_90_test_ff, 0x5bc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5bd0_90_test_ff, 0x5bd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bd8_90_test_ff, 0x5bd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be0_90_test_ff, 0x5be0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be8_90_test_ff, 0x5be8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf0_90_test_ff, 0x5bf0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5bf8_90_test_ff, 0x5bf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf9_90_test_ff, 0x5bf9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc0_90_test_ff, 0x5cc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc8_90_test_ff, 0x5cc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5cd0_90_test_ff, 0x5cd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cd8_90_test_ff, 0x5cd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce0_90_test_ff, 0x5ce0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce8_90_test_ff, 0x5ce8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf0_90_test_ff, 0x5cf0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5cf8_90_test_ff, 0x5cf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf9_90_test_ff, 0x5cf9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc0_90_test_ff, 0x5dc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc8_90_test_ff, 0x5dc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5dd0_90_test_ff, 0x5dd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dd8_90_test_ff, 0x5dd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de0_90_test_ff, 0x5de0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de8_90_test_ff, 0x5de8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df0_90_test_ff, 0x5df0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5df8_90_test_ff, 0x5df8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df9_90_test_ff, 0x5df9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec0_90_test_ff, 0x5ec0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec8_90_test_ff, 0x5ec8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ed0_90_test_ff, 0x5ed0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ed8_90_test_ff, 0x5ed8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee0_90_test_ff, 0x5ee0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee8_90_test_ff, 0x5ee8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef0_90_test_ff, 0x5ef0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5ef8_90_test_ff, 0x5ef8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef9_90_test_ff, 0x5ef9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc0_90_test_ff, 0x5fc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc8_90_test_ff, 0x5fc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5fd0_90_test_ff, 0x5fd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fd8_90_test_ff, 0x5fd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe0_90_test_ff, 0x5fe0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe8_90_test_ff, 0x5fe8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff0_90_test_ff, 0x5ff0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5ff8_90_test_ff, 0x5ff8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff9_90_test_ff, 0x5ff9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_6000_90_test_ff, 0x6000, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6001_90_test_ff, 0x6001, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_60ff_90_test_ff, 0x60ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6100_90_test_ff, 0x6100, 4, { 0, 0 }, 2 }, /* BSR */
{ NULL, op_6101_90_test_ff, 0x6101, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_61ff_90_test_ff, 0x61ff, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_6200_90_test_ff, 0x6200, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6201_90_test_ff, 0x6201, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_62ff_90_test_ff, 0x62ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6300_90_test_ff, 0x6300, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6301_90_test_ff, 0x6301, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_63ff_90_test_ff, 0x63ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6400_90_test_ff, 0x6400, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6401_90_test_ff, 0x6401, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_64ff_90_test_ff, 0x64ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6500_90_test_ff, 0x6500, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6501_90_test_ff, 0x6501, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_65ff_90_test_ff, 0x65ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6600_90_test_ff, 0x6600, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6601_90_test_ff, 0x6601, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_66ff_90_test_ff, 0x66ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6700_90_test_ff, 0x6700, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6701_90_test_ff, 0x6701, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_67ff_90_test_ff, 0x67ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6800_90_test_ff, 0x6800, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6801_90_test_ff, 0x6801, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_68ff_90_test_ff, 0x68ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6900_90_test_ff, 0x6900, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6901_90_test_ff, 0x6901, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_69ff_90_test_ff, 0x69ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6a00_90_test_ff, 0x6a00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6a01_90_test_ff, 0x6a01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6aff_90_test_ff, 0x6aff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6b00_90_test_ff, 0x6b00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6b01_90_test_ff, 0x6b01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6bff_90_test_ff, 0x6bff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6c00_90_test_ff, 0x6c00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6c01_90_test_ff, 0x6c01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6cff_90_test_ff, 0x6cff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6d00_90_test_ff, 0x6d00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6d01_90_test_ff, 0x6d01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6dff_90_test_ff, 0x6dff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6e00_90_test_ff, 0x6e00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6e01_90_test_ff, 0x6e01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6eff_90_test_ff, 0x6eff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6f00_90_test_ff, 0x6f00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6f01_90_test_ff, 0x6f01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6fff_90_test_ff, 0x6fff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_7000_90_test_ff, 0x7000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_8000_90_test_ff, 0x8000, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8010_90_test_ff, 0x8010, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8018_90_test_ff, 0x8018, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8020_90_test_ff, 0x8020, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8028_90_test_ff, 0x8028, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8030_90_test_ff, 0x8030, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8038_90_test_ff, 0x8038, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8039_90_test_ff, 0x8039, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803a_90_test_ff, 0x803a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803b_90_test_ff, 0x803b, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_803c_90_test_ff, 0x803c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8040_90_test_ff, 0x8040, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8050_90_test_ff, 0x8050, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8058_90_test_ff, 0x8058, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8060_90_test_ff, 0x8060, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8068_90_test_ff, 0x8068, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8070_90_test_ff, 0x8070, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8078_90_test_ff, 0x8078, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8079_90_test_ff, 0x8079, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807a_90_test_ff, 0x807a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807b_90_test_ff, 0x807b, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_807c_90_test_ff, 0x807c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8080_90_test_ff, 0x8080, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8090_90_test_ff, 0x8090, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8098_90_test_ff, 0x8098, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a0_90_test_ff, 0x80a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a8_90_test_ff, 0x80a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b0_90_test_ff, 0x80b0, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_80b8_90_test_ff, 0x80b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b9_90_test_ff, 0x80b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80ba_90_test_ff, 0x80ba, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80bb_90_test_ff, 0x80bb, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_80bc_90_test_ff, 0x80bc, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80c0_90_test_ff, 0x80c0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d0_90_test_ff, 0x80d0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d8_90_test_ff, 0x80d8, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e0_90_test_ff, 0x80e0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e8_90_test_ff, 0x80e8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f0_90_test_ff, 0x80f0, 4, { 4, 0 }, 0 }, /* DIVU */
{ NULL, op_80f8_90_test_ff, 0x80f8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f9_90_test_ff, 0x80f9, 6, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fa_90_test_ff, 0x80fa, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fb_90_test_ff, 0x80fb, 4, { 4, 0 }, 0 }, /* DIVU */
{ NULL, op_80fc_90_test_ff, 0x80fc, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_8100_90_test_ff, 0x8100, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8108_90_test_ff, 0x8108, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8110_90_test_ff, 0x8110, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8118_90_test_ff, 0x8118, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8120_90_test_ff, 0x8120, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8128_90_test_ff, 0x8128, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8130_90_test_ff, 0x8130, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8138_90_test_ff, 0x8138, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8139_90_test_ff, 0x8139, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8150_90_test_ff, 0x8150, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8158_90_test_ff, 0x8158, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8160_90_test_ff, 0x8160, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8168_90_test_ff, 0x8168, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8170_90_test_ff, 0x8170, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8178_90_test_ff, 0x8178, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8179_90_test_ff, 0x8179, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8190_90_test_ff, 0x8190, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8198_90_test_ff, 0x8198, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a0_90_test_ff, 0x81a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a8_90_test_ff, 0x81a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b0_90_test_ff, 0x81b0, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_81b8_90_test_ff, 0x81b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b9_90_test_ff, 0x81b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81c0_90_test_ff, 0x81c0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d0_90_test_ff, 0x81d0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d8_90_test_ff, 0x81d8, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e0_90_test_ff, 0x81e0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e8_90_test_ff, 0x81e8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f0_90_test_ff, 0x81f0, 4, { 4, 0 }, 0 }, /* DIVS */
{ NULL, op_81f8_90_test_ff, 0x81f8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f9_90_test_ff, 0x81f9, 6, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fa_90_test_ff, 0x81fa, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fb_90_test_ff, 0x81fb, 4, { 4, 0 }, 0 }, /* DIVS */
{ NULL, op_81fc_90_test_ff, 0x81fc, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_9000_90_test_ff, 0x9000, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9010_90_test_ff, 0x9010, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9018_90_test_ff, 0x9018, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9020_90_test_ff, 0x9020, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9028_90_test_ff, 0x9028, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9030_90_test_ff, 0x9030, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9038_90_test_ff, 0x9038, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9039_90_test_ff, 0x9039, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903a_90_test_ff, 0x903a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903b_90_test_ff, 0x903b, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_903c_90_test_ff, 0x903c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9040_90_test_ff, 0x9040, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9048_90_test_ff, 0x9048, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9050_90_test_ff, 0x9050, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9058_90_test_ff, 0x9058, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9060_90_test_ff, 0x9060, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9068_90_test_ff, 0x9068, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9070_90_test_ff, 0x9070, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9078_90_test_ff, 0x9078, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9079_90_test_ff, 0x9079, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907a_90_test_ff, 0x907a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907b_90_test_ff, 0x907b, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_907c_90_test_ff, 0x907c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9080_90_test_ff, 0x9080, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9088_90_test_ff, 0x9088, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9090_90_test_ff, 0x9090, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9098_90_test_ff, 0x9098, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a0_90_test_ff, 0x90a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a8_90_test_ff, 0x90a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b0_90_test_ff, 0x90b0, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_90b8_90_test_ff, 0x90b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b9_90_test_ff, 0x90b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90ba_90_test_ff, 0x90ba, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90bb_90_test_ff, 0x90bb, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_90bc_90_test_ff, 0x90bc, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90c0_90_test_ff, 0x90c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90c8_90_test_ff, 0x90c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d0_90_test_ff, 0x90d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d8_90_test_ff, 0x90d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e0_90_test_ff, 0x90e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e8_90_test_ff, 0x90e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f0_90_test_ff, 0x90f0, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_90f8_90_test_ff, 0x90f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f9_90_test_ff, 0x90f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fa_90_test_ff, 0x90fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fb_90_test_ff, 0x90fb, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_90fc_90_test_ff, 0x90fc, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_9100_90_test_ff, 0x9100, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9108_90_test_ff, 0x9108, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9110_90_test_ff, 0x9110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9118_90_test_ff, 0x9118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9120_90_test_ff, 0x9120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9128_90_test_ff, 0x9128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9130_90_test_ff, 0x9130, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9138_90_test_ff, 0x9138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9139_90_test_ff, 0x9139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9140_90_test_ff, 0x9140, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9148_90_test_ff, 0x9148, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9150_90_test_ff, 0x9150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9158_90_test_ff, 0x9158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9160_90_test_ff, 0x9160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9168_90_test_ff, 0x9168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9170_90_test_ff, 0x9170, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9178_90_test_ff, 0x9178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9179_90_test_ff, 0x9179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9180_90_test_ff, 0x9180, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9188_90_test_ff, 0x9188, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9190_90_test_ff, 0x9190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9198_90_test_ff, 0x9198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a0_90_test_ff, 0x91a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a8_90_test_ff, 0x91a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b0_90_test_ff, 0x91b0, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_91b8_90_test_ff, 0x91b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b9_90_test_ff, 0x91b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91c0_90_test_ff, 0x91c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91c8_90_test_ff, 0x91c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d0_90_test_ff, 0x91d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d8_90_test_ff, 0x91d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e0_90_test_ff, 0x91e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e8_90_test_ff, 0x91e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f0_90_test_ff, 0x91f0, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_91f8_90_test_ff, 0x91f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f9_90_test_ff, 0x91f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fa_90_test_ff, 0x91fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fb_90_test_ff, 0x91fb, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_91fc_90_test_ff, 0x91fc, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_b000_90_test_ff, 0xb000, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b010_90_test_ff, 0xb010, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b018_90_test_ff, 0xb018, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b020_90_test_ff, 0xb020, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b028_90_test_ff, 0xb028, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b030_90_test_ff, 0xb030, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b038_90_test_ff, 0xb038, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b039_90_test_ff, 0xb039, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03a_90_test_ff, 0xb03a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03b_90_test_ff, 0xb03b, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b03c_90_test_ff, 0xb03c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b040_90_test_ff, 0xb040, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b048_90_test_ff, 0xb048, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b050_90_test_ff, 0xb050, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b058_90_test_ff, 0xb058, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b060_90_test_ff, 0xb060, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b068_90_test_ff, 0xb068, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b070_90_test_ff, 0xb070, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b078_90_test_ff, 0xb078, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b079_90_test_ff, 0xb079, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07a_90_test_ff, 0xb07a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07b_90_test_ff, 0xb07b, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b07c_90_test_ff, 0xb07c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b080_90_test_ff, 0xb080, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b088_90_test_ff, 0xb088, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b090_90_test_ff, 0xb090, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b098_90_test_ff, 0xb098, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a0_90_test_ff, 0xb0a0, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a8_90_test_ff, 0xb0a8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b0_90_test_ff, 0xb0b0, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b0b8_90_test_ff, 0xb0b8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b9_90_test_ff, 0xb0b9, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0ba_90_test_ff, 0xb0ba, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0bb_90_test_ff, 0xb0bb, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b0bc_90_test_ff, 0xb0bc, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0c0_90_test_ff, 0xb0c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0c8_90_test_ff, 0xb0c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d0_90_test_ff, 0xb0d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d8_90_test_ff, 0xb0d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e0_90_test_ff, 0xb0e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e8_90_test_ff, 0xb0e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f0_90_test_ff, 0xb0f0, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f8_90_test_ff, 0xb0f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f9_90_test_ff, 0xb0f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fa_90_test_ff, 0xb0fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fb_90_test_ff, 0xb0fb, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fc_90_test_ff, 0xb0fc, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b100_90_test_ff, 0xb100, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b108_90_test_ff, 0xb108, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b110_90_test_ff, 0xb110, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b118_90_test_ff, 0xb118, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b120_90_test_ff, 0xb120, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b128_90_test_ff, 0xb128, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b130_90_test_ff, 0xb130, 4, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_b138_90_test_ff, 0xb138, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b139_90_test_ff, 0xb139, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b140_90_test_ff, 0xb140, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b148_90_test_ff, 0xb148, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b150_90_test_ff, 0xb150, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b158_90_test_ff, 0xb158, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b160_90_test_ff, 0xb160, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b168_90_test_ff, 0xb168, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b170_90_test_ff, 0xb170, 4, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_b178_90_test_ff, 0xb178, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b179_90_test_ff, 0xb179, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b180_90_test_ff, 0xb180, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b188_90_test_ff, 0xb188, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b190_90_test_ff, 0xb190, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b198_90_test_ff, 0xb198, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a0_90_test_ff, 0xb1a0, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a8_90_test_ff, 0xb1a8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b0_90_test_ff, 0xb1b0, 4, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_b1b8_90_test_ff, 0xb1b8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b9_90_test_ff, 0xb1b9, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1c0_90_test_ff, 0xb1c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1c8_90_test_ff, 0xb1c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d0_90_test_ff, 0xb1d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d8_90_test_ff, 0xb1d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e0_90_test_ff, 0xb1e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e8_90_test_ff, 0xb1e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f0_90_test_ff, 0xb1f0, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f8_90_test_ff, 0xb1f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f9_90_test_ff, 0xb1f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fa_90_test_ff, 0xb1fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fb_90_test_ff, 0xb1fb, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fc_90_test_ff, 0xb1fc, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_c000_90_test_ff, 0xc000, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c010_90_test_ff, 0xc010, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c018_90_test_ff, 0xc018, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c020_90_test_ff, 0xc020, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c028_90_test_ff, 0xc028, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c030_90_test_ff, 0xc030, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c038_90_test_ff, 0xc038, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c039_90_test_ff, 0xc039, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03a_90_test_ff, 0xc03a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03b_90_test_ff, 0xc03b, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c03c_90_test_ff, 0xc03c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c040_90_test_ff, 0xc040, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c050_90_test_ff, 0xc050, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c058_90_test_ff, 0xc058, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c060_90_test_ff, 0xc060, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c068_90_test_ff, 0xc068, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c070_90_test_ff, 0xc070, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c078_90_test_ff, 0xc078, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c079_90_test_ff, 0xc079, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07a_90_test_ff, 0xc07a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07b_90_test_ff, 0xc07b, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c07c_90_test_ff, 0xc07c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c080_90_test_ff, 0xc080, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c090_90_test_ff, 0xc090, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c098_90_test_ff, 0xc098, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a0_90_test_ff, 0xc0a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a8_90_test_ff, 0xc0a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b0_90_test_ff, 0xc0b0, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c0b8_90_test_ff, 0xc0b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b9_90_test_ff, 0xc0b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0ba_90_test_ff, 0xc0ba, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0bb_90_test_ff, 0xc0bb, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c0bc_90_test_ff, 0xc0bc, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0c0_90_test_ff, 0xc0c0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d0_90_test_ff, 0xc0d0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d8_90_test_ff, 0xc0d8, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e0_90_test_ff, 0xc0e0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e8_90_test_ff, 0xc0e8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f0_90_test_ff, 0xc0f0, 4, { 4, 0 }, 0 }, /* MULU */
{ NULL, op_c0f8_90_test_ff, 0xc0f8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f9_90_test_ff, 0xc0f9, 6, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fa_90_test_ff, 0xc0fa, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fb_90_test_ff, 0xc0fb, 4, { 4, 0 }, 0 }, /* MULU */
{ NULL, op_c0fc_90_test_ff, 0xc0fc, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c100_90_test_ff, 0xc100, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c108_90_test_ff, 0xc108, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c110_90_test_ff, 0xc110, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c118_90_test_ff, 0xc118, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c120_90_test_ff, 0xc120, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c128_90_test_ff, 0xc128, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c130_90_test_ff, 0xc130, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c138_90_test_ff, 0xc138, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c139_90_test_ff, 0xc139, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c140_90_test_ff, 0xc140, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c148_90_test_ff, 0xc148, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c150_90_test_ff, 0xc150, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c158_90_test_ff, 0xc158, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c160_90_test_ff, 0xc160, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c168_90_test_ff, 0xc168, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c170_90_test_ff, 0xc170, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c178_90_test_ff, 0xc178, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c179_90_test_ff, 0xc179, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c188_90_test_ff, 0xc188, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c190_90_test_ff, 0xc190, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c198_90_test_ff, 0xc198, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a0_90_test_ff, 0xc1a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a8_90_test_ff, 0xc1a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b0_90_test_ff, 0xc1b0, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c1b8_90_test_ff, 0xc1b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b9_90_test_ff, 0xc1b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1c0_90_test_ff, 0xc1c0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d0_90_test_ff, 0xc1d0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d8_90_test_ff, 0xc1d8, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e0_90_test_ff, 0xc1e0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e8_90_test_ff, 0xc1e8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f0_90_test_ff, 0xc1f0, 4, { 4, 0 }, 0 }, /* MULS */
{ NULL, op_c1f8_90_test_ff, 0xc1f8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f9_90_test_ff, 0xc1f9, 6, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fa_90_test_ff, 0xc1fa, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fb_90_test_ff, 0xc1fb, 4, { 4, 0 }, 0 }, /* MULS */
{ NULL, op_c1fc_90_test_ff, 0xc1fc, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_d000_90_test_ff, 0xd000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d010_90_test_ff, 0xd010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d018_90_test_ff, 0xd018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d020_90_test_ff, 0xd020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d028_90_test_ff, 0xd028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d030_90_test_ff, 0xd030, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d038_90_test_ff, 0xd038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d039_90_test_ff, 0xd039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03a_90_test_ff, 0xd03a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03b_90_test_ff, 0xd03b, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d03c_90_test_ff, 0xd03c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d040_90_test_ff, 0xd040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d048_90_test_ff, 0xd048, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d050_90_test_ff, 0xd050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d058_90_test_ff, 0xd058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d060_90_test_ff, 0xd060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d068_90_test_ff, 0xd068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d070_90_test_ff, 0xd070, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d078_90_test_ff, 0xd078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d079_90_test_ff, 0xd079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07a_90_test_ff, 0xd07a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07b_90_test_ff, 0xd07b, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d07c_90_test_ff, 0xd07c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d080_90_test_ff, 0xd080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d088_90_test_ff, 0xd088, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d090_90_test_ff, 0xd090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d098_90_test_ff, 0xd098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a0_90_test_ff, 0xd0a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a8_90_test_ff, 0xd0a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b0_90_test_ff, 0xd0b0, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d0b8_90_test_ff, 0xd0b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b9_90_test_ff, 0xd0b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0ba_90_test_ff, 0xd0ba, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0bb_90_test_ff, 0xd0bb, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d0bc_90_test_ff, 0xd0bc, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0c0_90_test_ff, 0xd0c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0c8_90_test_ff, 0xd0c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d0_90_test_ff, 0xd0d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d8_90_test_ff, 0xd0d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e0_90_test_ff, 0xd0e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e8_90_test_ff, 0xd0e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f0_90_test_ff, 0xd0f0, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f8_90_test_ff, 0xd0f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f9_90_test_ff, 0xd0f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fa_90_test_ff, 0xd0fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fb_90_test_ff, 0xd0fb, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fc_90_test_ff, 0xd0fc, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d100_90_test_ff, 0xd100, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d108_90_test_ff, 0xd108, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d110_90_test_ff, 0xd110, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d118_90_test_ff, 0xd118, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d120_90_test_ff, 0xd120, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d128_90_test_ff, 0xd128, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d130_90_test_ff, 0xd130, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d138_90_test_ff, 0xd138, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d139_90_test_ff, 0xd139, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d140_90_test_ff, 0xd140, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d148_90_test_ff, 0xd148, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d150_90_test_ff, 0xd150, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d158_90_test_ff, 0xd158, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d160_90_test_ff, 0xd160, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d168_90_test_ff, 0xd168, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d170_90_test_ff, 0xd170, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d178_90_test_ff, 0xd178, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d179_90_test_ff, 0xd179, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d180_90_test_ff, 0xd180, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d188_90_test_ff, 0xd188, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d190_90_test_ff, 0xd190, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d198_90_test_ff, 0xd198, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a0_90_test_ff, 0xd1a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a8_90_test_ff, 0xd1a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b0_90_test_ff, 0xd1b0, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d1b8_90_test_ff, 0xd1b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b9_90_test_ff, 0xd1b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1c0_90_test_ff, 0xd1c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1c8_90_test_ff, 0xd1c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d0_90_test_ff, 0xd1d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d8_90_test_ff, 0xd1d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e0_90_test_ff, 0xd1e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e8_90_test_ff, 0xd1e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f0_90_test_ff, 0xd1f0, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f8_90_test_ff, 0xd1f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f9_90_test_ff, 0xd1f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fa_90_test_ff, 0xd1fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fb_90_test_ff, 0xd1fb, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fc_90_test_ff, 0xd1fc, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_e000_90_test_ff, 0xe000, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e008_90_test_ff, 0xe008, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e010_90_test_ff, 0xe010, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e018_90_test_ff, 0xe018, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e020_90_test_ff, 0xe020, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e028_90_test_ff, 0xe028, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e030_90_test_ff, 0xe030, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e038_90_test_ff, 0xe038, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e040_90_test_ff, 0xe040, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e048_90_test_ff, 0xe048, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e050_90_test_ff, 0xe050, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e058_90_test_ff, 0xe058, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e060_90_test_ff, 0xe060, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e068_90_test_ff, 0xe068, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e070_90_test_ff, 0xe070, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e078_90_test_ff, 0xe078, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e080_90_test_ff, 0xe080, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e088_90_test_ff, 0xe088, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e090_90_test_ff, 0xe090, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e098_90_test_ff, 0xe098, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0a0_90_test_ff, 0xe0a0, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e0a8_90_test_ff, 0xe0a8, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e0b0_90_test_ff, 0xe0b0, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e0b8_90_test_ff, 0xe0b8, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0d0_90_test_ff, 0xe0d0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0d8_90_test_ff, 0xe0d8, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e0_90_test_ff, 0xe0e0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e8_90_test_ff, 0xe0e8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f0_90_test_ff, 0xe0f0, 4, { 4, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f8_90_test_ff, 0xe0f8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f9_90_test_ff, 0xe0f9, 6, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e100_90_test_ff, 0xe100, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e108_90_test_ff, 0xe108, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e110_90_test_ff, 0xe110, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e118_90_test_ff, 0xe118, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e120_90_test_ff, 0xe120, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e128_90_test_ff, 0xe128, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e130_90_test_ff, 0xe130, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e138_90_test_ff, 0xe138, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e140_90_test_ff, 0xe140, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e148_90_test_ff, 0xe148, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e150_90_test_ff, 0xe150, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e158_90_test_ff, 0xe158, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e160_90_test_ff, 0xe160, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e168_90_test_ff, 0xe168, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e170_90_test_ff, 0xe170, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e178_90_test_ff, 0xe178, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e180_90_test_ff, 0xe180, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e188_90_test_ff, 0xe188, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e190_90_test_ff, 0xe190, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e198_90_test_ff, 0xe198, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1a0_90_test_ff, 0xe1a0, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e1a8_90_test_ff, 0xe1a8, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e1b0_90_test_ff, 0xe1b0, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e1b8_90_test_ff, 0xe1b8, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1d0_90_test_ff, 0xe1d0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1d8_90_test_ff, 0xe1d8, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e0_90_test_ff, 0xe1e0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e8_90_test_ff, 0xe1e8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f0_90_test_ff, 0xe1f0, 4, { 4, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f8_90_test_ff, 0xe1f8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f9_90_test_ff, 0xe1f9, 6, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e2d0_90_test_ff, 0xe2d0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2d8_90_test_ff, 0xe2d8, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e0_90_test_ff, 0xe2e0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e8_90_test_ff, 0xe2e8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f0_90_test_ff, 0xe2f0, 4, { 4, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f8_90_test_ff, 0xe2f8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f9_90_test_ff, 0xe2f9, 6, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e3d0_90_test_ff, 0xe3d0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3d8_90_test_ff, 0xe3d8, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e0_90_test_ff, 0xe3e0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e8_90_test_ff, 0xe3e8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f0_90_test_ff, 0xe3f0, 4, { 4, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f8_90_test_ff, 0xe3f8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f9_90_test_ff, 0xe3f9, 6, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e4d0_90_test_ff, 0xe4d0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4d8_90_test_ff, 0xe4d8, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e0_90_test_ff, 0xe4e0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e8_90_test_ff, 0xe4e8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f0_90_test_ff, 0xe4f0, 4, { 4, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f8_90_test_ff, 0xe4f8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f9_90_test_ff, 0xe4f9, 6, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e5d0_90_test_ff, 0xe5d0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5d8_90_test_ff, 0xe5d8, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e0_90_test_ff, 0xe5e0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e8_90_test_ff, 0xe5e8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f0_90_test_ff, 0xe5f0, 4, { 4, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f8_90_test_ff, 0xe5f8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f9_90_test_ff, 0xe5f9, 6, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e6d0_90_test_ff, 0xe6d0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6d8_90_test_ff, 0xe6d8, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e0_90_test_ff, 0xe6e0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e8_90_test_ff, 0xe6e8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f0_90_test_ff, 0xe6f0, 4, { 4, 0 }, 0 }, /* RORW */
{ NULL, op_e6f8_90_test_ff, 0xe6f8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f9_90_test_ff, 0xe6f9, 6, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e7d0_90_test_ff, 0xe7d0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7d8_90_test_ff, 0xe7d8, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e0_90_test_ff, 0xe7e0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e8_90_test_ff, 0xe7e8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f0_90_test_ff, 0xe7f0, 4, { 4, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f8_90_test_ff, 0xe7f8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f9_90_test_ff, 0xe7f9, 6, { 0, 0 }, 0 }, /* ROLW */
{ 0 }};
#endif /* CPUEMU_90 */
#ifdef CPUEMU_91
const struct cputbl CPUFUNC(op_smalltbl_91_test)[] = {
{ NULL, op_0000_91_test_ff, 0x0000, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0010_91_test_ff, 0x0010, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0018_91_test_ff, 0x0018, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0020_91_test_ff, 0x0020, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0028_91_test_ff, 0x0028, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0030_91_test_ff, 0x0030, 6, { 4, 0 }, 0 }, /* OR */
{ NULL, op_0038_91_test_ff, 0x0038, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0039_91_test_ff, 0x0039, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_003c_91_test_ff, 0x003c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0040_91_test_ff, 0x0040, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0050_91_test_ff, 0x0050, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0058_91_test_ff, 0x0058, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0060_91_test_ff, 0x0060, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0068_91_test_ff, 0x0068, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0070_91_test_ff, 0x0070, 6, { 4, 0 }, 0 }, /* OR */
{ NULL, op_0078_91_test_ff, 0x0078, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0079_91_test_ff, 0x0079, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_007c_91_test_ff, 0x007c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0080_91_test_ff, 0x0080, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0090_91_test_ff, 0x0090, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0098_91_test_ff, 0x0098, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a0_91_test_ff, 0x00a0, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a8_91_test_ff, 0x00a8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b0_91_test_ff, 0x00b0, 8, { 4, 0 }, 0 }, /* OR */
{ NULL, op_00b8_91_test_ff, 0x00b8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b9_91_test_ff, 0x00b9, 10, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0100_91_test_ff, 0x0100, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0108_91_test_ff, 0x0108, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0110_91_test_ff, 0x0110, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0118_91_test_ff, 0x0118, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0120_91_test_ff, 0x0120, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0128_91_test_ff, 0x0128, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0130_91_test_ff, 0x0130, 4, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_0138_91_test_ff, 0x0138, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0139_91_test_ff, 0x0139, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013a_91_test_ff, 0x013a, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013b_91_test_ff, 0x013b, 4, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_013c_91_test_ff, 0x013c, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0140_91_test_ff, 0x0140, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0148_91_test_ff, 0x0148, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0150_91_test_ff, 0x0150, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0158_91_test_ff, 0x0158, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0160_91_test_ff, 0x0160, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0168_91_test_ff, 0x0168, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0170_91_test_ff, 0x0170, 4, { 4, 0 }, 0 }, /* BCHG */
{ NULL, op_0178_91_test_ff, 0x0178, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0179_91_test_ff, 0x0179, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0180_91_test_ff, 0x0180, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0188_91_test_ff, 0x0188, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_0190_91_test_ff, 0x0190, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0198_91_test_ff, 0x0198, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a0_91_test_ff, 0x01a0, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a8_91_test_ff, 0x01a8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b0_91_test_ff, 0x01b0, 4, { 4, 0 }, 0 }, /* BCLR */
{ NULL, op_01b8_91_test_ff, 0x01b8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b9_91_test_ff, 0x01b9, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01c0_91_test_ff, 0x01c0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01c8_91_test_ff, 0x01c8, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_01d0_91_test_ff, 0x01d0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01d8_91_test_ff, 0x01d8, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e0_91_test_ff, 0x01e0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e8_91_test_ff, 0x01e8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f0_91_test_ff, 0x01f0, 4, { 4, 0 }, 0 }, /* BSET */
{ NULL, op_01f8_91_test_ff, 0x01f8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f9_91_test_ff, 0x01f9, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0200_91_test_ff, 0x0200, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0210_91_test_ff, 0x0210, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0218_91_test_ff, 0x0218, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0220_91_test_ff, 0x0220, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0228_91_test_ff, 0x0228, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0230_91_test_ff, 0x0230, 6, { 4, 0 }, 0 }, /* AND */
{ NULL, op_0238_91_test_ff, 0x0238, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0239_91_test_ff, 0x0239, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_023c_91_test_ff, 0x023c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0240_91_test_ff, 0x0240, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0250_91_test_ff, 0x0250, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0258_91_test_ff, 0x0258, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0260_91_test_ff, 0x0260, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0268_91_test_ff, 0x0268, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0270_91_test_ff, 0x0270, 6, { 4, 0 }, 0 }, /* AND */
{ NULL, op_0278_91_test_ff, 0x0278, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0279_91_test_ff, 0x0279, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_027c_91_test_ff, 0x027c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0280_91_test_ff, 0x0280, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0290_91_test_ff, 0x0290, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0298_91_test_ff, 0x0298, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a0_91_test_ff, 0x02a0, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a8_91_test_ff, 0x02a8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b0_91_test_ff, 0x02b0, 8, { 4, 0 }, 0 }, /* AND */
{ NULL, op_02b8_91_test_ff, 0x02b8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b9_91_test_ff, 0x02b9, 10, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0400_91_test_ff, 0x0400, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0410_91_test_ff, 0x0410, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0418_91_test_ff, 0x0418, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0420_91_test_ff, 0x0420, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0428_91_test_ff, 0x0428, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0430_91_test_ff, 0x0430, 6, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_0438_91_test_ff, 0x0438, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0439_91_test_ff, 0x0439, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0440_91_test_ff, 0x0440, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0450_91_test_ff, 0x0450, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0458_91_test_ff, 0x0458, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0460_91_test_ff, 0x0460, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0468_91_test_ff, 0x0468, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0470_91_test_ff, 0x0470, 6, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_0478_91_test_ff, 0x0478, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0479_91_test_ff, 0x0479, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0480_91_test_ff, 0x0480, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0490_91_test_ff, 0x0490, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0498_91_test_ff, 0x0498, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a0_91_test_ff, 0x04a0, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a8_91_test_ff, 0x04a8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b0_91_test_ff, 0x04b0, 8, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_04b8_91_test_ff, 0x04b8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b9_91_test_ff, 0x04b9, 10, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0600_91_test_ff, 0x0600, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0610_91_test_ff, 0x0610, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0618_91_test_ff, 0x0618, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0620_91_test_ff, 0x0620, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0628_91_test_ff, 0x0628, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0630_91_test_ff, 0x0630, 6, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_0638_91_test_ff, 0x0638, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0639_91_test_ff, 0x0639, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0640_91_test_ff, 0x0640, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0650_91_test_ff, 0x0650, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0658_91_test_ff, 0x0658, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0660_91_test_ff, 0x0660, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0668_91_test_ff, 0x0668, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0670_91_test_ff, 0x0670, 6, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_0678_91_test_ff, 0x0678, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0679_91_test_ff, 0x0679, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0680_91_test_ff, 0x0680, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0690_91_test_ff, 0x0690, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0698_91_test_ff, 0x0698, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a0_91_test_ff, 0x06a0, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a8_91_test_ff, 0x06a8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b0_91_test_ff, 0x06b0, 8, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_06b8_91_test_ff, 0x06b8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b9_91_test_ff, 0x06b9, 10, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0800_91_test_ff, 0x0800, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0810_91_test_ff, 0x0810, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0818_91_test_ff, 0x0818, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0820_91_test_ff, 0x0820, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0828_91_test_ff, 0x0828, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0830_91_test_ff, 0x0830, 6, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_0838_91_test_ff, 0x0838, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0839_91_test_ff, 0x0839, 8, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083a_91_test_ff, 0x083a, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083b_91_test_ff, 0x083b, 6, { 4, 0 }, 0 }, /* BTST */
{ NULL, op_0840_91_test_ff, 0x0840, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0850_91_test_ff, 0x0850, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0858_91_test_ff, 0x0858, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0860_91_test_ff, 0x0860, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0868_91_test_ff, 0x0868, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0870_91_test_ff, 0x0870, 6, { 4, 0 }, 0 }, /* BCHG */
{ NULL, op_0878_91_test_ff, 0x0878, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0879_91_test_ff, 0x0879, 8, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0880_91_test_ff, 0x0880, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0890_91_test_ff, 0x0890, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0898_91_test_ff, 0x0898, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a0_91_test_ff, 0x08a0, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a8_91_test_ff, 0x08a8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b0_91_test_ff, 0x08b0, 6, { 4, 0 }, 0 }, /* BCLR */
{ NULL, op_08b8_91_test_ff, 0x08b8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b9_91_test_ff, 0x08b9, 8, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08c0_91_test_ff, 0x08c0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d0_91_test_ff, 0x08d0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d8_91_test_ff, 0x08d8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e0_91_test_ff, 0x08e0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e8_91_test_ff, 0x08e8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f0_91_test_ff, 0x08f0, 6, { 4, 0 }, 0 }, /* BSET */
{ NULL, op_08f8_91_test_ff, 0x08f8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f9_91_test_ff, 0x08f9, 8, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0a00_91_test_ff, 0x0a00, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a10_91_test_ff, 0x0a10, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a18_91_test_ff, 0x0a18, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a20_91_test_ff, 0x0a20, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a28_91_test_ff, 0x0a28, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a30_91_test_ff, 0x0a30, 6, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_0a38_91_test_ff, 0x0a38, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a39_91_test_ff, 0x0a39, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a3c_91_test_ff, 0x0a3c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a40_91_test_ff, 0x0a40, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a50_91_test_ff, 0x0a50, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a58_91_test_ff, 0x0a58, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a60_91_test_ff, 0x0a60, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a68_91_test_ff, 0x0a68, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a70_91_test_ff, 0x0a70, 6, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_0a78_91_test_ff, 0x0a78, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a79_91_test_ff, 0x0a79, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a7c_91_test_ff, 0x0a7c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a80_91_test_ff, 0x0a80, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a90_91_test_ff, 0x0a90, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a98_91_test_ff, 0x0a98, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa0_91_test_ff, 0x0aa0, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa8_91_test_ff, 0x0aa8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab0_91_test_ff, 0x0ab0, 8, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_0ab8_91_test_ff, 0x0ab8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab9_91_test_ff, 0x0ab9, 10, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0c00_91_test_ff, 0x0c00, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c10_91_test_ff, 0x0c10, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c18_91_test_ff, 0x0c18, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c20_91_test_ff, 0x0c20, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c28_91_test_ff, 0x0c28, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c30_91_test_ff, 0x0c30, 6, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_0c38_91_test_ff, 0x0c38, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c39_91_test_ff, 0x0c39, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c40_91_test_ff, 0x0c40, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c50_91_test_ff, 0x0c50, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c58_91_test_ff, 0x0c58, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c60_91_test_ff, 0x0c60, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c68_91_test_ff, 0x0c68, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c70_91_test_ff, 0x0c70, 6, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_0c78_91_test_ff, 0x0c78, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c79_91_test_ff, 0x0c79, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c80_91_test_ff, 0x0c80, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c90_91_test_ff, 0x0c90, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c98_91_test_ff, 0x0c98, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca0_91_test_ff, 0x0ca0, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca8_91_test_ff, 0x0ca8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb0_91_test_ff, 0x0cb0, 8, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_0cb8_91_test_ff, 0x0cb8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb9_91_test_ff, 0x0cb9, 10, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e10_91_test_ff, 0x0e10, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e18_91_test_ff, 0x0e18, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e20_91_test_ff, 0x0e20, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e28_91_test_ff, 0x0e28, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e30_91_test_ff, 0x0e30, 6, { 4, 4 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e38_91_test_ff, 0x0e38, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e39_91_test_ff, 0x0e39, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e50_91_test_ff, 0x0e50, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e58_91_test_ff, 0x0e58, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e60_91_test_ff, 0x0e60, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e68_91_test_ff, 0x0e68, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e70_91_test_ff, 0x0e70, 6, { 4, 4 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e78_91_test_ff, 0x0e78, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e79_91_test_ff, 0x0e79, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e90_91_test_ff, 0x0e90, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e98_91_test_ff, 0x0e98, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea0_91_test_ff, 0x0ea0, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea8_91_test_ff, 0x0ea8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb0_91_test_ff, 0x0eb0, 6, { 4, 4 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb8_91_test_ff, 0x0eb8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb9_91_test_ff, 0x0eb9, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
{ NULL, op_1000_91_test_ff, 0x1000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1010_91_test_ff, 0x1010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1018_91_test_ff, 0x1018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1020_91_test_ff, 0x1020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1028_91_test_ff, 0x1028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1030_91_test_ff, 0x1030, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1038_91_test_ff, 0x1038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1039_91_test_ff, 0x1039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103a_91_test_ff, 0x103a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103b_91_test_ff, 0x103b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_103c_91_test_ff, 0x103c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1080_91_test_ff, 0x1080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1090_91_test_ff, 0x1090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1098_91_test_ff, 0x1098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a0_91_test_ff, 0x10a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a8_91_test_ff, 0x10a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b0_91_test_ff, 0x10b0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10b8_91_test_ff, 0x10b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b9_91_test_ff, 0x10b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10ba_91_test_ff, 0x10ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10bb_91_test_ff, 0x10bb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10bc_91_test_ff, 0x10bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10c0_91_test_ff, 0x10c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d0_91_test_ff, 0x10d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d8_91_test_ff, 0x10d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e0_91_test_ff, 0x10e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e8_91_test_ff, 0x10e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f0_91_test_ff, 0x10f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10f8_91_test_ff, 0x10f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f9_91_test_ff, 0x10f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fa_91_test_ff, 0x10fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fb_91_test_ff, 0x10fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_10fc_91_test_ff, 0x10fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1100_91_test_ff, 0x1100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1110_91_test_ff, 0x1110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1118_91_test_ff, 0x1118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1120_91_test_ff, 0x1120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1128_91_test_ff, 0x1128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1130_91_test_ff, 0x1130, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1138_91_test_ff, 0x1138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1139_91_test_ff, 0x1139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113a_91_test_ff, 0x113a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113b_91_test_ff, 0x113b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_113c_91_test_ff, 0x113c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1140_91_test_ff, 0x1140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1150_91_test_ff, 0x1150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1158_91_test_ff, 0x1158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1160_91_test_ff, 0x1160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1168_91_test_ff, 0x1168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1170_91_test_ff, 0x1170, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_1178_91_test_ff, 0x1178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1179_91_test_ff, 0x1179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117a_91_test_ff, 0x117a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117b_91_test_ff, 0x117b, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_117c_91_test_ff, 0x117c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1180_91_test_ff, 0x1180, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1190_91_test_ff, 0x1190, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1198_91_test_ff, 0x1198, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11a0_91_test_ff, 0x11a0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11a8_91_test_ff, 0x11a8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11b0_91_test_ff, 0x11b0, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_11b8_91_test_ff, 0x11b8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11b9_91_test_ff, 0x11b9, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11ba_91_test_ff, 0x11ba, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11bb_91_test_ff, 0x11bb, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_11bc_91_test_ff, 0x11bc, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11c0_91_test_ff, 0x11c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d0_91_test_ff, 0x11d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d8_91_test_ff, 0x11d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e0_91_test_ff, 0x11e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e8_91_test_ff, 0x11e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f0_91_test_ff, 0x11f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_11f8_91_test_ff, 0x11f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f9_91_test_ff, 0x11f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fa_91_test_ff, 0x11fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fb_91_test_ff, 0x11fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_11fc_91_test_ff, 0x11fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13c0_91_test_ff, 0x13c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d0_91_test_ff, 0x13d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d8_91_test_ff, 0x13d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e0_91_test_ff, 0x13e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e8_91_test_ff, 0x13e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f0_91_test_ff, 0x13f0, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_13f8_91_test_ff, 0x13f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f9_91_test_ff, 0x13f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fa_91_test_ff, 0x13fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fb_91_test_ff, 0x13fb, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_13fc_91_test_ff, 0x13fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2000_91_test_ff, 0x2000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2008_91_test_ff, 0x2008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2010_91_test_ff, 0x2010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2018_91_test_ff, 0x2018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2020_91_test_ff, 0x2020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2028_91_test_ff, 0x2028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2030_91_test_ff, 0x2030, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2038_91_test_ff, 0x2038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2039_91_test_ff, 0x2039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203a_91_test_ff, 0x203a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203b_91_test_ff, 0x203b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_203c_91_test_ff, 0x203c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2040_91_test_ff, 0x2040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2048_91_test_ff, 0x2048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2050_91_test_ff, 0x2050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2058_91_test_ff, 0x2058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2060_91_test_ff, 0x2060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2068_91_test_ff, 0x2068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2070_91_test_ff, 0x2070, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_2078_91_test_ff, 0x2078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2079_91_test_ff, 0x2079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207a_91_test_ff, 0x207a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207b_91_test_ff, 0x207b, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_207c_91_test_ff, 0x207c, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2080_91_test_ff, 0x2080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2088_91_test_ff, 0x2088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2090_91_test_ff, 0x2090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2098_91_test_ff, 0x2098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a0_91_test_ff, 0x20a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a8_91_test_ff, 0x20a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b0_91_test_ff, 0x20b0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20b8_91_test_ff, 0x20b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b9_91_test_ff, 0x20b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20ba_91_test_ff, 0x20ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20bb_91_test_ff, 0x20bb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20bc_91_test_ff, 0x20bc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c0_91_test_ff, 0x20c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c8_91_test_ff, 0x20c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d0_91_test_ff, 0x20d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d8_91_test_ff, 0x20d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e0_91_test_ff, 0x20e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e8_91_test_ff, 0x20e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f0_91_test_ff, 0x20f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20f8_91_test_ff, 0x20f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f9_91_test_ff, 0x20f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fa_91_test_ff, 0x20fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fb_91_test_ff, 0x20fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_20fc_91_test_ff, 0x20fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2100_91_test_ff, 0x2100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2108_91_test_ff, 0x2108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2110_91_test_ff, 0x2110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2118_91_test_ff, 0x2118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2120_91_test_ff, 0x2120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2128_91_test_ff, 0x2128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2130_91_test_ff, 0x2130, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2138_91_test_ff, 0x2138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2139_91_test_ff, 0x2139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213a_91_test_ff, 0x213a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213b_91_test_ff, 0x213b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_213c_91_test_ff, 0x213c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2140_91_test_ff, 0x2140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2148_91_test_ff, 0x2148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2150_91_test_ff, 0x2150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2158_91_test_ff, 0x2158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2160_91_test_ff, 0x2160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2168_91_test_ff, 0x2168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2170_91_test_ff, 0x2170, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_2178_91_test_ff, 0x2178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2179_91_test_ff, 0x2179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217a_91_test_ff, 0x217a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217b_91_test_ff, 0x217b, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_217c_91_test_ff, 0x217c, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2180_91_test_ff, 0x2180, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2188_91_test_ff, 0x2188, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2190_91_test_ff, 0x2190, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2198_91_test_ff, 0x2198, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21a0_91_test_ff, 0x21a0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21a8_91_test_ff, 0x21a8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21b0_91_test_ff, 0x21b0, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_21b8_91_test_ff, 0x21b8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21b9_91_test_ff, 0x21b9, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21ba_91_test_ff, 0x21ba, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21bb_91_test_ff, 0x21bb, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_21bc_91_test_ff, 0x21bc, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21c0_91_test_ff, 0x21c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21c8_91_test_ff, 0x21c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d0_91_test_ff, 0x21d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d8_91_test_ff, 0x21d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e0_91_test_ff, 0x21e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e8_91_test_ff, 0x21e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f0_91_test_ff, 0x21f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_21f8_91_test_ff, 0x21f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f9_91_test_ff, 0x21f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fa_91_test_ff, 0x21fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fb_91_test_ff, 0x21fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_21fc_91_test_ff, 0x21fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c0_91_test_ff, 0x23c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c8_91_test_ff, 0x23c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d0_91_test_ff, 0x23d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d8_91_test_ff, 0x23d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e0_91_test_ff, 0x23e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e8_91_test_ff, 0x23e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f0_91_test_ff, 0x23f0, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_23f8_91_test_ff, 0x23f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f9_91_test_ff, 0x23f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fa_91_test_ff, 0x23fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fb_91_test_ff, 0x23fb, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_23fc_91_test_ff, 0x23fc, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3000_91_test_ff, 0x3000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3008_91_test_ff, 0x3008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3010_91_test_ff, 0x3010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3018_91_test_ff, 0x3018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3020_91_test_ff, 0x3020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3028_91_test_ff, 0x3028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3030_91_test_ff, 0x3030, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3038_91_test_ff, 0x3038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3039_91_test_ff, 0x3039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303a_91_test_ff, 0x303a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303b_91_test_ff, 0x303b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_303c_91_test_ff, 0x303c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3040_91_test_ff, 0x3040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3048_91_test_ff, 0x3048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3050_91_test_ff, 0x3050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3058_91_test_ff, 0x3058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3060_91_test_ff, 0x3060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3068_91_test_ff, 0x3068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3070_91_test_ff, 0x3070, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_3078_91_test_ff, 0x3078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3079_91_test_ff, 0x3079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307a_91_test_ff, 0x307a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307b_91_test_ff, 0x307b, 4, { 4, 0 }, 0 }, /* MOVEA */
{ NULL, op_307c_91_test_ff, 0x307c, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3080_91_test_ff, 0x3080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3088_91_test_ff, 0x3088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3090_91_test_ff, 0x3090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3098_91_test_ff, 0x3098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a0_91_test_ff, 0x30a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a8_91_test_ff, 0x30a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b0_91_test_ff, 0x30b0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30b8_91_test_ff, 0x30b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b9_91_test_ff, 0x30b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30ba_91_test_ff, 0x30ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30bb_91_test_ff, 0x30bb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30bc_91_test_ff, 0x30bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c0_91_test_ff, 0x30c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c8_91_test_ff, 0x30c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d0_91_test_ff, 0x30d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d8_91_test_ff, 0x30d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e0_91_test_ff, 0x30e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e8_91_test_ff, 0x30e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f0_91_test_ff, 0x30f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30f8_91_test_ff, 0x30f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f9_91_test_ff, 0x30f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fa_91_test_ff, 0x30fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fb_91_test_ff, 0x30fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_30fc_91_test_ff, 0x30fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3100_91_test_ff, 0x3100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3108_91_test_ff, 0x3108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3110_91_test_ff, 0x3110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3118_91_test_ff, 0x3118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3120_91_test_ff, 0x3120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3128_91_test_ff, 0x3128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3130_91_test_ff, 0x3130, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3138_91_test_ff, 0x3138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3139_91_test_ff, 0x3139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313a_91_test_ff, 0x313a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313b_91_test_ff, 0x313b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_313c_91_test_ff, 0x313c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3140_91_test_ff, 0x3140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3148_91_test_ff, 0x3148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3150_91_test_ff, 0x3150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3158_91_test_ff, 0x3158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3160_91_test_ff, 0x3160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3168_91_test_ff, 0x3168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3170_91_test_ff, 0x3170, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_3178_91_test_ff, 0x3178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3179_91_test_ff, 0x3179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317a_91_test_ff, 0x317a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317b_91_test_ff, 0x317b, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_317c_91_test_ff, 0x317c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3180_91_test_ff, 0x3180, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3188_91_test_ff, 0x3188, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3190_91_test_ff, 0x3190, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3198_91_test_ff, 0x3198, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31a0_91_test_ff, 0x31a0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31a8_91_test_ff, 0x31a8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31b0_91_test_ff, 0x31b0, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_31b8_91_test_ff, 0x31b8, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31b9_91_test_ff, 0x31b9, 8, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31ba_91_test_ff, 0x31ba, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31bb_91_test_ff, 0x31bb, 6, { 6, 4 }, 0 }, /* MOVE */
{ NULL, op_31bc_91_test_ff, 0x31bc, 6, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31c0_91_test_ff, 0x31c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31c8_91_test_ff, 0x31c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d0_91_test_ff, 0x31d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d8_91_test_ff, 0x31d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e0_91_test_ff, 0x31e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e8_91_test_ff, 0x31e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f0_91_test_ff, 0x31f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_31f8_91_test_ff, 0x31f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f9_91_test_ff, 0x31f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fa_91_test_ff, 0x31fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fb_91_test_ff, 0x31fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_31fc_91_test_ff, 0x31fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c0_91_test_ff, 0x33c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c8_91_test_ff, 0x33c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d0_91_test_ff, 0x33d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d8_91_test_ff, 0x33d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e0_91_test_ff, 0x33e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e8_91_test_ff, 0x33e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f0_91_test_ff, 0x33f0, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_33f8_91_test_ff, 0x33f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f9_91_test_ff, 0x33f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fa_91_test_ff, 0x33fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fb_91_test_ff, 0x33fb, 8, { 8, 0 }, 0 }, /* MOVE */
{ NULL, op_33fc_91_test_ff, 0x33fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_4000_91_test_ff, 0x4000, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4010_91_test_ff, 0x4010, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4018_91_test_ff, 0x4018, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4020_91_test_ff, 0x4020, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4028_91_test_ff, 0x4028, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4030_91_test_ff, 0x4030, 4, { 4, 0 }, 0 }, /* NEGX */
{ NULL, op_4038_91_test_ff, 0x4038, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4039_91_test_ff, 0x4039, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4040_91_test_ff, 0x4040, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4050_91_test_ff, 0x4050, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4058_91_test_ff, 0x4058, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4060_91_test_ff, 0x4060, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4068_91_test_ff, 0x4068, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4070_91_test_ff, 0x4070, 4, { 4, 0 }, 0 }, /* NEGX */
{ NULL, op_4078_91_test_ff, 0x4078, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4079_91_test_ff, 0x4079, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4080_91_test_ff, 0x4080, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4090_91_test_ff, 0x4090, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4098_91_test_ff, 0x4098, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a0_91_test_ff, 0x40a0, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a8_91_test_ff, 0x40a8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b0_91_test_ff, 0x40b0, 4, { 4, 0 }, 0 }, /* NEGX */
{ NULL, op_40b8_91_test_ff, 0x40b8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b9_91_test_ff, 0x40b9, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40c0_91_test_ff, 0x40c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d0_91_test_ff, 0x40d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d8_91_test_ff, 0x40d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e0_91_test_ff, 0x40e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e8_91_test_ff, 0x40e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f0_91_test_ff, 0x40f0, 4, { 4, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f8_91_test_ff, 0x40f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f9_91_test_ff, 0x40f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_4180_91_test_ff, 0x4180, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4190_91_test_ff, 0x4190, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4198_91_test_ff, 0x4198, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a0_91_test_ff, 0x41a0, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a8_91_test_ff, 0x41a8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b0_91_test_ff, 0x41b0, 4, { 4, 0 }, 0 }, /* CHK */
{ NULL, op_41b8_91_test_ff, 0x41b8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b9_91_test_ff, 0x41b9, 6, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41ba_91_test_ff, 0x41ba, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41bb_91_test_ff, 0x41bb, 4, { 4, 0 }, 0 }, /* CHK */
{ NULL, op_41bc_91_test_ff, 0x41bc, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41d0_91_test_ff, 0x41d0, 2, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41e8_91_test_ff, 0x41e8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f0_91_test_ff, 0x41f0, 4, { 4, 0 }, 0 }, /* LEA */
{ NULL, op_41f8_91_test_ff, 0x41f8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f9_91_test_ff, 0x41f9, 6, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fa_91_test_ff, 0x41fa, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fb_91_test_ff, 0x41fb, 4, { 4, 0 }, 0 }, /* LEA */
{ NULL, op_4200_91_test_ff, 0x4200, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4210_91_test_ff, 0x4210, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4218_91_test_ff, 0x4218, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4220_91_test_ff, 0x4220, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4228_91_test_ff, 0x4228, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4230_91_test_ff, 0x4230, 4, { 4, 0 }, 0 }, /* CLR */
{ NULL, op_4238_91_test_ff, 0x4238, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4239_91_test_ff, 0x4239, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4240_91_test_ff, 0x4240, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4250_91_test_ff, 0x4250, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4258_91_test_ff, 0x4258, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4260_91_test_ff, 0x4260, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4268_91_test_ff, 0x4268, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4270_91_test_ff, 0x4270, 4, { 4, 0 }, 0 }, /* CLR */
{ NULL, op_4278_91_test_ff, 0x4278, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4279_91_test_ff, 0x4279, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4280_91_test_ff, 0x4280, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4290_91_test_ff, 0x4290, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4298_91_test_ff, 0x4298, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a0_91_test_ff, 0x42a0, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a8_91_test_ff, 0x42a8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b0_91_test_ff, 0x42b0, 4, { 4, 0 }, 0 }, /* CLR */
{ NULL, op_42b8_91_test_ff, 0x42b8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b9_91_test_ff, 0x42b9, 6, { 0, 0 }, 0 }, /* CLR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42c0_91_test_ff, 0x42c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d0_91_test_ff, 0x42d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d8_91_test_ff, 0x42d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e0_91_test_ff, 0x42e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e8_91_test_ff, 0x42e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f0_91_test_ff, 0x42f0, 4, { 4, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f8_91_test_ff, 0x42f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f9_91_test_ff, 0x42f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#endif
{ NULL, op_4400_91_test_ff, 0x4400, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4410_91_test_ff, 0x4410, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4418_91_test_ff, 0x4418, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4420_91_test_ff, 0x4420, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4428_91_test_ff, 0x4428, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4430_91_test_ff, 0x4430, 4, { 4, 0 }, 0 }, /* NEG */
{ NULL, op_4438_91_test_ff, 0x4438, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4439_91_test_ff, 0x4439, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4440_91_test_ff, 0x4440, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4450_91_test_ff, 0x4450, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4458_91_test_ff, 0x4458, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4460_91_test_ff, 0x4460, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4468_91_test_ff, 0x4468, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4470_91_test_ff, 0x4470, 4, { 4, 0 }, 0 }, /* NEG */
{ NULL, op_4478_91_test_ff, 0x4478, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4479_91_test_ff, 0x4479, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4480_91_test_ff, 0x4480, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4490_91_test_ff, 0x4490, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4498_91_test_ff, 0x4498, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a0_91_test_ff, 0x44a0, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a8_91_test_ff, 0x44a8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b0_91_test_ff, 0x44b0, 4, { 4, 0 }, 0 }, /* NEG */
{ NULL, op_44b8_91_test_ff, 0x44b8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b9_91_test_ff, 0x44b9, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44c0_91_test_ff, 0x44c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d0_91_test_ff, 0x44d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d8_91_test_ff, 0x44d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e0_91_test_ff, 0x44e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e8_91_test_ff, 0x44e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f0_91_test_ff, 0x44f0, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f8_91_test_ff, 0x44f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f9_91_test_ff, 0x44f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fa_91_test_ff, 0x44fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fb_91_test_ff, 0x44fb, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fc_91_test_ff, 0x44fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4600_91_test_ff, 0x4600, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4610_91_test_ff, 0x4610, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4618_91_test_ff, 0x4618, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4620_91_test_ff, 0x4620, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4628_91_test_ff, 0x4628, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4630_91_test_ff, 0x4630, 4, { 4, 0 }, 0 }, /* NOT */
{ NULL, op_4638_91_test_ff, 0x4638, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4639_91_test_ff, 0x4639, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4640_91_test_ff, 0x4640, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4650_91_test_ff, 0x4650, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4658_91_test_ff, 0x4658, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4660_91_test_ff, 0x4660, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4668_91_test_ff, 0x4668, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4670_91_test_ff, 0x4670, 4, { 4, 0 }, 0 }, /* NOT */
{ NULL, op_4678_91_test_ff, 0x4678, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4679_91_test_ff, 0x4679, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4680_91_test_ff, 0x4680, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4690_91_test_ff, 0x4690, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4698_91_test_ff, 0x4698, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a0_91_test_ff, 0x46a0, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a8_91_test_ff, 0x46a8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b0_91_test_ff, 0x46b0, 4, { 4, 0 }, 0 }, /* NOT */
{ NULL, op_46b8_91_test_ff, 0x46b8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b9_91_test_ff, 0x46b9, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46c0_91_test_ff, 0x46c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d0_91_test_ff, 0x46d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d8_91_test_ff, 0x46d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e0_91_test_ff, 0x46e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e8_91_test_ff, 0x46e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f0_91_test_ff, 0x46f0, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f8_91_test_ff, 0x46f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f9_91_test_ff, 0x46f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fa_91_test_ff, 0x46fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fb_91_test_ff, 0x46fb, 4, { 4, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fc_91_test_ff, 0x46fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4800_91_test_ff, 0x4800, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4810_91_test_ff, 0x4810, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4818_91_test_ff, 0x4818, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4820_91_test_ff, 0x4820, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4828_91_test_ff, 0x4828, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4830_91_test_ff, 0x4830, 4, { 4, 0 }, 0 }, /* NBCD */
{ NULL, op_4838_91_test_ff, 0x4838, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4839_91_test_ff, 0x4839, 6, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4840_91_test_ff, 0x4840, 2, { 0, 0 }, 0 }, /* SWAP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4848_91_test_ff, 0x4848, 2, { 0, 0 }, 0 }, /* BKPT */
#endif
{ NULL, op_4850_91_test_ff, 0x4850, 2, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4868_91_test_ff, 0x4868, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4870_91_test_ff, 0x4870, 4, { 4, 0 }, 0 }, /* PEA */
{ NULL, op_4878_91_test_ff, 0x4878, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4879_91_test_ff, 0x4879, 6, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487a_91_test_ff, 0x487a, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487b_91_test_ff, 0x487b, 4, { 4, 0 }, 0 }, /* PEA */
{ NULL, op_4880_91_test_ff, 0x4880, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_4890_91_test_ff, 0x4890, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a0_91_test_ff, 0x48a0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a8_91_test_ff, 0x48a8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b0_91_test_ff, 0x48b0, 6, { 4, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b8_91_test_ff, 0x48b8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b9_91_test_ff, 0x48b9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48c0_91_test_ff, 0x48c0, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_48d0_91_test_ff, 0x48d0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e0_91_test_ff, 0x48e0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e8_91_test_ff, 0x48e8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f0_91_test_ff, 0x48f0, 6, { 4, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f8_91_test_ff, 0x48f8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f9_91_test_ff, 0x48f9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_4a00_91_test_ff, 0x4a00, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a10_91_test_ff, 0x4a10, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a18_91_test_ff, 0x4a18, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a20_91_test_ff, 0x4a20, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a28_91_test_ff, 0x4a28, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a30_91_test_ff, 0x4a30, 4, { 4, 0 }, 0 }, /* TST */
{ NULL, op_4a38_91_test_ff, 0x4a38, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a39_91_test_ff, 0x4a39, 6, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a40_91_test_ff, 0x4a40, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a50_91_test_ff, 0x4a50, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a58_91_test_ff, 0x4a58, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a60_91_test_ff, 0x4a60, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a68_91_test_ff, 0x4a68, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a70_91_test_ff, 0x4a70, 4, { 4, 0 }, 0 }, /* TST */
{ NULL, op_4a78_91_test_ff, 0x4a78, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a79_91_test_ff, 0x4a79, 6, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a80_91_test_ff, 0x4a80, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a90_91_test_ff, 0x4a90, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a98_91_test_ff, 0x4a98, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa0_91_test_ff, 0x4aa0, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa8_91_test_ff, 0x4aa8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab0_91_test_ff, 0x4ab0, 4, { 4, 0 }, 0 }, /* TST */
{ NULL, op_4ab8_91_test_ff, 0x4ab8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab9_91_test_ff, 0x4ab9, 6, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ac0_91_test_ff, 0x4ac0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad0_91_test_ff, 0x4ad0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad8_91_test_ff, 0x4ad8, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae0_91_test_ff, 0x4ae0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae8_91_test_ff, 0x4ae8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af0_91_test_ff, 0x4af0, 4, { 4, 0 }, 0 }, /* TAS */
{ NULL, op_4af8_91_test_ff, 0x4af8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af9_91_test_ff, 0x4af9, 6, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4c90_91_test_ff, 0x4c90, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4c98_91_test_ff, 0x4c98, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ca8_91_test_ff, 0x4ca8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb0_91_test_ff, 0x4cb0, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb8_91_test_ff, 0x4cb8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb9_91_test_ff, 0x4cb9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cba_91_test_ff, 0x4cba, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cbb_91_test_ff, 0x4cbb, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd0_91_test_ff, 0x4cd0, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd8_91_test_ff, 0x4cd8, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ce8_91_test_ff, 0x4ce8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf0_91_test_ff, 0x4cf0, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf8_91_test_ff, 0x4cf8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf9_91_test_ff, 0x4cf9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfa_91_test_ff, 0x4cfa, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfb_91_test_ff, 0x4cfb, 6, { 4, 0 }, 0 }, /* MVMEL */
{ NULL, op_4e40_91_test_ff, 0x4e40, 2, { 0, 0 }, 0 }, /* TRAP */
{ NULL, op_4e50_91_test_ff, 0x4e50, 4, { 0, 0 }, 0 }, /* LINK */
{ NULL, op_4e58_91_test_ff, 0x4e58, 2, { 0, 0 }, 0 }, /* UNLK */
{ NULL, op_4e60_91_test_ff, 0x4e60, 2, { 0, 0 }, 0 }, /* MVR2USP */
{ NULL, op_4e68_91_test_ff, 0x4e68, 2, { 0, 0 }, 0 }, /* MVUSP2R */
{ NULL, op_4e70_91_test_ff, 0x4e70, 2, { 0, 0 }, 0 }, /* RESET */
{ NULL, op_4e71_91_test_ff, 0x4e71, 2, { 0, 0 }, 0 }, /* NOP */
{ NULL, op_4e72_91_test_ff, 0x4e72, 0, { 0, 0 }, 0 }, /* STOP */
{ NULL, op_4e73_91_test_ff, 0x4e73, 2, { 0, 0 }, 1 }, /* RTE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e74_91_test_ff, 0x4e74, 4, { 0, 0 }, 2 }, /* RTD */
#endif
{ NULL, op_4e75_91_test_ff, 0x4e75, 2, { 0, 0 }, 1 }, /* RTS */
{ NULL, op_4e76_91_test_ff, 0x4e76, 2, { 0, 0 }, 0 }, /* TRAPV */
{ NULL, op_4e77_91_test_ff, 0x4e77, 2, { 0, 0 }, 1 }, /* RTR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7a_91_test_ff, 0x4e7a, 4, { 0, 0 }, 0 }, /* MOVEC2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7b_91_test_ff, 0x4e7b, 4, { 0, 0 }, 0 }, /* MOVE2C */
#endif
{ NULL, op_4e90_91_test_ff, 0x4e90, 2, { 0, 0 }, 1 }, /* JSR */
{ NULL, op_4ea8_91_test_ff, 0x4ea8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb0_91_test_ff, 0x4eb0, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4eb8_91_test_ff, 0x4eb8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb9_91_test_ff, 0x4eb9, 6, { 0, 0 }, 3 }, /* JSR */
{ NULL, op_4eba_91_test_ff, 0x4eba, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4ebb_91_test_ff, 0x4ebb, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4ed0_91_test_ff, 0x4ed0, 2, { 0, 0 }, 1 }, /* JMP */
{ NULL, op_4ee8_91_test_ff, 0x4ee8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef0_91_test_ff, 0x4ef0, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_4ef8_91_test_ff, 0x4ef8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef9_91_test_ff, 0x4ef9, 6, { 0, 0 }, 3 }, /* JMP */
{ NULL, op_4efa_91_test_ff, 0x4efa, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4efb_91_test_ff, 0x4efb, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_5000_91_test_ff, 0x5000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5010_91_test_ff, 0x5010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5018_91_test_ff, 0x5018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5020_91_test_ff, 0x5020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5028_91_test_ff, 0x5028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5030_91_test_ff, 0x5030, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_5038_91_test_ff, 0x5038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5039_91_test_ff, 0x5039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5040_91_test_ff, 0x5040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5048_91_test_ff, 0x5048, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5050_91_test_ff, 0x5050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5058_91_test_ff, 0x5058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5060_91_test_ff, 0x5060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5068_91_test_ff, 0x5068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5070_91_test_ff, 0x5070, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_5078_91_test_ff, 0x5078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5079_91_test_ff, 0x5079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5080_91_test_ff, 0x5080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5088_91_test_ff, 0x5088, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5090_91_test_ff, 0x5090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5098_91_test_ff, 0x5098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a0_91_test_ff, 0x50a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a8_91_test_ff, 0x50a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b0_91_test_ff, 0x50b0, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_50b8_91_test_ff, 0x50b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b9_91_test_ff, 0x50b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50c0_91_test_ff, 0x50c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50c8_91_test_ff, 0x50c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_50d0_91_test_ff, 0x50d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50d8_91_test_ff, 0x50d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e0_91_test_ff, 0x50e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e8_91_test_ff, 0x50e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f0_91_test_ff, 0x50f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_50f8_91_test_ff, 0x50f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f9_91_test_ff, 0x50f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5100_91_test_ff, 0x5100, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5110_91_test_ff, 0x5110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5118_91_test_ff, 0x5118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5120_91_test_ff, 0x5120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5128_91_test_ff, 0x5128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5130_91_test_ff, 0x5130, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_5138_91_test_ff, 0x5138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5139_91_test_ff, 0x5139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5140_91_test_ff, 0x5140, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5148_91_test_ff, 0x5148, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5150_91_test_ff, 0x5150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5158_91_test_ff, 0x5158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5160_91_test_ff, 0x5160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5168_91_test_ff, 0x5168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5170_91_test_ff, 0x5170, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_5178_91_test_ff, 0x5178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5179_91_test_ff, 0x5179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5180_91_test_ff, 0x5180, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5188_91_test_ff, 0x5188, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5190_91_test_ff, 0x5190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5198_91_test_ff, 0x5198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a0_91_test_ff, 0x51a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a8_91_test_ff, 0x51a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b0_91_test_ff, 0x51b0, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_51b8_91_test_ff, 0x51b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b9_91_test_ff, 0x51b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51c0_91_test_ff, 0x51c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51c8_91_test_ff, 0x51c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_51d0_91_test_ff, 0x51d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51d8_91_test_ff, 0x51d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e0_91_test_ff, 0x51e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e8_91_test_ff, 0x51e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f0_91_test_ff, 0x51f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_51f8_91_test_ff, 0x51f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f9_91_test_ff, 0x51f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c0_91_test_ff, 0x52c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c8_91_test_ff, 0x52c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_52d0_91_test_ff, 0x52d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52d8_91_test_ff, 0x52d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e0_91_test_ff, 0x52e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e8_91_test_ff, 0x52e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f0_91_test_ff, 0x52f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_52f8_91_test_ff, 0x52f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f9_91_test_ff, 0x52f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c0_91_test_ff, 0x53c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c8_91_test_ff, 0x53c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_53d0_91_test_ff, 0x53d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53d8_91_test_ff, 0x53d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e0_91_test_ff, 0x53e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e8_91_test_ff, 0x53e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f0_91_test_ff, 0x53f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_53f8_91_test_ff, 0x53f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f9_91_test_ff, 0x53f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c0_91_test_ff, 0x54c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c8_91_test_ff, 0x54c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_54d0_91_test_ff, 0x54d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54d8_91_test_ff, 0x54d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e0_91_test_ff, 0x54e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e8_91_test_ff, 0x54e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f0_91_test_ff, 0x54f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_54f8_91_test_ff, 0x54f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f9_91_test_ff, 0x54f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c0_91_test_ff, 0x55c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c8_91_test_ff, 0x55c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_55d0_91_test_ff, 0x55d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55d8_91_test_ff, 0x55d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e0_91_test_ff, 0x55e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e8_91_test_ff, 0x55e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f0_91_test_ff, 0x55f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_55f8_91_test_ff, 0x55f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f9_91_test_ff, 0x55f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c0_91_test_ff, 0x56c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c8_91_test_ff, 0x56c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_56d0_91_test_ff, 0x56d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56d8_91_test_ff, 0x56d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e0_91_test_ff, 0x56e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e8_91_test_ff, 0x56e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f0_91_test_ff, 0x56f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_56f8_91_test_ff, 0x56f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f9_91_test_ff, 0x56f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c0_91_test_ff, 0x57c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c8_91_test_ff, 0x57c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_57d0_91_test_ff, 0x57d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57d8_91_test_ff, 0x57d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e0_91_test_ff, 0x57e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e8_91_test_ff, 0x57e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f0_91_test_ff, 0x57f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_57f8_91_test_ff, 0x57f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f9_91_test_ff, 0x57f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c0_91_test_ff, 0x58c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c8_91_test_ff, 0x58c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_58d0_91_test_ff, 0x58d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58d8_91_test_ff, 0x58d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e0_91_test_ff, 0x58e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e8_91_test_ff, 0x58e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f0_91_test_ff, 0x58f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_58f8_91_test_ff, 0x58f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f9_91_test_ff, 0x58f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c0_91_test_ff, 0x59c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c8_91_test_ff, 0x59c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_59d0_91_test_ff, 0x59d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59d8_91_test_ff, 0x59d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e0_91_test_ff, 0x59e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e8_91_test_ff, 0x59e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f0_91_test_ff, 0x59f0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_59f8_91_test_ff, 0x59f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f9_91_test_ff, 0x59f9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac0_91_test_ff, 0x5ac0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac8_91_test_ff, 0x5ac8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ad0_91_test_ff, 0x5ad0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ad8_91_test_ff, 0x5ad8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae0_91_test_ff, 0x5ae0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae8_91_test_ff, 0x5ae8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af0_91_test_ff, 0x5af0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5af8_91_test_ff, 0x5af8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af9_91_test_ff, 0x5af9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc0_91_test_ff, 0x5bc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc8_91_test_ff, 0x5bc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5bd0_91_test_ff, 0x5bd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bd8_91_test_ff, 0x5bd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be0_91_test_ff, 0x5be0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be8_91_test_ff, 0x5be8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf0_91_test_ff, 0x5bf0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5bf8_91_test_ff, 0x5bf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf9_91_test_ff, 0x5bf9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc0_91_test_ff, 0x5cc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc8_91_test_ff, 0x5cc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5cd0_91_test_ff, 0x5cd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cd8_91_test_ff, 0x5cd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce0_91_test_ff, 0x5ce0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce8_91_test_ff, 0x5ce8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf0_91_test_ff, 0x5cf0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5cf8_91_test_ff, 0x5cf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf9_91_test_ff, 0x5cf9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc0_91_test_ff, 0x5dc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc8_91_test_ff, 0x5dc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5dd0_91_test_ff, 0x5dd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dd8_91_test_ff, 0x5dd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de0_91_test_ff, 0x5de0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de8_91_test_ff, 0x5de8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df0_91_test_ff, 0x5df0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5df8_91_test_ff, 0x5df8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df9_91_test_ff, 0x5df9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec0_91_test_ff, 0x5ec0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec8_91_test_ff, 0x5ec8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ed0_91_test_ff, 0x5ed0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ed8_91_test_ff, 0x5ed8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee0_91_test_ff, 0x5ee0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee8_91_test_ff, 0x5ee8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef0_91_test_ff, 0x5ef0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5ef8_91_test_ff, 0x5ef8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef9_91_test_ff, 0x5ef9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc0_91_test_ff, 0x5fc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc8_91_test_ff, 0x5fc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5fd0_91_test_ff, 0x5fd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fd8_91_test_ff, 0x5fd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe0_91_test_ff, 0x5fe0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe8_91_test_ff, 0x5fe8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff0_91_test_ff, 0x5ff0, 4, { 4, 0 }, 0 }, /* Scc */
{ NULL, op_5ff8_91_test_ff, 0x5ff8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff9_91_test_ff, 0x5ff9, 6, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_6000_91_test_ff, 0x6000, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6001_91_test_ff, 0x6001, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_60ff_91_test_ff, 0x60ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6100_91_test_ff, 0x6100, 4, { 0, 0 }, 2 }, /* BSR */
{ NULL, op_6101_91_test_ff, 0x6101, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_61ff_91_test_ff, 0x61ff, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_6200_91_test_ff, 0x6200, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6201_91_test_ff, 0x6201, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_62ff_91_test_ff, 0x62ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6300_91_test_ff, 0x6300, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6301_91_test_ff, 0x6301, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_63ff_91_test_ff, 0x63ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6400_91_test_ff, 0x6400, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6401_91_test_ff, 0x6401, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_64ff_91_test_ff, 0x64ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6500_91_test_ff, 0x6500, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6501_91_test_ff, 0x6501, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_65ff_91_test_ff, 0x65ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6600_91_test_ff, 0x6600, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6601_91_test_ff, 0x6601, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_66ff_91_test_ff, 0x66ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6700_91_test_ff, 0x6700, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6701_91_test_ff, 0x6701, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_67ff_91_test_ff, 0x67ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6800_91_test_ff, 0x6800, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6801_91_test_ff, 0x6801, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_68ff_91_test_ff, 0x68ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6900_91_test_ff, 0x6900, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6901_91_test_ff, 0x6901, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_69ff_91_test_ff, 0x69ff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6a00_91_test_ff, 0x6a00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6a01_91_test_ff, 0x6a01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6aff_91_test_ff, 0x6aff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6b00_91_test_ff, 0x6b00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6b01_91_test_ff, 0x6b01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6bff_91_test_ff, 0x6bff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6c00_91_test_ff, 0x6c00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6c01_91_test_ff, 0x6c01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6cff_91_test_ff, 0x6cff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6d00_91_test_ff, 0x6d00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6d01_91_test_ff, 0x6d01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6dff_91_test_ff, 0x6dff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6e00_91_test_ff, 0x6e00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6e01_91_test_ff, 0x6e01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6eff_91_test_ff, 0x6eff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_6f00_91_test_ff, 0x6f00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6f01_91_test_ff, 0x6f01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6fff_91_test_ff, 0x6fff, 2, { 0, 0 }, 0 }, /* Bcc */
{ NULL, op_7000_91_test_ff, 0x7000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_8000_91_test_ff, 0x8000, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8010_91_test_ff, 0x8010, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8018_91_test_ff, 0x8018, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8020_91_test_ff, 0x8020, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8028_91_test_ff, 0x8028, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8030_91_test_ff, 0x8030, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8038_91_test_ff, 0x8038, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8039_91_test_ff, 0x8039, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803a_91_test_ff, 0x803a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803b_91_test_ff, 0x803b, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_803c_91_test_ff, 0x803c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8040_91_test_ff, 0x8040, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8050_91_test_ff, 0x8050, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8058_91_test_ff, 0x8058, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8060_91_test_ff, 0x8060, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8068_91_test_ff, 0x8068, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8070_91_test_ff, 0x8070, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8078_91_test_ff, 0x8078, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8079_91_test_ff, 0x8079, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807a_91_test_ff, 0x807a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807b_91_test_ff, 0x807b, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_807c_91_test_ff, 0x807c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8080_91_test_ff, 0x8080, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8090_91_test_ff, 0x8090, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8098_91_test_ff, 0x8098, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a0_91_test_ff, 0x80a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a8_91_test_ff, 0x80a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b0_91_test_ff, 0x80b0, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_80b8_91_test_ff, 0x80b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b9_91_test_ff, 0x80b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80ba_91_test_ff, 0x80ba, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80bb_91_test_ff, 0x80bb, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_80bc_91_test_ff, 0x80bc, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80c0_91_test_ff, 0x80c0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d0_91_test_ff, 0x80d0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d8_91_test_ff, 0x80d8, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e0_91_test_ff, 0x80e0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e8_91_test_ff, 0x80e8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f0_91_test_ff, 0x80f0, 4, { 4, 0 }, 0 }, /* DIVU */
{ NULL, op_80f8_91_test_ff, 0x80f8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f9_91_test_ff, 0x80f9, 6, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fa_91_test_ff, 0x80fa, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fb_91_test_ff, 0x80fb, 4, { 4, 0 }, 0 }, /* DIVU */
{ NULL, op_80fc_91_test_ff, 0x80fc, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_8100_91_test_ff, 0x8100, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8108_91_test_ff, 0x8108, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8110_91_test_ff, 0x8110, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8118_91_test_ff, 0x8118, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8120_91_test_ff, 0x8120, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8128_91_test_ff, 0x8128, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8130_91_test_ff, 0x8130, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8138_91_test_ff, 0x8138, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8139_91_test_ff, 0x8139, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8150_91_test_ff, 0x8150, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8158_91_test_ff, 0x8158, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8160_91_test_ff, 0x8160, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8168_91_test_ff, 0x8168, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8170_91_test_ff, 0x8170, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_8178_91_test_ff, 0x8178, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8179_91_test_ff, 0x8179, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8190_91_test_ff, 0x8190, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8198_91_test_ff, 0x8198, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a0_91_test_ff, 0x81a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a8_91_test_ff, 0x81a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b0_91_test_ff, 0x81b0, 4, { 4, 0 }, 0 }, /* OR */
{ NULL, op_81b8_91_test_ff, 0x81b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b9_91_test_ff, 0x81b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81c0_91_test_ff, 0x81c0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d0_91_test_ff, 0x81d0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d8_91_test_ff, 0x81d8, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e0_91_test_ff, 0x81e0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e8_91_test_ff, 0x81e8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f0_91_test_ff, 0x81f0, 4, { 4, 0 }, 0 }, /* DIVS */
{ NULL, op_81f8_91_test_ff, 0x81f8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f9_91_test_ff, 0x81f9, 6, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fa_91_test_ff, 0x81fa, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fb_91_test_ff, 0x81fb, 4, { 4, 0 }, 0 }, /* DIVS */
{ NULL, op_81fc_91_test_ff, 0x81fc, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_9000_91_test_ff, 0x9000, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9010_91_test_ff, 0x9010, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9018_91_test_ff, 0x9018, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9020_91_test_ff, 0x9020, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9028_91_test_ff, 0x9028, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9030_91_test_ff, 0x9030, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9038_91_test_ff, 0x9038, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9039_91_test_ff, 0x9039, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903a_91_test_ff, 0x903a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903b_91_test_ff, 0x903b, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_903c_91_test_ff, 0x903c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9040_91_test_ff, 0x9040, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9048_91_test_ff, 0x9048, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9050_91_test_ff, 0x9050, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9058_91_test_ff, 0x9058, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9060_91_test_ff, 0x9060, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9068_91_test_ff, 0x9068, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9070_91_test_ff, 0x9070, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9078_91_test_ff, 0x9078, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9079_91_test_ff, 0x9079, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907a_91_test_ff, 0x907a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907b_91_test_ff, 0x907b, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_907c_91_test_ff, 0x907c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9080_91_test_ff, 0x9080, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9088_91_test_ff, 0x9088, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9090_91_test_ff, 0x9090, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9098_91_test_ff, 0x9098, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a0_91_test_ff, 0x90a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a8_91_test_ff, 0x90a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b0_91_test_ff, 0x90b0, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_90b8_91_test_ff, 0x90b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b9_91_test_ff, 0x90b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90ba_91_test_ff, 0x90ba, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90bb_91_test_ff, 0x90bb, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_90bc_91_test_ff, 0x90bc, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90c0_91_test_ff, 0x90c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90c8_91_test_ff, 0x90c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d0_91_test_ff, 0x90d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d8_91_test_ff, 0x90d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e0_91_test_ff, 0x90e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e8_91_test_ff, 0x90e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f0_91_test_ff, 0x90f0, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_90f8_91_test_ff, 0x90f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f9_91_test_ff, 0x90f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fa_91_test_ff, 0x90fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fb_91_test_ff, 0x90fb, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_90fc_91_test_ff, 0x90fc, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_9100_91_test_ff, 0x9100, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9108_91_test_ff, 0x9108, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9110_91_test_ff, 0x9110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9118_91_test_ff, 0x9118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9120_91_test_ff, 0x9120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9128_91_test_ff, 0x9128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9130_91_test_ff, 0x9130, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9138_91_test_ff, 0x9138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9139_91_test_ff, 0x9139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9140_91_test_ff, 0x9140, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9148_91_test_ff, 0x9148, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9150_91_test_ff, 0x9150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9158_91_test_ff, 0x9158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9160_91_test_ff, 0x9160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9168_91_test_ff, 0x9168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9170_91_test_ff, 0x9170, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_9178_91_test_ff, 0x9178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9179_91_test_ff, 0x9179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9180_91_test_ff, 0x9180, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9188_91_test_ff, 0x9188, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9190_91_test_ff, 0x9190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9198_91_test_ff, 0x9198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a0_91_test_ff, 0x91a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a8_91_test_ff, 0x91a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b0_91_test_ff, 0x91b0, 4, { 4, 0 }, 0 }, /* SUB */
{ NULL, op_91b8_91_test_ff, 0x91b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b9_91_test_ff, 0x91b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91c0_91_test_ff, 0x91c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91c8_91_test_ff, 0x91c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d0_91_test_ff, 0x91d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d8_91_test_ff, 0x91d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e0_91_test_ff, 0x91e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e8_91_test_ff, 0x91e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f0_91_test_ff, 0x91f0, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_91f8_91_test_ff, 0x91f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f9_91_test_ff, 0x91f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fa_91_test_ff, 0x91fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fb_91_test_ff, 0x91fb, 4, { 4, 0 }, 0 }, /* SUBA */
{ NULL, op_91fc_91_test_ff, 0x91fc, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_b000_91_test_ff, 0xb000, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b010_91_test_ff, 0xb010, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b018_91_test_ff, 0xb018, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b020_91_test_ff, 0xb020, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b028_91_test_ff, 0xb028, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b030_91_test_ff, 0xb030, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b038_91_test_ff, 0xb038, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b039_91_test_ff, 0xb039, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03a_91_test_ff, 0xb03a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03b_91_test_ff, 0xb03b, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b03c_91_test_ff, 0xb03c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b040_91_test_ff, 0xb040, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b048_91_test_ff, 0xb048, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b050_91_test_ff, 0xb050, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b058_91_test_ff, 0xb058, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b060_91_test_ff, 0xb060, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b068_91_test_ff, 0xb068, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b070_91_test_ff, 0xb070, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b078_91_test_ff, 0xb078, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b079_91_test_ff, 0xb079, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07a_91_test_ff, 0xb07a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07b_91_test_ff, 0xb07b, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b07c_91_test_ff, 0xb07c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b080_91_test_ff, 0xb080, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b088_91_test_ff, 0xb088, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b090_91_test_ff, 0xb090, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b098_91_test_ff, 0xb098, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a0_91_test_ff, 0xb0a0, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a8_91_test_ff, 0xb0a8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b0_91_test_ff, 0xb0b0, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b0b8_91_test_ff, 0xb0b8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b9_91_test_ff, 0xb0b9, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0ba_91_test_ff, 0xb0ba, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0bb_91_test_ff, 0xb0bb, 4, { 4, 0 }, 0 }, /* CMP */
{ NULL, op_b0bc_91_test_ff, 0xb0bc, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0c0_91_test_ff, 0xb0c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0c8_91_test_ff, 0xb0c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d0_91_test_ff, 0xb0d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d8_91_test_ff, 0xb0d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e0_91_test_ff, 0xb0e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e8_91_test_ff, 0xb0e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f0_91_test_ff, 0xb0f0, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f8_91_test_ff, 0xb0f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f9_91_test_ff, 0xb0f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fa_91_test_ff, 0xb0fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fb_91_test_ff, 0xb0fb, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fc_91_test_ff, 0xb0fc, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b100_91_test_ff, 0xb100, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b108_91_test_ff, 0xb108, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b110_91_test_ff, 0xb110, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b118_91_test_ff, 0xb118, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b120_91_test_ff, 0xb120, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b128_91_test_ff, 0xb128, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b130_91_test_ff, 0xb130, 4, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_b138_91_test_ff, 0xb138, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b139_91_test_ff, 0xb139, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b140_91_test_ff, 0xb140, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b148_91_test_ff, 0xb148, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b150_91_test_ff, 0xb150, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b158_91_test_ff, 0xb158, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b160_91_test_ff, 0xb160, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b168_91_test_ff, 0xb168, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b170_91_test_ff, 0xb170, 4, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_b178_91_test_ff, 0xb178, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b179_91_test_ff, 0xb179, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b180_91_test_ff, 0xb180, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b188_91_test_ff, 0xb188, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b190_91_test_ff, 0xb190, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b198_91_test_ff, 0xb198, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a0_91_test_ff, 0xb1a0, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a8_91_test_ff, 0xb1a8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b0_91_test_ff, 0xb1b0, 4, { 4, 0 }, 0 }, /* EOR */
{ NULL, op_b1b8_91_test_ff, 0xb1b8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b9_91_test_ff, 0xb1b9, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1c0_91_test_ff, 0xb1c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1c8_91_test_ff, 0xb1c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d0_91_test_ff, 0xb1d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d8_91_test_ff, 0xb1d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e0_91_test_ff, 0xb1e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e8_91_test_ff, 0xb1e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f0_91_test_ff, 0xb1f0, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f8_91_test_ff, 0xb1f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f9_91_test_ff, 0xb1f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fa_91_test_ff, 0xb1fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fb_91_test_ff, 0xb1fb, 4, { 4, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fc_91_test_ff, 0xb1fc, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_c000_91_test_ff, 0xc000, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c010_91_test_ff, 0xc010, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c018_91_test_ff, 0xc018, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c020_91_test_ff, 0xc020, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c028_91_test_ff, 0xc028, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c030_91_test_ff, 0xc030, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c038_91_test_ff, 0xc038, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c039_91_test_ff, 0xc039, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03a_91_test_ff, 0xc03a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03b_91_test_ff, 0xc03b, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c03c_91_test_ff, 0xc03c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c040_91_test_ff, 0xc040, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c050_91_test_ff, 0xc050, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c058_91_test_ff, 0xc058, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c060_91_test_ff, 0xc060, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c068_91_test_ff, 0xc068, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c070_91_test_ff, 0xc070, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c078_91_test_ff, 0xc078, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c079_91_test_ff, 0xc079, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07a_91_test_ff, 0xc07a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07b_91_test_ff, 0xc07b, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c07c_91_test_ff, 0xc07c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c080_91_test_ff, 0xc080, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c090_91_test_ff, 0xc090, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c098_91_test_ff, 0xc098, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a0_91_test_ff, 0xc0a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a8_91_test_ff, 0xc0a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b0_91_test_ff, 0xc0b0, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c0b8_91_test_ff, 0xc0b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b9_91_test_ff, 0xc0b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0ba_91_test_ff, 0xc0ba, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0bb_91_test_ff, 0xc0bb, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c0bc_91_test_ff, 0xc0bc, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0c0_91_test_ff, 0xc0c0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d0_91_test_ff, 0xc0d0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d8_91_test_ff, 0xc0d8, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e0_91_test_ff, 0xc0e0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e8_91_test_ff, 0xc0e8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f0_91_test_ff, 0xc0f0, 4, { 4, 0 }, 0 }, /* MULU */
{ NULL, op_c0f8_91_test_ff, 0xc0f8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f9_91_test_ff, 0xc0f9, 6, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fa_91_test_ff, 0xc0fa, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fb_91_test_ff, 0xc0fb, 4, { 4, 0 }, 0 }, /* MULU */
{ NULL, op_c0fc_91_test_ff, 0xc0fc, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c100_91_test_ff, 0xc100, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c108_91_test_ff, 0xc108, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c110_91_test_ff, 0xc110, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c118_91_test_ff, 0xc118, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c120_91_test_ff, 0xc120, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c128_91_test_ff, 0xc128, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c130_91_test_ff, 0xc130, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c138_91_test_ff, 0xc138, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c139_91_test_ff, 0xc139, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c140_91_test_ff, 0xc140, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c148_91_test_ff, 0xc148, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c150_91_test_ff, 0xc150, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c158_91_test_ff, 0xc158, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c160_91_test_ff, 0xc160, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c168_91_test_ff, 0xc168, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c170_91_test_ff, 0xc170, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c178_91_test_ff, 0xc178, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c179_91_test_ff, 0xc179, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c188_91_test_ff, 0xc188, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c190_91_test_ff, 0xc190, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c198_91_test_ff, 0xc198, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a0_91_test_ff, 0xc1a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a8_91_test_ff, 0xc1a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b0_91_test_ff, 0xc1b0, 4, { 4, 0 }, 0 }, /* AND */
{ NULL, op_c1b8_91_test_ff, 0xc1b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b9_91_test_ff, 0xc1b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1c0_91_test_ff, 0xc1c0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d0_91_test_ff, 0xc1d0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d8_91_test_ff, 0xc1d8, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e0_91_test_ff, 0xc1e0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e8_91_test_ff, 0xc1e8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f0_91_test_ff, 0xc1f0, 4, { 4, 0 }, 0 }, /* MULS */
{ NULL, op_c1f8_91_test_ff, 0xc1f8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f9_91_test_ff, 0xc1f9, 6, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fa_91_test_ff, 0xc1fa, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fb_91_test_ff, 0xc1fb, 4, { 4, 0 }, 0 }, /* MULS */
{ NULL, op_c1fc_91_test_ff, 0xc1fc, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_d000_91_test_ff, 0xd000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d010_91_test_ff, 0xd010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d018_91_test_ff, 0xd018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d020_91_test_ff, 0xd020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d028_91_test_ff, 0xd028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d030_91_test_ff, 0xd030, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d038_91_test_ff, 0xd038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d039_91_test_ff, 0xd039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03a_91_test_ff, 0xd03a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03b_91_test_ff, 0xd03b, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d03c_91_test_ff, 0xd03c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d040_91_test_ff, 0xd040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d048_91_test_ff, 0xd048, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d050_91_test_ff, 0xd050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d058_91_test_ff, 0xd058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d060_91_test_ff, 0xd060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d068_91_test_ff, 0xd068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d070_91_test_ff, 0xd070, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d078_91_test_ff, 0xd078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d079_91_test_ff, 0xd079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07a_91_test_ff, 0xd07a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07b_91_test_ff, 0xd07b, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d07c_91_test_ff, 0xd07c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d080_91_test_ff, 0xd080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d088_91_test_ff, 0xd088, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d090_91_test_ff, 0xd090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d098_91_test_ff, 0xd098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a0_91_test_ff, 0xd0a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a8_91_test_ff, 0xd0a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b0_91_test_ff, 0xd0b0, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d0b8_91_test_ff, 0xd0b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b9_91_test_ff, 0xd0b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0ba_91_test_ff, 0xd0ba, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0bb_91_test_ff, 0xd0bb, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d0bc_91_test_ff, 0xd0bc, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0c0_91_test_ff, 0xd0c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0c8_91_test_ff, 0xd0c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d0_91_test_ff, 0xd0d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d8_91_test_ff, 0xd0d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e0_91_test_ff, 0xd0e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e8_91_test_ff, 0xd0e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f0_91_test_ff, 0xd0f0, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f8_91_test_ff, 0xd0f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f9_91_test_ff, 0xd0f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fa_91_test_ff, 0xd0fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fb_91_test_ff, 0xd0fb, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fc_91_test_ff, 0xd0fc, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d100_91_test_ff, 0xd100, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d108_91_test_ff, 0xd108, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d110_91_test_ff, 0xd110, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d118_91_test_ff, 0xd118, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d120_91_test_ff, 0xd120, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d128_91_test_ff, 0xd128, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d130_91_test_ff, 0xd130, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d138_91_test_ff, 0xd138, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d139_91_test_ff, 0xd139, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d140_91_test_ff, 0xd140, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d148_91_test_ff, 0xd148, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d150_91_test_ff, 0xd150, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d158_91_test_ff, 0xd158, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d160_91_test_ff, 0xd160, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d168_91_test_ff, 0xd168, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d170_91_test_ff, 0xd170, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d178_91_test_ff, 0xd178, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d179_91_test_ff, 0xd179, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d180_91_test_ff, 0xd180, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d188_91_test_ff, 0xd188, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d190_91_test_ff, 0xd190, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d198_91_test_ff, 0xd198, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a0_91_test_ff, 0xd1a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a8_91_test_ff, 0xd1a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b0_91_test_ff, 0xd1b0, 4, { 4, 0 }, 0 }, /* ADD */
{ NULL, op_d1b8_91_test_ff, 0xd1b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b9_91_test_ff, 0xd1b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1c0_91_test_ff, 0xd1c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1c8_91_test_ff, 0xd1c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d0_91_test_ff, 0xd1d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d8_91_test_ff, 0xd1d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e0_91_test_ff, 0xd1e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e8_91_test_ff, 0xd1e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f0_91_test_ff, 0xd1f0, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f8_91_test_ff, 0xd1f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f9_91_test_ff, 0xd1f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fa_91_test_ff, 0xd1fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fb_91_test_ff, 0xd1fb, 4, { 4, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fc_91_test_ff, 0xd1fc, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_e000_91_test_ff, 0xe000, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e008_91_test_ff, 0xe008, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e010_91_test_ff, 0xe010, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e018_91_test_ff, 0xe018, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e020_91_test_ff, 0xe020, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e028_91_test_ff, 0xe028, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e030_91_test_ff, 0xe030, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e038_91_test_ff, 0xe038, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e040_91_test_ff, 0xe040, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e048_91_test_ff, 0xe048, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e050_91_test_ff, 0xe050, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e058_91_test_ff, 0xe058, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e060_91_test_ff, 0xe060, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e068_91_test_ff, 0xe068, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e070_91_test_ff, 0xe070, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e078_91_test_ff, 0xe078, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e080_91_test_ff, 0xe080, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e088_91_test_ff, 0xe088, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e090_91_test_ff, 0xe090, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e098_91_test_ff, 0xe098, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0a0_91_test_ff, 0xe0a0, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e0a8_91_test_ff, 0xe0a8, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e0b0_91_test_ff, 0xe0b0, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e0b8_91_test_ff, 0xe0b8, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0d0_91_test_ff, 0xe0d0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0d8_91_test_ff, 0xe0d8, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e0_91_test_ff, 0xe0e0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e8_91_test_ff, 0xe0e8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f0_91_test_ff, 0xe0f0, 4, { 4, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f8_91_test_ff, 0xe0f8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f9_91_test_ff, 0xe0f9, 6, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e100_91_test_ff, 0xe100, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e108_91_test_ff, 0xe108, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e110_91_test_ff, 0xe110, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e118_91_test_ff, 0xe118, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e120_91_test_ff, 0xe120, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e128_91_test_ff, 0xe128, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e130_91_test_ff, 0xe130, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e138_91_test_ff, 0xe138, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e140_91_test_ff, 0xe140, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e148_91_test_ff, 0xe148, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e150_91_test_ff, 0xe150, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e158_91_test_ff, 0xe158, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e160_91_test_ff, 0xe160, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e168_91_test_ff, 0xe168, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e170_91_test_ff, 0xe170, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e178_91_test_ff, 0xe178, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e180_91_test_ff, 0xe180, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e188_91_test_ff, 0xe188, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e190_91_test_ff, 0xe190, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e198_91_test_ff, 0xe198, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1a0_91_test_ff, 0xe1a0, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e1a8_91_test_ff, 0xe1a8, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e1b0_91_test_ff, 0xe1b0, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e1b8_91_test_ff, 0xe1b8, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1d0_91_test_ff, 0xe1d0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1d8_91_test_ff, 0xe1d8, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e0_91_test_ff, 0xe1e0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e8_91_test_ff, 0xe1e8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f0_91_test_ff, 0xe1f0, 4, { 4, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f8_91_test_ff, 0xe1f8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f9_91_test_ff, 0xe1f9, 6, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e2d0_91_test_ff, 0xe2d0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2d8_91_test_ff, 0xe2d8, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e0_91_test_ff, 0xe2e0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e8_91_test_ff, 0xe2e8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f0_91_test_ff, 0xe2f0, 4, { 4, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f8_91_test_ff, 0xe2f8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f9_91_test_ff, 0xe2f9, 6, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e3d0_91_test_ff, 0xe3d0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3d8_91_test_ff, 0xe3d8, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e0_91_test_ff, 0xe3e0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e8_91_test_ff, 0xe3e8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f0_91_test_ff, 0xe3f0, 4, { 4, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f8_91_test_ff, 0xe3f8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f9_91_test_ff, 0xe3f9, 6, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e4d0_91_test_ff, 0xe4d0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4d8_91_test_ff, 0xe4d8, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e0_91_test_ff, 0xe4e0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e8_91_test_ff, 0xe4e8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f0_91_test_ff, 0xe4f0, 4, { 4, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f8_91_test_ff, 0xe4f8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f9_91_test_ff, 0xe4f9, 6, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e5d0_91_test_ff, 0xe5d0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5d8_91_test_ff, 0xe5d8, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e0_91_test_ff, 0xe5e0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e8_91_test_ff, 0xe5e8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f0_91_test_ff, 0xe5f0, 4, { 4, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f8_91_test_ff, 0xe5f8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f9_91_test_ff, 0xe5f9, 6, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e6d0_91_test_ff, 0xe6d0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6d8_91_test_ff, 0xe6d8, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e0_91_test_ff, 0xe6e0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e8_91_test_ff, 0xe6e8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f0_91_test_ff, 0xe6f0, 4, { 4, 0 }, 0 }, /* RORW */
{ NULL, op_e6f8_91_test_ff, 0xe6f8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f9_91_test_ff, 0xe6f9, 6, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e7d0_91_test_ff, 0xe7d0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7d8_91_test_ff, 0xe7d8, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e0_91_test_ff, 0xe7e0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e8_91_test_ff, 0xe7e8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f0_91_test_ff, 0xe7f0, 4, { 4, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f8_91_test_ff, 0xe7f8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f9_91_test_ff, 0xe7f9, 6, { 0, 0 }, 0 }, /* ROLW */
{ 0 }};
#endif /* CPUEMU_91 */
#ifdef CPUEMU_92
const struct cputbl CPUFUNC(op_smalltbl_92_test)[] = {
{ NULL, op_0000_92_test_ff, 0x0000, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0010_92_test_ff, 0x0010, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0018_92_test_ff, 0x0018, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0020_92_test_ff, 0x0020, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0028_92_test_ff, 0x0028, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0030_92_test_ff, 0x0030, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0038_92_test_ff, 0x0038, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0039_92_test_ff, 0x0039, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_003c_92_test_ff, 0x003c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0040_92_test_ff, 0x0040, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0050_92_test_ff, 0x0050, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0058_92_test_ff, 0x0058, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0060_92_test_ff, 0x0060, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0068_92_test_ff, 0x0068, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0070_92_test_ff, 0x0070, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0078_92_test_ff, 0x0078, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0079_92_test_ff, 0x0079, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_007c_92_test_ff, 0x007c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0080_92_test_ff, 0x0080, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0090_92_test_ff, 0x0090, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0098_92_test_ff, 0x0098, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a0_92_test_ff, 0x00a0, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a8_92_test_ff, 0x00a8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b0_92_test_ff, 0x00b0, 6, { 2, 0 }, 0 }, /* OR */
{ NULL, op_00b8_92_test_ff, 0x00b8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b9_92_test_ff, 0x00b9, 10, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00d0_92_test_ff, 0x00d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00e8_92_test_ff, 0x00e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f0_92_test_ff, 0x00f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f8_92_test_ff, 0x00f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f9_92_test_ff, 0x00f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fa_92_test_ff, 0x00fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fb_92_test_ff, 0x00fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0100_92_test_ff, 0x0100, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0108_92_test_ff, 0x0108, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0110_92_test_ff, 0x0110, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0118_92_test_ff, 0x0118, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0120_92_test_ff, 0x0120, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0128_92_test_ff, 0x0128, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0130_92_test_ff, 0x0130, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0138_92_test_ff, 0x0138, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0139_92_test_ff, 0x0139, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013a_92_test_ff, 0x013a, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013b_92_test_ff, 0x013b, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_013c_92_test_ff, 0x013c, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0140_92_test_ff, 0x0140, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0148_92_test_ff, 0x0148, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0150_92_test_ff, 0x0150, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0158_92_test_ff, 0x0158, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0160_92_test_ff, 0x0160, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0168_92_test_ff, 0x0168, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0170_92_test_ff, 0x0170, 2, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0178_92_test_ff, 0x0178, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0179_92_test_ff, 0x0179, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0180_92_test_ff, 0x0180, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0188_92_test_ff, 0x0188, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_0190_92_test_ff, 0x0190, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0198_92_test_ff, 0x0198, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a0_92_test_ff, 0x01a0, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a8_92_test_ff, 0x01a8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b0_92_test_ff, 0x01b0, 2, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_01b8_92_test_ff, 0x01b8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b9_92_test_ff, 0x01b9, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01c0_92_test_ff, 0x01c0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01c8_92_test_ff, 0x01c8, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_01d0_92_test_ff, 0x01d0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01d8_92_test_ff, 0x01d8, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e0_92_test_ff, 0x01e0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e8_92_test_ff, 0x01e8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f0_92_test_ff, 0x01f0, 2, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_01f8_92_test_ff, 0x01f8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f9_92_test_ff, 0x01f9, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0200_92_test_ff, 0x0200, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0210_92_test_ff, 0x0210, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0218_92_test_ff, 0x0218, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0220_92_test_ff, 0x0220, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0228_92_test_ff, 0x0228, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0230_92_test_ff, 0x0230, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0238_92_test_ff, 0x0238, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0239_92_test_ff, 0x0239, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_023c_92_test_ff, 0x023c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0240_92_test_ff, 0x0240, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0250_92_test_ff, 0x0250, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0258_92_test_ff, 0x0258, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0260_92_test_ff, 0x0260, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0268_92_test_ff, 0x0268, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0270_92_test_ff, 0x0270, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0278_92_test_ff, 0x0278, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0279_92_test_ff, 0x0279, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_027c_92_test_ff, 0x027c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0280_92_test_ff, 0x0280, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0290_92_test_ff, 0x0290, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0298_92_test_ff, 0x0298, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a0_92_test_ff, 0x02a0, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a8_92_test_ff, 0x02a8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b0_92_test_ff, 0x02b0, 6, { 2, 0 }, 0 }, /* AND */
{ NULL, op_02b8_92_test_ff, 0x02b8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b9_92_test_ff, 0x02b9, 10, { 0, 0 }, 0 }, /* AND */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02d0_92_test_ff, 0x02d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02e8_92_test_ff, 0x02e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f0_92_test_ff, 0x02f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f8_92_test_ff, 0x02f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f9_92_test_ff, 0x02f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fa_92_test_ff, 0x02fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fb_92_test_ff, 0x02fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0400_92_test_ff, 0x0400, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0410_92_test_ff, 0x0410, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0418_92_test_ff, 0x0418, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0420_92_test_ff, 0x0420, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0428_92_test_ff, 0x0428, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0430_92_test_ff, 0x0430, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0438_92_test_ff, 0x0438, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0439_92_test_ff, 0x0439, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0440_92_test_ff, 0x0440, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0450_92_test_ff, 0x0450, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0458_92_test_ff, 0x0458, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0460_92_test_ff, 0x0460, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0468_92_test_ff, 0x0468, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0470_92_test_ff, 0x0470, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0478_92_test_ff, 0x0478, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0479_92_test_ff, 0x0479, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0480_92_test_ff, 0x0480, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0490_92_test_ff, 0x0490, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0498_92_test_ff, 0x0498, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a0_92_test_ff, 0x04a0, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a8_92_test_ff, 0x04a8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b0_92_test_ff, 0x04b0, 6, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_04b8_92_test_ff, 0x04b8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b9_92_test_ff, 0x04b9, 10, { 0, 0 }, 0 }, /* SUB */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04d0_92_test_ff, 0x04d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04e8_92_test_ff, 0x04e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f0_92_test_ff, 0x04f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f8_92_test_ff, 0x04f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f9_92_test_ff, 0x04f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fa_92_test_ff, 0x04fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fb_92_test_ff, 0x04fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0600_92_test_ff, 0x0600, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0610_92_test_ff, 0x0610, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0618_92_test_ff, 0x0618, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0620_92_test_ff, 0x0620, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0628_92_test_ff, 0x0628, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0630_92_test_ff, 0x0630, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0638_92_test_ff, 0x0638, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0639_92_test_ff, 0x0639, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0640_92_test_ff, 0x0640, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0650_92_test_ff, 0x0650, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0658_92_test_ff, 0x0658, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0660_92_test_ff, 0x0660, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0668_92_test_ff, 0x0668, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0670_92_test_ff, 0x0670, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0678_92_test_ff, 0x0678, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0679_92_test_ff, 0x0679, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0680_92_test_ff, 0x0680, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0690_92_test_ff, 0x0690, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0698_92_test_ff, 0x0698, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a0_92_test_ff, 0x06a0, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a8_92_test_ff, 0x06a8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b0_92_test_ff, 0x06b0, 6, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_06b8_92_test_ff, 0x06b8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b9_92_test_ff, 0x06b9, 10, { 0, 0 }, 0 }, /* ADD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c0_92_test_ff, 0x06c0, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c8_92_test_ff, 0x06c8, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06d0_92_test_ff, 0x06d0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06e8_92_test_ff, 0x06e8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f0_92_test_ff, 0x06f0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f8_92_test_ff, 0x06f8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f9_92_test_ff, 0x06f9, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fa_92_test_ff, 0x06fa, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fb_92_test_ff, 0x06fb, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
{ NULL, op_0800_92_test_ff, 0x0800, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0810_92_test_ff, 0x0810, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0818_92_test_ff, 0x0818, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0820_92_test_ff, 0x0820, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0828_92_test_ff, 0x0828, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0830_92_test_ff, 0x0830, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0838_92_test_ff, 0x0838, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0839_92_test_ff, 0x0839, 8, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083a_92_test_ff, 0x083a, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083b_92_test_ff, 0x083b, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0840_92_test_ff, 0x0840, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0850_92_test_ff, 0x0850, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0858_92_test_ff, 0x0858, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0860_92_test_ff, 0x0860, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0868_92_test_ff, 0x0868, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0870_92_test_ff, 0x0870, 4, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0878_92_test_ff, 0x0878, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0879_92_test_ff, 0x0879, 8, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0880_92_test_ff, 0x0880, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0890_92_test_ff, 0x0890, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0898_92_test_ff, 0x0898, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a0_92_test_ff, 0x08a0, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a8_92_test_ff, 0x08a8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b0_92_test_ff, 0x08b0, 4, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_08b8_92_test_ff, 0x08b8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b9_92_test_ff, 0x08b9, 8, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08c0_92_test_ff, 0x08c0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d0_92_test_ff, 0x08d0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d8_92_test_ff, 0x08d8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e0_92_test_ff, 0x08e0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e8_92_test_ff, 0x08e8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f0_92_test_ff, 0x08f0, 4, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_08f8_92_test_ff, 0x08f8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f9_92_test_ff, 0x08f9, 8, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0a00_92_test_ff, 0x0a00, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a10_92_test_ff, 0x0a10, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a18_92_test_ff, 0x0a18, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a20_92_test_ff, 0x0a20, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a28_92_test_ff, 0x0a28, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a30_92_test_ff, 0x0a30, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a38_92_test_ff, 0x0a38, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a39_92_test_ff, 0x0a39, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a3c_92_test_ff, 0x0a3c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a40_92_test_ff, 0x0a40, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a50_92_test_ff, 0x0a50, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a58_92_test_ff, 0x0a58, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a60_92_test_ff, 0x0a60, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a68_92_test_ff, 0x0a68, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a70_92_test_ff, 0x0a70, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a78_92_test_ff, 0x0a78, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a79_92_test_ff, 0x0a79, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a7c_92_test_ff, 0x0a7c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a80_92_test_ff, 0x0a80, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a90_92_test_ff, 0x0a90, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a98_92_test_ff, 0x0a98, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa0_92_test_ff, 0x0aa0, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa8_92_test_ff, 0x0aa8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab0_92_test_ff, 0x0ab0, 6, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0ab8_92_test_ff, 0x0ab8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab9_92_test_ff, 0x0ab9, 10, { 0, 0 }, 0 }, /* EOR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad0_92_test_ff, 0x0ad0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad8_92_test_ff, 0x0ad8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae0_92_test_ff, 0x0ae0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae8_92_test_ff, 0x0ae8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af0_92_test_ff, 0x0af0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af8_92_test_ff, 0x0af8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af9_92_test_ff, 0x0af9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
{ NULL, op_0c00_92_test_ff, 0x0c00, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c10_92_test_ff, 0x0c10, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c18_92_test_ff, 0x0c18, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c20_92_test_ff, 0x0c20, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c28_92_test_ff, 0x0c28, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c30_92_test_ff, 0x0c30, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c38_92_test_ff, 0x0c38, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c39_92_test_ff, 0x0c39, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3a_92_test_ff, 0x0c3a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3b_92_test_ff, 0x0c3b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c40_92_test_ff, 0x0c40, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c50_92_test_ff, 0x0c50, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c58_92_test_ff, 0x0c58, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c60_92_test_ff, 0x0c60, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c68_92_test_ff, 0x0c68, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c70_92_test_ff, 0x0c70, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c78_92_test_ff, 0x0c78, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c79_92_test_ff, 0x0c79, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7a_92_test_ff, 0x0c7a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7b_92_test_ff, 0x0c7b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c80_92_test_ff, 0x0c80, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c90_92_test_ff, 0x0c90, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c98_92_test_ff, 0x0c98, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca0_92_test_ff, 0x0ca0, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca8_92_test_ff, 0x0ca8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb0_92_test_ff, 0x0cb0, 6, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0cb8_92_test_ff, 0x0cb8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb9_92_test_ff, 0x0cb9, 10, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cba_92_test_ff, 0x0cba, 8, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cbb_92_test_ff, 0x0cbb, 6, { 2, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd0_92_test_ff, 0x0cd0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd8_92_test_ff, 0x0cd8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce0_92_test_ff, 0x0ce0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce8_92_test_ff, 0x0ce8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf0_92_test_ff, 0x0cf0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf8_92_test_ff, 0x0cf8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf9_92_test_ff, 0x0cf9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cfc_92_test_ff, 0x0cfc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e10_92_test_ff, 0x0e10, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e18_92_test_ff, 0x0e18, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e20_92_test_ff, 0x0e20, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e28_92_test_ff, 0x0e28, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e30_92_test_ff, 0x0e30, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e38_92_test_ff, 0x0e38, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e39_92_test_ff, 0x0e39, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e50_92_test_ff, 0x0e50, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e58_92_test_ff, 0x0e58, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e60_92_test_ff, 0x0e60, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e68_92_test_ff, 0x0e68, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e70_92_test_ff, 0x0e70, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e78_92_test_ff, 0x0e78, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e79_92_test_ff, 0x0e79, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e90_92_test_ff, 0x0e90, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e98_92_test_ff, 0x0e98, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea0_92_test_ff, 0x0ea0, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea8_92_test_ff, 0x0ea8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb0_92_test_ff, 0x0eb0, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb8_92_test_ff, 0x0eb8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb9_92_test_ff, 0x0eb9, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed0_92_test_ff, 0x0ed0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed8_92_test_ff, 0x0ed8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee0_92_test_ff, 0x0ee0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee8_92_test_ff, 0x0ee8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef0_92_test_ff, 0x0ef0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef8_92_test_ff, 0x0ef8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef9_92_test_ff, 0x0ef9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0efc_92_test_ff, 0x0efc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
{ NULL, op_1000_92_test_ff, 0x1000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1010_92_test_ff, 0x1010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1018_92_test_ff, 0x1018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1020_92_test_ff, 0x1020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1028_92_test_ff, 0x1028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1030_92_test_ff, 0x1030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1038_92_test_ff, 0x1038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1039_92_test_ff, 0x1039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103a_92_test_ff, 0x103a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103b_92_test_ff, 0x103b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_103c_92_test_ff, 0x103c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1080_92_test_ff, 0x1080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1090_92_test_ff, 0x1090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1098_92_test_ff, 0x1098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a0_92_test_ff, 0x10a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a8_92_test_ff, 0x10a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b0_92_test_ff, 0x10b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10b8_92_test_ff, 0x10b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b9_92_test_ff, 0x10b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10ba_92_test_ff, 0x10ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10bb_92_test_ff, 0x10bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10bc_92_test_ff, 0x10bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10c0_92_test_ff, 0x10c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d0_92_test_ff, 0x10d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d8_92_test_ff, 0x10d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e0_92_test_ff, 0x10e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e8_92_test_ff, 0x10e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f0_92_test_ff, 0x10f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10f8_92_test_ff, 0x10f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f9_92_test_ff, 0x10f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fa_92_test_ff, 0x10fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fb_92_test_ff, 0x10fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10fc_92_test_ff, 0x10fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1100_92_test_ff, 0x1100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1110_92_test_ff, 0x1110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1118_92_test_ff, 0x1118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1120_92_test_ff, 0x1120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1128_92_test_ff, 0x1128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1130_92_test_ff, 0x1130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1138_92_test_ff, 0x1138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1139_92_test_ff, 0x1139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113a_92_test_ff, 0x113a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113b_92_test_ff, 0x113b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_113c_92_test_ff, 0x113c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1140_92_test_ff, 0x1140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1150_92_test_ff, 0x1150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1158_92_test_ff, 0x1158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1160_92_test_ff, 0x1160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1168_92_test_ff, 0x1168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1170_92_test_ff, 0x1170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1178_92_test_ff, 0x1178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1179_92_test_ff, 0x1179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117a_92_test_ff, 0x117a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117b_92_test_ff, 0x117b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_117c_92_test_ff, 0x117c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1180_92_test_ff, 0x1180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1190_92_test_ff, 0x1190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1198_92_test_ff, 0x1198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a0_92_test_ff, 0x11a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a8_92_test_ff, 0x11a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b0_92_test_ff, 0x11b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11b8_92_test_ff, 0x11b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b9_92_test_ff, 0x11b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11ba_92_test_ff, 0x11ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11bb_92_test_ff, 0x11bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11bc_92_test_ff, 0x11bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11c0_92_test_ff, 0x11c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d0_92_test_ff, 0x11d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d8_92_test_ff, 0x11d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e0_92_test_ff, 0x11e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e8_92_test_ff, 0x11e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f0_92_test_ff, 0x11f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11f8_92_test_ff, 0x11f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f9_92_test_ff, 0x11f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fa_92_test_ff, 0x11fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fb_92_test_ff, 0x11fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11fc_92_test_ff, 0x11fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13c0_92_test_ff, 0x13c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d0_92_test_ff, 0x13d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d8_92_test_ff, 0x13d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e0_92_test_ff, 0x13e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e8_92_test_ff, 0x13e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f0_92_test_ff, 0x13f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13f8_92_test_ff, 0x13f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f9_92_test_ff, 0x13f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fa_92_test_ff, 0x13fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fb_92_test_ff, 0x13fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13fc_92_test_ff, 0x13fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2000_92_test_ff, 0x2000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2008_92_test_ff, 0x2008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2010_92_test_ff, 0x2010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2018_92_test_ff, 0x2018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2020_92_test_ff, 0x2020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2028_92_test_ff, 0x2028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2030_92_test_ff, 0x2030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2038_92_test_ff, 0x2038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2039_92_test_ff, 0x2039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203a_92_test_ff, 0x203a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203b_92_test_ff, 0x203b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_203c_92_test_ff, 0x203c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2040_92_test_ff, 0x2040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2048_92_test_ff, 0x2048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2050_92_test_ff, 0x2050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2058_92_test_ff, 0x2058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2060_92_test_ff, 0x2060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2068_92_test_ff, 0x2068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2070_92_test_ff, 0x2070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_2078_92_test_ff, 0x2078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2079_92_test_ff, 0x2079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207a_92_test_ff, 0x207a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207b_92_test_ff, 0x207b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_207c_92_test_ff, 0x207c, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2080_92_test_ff, 0x2080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2088_92_test_ff, 0x2088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2090_92_test_ff, 0x2090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2098_92_test_ff, 0x2098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a0_92_test_ff, 0x20a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a8_92_test_ff, 0x20a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b0_92_test_ff, 0x20b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20b8_92_test_ff, 0x20b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b9_92_test_ff, 0x20b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20ba_92_test_ff, 0x20ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20bb_92_test_ff, 0x20bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20bc_92_test_ff, 0x20bc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c0_92_test_ff, 0x20c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c8_92_test_ff, 0x20c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d0_92_test_ff, 0x20d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d8_92_test_ff, 0x20d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e0_92_test_ff, 0x20e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e8_92_test_ff, 0x20e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f0_92_test_ff, 0x20f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20f8_92_test_ff, 0x20f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f9_92_test_ff, 0x20f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fa_92_test_ff, 0x20fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fb_92_test_ff, 0x20fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20fc_92_test_ff, 0x20fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2100_92_test_ff, 0x2100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2108_92_test_ff, 0x2108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2110_92_test_ff, 0x2110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2118_92_test_ff, 0x2118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2120_92_test_ff, 0x2120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2128_92_test_ff, 0x2128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2130_92_test_ff, 0x2130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2138_92_test_ff, 0x2138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2139_92_test_ff, 0x2139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213a_92_test_ff, 0x213a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213b_92_test_ff, 0x213b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_213c_92_test_ff, 0x213c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2140_92_test_ff, 0x2140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2148_92_test_ff, 0x2148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2150_92_test_ff, 0x2150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2158_92_test_ff, 0x2158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2160_92_test_ff, 0x2160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2168_92_test_ff, 0x2168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2170_92_test_ff, 0x2170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2178_92_test_ff, 0x2178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2179_92_test_ff, 0x2179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217a_92_test_ff, 0x217a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217b_92_test_ff, 0x217b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_217c_92_test_ff, 0x217c, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2180_92_test_ff, 0x2180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2188_92_test_ff, 0x2188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2190_92_test_ff, 0x2190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2198_92_test_ff, 0x2198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a0_92_test_ff, 0x21a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a8_92_test_ff, 0x21a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b0_92_test_ff, 0x21b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21b8_92_test_ff, 0x21b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b9_92_test_ff, 0x21b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21ba_92_test_ff, 0x21ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21bb_92_test_ff, 0x21bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21bc_92_test_ff, 0x21bc, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21c0_92_test_ff, 0x21c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21c8_92_test_ff, 0x21c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d0_92_test_ff, 0x21d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d8_92_test_ff, 0x21d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e0_92_test_ff, 0x21e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e8_92_test_ff, 0x21e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f0_92_test_ff, 0x21f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21f8_92_test_ff, 0x21f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f9_92_test_ff, 0x21f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fa_92_test_ff, 0x21fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fb_92_test_ff, 0x21fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21fc_92_test_ff, 0x21fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c0_92_test_ff, 0x23c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c8_92_test_ff, 0x23c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d0_92_test_ff, 0x23d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d8_92_test_ff, 0x23d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e0_92_test_ff, 0x23e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e8_92_test_ff, 0x23e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f0_92_test_ff, 0x23f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23f8_92_test_ff, 0x23f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f9_92_test_ff, 0x23f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fa_92_test_ff, 0x23fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fb_92_test_ff, 0x23fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23fc_92_test_ff, 0x23fc, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3000_92_test_ff, 0x3000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3008_92_test_ff, 0x3008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3010_92_test_ff, 0x3010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3018_92_test_ff, 0x3018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3020_92_test_ff, 0x3020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3028_92_test_ff, 0x3028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3030_92_test_ff, 0x3030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3038_92_test_ff, 0x3038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3039_92_test_ff, 0x3039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303a_92_test_ff, 0x303a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303b_92_test_ff, 0x303b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_303c_92_test_ff, 0x303c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3040_92_test_ff, 0x3040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3048_92_test_ff, 0x3048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3050_92_test_ff, 0x3050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3058_92_test_ff, 0x3058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3060_92_test_ff, 0x3060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3068_92_test_ff, 0x3068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3070_92_test_ff, 0x3070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_3078_92_test_ff, 0x3078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3079_92_test_ff, 0x3079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307a_92_test_ff, 0x307a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307b_92_test_ff, 0x307b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_307c_92_test_ff, 0x307c, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3080_92_test_ff, 0x3080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3088_92_test_ff, 0x3088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3090_92_test_ff, 0x3090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3098_92_test_ff, 0x3098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a0_92_test_ff, 0x30a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a8_92_test_ff, 0x30a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b0_92_test_ff, 0x30b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30b8_92_test_ff, 0x30b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b9_92_test_ff, 0x30b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30ba_92_test_ff, 0x30ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30bb_92_test_ff, 0x30bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30bc_92_test_ff, 0x30bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c0_92_test_ff, 0x30c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c8_92_test_ff, 0x30c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d0_92_test_ff, 0x30d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d8_92_test_ff, 0x30d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e0_92_test_ff, 0x30e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e8_92_test_ff, 0x30e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f0_92_test_ff, 0x30f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30f8_92_test_ff, 0x30f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f9_92_test_ff, 0x30f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fa_92_test_ff, 0x30fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fb_92_test_ff, 0x30fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30fc_92_test_ff, 0x30fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3100_92_test_ff, 0x3100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3108_92_test_ff, 0x3108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3110_92_test_ff, 0x3110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3118_92_test_ff, 0x3118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3120_92_test_ff, 0x3120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3128_92_test_ff, 0x3128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3130_92_test_ff, 0x3130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3138_92_test_ff, 0x3138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3139_92_test_ff, 0x3139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313a_92_test_ff, 0x313a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313b_92_test_ff, 0x313b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_313c_92_test_ff, 0x313c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3140_92_test_ff, 0x3140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3148_92_test_ff, 0x3148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3150_92_test_ff, 0x3150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3158_92_test_ff, 0x3158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3160_92_test_ff, 0x3160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3168_92_test_ff, 0x3168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3170_92_test_ff, 0x3170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3178_92_test_ff, 0x3178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3179_92_test_ff, 0x3179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317a_92_test_ff, 0x317a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317b_92_test_ff, 0x317b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_317c_92_test_ff, 0x317c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3180_92_test_ff, 0x3180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3188_92_test_ff, 0x3188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3190_92_test_ff, 0x3190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3198_92_test_ff, 0x3198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a0_92_test_ff, 0x31a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a8_92_test_ff, 0x31a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b0_92_test_ff, 0x31b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31b8_92_test_ff, 0x31b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b9_92_test_ff, 0x31b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31ba_92_test_ff, 0x31ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31bb_92_test_ff, 0x31bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31bc_92_test_ff, 0x31bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31c0_92_test_ff, 0x31c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31c8_92_test_ff, 0x31c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d0_92_test_ff, 0x31d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d8_92_test_ff, 0x31d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e0_92_test_ff, 0x31e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e8_92_test_ff, 0x31e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f0_92_test_ff, 0x31f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31f8_92_test_ff, 0x31f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f9_92_test_ff, 0x31f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fa_92_test_ff, 0x31fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fb_92_test_ff, 0x31fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31fc_92_test_ff, 0x31fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c0_92_test_ff, 0x33c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c8_92_test_ff, 0x33c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d0_92_test_ff, 0x33d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d8_92_test_ff, 0x33d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e0_92_test_ff, 0x33e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e8_92_test_ff, 0x33e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f0_92_test_ff, 0x33f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33f8_92_test_ff, 0x33f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f9_92_test_ff, 0x33f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fa_92_test_ff, 0x33fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fb_92_test_ff, 0x33fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33fc_92_test_ff, 0x33fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_4000_92_test_ff, 0x4000, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4010_92_test_ff, 0x4010, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4018_92_test_ff, 0x4018, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4020_92_test_ff, 0x4020, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4028_92_test_ff, 0x4028, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4030_92_test_ff, 0x4030, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4038_92_test_ff, 0x4038, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4039_92_test_ff, 0x4039, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4040_92_test_ff, 0x4040, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4050_92_test_ff, 0x4050, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4058_92_test_ff, 0x4058, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4060_92_test_ff, 0x4060, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4068_92_test_ff, 0x4068, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4070_92_test_ff, 0x4070, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4078_92_test_ff, 0x4078, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4079_92_test_ff, 0x4079, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4080_92_test_ff, 0x4080, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4090_92_test_ff, 0x4090, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4098_92_test_ff, 0x4098, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a0_92_test_ff, 0x40a0, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a8_92_test_ff, 0x40a8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b0_92_test_ff, 0x40b0, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_40b8_92_test_ff, 0x40b8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b9_92_test_ff, 0x40b9, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40c0_92_test_ff, 0x40c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d0_92_test_ff, 0x40d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d8_92_test_ff, 0x40d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e0_92_test_ff, 0x40e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e8_92_test_ff, 0x40e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f0_92_test_ff, 0x40f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f8_92_test_ff, 0x40f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f9_92_test_ff, 0x40f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4100_92_test_ff, 0x4100, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4110_92_test_ff, 0x4110, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4118_92_test_ff, 0x4118, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4120_92_test_ff, 0x4120, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4128_92_test_ff, 0x4128, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4130_92_test_ff, 0x4130, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4138_92_test_ff, 0x4138, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4139_92_test_ff, 0x4139, 6, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413a_92_test_ff, 0x413a, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413b_92_test_ff, 0x413b, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413c_92_test_ff, 0x413c, 6, { 0, 0 }, 0 }, /* CHK */
#endif
{ NULL, op_4180_92_test_ff, 0x4180, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4190_92_test_ff, 0x4190, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4198_92_test_ff, 0x4198, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a0_92_test_ff, 0x41a0, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a8_92_test_ff, 0x41a8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b0_92_test_ff, 0x41b0, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41b8_92_test_ff, 0x41b8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b9_92_test_ff, 0x41b9, 6, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41ba_92_test_ff, 0x41ba, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41bb_92_test_ff, 0x41bb, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41bc_92_test_ff, 0x41bc, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41d0_92_test_ff, 0x41d0, 2, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41e8_92_test_ff, 0x41e8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f0_92_test_ff, 0x41f0, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_41f8_92_test_ff, 0x41f8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f9_92_test_ff, 0x41f9, 6, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fa_92_test_ff, 0x41fa, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fb_92_test_ff, 0x41fb, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_4200_92_test_ff, 0x4200, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4210_92_test_ff, 0x4210, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4218_92_test_ff, 0x4218, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4220_92_test_ff, 0x4220, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4228_92_test_ff, 0x4228, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4230_92_test_ff, 0x4230, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4238_92_test_ff, 0x4238, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4239_92_test_ff, 0x4239, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4240_92_test_ff, 0x4240, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4250_92_test_ff, 0x4250, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4258_92_test_ff, 0x4258, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4260_92_test_ff, 0x4260, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4268_92_test_ff, 0x4268, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4270_92_test_ff, 0x4270, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4278_92_test_ff, 0x4278, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4279_92_test_ff, 0x4279, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4280_92_test_ff, 0x4280, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4290_92_test_ff, 0x4290, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4298_92_test_ff, 0x4298, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a0_92_test_ff, 0x42a0, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a8_92_test_ff, 0x42a8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b0_92_test_ff, 0x42b0, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_42b8_92_test_ff, 0x42b8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b9_92_test_ff, 0x42b9, 6, { 0, 0 }, 0 }, /* CLR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42c0_92_test_ff, 0x42c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d0_92_test_ff, 0x42d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d8_92_test_ff, 0x42d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e0_92_test_ff, 0x42e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e8_92_test_ff, 0x42e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f0_92_test_ff, 0x42f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f8_92_test_ff, 0x42f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f9_92_test_ff, 0x42f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#endif
{ NULL, op_4400_92_test_ff, 0x4400, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4410_92_test_ff, 0x4410, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4418_92_test_ff, 0x4418, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4420_92_test_ff, 0x4420, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4428_92_test_ff, 0x4428, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4430_92_test_ff, 0x4430, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4438_92_test_ff, 0x4438, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4439_92_test_ff, 0x4439, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4440_92_test_ff, 0x4440, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4450_92_test_ff, 0x4450, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4458_92_test_ff, 0x4458, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4460_92_test_ff, 0x4460, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4468_92_test_ff, 0x4468, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4470_92_test_ff, 0x4470, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4478_92_test_ff, 0x4478, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4479_92_test_ff, 0x4479, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4480_92_test_ff, 0x4480, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4490_92_test_ff, 0x4490, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4498_92_test_ff, 0x4498, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a0_92_test_ff, 0x44a0, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a8_92_test_ff, 0x44a8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b0_92_test_ff, 0x44b0, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_44b8_92_test_ff, 0x44b8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b9_92_test_ff, 0x44b9, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44c0_92_test_ff, 0x44c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d0_92_test_ff, 0x44d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d8_92_test_ff, 0x44d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e0_92_test_ff, 0x44e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e8_92_test_ff, 0x44e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f0_92_test_ff, 0x44f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f8_92_test_ff, 0x44f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f9_92_test_ff, 0x44f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fa_92_test_ff, 0x44fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fb_92_test_ff, 0x44fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fc_92_test_ff, 0x44fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4600_92_test_ff, 0x4600, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4610_92_test_ff, 0x4610, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4618_92_test_ff, 0x4618, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4620_92_test_ff, 0x4620, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4628_92_test_ff, 0x4628, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4630_92_test_ff, 0x4630, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4638_92_test_ff, 0x4638, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4639_92_test_ff, 0x4639, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4640_92_test_ff, 0x4640, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4650_92_test_ff, 0x4650, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4658_92_test_ff, 0x4658, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4660_92_test_ff, 0x4660, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4668_92_test_ff, 0x4668, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4670_92_test_ff, 0x4670, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4678_92_test_ff, 0x4678, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4679_92_test_ff, 0x4679, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4680_92_test_ff, 0x4680, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4690_92_test_ff, 0x4690, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4698_92_test_ff, 0x4698, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a0_92_test_ff, 0x46a0, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a8_92_test_ff, 0x46a8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b0_92_test_ff, 0x46b0, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_46b8_92_test_ff, 0x46b8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b9_92_test_ff, 0x46b9, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46c0_92_test_ff, 0x46c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d0_92_test_ff, 0x46d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d8_92_test_ff, 0x46d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e0_92_test_ff, 0x46e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e8_92_test_ff, 0x46e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f0_92_test_ff, 0x46f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f8_92_test_ff, 0x46f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f9_92_test_ff, 0x46f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fa_92_test_ff, 0x46fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fb_92_test_ff, 0x46fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fc_92_test_ff, 0x46fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4800_92_test_ff, 0x4800, 2, { 0, 0 }, 0 }, /* NBCD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4808_92_test_ff, 0x4808, 6, { 0, 0 }, 0 }, /* LINK */
#endif
{ NULL, op_4810_92_test_ff, 0x4810, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4818_92_test_ff, 0x4818, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4820_92_test_ff, 0x4820, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4828_92_test_ff, 0x4828, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4830_92_test_ff, 0x4830, 2, { 2, 0 }, 0 }, /* NBCD */
{ NULL, op_4838_92_test_ff, 0x4838, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4839_92_test_ff, 0x4839, 6, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4840_92_test_ff, 0x4840, 2, { 0, 0 }, 0 }, /* SWAP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4848_92_test_ff, 0x4848, 2, { 0, 0 }, 0 }, /* BKPT */
#endif
{ NULL, op_4850_92_test_ff, 0x4850, 2, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4868_92_test_ff, 0x4868, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4870_92_test_ff, 0x4870, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4878_92_test_ff, 0x4878, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4879_92_test_ff, 0x4879, 6, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487a_92_test_ff, 0x487a, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487b_92_test_ff, 0x487b, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4880_92_test_ff, 0x4880, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_4890_92_test_ff, 0x4890, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a0_92_test_ff, 0x48a0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a8_92_test_ff, 0x48a8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b0_92_test_ff, 0x48b0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b8_92_test_ff, 0x48b8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b9_92_test_ff, 0x48b9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48c0_92_test_ff, 0x48c0, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_48d0_92_test_ff, 0x48d0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e0_92_test_ff, 0x48e0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e8_92_test_ff, 0x48e8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f0_92_test_ff, 0x48f0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f8_92_test_ff, 0x48f8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f9_92_test_ff, 0x48f9, 8, { 0, 0 }, 0 }, /* MVMLE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_49c0_92_test_ff, 0x49c0, 2, { 0, 0 }, 0 }, /* EXT */
#endif
{ NULL, op_4a00_92_test_ff, 0x4a00, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a10_92_test_ff, 0x4a10, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a18_92_test_ff, 0x4a18, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a20_92_test_ff, 0x4a20, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a28_92_test_ff, 0x4a28, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a30_92_test_ff, 0x4a30, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a38_92_test_ff, 0x4a38, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a39_92_test_ff, 0x4a39, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3a_92_test_ff, 0x4a3a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3b_92_test_ff, 0x4a3b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3c_92_test_ff, 0x4a3c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a40_92_test_ff, 0x4a40, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a48_92_test_ff, 0x4a48, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a50_92_test_ff, 0x4a50, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a58_92_test_ff, 0x4a58, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a60_92_test_ff, 0x4a60, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a68_92_test_ff, 0x4a68, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a70_92_test_ff, 0x4a70, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a78_92_test_ff, 0x4a78, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a79_92_test_ff, 0x4a79, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7a_92_test_ff, 0x4a7a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7b_92_test_ff, 0x4a7b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7c_92_test_ff, 0x4a7c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a80_92_test_ff, 0x4a80, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a88_92_test_ff, 0x4a88, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a90_92_test_ff, 0x4a90, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a98_92_test_ff, 0x4a98, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa0_92_test_ff, 0x4aa0, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa8_92_test_ff, 0x4aa8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab0_92_test_ff, 0x4ab0, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4ab8_92_test_ff, 0x4ab8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab9_92_test_ff, 0x4ab9, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4aba_92_test_ff, 0x4aba, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abb_92_test_ff, 0x4abb, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abc_92_test_ff, 0x4abc, 6, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4ac0_92_test_ff, 0x4ac0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad0_92_test_ff, 0x4ad0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad8_92_test_ff, 0x4ad8, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae0_92_test_ff, 0x4ae0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae8_92_test_ff, 0x4ae8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af0_92_test_ff, 0x4af0, 2, { 2, 0 }, 0 }, /* TAS */
{ NULL, op_4af8_92_test_ff, 0x4af8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af9_92_test_ff, 0x4af9, 6, { 0, 0 }, 0 }, /* TAS */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c00_92_test_ff, 0x4c00, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c10_92_test_ff, 0x4c10, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c18_92_test_ff, 0x4c18, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c20_92_test_ff, 0x4c20, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c28_92_test_ff, 0x4c28, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c30_92_test_ff, 0x4c30, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c38_92_test_ff, 0x4c38, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c39_92_test_ff, 0x4c39, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3a_92_test_ff, 0x4c3a, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3b_92_test_ff, 0x4c3b, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3c_92_test_ff, 0x4c3c, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c40_92_test_ff, 0x4c40, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c50_92_test_ff, 0x4c50, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c58_92_test_ff, 0x4c58, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c60_92_test_ff, 0x4c60, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c68_92_test_ff, 0x4c68, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c70_92_test_ff, 0x4c70, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c78_92_test_ff, 0x4c78, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c79_92_test_ff, 0x4c79, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7a_92_test_ff, 0x4c7a, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7b_92_test_ff, 0x4c7b, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7c_92_test_ff, 0x4c7c, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
{ NULL, op_4c90_92_test_ff, 0x4c90, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4c98_92_test_ff, 0x4c98, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ca8_92_test_ff, 0x4ca8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb0_92_test_ff, 0x4cb0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb8_92_test_ff, 0x4cb8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb9_92_test_ff, 0x4cb9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cba_92_test_ff, 0x4cba, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cbb_92_test_ff, 0x4cbb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd0_92_test_ff, 0x4cd0, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd8_92_test_ff, 0x4cd8, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ce8_92_test_ff, 0x4ce8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf0_92_test_ff, 0x4cf0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf8_92_test_ff, 0x4cf8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf9_92_test_ff, 0x4cf9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfa_92_test_ff, 0x4cfa, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfb_92_test_ff, 0x4cfb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4e40_92_test_ff, 0x4e40, 2, { 0, 0 }, 0 }, /* TRAP */
{ NULL, op_4e50_92_test_ff, 0x4e50, 4, { 0, 0 }, 0 }, /* LINK */
{ NULL, op_4e58_92_test_ff, 0x4e58, 2, { 0, 0 }, 0 }, /* UNLK */
{ NULL, op_4e60_92_test_ff, 0x4e60, 2, { 0, 0 }, 0 }, /* MVR2USP */
{ NULL, op_4e68_92_test_ff, 0x4e68, 2, { 0, 0 }, 0 }, /* MVUSP2R */
{ NULL, op_4e70_92_test_ff, 0x4e70, 2, { 0, 0 }, 0 }, /* RESET */
{ NULL, op_4e71_92_test_ff, 0x4e71, 2, { 0, 0 }, 0 }, /* NOP */
{ NULL, op_4e72_92_test_ff, 0x4e72, 0, { 0, 0 }, 0 }, /* STOP */
{ NULL, op_4e73_92_test_ff, 0x4e73, 2, { 0, 0 }, 1 }, /* RTE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e74_92_test_ff, 0x4e74, 4, { 0, 0 }, 2 }, /* RTD */
#endif
{ NULL, op_4e75_92_test_ff, 0x4e75, 2, { 0, 0 }, 1 }, /* RTS */
{ NULL, op_4e76_92_test_ff, 0x4e76, 2, { 0, 0 }, 0 }, /* TRAPV */
{ NULL, op_4e77_92_test_ff, 0x4e77, 2, { 0, 0 }, 1 }, /* RTR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7a_92_test_ff, 0x4e7a, 4, { 0, 0 }, 0 }, /* MOVEC2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7b_92_test_ff, 0x4e7b, 4, { 0, 0 }, 0 }, /* MOVE2C */
#endif
{ NULL, op_4e90_92_test_ff, 0x4e90, 2, { 0, 0 }, 1 }, /* JSR */
{ NULL, op_4ea8_92_test_ff, 0x4ea8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb0_92_test_ff, 0x4eb0, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4eb8_92_test_ff, 0x4eb8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb9_92_test_ff, 0x4eb9, 6, { 0, 0 }, 3 }, /* JSR */
{ NULL, op_4eba_92_test_ff, 0x4eba, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4ebb_92_test_ff, 0x4ebb, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4ed0_92_test_ff, 0x4ed0, 2, { 0, 0 }, 1 }, /* JMP */
{ NULL, op_4ee8_92_test_ff, 0x4ee8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef0_92_test_ff, 0x4ef0, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_4ef8_92_test_ff, 0x4ef8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef9_92_test_ff, 0x4ef9, 6, { 0, 0 }, 3 }, /* JMP */
{ NULL, op_4efa_92_test_ff, 0x4efa, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4efb_92_test_ff, 0x4efb, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_5000_92_test_ff, 0x5000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5010_92_test_ff, 0x5010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5018_92_test_ff, 0x5018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5020_92_test_ff, 0x5020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5028_92_test_ff, 0x5028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5030_92_test_ff, 0x5030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5038_92_test_ff, 0x5038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5039_92_test_ff, 0x5039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5040_92_test_ff, 0x5040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5048_92_test_ff, 0x5048, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5050_92_test_ff, 0x5050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5058_92_test_ff, 0x5058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5060_92_test_ff, 0x5060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5068_92_test_ff, 0x5068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5070_92_test_ff, 0x5070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5078_92_test_ff, 0x5078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5079_92_test_ff, 0x5079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5080_92_test_ff, 0x5080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5088_92_test_ff, 0x5088, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5090_92_test_ff, 0x5090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5098_92_test_ff, 0x5098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a0_92_test_ff, 0x50a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a8_92_test_ff, 0x50a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b0_92_test_ff, 0x50b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_50b8_92_test_ff, 0x50b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b9_92_test_ff, 0x50b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50c0_92_test_ff, 0x50c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50c8_92_test_ff, 0x50c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_50d0_92_test_ff, 0x50d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50d8_92_test_ff, 0x50d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e0_92_test_ff, 0x50e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e8_92_test_ff, 0x50e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f0_92_test_ff, 0x50f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_50f8_92_test_ff, 0x50f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f9_92_test_ff, 0x50f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fa_92_test_ff, 0x50fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fb_92_test_ff, 0x50fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fc_92_test_ff, 0x50fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5100_92_test_ff, 0x5100, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5110_92_test_ff, 0x5110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5118_92_test_ff, 0x5118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5120_92_test_ff, 0x5120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5128_92_test_ff, 0x5128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5130_92_test_ff, 0x5130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5138_92_test_ff, 0x5138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5139_92_test_ff, 0x5139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5140_92_test_ff, 0x5140, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5148_92_test_ff, 0x5148, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5150_92_test_ff, 0x5150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5158_92_test_ff, 0x5158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5160_92_test_ff, 0x5160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5168_92_test_ff, 0x5168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5170_92_test_ff, 0x5170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5178_92_test_ff, 0x5178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5179_92_test_ff, 0x5179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5180_92_test_ff, 0x5180, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5188_92_test_ff, 0x5188, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5190_92_test_ff, 0x5190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5198_92_test_ff, 0x5198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a0_92_test_ff, 0x51a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a8_92_test_ff, 0x51a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b0_92_test_ff, 0x51b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_51b8_92_test_ff, 0x51b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b9_92_test_ff, 0x51b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51c0_92_test_ff, 0x51c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51c8_92_test_ff, 0x51c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_51d0_92_test_ff, 0x51d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51d8_92_test_ff, 0x51d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e0_92_test_ff, 0x51e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e8_92_test_ff, 0x51e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f0_92_test_ff, 0x51f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_51f8_92_test_ff, 0x51f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f9_92_test_ff, 0x51f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fa_92_test_ff, 0x51fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fb_92_test_ff, 0x51fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fc_92_test_ff, 0x51fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_52c0_92_test_ff, 0x52c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c8_92_test_ff, 0x52c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_52d0_92_test_ff, 0x52d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52d8_92_test_ff, 0x52d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e0_92_test_ff, 0x52e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e8_92_test_ff, 0x52e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f0_92_test_ff, 0x52f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_52f8_92_test_ff, 0x52f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f9_92_test_ff, 0x52f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fa_92_test_ff, 0x52fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fb_92_test_ff, 0x52fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fc_92_test_ff, 0x52fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_53c0_92_test_ff, 0x53c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c8_92_test_ff, 0x53c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_53d0_92_test_ff, 0x53d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53d8_92_test_ff, 0x53d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e0_92_test_ff, 0x53e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e8_92_test_ff, 0x53e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f0_92_test_ff, 0x53f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_53f8_92_test_ff, 0x53f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f9_92_test_ff, 0x53f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fa_92_test_ff, 0x53fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fb_92_test_ff, 0x53fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fc_92_test_ff, 0x53fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_54c0_92_test_ff, 0x54c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c8_92_test_ff, 0x54c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_54d0_92_test_ff, 0x54d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54d8_92_test_ff, 0x54d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e0_92_test_ff, 0x54e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e8_92_test_ff, 0x54e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f0_92_test_ff, 0x54f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_54f8_92_test_ff, 0x54f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f9_92_test_ff, 0x54f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fa_92_test_ff, 0x54fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fb_92_test_ff, 0x54fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fc_92_test_ff, 0x54fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_55c0_92_test_ff, 0x55c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c8_92_test_ff, 0x55c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_55d0_92_test_ff, 0x55d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55d8_92_test_ff, 0x55d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e0_92_test_ff, 0x55e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e8_92_test_ff, 0x55e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f0_92_test_ff, 0x55f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_55f8_92_test_ff, 0x55f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f9_92_test_ff, 0x55f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fa_92_test_ff, 0x55fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fb_92_test_ff, 0x55fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fc_92_test_ff, 0x55fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_56c0_92_test_ff, 0x56c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c8_92_test_ff, 0x56c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_56d0_92_test_ff, 0x56d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56d8_92_test_ff, 0x56d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e0_92_test_ff, 0x56e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e8_92_test_ff, 0x56e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f0_92_test_ff, 0x56f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_56f8_92_test_ff, 0x56f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f9_92_test_ff, 0x56f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fa_92_test_ff, 0x56fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fb_92_test_ff, 0x56fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fc_92_test_ff, 0x56fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_57c0_92_test_ff, 0x57c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c8_92_test_ff, 0x57c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_57d0_92_test_ff, 0x57d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57d8_92_test_ff, 0x57d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e0_92_test_ff, 0x57e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e8_92_test_ff, 0x57e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f0_92_test_ff, 0x57f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_57f8_92_test_ff, 0x57f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f9_92_test_ff, 0x57f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fa_92_test_ff, 0x57fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fb_92_test_ff, 0x57fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fc_92_test_ff, 0x57fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_58c0_92_test_ff, 0x58c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c8_92_test_ff, 0x58c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_58d0_92_test_ff, 0x58d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58d8_92_test_ff, 0x58d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e0_92_test_ff, 0x58e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e8_92_test_ff, 0x58e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f0_92_test_ff, 0x58f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_58f8_92_test_ff, 0x58f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f9_92_test_ff, 0x58f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fa_92_test_ff, 0x58fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fb_92_test_ff, 0x58fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fc_92_test_ff, 0x58fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_59c0_92_test_ff, 0x59c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c8_92_test_ff, 0x59c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_59d0_92_test_ff, 0x59d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59d8_92_test_ff, 0x59d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e0_92_test_ff, 0x59e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e8_92_test_ff, 0x59e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f0_92_test_ff, 0x59f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_59f8_92_test_ff, 0x59f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f9_92_test_ff, 0x59f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fa_92_test_ff, 0x59fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fb_92_test_ff, 0x59fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fc_92_test_ff, 0x59fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ac0_92_test_ff, 0x5ac0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac8_92_test_ff, 0x5ac8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ad0_92_test_ff, 0x5ad0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ad8_92_test_ff, 0x5ad8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae0_92_test_ff, 0x5ae0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae8_92_test_ff, 0x5ae8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af0_92_test_ff, 0x5af0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5af8_92_test_ff, 0x5af8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af9_92_test_ff, 0x5af9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afa_92_test_ff, 0x5afa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afb_92_test_ff, 0x5afb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afc_92_test_ff, 0x5afc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5bc0_92_test_ff, 0x5bc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc8_92_test_ff, 0x5bc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5bd0_92_test_ff, 0x5bd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bd8_92_test_ff, 0x5bd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be0_92_test_ff, 0x5be0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be8_92_test_ff, 0x5be8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf0_92_test_ff, 0x5bf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5bf8_92_test_ff, 0x5bf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf9_92_test_ff, 0x5bf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfa_92_test_ff, 0x5bfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfb_92_test_ff, 0x5bfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfc_92_test_ff, 0x5bfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5cc0_92_test_ff, 0x5cc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc8_92_test_ff, 0x5cc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5cd0_92_test_ff, 0x5cd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cd8_92_test_ff, 0x5cd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce0_92_test_ff, 0x5ce0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce8_92_test_ff, 0x5ce8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf0_92_test_ff, 0x5cf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5cf8_92_test_ff, 0x5cf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf9_92_test_ff, 0x5cf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfa_92_test_ff, 0x5cfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfb_92_test_ff, 0x5cfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfc_92_test_ff, 0x5cfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5dc0_92_test_ff, 0x5dc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc8_92_test_ff, 0x5dc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5dd0_92_test_ff, 0x5dd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dd8_92_test_ff, 0x5dd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de0_92_test_ff, 0x5de0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de8_92_test_ff, 0x5de8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df0_92_test_ff, 0x5df0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5df8_92_test_ff, 0x5df8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df9_92_test_ff, 0x5df9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfa_92_test_ff, 0x5dfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfb_92_test_ff, 0x5dfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfc_92_test_ff, 0x5dfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ec0_92_test_ff, 0x5ec0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec8_92_test_ff, 0x5ec8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ed0_92_test_ff, 0x5ed0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ed8_92_test_ff, 0x5ed8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee0_92_test_ff, 0x5ee0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee8_92_test_ff, 0x5ee8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef0_92_test_ff, 0x5ef0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ef8_92_test_ff, 0x5ef8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef9_92_test_ff, 0x5ef9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efa_92_test_ff, 0x5efa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efb_92_test_ff, 0x5efb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efc_92_test_ff, 0x5efc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5fc0_92_test_ff, 0x5fc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc8_92_test_ff, 0x5fc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5fd0_92_test_ff, 0x5fd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fd8_92_test_ff, 0x5fd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe0_92_test_ff, 0x5fe0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe8_92_test_ff, 0x5fe8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff0_92_test_ff, 0x5ff0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ff8_92_test_ff, 0x5ff8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff9_92_test_ff, 0x5ff9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffa_92_test_ff, 0x5ffa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffb_92_test_ff, 0x5ffb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffc_92_test_ff, 0x5ffc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_6000_92_test_ff, 0x6000, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6001_92_test_ff, 0x6001, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_60ff_92_test_ff, 0x60ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6100_92_test_ff, 0x6100, 4, { 0, 0 }, 2 }, /* BSR */
{ NULL, op_6101_92_test_ff, 0x6101, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_61ff_92_test_ff, 0x61ff, 6, { 0, 0 }, 3 }, /* BSR */
{ NULL, op_6200_92_test_ff, 0x6200, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6201_92_test_ff, 0x6201, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_62ff_92_test_ff, 0x62ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6300_92_test_ff, 0x6300, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6301_92_test_ff, 0x6301, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_63ff_92_test_ff, 0x63ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6400_92_test_ff, 0x6400, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6401_92_test_ff, 0x6401, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_64ff_92_test_ff, 0x64ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6500_92_test_ff, 0x6500, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6501_92_test_ff, 0x6501, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_65ff_92_test_ff, 0x65ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6600_92_test_ff, 0x6600, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6601_92_test_ff, 0x6601, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_66ff_92_test_ff, 0x66ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6700_92_test_ff, 0x6700, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6701_92_test_ff, 0x6701, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_67ff_92_test_ff, 0x67ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6800_92_test_ff, 0x6800, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6801_92_test_ff, 0x6801, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_68ff_92_test_ff, 0x68ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6900_92_test_ff, 0x6900, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6901_92_test_ff, 0x6901, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_69ff_92_test_ff, 0x69ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6a00_92_test_ff, 0x6a00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6a01_92_test_ff, 0x6a01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6aff_92_test_ff, 0x6aff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6b00_92_test_ff, 0x6b00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6b01_92_test_ff, 0x6b01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6bff_92_test_ff, 0x6bff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6c00_92_test_ff, 0x6c00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6c01_92_test_ff, 0x6c01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6cff_92_test_ff, 0x6cff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6d00_92_test_ff, 0x6d00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6d01_92_test_ff, 0x6d01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6dff_92_test_ff, 0x6dff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6e00_92_test_ff, 0x6e00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6e01_92_test_ff, 0x6e01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6eff_92_test_ff, 0x6eff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6f00_92_test_ff, 0x6f00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6f01_92_test_ff, 0x6f01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6fff_92_test_ff, 0x6fff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_7000_92_test_ff, 0x7000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_8000_92_test_ff, 0x8000, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8010_92_test_ff, 0x8010, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8018_92_test_ff, 0x8018, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8020_92_test_ff, 0x8020, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8028_92_test_ff, 0x8028, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8030_92_test_ff, 0x8030, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8038_92_test_ff, 0x8038, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8039_92_test_ff, 0x8039, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803a_92_test_ff, 0x803a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803b_92_test_ff, 0x803b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_803c_92_test_ff, 0x803c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8040_92_test_ff, 0x8040, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8050_92_test_ff, 0x8050, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8058_92_test_ff, 0x8058, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8060_92_test_ff, 0x8060, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8068_92_test_ff, 0x8068, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8070_92_test_ff, 0x8070, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8078_92_test_ff, 0x8078, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8079_92_test_ff, 0x8079, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807a_92_test_ff, 0x807a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807b_92_test_ff, 0x807b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_807c_92_test_ff, 0x807c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8080_92_test_ff, 0x8080, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8090_92_test_ff, 0x8090, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8098_92_test_ff, 0x8098, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a0_92_test_ff, 0x80a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a8_92_test_ff, 0x80a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b0_92_test_ff, 0x80b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80b8_92_test_ff, 0x80b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b9_92_test_ff, 0x80b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80ba_92_test_ff, 0x80ba, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80bb_92_test_ff, 0x80bb, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80bc_92_test_ff, 0x80bc, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80c0_92_test_ff, 0x80c0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d0_92_test_ff, 0x80d0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d8_92_test_ff, 0x80d8, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e0_92_test_ff, 0x80e0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e8_92_test_ff, 0x80e8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f0_92_test_ff, 0x80f0, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80f8_92_test_ff, 0x80f8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f9_92_test_ff, 0x80f9, 6, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fa_92_test_ff, 0x80fa, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fb_92_test_ff, 0x80fb, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80fc_92_test_ff, 0x80fc, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_8100_92_test_ff, 0x8100, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8108_92_test_ff, 0x8108, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8110_92_test_ff, 0x8110, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8118_92_test_ff, 0x8118, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8120_92_test_ff, 0x8120, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8128_92_test_ff, 0x8128, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8130_92_test_ff, 0x8130, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8138_92_test_ff, 0x8138, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8139_92_test_ff, 0x8139, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8140_92_test_ff, 0x8140, 4, { 0, 0 }, 0 }, /* PACK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8148_92_test_ff, 0x8148, 4, { 0, 0 }, 0 }, /* PACK */
#endif
{ NULL, op_8150_92_test_ff, 0x8150, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8158_92_test_ff, 0x8158, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8160_92_test_ff, 0x8160, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8168_92_test_ff, 0x8168, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8170_92_test_ff, 0x8170, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8178_92_test_ff, 0x8178, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8179_92_test_ff, 0x8179, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8180_92_test_ff, 0x8180, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8188_92_test_ff, 0x8188, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
{ NULL, op_8190_92_test_ff, 0x8190, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8198_92_test_ff, 0x8198, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a0_92_test_ff, 0x81a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a8_92_test_ff, 0x81a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b0_92_test_ff, 0x81b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_81b8_92_test_ff, 0x81b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b9_92_test_ff, 0x81b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81c0_92_test_ff, 0x81c0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d0_92_test_ff, 0x81d0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d8_92_test_ff, 0x81d8, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e0_92_test_ff, 0x81e0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e8_92_test_ff, 0x81e8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f0_92_test_ff, 0x81f0, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81f8_92_test_ff, 0x81f8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f9_92_test_ff, 0x81f9, 6, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fa_92_test_ff, 0x81fa, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fb_92_test_ff, 0x81fb, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81fc_92_test_ff, 0x81fc, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_9000_92_test_ff, 0x9000, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9010_92_test_ff, 0x9010, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9018_92_test_ff, 0x9018, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9020_92_test_ff, 0x9020, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9028_92_test_ff, 0x9028, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9030_92_test_ff, 0x9030, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9038_92_test_ff, 0x9038, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9039_92_test_ff, 0x9039, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903a_92_test_ff, 0x903a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903b_92_test_ff, 0x903b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_903c_92_test_ff, 0x903c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9040_92_test_ff, 0x9040, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9048_92_test_ff, 0x9048, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9050_92_test_ff, 0x9050, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9058_92_test_ff, 0x9058, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9060_92_test_ff, 0x9060, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9068_92_test_ff, 0x9068, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9070_92_test_ff, 0x9070, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9078_92_test_ff, 0x9078, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9079_92_test_ff, 0x9079, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907a_92_test_ff, 0x907a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907b_92_test_ff, 0x907b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_907c_92_test_ff, 0x907c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9080_92_test_ff, 0x9080, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9088_92_test_ff, 0x9088, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9090_92_test_ff, 0x9090, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9098_92_test_ff, 0x9098, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a0_92_test_ff, 0x90a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a8_92_test_ff, 0x90a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b0_92_test_ff, 0x90b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90b8_92_test_ff, 0x90b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b9_92_test_ff, 0x90b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90ba_92_test_ff, 0x90ba, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90bb_92_test_ff, 0x90bb, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90bc_92_test_ff, 0x90bc, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90c0_92_test_ff, 0x90c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90c8_92_test_ff, 0x90c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d0_92_test_ff, 0x90d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d8_92_test_ff, 0x90d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e0_92_test_ff, 0x90e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e8_92_test_ff, 0x90e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f0_92_test_ff, 0x90f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90f8_92_test_ff, 0x90f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f9_92_test_ff, 0x90f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fa_92_test_ff, 0x90fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fb_92_test_ff, 0x90fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90fc_92_test_ff, 0x90fc, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_9100_92_test_ff, 0x9100, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9108_92_test_ff, 0x9108, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9110_92_test_ff, 0x9110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9118_92_test_ff, 0x9118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9120_92_test_ff, 0x9120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9128_92_test_ff, 0x9128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9130_92_test_ff, 0x9130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9138_92_test_ff, 0x9138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9139_92_test_ff, 0x9139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9140_92_test_ff, 0x9140, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9148_92_test_ff, 0x9148, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9150_92_test_ff, 0x9150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9158_92_test_ff, 0x9158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9160_92_test_ff, 0x9160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9168_92_test_ff, 0x9168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9170_92_test_ff, 0x9170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9178_92_test_ff, 0x9178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9179_92_test_ff, 0x9179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9180_92_test_ff, 0x9180, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9188_92_test_ff, 0x9188, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9190_92_test_ff, 0x9190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9198_92_test_ff, 0x9198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a0_92_test_ff, 0x91a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a8_92_test_ff, 0x91a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b0_92_test_ff, 0x91b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_91b8_92_test_ff, 0x91b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b9_92_test_ff, 0x91b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91c0_92_test_ff, 0x91c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91c8_92_test_ff, 0x91c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d0_92_test_ff, 0x91d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d8_92_test_ff, 0x91d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e0_92_test_ff, 0x91e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e8_92_test_ff, 0x91e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f0_92_test_ff, 0x91f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91f8_92_test_ff, 0x91f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f9_92_test_ff, 0x91f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fa_92_test_ff, 0x91fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fb_92_test_ff, 0x91fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91fc_92_test_ff, 0x91fc, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_b000_92_test_ff, 0xb000, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b010_92_test_ff, 0xb010, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b018_92_test_ff, 0xb018, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b020_92_test_ff, 0xb020, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b028_92_test_ff, 0xb028, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b030_92_test_ff, 0xb030, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b038_92_test_ff, 0xb038, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b039_92_test_ff, 0xb039, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03a_92_test_ff, 0xb03a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03b_92_test_ff, 0xb03b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b03c_92_test_ff, 0xb03c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b040_92_test_ff, 0xb040, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b048_92_test_ff, 0xb048, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b050_92_test_ff, 0xb050, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b058_92_test_ff, 0xb058, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b060_92_test_ff, 0xb060, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b068_92_test_ff, 0xb068, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b070_92_test_ff, 0xb070, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b078_92_test_ff, 0xb078, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b079_92_test_ff, 0xb079, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07a_92_test_ff, 0xb07a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07b_92_test_ff, 0xb07b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b07c_92_test_ff, 0xb07c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b080_92_test_ff, 0xb080, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b088_92_test_ff, 0xb088, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b090_92_test_ff, 0xb090, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b098_92_test_ff, 0xb098, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a0_92_test_ff, 0xb0a0, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a8_92_test_ff, 0xb0a8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b0_92_test_ff, 0xb0b0, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0b8_92_test_ff, 0xb0b8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b9_92_test_ff, 0xb0b9, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0ba_92_test_ff, 0xb0ba, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0bb_92_test_ff, 0xb0bb, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0bc_92_test_ff, 0xb0bc, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0c0_92_test_ff, 0xb0c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0c8_92_test_ff, 0xb0c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d0_92_test_ff, 0xb0d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d8_92_test_ff, 0xb0d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e0_92_test_ff, 0xb0e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e8_92_test_ff, 0xb0e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f0_92_test_ff, 0xb0f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f8_92_test_ff, 0xb0f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f9_92_test_ff, 0xb0f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fa_92_test_ff, 0xb0fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fb_92_test_ff, 0xb0fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fc_92_test_ff, 0xb0fc, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b100_92_test_ff, 0xb100, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b108_92_test_ff, 0xb108, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b110_92_test_ff, 0xb110, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b118_92_test_ff, 0xb118, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b120_92_test_ff, 0xb120, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b128_92_test_ff, 0xb128, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b130_92_test_ff, 0xb130, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b138_92_test_ff, 0xb138, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b139_92_test_ff, 0xb139, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b140_92_test_ff, 0xb140, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b148_92_test_ff, 0xb148, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b150_92_test_ff, 0xb150, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b158_92_test_ff, 0xb158, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b160_92_test_ff, 0xb160, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b168_92_test_ff, 0xb168, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b170_92_test_ff, 0xb170, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b178_92_test_ff, 0xb178, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b179_92_test_ff, 0xb179, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b180_92_test_ff, 0xb180, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b188_92_test_ff, 0xb188, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b190_92_test_ff, 0xb190, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b198_92_test_ff, 0xb198, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a0_92_test_ff, 0xb1a0, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a8_92_test_ff, 0xb1a8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b0_92_test_ff, 0xb1b0, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b1b8_92_test_ff, 0xb1b8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b9_92_test_ff, 0xb1b9, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1c0_92_test_ff, 0xb1c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1c8_92_test_ff, 0xb1c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d0_92_test_ff, 0xb1d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d8_92_test_ff, 0xb1d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e0_92_test_ff, 0xb1e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e8_92_test_ff, 0xb1e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f0_92_test_ff, 0xb1f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f8_92_test_ff, 0xb1f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f9_92_test_ff, 0xb1f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fa_92_test_ff, 0xb1fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fb_92_test_ff, 0xb1fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fc_92_test_ff, 0xb1fc, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_c000_92_test_ff, 0xc000, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c010_92_test_ff, 0xc010, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c018_92_test_ff, 0xc018, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c020_92_test_ff, 0xc020, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c028_92_test_ff, 0xc028, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c030_92_test_ff, 0xc030, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c038_92_test_ff, 0xc038, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c039_92_test_ff, 0xc039, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03a_92_test_ff, 0xc03a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03b_92_test_ff, 0xc03b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c03c_92_test_ff, 0xc03c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c040_92_test_ff, 0xc040, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c050_92_test_ff, 0xc050, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c058_92_test_ff, 0xc058, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c060_92_test_ff, 0xc060, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c068_92_test_ff, 0xc068, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c070_92_test_ff, 0xc070, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c078_92_test_ff, 0xc078, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c079_92_test_ff, 0xc079, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07a_92_test_ff, 0xc07a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07b_92_test_ff, 0xc07b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c07c_92_test_ff, 0xc07c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c080_92_test_ff, 0xc080, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c090_92_test_ff, 0xc090, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c098_92_test_ff, 0xc098, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a0_92_test_ff, 0xc0a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a8_92_test_ff, 0xc0a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b0_92_test_ff, 0xc0b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0b8_92_test_ff, 0xc0b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b9_92_test_ff, 0xc0b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0ba_92_test_ff, 0xc0ba, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0bb_92_test_ff, 0xc0bb, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0bc_92_test_ff, 0xc0bc, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0c0_92_test_ff, 0xc0c0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d0_92_test_ff, 0xc0d0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d8_92_test_ff, 0xc0d8, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e0_92_test_ff, 0xc0e0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e8_92_test_ff, 0xc0e8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f0_92_test_ff, 0xc0f0, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0f8_92_test_ff, 0xc0f8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f9_92_test_ff, 0xc0f9, 6, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fa_92_test_ff, 0xc0fa, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fb_92_test_ff, 0xc0fb, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0fc_92_test_ff, 0xc0fc, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c100_92_test_ff, 0xc100, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c108_92_test_ff, 0xc108, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c110_92_test_ff, 0xc110, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c118_92_test_ff, 0xc118, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c120_92_test_ff, 0xc120, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c128_92_test_ff, 0xc128, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c130_92_test_ff, 0xc130, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c138_92_test_ff, 0xc138, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c139_92_test_ff, 0xc139, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c140_92_test_ff, 0xc140, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c148_92_test_ff, 0xc148, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c150_92_test_ff, 0xc150, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c158_92_test_ff, 0xc158, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c160_92_test_ff, 0xc160, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c168_92_test_ff, 0xc168, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c170_92_test_ff, 0xc170, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c178_92_test_ff, 0xc178, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c179_92_test_ff, 0xc179, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c188_92_test_ff, 0xc188, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c190_92_test_ff, 0xc190, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c198_92_test_ff, 0xc198, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a0_92_test_ff, 0xc1a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a8_92_test_ff, 0xc1a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b0_92_test_ff, 0xc1b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c1b8_92_test_ff, 0xc1b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b9_92_test_ff, 0xc1b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1c0_92_test_ff, 0xc1c0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d0_92_test_ff, 0xc1d0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d8_92_test_ff, 0xc1d8, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e0_92_test_ff, 0xc1e0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e8_92_test_ff, 0xc1e8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f0_92_test_ff, 0xc1f0, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1f8_92_test_ff, 0xc1f8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f9_92_test_ff, 0xc1f9, 6, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fa_92_test_ff, 0xc1fa, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fb_92_test_ff, 0xc1fb, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1fc_92_test_ff, 0xc1fc, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_d000_92_test_ff, 0xd000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d010_92_test_ff, 0xd010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d018_92_test_ff, 0xd018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d020_92_test_ff, 0xd020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d028_92_test_ff, 0xd028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d030_92_test_ff, 0xd030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d038_92_test_ff, 0xd038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d039_92_test_ff, 0xd039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03a_92_test_ff, 0xd03a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03b_92_test_ff, 0xd03b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d03c_92_test_ff, 0xd03c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d040_92_test_ff, 0xd040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d048_92_test_ff, 0xd048, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d050_92_test_ff, 0xd050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d058_92_test_ff, 0xd058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d060_92_test_ff, 0xd060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d068_92_test_ff, 0xd068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d070_92_test_ff, 0xd070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d078_92_test_ff, 0xd078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d079_92_test_ff, 0xd079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07a_92_test_ff, 0xd07a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07b_92_test_ff, 0xd07b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d07c_92_test_ff, 0xd07c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d080_92_test_ff, 0xd080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d088_92_test_ff, 0xd088, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d090_92_test_ff, 0xd090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d098_92_test_ff, 0xd098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a0_92_test_ff, 0xd0a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a8_92_test_ff, 0xd0a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b0_92_test_ff, 0xd0b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0b8_92_test_ff, 0xd0b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b9_92_test_ff, 0xd0b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0ba_92_test_ff, 0xd0ba, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0bb_92_test_ff, 0xd0bb, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0bc_92_test_ff, 0xd0bc, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0c0_92_test_ff, 0xd0c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0c8_92_test_ff, 0xd0c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d0_92_test_ff, 0xd0d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d8_92_test_ff, 0xd0d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e0_92_test_ff, 0xd0e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e8_92_test_ff, 0xd0e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f0_92_test_ff, 0xd0f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f8_92_test_ff, 0xd0f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f9_92_test_ff, 0xd0f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fa_92_test_ff, 0xd0fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fb_92_test_ff, 0xd0fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fc_92_test_ff, 0xd0fc, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d100_92_test_ff, 0xd100, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d108_92_test_ff, 0xd108, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d110_92_test_ff, 0xd110, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d118_92_test_ff, 0xd118, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d120_92_test_ff, 0xd120, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d128_92_test_ff, 0xd128, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d130_92_test_ff, 0xd130, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d138_92_test_ff, 0xd138, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d139_92_test_ff, 0xd139, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d140_92_test_ff, 0xd140, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d148_92_test_ff, 0xd148, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d150_92_test_ff, 0xd150, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d158_92_test_ff, 0xd158, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d160_92_test_ff, 0xd160, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d168_92_test_ff, 0xd168, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d170_92_test_ff, 0xd170, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d178_92_test_ff, 0xd178, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d179_92_test_ff, 0xd179, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d180_92_test_ff, 0xd180, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d188_92_test_ff, 0xd188, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d190_92_test_ff, 0xd190, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d198_92_test_ff, 0xd198, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a0_92_test_ff, 0xd1a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a8_92_test_ff, 0xd1a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b0_92_test_ff, 0xd1b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d1b8_92_test_ff, 0xd1b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b9_92_test_ff, 0xd1b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1c0_92_test_ff, 0xd1c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1c8_92_test_ff, 0xd1c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d0_92_test_ff, 0xd1d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d8_92_test_ff, 0xd1d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e0_92_test_ff, 0xd1e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e8_92_test_ff, 0xd1e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f0_92_test_ff, 0xd1f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f8_92_test_ff, 0xd1f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f9_92_test_ff, 0xd1f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fa_92_test_ff, 0xd1fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fb_92_test_ff, 0xd1fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fc_92_test_ff, 0xd1fc, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_e000_92_test_ff, 0xe000, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e008_92_test_ff, 0xe008, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e010_92_test_ff, 0xe010, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e018_92_test_ff, 0xe018, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e020_92_test_ff, 0xe020, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e028_92_test_ff, 0xe028, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e030_92_test_ff, 0xe030, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e038_92_test_ff, 0xe038, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e040_92_test_ff, 0xe040, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e048_92_test_ff, 0xe048, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e050_92_test_ff, 0xe050, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e058_92_test_ff, 0xe058, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e060_92_test_ff, 0xe060, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e068_92_test_ff, 0xe068, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e070_92_test_ff, 0xe070, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e078_92_test_ff, 0xe078, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e080_92_test_ff, 0xe080, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e088_92_test_ff, 0xe088, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e090_92_test_ff, 0xe090, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e098_92_test_ff, 0xe098, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0a0_92_test_ff, 0xe0a0, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e0a8_92_test_ff, 0xe0a8, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e0b0_92_test_ff, 0xe0b0, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e0b8_92_test_ff, 0xe0b8, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0d0_92_test_ff, 0xe0d0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0d8_92_test_ff, 0xe0d8, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e0_92_test_ff, 0xe0e0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e8_92_test_ff, 0xe0e8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f0_92_test_ff, 0xe0f0, 2, { 2, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f8_92_test_ff, 0xe0f8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f9_92_test_ff, 0xe0f9, 6, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e100_92_test_ff, 0xe100, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e108_92_test_ff, 0xe108, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e110_92_test_ff, 0xe110, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e118_92_test_ff, 0xe118, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e120_92_test_ff, 0xe120, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e128_92_test_ff, 0xe128, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e130_92_test_ff, 0xe130, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e138_92_test_ff, 0xe138, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e140_92_test_ff, 0xe140, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e148_92_test_ff, 0xe148, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e150_92_test_ff, 0xe150, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e158_92_test_ff, 0xe158, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e160_92_test_ff, 0xe160, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e168_92_test_ff, 0xe168, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e170_92_test_ff, 0xe170, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e178_92_test_ff, 0xe178, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e180_92_test_ff, 0xe180, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e188_92_test_ff, 0xe188, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e190_92_test_ff, 0xe190, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e198_92_test_ff, 0xe198, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1a0_92_test_ff, 0xe1a0, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e1a8_92_test_ff, 0xe1a8, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e1b0_92_test_ff, 0xe1b0, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e1b8_92_test_ff, 0xe1b8, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1d0_92_test_ff, 0xe1d0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1d8_92_test_ff, 0xe1d8, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e0_92_test_ff, 0xe1e0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e8_92_test_ff, 0xe1e8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f0_92_test_ff, 0xe1f0, 2, { 2, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f8_92_test_ff, 0xe1f8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f9_92_test_ff, 0xe1f9, 6, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e2d0_92_test_ff, 0xe2d0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2d8_92_test_ff, 0xe2d8, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e0_92_test_ff, 0xe2e0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e8_92_test_ff, 0xe2e8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f0_92_test_ff, 0xe2f0, 2, { 2, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f8_92_test_ff, 0xe2f8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f9_92_test_ff, 0xe2f9, 6, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e3d0_92_test_ff, 0xe3d0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3d8_92_test_ff, 0xe3d8, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e0_92_test_ff, 0xe3e0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e8_92_test_ff, 0xe3e8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f0_92_test_ff, 0xe3f0, 2, { 2, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f8_92_test_ff, 0xe3f8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f9_92_test_ff, 0xe3f9, 6, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e4d0_92_test_ff, 0xe4d0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4d8_92_test_ff, 0xe4d8, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e0_92_test_ff, 0xe4e0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e8_92_test_ff, 0xe4e8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f0_92_test_ff, 0xe4f0, 2, { 2, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f8_92_test_ff, 0xe4f8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f9_92_test_ff, 0xe4f9, 6, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e5d0_92_test_ff, 0xe5d0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5d8_92_test_ff, 0xe5d8, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e0_92_test_ff, 0xe5e0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e8_92_test_ff, 0xe5e8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f0_92_test_ff, 0xe5f0, 2, { 2, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f8_92_test_ff, 0xe5f8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f9_92_test_ff, 0xe5f9, 6, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e6d0_92_test_ff, 0xe6d0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6d8_92_test_ff, 0xe6d8, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e0_92_test_ff, 0xe6e0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e8_92_test_ff, 0xe6e8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f0_92_test_ff, 0xe6f0, 2, { 2, 0 }, 0 }, /* RORW */
{ NULL, op_e6f8_92_test_ff, 0xe6f8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f9_92_test_ff, 0xe6f9, 6, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e7d0_92_test_ff, 0xe7d0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7d8_92_test_ff, 0xe7d8, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e0_92_test_ff, 0xe7e0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e8_92_test_ff, 0xe7e8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f0_92_test_ff, 0xe7f0, 2, { 2, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f8_92_test_ff, 0xe7f8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f9_92_test_ff, 0xe7f9, 6, { 0, 0 }, 0 }, /* ROLW */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8c0_92_test_ff, 0xe8c0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8d0_92_test_ff, 0xe8d0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8e8_92_test_ff, 0xe8e8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f0_92_test_ff, 0xe8f0, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f8_92_test_ff, 0xe8f8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f9_92_test_ff, 0xe8f9, 8, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fa_92_test_ff, 0xe8fa, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fb_92_test_ff, 0xe8fb, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9c0_92_test_ff, 0xe9c0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9d0_92_test_ff, 0xe9d0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9e8_92_test_ff, 0xe9e8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f0_92_test_ff, 0xe9f0, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f8_92_test_ff, 0xe9f8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f9_92_test_ff, 0xe9f9, 8, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fa_92_test_ff, 0xe9fa, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fb_92_test_ff, 0xe9fb, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eac0_92_test_ff, 0xeac0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ead0_92_test_ff, 0xead0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eae8_92_test_ff, 0xeae8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf0_92_test_ff, 0xeaf0, 4, { 2, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf8_92_test_ff, 0xeaf8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf9_92_test_ff, 0xeaf9, 8, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebc0_92_test_ff, 0xebc0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebd0_92_test_ff, 0xebd0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebe8_92_test_ff, 0xebe8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf0_92_test_ff, 0xebf0, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf8_92_test_ff, 0xebf8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf9_92_test_ff, 0xebf9, 8, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfa_92_test_ff, 0xebfa, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfb_92_test_ff, 0xebfb, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecc0_92_test_ff, 0xecc0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecd0_92_test_ff, 0xecd0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ece8_92_test_ff, 0xece8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf0_92_test_ff, 0xecf0, 4, { 2, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf8_92_test_ff, 0xecf8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf9_92_test_ff, 0xecf9, 8, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edc0_92_test_ff, 0xedc0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edd0_92_test_ff, 0xedd0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ede8_92_test_ff, 0xede8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf0_92_test_ff, 0xedf0, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf8_92_test_ff, 0xedf8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf9_92_test_ff, 0xedf9, 8, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfa_92_test_ff, 0xedfa, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfb_92_test_ff, 0xedfb, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eec0_92_test_ff, 0xeec0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eed0_92_test_ff, 0xeed0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eee8_92_test_ff, 0xeee8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef0_92_test_ff, 0xeef0, 4, { 2, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef8_92_test_ff, 0xeef8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef9_92_test_ff, 0xeef9, 8, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efc0_92_test_ff, 0xefc0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efd0_92_test_ff, 0xefd0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efe8_92_test_ff, 0xefe8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff0_92_test_ff, 0xeff0, 4, { 2, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff8_92_test_ff, 0xeff8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff9_92_test_ff, 0xeff9, 8, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f200_92_test_ff, 0xf200, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f208_92_test_ff, 0xf208, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f210_92_test_ff, 0xf210, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f218_92_test_ff, 0xf218, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f220_92_test_ff, 0xf220, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f228_92_test_ff, 0xf228, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f230_92_test_ff, 0xf230, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f238_92_test_ff, 0xf238, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f239_92_test_ff, 0xf239, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23a_92_test_ff, 0xf23a, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23b_92_test_ff, 0xf23b, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23c_92_test_ff, 0xf23c, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f240_92_test_ff, 0xf240, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f248_92_test_ff, 0xf248, -1, { 0, 0 }, 0 }, /* FDBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f250_92_test_ff, 0xf250, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f258_92_test_ff, 0xf258, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f260_92_test_ff, 0xf260, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f268_92_test_ff, 0xf268, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f270_92_test_ff, 0xf270, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f278_92_test_ff, 0xf278, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f279_92_test_ff, 0xf279, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27a_92_test_ff, 0xf27a, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27b_92_test_ff, 0xf27b, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27c_92_test_ff, 0xf27c, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f280_92_test_ff, 0xf280, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f2c0_92_test_ff, 0xf2c0, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f310_92_test_ff, 0xf310, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f320_92_test_ff, 0xf320, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f328_92_test_ff, 0xf328, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f330_92_test_ff, 0xf330, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f338_92_test_ff, 0xf338, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f339_92_test_ff, 0xf339, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f350_92_test_ff, 0xf350, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f358_92_test_ff, 0xf358, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f368_92_test_ff, 0xf368, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f370_92_test_ff, 0xf370, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f378_92_test_ff, 0xf378, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f379_92_test_ff, 0xf379, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37a_92_test_ff, 0xf37a, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37b_92_test_ff, 0xf37b, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
{ 0 }};
#endif /* CPUEMU_92 */
#ifdef CPUEMU_93
const struct cputbl CPUFUNC(op_smalltbl_93_test)[] = {
{ NULL, op_0000_93_test_ff, 0x0000, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0010_93_test_ff, 0x0010, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0018_93_test_ff, 0x0018, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0020_93_test_ff, 0x0020, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0028_93_test_ff, 0x0028, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0030_93_test_ff, 0x0030, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0038_93_test_ff, 0x0038, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0039_93_test_ff, 0x0039, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_003c_93_test_ff, 0x003c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0040_93_test_ff, 0x0040, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0050_93_test_ff, 0x0050, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0058_93_test_ff, 0x0058, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0060_93_test_ff, 0x0060, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0068_93_test_ff, 0x0068, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0070_93_test_ff, 0x0070, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0078_93_test_ff, 0x0078, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0079_93_test_ff, 0x0079, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_007c_93_test_ff, 0x007c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0080_93_test_ff, 0x0080, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0090_93_test_ff, 0x0090, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0098_93_test_ff, 0x0098, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a0_93_test_ff, 0x00a0, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a8_93_test_ff, 0x00a8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b0_93_test_ff, 0x00b0, 6, { 2, 0 }, 0 }, /* OR */
{ NULL, op_00b8_93_test_ff, 0x00b8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b9_93_test_ff, 0x00b9, 10, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00d0_93_test_ff, 0x00d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00e8_93_test_ff, 0x00e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f0_93_test_ff, 0x00f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f8_93_test_ff, 0x00f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f9_93_test_ff, 0x00f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fa_93_test_ff, 0x00fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fb_93_test_ff, 0x00fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0100_93_test_ff, 0x0100, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0108_93_test_ff, 0x0108, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0110_93_test_ff, 0x0110, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0118_93_test_ff, 0x0118, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0120_93_test_ff, 0x0120, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0128_93_test_ff, 0x0128, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0130_93_test_ff, 0x0130, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0138_93_test_ff, 0x0138, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0139_93_test_ff, 0x0139, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013a_93_test_ff, 0x013a, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013b_93_test_ff, 0x013b, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_013c_93_test_ff, 0x013c, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0140_93_test_ff, 0x0140, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0148_93_test_ff, 0x0148, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0150_93_test_ff, 0x0150, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0158_93_test_ff, 0x0158, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0160_93_test_ff, 0x0160, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0168_93_test_ff, 0x0168, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0170_93_test_ff, 0x0170, 2, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0178_93_test_ff, 0x0178, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0179_93_test_ff, 0x0179, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0180_93_test_ff, 0x0180, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0188_93_test_ff, 0x0188, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_0190_93_test_ff, 0x0190, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0198_93_test_ff, 0x0198, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a0_93_test_ff, 0x01a0, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a8_93_test_ff, 0x01a8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b0_93_test_ff, 0x01b0, 2, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_01b8_93_test_ff, 0x01b8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b9_93_test_ff, 0x01b9, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01c0_93_test_ff, 0x01c0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01c8_93_test_ff, 0x01c8, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_01d0_93_test_ff, 0x01d0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01d8_93_test_ff, 0x01d8, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e0_93_test_ff, 0x01e0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e8_93_test_ff, 0x01e8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f0_93_test_ff, 0x01f0, 2, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_01f8_93_test_ff, 0x01f8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f9_93_test_ff, 0x01f9, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0200_93_test_ff, 0x0200, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0210_93_test_ff, 0x0210, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0218_93_test_ff, 0x0218, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0220_93_test_ff, 0x0220, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0228_93_test_ff, 0x0228, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0230_93_test_ff, 0x0230, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0238_93_test_ff, 0x0238, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0239_93_test_ff, 0x0239, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_023c_93_test_ff, 0x023c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0240_93_test_ff, 0x0240, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0250_93_test_ff, 0x0250, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0258_93_test_ff, 0x0258, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0260_93_test_ff, 0x0260, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0268_93_test_ff, 0x0268, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0270_93_test_ff, 0x0270, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0278_93_test_ff, 0x0278, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0279_93_test_ff, 0x0279, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_027c_93_test_ff, 0x027c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0280_93_test_ff, 0x0280, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0290_93_test_ff, 0x0290, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0298_93_test_ff, 0x0298, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a0_93_test_ff, 0x02a0, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a8_93_test_ff, 0x02a8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b0_93_test_ff, 0x02b0, 6, { 2, 0 }, 0 }, /* AND */
{ NULL, op_02b8_93_test_ff, 0x02b8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b9_93_test_ff, 0x02b9, 10, { 0, 0 }, 0 }, /* AND */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02d0_93_test_ff, 0x02d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02e8_93_test_ff, 0x02e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f0_93_test_ff, 0x02f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f8_93_test_ff, 0x02f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f9_93_test_ff, 0x02f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fa_93_test_ff, 0x02fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fb_93_test_ff, 0x02fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0400_93_test_ff, 0x0400, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0410_93_test_ff, 0x0410, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0418_93_test_ff, 0x0418, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0420_93_test_ff, 0x0420, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0428_93_test_ff, 0x0428, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0430_93_test_ff, 0x0430, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0438_93_test_ff, 0x0438, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0439_93_test_ff, 0x0439, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0440_93_test_ff, 0x0440, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0450_93_test_ff, 0x0450, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0458_93_test_ff, 0x0458, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0460_93_test_ff, 0x0460, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0468_93_test_ff, 0x0468, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0470_93_test_ff, 0x0470, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0478_93_test_ff, 0x0478, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0479_93_test_ff, 0x0479, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0480_93_test_ff, 0x0480, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0490_93_test_ff, 0x0490, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0498_93_test_ff, 0x0498, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a0_93_test_ff, 0x04a0, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a8_93_test_ff, 0x04a8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b0_93_test_ff, 0x04b0, 6, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_04b8_93_test_ff, 0x04b8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b9_93_test_ff, 0x04b9, 10, { 0, 0 }, 0 }, /* SUB */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04d0_93_test_ff, 0x04d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04e8_93_test_ff, 0x04e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f0_93_test_ff, 0x04f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f8_93_test_ff, 0x04f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f9_93_test_ff, 0x04f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fa_93_test_ff, 0x04fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fb_93_test_ff, 0x04fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0600_93_test_ff, 0x0600, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0610_93_test_ff, 0x0610, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0618_93_test_ff, 0x0618, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0620_93_test_ff, 0x0620, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0628_93_test_ff, 0x0628, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0630_93_test_ff, 0x0630, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0638_93_test_ff, 0x0638, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0639_93_test_ff, 0x0639, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0640_93_test_ff, 0x0640, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0650_93_test_ff, 0x0650, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0658_93_test_ff, 0x0658, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0660_93_test_ff, 0x0660, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0668_93_test_ff, 0x0668, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0670_93_test_ff, 0x0670, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0678_93_test_ff, 0x0678, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0679_93_test_ff, 0x0679, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0680_93_test_ff, 0x0680, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0690_93_test_ff, 0x0690, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0698_93_test_ff, 0x0698, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a0_93_test_ff, 0x06a0, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a8_93_test_ff, 0x06a8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b0_93_test_ff, 0x06b0, 6, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_06b8_93_test_ff, 0x06b8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b9_93_test_ff, 0x06b9, 10, { 0, 0 }, 0 }, /* ADD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c0_93_test_ff, 0x06c0, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c8_93_test_ff, 0x06c8, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06d0_93_test_ff, 0x06d0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06e8_93_test_ff, 0x06e8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f0_93_test_ff, 0x06f0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f8_93_test_ff, 0x06f8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f9_93_test_ff, 0x06f9, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fa_93_test_ff, 0x06fa, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fb_93_test_ff, 0x06fb, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
{ NULL, op_0800_93_test_ff, 0x0800, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0810_93_test_ff, 0x0810, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0818_93_test_ff, 0x0818, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0820_93_test_ff, 0x0820, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0828_93_test_ff, 0x0828, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0830_93_test_ff, 0x0830, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0838_93_test_ff, 0x0838, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0839_93_test_ff, 0x0839, 8, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083a_93_test_ff, 0x083a, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083b_93_test_ff, 0x083b, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0840_93_test_ff, 0x0840, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0850_93_test_ff, 0x0850, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0858_93_test_ff, 0x0858, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0860_93_test_ff, 0x0860, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0868_93_test_ff, 0x0868, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0870_93_test_ff, 0x0870, 4, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0878_93_test_ff, 0x0878, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0879_93_test_ff, 0x0879, 8, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0880_93_test_ff, 0x0880, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0890_93_test_ff, 0x0890, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0898_93_test_ff, 0x0898, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a0_93_test_ff, 0x08a0, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a8_93_test_ff, 0x08a8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b0_93_test_ff, 0x08b0, 4, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_08b8_93_test_ff, 0x08b8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b9_93_test_ff, 0x08b9, 8, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08c0_93_test_ff, 0x08c0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d0_93_test_ff, 0x08d0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d8_93_test_ff, 0x08d8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e0_93_test_ff, 0x08e0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e8_93_test_ff, 0x08e8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f0_93_test_ff, 0x08f0, 4, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_08f8_93_test_ff, 0x08f8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f9_93_test_ff, 0x08f9, 8, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0a00_93_test_ff, 0x0a00, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a10_93_test_ff, 0x0a10, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a18_93_test_ff, 0x0a18, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a20_93_test_ff, 0x0a20, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a28_93_test_ff, 0x0a28, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a30_93_test_ff, 0x0a30, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a38_93_test_ff, 0x0a38, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a39_93_test_ff, 0x0a39, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a3c_93_test_ff, 0x0a3c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a40_93_test_ff, 0x0a40, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a50_93_test_ff, 0x0a50, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a58_93_test_ff, 0x0a58, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a60_93_test_ff, 0x0a60, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a68_93_test_ff, 0x0a68, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a70_93_test_ff, 0x0a70, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a78_93_test_ff, 0x0a78, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a79_93_test_ff, 0x0a79, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a7c_93_test_ff, 0x0a7c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a80_93_test_ff, 0x0a80, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a90_93_test_ff, 0x0a90, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a98_93_test_ff, 0x0a98, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa0_93_test_ff, 0x0aa0, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa8_93_test_ff, 0x0aa8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab0_93_test_ff, 0x0ab0, 6, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0ab8_93_test_ff, 0x0ab8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab9_93_test_ff, 0x0ab9, 10, { 0, 0 }, 0 }, /* EOR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad0_93_test_ff, 0x0ad0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad8_93_test_ff, 0x0ad8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae0_93_test_ff, 0x0ae0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae8_93_test_ff, 0x0ae8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af0_93_test_ff, 0x0af0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af8_93_test_ff, 0x0af8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af9_93_test_ff, 0x0af9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
{ NULL, op_0c00_93_test_ff, 0x0c00, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c10_93_test_ff, 0x0c10, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c18_93_test_ff, 0x0c18, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c20_93_test_ff, 0x0c20, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c28_93_test_ff, 0x0c28, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c30_93_test_ff, 0x0c30, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c38_93_test_ff, 0x0c38, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c39_93_test_ff, 0x0c39, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3a_93_test_ff, 0x0c3a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3b_93_test_ff, 0x0c3b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c40_93_test_ff, 0x0c40, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c50_93_test_ff, 0x0c50, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c58_93_test_ff, 0x0c58, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c60_93_test_ff, 0x0c60, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c68_93_test_ff, 0x0c68, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c70_93_test_ff, 0x0c70, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c78_93_test_ff, 0x0c78, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c79_93_test_ff, 0x0c79, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7a_93_test_ff, 0x0c7a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7b_93_test_ff, 0x0c7b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c80_93_test_ff, 0x0c80, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c90_93_test_ff, 0x0c90, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c98_93_test_ff, 0x0c98, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca0_93_test_ff, 0x0ca0, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca8_93_test_ff, 0x0ca8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb0_93_test_ff, 0x0cb0, 6, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0cb8_93_test_ff, 0x0cb8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb9_93_test_ff, 0x0cb9, 10, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cba_93_test_ff, 0x0cba, 8, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cbb_93_test_ff, 0x0cbb, 6, { 2, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd0_93_test_ff, 0x0cd0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd8_93_test_ff, 0x0cd8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce0_93_test_ff, 0x0ce0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce8_93_test_ff, 0x0ce8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf0_93_test_ff, 0x0cf0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf8_93_test_ff, 0x0cf8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf9_93_test_ff, 0x0cf9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cfc_93_test_ff, 0x0cfc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e10_93_test_ff, 0x0e10, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e18_93_test_ff, 0x0e18, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e20_93_test_ff, 0x0e20, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e28_93_test_ff, 0x0e28, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e30_93_test_ff, 0x0e30, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e38_93_test_ff, 0x0e38, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e39_93_test_ff, 0x0e39, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e50_93_test_ff, 0x0e50, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e58_93_test_ff, 0x0e58, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e60_93_test_ff, 0x0e60, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e68_93_test_ff, 0x0e68, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e70_93_test_ff, 0x0e70, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e78_93_test_ff, 0x0e78, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e79_93_test_ff, 0x0e79, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e90_93_test_ff, 0x0e90, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e98_93_test_ff, 0x0e98, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea0_93_test_ff, 0x0ea0, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea8_93_test_ff, 0x0ea8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb0_93_test_ff, 0x0eb0, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb8_93_test_ff, 0x0eb8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb9_93_test_ff, 0x0eb9, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed0_93_test_ff, 0x0ed0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed8_93_test_ff, 0x0ed8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee0_93_test_ff, 0x0ee0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee8_93_test_ff, 0x0ee8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef0_93_test_ff, 0x0ef0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef8_93_test_ff, 0x0ef8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef9_93_test_ff, 0x0ef9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0efc_93_test_ff, 0x0efc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
{ NULL, op_1000_93_test_ff, 0x1000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1010_93_test_ff, 0x1010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1018_93_test_ff, 0x1018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1020_93_test_ff, 0x1020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1028_93_test_ff, 0x1028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1030_93_test_ff, 0x1030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1038_93_test_ff, 0x1038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1039_93_test_ff, 0x1039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103a_93_test_ff, 0x103a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103b_93_test_ff, 0x103b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_103c_93_test_ff, 0x103c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1080_93_test_ff, 0x1080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1090_93_test_ff, 0x1090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1098_93_test_ff, 0x1098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a0_93_test_ff, 0x10a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a8_93_test_ff, 0x10a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b0_93_test_ff, 0x10b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10b8_93_test_ff, 0x10b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b9_93_test_ff, 0x10b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10ba_93_test_ff, 0x10ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10bb_93_test_ff, 0x10bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10bc_93_test_ff, 0x10bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10c0_93_test_ff, 0x10c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d0_93_test_ff, 0x10d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d8_93_test_ff, 0x10d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e0_93_test_ff, 0x10e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e8_93_test_ff, 0x10e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f0_93_test_ff, 0x10f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10f8_93_test_ff, 0x10f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f9_93_test_ff, 0x10f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fa_93_test_ff, 0x10fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fb_93_test_ff, 0x10fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10fc_93_test_ff, 0x10fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1100_93_test_ff, 0x1100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1110_93_test_ff, 0x1110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1118_93_test_ff, 0x1118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1120_93_test_ff, 0x1120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1128_93_test_ff, 0x1128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1130_93_test_ff, 0x1130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1138_93_test_ff, 0x1138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1139_93_test_ff, 0x1139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113a_93_test_ff, 0x113a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113b_93_test_ff, 0x113b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_113c_93_test_ff, 0x113c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1140_93_test_ff, 0x1140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1150_93_test_ff, 0x1150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1158_93_test_ff, 0x1158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1160_93_test_ff, 0x1160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1168_93_test_ff, 0x1168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1170_93_test_ff, 0x1170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1178_93_test_ff, 0x1178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1179_93_test_ff, 0x1179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117a_93_test_ff, 0x117a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117b_93_test_ff, 0x117b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_117c_93_test_ff, 0x117c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1180_93_test_ff, 0x1180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1190_93_test_ff, 0x1190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1198_93_test_ff, 0x1198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a0_93_test_ff, 0x11a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a8_93_test_ff, 0x11a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b0_93_test_ff, 0x11b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11b8_93_test_ff, 0x11b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b9_93_test_ff, 0x11b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11ba_93_test_ff, 0x11ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11bb_93_test_ff, 0x11bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11bc_93_test_ff, 0x11bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11c0_93_test_ff, 0x11c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d0_93_test_ff, 0x11d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d8_93_test_ff, 0x11d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e0_93_test_ff, 0x11e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e8_93_test_ff, 0x11e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f0_93_test_ff, 0x11f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11f8_93_test_ff, 0x11f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f9_93_test_ff, 0x11f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fa_93_test_ff, 0x11fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fb_93_test_ff, 0x11fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11fc_93_test_ff, 0x11fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13c0_93_test_ff, 0x13c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d0_93_test_ff, 0x13d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d8_93_test_ff, 0x13d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e0_93_test_ff, 0x13e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e8_93_test_ff, 0x13e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f0_93_test_ff, 0x13f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13f8_93_test_ff, 0x13f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f9_93_test_ff, 0x13f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fa_93_test_ff, 0x13fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fb_93_test_ff, 0x13fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13fc_93_test_ff, 0x13fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2000_93_test_ff, 0x2000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2008_93_test_ff, 0x2008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2010_93_test_ff, 0x2010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2018_93_test_ff, 0x2018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2020_93_test_ff, 0x2020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2028_93_test_ff, 0x2028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2030_93_test_ff, 0x2030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2038_93_test_ff, 0x2038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2039_93_test_ff, 0x2039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203a_93_test_ff, 0x203a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203b_93_test_ff, 0x203b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_203c_93_test_ff, 0x203c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2040_93_test_ff, 0x2040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2048_93_test_ff, 0x2048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2050_93_test_ff, 0x2050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2058_93_test_ff, 0x2058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2060_93_test_ff, 0x2060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2068_93_test_ff, 0x2068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2070_93_test_ff, 0x2070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_2078_93_test_ff, 0x2078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2079_93_test_ff, 0x2079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207a_93_test_ff, 0x207a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207b_93_test_ff, 0x207b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_207c_93_test_ff, 0x207c, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2080_93_test_ff, 0x2080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2088_93_test_ff, 0x2088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2090_93_test_ff, 0x2090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2098_93_test_ff, 0x2098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a0_93_test_ff, 0x20a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a8_93_test_ff, 0x20a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b0_93_test_ff, 0x20b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20b8_93_test_ff, 0x20b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b9_93_test_ff, 0x20b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20ba_93_test_ff, 0x20ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20bb_93_test_ff, 0x20bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20bc_93_test_ff, 0x20bc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c0_93_test_ff, 0x20c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c8_93_test_ff, 0x20c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d0_93_test_ff, 0x20d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d8_93_test_ff, 0x20d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e0_93_test_ff, 0x20e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e8_93_test_ff, 0x20e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f0_93_test_ff, 0x20f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20f8_93_test_ff, 0x20f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f9_93_test_ff, 0x20f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fa_93_test_ff, 0x20fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fb_93_test_ff, 0x20fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20fc_93_test_ff, 0x20fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2100_93_test_ff, 0x2100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2108_93_test_ff, 0x2108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2110_93_test_ff, 0x2110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2118_93_test_ff, 0x2118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2120_93_test_ff, 0x2120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2128_93_test_ff, 0x2128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2130_93_test_ff, 0x2130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2138_93_test_ff, 0x2138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2139_93_test_ff, 0x2139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213a_93_test_ff, 0x213a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213b_93_test_ff, 0x213b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_213c_93_test_ff, 0x213c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2140_93_test_ff, 0x2140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2148_93_test_ff, 0x2148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2150_93_test_ff, 0x2150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2158_93_test_ff, 0x2158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2160_93_test_ff, 0x2160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2168_93_test_ff, 0x2168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2170_93_test_ff, 0x2170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2178_93_test_ff, 0x2178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2179_93_test_ff, 0x2179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217a_93_test_ff, 0x217a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217b_93_test_ff, 0x217b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_217c_93_test_ff, 0x217c, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2180_93_test_ff, 0x2180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2188_93_test_ff, 0x2188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2190_93_test_ff, 0x2190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2198_93_test_ff, 0x2198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a0_93_test_ff, 0x21a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a8_93_test_ff, 0x21a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b0_93_test_ff, 0x21b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21b8_93_test_ff, 0x21b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b9_93_test_ff, 0x21b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21ba_93_test_ff, 0x21ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21bb_93_test_ff, 0x21bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21bc_93_test_ff, 0x21bc, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21c0_93_test_ff, 0x21c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21c8_93_test_ff, 0x21c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d0_93_test_ff, 0x21d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d8_93_test_ff, 0x21d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e0_93_test_ff, 0x21e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e8_93_test_ff, 0x21e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f0_93_test_ff, 0x21f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21f8_93_test_ff, 0x21f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f9_93_test_ff, 0x21f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fa_93_test_ff, 0x21fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fb_93_test_ff, 0x21fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21fc_93_test_ff, 0x21fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c0_93_test_ff, 0x23c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c8_93_test_ff, 0x23c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d0_93_test_ff, 0x23d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d8_93_test_ff, 0x23d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e0_93_test_ff, 0x23e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e8_93_test_ff, 0x23e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f0_93_test_ff, 0x23f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23f8_93_test_ff, 0x23f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f9_93_test_ff, 0x23f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fa_93_test_ff, 0x23fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fb_93_test_ff, 0x23fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23fc_93_test_ff, 0x23fc, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3000_93_test_ff, 0x3000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3008_93_test_ff, 0x3008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3010_93_test_ff, 0x3010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3018_93_test_ff, 0x3018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3020_93_test_ff, 0x3020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3028_93_test_ff, 0x3028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3030_93_test_ff, 0x3030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3038_93_test_ff, 0x3038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3039_93_test_ff, 0x3039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303a_93_test_ff, 0x303a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303b_93_test_ff, 0x303b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_303c_93_test_ff, 0x303c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3040_93_test_ff, 0x3040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3048_93_test_ff, 0x3048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3050_93_test_ff, 0x3050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3058_93_test_ff, 0x3058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3060_93_test_ff, 0x3060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3068_93_test_ff, 0x3068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3070_93_test_ff, 0x3070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_3078_93_test_ff, 0x3078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3079_93_test_ff, 0x3079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307a_93_test_ff, 0x307a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307b_93_test_ff, 0x307b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_307c_93_test_ff, 0x307c, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3080_93_test_ff, 0x3080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3088_93_test_ff, 0x3088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3090_93_test_ff, 0x3090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3098_93_test_ff, 0x3098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a0_93_test_ff, 0x30a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a8_93_test_ff, 0x30a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b0_93_test_ff, 0x30b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30b8_93_test_ff, 0x30b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b9_93_test_ff, 0x30b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30ba_93_test_ff, 0x30ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30bb_93_test_ff, 0x30bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30bc_93_test_ff, 0x30bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c0_93_test_ff, 0x30c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c8_93_test_ff, 0x30c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d0_93_test_ff, 0x30d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d8_93_test_ff, 0x30d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e0_93_test_ff, 0x30e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e8_93_test_ff, 0x30e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f0_93_test_ff, 0x30f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30f8_93_test_ff, 0x30f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f9_93_test_ff, 0x30f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fa_93_test_ff, 0x30fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fb_93_test_ff, 0x30fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30fc_93_test_ff, 0x30fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3100_93_test_ff, 0x3100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3108_93_test_ff, 0x3108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3110_93_test_ff, 0x3110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3118_93_test_ff, 0x3118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3120_93_test_ff, 0x3120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3128_93_test_ff, 0x3128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3130_93_test_ff, 0x3130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3138_93_test_ff, 0x3138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3139_93_test_ff, 0x3139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313a_93_test_ff, 0x313a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313b_93_test_ff, 0x313b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_313c_93_test_ff, 0x313c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3140_93_test_ff, 0x3140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3148_93_test_ff, 0x3148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3150_93_test_ff, 0x3150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3158_93_test_ff, 0x3158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3160_93_test_ff, 0x3160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3168_93_test_ff, 0x3168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3170_93_test_ff, 0x3170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3178_93_test_ff, 0x3178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3179_93_test_ff, 0x3179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317a_93_test_ff, 0x317a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317b_93_test_ff, 0x317b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_317c_93_test_ff, 0x317c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3180_93_test_ff, 0x3180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3188_93_test_ff, 0x3188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3190_93_test_ff, 0x3190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3198_93_test_ff, 0x3198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a0_93_test_ff, 0x31a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a8_93_test_ff, 0x31a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b0_93_test_ff, 0x31b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31b8_93_test_ff, 0x31b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b9_93_test_ff, 0x31b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31ba_93_test_ff, 0x31ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31bb_93_test_ff, 0x31bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31bc_93_test_ff, 0x31bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31c0_93_test_ff, 0x31c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31c8_93_test_ff, 0x31c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d0_93_test_ff, 0x31d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d8_93_test_ff, 0x31d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e0_93_test_ff, 0x31e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e8_93_test_ff, 0x31e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f0_93_test_ff, 0x31f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31f8_93_test_ff, 0x31f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f9_93_test_ff, 0x31f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fa_93_test_ff, 0x31fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fb_93_test_ff, 0x31fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31fc_93_test_ff, 0x31fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c0_93_test_ff, 0x33c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c8_93_test_ff, 0x33c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d0_93_test_ff, 0x33d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d8_93_test_ff, 0x33d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e0_93_test_ff, 0x33e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e8_93_test_ff, 0x33e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f0_93_test_ff, 0x33f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33f8_93_test_ff, 0x33f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f9_93_test_ff, 0x33f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fa_93_test_ff, 0x33fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fb_93_test_ff, 0x33fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33fc_93_test_ff, 0x33fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_4000_93_test_ff, 0x4000, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4010_93_test_ff, 0x4010, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4018_93_test_ff, 0x4018, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4020_93_test_ff, 0x4020, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4028_93_test_ff, 0x4028, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4030_93_test_ff, 0x4030, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4038_93_test_ff, 0x4038, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4039_93_test_ff, 0x4039, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4040_93_test_ff, 0x4040, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4050_93_test_ff, 0x4050, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4058_93_test_ff, 0x4058, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4060_93_test_ff, 0x4060, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4068_93_test_ff, 0x4068, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4070_93_test_ff, 0x4070, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4078_93_test_ff, 0x4078, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4079_93_test_ff, 0x4079, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4080_93_test_ff, 0x4080, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4090_93_test_ff, 0x4090, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4098_93_test_ff, 0x4098, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a0_93_test_ff, 0x40a0, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a8_93_test_ff, 0x40a8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b0_93_test_ff, 0x40b0, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_40b8_93_test_ff, 0x40b8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b9_93_test_ff, 0x40b9, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40c0_93_test_ff, 0x40c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d0_93_test_ff, 0x40d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d8_93_test_ff, 0x40d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e0_93_test_ff, 0x40e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e8_93_test_ff, 0x40e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f0_93_test_ff, 0x40f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f8_93_test_ff, 0x40f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f9_93_test_ff, 0x40f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4100_93_test_ff, 0x4100, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4110_93_test_ff, 0x4110, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4118_93_test_ff, 0x4118, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4120_93_test_ff, 0x4120, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4128_93_test_ff, 0x4128, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4130_93_test_ff, 0x4130, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4138_93_test_ff, 0x4138, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4139_93_test_ff, 0x4139, 6, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413a_93_test_ff, 0x413a, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413b_93_test_ff, 0x413b, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413c_93_test_ff, 0x413c, 6, { 0, 0 }, 0 }, /* CHK */
#endif
{ NULL, op_4180_93_test_ff, 0x4180, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4190_93_test_ff, 0x4190, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4198_93_test_ff, 0x4198, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a0_93_test_ff, 0x41a0, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a8_93_test_ff, 0x41a8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b0_93_test_ff, 0x41b0, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41b8_93_test_ff, 0x41b8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b9_93_test_ff, 0x41b9, 6, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41ba_93_test_ff, 0x41ba, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41bb_93_test_ff, 0x41bb, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41bc_93_test_ff, 0x41bc, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41d0_93_test_ff, 0x41d0, 2, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41e8_93_test_ff, 0x41e8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f0_93_test_ff, 0x41f0, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_41f8_93_test_ff, 0x41f8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f9_93_test_ff, 0x41f9, 6, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fa_93_test_ff, 0x41fa, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fb_93_test_ff, 0x41fb, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_4200_93_test_ff, 0x4200, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4210_93_test_ff, 0x4210, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4218_93_test_ff, 0x4218, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4220_93_test_ff, 0x4220, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4228_93_test_ff, 0x4228, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4230_93_test_ff, 0x4230, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4238_93_test_ff, 0x4238, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4239_93_test_ff, 0x4239, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4240_93_test_ff, 0x4240, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4250_93_test_ff, 0x4250, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4258_93_test_ff, 0x4258, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4260_93_test_ff, 0x4260, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4268_93_test_ff, 0x4268, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4270_93_test_ff, 0x4270, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4278_93_test_ff, 0x4278, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4279_93_test_ff, 0x4279, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4280_93_test_ff, 0x4280, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4290_93_test_ff, 0x4290, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4298_93_test_ff, 0x4298, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a0_93_test_ff, 0x42a0, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a8_93_test_ff, 0x42a8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b0_93_test_ff, 0x42b0, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_42b8_93_test_ff, 0x42b8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b9_93_test_ff, 0x42b9, 6, { 0, 0 }, 0 }, /* CLR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42c0_93_test_ff, 0x42c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d0_93_test_ff, 0x42d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d8_93_test_ff, 0x42d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e0_93_test_ff, 0x42e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e8_93_test_ff, 0x42e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f0_93_test_ff, 0x42f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f8_93_test_ff, 0x42f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f9_93_test_ff, 0x42f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#endif
{ NULL, op_4400_93_test_ff, 0x4400, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4410_93_test_ff, 0x4410, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4418_93_test_ff, 0x4418, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4420_93_test_ff, 0x4420, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4428_93_test_ff, 0x4428, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4430_93_test_ff, 0x4430, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4438_93_test_ff, 0x4438, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4439_93_test_ff, 0x4439, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4440_93_test_ff, 0x4440, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4450_93_test_ff, 0x4450, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4458_93_test_ff, 0x4458, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4460_93_test_ff, 0x4460, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4468_93_test_ff, 0x4468, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4470_93_test_ff, 0x4470, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4478_93_test_ff, 0x4478, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4479_93_test_ff, 0x4479, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4480_93_test_ff, 0x4480, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4490_93_test_ff, 0x4490, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4498_93_test_ff, 0x4498, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a0_93_test_ff, 0x44a0, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a8_93_test_ff, 0x44a8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b0_93_test_ff, 0x44b0, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_44b8_93_test_ff, 0x44b8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b9_93_test_ff, 0x44b9, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44c0_93_test_ff, 0x44c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d0_93_test_ff, 0x44d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d8_93_test_ff, 0x44d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e0_93_test_ff, 0x44e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e8_93_test_ff, 0x44e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f0_93_test_ff, 0x44f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f8_93_test_ff, 0x44f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f9_93_test_ff, 0x44f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fa_93_test_ff, 0x44fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fb_93_test_ff, 0x44fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fc_93_test_ff, 0x44fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4600_93_test_ff, 0x4600, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4610_93_test_ff, 0x4610, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4618_93_test_ff, 0x4618, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4620_93_test_ff, 0x4620, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4628_93_test_ff, 0x4628, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4630_93_test_ff, 0x4630, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4638_93_test_ff, 0x4638, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4639_93_test_ff, 0x4639, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4640_93_test_ff, 0x4640, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4650_93_test_ff, 0x4650, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4658_93_test_ff, 0x4658, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4660_93_test_ff, 0x4660, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4668_93_test_ff, 0x4668, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4670_93_test_ff, 0x4670, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4678_93_test_ff, 0x4678, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4679_93_test_ff, 0x4679, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4680_93_test_ff, 0x4680, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4690_93_test_ff, 0x4690, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4698_93_test_ff, 0x4698, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a0_93_test_ff, 0x46a0, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a8_93_test_ff, 0x46a8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b0_93_test_ff, 0x46b0, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_46b8_93_test_ff, 0x46b8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b9_93_test_ff, 0x46b9, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46c0_93_test_ff, 0x46c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d0_93_test_ff, 0x46d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d8_93_test_ff, 0x46d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e0_93_test_ff, 0x46e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e8_93_test_ff, 0x46e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f0_93_test_ff, 0x46f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f8_93_test_ff, 0x46f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f9_93_test_ff, 0x46f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fa_93_test_ff, 0x46fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fb_93_test_ff, 0x46fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fc_93_test_ff, 0x46fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4800_93_test_ff, 0x4800, 2, { 0, 0 }, 0 }, /* NBCD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4808_93_test_ff, 0x4808, 6, { 0, 0 }, 0 }, /* LINK */
#endif
{ NULL, op_4810_93_test_ff, 0x4810, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4818_93_test_ff, 0x4818, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4820_93_test_ff, 0x4820, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4828_93_test_ff, 0x4828, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4830_93_test_ff, 0x4830, 2, { 2, 0 }, 0 }, /* NBCD */
{ NULL, op_4838_93_test_ff, 0x4838, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4839_93_test_ff, 0x4839, 6, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4840_93_test_ff, 0x4840, 2, { 0, 0 }, 0 }, /* SWAP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4848_93_test_ff, 0x4848, 2, { 0, 0 }, 0 }, /* BKPT */
#endif
{ NULL, op_4850_93_test_ff, 0x4850, 2, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4868_93_test_ff, 0x4868, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4870_93_test_ff, 0x4870, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4878_93_test_ff, 0x4878, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4879_93_test_ff, 0x4879, 6, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487a_93_test_ff, 0x487a, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487b_93_test_ff, 0x487b, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4880_93_test_ff, 0x4880, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_4890_93_test_ff, 0x4890, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a0_93_test_ff, 0x48a0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a8_93_test_ff, 0x48a8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b0_93_test_ff, 0x48b0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b8_93_test_ff, 0x48b8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b9_93_test_ff, 0x48b9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48c0_93_test_ff, 0x48c0, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_48d0_93_test_ff, 0x48d0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e0_93_test_ff, 0x48e0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e8_93_test_ff, 0x48e8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f0_93_test_ff, 0x48f0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f8_93_test_ff, 0x48f8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f9_93_test_ff, 0x48f9, 8, { 0, 0 }, 0 }, /* MVMLE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_49c0_93_test_ff, 0x49c0, 2, { 0, 0 }, 0 }, /* EXT */
#endif
{ NULL, op_4a00_93_test_ff, 0x4a00, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a10_93_test_ff, 0x4a10, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a18_93_test_ff, 0x4a18, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a20_93_test_ff, 0x4a20, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a28_93_test_ff, 0x4a28, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a30_93_test_ff, 0x4a30, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a38_93_test_ff, 0x4a38, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a39_93_test_ff, 0x4a39, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3a_93_test_ff, 0x4a3a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3b_93_test_ff, 0x4a3b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3c_93_test_ff, 0x4a3c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a40_93_test_ff, 0x4a40, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a48_93_test_ff, 0x4a48, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a50_93_test_ff, 0x4a50, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a58_93_test_ff, 0x4a58, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a60_93_test_ff, 0x4a60, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a68_93_test_ff, 0x4a68, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a70_93_test_ff, 0x4a70, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a78_93_test_ff, 0x4a78, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a79_93_test_ff, 0x4a79, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7a_93_test_ff, 0x4a7a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7b_93_test_ff, 0x4a7b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7c_93_test_ff, 0x4a7c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a80_93_test_ff, 0x4a80, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a88_93_test_ff, 0x4a88, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a90_93_test_ff, 0x4a90, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a98_93_test_ff, 0x4a98, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa0_93_test_ff, 0x4aa0, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa8_93_test_ff, 0x4aa8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab0_93_test_ff, 0x4ab0, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4ab8_93_test_ff, 0x4ab8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab9_93_test_ff, 0x4ab9, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4aba_93_test_ff, 0x4aba, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abb_93_test_ff, 0x4abb, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abc_93_test_ff, 0x4abc, 6, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4ac0_93_test_ff, 0x4ac0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad0_93_test_ff, 0x4ad0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad8_93_test_ff, 0x4ad8, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae0_93_test_ff, 0x4ae0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae8_93_test_ff, 0x4ae8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af0_93_test_ff, 0x4af0, 2, { 2, 0 }, 0 }, /* TAS */
{ NULL, op_4af8_93_test_ff, 0x4af8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af9_93_test_ff, 0x4af9, 6, { 0, 0 }, 0 }, /* TAS */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c00_93_test_ff, 0x4c00, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c10_93_test_ff, 0x4c10, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c18_93_test_ff, 0x4c18, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c20_93_test_ff, 0x4c20, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c28_93_test_ff, 0x4c28, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c30_93_test_ff, 0x4c30, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c38_93_test_ff, 0x4c38, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c39_93_test_ff, 0x4c39, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3a_93_test_ff, 0x4c3a, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3b_93_test_ff, 0x4c3b, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3c_93_test_ff, 0x4c3c, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c40_93_test_ff, 0x4c40, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c50_93_test_ff, 0x4c50, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c58_93_test_ff, 0x4c58, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c60_93_test_ff, 0x4c60, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c68_93_test_ff, 0x4c68, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c70_93_test_ff, 0x4c70, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c78_93_test_ff, 0x4c78, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c79_93_test_ff, 0x4c79, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7a_93_test_ff, 0x4c7a, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7b_93_test_ff, 0x4c7b, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7c_93_test_ff, 0x4c7c, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
{ NULL, op_4c90_93_test_ff, 0x4c90, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4c98_93_test_ff, 0x4c98, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ca8_93_test_ff, 0x4ca8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb0_93_test_ff, 0x4cb0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb8_93_test_ff, 0x4cb8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb9_93_test_ff, 0x4cb9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cba_93_test_ff, 0x4cba, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cbb_93_test_ff, 0x4cbb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd0_93_test_ff, 0x4cd0, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd8_93_test_ff, 0x4cd8, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ce8_93_test_ff, 0x4ce8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf0_93_test_ff, 0x4cf0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf8_93_test_ff, 0x4cf8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf9_93_test_ff, 0x4cf9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfa_93_test_ff, 0x4cfa, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfb_93_test_ff, 0x4cfb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4e40_93_test_ff, 0x4e40, 2, { 0, 0 }, 0 }, /* TRAP */
{ NULL, op_4e50_93_test_ff, 0x4e50, 4, { 0, 0 }, 0 }, /* LINK */
{ NULL, op_4e58_93_test_ff, 0x4e58, 2, { 0, 0 }, 0 }, /* UNLK */
{ NULL, op_4e60_93_test_ff, 0x4e60, 2, { 0, 0 }, 0 }, /* MVR2USP */
{ NULL, op_4e68_93_test_ff, 0x4e68, 2, { 0, 0 }, 0 }, /* MVUSP2R */
{ NULL, op_4e70_93_test_ff, 0x4e70, 2, { 0, 0 }, 0 }, /* RESET */
{ NULL, op_4e71_93_test_ff, 0x4e71, 2, { 0, 0 }, 0 }, /* NOP */
{ NULL, op_4e72_93_test_ff, 0x4e72, 0, { 0, 0 }, 0 }, /* STOP */
{ NULL, op_4e73_93_test_ff, 0x4e73, 2, { 0, 0 }, 1 }, /* RTE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e74_93_test_ff, 0x4e74, 4, { 0, 0 }, 2 }, /* RTD */
#endif
{ NULL, op_4e75_93_test_ff, 0x4e75, 2, { 0, 0 }, 1 }, /* RTS */
{ NULL, op_4e76_93_test_ff, 0x4e76, 2, { 0, 0 }, 0 }, /* TRAPV */
{ NULL, op_4e77_93_test_ff, 0x4e77, 2, { 0, 0 }, 1 }, /* RTR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7a_93_test_ff, 0x4e7a, 4, { 0, 0 }, 0 }, /* MOVEC2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7b_93_test_ff, 0x4e7b, 4, { 0, 0 }, 0 }, /* MOVE2C */
#endif
{ NULL, op_4e90_93_test_ff, 0x4e90, 2, { 0, 0 }, 1 }, /* JSR */
{ NULL, op_4ea8_93_test_ff, 0x4ea8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb0_93_test_ff, 0x4eb0, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4eb8_93_test_ff, 0x4eb8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb9_93_test_ff, 0x4eb9, 6, { 0, 0 }, 3 }, /* JSR */
{ NULL, op_4eba_93_test_ff, 0x4eba, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4ebb_93_test_ff, 0x4ebb, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4ed0_93_test_ff, 0x4ed0, 2, { 0, 0 }, 1 }, /* JMP */
{ NULL, op_4ee8_93_test_ff, 0x4ee8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef0_93_test_ff, 0x4ef0, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_4ef8_93_test_ff, 0x4ef8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef9_93_test_ff, 0x4ef9, 6, { 0, 0 }, 3 }, /* JMP */
{ NULL, op_4efa_93_test_ff, 0x4efa, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4efb_93_test_ff, 0x4efb, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_5000_93_test_ff, 0x5000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5010_93_test_ff, 0x5010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5018_93_test_ff, 0x5018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5020_93_test_ff, 0x5020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5028_93_test_ff, 0x5028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5030_93_test_ff, 0x5030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5038_93_test_ff, 0x5038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5039_93_test_ff, 0x5039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5040_93_test_ff, 0x5040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5048_93_test_ff, 0x5048, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5050_93_test_ff, 0x5050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5058_93_test_ff, 0x5058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5060_93_test_ff, 0x5060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5068_93_test_ff, 0x5068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5070_93_test_ff, 0x5070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5078_93_test_ff, 0x5078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5079_93_test_ff, 0x5079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5080_93_test_ff, 0x5080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5088_93_test_ff, 0x5088, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5090_93_test_ff, 0x5090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5098_93_test_ff, 0x5098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a0_93_test_ff, 0x50a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a8_93_test_ff, 0x50a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b0_93_test_ff, 0x50b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_50b8_93_test_ff, 0x50b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b9_93_test_ff, 0x50b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50c0_93_test_ff, 0x50c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50c8_93_test_ff, 0x50c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_50d0_93_test_ff, 0x50d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50d8_93_test_ff, 0x50d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e0_93_test_ff, 0x50e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e8_93_test_ff, 0x50e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f0_93_test_ff, 0x50f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_50f8_93_test_ff, 0x50f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f9_93_test_ff, 0x50f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fa_93_test_ff, 0x50fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fb_93_test_ff, 0x50fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fc_93_test_ff, 0x50fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5100_93_test_ff, 0x5100, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5110_93_test_ff, 0x5110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5118_93_test_ff, 0x5118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5120_93_test_ff, 0x5120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5128_93_test_ff, 0x5128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5130_93_test_ff, 0x5130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5138_93_test_ff, 0x5138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5139_93_test_ff, 0x5139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5140_93_test_ff, 0x5140, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5148_93_test_ff, 0x5148, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5150_93_test_ff, 0x5150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5158_93_test_ff, 0x5158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5160_93_test_ff, 0x5160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5168_93_test_ff, 0x5168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5170_93_test_ff, 0x5170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5178_93_test_ff, 0x5178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5179_93_test_ff, 0x5179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5180_93_test_ff, 0x5180, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5188_93_test_ff, 0x5188, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5190_93_test_ff, 0x5190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5198_93_test_ff, 0x5198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a0_93_test_ff, 0x51a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a8_93_test_ff, 0x51a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b0_93_test_ff, 0x51b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_51b8_93_test_ff, 0x51b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b9_93_test_ff, 0x51b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51c0_93_test_ff, 0x51c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51c8_93_test_ff, 0x51c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_51d0_93_test_ff, 0x51d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51d8_93_test_ff, 0x51d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e0_93_test_ff, 0x51e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e8_93_test_ff, 0x51e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f0_93_test_ff, 0x51f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_51f8_93_test_ff, 0x51f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f9_93_test_ff, 0x51f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fa_93_test_ff, 0x51fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fb_93_test_ff, 0x51fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fc_93_test_ff, 0x51fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_52c0_93_test_ff, 0x52c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c8_93_test_ff, 0x52c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_52d0_93_test_ff, 0x52d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52d8_93_test_ff, 0x52d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e0_93_test_ff, 0x52e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e8_93_test_ff, 0x52e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f0_93_test_ff, 0x52f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_52f8_93_test_ff, 0x52f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f9_93_test_ff, 0x52f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fa_93_test_ff, 0x52fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fb_93_test_ff, 0x52fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fc_93_test_ff, 0x52fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_53c0_93_test_ff, 0x53c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c8_93_test_ff, 0x53c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_53d0_93_test_ff, 0x53d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53d8_93_test_ff, 0x53d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e0_93_test_ff, 0x53e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e8_93_test_ff, 0x53e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f0_93_test_ff, 0x53f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_53f8_93_test_ff, 0x53f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f9_93_test_ff, 0x53f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fa_93_test_ff, 0x53fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fb_93_test_ff, 0x53fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fc_93_test_ff, 0x53fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_54c0_93_test_ff, 0x54c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c8_93_test_ff, 0x54c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_54d0_93_test_ff, 0x54d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54d8_93_test_ff, 0x54d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e0_93_test_ff, 0x54e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e8_93_test_ff, 0x54e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f0_93_test_ff, 0x54f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_54f8_93_test_ff, 0x54f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f9_93_test_ff, 0x54f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fa_93_test_ff, 0x54fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fb_93_test_ff, 0x54fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fc_93_test_ff, 0x54fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_55c0_93_test_ff, 0x55c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c8_93_test_ff, 0x55c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_55d0_93_test_ff, 0x55d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55d8_93_test_ff, 0x55d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e0_93_test_ff, 0x55e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e8_93_test_ff, 0x55e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f0_93_test_ff, 0x55f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_55f8_93_test_ff, 0x55f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f9_93_test_ff, 0x55f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fa_93_test_ff, 0x55fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fb_93_test_ff, 0x55fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fc_93_test_ff, 0x55fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_56c0_93_test_ff, 0x56c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c8_93_test_ff, 0x56c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_56d0_93_test_ff, 0x56d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56d8_93_test_ff, 0x56d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e0_93_test_ff, 0x56e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e8_93_test_ff, 0x56e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f0_93_test_ff, 0x56f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_56f8_93_test_ff, 0x56f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f9_93_test_ff, 0x56f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fa_93_test_ff, 0x56fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fb_93_test_ff, 0x56fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fc_93_test_ff, 0x56fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_57c0_93_test_ff, 0x57c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c8_93_test_ff, 0x57c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_57d0_93_test_ff, 0x57d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57d8_93_test_ff, 0x57d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e0_93_test_ff, 0x57e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e8_93_test_ff, 0x57e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f0_93_test_ff, 0x57f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_57f8_93_test_ff, 0x57f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f9_93_test_ff, 0x57f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fa_93_test_ff, 0x57fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fb_93_test_ff, 0x57fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fc_93_test_ff, 0x57fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_58c0_93_test_ff, 0x58c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c8_93_test_ff, 0x58c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_58d0_93_test_ff, 0x58d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58d8_93_test_ff, 0x58d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e0_93_test_ff, 0x58e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e8_93_test_ff, 0x58e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f0_93_test_ff, 0x58f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_58f8_93_test_ff, 0x58f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f9_93_test_ff, 0x58f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fa_93_test_ff, 0x58fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fb_93_test_ff, 0x58fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fc_93_test_ff, 0x58fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_59c0_93_test_ff, 0x59c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c8_93_test_ff, 0x59c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_59d0_93_test_ff, 0x59d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59d8_93_test_ff, 0x59d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e0_93_test_ff, 0x59e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e8_93_test_ff, 0x59e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f0_93_test_ff, 0x59f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_59f8_93_test_ff, 0x59f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f9_93_test_ff, 0x59f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fa_93_test_ff, 0x59fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fb_93_test_ff, 0x59fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fc_93_test_ff, 0x59fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ac0_93_test_ff, 0x5ac0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac8_93_test_ff, 0x5ac8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ad0_93_test_ff, 0x5ad0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ad8_93_test_ff, 0x5ad8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae0_93_test_ff, 0x5ae0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae8_93_test_ff, 0x5ae8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af0_93_test_ff, 0x5af0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5af8_93_test_ff, 0x5af8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af9_93_test_ff, 0x5af9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afa_93_test_ff, 0x5afa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afb_93_test_ff, 0x5afb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afc_93_test_ff, 0x5afc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5bc0_93_test_ff, 0x5bc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc8_93_test_ff, 0x5bc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5bd0_93_test_ff, 0x5bd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bd8_93_test_ff, 0x5bd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be0_93_test_ff, 0x5be0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be8_93_test_ff, 0x5be8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf0_93_test_ff, 0x5bf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5bf8_93_test_ff, 0x5bf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf9_93_test_ff, 0x5bf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfa_93_test_ff, 0x5bfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfb_93_test_ff, 0x5bfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfc_93_test_ff, 0x5bfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5cc0_93_test_ff, 0x5cc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc8_93_test_ff, 0x5cc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5cd0_93_test_ff, 0x5cd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cd8_93_test_ff, 0x5cd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce0_93_test_ff, 0x5ce0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce8_93_test_ff, 0x5ce8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf0_93_test_ff, 0x5cf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5cf8_93_test_ff, 0x5cf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf9_93_test_ff, 0x5cf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfa_93_test_ff, 0x5cfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfb_93_test_ff, 0x5cfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfc_93_test_ff, 0x5cfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5dc0_93_test_ff, 0x5dc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc8_93_test_ff, 0x5dc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5dd0_93_test_ff, 0x5dd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dd8_93_test_ff, 0x5dd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de0_93_test_ff, 0x5de0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de8_93_test_ff, 0x5de8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df0_93_test_ff, 0x5df0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5df8_93_test_ff, 0x5df8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df9_93_test_ff, 0x5df9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfa_93_test_ff, 0x5dfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfb_93_test_ff, 0x5dfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfc_93_test_ff, 0x5dfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ec0_93_test_ff, 0x5ec0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec8_93_test_ff, 0x5ec8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ed0_93_test_ff, 0x5ed0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ed8_93_test_ff, 0x5ed8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee0_93_test_ff, 0x5ee0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee8_93_test_ff, 0x5ee8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef0_93_test_ff, 0x5ef0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ef8_93_test_ff, 0x5ef8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef9_93_test_ff, 0x5ef9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efa_93_test_ff, 0x5efa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efb_93_test_ff, 0x5efb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efc_93_test_ff, 0x5efc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5fc0_93_test_ff, 0x5fc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc8_93_test_ff, 0x5fc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5fd0_93_test_ff, 0x5fd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fd8_93_test_ff, 0x5fd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe0_93_test_ff, 0x5fe0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe8_93_test_ff, 0x5fe8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff0_93_test_ff, 0x5ff0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ff8_93_test_ff, 0x5ff8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff9_93_test_ff, 0x5ff9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffa_93_test_ff, 0x5ffa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffb_93_test_ff, 0x5ffb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffc_93_test_ff, 0x5ffc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_6000_93_test_ff, 0x6000, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6001_93_test_ff, 0x6001, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_60ff_93_test_ff, 0x60ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6100_93_test_ff, 0x6100, 4, { 0, 0 }, 2 }, /* BSR */
{ NULL, op_6101_93_test_ff, 0x6101, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_61ff_93_test_ff, 0x61ff, 6, { 0, 0 }, 3 }, /* BSR */
{ NULL, op_6200_93_test_ff, 0x6200, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6201_93_test_ff, 0x6201, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_62ff_93_test_ff, 0x62ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6300_93_test_ff, 0x6300, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6301_93_test_ff, 0x6301, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_63ff_93_test_ff, 0x63ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6400_93_test_ff, 0x6400, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6401_93_test_ff, 0x6401, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_64ff_93_test_ff, 0x64ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6500_93_test_ff, 0x6500, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6501_93_test_ff, 0x6501, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_65ff_93_test_ff, 0x65ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6600_93_test_ff, 0x6600, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6601_93_test_ff, 0x6601, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_66ff_93_test_ff, 0x66ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6700_93_test_ff, 0x6700, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6701_93_test_ff, 0x6701, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_67ff_93_test_ff, 0x67ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6800_93_test_ff, 0x6800, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6801_93_test_ff, 0x6801, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_68ff_93_test_ff, 0x68ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6900_93_test_ff, 0x6900, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6901_93_test_ff, 0x6901, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_69ff_93_test_ff, 0x69ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6a00_93_test_ff, 0x6a00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6a01_93_test_ff, 0x6a01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6aff_93_test_ff, 0x6aff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6b00_93_test_ff, 0x6b00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6b01_93_test_ff, 0x6b01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6bff_93_test_ff, 0x6bff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6c00_93_test_ff, 0x6c00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6c01_93_test_ff, 0x6c01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6cff_93_test_ff, 0x6cff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6d00_93_test_ff, 0x6d00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6d01_93_test_ff, 0x6d01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6dff_93_test_ff, 0x6dff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6e00_93_test_ff, 0x6e00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6e01_93_test_ff, 0x6e01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6eff_93_test_ff, 0x6eff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6f00_93_test_ff, 0x6f00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6f01_93_test_ff, 0x6f01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6fff_93_test_ff, 0x6fff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_7000_93_test_ff, 0x7000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_8000_93_test_ff, 0x8000, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8010_93_test_ff, 0x8010, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8018_93_test_ff, 0x8018, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8020_93_test_ff, 0x8020, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8028_93_test_ff, 0x8028, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8030_93_test_ff, 0x8030, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8038_93_test_ff, 0x8038, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8039_93_test_ff, 0x8039, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803a_93_test_ff, 0x803a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803b_93_test_ff, 0x803b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_803c_93_test_ff, 0x803c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8040_93_test_ff, 0x8040, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8050_93_test_ff, 0x8050, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8058_93_test_ff, 0x8058, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8060_93_test_ff, 0x8060, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8068_93_test_ff, 0x8068, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8070_93_test_ff, 0x8070, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8078_93_test_ff, 0x8078, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8079_93_test_ff, 0x8079, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807a_93_test_ff, 0x807a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807b_93_test_ff, 0x807b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_807c_93_test_ff, 0x807c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8080_93_test_ff, 0x8080, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8090_93_test_ff, 0x8090, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8098_93_test_ff, 0x8098, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a0_93_test_ff, 0x80a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a8_93_test_ff, 0x80a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b0_93_test_ff, 0x80b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80b8_93_test_ff, 0x80b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b9_93_test_ff, 0x80b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80ba_93_test_ff, 0x80ba, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80bb_93_test_ff, 0x80bb, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80bc_93_test_ff, 0x80bc, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80c0_93_test_ff, 0x80c0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d0_93_test_ff, 0x80d0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d8_93_test_ff, 0x80d8, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e0_93_test_ff, 0x80e0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e8_93_test_ff, 0x80e8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f0_93_test_ff, 0x80f0, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80f8_93_test_ff, 0x80f8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f9_93_test_ff, 0x80f9, 6, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fa_93_test_ff, 0x80fa, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fb_93_test_ff, 0x80fb, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80fc_93_test_ff, 0x80fc, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_8100_93_test_ff, 0x8100, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8108_93_test_ff, 0x8108, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8110_93_test_ff, 0x8110, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8118_93_test_ff, 0x8118, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8120_93_test_ff, 0x8120, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8128_93_test_ff, 0x8128, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8130_93_test_ff, 0x8130, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8138_93_test_ff, 0x8138, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8139_93_test_ff, 0x8139, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8140_93_test_ff, 0x8140, 4, { 0, 0 }, 0 }, /* PACK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8148_93_test_ff, 0x8148, 4, { 0, 0 }, 0 }, /* PACK */
#endif
{ NULL, op_8150_93_test_ff, 0x8150, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8158_93_test_ff, 0x8158, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8160_93_test_ff, 0x8160, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8168_93_test_ff, 0x8168, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8170_93_test_ff, 0x8170, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8178_93_test_ff, 0x8178, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8179_93_test_ff, 0x8179, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8180_93_test_ff, 0x8180, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8188_93_test_ff, 0x8188, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
{ NULL, op_8190_93_test_ff, 0x8190, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8198_93_test_ff, 0x8198, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a0_93_test_ff, 0x81a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a8_93_test_ff, 0x81a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b0_93_test_ff, 0x81b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_81b8_93_test_ff, 0x81b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b9_93_test_ff, 0x81b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81c0_93_test_ff, 0x81c0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d0_93_test_ff, 0x81d0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d8_93_test_ff, 0x81d8, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e0_93_test_ff, 0x81e0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e8_93_test_ff, 0x81e8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f0_93_test_ff, 0x81f0, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81f8_93_test_ff, 0x81f8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f9_93_test_ff, 0x81f9, 6, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fa_93_test_ff, 0x81fa, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fb_93_test_ff, 0x81fb, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81fc_93_test_ff, 0x81fc, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_9000_93_test_ff, 0x9000, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9010_93_test_ff, 0x9010, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9018_93_test_ff, 0x9018, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9020_93_test_ff, 0x9020, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9028_93_test_ff, 0x9028, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9030_93_test_ff, 0x9030, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9038_93_test_ff, 0x9038, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9039_93_test_ff, 0x9039, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903a_93_test_ff, 0x903a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903b_93_test_ff, 0x903b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_903c_93_test_ff, 0x903c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9040_93_test_ff, 0x9040, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9048_93_test_ff, 0x9048, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9050_93_test_ff, 0x9050, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9058_93_test_ff, 0x9058, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9060_93_test_ff, 0x9060, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9068_93_test_ff, 0x9068, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9070_93_test_ff, 0x9070, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9078_93_test_ff, 0x9078, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9079_93_test_ff, 0x9079, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907a_93_test_ff, 0x907a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907b_93_test_ff, 0x907b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_907c_93_test_ff, 0x907c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9080_93_test_ff, 0x9080, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9088_93_test_ff, 0x9088, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9090_93_test_ff, 0x9090, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9098_93_test_ff, 0x9098, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a0_93_test_ff, 0x90a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a8_93_test_ff, 0x90a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b0_93_test_ff, 0x90b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90b8_93_test_ff, 0x90b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b9_93_test_ff, 0x90b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90ba_93_test_ff, 0x90ba, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90bb_93_test_ff, 0x90bb, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90bc_93_test_ff, 0x90bc, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90c0_93_test_ff, 0x90c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90c8_93_test_ff, 0x90c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d0_93_test_ff, 0x90d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d8_93_test_ff, 0x90d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e0_93_test_ff, 0x90e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e8_93_test_ff, 0x90e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f0_93_test_ff, 0x90f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90f8_93_test_ff, 0x90f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f9_93_test_ff, 0x90f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fa_93_test_ff, 0x90fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fb_93_test_ff, 0x90fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90fc_93_test_ff, 0x90fc, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_9100_93_test_ff, 0x9100, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9108_93_test_ff, 0x9108, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9110_93_test_ff, 0x9110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9118_93_test_ff, 0x9118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9120_93_test_ff, 0x9120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9128_93_test_ff, 0x9128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9130_93_test_ff, 0x9130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9138_93_test_ff, 0x9138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9139_93_test_ff, 0x9139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9140_93_test_ff, 0x9140, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9148_93_test_ff, 0x9148, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9150_93_test_ff, 0x9150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9158_93_test_ff, 0x9158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9160_93_test_ff, 0x9160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9168_93_test_ff, 0x9168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9170_93_test_ff, 0x9170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9178_93_test_ff, 0x9178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9179_93_test_ff, 0x9179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9180_93_test_ff, 0x9180, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9188_93_test_ff, 0x9188, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9190_93_test_ff, 0x9190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9198_93_test_ff, 0x9198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a0_93_test_ff, 0x91a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a8_93_test_ff, 0x91a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b0_93_test_ff, 0x91b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_91b8_93_test_ff, 0x91b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b9_93_test_ff, 0x91b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91c0_93_test_ff, 0x91c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91c8_93_test_ff, 0x91c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d0_93_test_ff, 0x91d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d8_93_test_ff, 0x91d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e0_93_test_ff, 0x91e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e8_93_test_ff, 0x91e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f0_93_test_ff, 0x91f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91f8_93_test_ff, 0x91f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f9_93_test_ff, 0x91f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fa_93_test_ff, 0x91fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fb_93_test_ff, 0x91fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91fc_93_test_ff, 0x91fc, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_b000_93_test_ff, 0xb000, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b010_93_test_ff, 0xb010, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b018_93_test_ff, 0xb018, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b020_93_test_ff, 0xb020, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b028_93_test_ff, 0xb028, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b030_93_test_ff, 0xb030, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b038_93_test_ff, 0xb038, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b039_93_test_ff, 0xb039, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03a_93_test_ff, 0xb03a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03b_93_test_ff, 0xb03b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b03c_93_test_ff, 0xb03c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b040_93_test_ff, 0xb040, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b048_93_test_ff, 0xb048, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b050_93_test_ff, 0xb050, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b058_93_test_ff, 0xb058, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b060_93_test_ff, 0xb060, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b068_93_test_ff, 0xb068, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b070_93_test_ff, 0xb070, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b078_93_test_ff, 0xb078, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b079_93_test_ff, 0xb079, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07a_93_test_ff, 0xb07a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07b_93_test_ff, 0xb07b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b07c_93_test_ff, 0xb07c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b080_93_test_ff, 0xb080, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b088_93_test_ff, 0xb088, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b090_93_test_ff, 0xb090, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b098_93_test_ff, 0xb098, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a0_93_test_ff, 0xb0a0, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a8_93_test_ff, 0xb0a8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b0_93_test_ff, 0xb0b0, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0b8_93_test_ff, 0xb0b8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b9_93_test_ff, 0xb0b9, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0ba_93_test_ff, 0xb0ba, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0bb_93_test_ff, 0xb0bb, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0bc_93_test_ff, 0xb0bc, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0c0_93_test_ff, 0xb0c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0c8_93_test_ff, 0xb0c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d0_93_test_ff, 0xb0d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d8_93_test_ff, 0xb0d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e0_93_test_ff, 0xb0e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e8_93_test_ff, 0xb0e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f0_93_test_ff, 0xb0f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f8_93_test_ff, 0xb0f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f9_93_test_ff, 0xb0f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fa_93_test_ff, 0xb0fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fb_93_test_ff, 0xb0fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fc_93_test_ff, 0xb0fc, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b100_93_test_ff, 0xb100, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b108_93_test_ff, 0xb108, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b110_93_test_ff, 0xb110, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b118_93_test_ff, 0xb118, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b120_93_test_ff, 0xb120, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b128_93_test_ff, 0xb128, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b130_93_test_ff, 0xb130, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b138_93_test_ff, 0xb138, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b139_93_test_ff, 0xb139, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b140_93_test_ff, 0xb140, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b148_93_test_ff, 0xb148, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b150_93_test_ff, 0xb150, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b158_93_test_ff, 0xb158, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b160_93_test_ff, 0xb160, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b168_93_test_ff, 0xb168, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b170_93_test_ff, 0xb170, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b178_93_test_ff, 0xb178, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b179_93_test_ff, 0xb179, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b180_93_test_ff, 0xb180, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b188_93_test_ff, 0xb188, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b190_93_test_ff, 0xb190, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b198_93_test_ff, 0xb198, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a0_93_test_ff, 0xb1a0, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a8_93_test_ff, 0xb1a8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b0_93_test_ff, 0xb1b0, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b1b8_93_test_ff, 0xb1b8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b9_93_test_ff, 0xb1b9, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1c0_93_test_ff, 0xb1c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1c8_93_test_ff, 0xb1c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d0_93_test_ff, 0xb1d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d8_93_test_ff, 0xb1d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e0_93_test_ff, 0xb1e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e8_93_test_ff, 0xb1e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f0_93_test_ff, 0xb1f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f8_93_test_ff, 0xb1f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f9_93_test_ff, 0xb1f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fa_93_test_ff, 0xb1fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fb_93_test_ff, 0xb1fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fc_93_test_ff, 0xb1fc, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_c000_93_test_ff, 0xc000, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c010_93_test_ff, 0xc010, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c018_93_test_ff, 0xc018, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c020_93_test_ff, 0xc020, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c028_93_test_ff, 0xc028, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c030_93_test_ff, 0xc030, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c038_93_test_ff, 0xc038, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c039_93_test_ff, 0xc039, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03a_93_test_ff, 0xc03a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03b_93_test_ff, 0xc03b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c03c_93_test_ff, 0xc03c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c040_93_test_ff, 0xc040, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c050_93_test_ff, 0xc050, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c058_93_test_ff, 0xc058, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c060_93_test_ff, 0xc060, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c068_93_test_ff, 0xc068, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c070_93_test_ff, 0xc070, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c078_93_test_ff, 0xc078, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c079_93_test_ff, 0xc079, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07a_93_test_ff, 0xc07a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07b_93_test_ff, 0xc07b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c07c_93_test_ff, 0xc07c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c080_93_test_ff, 0xc080, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c090_93_test_ff, 0xc090, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c098_93_test_ff, 0xc098, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a0_93_test_ff, 0xc0a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a8_93_test_ff, 0xc0a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b0_93_test_ff, 0xc0b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0b8_93_test_ff, 0xc0b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b9_93_test_ff, 0xc0b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0ba_93_test_ff, 0xc0ba, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0bb_93_test_ff, 0xc0bb, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0bc_93_test_ff, 0xc0bc, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0c0_93_test_ff, 0xc0c0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d0_93_test_ff, 0xc0d0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d8_93_test_ff, 0xc0d8, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e0_93_test_ff, 0xc0e0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e8_93_test_ff, 0xc0e8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f0_93_test_ff, 0xc0f0, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0f8_93_test_ff, 0xc0f8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f9_93_test_ff, 0xc0f9, 6, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fa_93_test_ff, 0xc0fa, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fb_93_test_ff, 0xc0fb, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0fc_93_test_ff, 0xc0fc, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c100_93_test_ff, 0xc100, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c108_93_test_ff, 0xc108, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c110_93_test_ff, 0xc110, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c118_93_test_ff, 0xc118, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c120_93_test_ff, 0xc120, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c128_93_test_ff, 0xc128, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c130_93_test_ff, 0xc130, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c138_93_test_ff, 0xc138, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c139_93_test_ff, 0xc139, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c140_93_test_ff, 0xc140, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c148_93_test_ff, 0xc148, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c150_93_test_ff, 0xc150, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c158_93_test_ff, 0xc158, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c160_93_test_ff, 0xc160, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c168_93_test_ff, 0xc168, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c170_93_test_ff, 0xc170, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c178_93_test_ff, 0xc178, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c179_93_test_ff, 0xc179, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c188_93_test_ff, 0xc188, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c190_93_test_ff, 0xc190, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c198_93_test_ff, 0xc198, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a0_93_test_ff, 0xc1a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a8_93_test_ff, 0xc1a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b0_93_test_ff, 0xc1b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c1b8_93_test_ff, 0xc1b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b9_93_test_ff, 0xc1b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1c0_93_test_ff, 0xc1c0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d0_93_test_ff, 0xc1d0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d8_93_test_ff, 0xc1d8, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e0_93_test_ff, 0xc1e0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e8_93_test_ff, 0xc1e8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f0_93_test_ff, 0xc1f0, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1f8_93_test_ff, 0xc1f8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f9_93_test_ff, 0xc1f9, 6, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fa_93_test_ff, 0xc1fa, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fb_93_test_ff, 0xc1fb, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1fc_93_test_ff, 0xc1fc, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_d000_93_test_ff, 0xd000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d010_93_test_ff, 0xd010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d018_93_test_ff, 0xd018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d020_93_test_ff, 0xd020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d028_93_test_ff, 0xd028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d030_93_test_ff, 0xd030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d038_93_test_ff, 0xd038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d039_93_test_ff, 0xd039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03a_93_test_ff, 0xd03a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03b_93_test_ff, 0xd03b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d03c_93_test_ff, 0xd03c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d040_93_test_ff, 0xd040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d048_93_test_ff, 0xd048, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d050_93_test_ff, 0xd050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d058_93_test_ff, 0xd058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d060_93_test_ff, 0xd060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d068_93_test_ff, 0xd068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d070_93_test_ff, 0xd070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d078_93_test_ff, 0xd078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d079_93_test_ff, 0xd079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07a_93_test_ff, 0xd07a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07b_93_test_ff, 0xd07b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d07c_93_test_ff, 0xd07c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d080_93_test_ff, 0xd080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d088_93_test_ff, 0xd088, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d090_93_test_ff, 0xd090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d098_93_test_ff, 0xd098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a0_93_test_ff, 0xd0a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a8_93_test_ff, 0xd0a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b0_93_test_ff, 0xd0b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0b8_93_test_ff, 0xd0b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b9_93_test_ff, 0xd0b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0ba_93_test_ff, 0xd0ba, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0bb_93_test_ff, 0xd0bb, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0bc_93_test_ff, 0xd0bc, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0c0_93_test_ff, 0xd0c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0c8_93_test_ff, 0xd0c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d0_93_test_ff, 0xd0d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d8_93_test_ff, 0xd0d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e0_93_test_ff, 0xd0e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e8_93_test_ff, 0xd0e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f0_93_test_ff, 0xd0f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f8_93_test_ff, 0xd0f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f9_93_test_ff, 0xd0f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fa_93_test_ff, 0xd0fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fb_93_test_ff, 0xd0fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fc_93_test_ff, 0xd0fc, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d100_93_test_ff, 0xd100, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d108_93_test_ff, 0xd108, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d110_93_test_ff, 0xd110, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d118_93_test_ff, 0xd118, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d120_93_test_ff, 0xd120, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d128_93_test_ff, 0xd128, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d130_93_test_ff, 0xd130, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d138_93_test_ff, 0xd138, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d139_93_test_ff, 0xd139, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d140_93_test_ff, 0xd140, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d148_93_test_ff, 0xd148, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d150_93_test_ff, 0xd150, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d158_93_test_ff, 0xd158, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d160_93_test_ff, 0xd160, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d168_93_test_ff, 0xd168, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d170_93_test_ff, 0xd170, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d178_93_test_ff, 0xd178, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d179_93_test_ff, 0xd179, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d180_93_test_ff, 0xd180, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d188_93_test_ff, 0xd188, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d190_93_test_ff, 0xd190, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d198_93_test_ff, 0xd198, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a0_93_test_ff, 0xd1a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a8_93_test_ff, 0xd1a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b0_93_test_ff, 0xd1b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d1b8_93_test_ff, 0xd1b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b9_93_test_ff, 0xd1b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1c0_93_test_ff, 0xd1c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1c8_93_test_ff, 0xd1c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d0_93_test_ff, 0xd1d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d8_93_test_ff, 0xd1d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e0_93_test_ff, 0xd1e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e8_93_test_ff, 0xd1e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f0_93_test_ff, 0xd1f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f8_93_test_ff, 0xd1f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f9_93_test_ff, 0xd1f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fa_93_test_ff, 0xd1fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fb_93_test_ff, 0xd1fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fc_93_test_ff, 0xd1fc, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_e000_93_test_ff, 0xe000, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e008_93_test_ff, 0xe008, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e010_93_test_ff, 0xe010, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e018_93_test_ff, 0xe018, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e020_93_test_ff, 0xe020, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e028_93_test_ff, 0xe028, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e030_93_test_ff, 0xe030, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e038_93_test_ff, 0xe038, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e040_93_test_ff, 0xe040, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e048_93_test_ff, 0xe048, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e050_93_test_ff, 0xe050, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e058_93_test_ff, 0xe058, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e060_93_test_ff, 0xe060, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e068_93_test_ff, 0xe068, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e070_93_test_ff, 0xe070, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e078_93_test_ff, 0xe078, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e080_93_test_ff, 0xe080, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e088_93_test_ff, 0xe088, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e090_93_test_ff, 0xe090, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e098_93_test_ff, 0xe098, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0a0_93_test_ff, 0xe0a0, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e0a8_93_test_ff, 0xe0a8, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e0b0_93_test_ff, 0xe0b0, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e0b8_93_test_ff, 0xe0b8, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0d0_93_test_ff, 0xe0d0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0d8_93_test_ff, 0xe0d8, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e0_93_test_ff, 0xe0e0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e8_93_test_ff, 0xe0e8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f0_93_test_ff, 0xe0f0, 2, { 2, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f8_93_test_ff, 0xe0f8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f9_93_test_ff, 0xe0f9, 6, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e100_93_test_ff, 0xe100, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e108_93_test_ff, 0xe108, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e110_93_test_ff, 0xe110, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e118_93_test_ff, 0xe118, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e120_93_test_ff, 0xe120, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e128_93_test_ff, 0xe128, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e130_93_test_ff, 0xe130, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e138_93_test_ff, 0xe138, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e140_93_test_ff, 0xe140, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e148_93_test_ff, 0xe148, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e150_93_test_ff, 0xe150, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e158_93_test_ff, 0xe158, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e160_93_test_ff, 0xe160, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e168_93_test_ff, 0xe168, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e170_93_test_ff, 0xe170, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e178_93_test_ff, 0xe178, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e180_93_test_ff, 0xe180, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e188_93_test_ff, 0xe188, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e190_93_test_ff, 0xe190, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e198_93_test_ff, 0xe198, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1a0_93_test_ff, 0xe1a0, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e1a8_93_test_ff, 0xe1a8, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e1b0_93_test_ff, 0xe1b0, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e1b8_93_test_ff, 0xe1b8, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1d0_93_test_ff, 0xe1d0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1d8_93_test_ff, 0xe1d8, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e0_93_test_ff, 0xe1e0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e8_93_test_ff, 0xe1e8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f0_93_test_ff, 0xe1f0, 2, { 2, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f8_93_test_ff, 0xe1f8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f9_93_test_ff, 0xe1f9, 6, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e2d0_93_test_ff, 0xe2d0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2d8_93_test_ff, 0xe2d8, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e0_93_test_ff, 0xe2e0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e8_93_test_ff, 0xe2e8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f0_93_test_ff, 0xe2f0, 2, { 2, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f8_93_test_ff, 0xe2f8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f9_93_test_ff, 0xe2f9, 6, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e3d0_93_test_ff, 0xe3d0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3d8_93_test_ff, 0xe3d8, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e0_93_test_ff, 0xe3e0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e8_93_test_ff, 0xe3e8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f0_93_test_ff, 0xe3f0, 2, { 2, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f8_93_test_ff, 0xe3f8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f9_93_test_ff, 0xe3f9, 6, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e4d0_93_test_ff, 0xe4d0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4d8_93_test_ff, 0xe4d8, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e0_93_test_ff, 0xe4e0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e8_93_test_ff, 0xe4e8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f0_93_test_ff, 0xe4f0, 2, { 2, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f8_93_test_ff, 0xe4f8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f9_93_test_ff, 0xe4f9, 6, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e5d0_93_test_ff, 0xe5d0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5d8_93_test_ff, 0xe5d8, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e0_93_test_ff, 0xe5e0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e8_93_test_ff, 0xe5e8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f0_93_test_ff, 0xe5f0, 2, { 2, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f8_93_test_ff, 0xe5f8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f9_93_test_ff, 0xe5f9, 6, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e6d0_93_test_ff, 0xe6d0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6d8_93_test_ff, 0xe6d8, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e0_93_test_ff, 0xe6e0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e8_93_test_ff, 0xe6e8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f0_93_test_ff, 0xe6f0, 2, { 2, 0 }, 0 }, /* RORW */
{ NULL, op_e6f8_93_test_ff, 0xe6f8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f9_93_test_ff, 0xe6f9, 6, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e7d0_93_test_ff, 0xe7d0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7d8_93_test_ff, 0xe7d8, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e0_93_test_ff, 0xe7e0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e8_93_test_ff, 0xe7e8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f0_93_test_ff, 0xe7f0, 2, { 2, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f8_93_test_ff, 0xe7f8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f9_93_test_ff, 0xe7f9, 6, { 0, 0 }, 0 }, /* ROLW */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8c0_93_test_ff, 0xe8c0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8d0_93_test_ff, 0xe8d0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8e8_93_test_ff, 0xe8e8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f0_93_test_ff, 0xe8f0, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f8_93_test_ff, 0xe8f8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f9_93_test_ff, 0xe8f9, 8, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fa_93_test_ff, 0xe8fa, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fb_93_test_ff, 0xe8fb, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9c0_93_test_ff, 0xe9c0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9d0_93_test_ff, 0xe9d0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9e8_93_test_ff, 0xe9e8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f0_93_test_ff, 0xe9f0, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f8_93_test_ff, 0xe9f8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f9_93_test_ff, 0xe9f9, 8, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fa_93_test_ff, 0xe9fa, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fb_93_test_ff, 0xe9fb, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eac0_93_test_ff, 0xeac0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ead0_93_test_ff, 0xead0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eae8_93_test_ff, 0xeae8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf0_93_test_ff, 0xeaf0, 4, { 2, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf8_93_test_ff, 0xeaf8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf9_93_test_ff, 0xeaf9, 8, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebc0_93_test_ff, 0xebc0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebd0_93_test_ff, 0xebd0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebe8_93_test_ff, 0xebe8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf0_93_test_ff, 0xebf0, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf8_93_test_ff, 0xebf8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf9_93_test_ff, 0xebf9, 8, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfa_93_test_ff, 0xebfa, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfb_93_test_ff, 0xebfb, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecc0_93_test_ff, 0xecc0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecd0_93_test_ff, 0xecd0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ece8_93_test_ff, 0xece8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf0_93_test_ff, 0xecf0, 4, { 2, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf8_93_test_ff, 0xecf8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf9_93_test_ff, 0xecf9, 8, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edc0_93_test_ff, 0xedc0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edd0_93_test_ff, 0xedd0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ede8_93_test_ff, 0xede8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf0_93_test_ff, 0xedf0, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf8_93_test_ff, 0xedf8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf9_93_test_ff, 0xedf9, 8, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfa_93_test_ff, 0xedfa, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfb_93_test_ff, 0xedfb, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eec0_93_test_ff, 0xeec0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eed0_93_test_ff, 0xeed0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eee8_93_test_ff, 0xeee8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef0_93_test_ff, 0xeef0, 4, { 2, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef8_93_test_ff, 0xeef8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef9_93_test_ff, 0xeef9, 8, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efc0_93_test_ff, 0xefc0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efd0_93_test_ff, 0xefd0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efe8_93_test_ff, 0xefe8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff0_93_test_ff, 0xeff0, 4, { 2, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff8_93_test_ff, 0xeff8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff9_93_test_ff, 0xeff9, 8, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f000_93_test_ff, 0xf000, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f008_93_test_ff, 0xf008, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f010_93_test_ff, 0xf010, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f018_93_test_ff, 0xf018, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f020_93_test_ff, 0xf020, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f028_93_test_ff, 0xf028, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f030_93_test_ff, 0xf030, -1, { -3, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f038_93_test_ff, 0xf038, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f039_93_test_ff, 0xf039, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f200_93_test_ff, 0xf200, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f208_93_test_ff, 0xf208, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f210_93_test_ff, 0xf210, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f218_93_test_ff, 0xf218, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f220_93_test_ff, 0xf220, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f228_93_test_ff, 0xf228, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f230_93_test_ff, 0xf230, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f238_93_test_ff, 0xf238, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f239_93_test_ff, 0xf239, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23a_93_test_ff, 0xf23a, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23b_93_test_ff, 0xf23b, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23c_93_test_ff, 0xf23c, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f240_93_test_ff, 0xf240, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f248_93_test_ff, 0xf248, -1, { 0, 0 }, 0 }, /* FDBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f250_93_test_ff, 0xf250, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f258_93_test_ff, 0xf258, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f260_93_test_ff, 0xf260, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f268_93_test_ff, 0xf268, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f270_93_test_ff, 0xf270, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f278_93_test_ff, 0xf278, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f279_93_test_ff, 0xf279, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27a_93_test_ff, 0xf27a, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27b_93_test_ff, 0xf27b, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27c_93_test_ff, 0xf27c, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f280_93_test_ff, 0xf280, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f2c0_93_test_ff, 0xf2c0, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f310_93_test_ff, 0xf310, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f320_93_test_ff, 0xf320, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f328_93_test_ff, 0xf328, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f330_93_test_ff, 0xf330, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f338_93_test_ff, 0xf338, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f339_93_test_ff, 0xf339, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f350_93_test_ff, 0xf350, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f358_93_test_ff, 0xf358, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f368_93_test_ff, 0xf368, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f370_93_test_ff, 0xf370, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f378_93_test_ff, 0xf378, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f379_93_test_ff, 0xf379, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37a_93_test_ff, 0xf37a, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37b_93_test_ff, 0xf37b, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
{ 0 }};
#endif /* CPUEMU_93 */
#ifdef CPUEMU_94
const struct cputbl CPUFUNC(op_smalltbl_94_test)[] = {
{ NULL, op_0000_94_test_ff, 0x0000, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0010_94_test_ff, 0x0010, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0018_94_test_ff, 0x0018, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0020_94_test_ff, 0x0020, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0028_94_test_ff, 0x0028, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0030_94_test_ff, 0x0030, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0038_94_test_ff, 0x0038, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0039_94_test_ff, 0x0039, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_003c_94_test_ff, 0x003c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0040_94_test_ff, 0x0040, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0050_94_test_ff, 0x0050, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0058_94_test_ff, 0x0058, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0060_94_test_ff, 0x0060, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0068_94_test_ff, 0x0068, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0070_94_test_ff, 0x0070, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0078_94_test_ff, 0x0078, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0079_94_test_ff, 0x0079, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_007c_94_test_ff, 0x007c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0080_94_test_ff, 0x0080, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0090_94_test_ff, 0x0090, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0098_94_test_ff, 0x0098, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a0_94_test_ff, 0x00a0, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a8_94_test_ff, 0x00a8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b0_94_test_ff, 0x00b0, 6, { 2, 0 }, 0 }, /* OR */
{ NULL, op_00b8_94_test_ff, 0x00b8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b9_94_test_ff, 0x00b9, 10, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00d0_94_test_ff, 0x00d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00e8_94_test_ff, 0x00e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f0_94_test_ff, 0x00f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f8_94_test_ff, 0x00f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f9_94_test_ff, 0x00f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fa_94_test_ff, 0x00fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fb_94_test_ff, 0x00fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0100_94_test_ff, 0x0100, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0108_94_test_ff, 0x0108, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0110_94_test_ff, 0x0110, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0118_94_test_ff, 0x0118, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0120_94_test_ff, 0x0120, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0128_94_test_ff, 0x0128, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0130_94_test_ff, 0x0130, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0138_94_test_ff, 0x0138, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0139_94_test_ff, 0x0139, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013a_94_test_ff, 0x013a, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013b_94_test_ff, 0x013b, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_013c_94_test_ff, 0x013c, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0140_94_test_ff, 0x0140, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0148_94_test_ff, 0x0148, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0150_94_test_ff, 0x0150, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0158_94_test_ff, 0x0158, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0160_94_test_ff, 0x0160, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0168_94_test_ff, 0x0168, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0170_94_test_ff, 0x0170, 2, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0178_94_test_ff, 0x0178, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0179_94_test_ff, 0x0179, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0180_94_test_ff, 0x0180, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0188_94_test_ff, 0x0188, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_0190_94_test_ff, 0x0190, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0198_94_test_ff, 0x0198, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a0_94_test_ff, 0x01a0, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a8_94_test_ff, 0x01a8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b0_94_test_ff, 0x01b0, 2, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_01b8_94_test_ff, 0x01b8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b9_94_test_ff, 0x01b9, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01c0_94_test_ff, 0x01c0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01c8_94_test_ff, 0x01c8, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_01d0_94_test_ff, 0x01d0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01d8_94_test_ff, 0x01d8, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e0_94_test_ff, 0x01e0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e8_94_test_ff, 0x01e8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f0_94_test_ff, 0x01f0, 2, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_01f8_94_test_ff, 0x01f8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f9_94_test_ff, 0x01f9, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0200_94_test_ff, 0x0200, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0210_94_test_ff, 0x0210, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0218_94_test_ff, 0x0218, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0220_94_test_ff, 0x0220, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0228_94_test_ff, 0x0228, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0230_94_test_ff, 0x0230, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0238_94_test_ff, 0x0238, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0239_94_test_ff, 0x0239, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_023c_94_test_ff, 0x023c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0240_94_test_ff, 0x0240, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0250_94_test_ff, 0x0250, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0258_94_test_ff, 0x0258, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0260_94_test_ff, 0x0260, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0268_94_test_ff, 0x0268, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0270_94_test_ff, 0x0270, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0278_94_test_ff, 0x0278, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0279_94_test_ff, 0x0279, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_027c_94_test_ff, 0x027c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0280_94_test_ff, 0x0280, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0290_94_test_ff, 0x0290, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0298_94_test_ff, 0x0298, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a0_94_test_ff, 0x02a0, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a8_94_test_ff, 0x02a8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b0_94_test_ff, 0x02b0, 6, { 2, 0 }, 0 }, /* AND */
{ NULL, op_02b8_94_test_ff, 0x02b8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b9_94_test_ff, 0x02b9, 10, { 0, 0 }, 0 }, /* AND */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02d0_94_test_ff, 0x02d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02e8_94_test_ff, 0x02e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f0_94_test_ff, 0x02f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f8_94_test_ff, 0x02f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f9_94_test_ff, 0x02f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fa_94_test_ff, 0x02fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fb_94_test_ff, 0x02fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0400_94_test_ff, 0x0400, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0410_94_test_ff, 0x0410, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0418_94_test_ff, 0x0418, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0420_94_test_ff, 0x0420, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0428_94_test_ff, 0x0428, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0430_94_test_ff, 0x0430, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0438_94_test_ff, 0x0438, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0439_94_test_ff, 0x0439, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0440_94_test_ff, 0x0440, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0450_94_test_ff, 0x0450, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0458_94_test_ff, 0x0458, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0460_94_test_ff, 0x0460, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0468_94_test_ff, 0x0468, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0470_94_test_ff, 0x0470, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0478_94_test_ff, 0x0478, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0479_94_test_ff, 0x0479, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0480_94_test_ff, 0x0480, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0490_94_test_ff, 0x0490, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0498_94_test_ff, 0x0498, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a0_94_test_ff, 0x04a0, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a8_94_test_ff, 0x04a8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b0_94_test_ff, 0x04b0, 6, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_04b8_94_test_ff, 0x04b8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b9_94_test_ff, 0x04b9, 10, { 0, 0 }, 0 }, /* SUB */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04d0_94_test_ff, 0x04d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04e8_94_test_ff, 0x04e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f0_94_test_ff, 0x04f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f8_94_test_ff, 0x04f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f9_94_test_ff, 0x04f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fa_94_test_ff, 0x04fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fb_94_test_ff, 0x04fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0600_94_test_ff, 0x0600, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0610_94_test_ff, 0x0610, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0618_94_test_ff, 0x0618, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0620_94_test_ff, 0x0620, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0628_94_test_ff, 0x0628, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0630_94_test_ff, 0x0630, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0638_94_test_ff, 0x0638, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0639_94_test_ff, 0x0639, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0640_94_test_ff, 0x0640, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0650_94_test_ff, 0x0650, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0658_94_test_ff, 0x0658, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0660_94_test_ff, 0x0660, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0668_94_test_ff, 0x0668, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0670_94_test_ff, 0x0670, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0678_94_test_ff, 0x0678, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0679_94_test_ff, 0x0679, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0680_94_test_ff, 0x0680, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0690_94_test_ff, 0x0690, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0698_94_test_ff, 0x0698, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a0_94_test_ff, 0x06a0, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a8_94_test_ff, 0x06a8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b0_94_test_ff, 0x06b0, 6, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_06b8_94_test_ff, 0x06b8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b9_94_test_ff, 0x06b9, 10, { 0, 0 }, 0 }, /* ADD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c0_94_test_ff, 0x06c0, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c8_94_test_ff, 0x06c8, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06d0_94_test_ff, 0x06d0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06e8_94_test_ff, 0x06e8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f0_94_test_ff, 0x06f0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f8_94_test_ff, 0x06f8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f9_94_test_ff, 0x06f9, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fa_94_test_ff, 0x06fa, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fb_94_test_ff, 0x06fb, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
{ NULL, op_0800_94_test_ff, 0x0800, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0810_94_test_ff, 0x0810, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0818_94_test_ff, 0x0818, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0820_94_test_ff, 0x0820, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0828_94_test_ff, 0x0828, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0830_94_test_ff, 0x0830, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0838_94_test_ff, 0x0838, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0839_94_test_ff, 0x0839, 8, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083a_94_test_ff, 0x083a, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083b_94_test_ff, 0x083b, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0840_94_test_ff, 0x0840, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0850_94_test_ff, 0x0850, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0858_94_test_ff, 0x0858, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0860_94_test_ff, 0x0860, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0868_94_test_ff, 0x0868, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0870_94_test_ff, 0x0870, 4, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0878_94_test_ff, 0x0878, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0879_94_test_ff, 0x0879, 8, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0880_94_test_ff, 0x0880, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0890_94_test_ff, 0x0890, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0898_94_test_ff, 0x0898, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a0_94_test_ff, 0x08a0, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a8_94_test_ff, 0x08a8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b0_94_test_ff, 0x08b0, 4, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_08b8_94_test_ff, 0x08b8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b9_94_test_ff, 0x08b9, 8, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08c0_94_test_ff, 0x08c0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d0_94_test_ff, 0x08d0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d8_94_test_ff, 0x08d8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e0_94_test_ff, 0x08e0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e8_94_test_ff, 0x08e8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f0_94_test_ff, 0x08f0, 4, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_08f8_94_test_ff, 0x08f8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f9_94_test_ff, 0x08f9, 8, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0a00_94_test_ff, 0x0a00, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a10_94_test_ff, 0x0a10, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a18_94_test_ff, 0x0a18, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a20_94_test_ff, 0x0a20, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a28_94_test_ff, 0x0a28, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a30_94_test_ff, 0x0a30, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a38_94_test_ff, 0x0a38, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a39_94_test_ff, 0x0a39, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a3c_94_test_ff, 0x0a3c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a40_94_test_ff, 0x0a40, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a50_94_test_ff, 0x0a50, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a58_94_test_ff, 0x0a58, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a60_94_test_ff, 0x0a60, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a68_94_test_ff, 0x0a68, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a70_94_test_ff, 0x0a70, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a78_94_test_ff, 0x0a78, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a79_94_test_ff, 0x0a79, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a7c_94_test_ff, 0x0a7c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a80_94_test_ff, 0x0a80, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a90_94_test_ff, 0x0a90, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a98_94_test_ff, 0x0a98, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa0_94_test_ff, 0x0aa0, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa8_94_test_ff, 0x0aa8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab0_94_test_ff, 0x0ab0, 6, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0ab8_94_test_ff, 0x0ab8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab9_94_test_ff, 0x0ab9, 10, { 0, 0 }, 0 }, /* EOR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad0_94_test_ff, 0x0ad0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad8_94_test_ff, 0x0ad8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae0_94_test_ff, 0x0ae0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae8_94_test_ff, 0x0ae8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af0_94_test_ff, 0x0af0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af8_94_test_ff, 0x0af8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af9_94_test_ff, 0x0af9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
{ NULL, op_0c00_94_test_ff, 0x0c00, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c10_94_test_ff, 0x0c10, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c18_94_test_ff, 0x0c18, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c20_94_test_ff, 0x0c20, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c28_94_test_ff, 0x0c28, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c30_94_test_ff, 0x0c30, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c38_94_test_ff, 0x0c38, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c39_94_test_ff, 0x0c39, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3a_94_test_ff, 0x0c3a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3b_94_test_ff, 0x0c3b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c40_94_test_ff, 0x0c40, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c50_94_test_ff, 0x0c50, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c58_94_test_ff, 0x0c58, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c60_94_test_ff, 0x0c60, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c68_94_test_ff, 0x0c68, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c70_94_test_ff, 0x0c70, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c78_94_test_ff, 0x0c78, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c79_94_test_ff, 0x0c79, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7a_94_test_ff, 0x0c7a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7b_94_test_ff, 0x0c7b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c80_94_test_ff, 0x0c80, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c90_94_test_ff, 0x0c90, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c98_94_test_ff, 0x0c98, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca0_94_test_ff, 0x0ca0, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca8_94_test_ff, 0x0ca8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb0_94_test_ff, 0x0cb0, 6, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0cb8_94_test_ff, 0x0cb8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb9_94_test_ff, 0x0cb9, 10, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cba_94_test_ff, 0x0cba, 8, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cbb_94_test_ff, 0x0cbb, 6, { 2, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd0_94_test_ff, 0x0cd0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd8_94_test_ff, 0x0cd8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce0_94_test_ff, 0x0ce0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce8_94_test_ff, 0x0ce8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf0_94_test_ff, 0x0cf0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf8_94_test_ff, 0x0cf8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf9_94_test_ff, 0x0cf9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cfc_94_test_ff, 0x0cfc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e10_94_test_ff, 0x0e10, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e18_94_test_ff, 0x0e18, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e20_94_test_ff, 0x0e20, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e28_94_test_ff, 0x0e28, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e30_94_test_ff, 0x0e30, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e38_94_test_ff, 0x0e38, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e39_94_test_ff, 0x0e39, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e50_94_test_ff, 0x0e50, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e58_94_test_ff, 0x0e58, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e60_94_test_ff, 0x0e60, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e68_94_test_ff, 0x0e68, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e70_94_test_ff, 0x0e70, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e78_94_test_ff, 0x0e78, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e79_94_test_ff, 0x0e79, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e90_94_test_ff, 0x0e90, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e98_94_test_ff, 0x0e98, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea0_94_test_ff, 0x0ea0, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea8_94_test_ff, 0x0ea8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb0_94_test_ff, 0x0eb0, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb8_94_test_ff, 0x0eb8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb9_94_test_ff, 0x0eb9, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed0_94_test_ff, 0x0ed0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed8_94_test_ff, 0x0ed8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee0_94_test_ff, 0x0ee0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee8_94_test_ff, 0x0ee8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef0_94_test_ff, 0x0ef0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef8_94_test_ff, 0x0ef8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef9_94_test_ff, 0x0ef9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0efc_94_test_ff, 0x0efc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
{ NULL, op_1000_94_test_ff, 0x1000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1010_94_test_ff, 0x1010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1018_94_test_ff, 0x1018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1020_94_test_ff, 0x1020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1028_94_test_ff, 0x1028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1030_94_test_ff, 0x1030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1038_94_test_ff, 0x1038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1039_94_test_ff, 0x1039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103a_94_test_ff, 0x103a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103b_94_test_ff, 0x103b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_103c_94_test_ff, 0x103c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1080_94_test_ff, 0x1080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1090_94_test_ff, 0x1090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1098_94_test_ff, 0x1098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a0_94_test_ff, 0x10a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a8_94_test_ff, 0x10a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b0_94_test_ff, 0x10b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10b8_94_test_ff, 0x10b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b9_94_test_ff, 0x10b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10ba_94_test_ff, 0x10ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10bb_94_test_ff, 0x10bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10bc_94_test_ff, 0x10bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10c0_94_test_ff, 0x10c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d0_94_test_ff, 0x10d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d8_94_test_ff, 0x10d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e0_94_test_ff, 0x10e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e8_94_test_ff, 0x10e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f0_94_test_ff, 0x10f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10f8_94_test_ff, 0x10f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f9_94_test_ff, 0x10f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fa_94_test_ff, 0x10fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fb_94_test_ff, 0x10fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10fc_94_test_ff, 0x10fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1100_94_test_ff, 0x1100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1110_94_test_ff, 0x1110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1118_94_test_ff, 0x1118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1120_94_test_ff, 0x1120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1128_94_test_ff, 0x1128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1130_94_test_ff, 0x1130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1138_94_test_ff, 0x1138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1139_94_test_ff, 0x1139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113a_94_test_ff, 0x113a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113b_94_test_ff, 0x113b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_113c_94_test_ff, 0x113c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1140_94_test_ff, 0x1140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1150_94_test_ff, 0x1150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1158_94_test_ff, 0x1158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1160_94_test_ff, 0x1160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1168_94_test_ff, 0x1168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1170_94_test_ff, 0x1170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1178_94_test_ff, 0x1178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1179_94_test_ff, 0x1179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117a_94_test_ff, 0x117a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117b_94_test_ff, 0x117b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_117c_94_test_ff, 0x117c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1180_94_test_ff, 0x1180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1190_94_test_ff, 0x1190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1198_94_test_ff, 0x1198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a0_94_test_ff, 0x11a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a8_94_test_ff, 0x11a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b0_94_test_ff, 0x11b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11b8_94_test_ff, 0x11b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b9_94_test_ff, 0x11b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11ba_94_test_ff, 0x11ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11bb_94_test_ff, 0x11bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11bc_94_test_ff, 0x11bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11c0_94_test_ff, 0x11c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d0_94_test_ff, 0x11d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d8_94_test_ff, 0x11d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e0_94_test_ff, 0x11e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e8_94_test_ff, 0x11e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f0_94_test_ff, 0x11f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11f8_94_test_ff, 0x11f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f9_94_test_ff, 0x11f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fa_94_test_ff, 0x11fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fb_94_test_ff, 0x11fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11fc_94_test_ff, 0x11fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13c0_94_test_ff, 0x13c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d0_94_test_ff, 0x13d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d8_94_test_ff, 0x13d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e0_94_test_ff, 0x13e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e8_94_test_ff, 0x13e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f0_94_test_ff, 0x13f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13f8_94_test_ff, 0x13f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f9_94_test_ff, 0x13f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fa_94_test_ff, 0x13fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fb_94_test_ff, 0x13fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13fc_94_test_ff, 0x13fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2000_94_test_ff, 0x2000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2008_94_test_ff, 0x2008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2010_94_test_ff, 0x2010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2018_94_test_ff, 0x2018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2020_94_test_ff, 0x2020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2028_94_test_ff, 0x2028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2030_94_test_ff, 0x2030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2038_94_test_ff, 0x2038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2039_94_test_ff, 0x2039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203a_94_test_ff, 0x203a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203b_94_test_ff, 0x203b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_203c_94_test_ff, 0x203c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2040_94_test_ff, 0x2040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2048_94_test_ff, 0x2048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2050_94_test_ff, 0x2050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2058_94_test_ff, 0x2058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2060_94_test_ff, 0x2060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2068_94_test_ff, 0x2068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2070_94_test_ff, 0x2070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_2078_94_test_ff, 0x2078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2079_94_test_ff, 0x2079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207a_94_test_ff, 0x207a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207b_94_test_ff, 0x207b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_207c_94_test_ff, 0x207c, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2080_94_test_ff, 0x2080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2088_94_test_ff, 0x2088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2090_94_test_ff, 0x2090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2098_94_test_ff, 0x2098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a0_94_test_ff, 0x20a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a8_94_test_ff, 0x20a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b0_94_test_ff, 0x20b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20b8_94_test_ff, 0x20b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b9_94_test_ff, 0x20b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20ba_94_test_ff, 0x20ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20bb_94_test_ff, 0x20bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20bc_94_test_ff, 0x20bc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c0_94_test_ff, 0x20c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c8_94_test_ff, 0x20c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d0_94_test_ff, 0x20d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d8_94_test_ff, 0x20d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e0_94_test_ff, 0x20e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e8_94_test_ff, 0x20e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f0_94_test_ff, 0x20f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20f8_94_test_ff, 0x20f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f9_94_test_ff, 0x20f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fa_94_test_ff, 0x20fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fb_94_test_ff, 0x20fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20fc_94_test_ff, 0x20fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2100_94_test_ff, 0x2100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2108_94_test_ff, 0x2108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2110_94_test_ff, 0x2110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2118_94_test_ff, 0x2118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2120_94_test_ff, 0x2120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2128_94_test_ff, 0x2128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2130_94_test_ff, 0x2130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2138_94_test_ff, 0x2138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2139_94_test_ff, 0x2139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213a_94_test_ff, 0x213a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213b_94_test_ff, 0x213b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_213c_94_test_ff, 0x213c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2140_94_test_ff, 0x2140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2148_94_test_ff, 0x2148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2150_94_test_ff, 0x2150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2158_94_test_ff, 0x2158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2160_94_test_ff, 0x2160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2168_94_test_ff, 0x2168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2170_94_test_ff, 0x2170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2178_94_test_ff, 0x2178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2179_94_test_ff, 0x2179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217a_94_test_ff, 0x217a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217b_94_test_ff, 0x217b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_217c_94_test_ff, 0x217c, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2180_94_test_ff, 0x2180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2188_94_test_ff, 0x2188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2190_94_test_ff, 0x2190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2198_94_test_ff, 0x2198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a0_94_test_ff, 0x21a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a8_94_test_ff, 0x21a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b0_94_test_ff, 0x21b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21b8_94_test_ff, 0x21b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b9_94_test_ff, 0x21b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21ba_94_test_ff, 0x21ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21bb_94_test_ff, 0x21bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21bc_94_test_ff, 0x21bc, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21c0_94_test_ff, 0x21c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21c8_94_test_ff, 0x21c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d0_94_test_ff, 0x21d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d8_94_test_ff, 0x21d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e0_94_test_ff, 0x21e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e8_94_test_ff, 0x21e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f0_94_test_ff, 0x21f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21f8_94_test_ff, 0x21f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f9_94_test_ff, 0x21f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fa_94_test_ff, 0x21fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fb_94_test_ff, 0x21fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21fc_94_test_ff, 0x21fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c0_94_test_ff, 0x23c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c8_94_test_ff, 0x23c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d0_94_test_ff, 0x23d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d8_94_test_ff, 0x23d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e0_94_test_ff, 0x23e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e8_94_test_ff, 0x23e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f0_94_test_ff, 0x23f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23f8_94_test_ff, 0x23f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f9_94_test_ff, 0x23f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fa_94_test_ff, 0x23fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fb_94_test_ff, 0x23fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23fc_94_test_ff, 0x23fc, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3000_94_test_ff, 0x3000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3008_94_test_ff, 0x3008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3010_94_test_ff, 0x3010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3018_94_test_ff, 0x3018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3020_94_test_ff, 0x3020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3028_94_test_ff, 0x3028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3030_94_test_ff, 0x3030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3038_94_test_ff, 0x3038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3039_94_test_ff, 0x3039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303a_94_test_ff, 0x303a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303b_94_test_ff, 0x303b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_303c_94_test_ff, 0x303c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3040_94_test_ff, 0x3040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3048_94_test_ff, 0x3048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3050_94_test_ff, 0x3050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3058_94_test_ff, 0x3058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3060_94_test_ff, 0x3060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3068_94_test_ff, 0x3068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3070_94_test_ff, 0x3070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_3078_94_test_ff, 0x3078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3079_94_test_ff, 0x3079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307a_94_test_ff, 0x307a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307b_94_test_ff, 0x307b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_307c_94_test_ff, 0x307c, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3080_94_test_ff, 0x3080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3088_94_test_ff, 0x3088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3090_94_test_ff, 0x3090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3098_94_test_ff, 0x3098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a0_94_test_ff, 0x30a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a8_94_test_ff, 0x30a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b0_94_test_ff, 0x30b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30b8_94_test_ff, 0x30b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b9_94_test_ff, 0x30b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30ba_94_test_ff, 0x30ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30bb_94_test_ff, 0x30bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30bc_94_test_ff, 0x30bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c0_94_test_ff, 0x30c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c8_94_test_ff, 0x30c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d0_94_test_ff, 0x30d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d8_94_test_ff, 0x30d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e0_94_test_ff, 0x30e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e8_94_test_ff, 0x30e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f0_94_test_ff, 0x30f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30f8_94_test_ff, 0x30f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f9_94_test_ff, 0x30f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fa_94_test_ff, 0x30fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fb_94_test_ff, 0x30fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30fc_94_test_ff, 0x30fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3100_94_test_ff, 0x3100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3108_94_test_ff, 0x3108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3110_94_test_ff, 0x3110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3118_94_test_ff, 0x3118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3120_94_test_ff, 0x3120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3128_94_test_ff, 0x3128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3130_94_test_ff, 0x3130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3138_94_test_ff, 0x3138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3139_94_test_ff, 0x3139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313a_94_test_ff, 0x313a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313b_94_test_ff, 0x313b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_313c_94_test_ff, 0x313c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3140_94_test_ff, 0x3140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3148_94_test_ff, 0x3148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3150_94_test_ff, 0x3150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3158_94_test_ff, 0x3158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3160_94_test_ff, 0x3160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3168_94_test_ff, 0x3168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3170_94_test_ff, 0x3170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3178_94_test_ff, 0x3178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3179_94_test_ff, 0x3179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317a_94_test_ff, 0x317a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317b_94_test_ff, 0x317b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_317c_94_test_ff, 0x317c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3180_94_test_ff, 0x3180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3188_94_test_ff, 0x3188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3190_94_test_ff, 0x3190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3198_94_test_ff, 0x3198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a0_94_test_ff, 0x31a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a8_94_test_ff, 0x31a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b0_94_test_ff, 0x31b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31b8_94_test_ff, 0x31b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b9_94_test_ff, 0x31b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31ba_94_test_ff, 0x31ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31bb_94_test_ff, 0x31bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31bc_94_test_ff, 0x31bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31c0_94_test_ff, 0x31c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31c8_94_test_ff, 0x31c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d0_94_test_ff, 0x31d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d8_94_test_ff, 0x31d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e0_94_test_ff, 0x31e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e8_94_test_ff, 0x31e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f0_94_test_ff, 0x31f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31f8_94_test_ff, 0x31f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f9_94_test_ff, 0x31f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fa_94_test_ff, 0x31fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fb_94_test_ff, 0x31fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31fc_94_test_ff, 0x31fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c0_94_test_ff, 0x33c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c8_94_test_ff, 0x33c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d0_94_test_ff, 0x33d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d8_94_test_ff, 0x33d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e0_94_test_ff, 0x33e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e8_94_test_ff, 0x33e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f0_94_test_ff, 0x33f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33f8_94_test_ff, 0x33f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f9_94_test_ff, 0x33f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fa_94_test_ff, 0x33fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fb_94_test_ff, 0x33fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33fc_94_test_ff, 0x33fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_4000_94_test_ff, 0x4000, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4010_94_test_ff, 0x4010, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4018_94_test_ff, 0x4018, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4020_94_test_ff, 0x4020, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4028_94_test_ff, 0x4028, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4030_94_test_ff, 0x4030, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4038_94_test_ff, 0x4038, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4039_94_test_ff, 0x4039, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4040_94_test_ff, 0x4040, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4050_94_test_ff, 0x4050, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4058_94_test_ff, 0x4058, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4060_94_test_ff, 0x4060, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4068_94_test_ff, 0x4068, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4070_94_test_ff, 0x4070, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4078_94_test_ff, 0x4078, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4079_94_test_ff, 0x4079, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4080_94_test_ff, 0x4080, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4090_94_test_ff, 0x4090, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4098_94_test_ff, 0x4098, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a0_94_test_ff, 0x40a0, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a8_94_test_ff, 0x40a8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b0_94_test_ff, 0x40b0, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_40b8_94_test_ff, 0x40b8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b9_94_test_ff, 0x40b9, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40c0_94_test_ff, 0x40c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d0_94_test_ff, 0x40d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d8_94_test_ff, 0x40d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e0_94_test_ff, 0x40e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e8_94_test_ff, 0x40e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f0_94_test_ff, 0x40f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f8_94_test_ff, 0x40f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f9_94_test_ff, 0x40f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4100_94_test_ff, 0x4100, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4110_94_test_ff, 0x4110, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4118_94_test_ff, 0x4118, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4120_94_test_ff, 0x4120, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4128_94_test_ff, 0x4128, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4130_94_test_ff, 0x4130, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4138_94_test_ff, 0x4138, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4139_94_test_ff, 0x4139, 6, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413a_94_test_ff, 0x413a, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413b_94_test_ff, 0x413b, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413c_94_test_ff, 0x413c, 6, { 0, 0 }, 0 }, /* CHK */
#endif
{ NULL, op_4180_94_test_ff, 0x4180, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4190_94_test_ff, 0x4190, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4198_94_test_ff, 0x4198, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a0_94_test_ff, 0x41a0, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a8_94_test_ff, 0x41a8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b0_94_test_ff, 0x41b0, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41b8_94_test_ff, 0x41b8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b9_94_test_ff, 0x41b9, 6, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41ba_94_test_ff, 0x41ba, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41bb_94_test_ff, 0x41bb, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41bc_94_test_ff, 0x41bc, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41d0_94_test_ff, 0x41d0, 2, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41e8_94_test_ff, 0x41e8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f0_94_test_ff, 0x41f0, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_41f8_94_test_ff, 0x41f8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f9_94_test_ff, 0x41f9, 6, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fa_94_test_ff, 0x41fa, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fb_94_test_ff, 0x41fb, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_4200_94_test_ff, 0x4200, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4210_94_test_ff, 0x4210, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4218_94_test_ff, 0x4218, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4220_94_test_ff, 0x4220, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4228_94_test_ff, 0x4228, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4230_94_test_ff, 0x4230, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4238_94_test_ff, 0x4238, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4239_94_test_ff, 0x4239, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4240_94_test_ff, 0x4240, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4250_94_test_ff, 0x4250, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4258_94_test_ff, 0x4258, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4260_94_test_ff, 0x4260, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4268_94_test_ff, 0x4268, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4270_94_test_ff, 0x4270, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4278_94_test_ff, 0x4278, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4279_94_test_ff, 0x4279, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4280_94_test_ff, 0x4280, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4290_94_test_ff, 0x4290, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4298_94_test_ff, 0x4298, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a0_94_test_ff, 0x42a0, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a8_94_test_ff, 0x42a8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b0_94_test_ff, 0x42b0, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_42b8_94_test_ff, 0x42b8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b9_94_test_ff, 0x42b9, 6, { 0, 0 }, 0 }, /* CLR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42c0_94_test_ff, 0x42c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d0_94_test_ff, 0x42d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d8_94_test_ff, 0x42d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e0_94_test_ff, 0x42e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e8_94_test_ff, 0x42e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f0_94_test_ff, 0x42f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f8_94_test_ff, 0x42f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f9_94_test_ff, 0x42f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#endif
{ NULL, op_4400_94_test_ff, 0x4400, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4410_94_test_ff, 0x4410, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4418_94_test_ff, 0x4418, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4420_94_test_ff, 0x4420, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4428_94_test_ff, 0x4428, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4430_94_test_ff, 0x4430, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4438_94_test_ff, 0x4438, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4439_94_test_ff, 0x4439, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4440_94_test_ff, 0x4440, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4450_94_test_ff, 0x4450, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4458_94_test_ff, 0x4458, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4460_94_test_ff, 0x4460, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4468_94_test_ff, 0x4468, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4470_94_test_ff, 0x4470, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4478_94_test_ff, 0x4478, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4479_94_test_ff, 0x4479, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4480_94_test_ff, 0x4480, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4490_94_test_ff, 0x4490, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4498_94_test_ff, 0x4498, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a0_94_test_ff, 0x44a0, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a8_94_test_ff, 0x44a8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b0_94_test_ff, 0x44b0, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_44b8_94_test_ff, 0x44b8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b9_94_test_ff, 0x44b9, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44c0_94_test_ff, 0x44c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d0_94_test_ff, 0x44d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d8_94_test_ff, 0x44d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e0_94_test_ff, 0x44e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e8_94_test_ff, 0x44e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f0_94_test_ff, 0x44f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f8_94_test_ff, 0x44f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f9_94_test_ff, 0x44f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fa_94_test_ff, 0x44fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fb_94_test_ff, 0x44fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fc_94_test_ff, 0x44fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4600_94_test_ff, 0x4600, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4610_94_test_ff, 0x4610, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4618_94_test_ff, 0x4618, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4620_94_test_ff, 0x4620, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4628_94_test_ff, 0x4628, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4630_94_test_ff, 0x4630, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4638_94_test_ff, 0x4638, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4639_94_test_ff, 0x4639, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4640_94_test_ff, 0x4640, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4650_94_test_ff, 0x4650, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4658_94_test_ff, 0x4658, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4660_94_test_ff, 0x4660, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4668_94_test_ff, 0x4668, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4670_94_test_ff, 0x4670, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4678_94_test_ff, 0x4678, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4679_94_test_ff, 0x4679, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4680_94_test_ff, 0x4680, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4690_94_test_ff, 0x4690, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4698_94_test_ff, 0x4698, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a0_94_test_ff, 0x46a0, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a8_94_test_ff, 0x46a8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b0_94_test_ff, 0x46b0, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_46b8_94_test_ff, 0x46b8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b9_94_test_ff, 0x46b9, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46c0_94_test_ff, 0x46c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d0_94_test_ff, 0x46d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d8_94_test_ff, 0x46d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e0_94_test_ff, 0x46e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e8_94_test_ff, 0x46e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f0_94_test_ff, 0x46f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f8_94_test_ff, 0x46f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f9_94_test_ff, 0x46f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fa_94_test_ff, 0x46fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fb_94_test_ff, 0x46fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fc_94_test_ff, 0x46fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4800_94_test_ff, 0x4800, 2, { 0, 0 }, 0 }, /* NBCD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4808_94_test_ff, 0x4808, 6, { 0, 0 }, 0 }, /* LINK */
#endif
{ NULL, op_4810_94_test_ff, 0x4810, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4818_94_test_ff, 0x4818, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4820_94_test_ff, 0x4820, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4828_94_test_ff, 0x4828, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4830_94_test_ff, 0x4830, 2, { 2, 0 }, 0 }, /* NBCD */
{ NULL, op_4838_94_test_ff, 0x4838, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4839_94_test_ff, 0x4839, 6, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4840_94_test_ff, 0x4840, 2, { 0, 0 }, 0 }, /* SWAP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4848_94_test_ff, 0x4848, 2, { 0, 0 }, 0 }, /* BKPT */
#endif
{ NULL, op_4850_94_test_ff, 0x4850, 2, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4868_94_test_ff, 0x4868, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4870_94_test_ff, 0x4870, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4878_94_test_ff, 0x4878, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4879_94_test_ff, 0x4879, 6, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487a_94_test_ff, 0x487a, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487b_94_test_ff, 0x487b, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4880_94_test_ff, 0x4880, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_4890_94_test_ff, 0x4890, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a0_94_test_ff, 0x48a0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a8_94_test_ff, 0x48a8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b0_94_test_ff, 0x48b0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b8_94_test_ff, 0x48b8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b9_94_test_ff, 0x48b9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48c0_94_test_ff, 0x48c0, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_48d0_94_test_ff, 0x48d0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e0_94_test_ff, 0x48e0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e8_94_test_ff, 0x48e8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f0_94_test_ff, 0x48f0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f8_94_test_ff, 0x48f8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f9_94_test_ff, 0x48f9, 8, { 0, 0 }, 0 }, /* MVMLE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_49c0_94_test_ff, 0x49c0, 2, { 0, 0 }, 0 }, /* EXT */
#endif
{ NULL, op_4a00_94_test_ff, 0x4a00, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a10_94_test_ff, 0x4a10, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a18_94_test_ff, 0x4a18, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a20_94_test_ff, 0x4a20, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a28_94_test_ff, 0x4a28, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a30_94_test_ff, 0x4a30, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a38_94_test_ff, 0x4a38, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a39_94_test_ff, 0x4a39, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3a_94_test_ff, 0x4a3a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3b_94_test_ff, 0x4a3b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3c_94_test_ff, 0x4a3c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a40_94_test_ff, 0x4a40, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a48_94_test_ff, 0x4a48, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a50_94_test_ff, 0x4a50, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a58_94_test_ff, 0x4a58, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a60_94_test_ff, 0x4a60, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a68_94_test_ff, 0x4a68, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a70_94_test_ff, 0x4a70, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a78_94_test_ff, 0x4a78, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a79_94_test_ff, 0x4a79, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7a_94_test_ff, 0x4a7a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7b_94_test_ff, 0x4a7b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7c_94_test_ff, 0x4a7c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a80_94_test_ff, 0x4a80, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a88_94_test_ff, 0x4a88, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a90_94_test_ff, 0x4a90, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a98_94_test_ff, 0x4a98, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa0_94_test_ff, 0x4aa0, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa8_94_test_ff, 0x4aa8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab0_94_test_ff, 0x4ab0, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4ab8_94_test_ff, 0x4ab8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab9_94_test_ff, 0x4ab9, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4aba_94_test_ff, 0x4aba, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abb_94_test_ff, 0x4abb, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abc_94_test_ff, 0x4abc, 6, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4ac0_94_test_ff, 0x4ac0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad0_94_test_ff, 0x4ad0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad8_94_test_ff, 0x4ad8, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae0_94_test_ff, 0x4ae0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae8_94_test_ff, 0x4ae8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af0_94_test_ff, 0x4af0, 2, { 2, 0 }, 0 }, /* TAS */
{ NULL, op_4af8_94_test_ff, 0x4af8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af9_94_test_ff, 0x4af9, 6, { 0, 0 }, 0 }, /* TAS */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c00_94_test_ff, 0x4c00, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c10_94_test_ff, 0x4c10, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c18_94_test_ff, 0x4c18, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c20_94_test_ff, 0x4c20, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c28_94_test_ff, 0x4c28, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c30_94_test_ff, 0x4c30, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c38_94_test_ff, 0x4c38, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c39_94_test_ff, 0x4c39, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3a_94_test_ff, 0x4c3a, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3b_94_test_ff, 0x4c3b, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3c_94_test_ff, 0x4c3c, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c40_94_test_ff, 0x4c40, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c50_94_test_ff, 0x4c50, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c58_94_test_ff, 0x4c58, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c60_94_test_ff, 0x4c60, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c68_94_test_ff, 0x4c68, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c70_94_test_ff, 0x4c70, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c78_94_test_ff, 0x4c78, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c79_94_test_ff, 0x4c79, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7a_94_test_ff, 0x4c7a, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7b_94_test_ff, 0x4c7b, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7c_94_test_ff, 0x4c7c, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
{ NULL, op_4c90_94_test_ff, 0x4c90, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4c98_94_test_ff, 0x4c98, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ca8_94_test_ff, 0x4ca8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb0_94_test_ff, 0x4cb0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb8_94_test_ff, 0x4cb8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb9_94_test_ff, 0x4cb9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cba_94_test_ff, 0x4cba, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cbb_94_test_ff, 0x4cbb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd0_94_test_ff, 0x4cd0, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd8_94_test_ff, 0x4cd8, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ce8_94_test_ff, 0x4ce8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf0_94_test_ff, 0x4cf0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf8_94_test_ff, 0x4cf8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf9_94_test_ff, 0x4cf9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfa_94_test_ff, 0x4cfa, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfb_94_test_ff, 0x4cfb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4e40_94_test_ff, 0x4e40, 2, { 0, 0 }, 0 }, /* TRAP */
{ NULL, op_4e50_94_test_ff, 0x4e50, 4, { 0, 0 }, 0 }, /* LINK */
{ NULL, op_4e58_94_test_ff, 0x4e58, 2, { 0, 0 }, 0 }, /* UNLK */
{ NULL, op_4e60_94_test_ff, 0x4e60, 2, { 0, 0 }, 0 }, /* MVR2USP */
{ NULL, op_4e68_94_test_ff, 0x4e68, 2, { 0, 0 }, 0 }, /* MVUSP2R */
{ NULL, op_4e70_94_test_ff, 0x4e70, 2, { 0, 0 }, 0 }, /* RESET */
{ NULL, op_4e71_94_test_ff, 0x4e71, 2, { 0, 0 }, 0 }, /* NOP */
{ NULL, op_4e72_94_test_ff, 0x4e72, 0, { 0, 0 }, 0 }, /* STOP */
{ NULL, op_4e73_94_test_ff, 0x4e73, 2, { 0, 0 }, 1 }, /* RTE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e74_94_test_ff, 0x4e74, 4, { 0, 0 }, 2 }, /* RTD */
#endif
{ NULL, op_4e75_94_test_ff, 0x4e75, 2, { 0, 0 }, 1 }, /* RTS */
{ NULL, op_4e76_94_test_ff, 0x4e76, 2, { 0, 0 }, 0 }, /* TRAPV */
{ NULL, op_4e77_94_test_ff, 0x4e77, 2, { 0, 0 }, 1 }, /* RTR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7a_94_test_ff, 0x4e7a, 4, { 0, 0 }, 0 }, /* MOVEC2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7b_94_test_ff, 0x4e7b, 4, { 0, 0 }, 0 }, /* MOVE2C */
#endif
{ NULL, op_4e90_94_test_ff, 0x4e90, 2, { 0, 0 }, 1 }, /* JSR */
{ NULL, op_4ea8_94_test_ff, 0x4ea8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb0_94_test_ff, 0x4eb0, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4eb8_94_test_ff, 0x4eb8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb9_94_test_ff, 0x4eb9, 6, { 0, 0 }, 3 }, /* JSR */
{ NULL, op_4eba_94_test_ff, 0x4eba, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4ebb_94_test_ff, 0x4ebb, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4ed0_94_test_ff, 0x4ed0, 2, { 0, 0 }, 1 }, /* JMP */
{ NULL, op_4ee8_94_test_ff, 0x4ee8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef0_94_test_ff, 0x4ef0, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_4ef8_94_test_ff, 0x4ef8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef9_94_test_ff, 0x4ef9, 6, { 0, 0 }, 3 }, /* JMP */
{ NULL, op_4efa_94_test_ff, 0x4efa, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4efb_94_test_ff, 0x4efb, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_5000_94_test_ff, 0x5000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5010_94_test_ff, 0x5010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5018_94_test_ff, 0x5018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5020_94_test_ff, 0x5020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5028_94_test_ff, 0x5028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5030_94_test_ff, 0x5030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5038_94_test_ff, 0x5038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5039_94_test_ff, 0x5039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5040_94_test_ff, 0x5040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5048_94_test_ff, 0x5048, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5050_94_test_ff, 0x5050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5058_94_test_ff, 0x5058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5060_94_test_ff, 0x5060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5068_94_test_ff, 0x5068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5070_94_test_ff, 0x5070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5078_94_test_ff, 0x5078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5079_94_test_ff, 0x5079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5080_94_test_ff, 0x5080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5088_94_test_ff, 0x5088, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5090_94_test_ff, 0x5090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5098_94_test_ff, 0x5098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a0_94_test_ff, 0x50a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a8_94_test_ff, 0x50a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b0_94_test_ff, 0x50b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_50b8_94_test_ff, 0x50b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b9_94_test_ff, 0x50b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50c0_94_test_ff, 0x50c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50c8_94_test_ff, 0x50c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_50d0_94_test_ff, 0x50d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50d8_94_test_ff, 0x50d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e0_94_test_ff, 0x50e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e8_94_test_ff, 0x50e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f0_94_test_ff, 0x50f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_50f8_94_test_ff, 0x50f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f9_94_test_ff, 0x50f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fa_94_test_ff, 0x50fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fb_94_test_ff, 0x50fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fc_94_test_ff, 0x50fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5100_94_test_ff, 0x5100, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5110_94_test_ff, 0x5110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5118_94_test_ff, 0x5118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5120_94_test_ff, 0x5120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5128_94_test_ff, 0x5128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5130_94_test_ff, 0x5130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5138_94_test_ff, 0x5138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5139_94_test_ff, 0x5139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5140_94_test_ff, 0x5140, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5148_94_test_ff, 0x5148, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5150_94_test_ff, 0x5150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5158_94_test_ff, 0x5158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5160_94_test_ff, 0x5160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5168_94_test_ff, 0x5168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5170_94_test_ff, 0x5170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5178_94_test_ff, 0x5178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5179_94_test_ff, 0x5179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5180_94_test_ff, 0x5180, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5188_94_test_ff, 0x5188, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5190_94_test_ff, 0x5190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5198_94_test_ff, 0x5198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a0_94_test_ff, 0x51a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a8_94_test_ff, 0x51a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b0_94_test_ff, 0x51b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_51b8_94_test_ff, 0x51b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b9_94_test_ff, 0x51b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51c0_94_test_ff, 0x51c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51c8_94_test_ff, 0x51c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_51d0_94_test_ff, 0x51d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51d8_94_test_ff, 0x51d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e0_94_test_ff, 0x51e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e8_94_test_ff, 0x51e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f0_94_test_ff, 0x51f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_51f8_94_test_ff, 0x51f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f9_94_test_ff, 0x51f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fa_94_test_ff, 0x51fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fb_94_test_ff, 0x51fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fc_94_test_ff, 0x51fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_52c0_94_test_ff, 0x52c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c8_94_test_ff, 0x52c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_52d0_94_test_ff, 0x52d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52d8_94_test_ff, 0x52d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e0_94_test_ff, 0x52e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e8_94_test_ff, 0x52e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f0_94_test_ff, 0x52f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_52f8_94_test_ff, 0x52f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f9_94_test_ff, 0x52f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fa_94_test_ff, 0x52fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fb_94_test_ff, 0x52fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fc_94_test_ff, 0x52fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_53c0_94_test_ff, 0x53c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c8_94_test_ff, 0x53c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_53d0_94_test_ff, 0x53d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53d8_94_test_ff, 0x53d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e0_94_test_ff, 0x53e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e8_94_test_ff, 0x53e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f0_94_test_ff, 0x53f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_53f8_94_test_ff, 0x53f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f9_94_test_ff, 0x53f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fa_94_test_ff, 0x53fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fb_94_test_ff, 0x53fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fc_94_test_ff, 0x53fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_54c0_94_test_ff, 0x54c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c8_94_test_ff, 0x54c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_54d0_94_test_ff, 0x54d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54d8_94_test_ff, 0x54d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e0_94_test_ff, 0x54e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e8_94_test_ff, 0x54e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f0_94_test_ff, 0x54f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_54f8_94_test_ff, 0x54f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f9_94_test_ff, 0x54f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fa_94_test_ff, 0x54fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fb_94_test_ff, 0x54fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fc_94_test_ff, 0x54fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_55c0_94_test_ff, 0x55c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c8_94_test_ff, 0x55c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_55d0_94_test_ff, 0x55d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55d8_94_test_ff, 0x55d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e0_94_test_ff, 0x55e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e8_94_test_ff, 0x55e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f0_94_test_ff, 0x55f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_55f8_94_test_ff, 0x55f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f9_94_test_ff, 0x55f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fa_94_test_ff, 0x55fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fb_94_test_ff, 0x55fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fc_94_test_ff, 0x55fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_56c0_94_test_ff, 0x56c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c8_94_test_ff, 0x56c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_56d0_94_test_ff, 0x56d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56d8_94_test_ff, 0x56d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e0_94_test_ff, 0x56e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e8_94_test_ff, 0x56e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f0_94_test_ff, 0x56f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_56f8_94_test_ff, 0x56f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f9_94_test_ff, 0x56f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fa_94_test_ff, 0x56fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fb_94_test_ff, 0x56fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fc_94_test_ff, 0x56fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_57c0_94_test_ff, 0x57c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c8_94_test_ff, 0x57c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_57d0_94_test_ff, 0x57d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57d8_94_test_ff, 0x57d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e0_94_test_ff, 0x57e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e8_94_test_ff, 0x57e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f0_94_test_ff, 0x57f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_57f8_94_test_ff, 0x57f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f9_94_test_ff, 0x57f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fa_94_test_ff, 0x57fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fb_94_test_ff, 0x57fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fc_94_test_ff, 0x57fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_58c0_94_test_ff, 0x58c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c8_94_test_ff, 0x58c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_58d0_94_test_ff, 0x58d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58d8_94_test_ff, 0x58d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e0_94_test_ff, 0x58e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e8_94_test_ff, 0x58e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f0_94_test_ff, 0x58f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_58f8_94_test_ff, 0x58f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f9_94_test_ff, 0x58f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fa_94_test_ff, 0x58fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fb_94_test_ff, 0x58fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fc_94_test_ff, 0x58fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_59c0_94_test_ff, 0x59c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c8_94_test_ff, 0x59c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_59d0_94_test_ff, 0x59d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59d8_94_test_ff, 0x59d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e0_94_test_ff, 0x59e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e8_94_test_ff, 0x59e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f0_94_test_ff, 0x59f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_59f8_94_test_ff, 0x59f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f9_94_test_ff, 0x59f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fa_94_test_ff, 0x59fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fb_94_test_ff, 0x59fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fc_94_test_ff, 0x59fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ac0_94_test_ff, 0x5ac0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac8_94_test_ff, 0x5ac8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ad0_94_test_ff, 0x5ad0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ad8_94_test_ff, 0x5ad8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae0_94_test_ff, 0x5ae0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae8_94_test_ff, 0x5ae8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af0_94_test_ff, 0x5af0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5af8_94_test_ff, 0x5af8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af9_94_test_ff, 0x5af9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afa_94_test_ff, 0x5afa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afb_94_test_ff, 0x5afb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afc_94_test_ff, 0x5afc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5bc0_94_test_ff, 0x5bc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc8_94_test_ff, 0x5bc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5bd0_94_test_ff, 0x5bd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bd8_94_test_ff, 0x5bd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be0_94_test_ff, 0x5be0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be8_94_test_ff, 0x5be8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf0_94_test_ff, 0x5bf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5bf8_94_test_ff, 0x5bf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf9_94_test_ff, 0x5bf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfa_94_test_ff, 0x5bfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfb_94_test_ff, 0x5bfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfc_94_test_ff, 0x5bfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5cc0_94_test_ff, 0x5cc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc8_94_test_ff, 0x5cc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5cd0_94_test_ff, 0x5cd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cd8_94_test_ff, 0x5cd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce0_94_test_ff, 0x5ce0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce8_94_test_ff, 0x5ce8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf0_94_test_ff, 0x5cf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5cf8_94_test_ff, 0x5cf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf9_94_test_ff, 0x5cf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfa_94_test_ff, 0x5cfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfb_94_test_ff, 0x5cfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfc_94_test_ff, 0x5cfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5dc0_94_test_ff, 0x5dc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc8_94_test_ff, 0x5dc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5dd0_94_test_ff, 0x5dd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dd8_94_test_ff, 0x5dd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de0_94_test_ff, 0x5de0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de8_94_test_ff, 0x5de8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df0_94_test_ff, 0x5df0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5df8_94_test_ff, 0x5df8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df9_94_test_ff, 0x5df9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfa_94_test_ff, 0x5dfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfb_94_test_ff, 0x5dfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfc_94_test_ff, 0x5dfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ec0_94_test_ff, 0x5ec0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec8_94_test_ff, 0x5ec8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ed0_94_test_ff, 0x5ed0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ed8_94_test_ff, 0x5ed8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee0_94_test_ff, 0x5ee0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee8_94_test_ff, 0x5ee8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef0_94_test_ff, 0x5ef0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ef8_94_test_ff, 0x5ef8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef9_94_test_ff, 0x5ef9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efa_94_test_ff, 0x5efa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efb_94_test_ff, 0x5efb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efc_94_test_ff, 0x5efc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5fc0_94_test_ff, 0x5fc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc8_94_test_ff, 0x5fc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5fd0_94_test_ff, 0x5fd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fd8_94_test_ff, 0x5fd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe0_94_test_ff, 0x5fe0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe8_94_test_ff, 0x5fe8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff0_94_test_ff, 0x5ff0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ff8_94_test_ff, 0x5ff8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff9_94_test_ff, 0x5ff9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffa_94_test_ff, 0x5ffa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffb_94_test_ff, 0x5ffb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffc_94_test_ff, 0x5ffc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_6000_94_test_ff, 0x6000, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6001_94_test_ff, 0x6001, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_60ff_94_test_ff, 0x60ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6100_94_test_ff, 0x6100, 4, { 0, 0 }, 2 }, /* BSR */
{ NULL, op_6101_94_test_ff, 0x6101, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_61ff_94_test_ff, 0x61ff, 6, { 0, 0 }, 3 }, /* BSR */
{ NULL, op_6200_94_test_ff, 0x6200, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6201_94_test_ff, 0x6201, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_62ff_94_test_ff, 0x62ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6300_94_test_ff, 0x6300, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6301_94_test_ff, 0x6301, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_63ff_94_test_ff, 0x63ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6400_94_test_ff, 0x6400, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6401_94_test_ff, 0x6401, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_64ff_94_test_ff, 0x64ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6500_94_test_ff, 0x6500, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6501_94_test_ff, 0x6501, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_65ff_94_test_ff, 0x65ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6600_94_test_ff, 0x6600, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6601_94_test_ff, 0x6601, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_66ff_94_test_ff, 0x66ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6700_94_test_ff, 0x6700, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6701_94_test_ff, 0x6701, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_67ff_94_test_ff, 0x67ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6800_94_test_ff, 0x6800, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6801_94_test_ff, 0x6801, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_68ff_94_test_ff, 0x68ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6900_94_test_ff, 0x6900, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6901_94_test_ff, 0x6901, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_69ff_94_test_ff, 0x69ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6a00_94_test_ff, 0x6a00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6a01_94_test_ff, 0x6a01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6aff_94_test_ff, 0x6aff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6b00_94_test_ff, 0x6b00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6b01_94_test_ff, 0x6b01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6bff_94_test_ff, 0x6bff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6c00_94_test_ff, 0x6c00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6c01_94_test_ff, 0x6c01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6cff_94_test_ff, 0x6cff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6d00_94_test_ff, 0x6d00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6d01_94_test_ff, 0x6d01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6dff_94_test_ff, 0x6dff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6e00_94_test_ff, 0x6e00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6e01_94_test_ff, 0x6e01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6eff_94_test_ff, 0x6eff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6f00_94_test_ff, 0x6f00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6f01_94_test_ff, 0x6f01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6fff_94_test_ff, 0x6fff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_7000_94_test_ff, 0x7000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_8000_94_test_ff, 0x8000, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8010_94_test_ff, 0x8010, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8018_94_test_ff, 0x8018, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8020_94_test_ff, 0x8020, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8028_94_test_ff, 0x8028, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8030_94_test_ff, 0x8030, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8038_94_test_ff, 0x8038, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8039_94_test_ff, 0x8039, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803a_94_test_ff, 0x803a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803b_94_test_ff, 0x803b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_803c_94_test_ff, 0x803c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8040_94_test_ff, 0x8040, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8050_94_test_ff, 0x8050, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8058_94_test_ff, 0x8058, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8060_94_test_ff, 0x8060, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8068_94_test_ff, 0x8068, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8070_94_test_ff, 0x8070, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8078_94_test_ff, 0x8078, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8079_94_test_ff, 0x8079, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807a_94_test_ff, 0x807a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807b_94_test_ff, 0x807b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_807c_94_test_ff, 0x807c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8080_94_test_ff, 0x8080, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8090_94_test_ff, 0x8090, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8098_94_test_ff, 0x8098, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a0_94_test_ff, 0x80a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a8_94_test_ff, 0x80a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b0_94_test_ff, 0x80b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80b8_94_test_ff, 0x80b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b9_94_test_ff, 0x80b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80ba_94_test_ff, 0x80ba, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80bb_94_test_ff, 0x80bb, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80bc_94_test_ff, 0x80bc, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80c0_94_test_ff, 0x80c0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d0_94_test_ff, 0x80d0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d8_94_test_ff, 0x80d8, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e0_94_test_ff, 0x80e0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e8_94_test_ff, 0x80e8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f0_94_test_ff, 0x80f0, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80f8_94_test_ff, 0x80f8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f9_94_test_ff, 0x80f9, 6, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fa_94_test_ff, 0x80fa, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fb_94_test_ff, 0x80fb, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80fc_94_test_ff, 0x80fc, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_8100_94_test_ff, 0x8100, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8108_94_test_ff, 0x8108, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8110_94_test_ff, 0x8110, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8118_94_test_ff, 0x8118, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8120_94_test_ff, 0x8120, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8128_94_test_ff, 0x8128, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8130_94_test_ff, 0x8130, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8138_94_test_ff, 0x8138, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8139_94_test_ff, 0x8139, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8140_94_test_ff, 0x8140, 4, { 0, 0 }, 0 }, /* PACK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8148_94_test_ff, 0x8148, 4, { 0, 0 }, 0 }, /* PACK */
#endif
{ NULL, op_8150_94_test_ff, 0x8150, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8158_94_test_ff, 0x8158, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8160_94_test_ff, 0x8160, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8168_94_test_ff, 0x8168, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8170_94_test_ff, 0x8170, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8178_94_test_ff, 0x8178, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8179_94_test_ff, 0x8179, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8180_94_test_ff, 0x8180, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8188_94_test_ff, 0x8188, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
{ NULL, op_8190_94_test_ff, 0x8190, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8198_94_test_ff, 0x8198, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a0_94_test_ff, 0x81a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a8_94_test_ff, 0x81a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b0_94_test_ff, 0x81b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_81b8_94_test_ff, 0x81b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b9_94_test_ff, 0x81b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81c0_94_test_ff, 0x81c0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d0_94_test_ff, 0x81d0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d8_94_test_ff, 0x81d8, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e0_94_test_ff, 0x81e0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e8_94_test_ff, 0x81e8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f0_94_test_ff, 0x81f0, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81f8_94_test_ff, 0x81f8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f9_94_test_ff, 0x81f9, 6, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fa_94_test_ff, 0x81fa, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fb_94_test_ff, 0x81fb, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81fc_94_test_ff, 0x81fc, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_9000_94_test_ff, 0x9000, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9010_94_test_ff, 0x9010, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9018_94_test_ff, 0x9018, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9020_94_test_ff, 0x9020, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9028_94_test_ff, 0x9028, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9030_94_test_ff, 0x9030, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9038_94_test_ff, 0x9038, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9039_94_test_ff, 0x9039, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903a_94_test_ff, 0x903a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903b_94_test_ff, 0x903b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_903c_94_test_ff, 0x903c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9040_94_test_ff, 0x9040, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9048_94_test_ff, 0x9048, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9050_94_test_ff, 0x9050, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9058_94_test_ff, 0x9058, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9060_94_test_ff, 0x9060, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9068_94_test_ff, 0x9068, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9070_94_test_ff, 0x9070, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9078_94_test_ff, 0x9078, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9079_94_test_ff, 0x9079, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907a_94_test_ff, 0x907a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907b_94_test_ff, 0x907b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_907c_94_test_ff, 0x907c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9080_94_test_ff, 0x9080, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9088_94_test_ff, 0x9088, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9090_94_test_ff, 0x9090, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9098_94_test_ff, 0x9098, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a0_94_test_ff, 0x90a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a8_94_test_ff, 0x90a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b0_94_test_ff, 0x90b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90b8_94_test_ff, 0x90b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b9_94_test_ff, 0x90b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90ba_94_test_ff, 0x90ba, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90bb_94_test_ff, 0x90bb, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90bc_94_test_ff, 0x90bc, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90c0_94_test_ff, 0x90c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90c8_94_test_ff, 0x90c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d0_94_test_ff, 0x90d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d8_94_test_ff, 0x90d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e0_94_test_ff, 0x90e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e8_94_test_ff, 0x90e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f0_94_test_ff, 0x90f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90f8_94_test_ff, 0x90f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f9_94_test_ff, 0x90f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fa_94_test_ff, 0x90fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fb_94_test_ff, 0x90fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90fc_94_test_ff, 0x90fc, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_9100_94_test_ff, 0x9100, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9108_94_test_ff, 0x9108, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9110_94_test_ff, 0x9110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9118_94_test_ff, 0x9118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9120_94_test_ff, 0x9120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9128_94_test_ff, 0x9128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9130_94_test_ff, 0x9130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9138_94_test_ff, 0x9138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9139_94_test_ff, 0x9139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9140_94_test_ff, 0x9140, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9148_94_test_ff, 0x9148, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9150_94_test_ff, 0x9150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9158_94_test_ff, 0x9158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9160_94_test_ff, 0x9160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9168_94_test_ff, 0x9168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9170_94_test_ff, 0x9170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9178_94_test_ff, 0x9178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9179_94_test_ff, 0x9179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9180_94_test_ff, 0x9180, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9188_94_test_ff, 0x9188, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9190_94_test_ff, 0x9190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9198_94_test_ff, 0x9198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a0_94_test_ff, 0x91a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a8_94_test_ff, 0x91a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b0_94_test_ff, 0x91b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_91b8_94_test_ff, 0x91b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b9_94_test_ff, 0x91b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91c0_94_test_ff, 0x91c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91c8_94_test_ff, 0x91c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d0_94_test_ff, 0x91d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d8_94_test_ff, 0x91d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e0_94_test_ff, 0x91e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e8_94_test_ff, 0x91e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f0_94_test_ff, 0x91f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91f8_94_test_ff, 0x91f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f9_94_test_ff, 0x91f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fa_94_test_ff, 0x91fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fb_94_test_ff, 0x91fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91fc_94_test_ff, 0x91fc, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_b000_94_test_ff, 0xb000, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b010_94_test_ff, 0xb010, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b018_94_test_ff, 0xb018, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b020_94_test_ff, 0xb020, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b028_94_test_ff, 0xb028, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b030_94_test_ff, 0xb030, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b038_94_test_ff, 0xb038, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b039_94_test_ff, 0xb039, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03a_94_test_ff, 0xb03a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03b_94_test_ff, 0xb03b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b03c_94_test_ff, 0xb03c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b040_94_test_ff, 0xb040, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b048_94_test_ff, 0xb048, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b050_94_test_ff, 0xb050, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b058_94_test_ff, 0xb058, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b060_94_test_ff, 0xb060, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b068_94_test_ff, 0xb068, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b070_94_test_ff, 0xb070, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b078_94_test_ff, 0xb078, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b079_94_test_ff, 0xb079, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07a_94_test_ff, 0xb07a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07b_94_test_ff, 0xb07b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b07c_94_test_ff, 0xb07c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b080_94_test_ff, 0xb080, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b088_94_test_ff, 0xb088, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b090_94_test_ff, 0xb090, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b098_94_test_ff, 0xb098, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a0_94_test_ff, 0xb0a0, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a8_94_test_ff, 0xb0a8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b0_94_test_ff, 0xb0b0, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0b8_94_test_ff, 0xb0b8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b9_94_test_ff, 0xb0b9, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0ba_94_test_ff, 0xb0ba, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0bb_94_test_ff, 0xb0bb, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0bc_94_test_ff, 0xb0bc, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0c0_94_test_ff, 0xb0c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0c8_94_test_ff, 0xb0c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d0_94_test_ff, 0xb0d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d8_94_test_ff, 0xb0d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e0_94_test_ff, 0xb0e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e8_94_test_ff, 0xb0e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f0_94_test_ff, 0xb0f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f8_94_test_ff, 0xb0f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f9_94_test_ff, 0xb0f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fa_94_test_ff, 0xb0fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fb_94_test_ff, 0xb0fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fc_94_test_ff, 0xb0fc, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b100_94_test_ff, 0xb100, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b108_94_test_ff, 0xb108, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b110_94_test_ff, 0xb110, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b118_94_test_ff, 0xb118, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b120_94_test_ff, 0xb120, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b128_94_test_ff, 0xb128, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b130_94_test_ff, 0xb130, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b138_94_test_ff, 0xb138, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b139_94_test_ff, 0xb139, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b140_94_test_ff, 0xb140, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b148_94_test_ff, 0xb148, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b150_94_test_ff, 0xb150, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b158_94_test_ff, 0xb158, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b160_94_test_ff, 0xb160, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b168_94_test_ff, 0xb168, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b170_94_test_ff, 0xb170, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b178_94_test_ff, 0xb178, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b179_94_test_ff, 0xb179, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b180_94_test_ff, 0xb180, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b188_94_test_ff, 0xb188, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b190_94_test_ff, 0xb190, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b198_94_test_ff, 0xb198, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a0_94_test_ff, 0xb1a0, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a8_94_test_ff, 0xb1a8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b0_94_test_ff, 0xb1b0, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b1b8_94_test_ff, 0xb1b8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b9_94_test_ff, 0xb1b9, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1c0_94_test_ff, 0xb1c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1c8_94_test_ff, 0xb1c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d0_94_test_ff, 0xb1d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d8_94_test_ff, 0xb1d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e0_94_test_ff, 0xb1e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e8_94_test_ff, 0xb1e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f0_94_test_ff, 0xb1f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f8_94_test_ff, 0xb1f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f9_94_test_ff, 0xb1f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fa_94_test_ff, 0xb1fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fb_94_test_ff, 0xb1fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fc_94_test_ff, 0xb1fc, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_c000_94_test_ff, 0xc000, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c010_94_test_ff, 0xc010, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c018_94_test_ff, 0xc018, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c020_94_test_ff, 0xc020, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c028_94_test_ff, 0xc028, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c030_94_test_ff, 0xc030, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c038_94_test_ff, 0xc038, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c039_94_test_ff, 0xc039, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03a_94_test_ff, 0xc03a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03b_94_test_ff, 0xc03b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c03c_94_test_ff, 0xc03c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c040_94_test_ff, 0xc040, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c050_94_test_ff, 0xc050, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c058_94_test_ff, 0xc058, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c060_94_test_ff, 0xc060, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c068_94_test_ff, 0xc068, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c070_94_test_ff, 0xc070, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c078_94_test_ff, 0xc078, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c079_94_test_ff, 0xc079, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07a_94_test_ff, 0xc07a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07b_94_test_ff, 0xc07b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c07c_94_test_ff, 0xc07c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c080_94_test_ff, 0xc080, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c090_94_test_ff, 0xc090, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c098_94_test_ff, 0xc098, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a0_94_test_ff, 0xc0a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a8_94_test_ff, 0xc0a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b0_94_test_ff, 0xc0b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0b8_94_test_ff, 0xc0b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b9_94_test_ff, 0xc0b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0ba_94_test_ff, 0xc0ba, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0bb_94_test_ff, 0xc0bb, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0bc_94_test_ff, 0xc0bc, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0c0_94_test_ff, 0xc0c0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d0_94_test_ff, 0xc0d0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d8_94_test_ff, 0xc0d8, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e0_94_test_ff, 0xc0e0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e8_94_test_ff, 0xc0e8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f0_94_test_ff, 0xc0f0, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0f8_94_test_ff, 0xc0f8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f9_94_test_ff, 0xc0f9, 6, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fa_94_test_ff, 0xc0fa, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fb_94_test_ff, 0xc0fb, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0fc_94_test_ff, 0xc0fc, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c100_94_test_ff, 0xc100, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c108_94_test_ff, 0xc108, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c110_94_test_ff, 0xc110, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c118_94_test_ff, 0xc118, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c120_94_test_ff, 0xc120, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c128_94_test_ff, 0xc128, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c130_94_test_ff, 0xc130, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c138_94_test_ff, 0xc138, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c139_94_test_ff, 0xc139, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c140_94_test_ff, 0xc140, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c148_94_test_ff, 0xc148, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c150_94_test_ff, 0xc150, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c158_94_test_ff, 0xc158, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c160_94_test_ff, 0xc160, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c168_94_test_ff, 0xc168, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c170_94_test_ff, 0xc170, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c178_94_test_ff, 0xc178, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c179_94_test_ff, 0xc179, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c188_94_test_ff, 0xc188, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c190_94_test_ff, 0xc190, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c198_94_test_ff, 0xc198, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a0_94_test_ff, 0xc1a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a8_94_test_ff, 0xc1a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b0_94_test_ff, 0xc1b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c1b8_94_test_ff, 0xc1b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b9_94_test_ff, 0xc1b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1c0_94_test_ff, 0xc1c0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d0_94_test_ff, 0xc1d0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d8_94_test_ff, 0xc1d8, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e0_94_test_ff, 0xc1e0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e8_94_test_ff, 0xc1e8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f0_94_test_ff, 0xc1f0, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1f8_94_test_ff, 0xc1f8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f9_94_test_ff, 0xc1f9, 6, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fa_94_test_ff, 0xc1fa, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fb_94_test_ff, 0xc1fb, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1fc_94_test_ff, 0xc1fc, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_d000_94_test_ff, 0xd000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d010_94_test_ff, 0xd010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d018_94_test_ff, 0xd018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d020_94_test_ff, 0xd020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d028_94_test_ff, 0xd028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d030_94_test_ff, 0xd030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d038_94_test_ff, 0xd038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d039_94_test_ff, 0xd039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03a_94_test_ff, 0xd03a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03b_94_test_ff, 0xd03b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d03c_94_test_ff, 0xd03c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d040_94_test_ff, 0xd040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d048_94_test_ff, 0xd048, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d050_94_test_ff, 0xd050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d058_94_test_ff, 0xd058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d060_94_test_ff, 0xd060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d068_94_test_ff, 0xd068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d070_94_test_ff, 0xd070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d078_94_test_ff, 0xd078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d079_94_test_ff, 0xd079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07a_94_test_ff, 0xd07a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07b_94_test_ff, 0xd07b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d07c_94_test_ff, 0xd07c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d080_94_test_ff, 0xd080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d088_94_test_ff, 0xd088, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d090_94_test_ff, 0xd090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d098_94_test_ff, 0xd098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a0_94_test_ff, 0xd0a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a8_94_test_ff, 0xd0a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b0_94_test_ff, 0xd0b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0b8_94_test_ff, 0xd0b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b9_94_test_ff, 0xd0b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0ba_94_test_ff, 0xd0ba, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0bb_94_test_ff, 0xd0bb, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0bc_94_test_ff, 0xd0bc, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0c0_94_test_ff, 0xd0c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0c8_94_test_ff, 0xd0c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d0_94_test_ff, 0xd0d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d8_94_test_ff, 0xd0d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e0_94_test_ff, 0xd0e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e8_94_test_ff, 0xd0e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f0_94_test_ff, 0xd0f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f8_94_test_ff, 0xd0f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f9_94_test_ff, 0xd0f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fa_94_test_ff, 0xd0fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fb_94_test_ff, 0xd0fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fc_94_test_ff, 0xd0fc, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d100_94_test_ff, 0xd100, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d108_94_test_ff, 0xd108, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d110_94_test_ff, 0xd110, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d118_94_test_ff, 0xd118, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d120_94_test_ff, 0xd120, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d128_94_test_ff, 0xd128, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d130_94_test_ff, 0xd130, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d138_94_test_ff, 0xd138, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d139_94_test_ff, 0xd139, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d140_94_test_ff, 0xd140, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d148_94_test_ff, 0xd148, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d150_94_test_ff, 0xd150, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d158_94_test_ff, 0xd158, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d160_94_test_ff, 0xd160, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d168_94_test_ff, 0xd168, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d170_94_test_ff, 0xd170, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d178_94_test_ff, 0xd178, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d179_94_test_ff, 0xd179, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d180_94_test_ff, 0xd180, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d188_94_test_ff, 0xd188, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d190_94_test_ff, 0xd190, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d198_94_test_ff, 0xd198, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a0_94_test_ff, 0xd1a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a8_94_test_ff, 0xd1a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b0_94_test_ff, 0xd1b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d1b8_94_test_ff, 0xd1b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b9_94_test_ff, 0xd1b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1c0_94_test_ff, 0xd1c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1c8_94_test_ff, 0xd1c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d0_94_test_ff, 0xd1d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d8_94_test_ff, 0xd1d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e0_94_test_ff, 0xd1e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e8_94_test_ff, 0xd1e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f0_94_test_ff, 0xd1f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f8_94_test_ff, 0xd1f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f9_94_test_ff, 0xd1f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fa_94_test_ff, 0xd1fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fb_94_test_ff, 0xd1fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fc_94_test_ff, 0xd1fc, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_e000_94_test_ff, 0xe000, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e008_94_test_ff, 0xe008, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e010_94_test_ff, 0xe010, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e018_94_test_ff, 0xe018, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e020_94_test_ff, 0xe020, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e028_94_test_ff, 0xe028, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e030_94_test_ff, 0xe030, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e038_94_test_ff, 0xe038, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e040_94_test_ff, 0xe040, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e048_94_test_ff, 0xe048, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e050_94_test_ff, 0xe050, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e058_94_test_ff, 0xe058, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e060_94_test_ff, 0xe060, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e068_94_test_ff, 0xe068, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e070_94_test_ff, 0xe070, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e078_94_test_ff, 0xe078, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e080_94_test_ff, 0xe080, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e088_94_test_ff, 0xe088, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e090_94_test_ff, 0xe090, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e098_94_test_ff, 0xe098, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0a0_94_test_ff, 0xe0a0, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e0a8_94_test_ff, 0xe0a8, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e0b0_94_test_ff, 0xe0b0, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e0b8_94_test_ff, 0xe0b8, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0d0_94_test_ff, 0xe0d0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0d8_94_test_ff, 0xe0d8, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e0_94_test_ff, 0xe0e0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e8_94_test_ff, 0xe0e8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f0_94_test_ff, 0xe0f0, 2, { 2, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f8_94_test_ff, 0xe0f8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f9_94_test_ff, 0xe0f9, 6, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e100_94_test_ff, 0xe100, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e108_94_test_ff, 0xe108, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e110_94_test_ff, 0xe110, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e118_94_test_ff, 0xe118, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e120_94_test_ff, 0xe120, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e128_94_test_ff, 0xe128, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e130_94_test_ff, 0xe130, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e138_94_test_ff, 0xe138, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e140_94_test_ff, 0xe140, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e148_94_test_ff, 0xe148, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e150_94_test_ff, 0xe150, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e158_94_test_ff, 0xe158, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e160_94_test_ff, 0xe160, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e168_94_test_ff, 0xe168, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e170_94_test_ff, 0xe170, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e178_94_test_ff, 0xe178, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e180_94_test_ff, 0xe180, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e188_94_test_ff, 0xe188, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e190_94_test_ff, 0xe190, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e198_94_test_ff, 0xe198, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1a0_94_test_ff, 0xe1a0, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e1a8_94_test_ff, 0xe1a8, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e1b0_94_test_ff, 0xe1b0, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e1b8_94_test_ff, 0xe1b8, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1d0_94_test_ff, 0xe1d0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1d8_94_test_ff, 0xe1d8, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e0_94_test_ff, 0xe1e0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e8_94_test_ff, 0xe1e8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f0_94_test_ff, 0xe1f0, 2, { 2, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f8_94_test_ff, 0xe1f8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f9_94_test_ff, 0xe1f9, 6, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e2d0_94_test_ff, 0xe2d0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2d8_94_test_ff, 0xe2d8, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e0_94_test_ff, 0xe2e0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e8_94_test_ff, 0xe2e8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f0_94_test_ff, 0xe2f0, 2, { 2, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f8_94_test_ff, 0xe2f8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f9_94_test_ff, 0xe2f9, 6, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e3d0_94_test_ff, 0xe3d0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3d8_94_test_ff, 0xe3d8, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e0_94_test_ff, 0xe3e0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e8_94_test_ff, 0xe3e8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f0_94_test_ff, 0xe3f0, 2, { 2, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f8_94_test_ff, 0xe3f8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f9_94_test_ff, 0xe3f9, 6, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e4d0_94_test_ff, 0xe4d0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4d8_94_test_ff, 0xe4d8, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e0_94_test_ff, 0xe4e0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e8_94_test_ff, 0xe4e8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f0_94_test_ff, 0xe4f0, 2, { 2, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f8_94_test_ff, 0xe4f8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f9_94_test_ff, 0xe4f9, 6, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e5d0_94_test_ff, 0xe5d0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5d8_94_test_ff, 0xe5d8, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e0_94_test_ff, 0xe5e0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e8_94_test_ff, 0xe5e8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f0_94_test_ff, 0xe5f0, 2, { 2, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f8_94_test_ff, 0xe5f8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f9_94_test_ff, 0xe5f9, 6, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e6d0_94_test_ff, 0xe6d0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6d8_94_test_ff, 0xe6d8, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e0_94_test_ff, 0xe6e0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e8_94_test_ff, 0xe6e8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f0_94_test_ff, 0xe6f0, 2, { 2, 0 }, 0 }, /* RORW */
{ NULL, op_e6f8_94_test_ff, 0xe6f8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f9_94_test_ff, 0xe6f9, 6, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e7d0_94_test_ff, 0xe7d0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7d8_94_test_ff, 0xe7d8, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e0_94_test_ff, 0xe7e0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e8_94_test_ff, 0xe7e8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f0_94_test_ff, 0xe7f0, 2, { 2, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f8_94_test_ff, 0xe7f8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f9_94_test_ff, 0xe7f9, 6, { 0, 0 }, 0 }, /* ROLW */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8c0_94_test_ff, 0xe8c0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8d0_94_test_ff, 0xe8d0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8e8_94_test_ff, 0xe8e8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f0_94_test_ff, 0xe8f0, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f8_94_test_ff, 0xe8f8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f9_94_test_ff, 0xe8f9, 8, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fa_94_test_ff, 0xe8fa, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fb_94_test_ff, 0xe8fb, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9c0_94_test_ff, 0xe9c0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9d0_94_test_ff, 0xe9d0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9e8_94_test_ff, 0xe9e8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f0_94_test_ff, 0xe9f0, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f8_94_test_ff, 0xe9f8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f9_94_test_ff, 0xe9f9, 8, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fa_94_test_ff, 0xe9fa, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fb_94_test_ff, 0xe9fb, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eac0_94_test_ff, 0xeac0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ead0_94_test_ff, 0xead0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eae8_94_test_ff, 0xeae8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf0_94_test_ff, 0xeaf0, 4, { 2, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf8_94_test_ff, 0xeaf8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf9_94_test_ff, 0xeaf9, 8, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebc0_94_test_ff, 0xebc0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebd0_94_test_ff, 0xebd0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebe8_94_test_ff, 0xebe8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf0_94_test_ff, 0xebf0, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf8_94_test_ff, 0xebf8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf9_94_test_ff, 0xebf9, 8, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfa_94_test_ff, 0xebfa, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfb_94_test_ff, 0xebfb, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecc0_94_test_ff, 0xecc0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecd0_94_test_ff, 0xecd0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ece8_94_test_ff, 0xece8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf0_94_test_ff, 0xecf0, 4, { 2, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf8_94_test_ff, 0xecf8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf9_94_test_ff, 0xecf9, 8, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edc0_94_test_ff, 0xedc0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edd0_94_test_ff, 0xedd0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ede8_94_test_ff, 0xede8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf0_94_test_ff, 0xedf0, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf8_94_test_ff, 0xedf8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf9_94_test_ff, 0xedf9, 8, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfa_94_test_ff, 0xedfa, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfb_94_test_ff, 0xedfb, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eec0_94_test_ff, 0xeec0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eed0_94_test_ff, 0xeed0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eee8_94_test_ff, 0xeee8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef0_94_test_ff, 0xeef0, 4, { 2, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef8_94_test_ff, 0xeef8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef9_94_test_ff, 0xeef9, 8, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efc0_94_test_ff, 0xefc0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efd0_94_test_ff, 0xefd0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efe8_94_test_ff, 0xefe8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff0_94_test_ff, 0xeff0, 4, { 2, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff8_94_test_ff, 0xeff8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff9_94_test_ff, 0xeff9, 8, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f000_94_test_ff, 0xf000, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f008_94_test_ff, 0xf008, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f010_94_test_ff, 0xf010, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f018_94_test_ff, 0xf018, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f020_94_test_ff, 0xf020, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f028_94_test_ff, 0xf028, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f030_94_test_ff, 0xf030, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f038_94_test_ff, 0xf038, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f039_94_test_ff, 0xf039, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f200_94_test_ff, 0xf200, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f208_94_test_ff, 0xf208, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f210_94_test_ff, 0xf210, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f218_94_test_ff, 0xf218, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f220_94_test_ff, 0xf220, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f228_94_test_ff, 0xf228, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f230_94_test_ff, 0xf230, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f238_94_test_ff, 0xf238, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f239_94_test_ff, 0xf239, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23a_94_test_ff, 0xf23a, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23b_94_test_ff, 0xf23b, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23c_94_test_ff, 0xf23c, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f240_94_test_ff, 0xf240, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f248_94_test_ff, 0xf248, -1, { 0, 0 }, 0 }, /* FDBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f250_94_test_ff, 0xf250, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f258_94_test_ff, 0xf258, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f260_94_test_ff, 0xf260, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f268_94_test_ff, 0xf268, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f270_94_test_ff, 0xf270, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f278_94_test_ff, 0xf278, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f279_94_test_ff, 0xf279, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27a_94_test_ff, 0xf27a, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27b_94_test_ff, 0xf27b, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27c_94_test_ff, 0xf27c, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f280_94_test_ff, 0xf280, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f2c0_94_test_ff, 0xf2c0, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f310_94_test_ff, 0xf310, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f320_94_test_ff, 0xf320, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f328_94_test_ff, 0xf328, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f330_94_test_ff, 0xf330, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f338_94_test_ff, 0xf338, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f339_94_test_ff, 0xf339, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f350_94_test_ff, 0xf350, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f358_94_test_ff, 0xf358, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f368_94_test_ff, 0xf368, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f370_94_test_ff, 0xf370, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f378_94_test_ff, 0xf378, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f379_94_test_ff, 0xf379, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37a_94_test_ff, 0xf37a, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37b_94_test_ff, 0xf37b, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f408_94_test_ff, 0xf408, -1, { 0, 0 }, 0 }, /* CINVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f410_94_test_ff, 0xf410, -1, { 0, 0 }, 0 }, /* CINVP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f418_94_test_ff, 0xf418, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f419_94_test_ff, 0xf419, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41a_94_test_ff, 0xf41a, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41b_94_test_ff, 0xf41b, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41c_94_test_ff, 0xf41c, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41d_94_test_ff, 0xf41d, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41e_94_test_ff, 0xf41e, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41f_94_test_ff, 0xf41f, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f428_94_test_ff, 0xf428, -1, { 0, 0 }, 0 }, /* CPUSHL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f430_94_test_ff, 0xf430, -1, { 0, 0 }, 0 }, /* CPUSHP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f438_94_test_ff, 0xf438, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f439_94_test_ff, 0xf439, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43a_94_test_ff, 0xf43a, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43b_94_test_ff, 0xf43b, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43c_94_test_ff, 0xf43c, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43d_94_test_ff, 0xf43d, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43e_94_test_ff, 0xf43e, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43f_94_test_ff, 0xf43f, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f500_94_test_ff, 0xf500, -1, { 0, 0 }, 0 }, /* PFLUSHN */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f508_94_test_ff, 0xf508, -1, { 0, 0 }, 0 }, /* PFLUSH */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f510_94_test_ff, 0xf510, -1, { 0, 0 }, 0 }, /* PFLUSHAN */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f518_94_test_ff, 0xf518, -1, { 0, 0 }, 0 }, /* PFLUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f548_94_test_ff, 0xf548, -1, { 0, 0 }, 0 }, /* PTESTW */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f568_94_test_ff, 0xf568, -1, { 0, 0 }, 0 }, /* PTESTR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f600_94_test_ff, 0xf600, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f608_94_test_ff, 0xf608, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f610_94_test_ff, 0xf610, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f618_94_test_ff, 0xf618, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f620_94_test_ff, 0xf620, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
{ 0 }};
#endif /* CPUEMU_94 */
#ifdef CPUEMU_95
const struct cputbl CPUFUNC(op_smalltbl_95_test)[] = {
{ NULL, op_0000_95_test_ff, 0x0000, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0010_95_test_ff, 0x0010, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0018_95_test_ff, 0x0018, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0020_95_test_ff, 0x0020, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0028_95_test_ff, 0x0028, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0030_95_test_ff, 0x0030, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0038_95_test_ff, 0x0038, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0039_95_test_ff, 0x0039, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_003c_95_test_ff, 0x003c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0040_95_test_ff, 0x0040, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0050_95_test_ff, 0x0050, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0058_95_test_ff, 0x0058, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0060_95_test_ff, 0x0060, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0068_95_test_ff, 0x0068, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0070_95_test_ff, 0x0070, 4, { 2, 0 }, 0 }, /* OR */
{ NULL, op_0078_95_test_ff, 0x0078, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0079_95_test_ff, 0x0079, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_007c_95_test_ff, 0x007c, 4, { 0, 0 }, 0 }, /* ORSR */
{ NULL, op_0080_95_test_ff, 0x0080, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0090_95_test_ff, 0x0090, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_0098_95_test_ff, 0x0098, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a0_95_test_ff, 0x00a0, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00a8_95_test_ff, 0x00a8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b0_95_test_ff, 0x00b0, 6, { 2, 0 }, 0 }, /* OR */
{ NULL, op_00b8_95_test_ff, 0x00b8, 8, { 0, 0 }, 0 }, /* OR */
{ NULL, op_00b9_95_test_ff, 0x00b9, 10, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00d0_95_test_ff, 0x00d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00e8_95_test_ff, 0x00e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f0_95_test_ff, 0x00f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f8_95_test_ff, 0x00f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00f9_95_test_ff, 0x00f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fa_95_test_ff, 0x00fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_00fb_95_test_ff, 0x00fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0100_95_test_ff, 0x0100, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0108_95_test_ff, 0x0108, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0110_95_test_ff, 0x0110, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0118_95_test_ff, 0x0118, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0120_95_test_ff, 0x0120, 2, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0128_95_test_ff, 0x0128, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0130_95_test_ff, 0x0130, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0138_95_test_ff, 0x0138, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0139_95_test_ff, 0x0139, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013a_95_test_ff, 0x013a, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_013b_95_test_ff, 0x013b, 2, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_013c_95_test_ff, 0x013c, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0140_95_test_ff, 0x0140, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0148_95_test_ff, 0x0148, 4, { 0, 0 }, 0 }, /* MVPMR */
{ NULL, op_0150_95_test_ff, 0x0150, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0158_95_test_ff, 0x0158, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0160_95_test_ff, 0x0160, 2, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0168_95_test_ff, 0x0168, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0170_95_test_ff, 0x0170, 2, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0178_95_test_ff, 0x0178, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0179_95_test_ff, 0x0179, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0180_95_test_ff, 0x0180, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0188_95_test_ff, 0x0188, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_0190_95_test_ff, 0x0190, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0198_95_test_ff, 0x0198, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a0_95_test_ff, 0x01a0, 2, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01a8_95_test_ff, 0x01a8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b0_95_test_ff, 0x01b0, 2, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_01b8_95_test_ff, 0x01b8, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01b9_95_test_ff, 0x01b9, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_01c0_95_test_ff, 0x01c0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01c8_95_test_ff, 0x01c8, 4, { 0, 0 }, 0 }, /* MVPRM */
{ NULL, op_01d0_95_test_ff, 0x01d0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01d8_95_test_ff, 0x01d8, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e0_95_test_ff, 0x01e0, 2, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01e8_95_test_ff, 0x01e8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f0_95_test_ff, 0x01f0, 2, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_01f8_95_test_ff, 0x01f8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_01f9_95_test_ff, 0x01f9, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0200_95_test_ff, 0x0200, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0210_95_test_ff, 0x0210, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0218_95_test_ff, 0x0218, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0220_95_test_ff, 0x0220, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0228_95_test_ff, 0x0228, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0230_95_test_ff, 0x0230, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0238_95_test_ff, 0x0238, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0239_95_test_ff, 0x0239, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_023c_95_test_ff, 0x023c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0240_95_test_ff, 0x0240, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0250_95_test_ff, 0x0250, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0258_95_test_ff, 0x0258, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0260_95_test_ff, 0x0260, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0268_95_test_ff, 0x0268, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0270_95_test_ff, 0x0270, 4, { 2, 0 }, 0 }, /* AND */
{ NULL, op_0278_95_test_ff, 0x0278, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0279_95_test_ff, 0x0279, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_027c_95_test_ff, 0x027c, 4, { 0, 0 }, 0 }, /* ANDSR */
{ NULL, op_0280_95_test_ff, 0x0280, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0290_95_test_ff, 0x0290, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_0298_95_test_ff, 0x0298, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a0_95_test_ff, 0x02a0, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02a8_95_test_ff, 0x02a8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b0_95_test_ff, 0x02b0, 6, { 2, 0 }, 0 }, /* AND */
{ NULL, op_02b8_95_test_ff, 0x02b8, 8, { 0, 0 }, 0 }, /* AND */
{ NULL, op_02b9_95_test_ff, 0x02b9, 10, { 0, 0 }, 0 }, /* AND */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02d0_95_test_ff, 0x02d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02e8_95_test_ff, 0x02e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f0_95_test_ff, 0x02f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f8_95_test_ff, 0x02f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02f9_95_test_ff, 0x02f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fa_95_test_ff, 0x02fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_02fb_95_test_ff, 0x02fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0400_95_test_ff, 0x0400, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0410_95_test_ff, 0x0410, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0418_95_test_ff, 0x0418, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0420_95_test_ff, 0x0420, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0428_95_test_ff, 0x0428, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0430_95_test_ff, 0x0430, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0438_95_test_ff, 0x0438, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0439_95_test_ff, 0x0439, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0440_95_test_ff, 0x0440, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0450_95_test_ff, 0x0450, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0458_95_test_ff, 0x0458, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0460_95_test_ff, 0x0460, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0468_95_test_ff, 0x0468, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0470_95_test_ff, 0x0470, 4, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_0478_95_test_ff, 0x0478, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0479_95_test_ff, 0x0479, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0480_95_test_ff, 0x0480, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0490_95_test_ff, 0x0490, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_0498_95_test_ff, 0x0498, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a0_95_test_ff, 0x04a0, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04a8_95_test_ff, 0x04a8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b0_95_test_ff, 0x04b0, 6, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_04b8_95_test_ff, 0x04b8, 8, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_04b9_95_test_ff, 0x04b9, 10, { 0, 0 }, 0 }, /* SUB */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04d0_95_test_ff, 0x04d0, 4, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04e8_95_test_ff, 0x04e8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f0_95_test_ff, 0x04f0, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f8_95_test_ff, 0x04f8, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04f9_95_test_ff, 0x04f9, 8, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fa_95_test_ff, 0x04fa, 6, { 0, 0 }, 0 }, /* CHK2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_04fb_95_test_ff, 0x04fb, 4, { 2, 0 }, 0 }, /* CHK2 */
#endif
{ NULL, op_0600_95_test_ff, 0x0600, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0610_95_test_ff, 0x0610, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0618_95_test_ff, 0x0618, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0620_95_test_ff, 0x0620, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0628_95_test_ff, 0x0628, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0630_95_test_ff, 0x0630, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0638_95_test_ff, 0x0638, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0639_95_test_ff, 0x0639, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0640_95_test_ff, 0x0640, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0650_95_test_ff, 0x0650, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0658_95_test_ff, 0x0658, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0660_95_test_ff, 0x0660, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0668_95_test_ff, 0x0668, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0670_95_test_ff, 0x0670, 4, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_0678_95_test_ff, 0x0678, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0679_95_test_ff, 0x0679, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0680_95_test_ff, 0x0680, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0690_95_test_ff, 0x0690, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_0698_95_test_ff, 0x0698, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a0_95_test_ff, 0x06a0, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06a8_95_test_ff, 0x06a8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b0_95_test_ff, 0x06b0, 6, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_06b8_95_test_ff, 0x06b8, 8, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_06b9_95_test_ff, 0x06b9, 10, { 0, 0 }, 0 }, /* ADD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c0_95_test_ff, 0x06c0, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06c8_95_test_ff, 0x06c8, 2, { 0, 0 }, 0 }, /* RTM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06d0_95_test_ff, 0x06d0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06e8_95_test_ff, 0x06e8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f0_95_test_ff, 0x06f0, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f8_95_test_ff, 0x06f8, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06f9_95_test_ff, 0x06f9, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fa_95_test_ff, 0x06fa, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_06fb_95_test_ff, 0x06fb, 2, { 0, 0 }, 0 }, /* CALLM */
#endif
{ NULL, op_0800_95_test_ff, 0x0800, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0810_95_test_ff, 0x0810, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0818_95_test_ff, 0x0818, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0820_95_test_ff, 0x0820, 4, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0828_95_test_ff, 0x0828, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0830_95_test_ff, 0x0830, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0838_95_test_ff, 0x0838, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_0839_95_test_ff, 0x0839, 8, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083a_95_test_ff, 0x083a, 6, { 0, 0 }, 0 }, /* BTST */
{ NULL, op_083b_95_test_ff, 0x083b, 4, { 2, 0 }, 0 }, /* BTST */
{ NULL, op_0840_95_test_ff, 0x0840, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0850_95_test_ff, 0x0850, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0858_95_test_ff, 0x0858, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0860_95_test_ff, 0x0860, 4, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0868_95_test_ff, 0x0868, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0870_95_test_ff, 0x0870, 4, { 2, 0 }, 0 }, /* BCHG */
{ NULL, op_0878_95_test_ff, 0x0878, 6, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0879_95_test_ff, 0x0879, 8, { 0, 0 }, 0 }, /* BCHG */
{ NULL, op_0880_95_test_ff, 0x0880, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0890_95_test_ff, 0x0890, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_0898_95_test_ff, 0x0898, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a0_95_test_ff, 0x08a0, 4, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08a8_95_test_ff, 0x08a8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b0_95_test_ff, 0x08b0, 4, { 2, 0 }, 0 }, /* BCLR */
{ NULL, op_08b8_95_test_ff, 0x08b8, 6, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08b9_95_test_ff, 0x08b9, 8, { 0, 0 }, 0 }, /* BCLR */
{ NULL, op_08c0_95_test_ff, 0x08c0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d0_95_test_ff, 0x08d0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08d8_95_test_ff, 0x08d8, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e0_95_test_ff, 0x08e0, 4, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08e8_95_test_ff, 0x08e8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f0_95_test_ff, 0x08f0, 4, { 2, 0 }, 0 }, /* BSET */
{ NULL, op_08f8_95_test_ff, 0x08f8, 6, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_08f9_95_test_ff, 0x08f9, 8, { 0, 0 }, 0 }, /* BSET */
{ NULL, op_0a00_95_test_ff, 0x0a00, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a10_95_test_ff, 0x0a10, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a18_95_test_ff, 0x0a18, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a20_95_test_ff, 0x0a20, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a28_95_test_ff, 0x0a28, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a30_95_test_ff, 0x0a30, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a38_95_test_ff, 0x0a38, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a39_95_test_ff, 0x0a39, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a3c_95_test_ff, 0x0a3c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a40_95_test_ff, 0x0a40, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a50_95_test_ff, 0x0a50, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a58_95_test_ff, 0x0a58, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a60_95_test_ff, 0x0a60, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a68_95_test_ff, 0x0a68, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a70_95_test_ff, 0x0a70, 4, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0a78_95_test_ff, 0x0a78, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a79_95_test_ff, 0x0a79, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a7c_95_test_ff, 0x0a7c, 4, { 0, 0 }, 0 }, /* EORSR */
{ NULL, op_0a80_95_test_ff, 0x0a80, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a90_95_test_ff, 0x0a90, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0a98_95_test_ff, 0x0a98, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa0_95_test_ff, 0x0aa0, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0aa8_95_test_ff, 0x0aa8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab0_95_test_ff, 0x0ab0, 6, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_0ab8_95_test_ff, 0x0ab8, 8, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_0ab9_95_test_ff, 0x0ab9, 10, { 0, 0 }, 0 }, /* EOR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad0_95_test_ff, 0x0ad0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ad8_95_test_ff, 0x0ad8, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae0_95_test_ff, 0x0ae0, 4, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ae8_95_test_ff, 0x0ae8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af0_95_test_ff, 0x0af0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af8_95_test_ff, 0x0af8, 6, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0af9_95_test_ff, 0x0af9, 8, { 0, 0 }, 0 }, /* CAS */
#endif
{ NULL, op_0c00_95_test_ff, 0x0c00, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c10_95_test_ff, 0x0c10, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c18_95_test_ff, 0x0c18, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c20_95_test_ff, 0x0c20, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c28_95_test_ff, 0x0c28, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c30_95_test_ff, 0x0c30, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c38_95_test_ff, 0x0c38, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c39_95_test_ff, 0x0c39, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3a_95_test_ff, 0x0c3a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c3b_95_test_ff, 0x0c3b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c40_95_test_ff, 0x0c40, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c50_95_test_ff, 0x0c50, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c58_95_test_ff, 0x0c58, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c60_95_test_ff, 0x0c60, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c68_95_test_ff, 0x0c68, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c70_95_test_ff, 0x0c70, 4, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0c78_95_test_ff, 0x0c78, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c79_95_test_ff, 0x0c79, 8, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7a_95_test_ff, 0x0c7a, 6, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0c7b_95_test_ff, 0x0c7b, 4, { 2, 0 }, 0 }, /* CMP */
#endif
{ NULL, op_0c80_95_test_ff, 0x0c80, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c90_95_test_ff, 0x0c90, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0c98_95_test_ff, 0x0c98, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca0_95_test_ff, 0x0ca0, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0ca8_95_test_ff, 0x0ca8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb0_95_test_ff, 0x0cb0, 6, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_0cb8_95_test_ff, 0x0cb8, 8, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_0cb9_95_test_ff, 0x0cb9, 10, { 0, 0 }, 0 }, /* CMP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cba_95_test_ff, 0x0cba, 8, { 0, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cbb_95_test_ff, 0x0cbb, 6, { 2, 0 }, 0 }, /* CMP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd0_95_test_ff, 0x0cd0, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cd8_95_test_ff, 0x0cd8, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce0_95_test_ff, 0x0ce0, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ce8_95_test_ff, 0x0ce8, 12, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf0_95_test_ff, 0x0cf0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf8_95_test_ff, 0x0cf8, 12, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cf9_95_test_ff, 0x0cf9, 16, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0cfc_95_test_ff, 0x0cfc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e10_95_test_ff, 0x0e10, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e18_95_test_ff, 0x0e18, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e20_95_test_ff, 0x0e20, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e28_95_test_ff, 0x0e28, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e30_95_test_ff, 0x0e30, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e38_95_test_ff, 0x0e38, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e39_95_test_ff, 0x0e39, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e50_95_test_ff, 0x0e50, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e58_95_test_ff, 0x0e58, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e60_95_test_ff, 0x0e60, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e68_95_test_ff, 0x0e68, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e70_95_test_ff, 0x0e70, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e78_95_test_ff, 0x0e78, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e79_95_test_ff, 0x0e79, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e90_95_test_ff, 0x0e90, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0e98_95_test_ff, 0x0e98, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea0_95_test_ff, 0x0ea0, 4, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ea8_95_test_ff, 0x0ea8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb0_95_test_ff, 0x0eb0, 4, { 2, 2 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb8_95_test_ff, 0x0eb8, 6, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0eb9_95_test_ff, 0x0eb9, 8, { 0, 0 }, 0 }, /* MOVES */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed0_95_test_ff, 0x0ed0, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ed8_95_test_ff, 0x0ed8, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee0_95_test_ff, 0x0ee0, 8, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ee8_95_test_ff, 0x0ee8, 12, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef0_95_test_ff, 0x0ef0, 4, { 2, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef8_95_test_ff, 0x0ef8, 12, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0ef9_95_test_ff, 0x0ef9, 16, { 0, 0 }, 0 }, /* CAS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_0efc_95_test_ff, 0x0efc, 6, { 0, 0 }, 0 }, /* CAS2 */
#endif
{ NULL, op_1000_95_test_ff, 0x1000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1010_95_test_ff, 0x1010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1018_95_test_ff, 0x1018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1020_95_test_ff, 0x1020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1028_95_test_ff, 0x1028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1030_95_test_ff, 0x1030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1038_95_test_ff, 0x1038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1039_95_test_ff, 0x1039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103a_95_test_ff, 0x103a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_103b_95_test_ff, 0x103b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_103c_95_test_ff, 0x103c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1080_95_test_ff, 0x1080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1090_95_test_ff, 0x1090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1098_95_test_ff, 0x1098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a0_95_test_ff, 0x10a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10a8_95_test_ff, 0x10a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b0_95_test_ff, 0x10b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10b8_95_test_ff, 0x10b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10b9_95_test_ff, 0x10b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10ba_95_test_ff, 0x10ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10bb_95_test_ff, 0x10bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10bc_95_test_ff, 0x10bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10c0_95_test_ff, 0x10c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d0_95_test_ff, 0x10d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10d8_95_test_ff, 0x10d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e0_95_test_ff, 0x10e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10e8_95_test_ff, 0x10e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f0_95_test_ff, 0x10f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10f8_95_test_ff, 0x10f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10f9_95_test_ff, 0x10f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fa_95_test_ff, 0x10fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_10fb_95_test_ff, 0x10fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_10fc_95_test_ff, 0x10fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1100_95_test_ff, 0x1100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1110_95_test_ff, 0x1110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1118_95_test_ff, 0x1118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1120_95_test_ff, 0x1120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1128_95_test_ff, 0x1128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1130_95_test_ff, 0x1130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1138_95_test_ff, 0x1138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1139_95_test_ff, 0x1139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113a_95_test_ff, 0x113a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_113b_95_test_ff, 0x113b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_113c_95_test_ff, 0x113c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1140_95_test_ff, 0x1140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1150_95_test_ff, 0x1150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1158_95_test_ff, 0x1158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1160_95_test_ff, 0x1160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1168_95_test_ff, 0x1168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1170_95_test_ff, 0x1170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_1178_95_test_ff, 0x1178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1179_95_test_ff, 0x1179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117a_95_test_ff, 0x117a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_117b_95_test_ff, 0x117b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_117c_95_test_ff, 0x117c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_1180_95_test_ff, 0x1180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1190_95_test_ff, 0x1190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_1198_95_test_ff, 0x1198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a0_95_test_ff, 0x11a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11a8_95_test_ff, 0x11a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b0_95_test_ff, 0x11b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11b8_95_test_ff, 0x11b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11b9_95_test_ff, 0x11b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11ba_95_test_ff, 0x11ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11bb_95_test_ff, 0x11bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_11bc_95_test_ff, 0x11bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_11c0_95_test_ff, 0x11c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d0_95_test_ff, 0x11d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11d8_95_test_ff, 0x11d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e0_95_test_ff, 0x11e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11e8_95_test_ff, 0x11e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f0_95_test_ff, 0x11f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11f8_95_test_ff, 0x11f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11f9_95_test_ff, 0x11f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fa_95_test_ff, 0x11fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_11fb_95_test_ff, 0x11fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_11fc_95_test_ff, 0x11fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13c0_95_test_ff, 0x13c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d0_95_test_ff, 0x13d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13d8_95_test_ff, 0x13d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e0_95_test_ff, 0x13e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13e8_95_test_ff, 0x13e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f0_95_test_ff, 0x13f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13f8_95_test_ff, 0x13f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13f9_95_test_ff, 0x13f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fa_95_test_ff, 0x13fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_13fb_95_test_ff, 0x13fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_13fc_95_test_ff, 0x13fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2000_95_test_ff, 0x2000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2008_95_test_ff, 0x2008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2010_95_test_ff, 0x2010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2018_95_test_ff, 0x2018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2020_95_test_ff, 0x2020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2028_95_test_ff, 0x2028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2030_95_test_ff, 0x2030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2038_95_test_ff, 0x2038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2039_95_test_ff, 0x2039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203a_95_test_ff, 0x203a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_203b_95_test_ff, 0x203b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_203c_95_test_ff, 0x203c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2040_95_test_ff, 0x2040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2048_95_test_ff, 0x2048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2050_95_test_ff, 0x2050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2058_95_test_ff, 0x2058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2060_95_test_ff, 0x2060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2068_95_test_ff, 0x2068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2070_95_test_ff, 0x2070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_2078_95_test_ff, 0x2078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2079_95_test_ff, 0x2079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207a_95_test_ff, 0x207a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_207b_95_test_ff, 0x207b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_207c_95_test_ff, 0x207c, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_2080_95_test_ff, 0x2080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2088_95_test_ff, 0x2088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2090_95_test_ff, 0x2090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2098_95_test_ff, 0x2098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a0_95_test_ff, 0x20a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20a8_95_test_ff, 0x20a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b0_95_test_ff, 0x20b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20b8_95_test_ff, 0x20b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20b9_95_test_ff, 0x20b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20ba_95_test_ff, 0x20ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20bb_95_test_ff, 0x20bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20bc_95_test_ff, 0x20bc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c0_95_test_ff, 0x20c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20c8_95_test_ff, 0x20c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d0_95_test_ff, 0x20d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20d8_95_test_ff, 0x20d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e0_95_test_ff, 0x20e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20e8_95_test_ff, 0x20e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f0_95_test_ff, 0x20f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20f8_95_test_ff, 0x20f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20f9_95_test_ff, 0x20f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fa_95_test_ff, 0x20fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_20fb_95_test_ff, 0x20fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_20fc_95_test_ff, 0x20fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2100_95_test_ff, 0x2100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2108_95_test_ff, 0x2108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2110_95_test_ff, 0x2110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2118_95_test_ff, 0x2118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2120_95_test_ff, 0x2120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2128_95_test_ff, 0x2128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2130_95_test_ff, 0x2130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2138_95_test_ff, 0x2138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2139_95_test_ff, 0x2139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213a_95_test_ff, 0x213a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_213b_95_test_ff, 0x213b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_213c_95_test_ff, 0x213c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2140_95_test_ff, 0x2140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2148_95_test_ff, 0x2148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2150_95_test_ff, 0x2150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2158_95_test_ff, 0x2158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2160_95_test_ff, 0x2160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2168_95_test_ff, 0x2168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2170_95_test_ff, 0x2170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_2178_95_test_ff, 0x2178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2179_95_test_ff, 0x2179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217a_95_test_ff, 0x217a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_217b_95_test_ff, 0x217b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_217c_95_test_ff, 0x217c, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_2180_95_test_ff, 0x2180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2188_95_test_ff, 0x2188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2190_95_test_ff, 0x2190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_2198_95_test_ff, 0x2198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a0_95_test_ff, 0x21a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21a8_95_test_ff, 0x21a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b0_95_test_ff, 0x21b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21b8_95_test_ff, 0x21b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21b9_95_test_ff, 0x21b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21ba_95_test_ff, 0x21ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21bb_95_test_ff, 0x21bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_21bc_95_test_ff, 0x21bc, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_21c0_95_test_ff, 0x21c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21c8_95_test_ff, 0x21c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d0_95_test_ff, 0x21d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21d8_95_test_ff, 0x21d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e0_95_test_ff, 0x21e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21e8_95_test_ff, 0x21e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f0_95_test_ff, 0x21f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21f8_95_test_ff, 0x21f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21f9_95_test_ff, 0x21f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fa_95_test_ff, 0x21fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_21fb_95_test_ff, 0x21fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_21fc_95_test_ff, 0x21fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c0_95_test_ff, 0x23c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23c8_95_test_ff, 0x23c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d0_95_test_ff, 0x23d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23d8_95_test_ff, 0x23d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e0_95_test_ff, 0x23e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23e8_95_test_ff, 0x23e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f0_95_test_ff, 0x23f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23f8_95_test_ff, 0x23f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23f9_95_test_ff, 0x23f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fa_95_test_ff, 0x23fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_23fb_95_test_ff, 0x23fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_23fc_95_test_ff, 0x23fc, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3000_95_test_ff, 0x3000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3008_95_test_ff, 0x3008, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3010_95_test_ff, 0x3010, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3018_95_test_ff, 0x3018, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3020_95_test_ff, 0x3020, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3028_95_test_ff, 0x3028, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3030_95_test_ff, 0x3030, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3038_95_test_ff, 0x3038, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3039_95_test_ff, 0x3039, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303a_95_test_ff, 0x303a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_303b_95_test_ff, 0x303b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_303c_95_test_ff, 0x303c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3040_95_test_ff, 0x3040, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3048_95_test_ff, 0x3048, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3050_95_test_ff, 0x3050, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3058_95_test_ff, 0x3058, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3060_95_test_ff, 0x3060, 2, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3068_95_test_ff, 0x3068, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3070_95_test_ff, 0x3070, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_3078_95_test_ff, 0x3078, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3079_95_test_ff, 0x3079, 6, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307a_95_test_ff, 0x307a, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_307b_95_test_ff, 0x307b, 2, { 2, 0 }, 0 }, /* MOVEA */
{ NULL, op_307c_95_test_ff, 0x307c, 4, { 0, 0 }, 0 }, /* MOVEA */
{ NULL, op_3080_95_test_ff, 0x3080, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3088_95_test_ff, 0x3088, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3090_95_test_ff, 0x3090, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3098_95_test_ff, 0x3098, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a0_95_test_ff, 0x30a0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30a8_95_test_ff, 0x30a8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b0_95_test_ff, 0x30b0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30b8_95_test_ff, 0x30b8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30b9_95_test_ff, 0x30b9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30ba_95_test_ff, 0x30ba, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30bb_95_test_ff, 0x30bb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30bc_95_test_ff, 0x30bc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c0_95_test_ff, 0x30c0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30c8_95_test_ff, 0x30c8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d0_95_test_ff, 0x30d0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30d8_95_test_ff, 0x30d8, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e0_95_test_ff, 0x30e0, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30e8_95_test_ff, 0x30e8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f0_95_test_ff, 0x30f0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30f8_95_test_ff, 0x30f8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30f9_95_test_ff, 0x30f9, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fa_95_test_ff, 0x30fa, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_30fb_95_test_ff, 0x30fb, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_30fc_95_test_ff, 0x30fc, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3100_95_test_ff, 0x3100, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3108_95_test_ff, 0x3108, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3110_95_test_ff, 0x3110, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3118_95_test_ff, 0x3118, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3120_95_test_ff, 0x3120, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3128_95_test_ff, 0x3128, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3130_95_test_ff, 0x3130, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3138_95_test_ff, 0x3138, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3139_95_test_ff, 0x3139, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313a_95_test_ff, 0x313a, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_313b_95_test_ff, 0x313b, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_313c_95_test_ff, 0x313c, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3140_95_test_ff, 0x3140, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3148_95_test_ff, 0x3148, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3150_95_test_ff, 0x3150, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3158_95_test_ff, 0x3158, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3160_95_test_ff, 0x3160, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3168_95_test_ff, 0x3168, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3170_95_test_ff, 0x3170, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_3178_95_test_ff, 0x3178, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3179_95_test_ff, 0x3179, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317a_95_test_ff, 0x317a, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_317b_95_test_ff, 0x317b, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_317c_95_test_ff, 0x317c, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_3180_95_test_ff, 0x3180, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3188_95_test_ff, 0x3188, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3190_95_test_ff, 0x3190, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_3198_95_test_ff, 0x3198, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a0_95_test_ff, 0x31a0, 2, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31a8_95_test_ff, 0x31a8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b0_95_test_ff, 0x31b0, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31b8_95_test_ff, 0x31b8, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31b9_95_test_ff, 0x31b9, 6, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31ba_95_test_ff, 0x31ba, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31bb_95_test_ff, 0x31bb, 2, { 2, 2 }, 0 }, /* MOVE */
{ NULL, op_31bc_95_test_ff, 0x31bc, 4, { 2, 0 }, 0 }, /* MOVE */
{ NULL, op_31c0_95_test_ff, 0x31c0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31c8_95_test_ff, 0x31c8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d0_95_test_ff, 0x31d0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31d8_95_test_ff, 0x31d8, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e0_95_test_ff, 0x31e0, 4, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31e8_95_test_ff, 0x31e8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f0_95_test_ff, 0x31f0, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31f8_95_test_ff, 0x31f8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31f9_95_test_ff, 0x31f9, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fa_95_test_ff, 0x31fa, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_31fb_95_test_ff, 0x31fb, 4, { 4, 0 }, 0 }, /* MOVE */
{ NULL, op_31fc_95_test_ff, 0x31fc, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c0_95_test_ff, 0x33c0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33c8_95_test_ff, 0x33c8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d0_95_test_ff, 0x33d0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33d8_95_test_ff, 0x33d8, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e0_95_test_ff, 0x33e0, 6, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33e8_95_test_ff, 0x33e8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f0_95_test_ff, 0x33f0, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33f8_95_test_ff, 0x33f8, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33f9_95_test_ff, 0x33f9, 10, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fa_95_test_ff, 0x33fa, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_33fb_95_test_ff, 0x33fb, 6, { 6, 0 }, 0 }, /* MOVE */
{ NULL, op_33fc_95_test_ff, 0x33fc, 8, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_4000_95_test_ff, 0x4000, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4010_95_test_ff, 0x4010, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4018_95_test_ff, 0x4018, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4020_95_test_ff, 0x4020, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4028_95_test_ff, 0x4028, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4030_95_test_ff, 0x4030, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4038_95_test_ff, 0x4038, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4039_95_test_ff, 0x4039, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4040_95_test_ff, 0x4040, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4050_95_test_ff, 0x4050, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4058_95_test_ff, 0x4058, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4060_95_test_ff, 0x4060, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4068_95_test_ff, 0x4068, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4070_95_test_ff, 0x4070, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_4078_95_test_ff, 0x4078, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4079_95_test_ff, 0x4079, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4080_95_test_ff, 0x4080, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4090_95_test_ff, 0x4090, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_4098_95_test_ff, 0x4098, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a0_95_test_ff, 0x40a0, 2, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40a8_95_test_ff, 0x40a8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b0_95_test_ff, 0x40b0, 2, { 2, 0 }, 0 }, /* NEGX */
{ NULL, op_40b8_95_test_ff, 0x40b8, 4, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40b9_95_test_ff, 0x40b9, 6, { 0, 0 }, 0 }, /* NEGX */
{ NULL, op_40c0_95_test_ff, 0x40c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d0_95_test_ff, 0x40d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40d8_95_test_ff, 0x40d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e0_95_test_ff, 0x40e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40e8_95_test_ff, 0x40e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f0_95_test_ff, 0x40f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f8_95_test_ff, 0x40f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
{ NULL, op_40f9_95_test_ff, 0x40f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4100_95_test_ff, 0x4100, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4110_95_test_ff, 0x4110, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4118_95_test_ff, 0x4118, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4120_95_test_ff, 0x4120, 2, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4128_95_test_ff, 0x4128, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4130_95_test_ff, 0x4130, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4138_95_test_ff, 0x4138, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4139_95_test_ff, 0x4139, 6, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413a_95_test_ff, 0x413a, 4, { 0, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413b_95_test_ff, 0x413b, 2, { 2, 0 }, 0 }, /* CHK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_413c_95_test_ff, 0x413c, 6, { 0, 0 }, 0 }, /* CHK */
#endif
{ NULL, op_4180_95_test_ff, 0x4180, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4190_95_test_ff, 0x4190, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_4198_95_test_ff, 0x4198, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a0_95_test_ff, 0x41a0, 2, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41a8_95_test_ff, 0x41a8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b0_95_test_ff, 0x41b0, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41b8_95_test_ff, 0x41b8, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41b9_95_test_ff, 0x41b9, 6, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41ba_95_test_ff, 0x41ba, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41bb_95_test_ff, 0x41bb, 2, { 2, 0 }, 0 }, /* CHK */
{ NULL, op_41bc_95_test_ff, 0x41bc, 4, { 0, 0 }, 0 }, /* CHK */
{ NULL, op_41d0_95_test_ff, 0x41d0, 2, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41e8_95_test_ff, 0x41e8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f0_95_test_ff, 0x41f0, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_41f8_95_test_ff, 0x41f8, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41f9_95_test_ff, 0x41f9, 6, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fa_95_test_ff, 0x41fa, 4, { 0, 0 }, 0 }, /* LEA */
{ NULL, op_41fb_95_test_ff, 0x41fb, 2, { 2, 0 }, 0 }, /* LEA */
{ NULL, op_4200_95_test_ff, 0x4200, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4210_95_test_ff, 0x4210, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4218_95_test_ff, 0x4218, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4220_95_test_ff, 0x4220, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4228_95_test_ff, 0x4228, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4230_95_test_ff, 0x4230, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4238_95_test_ff, 0x4238, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4239_95_test_ff, 0x4239, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4240_95_test_ff, 0x4240, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4250_95_test_ff, 0x4250, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4258_95_test_ff, 0x4258, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4260_95_test_ff, 0x4260, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4268_95_test_ff, 0x4268, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4270_95_test_ff, 0x4270, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_4278_95_test_ff, 0x4278, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4279_95_test_ff, 0x4279, 6, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4280_95_test_ff, 0x4280, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4290_95_test_ff, 0x4290, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_4298_95_test_ff, 0x4298, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a0_95_test_ff, 0x42a0, 2, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42a8_95_test_ff, 0x42a8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b0_95_test_ff, 0x42b0, 2, { 2, 0 }, 0 }, /* CLR */
{ NULL, op_42b8_95_test_ff, 0x42b8, 4, { 0, 0 }, 0 }, /* CLR */
{ NULL, op_42b9_95_test_ff, 0x42b9, 6, { 0, 0 }, 0 }, /* CLR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42c0_95_test_ff, 0x42c0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d0_95_test_ff, 0x42d0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42d8_95_test_ff, 0x42d8, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e0_95_test_ff, 0x42e0, 2, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42e8_95_test_ff, 0x42e8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f0_95_test_ff, 0x42f0, 2, { 2, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f8_95_test_ff, 0x42f8, 4, { 0, 0 }, 0 }, /* MVSR2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_42f9_95_test_ff, 0x42f9, 6, { 0, 0 }, 0 }, /* MVSR2 */
#endif
{ NULL, op_4400_95_test_ff, 0x4400, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4410_95_test_ff, 0x4410, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4418_95_test_ff, 0x4418, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4420_95_test_ff, 0x4420, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4428_95_test_ff, 0x4428, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4430_95_test_ff, 0x4430, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4438_95_test_ff, 0x4438, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4439_95_test_ff, 0x4439, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4440_95_test_ff, 0x4440, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4450_95_test_ff, 0x4450, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4458_95_test_ff, 0x4458, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4460_95_test_ff, 0x4460, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4468_95_test_ff, 0x4468, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4470_95_test_ff, 0x4470, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_4478_95_test_ff, 0x4478, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4479_95_test_ff, 0x4479, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4480_95_test_ff, 0x4480, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4490_95_test_ff, 0x4490, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_4498_95_test_ff, 0x4498, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a0_95_test_ff, 0x44a0, 2, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44a8_95_test_ff, 0x44a8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b0_95_test_ff, 0x44b0, 2, { 2, 0 }, 0 }, /* NEG */
{ NULL, op_44b8_95_test_ff, 0x44b8, 4, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44b9_95_test_ff, 0x44b9, 6, { 0, 0 }, 0 }, /* NEG */
{ NULL, op_44c0_95_test_ff, 0x44c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d0_95_test_ff, 0x44d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44d8_95_test_ff, 0x44d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e0_95_test_ff, 0x44e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44e8_95_test_ff, 0x44e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f0_95_test_ff, 0x44f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f8_95_test_ff, 0x44f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44f9_95_test_ff, 0x44f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fa_95_test_ff, 0x44fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fb_95_test_ff, 0x44fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_44fc_95_test_ff, 0x44fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4600_95_test_ff, 0x4600, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4610_95_test_ff, 0x4610, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4618_95_test_ff, 0x4618, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4620_95_test_ff, 0x4620, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4628_95_test_ff, 0x4628, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4630_95_test_ff, 0x4630, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4638_95_test_ff, 0x4638, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4639_95_test_ff, 0x4639, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4640_95_test_ff, 0x4640, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4650_95_test_ff, 0x4650, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4658_95_test_ff, 0x4658, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4660_95_test_ff, 0x4660, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4668_95_test_ff, 0x4668, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4670_95_test_ff, 0x4670, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_4678_95_test_ff, 0x4678, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4679_95_test_ff, 0x4679, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4680_95_test_ff, 0x4680, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4690_95_test_ff, 0x4690, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_4698_95_test_ff, 0x4698, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a0_95_test_ff, 0x46a0, 2, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46a8_95_test_ff, 0x46a8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b0_95_test_ff, 0x46b0, 2, { 2, 0 }, 0 }, /* NOT */
{ NULL, op_46b8_95_test_ff, 0x46b8, 4, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46b9_95_test_ff, 0x46b9, 6, { 0, 0 }, 0 }, /* NOT */
{ NULL, op_46c0_95_test_ff, 0x46c0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d0_95_test_ff, 0x46d0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46d8_95_test_ff, 0x46d8, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e0_95_test_ff, 0x46e0, 2, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46e8_95_test_ff, 0x46e8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f0_95_test_ff, 0x46f0, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f8_95_test_ff, 0x46f8, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46f9_95_test_ff, 0x46f9, 6, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fa_95_test_ff, 0x46fa, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fb_95_test_ff, 0x46fb, 2, { 2, 0 }, 0 }, /* MV2SR */
{ NULL, op_46fc_95_test_ff, 0x46fc, 4, { 0, 0 }, 0 }, /* MV2SR */
{ NULL, op_4800_95_test_ff, 0x4800, 2, { 0, 0 }, 0 }, /* NBCD */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4808_95_test_ff, 0x4808, 6, { 0, 0 }, 0 }, /* LINK */
#endif
{ NULL, op_4810_95_test_ff, 0x4810, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4818_95_test_ff, 0x4818, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4820_95_test_ff, 0x4820, 2, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4828_95_test_ff, 0x4828, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4830_95_test_ff, 0x4830, 2, { 2, 0 }, 0 }, /* NBCD */
{ NULL, op_4838_95_test_ff, 0x4838, 4, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4839_95_test_ff, 0x4839, 6, { 0, 0 }, 0 }, /* NBCD */
{ NULL, op_4840_95_test_ff, 0x4840, 2, { 0, 0 }, 0 }, /* SWAP */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4848_95_test_ff, 0x4848, 2, { 0, 0 }, 0 }, /* BKPT */
#endif
{ NULL, op_4850_95_test_ff, 0x4850, 2, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4868_95_test_ff, 0x4868, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4870_95_test_ff, 0x4870, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4878_95_test_ff, 0x4878, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_4879_95_test_ff, 0x4879, 6, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487a_95_test_ff, 0x487a, 4, { 0, 0 }, 0 }, /* PEA */
{ NULL, op_487b_95_test_ff, 0x487b, 2, { 2, 0 }, 0 }, /* PEA */
{ NULL, op_4880_95_test_ff, 0x4880, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_4890_95_test_ff, 0x4890, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a0_95_test_ff, 0x48a0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48a8_95_test_ff, 0x48a8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b0_95_test_ff, 0x48b0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b8_95_test_ff, 0x48b8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48b9_95_test_ff, 0x48b9, 8, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48c0_95_test_ff, 0x48c0, 2, { 0, 0 }, 0 }, /* EXT */
{ NULL, op_48d0_95_test_ff, 0x48d0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e0_95_test_ff, 0x48e0, 4, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48e8_95_test_ff, 0x48e8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f0_95_test_ff, 0x48f0, 4, { 2, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f8_95_test_ff, 0x48f8, 6, { 0, 0 }, 0 }, /* MVMLE */
{ NULL, op_48f9_95_test_ff, 0x48f9, 8, { 0, 0 }, 0 }, /* MVMLE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_49c0_95_test_ff, 0x49c0, 2, { 0, 0 }, 0 }, /* EXT */
#endif
{ NULL, op_4a00_95_test_ff, 0x4a00, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a10_95_test_ff, 0x4a10, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a18_95_test_ff, 0x4a18, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a20_95_test_ff, 0x4a20, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a28_95_test_ff, 0x4a28, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a30_95_test_ff, 0x4a30, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a38_95_test_ff, 0x4a38, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a39_95_test_ff, 0x4a39, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3a_95_test_ff, 0x4a3a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3b_95_test_ff, 0x4a3b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a3c_95_test_ff, 0x4a3c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a40_95_test_ff, 0x4a40, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a48_95_test_ff, 0x4a48, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a50_95_test_ff, 0x4a50, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a58_95_test_ff, 0x4a58, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a60_95_test_ff, 0x4a60, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a68_95_test_ff, 0x4a68, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a70_95_test_ff, 0x4a70, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4a78_95_test_ff, 0x4a78, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a79_95_test_ff, 0x4a79, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7a_95_test_ff, 0x4a7a, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7b_95_test_ff, 0x4a7b, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a7c_95_test_ff, 0x4a7c, 4, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a80_95_test_ff, 0x4a80, 2, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4a88_95_test_ff, 0x4a88, 2, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4a90_95_test_ff, 0x4a90, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4a98_95_test_ff, 0x4a98, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa0_95_test_ff, 0x4aa0, 2, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4aa8_95_test_ff, 0x4aa8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab0_95_test_ff, 0x4ab0, 2, { 2, 0 }, 0 }, /* TST */
{ NULL, op_4ab8_95_test_ff, 0x4ab8, 4, { 0, 0 }, 0 }, /* TST */
{ NULL, op_4ab9_95_test_ff, 0x4ab9, 6, { 0, 0 }, 0 }, /* TST */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4aba_95_test_ff, 0x4aba, 4, { 0, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abb_95_test_ff, 0x4abb, 2, { 2, 0 }, 0 }, /* TST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4abc_95_test_ff, 0x4abc, 6, { 0, 0 }, 0 }, /* TST */
#endif
{ NULL, op_4ac0_95_test_ff, 0x4ac0, 2, { 0, 0 }, 0 }, /* TAS */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4ac8_95_test_ff, 0x4ac8, 2, { 0, 0 }, 0 }, /* HALT */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4acc_95_test_ff, 0x4acc, 2, { 0, 0 }, 0 }, /* PULSE */
#endif
{ NULL, op_4ad0_95_test_ff, 0x4ad0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ad8_95_test_ff, 0x4ad8, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae0_95_test_ff, 0x4ae0, 2, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4ae8_95_test_ff, 0x4ae8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af0_95_test_ff, 0x4af0, 2, { 2, 0 }, 0 }, /* TAS */
{ NULL, op_4af8_95_test_ff, 0x4af8, 4, { 0, 0 }, 0 }, /* TAS */
{ NULL, op_4af9_95_test_ff, 0x4af9, 6, { 0, 0 }, 0 }, /* TAS */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c00_95_test_ff, 0x4c00, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c10_95_test_ff, 0x4c10, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c18_95_test_ff, 0x4c18, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c20_95_test_ff, 0x4c20, 4, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c28_95_test_ff, 0x4c28, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c30_95_test_ff, 0x4c30, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c38_95_test_ff, 0x4c38, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c39_95_test_ff, 0x4c39, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3a_95_test_ff, 0x4c3a, 6, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3b_95_test_ff, 0x4c3b, 4, { 2, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c3c_95_test_ff, 0x4c3c, 8, { 0, 0 }, 0 }, /* MULL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c40_95_test_ff, 0x4c40, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c50_95_test_ff, 0x4c50, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c58_95_test_ff, 0x4c58, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c60_95_test_ff, 0x4c60, 4, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c68_95_test_ff, 0x4c68, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c70_95_test_ff, 0x4c70, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c78_95_test_ff, 0x4c78, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c79_95_test_ff, 0x4c79, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7a_95_test_ff, 0x4c7a, 6, { 0, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7b_95_test_ff, 0x4c7b, 4, { 2, 0 }, 0 }, /* DIVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4c7c_95_test_ff, 0x4c7c, 8, { 0, 0 }, 0 }, /* DIVL */
#endif
{ NULL, op_4c90_95_test_ff, 0x4c90, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4c98_95_test_ff, 0x4c98, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ca8_95_test_ff, 0x4ca8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb0_95_test_ff, 0x4cb0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb8_95_test_ff, 0x4cb8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cb9_95_test_ff, 0x4cb9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cba_95_test_ff, 0x4cba, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cbb_95_test_ff, 0x4cbb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd0_95_test_ff, 0x4cd0, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cd8_95_test_ff, 0x4cd8, 4, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4ce8_95_test_ff, 0x4ce8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf0_95_test_ff, 0x4cf0, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf8_95_test_ff, 0x4cf8, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cf9_95_test_ff, 0x4cf9, 8, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfa_95_test_ff, 0x4cfa, 6, { 0, 0 }, 0 }, /* MVMEL */
{ NULL, op_4cfb_95_test_ff, 0x4cfb, 4, { 2, 0 }, 0 }, /* MVMEL */
{ NULL, op_4e40_95_test_ff, 0x4e40, 2, { 0, 0 }, 0 }, /* TRAP */
{ NULL, op_4e50_95_test_ff, 0x4e50, 4, { 0, 0 }, 0 }, /* LINK */
{ NULL, op_4e58_95_test_ff, 0x4e58, 2, { 0, 0 }, 0 }, /* UNLK */
{ NULL, op_4e60_95_test_ff, 0x4e60, 2, { 0, 0 }, 0 }, /* MVR2USP */
{ NULL, op_4e68_95_test_ff, 0x4e68, 2, { 0, 0 }, 0 }, /* MVUSP2R */
{ NULL, op_4e70_95_test_ff, 0x4e70, 2, { 0, 0 }, 0 }, /* RESET */
{ NULL, op_4e71_95_test_ff, 0x4e71, 2, { 0, 0 }, 0 }, /* NOP */
{ NULL, op_4e72_95_test_ff, 0x4e72, 0, { 0, 0 }, 0 }, /* STOP */
{ NULL, op_4e73_95_test_ff, 0x4e73, 2, { 0, 0 }, 1 }, /* RTE */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e74_95_test_ff, 0x4e74, 4, { 0, 0 }, 2 }, /* RTD */
#endif
{ NULL, op_4e75_95_test_ff, 0x4e75, 2, { 0, 0 }, 1 }, /* RTS */
{ NULL, op_4e76_95_test_ff, 0x4e76, 2, { 0, 0 }, 0 }, /* TRAPV */
{ NULL, op_4e77_95_test_ff, 0x4e77, 2, { 0, 0 }, 1 }, /* RTR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7a_95_test_ff, 0x4e7a, 4, { 0, 0 }, 0 }, /* MOVEC2 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_4e7b_95_test_ff, 0x4e7b, 4, { 0, 0 }, 0 }, /* MOVE2C */
#endif
{ NULL, op_4e90_95_test_ff, 0x4e90, 2, { 0, 0 }, 1 }, /* JSR */
{ NULL, op_4ea8_95_test_ff, 0x4ea8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb0_95_test_ff, 0x4eb0, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4eb8_95_test_ff, 0x4eb8, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4eb9_95_test_ff, 0x4eb9, 6, { 0, 0 }, 3 }, /* JSR */
{ NULL, op_4eba_95_test_ff, 0x4eba, 4, { 0, 0 }, 2 }, /* JSR */
{ NULL, op_4ebb_95_test_ff, 0x4ebb, 2, { 2, 0 }, 1 }, /* JSR */
{ NULL, op_4ed0_95_test_ff, 0x4ed0, 2, { 0, 0 }, 1 }, /* JMP */
{ NULL, op_4ee8_95_test_ff, 0x4ee8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef0_95_test_ff, 0x4ef0, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_4ef8_95_test_ff, 0x4ef8, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4ef9_95_test_ff, 0x4ef9, 6, { 0, 0 }, 3 }, /* JMP */
{ NULL, op_4efa_95_test_ff, 0x4efa, 4, { 0, 0 }, 2 }, /* JMP */
{ NULL, op_4efb_95_test_ff, 0x4efb, 2, { 2, 0 }, 1 }, /* JMP */
{ NULL, op_5000_95_test_ff, 0x5000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5010_95_test_ff, 0x5010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5018_95_test_ff, 0x5018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5020_95_test_ff, 0x5020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5028_95_test_ff, 0x5028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5030_95_test_ff, 0x5030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5038_95_test_ff, 0x5038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5039_95_test_ff, 0x5039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5040_95_test_ff, 0x5040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5048_95_test_ff, 0x5048, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5050_95_test_ff, 0x5050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5058_95_test_ff, 0x5058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5060_95_test_ff, 0x5060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5068_95_test_ff, 0x5068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5070_95_test_ff, 0x5070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_5078_95_test_ff, 0x5078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5079_95_test_ff, 0x5079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5080_95_test_ff, 0x5080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5088_95_test_ff, 0x5088, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_5090_95_test_ff, 0x5090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_5098_95_test_ff, 0x5098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a0_95_test_ff, 0x50a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50a8_95_test_ff, 0x50a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b0_95_test_ff, 0x50b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_50b8_95_test_ff, 0x50b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50b9_95_test_ff, 0x50b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_50c0_95_test_ff, 0x50c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50c8_95_test_ff, 0x50c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_50d0_95_test_ff, 0x50d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50d8_95_test_ff, 0x50d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e0_95_test_ff, 0x50e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50e8_95_test_ff, 0x50e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f0_95_test_ff, 0x50f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_50f8_95_test_ff, 0x50f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_50f9_95_test_ff, 0x50f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fa_95_test_ff, 0x50fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fb_95_test_ff, 0x50fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_50fc_95_test_ff, 0x50fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5100_95_test_ff, 0x5100, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5110_95_test_ff, 0x5110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5118_95_test_ff, 0x5118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5120_95_test_ff, 0x5120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5128_95_test_ff, 0x5128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5130_95_test_ff, 0x5130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5138_95_test_ff, 0x5138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5139_95_test_ff, 0x5139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5140_95_test_ff, 0x5140, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5148_95_test_ff, 0x5148, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5150_95_test_ff, 0x5150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5158_95_test_ff, 0x5158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5160_95_test_ff, 0x5160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5168_95_test_ff, 0x5168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5170_95_test_ff, 0x5170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_5178_95_test_ff, 0x5178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5179_95_test_ff, 0x5179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5180_95_test_ff, 0x5180, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5188_95_test_ff, 0x5188, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_5190_95_test_ff, 0x5190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_5198_95_test_ff, 0x5198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a0_95_test_ff, 0x51a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51a8_95_test_ff, 0x51a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b0_95_test_ff, 0x51b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_51b8_95_test_ff, 0x51b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51b9_95_test_ff, 0x51b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_51c0_95_test_ff, 0x51c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51c8_95_test_ff, 0x51c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_51d0_95_test_ff, 0x51d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51d8_95_test_ff, 0x51d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e0_95_test_ff, 0x51e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51e8_95_test_ff, 0x51e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f0_95_test_ff, 0x51f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_51f8_95_test_ff, 0x51f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_51f9_95_test_ff, 0x51f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fa_95_test_ff, 0x51fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fb_95_test_ff, 0x51fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_51fc_95_test_ff, 0x51fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_52c0_95_test_ff, 0x52c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52c8_95_test_ff, 0x52c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_52d0_95_test_ff, 0x52d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52d8_95_test_ff, 0x52d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e0_95_test_ff, 0x52e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52e8_95_test_ff, 0x52e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f0_95_test_ff, 0x52f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_52f8_95_test_ff, 0x52f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_52f9_95_test_ff, 0x52f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fa_95_test_ff, 0x52fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fb_95_test_ff, 0x52fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_52fc_95_test_ff, 0x52fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_53c0_95_test_ff, 0x53c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53c8_95_test_ff, 0x53c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_53d0_95_test_ff, 0x53d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53d8_95_test_ff, 0x53d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e0_95_test_ff, 0x53e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53e8_95_test_ff, 0x53e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f0_95_test_ff, 0x53f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_53f8_95_test_ff, 0x53f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_53f9_95_test_ff, 0x53f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fa_95_test_ff, 0x53fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fb_95_test_ff, 0x53fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_53fc_95_test_ff, 0x53fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_54c0_95_test_ff, 0x54c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54c8_95_test_ff, 0x54c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_54d0_95_test_ff, 0x54d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54d8_95_test_ff, 0x54d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e0_95_test_ff, 0x54e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54e8_95_test_ff, 0x54e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f0_95_test_ff, 0x54f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_54f8_95_test_ff, 0x54f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_54f9_95_test_ff, 0x54f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fa_95_test_ff, 0x54fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fb_95_test_ff, 0x54fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_54fc_95_test_ff, 0x54fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_55c0_95_test_ff, 0x55c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55c8_95_test_ff, 0x55c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_55d0_95_test_ff, 0x55d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55d8_95_test_ff, 0x55d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e0_95_test_ff, 0x55e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55e8_95_test_ff, 0x55e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f0_95_test_ff, 0x55f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_55f8_95_test_ff, 0x55f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_55f9_95_test_ff, 0x55f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fa_95_test_ff, 0x55fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fb_95_test_ff, 0x55fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_55fc_95_test_ff, 0x55fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_56c0_95_test_ff, 0x56c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56c8_95_test_ff, 0x56c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_56d0_95_test_ff, 0x56d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56d8_95_test_ff, 0x56d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e0_95_test_ff, 0x56e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56e8_95_test_ff, 0x56e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f0_95_test_ff, 0x56f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_56f8_95_test_ff, 0x56f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_56f9_95_test_ff, 0x56f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fa_95_test_ff, 0x56fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fb_95_test_ff, 0x56fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_56fc_95_test_ff, 0x56fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_57c0_95_test_ff, 0x57c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57c8_95_test_ff, 0x57c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_57d0_95_test_ff, 0x57d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57d8_95_test_ff, 0x57d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e0_95_test_ff, 0x57e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57e8_95_test_ff, 0x57e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f0_95_test_ff, 0x57f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_57f8_95_test_ff, 0x57f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_57f9_95_test_ff, 0x57f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fa_95_test_ff, 0x57fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fb_95_test_ff, 0x57fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_57fc_95_test_ff, 0x57fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_58c0_95_test_ff, 0x58c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58c8_95_test_ff, 0x58c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_58d0_95_test_ff, 0x58d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58d8_95_test_ff, 0x58d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e0_95_test_ff, 0x58e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58e8_95_test_ff, 0x58e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f0_95_test_ff, 0x58f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_58f8_95_test_ff, 0x58f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_58f9_95_test_ff, 0x58f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fa_95_test_ff, 0x58fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fb_95_test_ff, 0x58fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_58fc_95_test_ff, 0x58fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_59c0_95_test_ff, 0x59c0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59c8_95_test_ff, 0x59c8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_59d0_95_test_ff, 0x59d0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59d8_95_test_ff, 0x59d8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e0_95_test_ff, 0x59e0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59e8_95_test_ff, 0x59e8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f0_95_test_ff, 0x59f0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_59f8_95_test_ff, 0x59f8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_59f9_95_test_ff, 0x59f9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fa_95_test_ff, 0x59fa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fb_95_test_ff, 0x59fb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_59fc_95_test_ff, 0x59fc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ac0_95_test_ff, 0x5ac0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ac8_95_test_ff, 0x5ac8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ad0_95_test_ff, 0x5ad0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ad8_95_test_ff, 0x5ad8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae0_95_test_ff, 0x5ae0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ae8_95_test_ff, 0x5ae8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af0_95_test_ff, 0x5af0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5af8_95_test_ff, 0x5af8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5af9_95_test_ff, 0x5af9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afa_95_test_ff, 0x5afa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afb_95_test_ff, 0x5afb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5afc_95_test_ff, 0x5afc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5bc0_95_test_ff, 0x5bc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bc8_95_test_ff, 0x5bc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5bd0_95_test_ff, 0x5bd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bd8_95_test_ff, 0x5bd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be0_95_test_ff, 0x5be0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5be8_95_test_ff, 0x5be8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf0_95_test_ff, 0x5bf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5bf8_95_test_ff, 0x5bf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5bf9_95_test_ff, 0x5bf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfa_95_test_ff, 0x5bfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfb_95_test_ff, 0x5bfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5bfc_95_test_ff, 0x5bfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5cc0_95_test_ff, 0x5cc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cc8_95_test_ff, 0x5cc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5cd0_95_test_ff, 0x5cd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cd8_95_test_ff, 0x5cd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce0_95_test_ff, 0x5ce0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ce8_95_test_ff, 0x5ce8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf0_95_test_ff, 0x5cf0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5cf8_95_test_ff, 0x5cf8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5cf9_95_test_ff, 0x5cf9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfa_95_test_ff, 0x5cfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfb_95_test_ff, 0x5cfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5cfc_95_test_ff, 0x5cfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5dc0_95_test_ff, 0x5dc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dc8_95_test_ff, 0x5dc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5dd0_95_test_ff, 0x5dd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5dd8_95_test_ff, 0x5dd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de0_95_test_ff, 0x5de0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5de8_95_test_ff, 0x5de8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df0_95_test_ff, 0x5df0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5df8_95_test_ff, 0x5df8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5df9_95_test_ff, 0x5df9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfa_95_test_ff, 0x5dfa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfb_95_test_ff, 0x5dfb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5dfc_95_test_ff, 0x5dfc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5ec0_95_test_ff, 0x5ec0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ec8_95_test_ff, 0x5ec8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5ed0_95_test_ff, 0x5ed0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ed8_95_test_ff, 0x5ed8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee0_95_test_ff, 0x5ee0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ee8_95_test_ff, 0x5ee8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef0_95_test_ff, 0x5ef0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ef8_95_test_ff, 0x5ef8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ef9_95_test_ff, 0x5ef9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efa_95_test_ff, 0x5efa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efb_95_test_ff, 0x5efb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5efc_95_test_ff, 0x5efc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_5fc0_95_test_ff, 0x5fc0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fc8_95_test_ff, 0x5fc8, 4, { 0, 0 }, -2 }, /* DBcc */
{ NULL, op_5fd0_95_test_ff, 0x5fd0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fd8_95_test_ff, 0x5fd8, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe0_95_test_ff, 0x5fe0, 2, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5fe8_95_test_ff, 0x5fe8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff0_95_test_ff, 0x5ff0, 2, { 2, 0 }, 0 }, /* Scc */
{ NULL, op_5ff8_95_test_ff, 0x5ff8, 4, { 0, 0 }, 0 }, /* Scc */
{ NULL, op_5ff9_95_test_ff, 0x5ff9, 6, { 0, 0 }, 0 }, /* Scc */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffa_95_test_ff, 0x5ffa, 4, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffb_95_test_ff, 0x5ffb, 6, { 0, 0 }, 0 }, /* TRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_5ffc_95_test_ff, 0x5ffc, 2, { 0, 0 }, 0 }, /* TRAPcc */
#endif
{ NULL, op_6000_95_test_ff, 0x6000, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6001_95_test_ff, 0x6001, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_60ff_95_test_ff, 0x60ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6100_95_test_ff, 0x6100, 4, { 0, 0 }, 2 }, /* BSR */
{ NULL, op_6101_95_test_ff, 0x6101, 2, { 0, 0 }, 1 }, /* BSR */
{ NULL, op_61ff_95_test_ff, 0x61ff, 6, { 0, 0 }, 3 }, /* BSR */
{ NULL, op_6200_95_test_ff, 0x6200, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6201_95_test_ff, 0x6201, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_62ff_95_test_ff, 0x62ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6300_95_test_ff, 0x6300, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6301_95_test_ff, 0x6301, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_63ff_95_test_ff, 0x63ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6400_95_test_ff, 0x6400, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6401_95_test_ff, 0x6401, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_64ff_95_test_ff, 0x64ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6500_95_test_ff, 0x6500, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6501_95_test_ff, 0x6501, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_65ff_95_test_ff, 0x65ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6600_95_test_ff, 0x6600, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6601_95_test_ff, 0x6601, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_66ff_95_test_ff, 0x66ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6700_95_test_ff, 0x6700, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6701_95_test_ff, 0x6701, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_67ff_95_test_ff, 0x67ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6800_95_test_ff, 0x6800, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6801_95_test_ff, 0x6801, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_68ff_95_test_ff, 0x68ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6900_95_test_ff, 0x6900, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6901_95_test_ff, 0x6901, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_69ff_95_test_ff, 0x69ff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6a00_95_test_ff, 0x6a00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6a01_95_test_ff, 0x6a01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6aff_95_test_ff, 0x6aff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6b00_95_test_ff, 0x6b00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6b01_95_test_ff, 0x6b01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6bff_95_test_ff, 0x6bff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6c00_95_test_ff, 0x6c00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6c01_95_test_ff, 0x6c01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6cff_95_test_ff, 0x6cff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6d00_95_test_ff, 0x6d00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6d01_95_test_ff, 0x6d01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6dff_95_test_ff, 0x6dff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6e00_95_test_ff, 0x6e00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6e01_95_test_ff, 0x6e01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6eff_95_test_ff, 0x6eff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_6f00_95_test_ff, 0x6f00, 4, { 0, 0 }, -2 }, /* Bcc */
{ NULL, op_6f01_95_test_ff, 0x6f01, 2, { 0, 0 }, -1 }, /* Bcc */
{ NULL, op_6fff_95_test_ff, 0x6fff, 6, { 0, 0 }, -3 }, /* Bcc */
{ NULL, op_7000_95_test_ff, 0x7000, 2, { 0, 0 }, 0 }, /* MOVE */
{ NULL, op_8000_95_test_ff, 0x8000, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8010_95_test_ff, 0x8010, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8018_95_test_ff, 0x8018, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8020_95_test_ff, 0x8020, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8028_95_test_ff, 0x8028, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8030_95_test_ff, 0x8030, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8038_95_test_ff, 0x8038, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8039_95_test_ff, 0x8039, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803a_95_test_ff, 0x803a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_803b_95_test_ff, 0x803b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_803c_95_test_ff, 0x803c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8040_95_test_ff, 0x8040, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8050_95_test_ff, 0x8050, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8058_95_test_ff, 0x8058, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8060_95_test_ff, 0x8060, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8068_95_test_ff, 0x8068, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8070_95_test_ff, 0x8070, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8078_95_test_ff, 0x8078, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8079_95_test_ff, 0x8079, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807a_95_test_ff, 0x807a, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_807b_95_test_ff, 0x807b, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_807c_95_test_ff, 0x807c, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8080_95_test_ff, 0x8080, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8090_95_test_ff, 0x8090, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8098_95_test_ff, 0x8098, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a0_95_test_ff, 0x80a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80a8_95_test_ff, 0x80a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b0_95_test_ff, 0x80b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80b8_95_test_ff, 0x80b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80b9_95_test_ff, 0x80b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80ba_95_test_ff, 0x80ba, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80bb_95_test_ff, 0x80bb, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_80bc_95_test_ff, 0x80bc, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_80c0_95_test_ff, 0x80c0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d0_95_test_ff, 0x80d0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80d8_95_test_ff, 0x80d8, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e0_95_test_ff, 0x80e0, 2, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80e8_95_test_ff, 0x80e8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f0_95_test_ff, 0x80f0, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80f8_95_test_ff, 0x80f8, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80f9_95_test_ff, 0x80f9, 6, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fa_95_test_ff, 0x80fa, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_80fb_95_test_ff, 0x80fb, 2, { 2, 0 }, 0 }, /* DIVU */
{ NULL, op_80fc_95_test_ff, 0x80fc, 4, { 0, 0 }, 0 }, /* DIVU */
{ NULL, op_8100_95_test_ff, 0x8100, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8108_95_test_ff, 0x8108, 2, { 0, 0 }, 0 }, /* SBCD */
{ NULL, op_8110_95_test_ff, 0x8110, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8118_95_test_ff, 0x8118, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8120_95_test_ff, 0x8120, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8128_95_test_ff, 0x8128, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8130_95_test_ff, 0x8130, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8138_95_test_ff, 0x8138, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8139_95_test_ff, 0x8139, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8140_95_test_ff, 0x8140, 4, { 0, 0 }, 0 }, /* PACK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8148_95_test_ff, 0x8148, 4, { 0, 0 }, 0 }, /* PACK */
#endif
{ NULL, op_8150_95_test_ff, 0x8150, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8158_95_test_ff, 0x8158, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8160_95_test_ff, 0x8160, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8168_95_test_ff, 0x8168, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8170_95_test_ff, 0x8170, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_8178_95_test_ff, 0x8178, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8179_95_test_ff, 0x8179, 6, { 0, 0 }, 0 }, /* OR */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8180_95_test_ff, 0x8180, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_8188_95_test_ff, 0x8188, 4, { 0, 0 }, 0 }, /* UNPK */
#endif
{ NULL, op_8190_95_test_ff, 0x8190, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_8198_95_test_ff, 0x8198, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a0_95_test_ff, 0x81a0, 2, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81a8_95_test_ff, 0x81a8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b0_95_test_ff, 0x81b0, 2, { 2, 0 }, 0 }, /* OR */
{ NULL, op_81b8_95_test_ff, 0x81b8, 4, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81b9_95_test_ff, 0x81b9, 6, { 0, 0 }, 0 }, /* OR */
{ NULL, op_81c0_95_test_ff, 0x81c0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d0_95_test_ff, 0x81d0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81d8_95_test_ff, 0x81d8, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e0_95_test_ff, 0x81e0, 2, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81e8_95_test_ff, 0x81e8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f0_95_test_ff, 0x81f0, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81f8_95_test_ff, 0x81f8, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81f9_95_test_ff, 0x81f9, 6, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fa_95_test_ff, 0x81fa, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_81fb_95_test_ff, 0x81fb, 2, { 2, 0 }, 0 }, /* DIVS */
{ NULL, op_81fc_95_test_ff, 0x81fc, 4, { 0, 0 }, 0 }, /* DIVS */
{ NULL, op_9000_95_test_ff, 0x9000, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9010_95_test_ff, 0x9010, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9018_95_test_ff, 0x9018, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9020_95_test_ff, 0x9020, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9028_95_test_ff, 0x9028, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9030_95_test_ff, 0x9030, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9038_95_test_ff, 0x9038, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9039_95_test_ff, 0x9039, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903a_95_test_ff, 0x903a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_903b_95_test_ff, 0x903b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_903c_95_test_ff, 0x903c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9040_95_test_ff, 0x9040, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9048_95_test_ff, 0x9048, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9050_95_test_ff, 0x9050, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9058_95_test_ff, 0x9058, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9060_95_test_ff, 0x9060, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9068_95_test_ff, 0x9068, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9070_95_test_ff, 0x9070, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9078_95_test_ff, 0x9078, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9079_95_test_ff, 0x9079, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907a_95_test_ff, 0x907a, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_907b_95_test_ff, 0x907b, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_907c_95_test_ff, 0x907c, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9080_95_test_ff, 0x9080, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9088_95_test_ff, 0x9088, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9090_95_test_ff, 0x9090, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9098_95_test_ff, 0x9098, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a0_95_test_ff, 0x90a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90a8_95_test_ff, 0x90a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b0_95_test_ff, 0x90b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90b8_95_test_ff, 0x90b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90b9_95_test_ff, 0x90b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90ba_95_test_ff, 0x90ba, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90bb_95_test_ff, 0x90bb, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_90bc_95_test_ff, 0x90bc, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_90c0_95_test_ff, 0x90c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90c8_95_test_ff, 0x90c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d0_95_test_ff, 0x90d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90d8_95_test_ff, 0x90d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e0_95_test_ff, 0x90e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90e8_95_test_ff, 0x90e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f0_95_test_ff, 0x90f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90f8_95_test_ff, 0x90f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90f9_95_test_ff, 0x90f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fa_95_test_ff, 0x90fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_90fb_95_test_ff, 0x90fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_90fc_95_test_ff, 0x90fc, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_9100_95_test_ff, 0x9100, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9108_95_test_ff, 0x9108, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9110_95_test_ff, 0x9110, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9118_95_test_ff, 0x9118, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9120_95_test_ff, 0x9120, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9128_95_test_ff, 0x9128, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9130_95_test_ff, 0x9130, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9138_95_test_ff, 0x9138, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9139_95_test_ff, 0x9139, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9140_95_test_ff, 0x9140, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9148_95_test_ff, 0x9148, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9150_95_test_ff, 0x9150, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9158_95_test_ff, 0x9158, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9160_95_test_ff, 0x9160, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9168_95_test_ff, 0x9168, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9170_95_test_ff, 0x9170, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_9178_95_test_ff, 0x9178, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9179_95_test_ff, 0x9179, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9180_95_test_ff, 0x9180, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9188_95_test_ff, 0x9188, 2, { 0, 0 }, 0 }, /* SUBX */
{ NULL, op_9190_95_test_ff, 0x9190, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_9198_95_test_ff, 0x9198, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a0_95_test_ff, 0x91a0, 2, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91a8_95_test_ff, 0x91a8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b0_95_test_ff, 0x91b0, 2, { 2, 0 }, 0 }, /* SUB */
{ NULL, op_91b8_95_test_ff, 0x91b8, 4, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91b9_95_test_ff, 0x91b9, 6, { 0, 0 }, 0 }, /* SUB */
{ NULL, op_91c0_95_test_ff, 0x91c0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91c8_95_test_ff, 0x91c8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d0_95_test_ff, 0x91d0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91d8_95_test_ff, 0x91d8, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e0_95_test_ff, 0x91e0, 2, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91e8_95_test_ff, 0x91e8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f0_95_test_ff, 0x91f0, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91f8_95_test_ff, 0x91f8, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91f9_95_test_ff, 0x91f9, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fa_95_test_ff, 0x91fa, 4, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_91fb_95_test_ff, 0x91fb, 2, { 2, 0 }, 0 }, /* SUBA */
{ NULL, op_91fc_95_test_ff, 0x91fc, 6, { 0, 0 }, 0 }, /* SUBA */
{ NULL, op_b000_95_test_ff, 0xb000, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b010_95_test_ff, 0xb010, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b018_95_test_ff, 0xb018, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b020_95_test_ff, 0xb020, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b028_95_test_ff, 0xb028, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b030_95_test_ff, 0xb030, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b038_95_test_ff, 0xb038, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b039_95_test_ff, 0xb039, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03a_95_test_ff, 0xb03a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b03b_95_test_ff, 0xb03b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b03c_95_test_ff, 0xb03c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b040_95_test_ff, 0xb040, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b048_95_test_ff, 0xb048, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b050_95_test_ff, 0xb050, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b058_95_test_ff, 0xb058, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b060_95_test_ff, 0xb060, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b068_95_test_ff, 0xb068, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b070_95_test_ff, 0xb070, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b078_95_test_ff, 0xb078, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b079_95_test_ff, 0xb079, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07a_95_test_ff, 0xb07a, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b07b_95_test_ff, 0xb07b, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b07c_95_test_ff, 0xb07c, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b080_95_test_ff, 0xb080, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b088_95_test_ff, 0xb088, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b090_95_test_ff, 0xb090, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b098_95_test_ff, 0xb098, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a0_95_test_ff, 0xb0a0, 2, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0a8_95_test_ff, 0xb0a8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b0_95_test_ff, 0xb0b0, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0b8_95_test_ff, 0xb0b8, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0b9_95_test_ff, 0xb0b9, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0ba_95_test_ff, 0xb0ba, 4, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0bb_95_test_ff, 0xb0bb, 2, { 2, 0 }, 0 }, /* CMP */
{ NULL, op_b0bc_95_test_ff, 0xb0bc, 6, { 0, 0 }, 0 }, /* CMP */
{ NULL, op_b0c0_95_test_ff, 0xb0c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0c8_95_test_ff, 0xb0c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d0_95_test_ff, 0xb0d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0d8_95_test_ff, 0xb0d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e0_95_test_ff, 0xb0e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0e8_95_test_ff, 0xb0e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f0_95_test_ff, 0xb0f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f8_95_test_ff, 0xb0f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0f9_95_test_ff, 0xb0f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fa_95_test_ff, 0xb0fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fb_95_test_ff, 0xb0fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b0fc_95_test_ff, 0xb0fc, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b100_95_test_ff, 0xb100, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b108_95_test_ff, 0xb108, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b110_95_test_ff, 0xb110, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b118_95_test_ff, 0xb118, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b120_95_test_ff, 0xb120, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b128_95_test_ff, 0xb128, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b130_95_test_ff, 0xb130, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b138_95_test_ff, 0xb138, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b139_95_test_ff, 0xb139, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b140_95_test_ff, 0xb140, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b148_95_test_ff, 0xb148, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b150_95_test_ff, 0xb150, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b158_95_test_ff, 0xb158, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b160_95_test_ff, 0xb160, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b168_95_test_ff, 0xb168, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b170_95_test_ff, 0xb170, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b178_95_test_ff, 0xb178, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b179_95_test_ff, 0xb179, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b180_95_test_ff, 0xb180, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b188_95_test_ff, 0xb188, 2, { 0, 0 }, 0 }, /* CMPM */
{ NULL, op_b190_95_test_ff, 0xb190, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b198_95_test_ff, 0xb198, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a0_95_test_ff, 0xb1a0, 2, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1a8_95_test_ff, 0xb1a8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b0_95_test_ff, 0xb1b0, 2, { 2, 0 }, 0 }, /* EOR */
{ NULL, op_b1b8_95_test_ff, 0xb1b8, 4, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1b9_95_test_ff, 0xb1b9, 6, { 0, 0 }, 0 }, /* EOR */
{ NULL, op_b1c0_95_test_ff, 0xb1c0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1c8_95_test_ff, 0xb1c8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d0_95_test_ff, 0xb1d0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1d8_95_test_ff, 0xb1d8, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e0_95_test_ff, 0xb1e0, 2, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1e8_95_test_ff, 0xb1e8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f0_95_test_ff, 0xb1f0, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f8_95_test_ff, 0xb1f8, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1f9_95_test_ff, 0xb1f9, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fa_95_test_ff, 0xb1fa, 4, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fb_95_test_ff, 0xb1fb, 2, { 2, 0 }, 0 }, /* CMPA */
{ NULL, op_b1fc_95_test_ff, 0xb1fc, 6, { 0, 0 }, 0 }, /* CMPA */
{ NULL, op_c000_95_test_ff, 0xc000, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c010_95_test_ff, 0xc010, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c018_95_test_ff, 0xc018, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c020_95_test_ff, 0xc020, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c028_95_test_ff, 0xc028, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c030_95_test_ff, 0xc030, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c038_95_test_ff, 0xc038, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c039_95_test_ff, 0xc039, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03a_95_test_ff, 0xc03a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c03b_95_test_ff, 0xc03b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c03c_95_test_ff, 0xc03c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c040_95_test_ff, 0xc040, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c050_95_test_ff, 0xc050, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c058_95_test_ff, 0xc058, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c060_95_test_ff, 0xc060, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c068_95_test_ff, 0xc068, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c070_95_test_ff, 0xc070, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c078_95_test_ff, 0xc078, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c079_95_test_ff, 0xc079, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07a_95_test_ff, 0xc07a, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c07b_95_test_ff, 0xc07b, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c07c_95_test_ff, 0xc07c, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c080_95_test_ff, 0xc080, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c090_95_test_ff, 0xc090, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c098_95_test_ff, 0xc098, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a0_95_test_ff, 0xc0a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0a8_95_test_ff, 0xc0a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b0_95_test_ff, 0xc0b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0b8_95_test_ff, 0xc0b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0b9_95_test_ff, 0xc0b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0ba_95_test_ff, 0xc0ba, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0bb_95_test_ff, 0xc0bb, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c0bc_95_test_ff, 0xc0bc, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c0c0_95_test_ff, 0xc0c0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d0_95_test_ff, 0xc0d0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0d8_95_test_ff, 0xc0d8, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e0_95_test_ff, 0xc0e0, 2, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0e8_95_test_ff, 0xc0e8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f0_95_test_ff, 0xc0f0, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0f8_95_test_ff, 0xc0f8, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0f9_95_test_ff, 0xc0f9, 6, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fa_95_test_ff, 0xc0fa, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c0fb_95_test_ff, 0xc0fb, 2, { 2, 0 }, 0 }, /* MULU */
{ NULL, op_c0fc_95_test_ff, 0xc0fc, 4, { 0, 0 }, 0 }, /* MULU */
{ NULL, op_c100_95_test_ff, 0xc100, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c108_95_test_ff, 0xc108, 2, { 0, 0 }, 0 }, /* ABCD */
{ NULL, op_c110_95_test_ff, 0xc110, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c118_95_test_ff, 0xc118, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c120_95_test_ff, 0xc120, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c128_95_test_ff, 0xc128, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c130_95_test_ff, 0xc130, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c138_95_test_ff, 0xc138, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c139_95_test_ff, 0xc139, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c140_95_test_ff, 0xc140, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c148_95_test_ff, 0xc148, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c150_95_test_ff, 0xc150, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c158_95_test_ff, 0xc158, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c160_95_test_ff, 0xc160, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c168_95_test_ff, 0xc168, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c170_95_test_ff, 0xc170, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c178_95_test_ff, 0xc178, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c179_95_test_ff, 0xc179, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c188_95_test_ff, 0xc188, 2, { 0, 0 }, 0 }, /* EXG */
{ NULL, op_c190_95_test_ff, 0xc190, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c198_95_test_ff, 0xc198, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a0_95_test_ff, 0xc1a0, 2, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1a8_95_test_ff, 0xc1a8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b0_95_test_ff, 0xc1b0, 2, { 2, 0 }, 0 }, /* AND */
{ NULL, op_c1b8_95_test_ff, 0xc1b8, 4, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1b9_95_test_ff, 0xc1b9, 6, { 0, 0 }, 0 }, /* AND */
{ NULL, op_c1c0_95_test_ff, 0xc1c0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d0_95_test_ff, 0xc1d0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1d8_95_test_ff, 0xc1d8, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e0_95_test_ff, 0xc1e0, 2, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1e8_95_test_ff, 0xc1e8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f0_95_test_ff, 0xc1f0, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1f8_95_test_ff, 0xc1f8, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1f9_95_test_ff, 0xc1f9, 6, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fa_95_test_ff, 0xc1fa, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_c1fb_95_test_ff, 0xc1fb, 2, { 2, 0 }, 0 }, /* MULS */
{ NULL, op_c1fc_95_test_ff, 0xc1fc, 4, { 0, 0 }, 0 }, /* MULS */
{ NULL, op_d000_95_test_ff, 0xd000, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d010_95_test_ff, 0xd010, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d018_95_test_ff, 0xd018, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d020_95_test_ff, 0xd020, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d028_95_test_ff, 0xd028, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d030_95_test_ff, 0xd030, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d038_95_test_ff, 0xd038, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d039_95_test_ff, 0xd039, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03a_95_test_ff, 0xd03a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d03b_95_test_ff, 0xd03b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d03c_95_test_ff, 0xd03c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d040_95_test_ff, 0xd040, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d048_95_test_ff, 0xd048, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d050_95_test_ff, 0xd050, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d058_95_test_ff, 0xd058, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d060_95_test_ff, 0xd060, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d068_95_test_ff, 0xd068, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d070_95_test_ff, 0xd070, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d078_95_test_ff, 0xd078, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d079_95_test_ff, 0xd079, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07a_95_test_ff, 0xd07a, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d07b_95_test_ff, 0xd07b, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d07c_95_test_ff, 0xd07c, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d080_95_test_ff, 0xd080, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d088_95_test_ff, 0xd088, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d090_95_test_ff, 0xd090, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d098_95_test_ff, 0xd098, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a0_95_test_ff, 0xd0a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0a8_95_test_ff, 0xd0a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b0_95_test_ff, 0xd0b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0b8_95_test_ff, 0xd0b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0b9_95_test_ff, 0xd0b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0ba_95_test_ff, 0xd0ba, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0bb_95_test_ff, 0xd0bb, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d0bc_95_test_ff, 0xd0bc, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d0c0_95_test_ff, 0xd0c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0c8_95_test_ff, 0xd0c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d0_95_test_ff, 0xd0d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0d8_95_test_ff, 0xd0d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e0_95_test_ff, 0xd0e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0e8_95_test_ff, 0xd0e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f0_95_test_ff, 0xd0f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f8_95_test_ff, 0xd0f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0f9_95_test_ff, 0xd0f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fa_95_test_ff, 0xd0fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fb_95_test_ff, 0xd0fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d0fc_95_test_ff, 0xd0fc, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d100_95_test_ff, 0xd100, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d108_95_test_ff, 0xd108, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d110_95_test_ff, 0xd110, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d118_95_test_ff, 0xd118, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d120_95_test_ff, 0xd120, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d128_95_test_ff, 0xd128, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d130_95_test_ff, 0xd130, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d138_95_test_ff, 0xd138, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d139_95_test_ff, 0xd139, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d140_95_test_ff, 0xd140, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d148_95_test_ff, 0xd148, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d150_95_test_ff, 0xd150, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d158_95_test_ff, 0xd158, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d160_95_test_ff, 0xd160, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d168_95_test_ff, 0xd168, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d170_95_test_ff, 0xd170, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d178_95_test_ff, 0xd178, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d179_95_test_ff, 0xd179, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d180_95_test_ff, 0xd180, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d188_95_test_ff, 0xd188, 2, { 0, 0 }, 0 }, /* ADDX */
{ NULL, op_d190_95_test_ff, 0xd190, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d198_95_test_ff, 0xd198, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a0_95_test_ff, 0xd1a0, 2, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1a8_95_test_ff, 0xd1a8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b0_95_test_ff, 0xd1b0, 2, { 2, 0 }, 0 }, /* ADD */
{ NULL, op_d1b8_95_test_ff, 0xd1b8, 4, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1b9_95_test_ff, 0xd1b9, 6, { 0, 0 }, 0 }, /* ADD */
{ NULL, op_d1c0_95_test_ff, 0xd1c0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1c8_95_test_ff, 0xd1c8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d0_95_test_ff, 0xd1d0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1d8_95_test_ff, 0xd1d8, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e0_95_test_ff, 0xd1e0, 2, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1e8_95_test_ff, 0xd1e8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f0_95_test_ff, 0xd1f0, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f8_95_test_ff, 0xd1f8, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1f9_95_test_ff, 0xd1f9, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fa_95_test_ff, 0xd1fa, 4, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fb_95_test_ff, 0xd1fb, 2, { 2, 0 }, 0 }, /* ADDA */
{ NULL, op_d1fc_95_test_ff, 0xd1fc, 6, { 0, 0 }, 0 }, /* ADDA */
{ NULL, op_e000_95_test_ff, 0xe000, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e008_95_test_ff, 0xe008, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e010_95_test_ff, 0xe010, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e018_95_test_ff, 0xe018, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e020_95_test_ff, 0xe020, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e028_95_test_ff, 0xe028, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e030_95_test_ff, 0xe030, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e038_95_test_ff, 0xe038, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e040_95_test_ff, 0xe040, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e048_95_test_ff, 0xe048, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e050_95_test_ff, 0xe050, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e058_95_test_ff, 0xe058, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e060_95_test_ff, 0xe060, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e068_95_test_ff, 0xe068, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e070_95_test_ff, 0xe070, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e078_95_test_ff, 0xe078, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e080_95_test_ff, 0xe080, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e088_95_test_ff, 0xe088, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e090_95_test_ff, 0xe090, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e098_95_test_ff, 0xe098, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0a0_95_test_ff, 0xe0a0, 2, { 0, 0 }, 0 }, /* ASR */
{ NULL, op_e0a8_95_test_ff, 0xe0a8, 2, { 0, 0 }, 0 }, /* LSR */
{ NULL, op_e0b0_95_test_ff, 0xe0b0, 2, { 0, 0 }, 0 }, /* ROXR */
{ NULL, op_e0b8_95_test_ff, 0xe0b8, 2, { 0, 0 }, 0 }, /* ROR */
{ NULL, op_e0d0_95_test_ff, 0xe0d0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0d8_95_test_ff, 0xe0d8, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e0_95_test_ff, 0xe0e0, 2, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0e8_95_test_ff, 0xe0e8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f0_95_test_ff, 0xe0f0, 2, { 2, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f8_95_test_ff, 0xe0f8, 4, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e0f9_95_test_ff, 0xe0f9, 6, { 0, 0 }, 0 }, /* ASRW */
{ NULL, op_e100_95_test_ff, 0xe100, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e108_95_test_ff, 0xe108, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e110_95_test_ff, 0xe110, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e118_95_test_ff, 0xe118, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e120_95_test_ff, 0xe120, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e128_95_test_ff, 0xe128, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e130_95_test_ff, 0xe130, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e138_95_test_ff, 0xe138, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e140_95_test_ff, 0xe140, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e148_95_test_ff, 0xe148, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e150_95_test_ff, 0xe150, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e158_95_test_ff, 0xe158, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e160_95_test_ff, 0xe160, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e168_95_test_ff, 0xe168, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e170_95_test_ff, 0xe170, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e178_95_test_ff, 0xe178, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e180_95_test_ff, 0xe180, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e188_95_test_ff, 0xe188, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e190_95_test_ff, 0xe190, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e198_95_test_ff, 0xe198, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1a0_95_test_ff, 0xe1a0, 2, { 0, 0 }, 0 }, /* ASL */
{ NULL, op_e1a8_95_test_ff, 0xe1a8, 2, { 0, 0 }, 0 }, /* LSL */
{ NULL, op_e1b0_95_test_ff, 0xe1b0, 2, { 0, 0 }, 0 }, /* ROXL */
{ NULL, op_e1b8_95_test_ff, 0xe1b8, 2, { 0, 0 }, 0 }, /* ROL */
{ NULL, op_e1d0_95_test_ff, 0xe1d0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1d8_95_test_ff, 0xe1d8, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e0_95_test_ff, 0xe1e0, 2, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1e8_95_test_ff, 0xe1e8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f0_95_test_ff, 0xe1f0, 2, { 2, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f8_95_test_ff, 0xe1f8, 4, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e1f9_95_test_ff, 0xe1f9, 6, { 0, 0 }, 0 }, /* ASLW */
{ NULL, op_e2d0_95_test_ff, 0xe2d0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2d8_95_test_ff, 0xe2d8, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e0_95_test_ff, 0xe2e0, 2, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2e8_95_test_ff, 0xe2e8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f0_95_test_ff, 0xe2f0, 2, { 2, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f8_95_test_ff, 0xe2f8, 4, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e2f9_95_test_ff, 0xe2f9, 6, { 0, 0 }, 0 }, /* LSRW */
{ NULL, op_e3d0_95_test_ff, 0xe3d0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3d8_95_test_ff, 0xe3d8, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e0_95_test_ff, 0xe3e0, 2, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3e8_95_test_ff, 0xe3e8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f0_95_test_ff, 0xe3f0, 2, { 2, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f8_95_test_ff, 0xe3f8, 4, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e3f9_95_test_ff, 0xe3f9, 6, { 0, 0 }, 0 }, /* LSLW */
{ NULL, op_e4d0_95_test_ff, 0xe4d0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4d8_95_test_ff, 0xe4d8, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e0_95_test_ff, 0xe4e0, 2, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4e8_95_test_ff, 0xe4e8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f0_95_test_ff, 0xe4f0, 2, { 2, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f8_95_test_ff, 0xe4f8, 4, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e4f9_95_test_ff, 0xe4f9, 6, { 0, 0 }, 0 }, /* ROXRW */
{ NULL, op_e5d0_95_test_ff, 0xe5d0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5d8_95_test_ff, 0xe5d8, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e0_95_test_ff, 0xe5e0, 2, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5e8_95_test_ff, 0xe5e8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f0_95_test_ff, 0xe5f0, 2, { 2, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f8_95_test_ff, 0xe5f8, 4, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e5f9_95_test_ff, 0xe5f9, 6, { 0, 0 }, 0 }, /* ROXLW */
{ NULL, op_e6d0_95_test_ff, 0xe6d0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6d8_95_test_ff, 0xe6d8, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e0_95_test_ff, 0xe6e0, 2, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6e8_95_test_ff, 0xe6e8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f0_95_test_ff, 0xe6f0, 2, { 2, 0 }, 0 }, /* RORW */
{ NULL, op_e6f8_95_test_ff, 0xe6f8, 4, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e6f9_95_test_ff, 0xe6f9, 6, { 0, 0 }, 0 }, /* RORW */
{ NULL, op_e7d0_95_test_ff, 0xe7d0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7d8_95_test_ff, 0xe7d8, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e0_95_test_ff, 0xe7e0, 2, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7e8_95_test_ff, 0xe7e8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f0_95_test_ff, 0xe7f0, 2, { 2, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f8_95_test_ff, 0xe7f8, 4, { 0, 0 }, 0 }, /* ROLW */
{ NULL, op_e7f9_95_test_ff, 0xe7f9, 6, { 0, 0 }, 0 }, /* ROLW */
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8c0_95_test_ff, 0xe8c0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8d0_95_test_ff, 0xe8d0, 4, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8e8_95_test_ff, 0xe8e8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f0_95_test_ff, 0xe8f0, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f8_95_test_ff, 0xe8f8, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8f9_95_test_ff, 0xe8f9, 8, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fa_95_test_ff, 0xe8fa, 6, { 0, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e8fb_95_test_ff, 0xe8fb, 4, { 2, 0 }, 0 }, /* BFTST */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9c0_95_test_ff, 0xe9c0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9d0_95_test_ff, 0xe9d0, 4, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9e8_95_test_ff, 0xe9e8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f0_95_test_ff, 0xe9f0, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f8_95_test_ff, 0xe9f8, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9f9_95_test_ff, 0xe9f9, 8, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fa_95_test_ff, 0xe9fa, 6, { 0, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_e9fb_95_test_ff, 0xe9fb, 4, { 2, 0 }, 0 }, /* BFEXTU */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eac0_95_test_ff, 0xeac0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ead0_95_test_ff, 0xead0, 4, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eae8_95_test_ff, 0xeae8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf0_95_test_ff, 0xeaf0, 4, { 2, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf8_95_test_ff, 0xeaf8, 6, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eaf9_95_test_ff, 0xeaf9, 8, { 0, 0 }, 0 }, /* BFCHG */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebc0_95_test_ff, 0xebc0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebd0_95_test_ff, 0xebd0, 4, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebe8_95_test_ff, 0xebe8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf0_95_test_ff, 0xebf0, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf8_95_test_ff, 0xebf8, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebf9_95_test_ff, 0xebf9, 8, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfa_95_test_ff, 0xebfa, 6, { 0, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ebfb_95_test_ff, 0xebfb, 4, { 2, 0 }, 0 }, /* BFEXTS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecc0_95_test_ff, 0xecc0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecd0_95_test_ff, 0xecd0, 4, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ece8_95_test_ff, 0xece8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf0_95_test_ff, 0xecf0, 4, { 2, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf8_95_test_ff, 0xecf8, 6, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ecf9_95_test_ff, 0xecf9, 8, { 0, 0 }, 0 }, /* BFCLR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edc0_95_test_ff, 0xedc0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edd0_95_test_ff, 0xedd0, 4, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_ede8_95_test_ff, 0xede8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf0_95_test_ff, 0xedf0, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf8_95_test_ff, 0xedf8, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edf9_95_test_ff, 0xedf9, 8, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfa_95_test_ff, 0xedfa, 6, { 0, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_edfb_95_test_ff, 0xedfb, 4, { 2, 0 }, 0 }, /* BFFFO */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eec0_95_test_ff, 0xeec0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eed0_95_test_ff, 0xeed0, 4, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eee8_95_test_ff, 0xeee8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef0_95_test_ff, 0xeef0, 4, { 2, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef8_95_test_ff, 0xeef8, 6, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eef9_95_test_ff, 0xeef9, 8, { 0, 0 }, 0 }, /* BFSET */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efc0_95_test_ff, 0xefc0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efd0_95_test_ff, 0xefd0, 4, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_efe8_95_test_ff, 0xefe8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff0_95_test_ff, 0xeff0, 4, { 2, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff8_95_test_ff, 0xeff8, 6, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_eff9_95_test_ff, 0xeff9, 8, { 0, 0 }, 0 }, /* BFINS */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f000_95_test_ff, 0xf000, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f008_95_test_ff, 0xf008, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f010_95_test_ff, 0xf010, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f018_95_test_ff, 0xf018, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f020_95_test_ff, 0xf020, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f028_95_test_ff, 0xf028, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f030_95_test_ff, 0xf030, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f038_95_test_ff, 0xf038, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f039_95_test_ff, 0xf039, -1, { 0, 0 }, 0 }, /* MMUOP030 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f200_95_test_ff, 0xf200, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f208_95_test_ff, 0xf208, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f210_95_test_ff, 0xf210, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f218_95_test_ff, 0xf218, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f220_95_test_ff, 0xf220, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f228_95_test_ff, 0xf228, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f230_95_test_ff, 0xf230, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f238_95_test_ff, 0xf238, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f239_95_test_ff, 0xf239, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23a_95_test_ff, 0xf23a, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23b_95_test_ff, 0xf23b, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f23c_95_test_ff, 0xf23c, -1, { 0, 0 }, 0 }, /* FPP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f240_95_test_ff, 0xf240, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f248_95_test_ff, 0xf248, -1, { 0, 0 }, 0 }, /* FDBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f250_95_test_ff, 0xf250, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f258_95_test_ff, 0xf258, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f260_95_test_ff, 0xf260, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f268_95_test_ff, 0xf268, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f270_95_test_ff, 0xf270, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f278_95_test_ff, 0xf278, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f279_95_test_ff, 0xf279, -1, { 0, 0 }, 0 }, /* FScc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27a_95_test_ff, 0xf27a, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27b_95_test_ff, 0xf27b, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f27c_95_test_ff, 0xf27c, -1, { 0, 0 }, 0 }, /* FTRAPcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f280_95_test_ff, 0xf280, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f2c0_95_test_ff, 0xf2c0, -1, { 0, 0 }, 0 }, /* FBcc */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f310_95_test_ff, 0xf310, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f320_95_test_ff, 0xf320, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f328_95_test_ff, 0xf328, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f330_95_test_ff, 0xf330, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f338_95_test_ff, 0xf338, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f339_95_test_ff, 0xf339, -1, { 0, 0 }, 0 }, /* FSAVE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f350_95_test_ff, 0xf350, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f358_95_test_ff, 0xf358, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f368_95_test_ff, 0xf368, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f370_95_test_ff, 0xf370, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f378_95_test_ff, 0xf378, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f379_95_test_ff, 0xf379, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37a_95_test_ff, 0xf37a, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f37b_95_test_ff, 0xf37b, -1, { 0, 0 }, 0 }, /* FRESTORE */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f408_95_test_ff, 0xf408, -1, { 0, 0 }, 0 }, /* CINVL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f410_95_test_ff, 0xf410, -1, { 0, 0 }, 0 }, /* CINVP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f418_95_test_ff, 0xf418, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f419_95_test_ff, 0xf419, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41a_95_test_ff, 0xf41a, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41b_95_test_ff, 0xf41b, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41c_95_test_ff, 0xf41c, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41d_95_test_ff, 0xf41d, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41e_95_test_ff, 0xf41e, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f41f_95_test_ff, 0xf41f, -1, { 0, 0 }, 0 }, /* CINVA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f428_95_test_ff, 0xf428, -1, { 0, 0 }, 0 }, /* CPUSHL */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f430_95_test_ff, 0xf430, -1, { 0, 0 }, 0 }, /* CPUSHP */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f438_95_test_ff, 0xf438, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f439_95_test_ff, 0xf439, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43a_95_test_ff, 0xf43a, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43b_95_test_ff, 0xf43b, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43c_95_test_ff, 0xf43c, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43d_95_test_ff, 0xf43d, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43e_95_test_ff, 0xf43e, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f43f_95_test_ff, 0xf43f, -1, { 0, 0 }, 0 }, /* CPUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f500_95_test_ff, 0xf500, -1, { 0, 0 }, 0 }, /* PFLUSHN */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f508_95_test_ff, 0xf508, -1, { 0, 0 }, 0 }, /* PFLUSH */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f510_95_test_ff, 0xf510, -1, { 0, 0 }, 0 }, /* PFLUSHAN */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f518_95_test_ff, 0xf518, -1, { 0, 0 }, 0 }, /* PFLUSHA */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f548_95_test_ff, 0xf548, -1, { 0, 0 }, 0 }, /* PTESTW */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f568_95_test_ff, 0xf568, -1, { 0, 0 }, 0 }, /* PTESTR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f588_95_test_ff, 0xf588, -1, { 0, 0 }, 0 }, /* PLPAW */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f5c8_95_test_ff, 0xf5c8, -1, { 0, 0 }, 0 }, /* PLPAR */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f600_95_test_ff, 0xf600, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f608_95_test_ff, 0xf608, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f610_95_test_ff, 0xf610, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f618_95_test_ff, 0xf618, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f620_95_test_ff, 0xf620, -1, { 0, 0 }, 0 }, /* MOVE16 */
#endif
#ifndef CPUEMU_68000_ONLY
{ NULL, op_f800_95_test_ff, 0xf800, -1, { 0, 0 }, 0 }, /* LPSTOP */
#endif
{ 0 }};
#endif /* CPUEMU_95 */
