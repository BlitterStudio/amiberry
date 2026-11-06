# 86Box: the x86 CPU, PC chipset, keyboard, mouse, serial and ISA sound for
# the bridgeboards, ATonce and DraCo, and the video cards behind the RTG
# boards in src/gfxboard.cpp. Imported verbatim from WinUAE's 86box/
# subtree; the lists mirror cmake/86Box.cmake. Included by libretro/Makefile
# after SOURCES_CXX is composed.

SOURCES_86BOX := \
	$(AMIBERRY_DIR)/src/86box/cpu/386_common.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/386_dynarec.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/386.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/808x_common.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/808x_marty_86box.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/808x.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/cache_as_ram.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/cpu.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/fpu.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/i8080.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/consts.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_addsub.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_class.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_compare.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_div.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_extract.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_mul.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_rem.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_roundToInt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_scale.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_sqrt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_f128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_f16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_i32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_i32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_i64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_i64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_ui32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_ui32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_ui64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/extF80_to_ui64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_addsub.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_div.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_mul.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_mulAdd.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_roundToInt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_i32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_i32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_i64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_i64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_ui32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_ui32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_ui64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f128_to_ui64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_addsub.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_class.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_compare.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_div.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_getExp.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_getMant.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_minmax.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_mul.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_mulAdd.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_range.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_roundToInt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_sqrt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_i32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_i32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_i64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_i64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_ui32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_ui32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_ui64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f16_to_ui64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f2xm1.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_addsub.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_class.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_compare.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_div.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_frc.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_getExp.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_getMant.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_minmax.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_mul.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_mulAdd.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_range.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_roundToInt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_scalef.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_sqrt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_f128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_f16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_i32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_i32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_i64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_i64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_ui32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_ui32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_ui64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f32_to_ui64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_addsub.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_class.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_compare.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_div.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_frc.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_getExp.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_getMant.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_minmax.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_mul.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_mulAdd.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_range.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_roundToInt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_scalef.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_sqrt.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_f128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_f16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_i32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_i32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_i64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_i64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_ui32_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_ui32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_ui64_r_minMag.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/f64_to_ui64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/fpatan.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/fprem.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/fsincos.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/fyl2x.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i32_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i32_to_f128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i32_to_f16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i32_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i32_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i64_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i64_to_f128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i64_to_f16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i64_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/i64_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/isNaN.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/isSignalingNaN.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/poly.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/primitives.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_add128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_add256M.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_addMagsExtF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_addMagsF128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_addMagsF16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_addMagsF32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_addMagsF64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_approxRecip_1Ks.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_approxRecipSqrt_1Ks.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_approxRecipSqrt32_1.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_commonNaNToExtF80UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_commonNaNToF128UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_commonNaNToF16UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_commonNaNToF32UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_commonNaNToF64UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_countLeadingZeros16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_countLeadingZeros32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_countLeadingZeros64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_countLeadingZeros8.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_eq128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_le128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_lt128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_mul128By32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_mul128To256M.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_mul64ByShifted32To128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_mul64To128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normRoundPackToExtF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normRoundPackToF128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normRoundPackToF16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normRoundPackToF32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normRoundPackToF64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normSubnormalExtF80Sig.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normSubnormalF128Sig.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normSubnormalF16Sig.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normSubnormalF32Sig.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_normSubnormalF64Sig.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_packToExtF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_propagateNaNExtF80UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_propagateNaNF128UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_propagateNaNF16UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_propagateNaNF32UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_propagateNaNF64UI.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundPackToExtF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundPackToF128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundPackToF16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundPackToF32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundPackToF64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundToI32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundToI64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundToUI32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_roundToUI64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shiftRightJam128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shiftRightJam128Extra.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shiftRightJam256M.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shiftRightJam32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shiftRightJam64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shiftRightJam64Extra.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shortShiftLeft128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shortShiftRight128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shortShiftRightJam64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_shortShiftRightJam64Extra.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_sub128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_sub256M.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_subMagsExtF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_subMagsF128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_subMagsF16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_subMagsF32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/s_subMagsF64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/softfloat-specialize.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui32_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui32_to_f128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui32_to_f16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui32_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui32_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui64_to_extF80.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui64_to_f128.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui64_to_f16.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui64_to_f32.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e/ui64_to_f64.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/vx0_biu.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/vx0.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x86_ops_mmx.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x86.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x86seg_2386.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x86seg_common.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x86seg.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x87_ferr.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x87_timings.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/x87.cpp \
	$(AMIBERRY_DIR)/src/86box/device/kbc_at_dev.cpp \
	$(AMIBERRY_DIR)/src/86box/device/kbc_at.cpp \
	$(AMIBERRY_DIR)/src/86box/device/keyboard_at.cpp \
	$(AMIBERRY_DIR)/src/86box/device/keyboard.cpp \
	$(AMIBERRY_DIR)/src/86box/device/mouse_ps2.cpp \
	$(AMIBERRY_DIR)/src/86box/device/mouse_serial.cpp \
	$(AMIBERRY_DIR)/src/86box/device/serial.cpp \
	$(AMIBERRY_DIR)/src/86box/dma.cpp \
	$(AMIBERRY_DIR)/src/86box/mem/mem.cpp \
	$(AMIBERRY_DIR)/src/86box/mem/mmu_2386.cpp \
	$(AMIBERRY_DIR)/src/86box/mem/smram.cpp \
	$(AMIBERRY_DIR)/src/86box/nmi.cpp \
	$(AMIBERRY_DIR)/src/86box/nvr_at.cpp \
	$(AMIBERRY_DIR)/src/86box/nvr.cpp \
	$(AMIBERRY_DIR)/src/86box/pic.cpp \
	$(AMIBERRY_DIR)/src/86box/pit_fast.cpp \
	$(AMIBERRY_DIR)/src/86box/pit.cpp \
	$(AMIBERRY_DIR)/src/86box/port_6x.cpp \
	$(AMIBERRY_DIR)/src/86box/ppi.cpp \
	$(AMIBERRY_DIR)/src/86box/sound/snd_cms.cpp \
	$(AMIBERRY_DIR)/src/86box/sound/snd_mpu401.cpp \
	$(AMIBERRY_DIR)/src/86box/sound/snd_opl2_nuked.cpp \
	$(AMIBERRY_DIR)/src/86box/sound/snd_opl3_nuked.cpp \
	$(AMIBERRY_DIR)/src/86box/sound/snd_sb_dsp.cpp \
	$(AMIBERRY_DIR)/src/86box/sound/snd_sb.cpp \
	$(AMIBERRY_DIR)/src/86box/sound/snd_speaker.cpp \
	$(AMIBERRY_DIR)/src/86box/timer.cpp \
	$(AMIBERRY_DIR)/src/86box/utils/fifo.cpp

