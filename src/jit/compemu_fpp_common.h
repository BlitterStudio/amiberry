#pragma once

// FPU operand sizes in bytes by the extension word's source format, for the
// (An)+ / -(An) increment; sz2 is for A7, which keeps the stack word aligned.
static const int fpp_ea_sz1[8] = { 4, 4, 12, 12, 2, 8, 1, 0 };
static const int fpp_ea_sz2[8] = { 4, 4, 12, 12, 2, 8, 2, 0 };