# The x86 recompiler, common part
SOURCES_86BOX_DYNAREC := \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_accumulate.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_allocator.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_block.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ir.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_3dnow.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_arith.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_branch.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_fpu_arith.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_fpu_constant.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_fpu_loadstore.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_fpu_misc.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_helpers.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_jump.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_logic.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_misc.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_mmx_arith.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_mmx_cmp.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_mmx_loadstore.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_mmx_logic.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_mmx_pack.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_mmx_shift.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_mov.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_rep.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_shift.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_ops_stack.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_reg.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/386_dynarec_ops.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_486.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_686.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_common.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_k6.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_p6.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_pentium.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_winchip.cpp \
	$(AMIBERRY_DIR)/src/86box/cpu/codegen_timing_winchip2.cpp

SOURCES_86BOX_DYNAREC_AARCH64 := \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_arm64.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_arm64_imm.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_arm64_ops.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_arm64_uops.cpp

SOURCES_86BOX_DYNAREC_X86_64 := \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_x86-64.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_x86-64_ops.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_x86-64_ops_sse.cpp \
	$(AMIBERRY_DIR)/src/86box/codegen/codegen_backend_x86-64_uops.cpp

# The RTG video cards
SOURCES_86BOX_VIDEO := \
	$(AMIBERRY_DIR)/src/86box/device/i2c.cpp \
	$(AMIBERRY_DIR)/src/86box/device/i2c_gpio.cpp \
	$(AMIBERRY_DIR)/src/86box/mem/i2c_eeprom.cpp \
	$(AMIBERRY_DIR)/src/86box/mem/nmc93cxx.cpp \
	$(AMIBERRY_DIR)/src/86box/video/clockgen/vid_clockgen_av9194.cpp \
	$(AMIBERRY_DIR)/src/86box/video/clockgen/vid_clockgen_ch9204.cpp \
	$(AMIBERRY_DIR)/src/86box/video/clockgen/vid_clockgen_icd2061.cpp \
	$(AMIBERRY_DIR)/src/86box/video/clockgen/vid_clockgen_ics2494.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_att20c49x.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_att2xc498.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_bt48x.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_ibm_rgb528.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_mu9c1880.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_sc1148x.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_sc1502x.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_sdac.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_stg1702.cpp \
	$(AMIBERRY_DIR)/src/86box/video/ramdac/vid_ramdac_tvp3026.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_cl54xx.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_ddc.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_et4000.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_et4000w32.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_mga.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_s3.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_s3_virge.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_svga.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_svga_render.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_banshee.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_banshee_blitter.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_blitter.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_display.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_fb.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_fifo.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_reg.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_render.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_setup.cpp \
	$(AMIBERRY_DIR)/src/86box/video/vid_voodoo_texture.cpp \
	$(AMIBERRY_DIR)/src/86box/winuae/vid_inmos.cpp \
	$(AMIBERRY_DIR)/src/86box/winuae/vid_ncr.cpp \
	$(AMIBERRY_DIR)/src/86box/winuae/vid_permedia2.cpp \
	$(AMIBERRY_DIR)/src/86box/winuae/vid_ramdac_bt482.cpp

# The WinUAE glue builds with the core's flags so it can use Amiberry's
# headers; it only needs 86Box's include paths.
SOURCES_86BOX_GLUE := \
	$(AMIBERRY_DIR)/src/86box/winuae/86box_glue.cpp \
	$(AMIBERRY_DIR)/src/86box/winuae/86box_machine.cpp \
	$(AMIBERRY_DIR)/src/86box/winuae/keyboard_at_draco.cpp \
	$(AMIBERRY_DIR)/src/86box/winuae/86box_video.cpp

# PC hardware emulation is opt-in, as it was with PCem: it adds ~300
# compilation units to the core.
ifeq ($(USE_86BOX),1)
FLAGS += -DUSE_86BOX
INCFLAGS += -I$(AMIBERRY_DIR)/src/86box/include -I$(AMIBERRY_DIR)/src/86box/winuae

# 86Box's x86 recompiler has one backend per binary (aarch64 or x86-64), so
# universal binaries and other architectures (RISC-V, LoongArch) keep
# the interpreter.
86BOX_DYNAREC := 0
ifeq ($(ARCH),aarch64)
	86BOX_DYNAREC := 1
	86BOX_DYNAREC_BACKEND := AARCH64
else ifeq ($(ARCH),x86_64)
	86BOX_DYNAREC := 1
	86BOX_DYNAREC_BACKEND := X86_64
else ifeq ($(ARCH),amd64)
	86BOX_DYNAREC := 1
	86BOX_DYNAREC_BACKEND := X86_64
endif

86BOX_DEFINES := -DUAE= -D_FILE_OFFSET_BITS=64
SOURCES_86BOX_IMPORTED := $(SOURCES_86BOX) $(SOURCES_86BOX_VIDEO)
ifeq ($(86BOX_DYNAREC),1)
# USE_DYNAREC changes 86Box's CPU state, so every file that includes its
# cpu.h must see it: the imported objects, the glue and src/x86.cpp.
FLAGS += -DUSE_DYNAREC
86BOX_DEFINES += -DUSE_DYNAREC
SOURCES_86BOX_IMPORTED += $(SOURCES_86BOX_DYNAREC) $(SOURCES_86BOX_DYNAREC_$(86BOX_DYNAREC_BACKEND))
endif

SOURCES_CXX += $(SOURCES_86BOX_IMPORTED) $(SOURCES_86BOX_GLUE)

# The imported sources are third-party code: C++20, 86Box's own include
# layout, warnings off. The video ones get their memory, I/O, timer and
# device calls redirected to the RTG versions in 86box_video.cpp first.
86BOX_INCFLAGS := -I$(AMIBERRY_DIR)/src/86box/winuae/include \
	-I$(AMIBERRY_DIR)/src/86box/include \
	-I$(AMIBERRY_DIR)/src/86box/cpu \
	-I$(AMIBERRY_DIR)/src/86box/cpu/softfloat3e \
	-I$(AMIBERRY_DIR)/src/86box/codegen \
	-I$(AMIBERRY_DIR)/src/86box
ifneq (,$(findstring clang,$(CXX)))
	# Clang takes C11's _Static_assert in C++ too.
	86BOX_GNU_STATIC_ASSERT :=
else
	86BOX_GNU_STATIC_ASSERT := -D_Static_assert=static_assert
endif
ifeq ($(DEBUG), 1)
	86BOX_OPTFLAGS := -O0 -g -fno-omit-frame-pointer
else
	86BOX_OPTFLAGS := -O3 -DNDEBUG
endif
ifeq ($(SANITIZE), 1)
	86BOX_OPTFLAGS += -fsanitize=address -fno-omit-frame-pointer -g
endif

86BOX_OBJECTS := $(SOURCES_86BOX_IMPORTED:.cpp=.o)
$(86BOX_OBJECTS): STD_CXX := -std=gnu++20
$(86BOX_OBJECTS): FLAGS := $(fpic) -w $(86BOX_DEFINES) $(86BOX_GNU_STATIC_ASSERT) \
	$(86BOX_OPTFLAGS) -fvisibility=hidden -ffunction-sections -fdata-sections
$(86BOX_OBJECTS): INCFLAGS := $(86BOX_INCFLAGS)
$(SOURCES_86BOX_VIDEO:.cpp=.o): INCFLAGS := $(86BOX_INCFLAGS) \
	-include $(AMIBERRY_DIR)/src/86box/winuae/86box_video_redirect.h
else ifeq ($(USE_PCEM),1)
$(warning USE_PCEM was removed: Amiberry uses 86Box for the x86 bridgeboards. Build with USE_86BOX=1.)
endif
