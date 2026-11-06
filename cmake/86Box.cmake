# 86Box: the x86 CPU, PC chipset, keyboard, mouse, serial and ISA sound for
# the bridgeboards (A1060/A2088/A2286/A2386SX), ATonce and DraCo, plus the
# video cards behind the RTG boards in src/gfxboard.cpp. Imported verbatim
# from WinUAE's 86box/ subtree.
#
# The imported sources are third-party code: they are built as C++20 with
# 86Box's own include layout, without Amiberry's headers and with warnings
# suppressed, by the amiberry_86box and amiberry_86box_video object
# libraries. UAE= and USE_DYNAREC enable the WinUAE hooks in them and select
# the CPU state layout. The WinUAE glue under src/86box/winuae/ is built
# with the core instead, so it can use Amiberry's headers; it gets 86Box's
# include directories and the same defines through per-file source
# properties, as do src/x86.cpp, src/atonce.cpp and src/draco.cpp, which
# drive the PC from the core side.

set(AMIBERRY_86BOX_X86_SOURCES
    src/86box/cpu/386_common.cpp
    src/86box/cpu/386_dynarec.cpp
    src/86box/cpu/386.cpp
    src/86box/cpu/808x_common.cpp
    src/86box/cpu/808x_marty_86box.cpp
    src/86box/cpu/808x.cpp
    src/86box/cpu/cache_as_ram.cpp
    src/86box/cpu/cpu.cpp
    src/86box/cpu/fpu.cpp
    src/86box/cpu/i8080.cpp
    src/86box/cpu/softfloat3e/consts.cpp
    src/86box/cpu/softfloat3e/extF80_addsub.cpp
    src/86box/cpu/softfloat3e/extF80_class.cpp
    src/86box/cpu/softfloat3e/extF80_compare.cpp
    src/86box/cpu/softfloat3e/extF80_div.cpp
    src/86box/cpu/softfloat3e/extF80_extract.cpp
    src/86box/cpu/softfloat3e/extF80_mul.cpp
    src/86box/cpu/softfloat3e/extF80_rem.cpp
    src/86box/cpu/softfloat3e/extF80_roundToInt.cpp
    src/86box/cpu/softfloat3e/extF80_scale.cpp
    src/86box/cpu/softfloat3e/extF80_sqrt.cpp
    src/86box/cpu/softfloat3e/extF80_to_f128.cpp
    src/86box/cpu/softfloat3e/extF80_to_f16.cpp
    src/86box/cpu/softfloat3e/extF80_to_f32.cpp
    src/86box/cpu/softfloat3e/extF80_to_f64.cpp
    src/86box/cpu/softfloat3e/extF80_to_i32_r_minMag.cpp
    src/86box/cpu/softfloat3e/extF80_to_i32.cpp
    src/86box/cpu/softfloat3e/extF80_to_i64_r_minMag.cpp
    src/86box/cpu/softfloat3e/extF80_to_i64.cpp
    src/86box/cpu/softfloat3e/extF80_to_ui32_r_minMag.cpp
    src/86box/cpu/softfloat3e/extF80_to_ui32.cpp
    src/86box/cpu/softfloat3e/extF80_to_ui64_r_minMag.cpp
    src/86box/cpu/softfloat3e/extF80_to_ui64.cpp
    src/86box/cpu/softfloat3e/f128_addsub.cpp
    src/86box/cpu/softfloat3e/f128_div.cpp
    src/86box/cpu/softfloat3e/f128_mul.cpp
    src/86box/cpu/softfloat3e/f128_mulAdd.cpp
    src/86box/cpu/softfloat3e/f128_roundToInt.cpp
    src/86box/cpu/softfloat3e/f128_to_extF80.cpp
    src/86box/cpu/softfloat3e/f128_to_f32.cpp
    src/86box/cpu/softfloat3e/f128_to_f64.cpp
    src/86box/cpu/softfloat3e/f128_to_i32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f128_to_i32.cpp
    src/86box/cpu/softfloat3e/f128_to_i64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f128_to_i64.cpp
    src/86box/cpu/softfloat3e/f128_to_ui32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f128_to_ui32.cpp
    src/86box/cpu/softfloat3e/f128_to_ui64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f128_to_ui64.cpp
    src/86box/cpu/softfloat3e/f16_addsub.cpp
    src/86box/cpu/softfloat3e/f16_class.cpp
    src/86box/cpu/softfloat3e/f16_compare.cpp
    src/86box/cpu/softfloat3e/f16_div.cpp
    src/86box/cpu/softfloat3e/f16_getExp.cpp
    src/86box/cpu/softfloat3e/f16_getMant.cpp
    src/86box/cpu/softfloat3e/f16_minmax.cpp
    src/86box/cpu/softfloat3e/f16_mul.cpp
    src/86box/cpu/softfloat3e/f16_mulAdd.cpp
    src/86box/cpu/softfloat3e/f16_range.cpp
    src/86box/cpu/softfloat3e/f16_roundToInt.cpp
    src/86box/cpu/softfloat3e/f16_sqrt.cpp
    src/86box/cpu/softfloat3e/f16_to_extF80.cpp
    src/86box/cpu/softfloat3e/f16_to_f32.cpp
    src/86box/cpu/softfloat3e/f16_to_f64.cpp
    src/86box/cpu/softfloat3e/f16_to_i32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f16_to_i32.cpp
    src/86box/cpu/softfloat3e/f16_to_i64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f16_to_i64.cpp
    src/86box/cpu/softfloat3e/f16_to_ui32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f16_to_ui32.cpp
    src/86box/cpu/softfloat3e/f16_to_ui64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f16_to_ui64.cpp
    src/86box/cpu/softfloat3e/f2xm1.cpp
    src/86box/cpu/softfloat3e/f32_addsub.cpp
    src/86box/cpu/softfloat3e/f32_class.cpp
    src/86box/cpu/softfloat3e/f32_compare.cpp
    src/86box/cpu/softfloat3e/f32_div.cpp
    src/86box/cpu/softfloat3e/f32_frc.cpp
    src/86box/cpu/softfloat3e/f32_getExp.cpp
    src/86box/cpu/softfloat3e/f32_getMant.cpp
    src/86box/cpu/softfloat3e/f32_minmax.cpp
    src/86box/cpu/softfloat3e/f32_mul.cpp
    src/86box/cpu/softfloat3e/f32_mulAdd.cpp
    src/86box/cpu/softfloat3e/f32_range.cpp
    src/86box/cpu/softfloat3e/f32_roundToInt.cpp
    src/86box/cpu/softfloat3e/f32_scalef.cpp
    src/86box/cpu/softfloat3e/f32_sqrt.cpp
    src/86box/cpu/softfloat3e/f32_to_extF80.cpp
    src/86box/cpu/softfloat3e/f32_to_f128.cpp
    src/86box/cpu/softfloat3e/f32_to_f16.cpp
    src/86box/cpu/softfloat3e/f32_to_f64.cpp
    src/86box/cpu/softfloat3e/f32_to_i32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f32_to_i32.cpp
    src/86box/cpu/softfloat3e/f32_to_i64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f32_to_i64.cpp
    src/86box/cpu/softfloat3e/f32_to_ui32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f32_to_ui32.cpp
    src/86box/cpu/softfloat3e/f32_to_ui64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f32_to_ui64.cpp
    src/86box/cpu/softfloat3e/f64_addsub.cpp
    src/86box/cpu/softfloat3e/f64_class.cpp
    src/86box/cpu/softfloat3e/f64_compare.cpp
    src/86box/cpu/softfloat3e/f64_div.cpp
    src/86box/cpu/softfloat3e/f64_frc.cpp
    src/86box/cpu/softfloat3e/f64_getExp.cpp
    src/86box/cpu/softfloat3e/f64_getMant.cpp
    src/86box/cpu/softfloat3e/f64_minmax.cpp
    src/86box/cpu/softfloat3e/f64_mul.cpp
    src/86box/cpu/softfloat3e/f64_mulAdd.cpp
    src/86box/cpu/softfloat3e/f64_range.cpp
    src/86box/cpu/softfloat3e/f64_roundToInt.cpp
    src/86box/cpu/softfloat3e/f64_scalef.cpp
    src/86box/cpu/softfloat3e/f64_sqrt.cpp
    src/86box/cpu/softfloat3e/f64_to_extF80.cpp
    src/86box/cpu/softfloat3e/f64_to_f128.cpp
    src/86box/cpu/softfloat3e/f64_to_f16.cpp
    src/86box/cpu/softfloat3e/f64_to_f32.cpp
    src/86box/cpu/softfloat3e/f64_to_i32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f64_to_i32.cpp
    src/86box/cpu/softfloat3e/f64_to_i64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f64_to_i64.cpp
    src/86box/cpu/softfloat3e/f64_to_ui32_r_minMag.cpp
    src/86box/cpu/softfloat3e/f64_to_ui32.cpp
    src/86box/cpu/softfloat3e/f64_to_ui64_r_minMag.cpp
    src/86box/cpu/softfloat3e/f64_to_ui64.cpp
    src/86box/cpu/softfloat3e/fpatan.cpp
    src/86box/cpu/softfloat3e/fprem.cpp
    src/86box/cpu/softfloat3e/fsincos.cpp
    src/86box/cpu/softfloat3e/fyl2x.cpp
    src/86box/cpu/softfloat3e/i32_to_extF80.cpp
    src/86box/cpu/softfloat3e/i32_to_f128.cpp
    src/86box/cpu/softfloat3e/i32_to_f16.cpp
    src/86box/cpu/softfloat3e/i32_to_f32.cpp
    src/86box/cpu/softfloat3e/i32_to_f64.cpp
    src/86box/cpu/softfloat3e/i64_to_extF80.cpp
    src/86box/cpu/softfloat3e/i64_to_f128.cpp
    src/86box/cpu/softfloat3e/i64_to_f16.cpp
    src/86box/cpu/softfloat3e/i64_to_f32.cpp
    src/86box/cpu/softfloat3e/i64_to_f64.cpp
    src/86box/cpu/softfloat3e/isNaN.cpp
    src/86box/cpu/softfloat3e/isSignalingNaN.cpp
    src/86box/cpu/softfloat3e/poly.cpp
    src/86box/cpu/softfloat3e/primitives.cpp
    src/86box/cpu/softfloat3e/s_add128.cpp
    src/86box/cpu/softfloat3e/s_add256M.cpp
    src/86box/cpu/softfloat3e/s_addMagsExtF80.cpp
    src/86box/cpu/softfloat3e/s_addMagsF128.cpp
    src/86box/cpu/softfloat3e/s_addMagsF16.cpp
    src/86box/cpu/softfloat3e/s_addMagsF32.cpp
    src/86box/cpu/softfloat3e/s_addMagsF64.cpp
    src/86box/cpu/softfloat3e/s_approxRecip_1Ks.cpp
    src/86box/cpu/softfloat3e/s_approxRecipSqrt_1Ks.cpp
    src/86box/cpu/softfloat3e/s_approxRecipSqrt32_1.cpp
    src/86box/cpu/softfloat3e/s_commonNaNToExtF80UI.cpp
    src/86box/cpu/softfloat3e/s_commonNaNToF128UI.cpp
    src/86box/cpu/softfloat3e/s_commonNaNToF16UI.cpp
    src/86box/cpu/softfloat3e/s_commonNaNToF32UI.cpp
    src/86box/cpu/softfloat3e/s_commonNaNToF64UI.cpp
    src/86box/cpu/softfloat3e/s_countLeadingZeros16.cpp
    src/86box/cpu/softfloat3e/s_countLeadingZeros32.cpp
    src/86box/cpu/softfloat3e/s_countLeadingZeros64.cpp
    src/86box/cpu/softfloat3e/s_countLeadingZeros8.cpp
    src/86box/cpu/softfloat3e/s_eq128.cpp
    src/86box/cpu/softfloat3e/s_le128.cpp
    src/86box/cpu/softfloat3e/s_lt128.cpp
    src/86box/cpu/softfloat3e/s_mul128By32.cpp
    src/86box/cpu/softfloat3e/s_mul128To256M.cpp
    src/86box/cpu/softfloat3e/s_mul64ByShifted32To128.cpp
    src/86box/cpu/softfloat3e/s_mul64To128.cpp
    src/86box/cpu/softfloat3e/s_normRoundPackToExtF80.cpp
    src/86box/cpu/softfloat3e/s_normRoundPackToF128.cpp
    src/86box/cpu/softfloat3e/s_normRoundPackToF16.cpp
    src/86box/cpu/softfloat3e/s_normRoundPackToF32.cpp
    src/86box/cpu/softfloat3e/s_normRoundPackToF64.cpp
    src/86box/cpu/softfloat3e/s_normSubnormalExtF80Sig.cpp
    src/86box/cpu/softfloat3e/s_normSubnormalF128Sig.cpp
    src/86box/cpu/softfloat3e/s_normSubnormalF16Sig.cpp
    src/86box/cpu/softfloat3e/s_normSubnormalF32Sig.cpp
    src/86box/cpu/softfloat3e/s_normSubnormalF64Sig.cpp
    src/86box/cpu/softfloat3e/s_packToExtF80.cpp
    src/86box/cpu/softfloat3e/s_propagateNaNExtF80UI.cpp
    src/86box/cpu/softfloat3e/s_propagateNaNF128UI.cpp
    src/86box/cpu/softfloat3e/s_propagateNaNF16UI.cpp
    src/86box/cpu/softfloat3e/s_propagateNaNF32UI.cpp
    src/86box/cpu/softfloat3e/s_propagateNaNF64UI.cpp
    src/86box/cpu/softfloat3e/s_roundPackToExtF80.cpp
    src/86box/cpu/softfloat3e/s_roundPackToF128.cpp
    src/86box/cpu/softfloat3e/s_roundPackToF16.cpp
    src/86box/cpu/softfloat3e/s_roundPackToF32.cpp
    src/86box/cpu/softfloat3e/s_roundPackToF64.cpp
    src/86box/cpu/softfloat3e/s_roundToI32.cpp
    src/86box/cpu/softfloat3e/s_roundToI64.cpp
    src/86box/cpu/softfloat3e/s_roundToUI32.cpp
    src/86box/cpu/softfloat3e/s_roundToUI64.cpp
    src/86box/cpu/softfloat3e/s_shiftRightJam128.cpp
    src/86box/cpu/softfloat3e/s_shiftRightJam128Extra.cpp
    src/86box/cpu/softfloat3e/s_shiftRightJam256M.cpp
    src/86box/cpu/softfloat3e/s_shiftRightJam32.cpp
    src/86box/cpu/softfloat3e/s_shiftRightJam64.cpp
    src/86box/cpu/softfloat3e/s_shiftRightJam64Extra.cpp
    src/86box/cpu/softfloat3e/s_shortShiftLeft128.cpp
    src/86box/cpu/softfloat3e/s_shortShiftRight128.cpp
    src/86box/cpu/softfloat3e/s_shortShiftRightJam64.cpp
    src/86box/cpu/softfloat3e/s_shortShiftRightJam64Extra.cpp
    src/86box/cpu/softfloat3e/s_sub128.cpp
    src/86box/cpu/softfloat3e/s_sub256M.cpp
    src/86box/cpu/softfloat3e/s_subMagsExtF80.cpp
    src/86box/cpu/softfloat3e/s_subMagsF128.cpp
    src/86box/cpu/softfloat3e/s_subMagsF16.cpp
    src/86box/cpu/softfloat3e/s_subMagsF32.cpp
    src/86box/cpu/softfloat3e/s_subMagsF64.cpp
    src/86box/cpu/softfloat3e/softfloat-specialize.cpp
    src/86box/cpu/softfloat3e/ui32_to_extF80.cpp
    src/86box/cpu/softfloat3e/ui32_to_f128.cpp
    src/86box/cpu/softfloat3e/ui32_to_f16.cpp
    src/86box/cpu/softfloat3e/ui32_to_f32.cpp
    src/86box/cpu/softfloat3e/ui32_to_f64.cpp
    src/86box/cpu/softfloat3e/ui64_to_extF80.cpp
    src/86box/cpu/softfloat3e/ui64_to_f128.cpp
    src/86box/cpu/softfloat3e/ui64_to_f16.cpp
    src/86box/cpu/softfloat3e/ui64_to_f32.cpp
    src/86box/cpu/softfloat3e/ui64_to_f64.cpp
    src/86box/cpu/vx0_biu.cpp
    src/86box/cpu/vx0.cpp
    src/86box/cpu/x86_ops_mmx.cpp
    src/86box/cpu/x86.cpp
    src/86box/cpu/x86seg_2386.cpp
    src/86box/cpu/x86seg_common.cpp
    src/86box/cpu/x86seg.cpp
    src/86box/cpu/x87_ferr.cpp
    src/86box/cpu/x87_timings.cpp
    src/86box/cpu/x87.cpp
    src/86box/device/kbc_at_dev.cpp
    src/86box/device/kbc_at.cpp
    src/86box/device/keyboard_at.cpp
    src/86box/device/keyboard.cpp
    src/86box/device/mouse_ps2.cpp
    src/86box/device/mouse_serial.cpp
    src/86box/device/serial.cpp
    src/86box/dma.cpp
    src/86box/mem/mem.cpp
    src/86box/mem/mmu_2386.cpp
    src/86box/mem/smram.cpp
    src/86box/nmi.cpp
    src/86box/nvr_at.cpp
    src/86box/nvr.cpp
    src/86box/pic.cpp
    src/86box/pit_fast.cpp
    src/86box/pit.cpp
    src/86box/port_6x.cpp
    src/86box/ppi.cpp
    src/86box/sound/snd_cms.cpp
    src/86box/sound/snd_mpu401.cpp
    src/86box/sound/snd_opl2_nuked.cpp
    src/86box/sound/snd_opl3_nuked.cpp
    src/86box/sound/snd_sb_dsp.cpp
    src/86box/sound/snd_sb.cpp
    src/86box/sound/snd_speaker.cpp
    src/86box/timer.cpp
    src/86box/utils/fifo.cpp
)

set(AMIBERRY_86BOX_DYNAREC_SOURCES
    src/86box/codegen/codegen.cpp
    src/86box/codegen/codegen_accumulate.cpp
    src/86box/codegen/codegen_allocator.cpp
    src/86box/codegen/codegen_block.cpp
    src/86box/codegen/codegen_ir.cpp
    src/86box/codegen/codegen_ops.cpp
    src/86box/codegen/codegen_ops_3dnow.cpp
    src/86box/codegen/codegen_ops_arith.cpp
    src/86box/codegen/codegen_ops_branch.cpp
    src/86box/codegen/codegen_ops_fpu_arith.cpp
    src/86box/codegen/codegen_ops_fpu_constant.cpp
    src/86box/codegen/codegen_ops_fpu_loadstore.cpp
    src/86box/codegen/codegen_ops_fpu_misc.cpp
    src/86box/codegen/codegen_ops_helpers.cpp
    src/86box/codegen/codegen_ops_jump.cpp
    src/86box/codegen/codegen_ops_logic.cpp
    src/86box/codegen/codegen_ops_misc.cpp
    src/86box/codegen/codegen_ops_mmx_arith.cpp
    src/86box/codegen/codegen_ops_mmx_cmp.cpp
    src/86box/codegen/codegen_ops_mmx_loadstore.cpp
    src/86box/codegen/codegen_ops_mmx_logic.cpp
    src/86box/codegen/codegen_ops_mmx_pack.cpp
    src/86box/codegen/codegen_ops_mmx_shift.cpp
    src/86box/codegen/codegen_ops_mov.cpp
    src/86box/codegen/codegen_ops_rep.cpp
    src/86box/codegen/codegen_ops_shift.cpp
    src/86box/codegen/codegen_ops_stack.cpp
    src/86box/codegen/codegen_reg.cpp
    src/86box/cpu/386_dynarec_ops.cpp
    src/86box/cpu/codegen_timing_486.cpp
    src/86box/cpu/codegen_timing_686.cpp
    src/86box/cpu/codegen_timing_common.cpp
    src/86box/cpu/codegen_timing_k6.cpp
    src/86box/cpu/codegen_timing_p6.cpp
    src/86box/cpu/codegen_timing_pentium.cpp
    src/86box/cpu/codegen_timing_winchip.cpp
    src/86box/cpu/codegen_timing_winchip2.cpp
)

set(AMIBERRY_86BOX_DYNAREC_AARCH64_SOURCES
    src/86box/codegen/codegen_backend_arm64.cpp
    src/86box/codegen/codegen_backend_arm64_imm.cpp
    src/86box/codegen/codegen_backend_arm64_ops.cpp
    src/86box/codegen/codegen_backend_arm64_uops.cpp
)

set(AMIBERRY_86BOX_DYNAREC_X86_64_SOURCES
    src/86box/codegen/codegen_backend_x86-64.cpp
    src/86box/codegen/codegen_backend_x86-64_ops.cpp
    src/86box/codegen/codegen_backend_x86-64_ops_sse.cpp
    src/86box/codegen/codegen_backend_x86-64_uops.cpp
)

set(AMIBERRY_86BOX_GLUE_SOURCES
    src/86box/winuae/86box_glue.cpp
    src/86box/winuae/86box_machine.cpp
)

set(AMIBERRY_86BOX_DRACO_SOURCES
    src/86box/winuae/keyboard_at_draco.cpp
)

set(AMIBERRY_86BOX_VIDEO_SOURCES
    src/86box/device/i2c.cpp
    src/86box/device/i2c_gpio.cpp
    src/86box/mem/i2c_eeprom.cpp
    src/86box/mem/nmc93cxx.cpp
    src/86box/video/clockgen/vid_clockgen_av9194.cpp
    src/86box/video/clockgen/vid_clockgen_ch9204.cpp
    src/86box/video/clockgen/vid_clockgen_icd2061.cpp
    src/86box/video/clockgen/vid_clockgen_ics2494.cpp
    src/86box/video/ramdac/vid_ramdac_att20c49x.cpp
    src/86box/video/ramdac/vid_ramdac_att2xc498.cpp
    src/86box/video/ramdac/vid_ramdac_bt48x.cpp
    src/86box/video/ramdac/vid_ramdac_ibm_rgb528.cpp
    src/86box/video/ramdac/vid_ramdac_mu9c1880.cpp
    src/86box/video/ramdac/vid_ramdac_sc1148x.cpp
    src/86box/video/ramdac/vid_ramdac_sc1502x.cpp
    src/86box/video/ramdac/vid_ramdac_sdac.cpp
    src/86box/video/ramdac/vid_ramdac_stg1702.cpp
    src/86box/video/ramdac/vid_ramdac_tvp3026.cpp
    src/86box/video/vid_cl54xx.cpp
    src/86box/video/vid_ddc.cpp
    src/86box/video/vid_et4000.cpp
    src/86box/video/vid_et4000w32.cpp
    src/86box/video/vid_mga.cpp
    src/86box/video/vid_s3.cpp
    src/86box/video/vid_s3_virge.cpp
    src/86box/video/vid_svga.cpp
    src/86box/video/vid_svga_render.cpp
    src/86box/video/vid_voodoo.cpp
    src/86box/video/vid_voodoo_banshee.cpp
    src/86box/video/vid_voodoo_banshee_blitter.cpp
    src/86box/video/vid_voodoo_blitter.cpp
    src/86box/video/vid_voodoo_display.cpp
    src/86box/video/vid_voodoo_fb.cpp
    src/86box/video/vid_voodoo_fifo.cpp
    src/86box/video/vid_voodoo_reg.cpp
    src/86box/video/vid_voodoo_render.cpp
    src/86box/video/vid_voodoo_setup.cpp
    src/86box/video/vid_voodoo_texture.cpp
    src/86box/winuae/vid_inmos.cpp
    src/86box/winuae/vid_ncr.cpp
    src/86box/winuae/vid_permedia2.cpp
    src/86box/winuae/vid_ramdac_bt482.cpp
)

set(AMIBERRY_86BOX_VIDEO_GLUE_SOURCES
    src/86box/winuae/86box_video.cpp
)

if(USE_86BOX)
    if(USE_86BOX_DYNAREC)
        message(STATUS "86Box support enabled (x86 recompiler backend: ${AMIBERRY_86BOX_DYNAREC_BACKEND})")
    else()
        message(STATUS "86Box support enabled (interpreter only)")
    endif()
else()
    message(STATUS "86Box support disabled")
endif()
if(USE_86BOX)
    set(AMIBERRY_86BOX_DEFINITIONS UAE=)
    set(AMIBERRY_86BOX_OBJECT_SOURCES ${AMIBERRY_86BOX_X86_SOURCES})
    if(USE_86BOX_DYNAREC)
        # USE_DYNAREC changes 86Box's CPU state, so it applies to every file
        # that includes 86Box's cpu.h.
        list(APPEND AMIBERRY_86BOX_OBJECT_SOURCES ${AMIBERRY_86BOX_DYNAREC_SOURCES})
        if(AMIBERRY_86BOX_DYNAREC_BACKEND STREQUAL "aarch64")
            list(APPEND AMIBERRY_86BOX_OBJECT_SOURCES ${AMIBERRY_86BOX_DYNAREC_AARCH64_SOURCES})
        else()
            list(APPEND AMIBERRY_86BOX_OBJECT_SOURCES ${AMIBERRY_86BOX_DYNAREC_X86_64_SOURCES})
        endif()
        list(APPEND AMIBERRY_86BOX_DEFINITIONS USE_DYNAREC)
    endif()

    add_library(amiberry_86box OBJECT ${AMIBERRY_86BOX_OBJECT_SOURCES})
    set_target_properties(amiberry_86box PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS ON)
    target_include_directories(amiberry_86box PRIVATE
            "${CMAKE_SOURCE_DIR}/src/86box/winuae/include"
            "${CMAKE_SOURCE_DIR}/src/86box/include"
            "${CMAKE_SOURCE_DIR}/src/86box/cpu"
            "${CMAKE_SOURCE_DIR}/src/86box/cpu/softfloat3e"
            "${CMAKE_SOURCE_DIR}/src/86box/codegen"
            "${CMAKE_SOURCE_DIR}/src/86box"
    )
    target_compile_definitions(amiberry_86box PRIVATE ${AMIBERRY_86BOX_DEFINITIONS})
    # C11's _Static_assert, which older GCCs don't take in C++
    target_compile_definitions(amiberry_86box PRIVATE $<$<CXX_COMPILER_ID:GNU>:_Static_assert=static_assert>)
    target_compile_options(amiberry_86box PRIVATE -w)

    # The video cards get their memory, I/O, timer and device calls renamed
    # to the RTG versions in 86box_video.cpp.
    add_library(amiberry_86box_video OBJECT ${AMIBERRY_86BOX_VIDEO_SOURCES})
    set_target_properties(amiberry_86box_video PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS ON)
    target_include_directories(amiberry_86box_video PRIVATE
            "${CMAKE_SOURCE_DIR}/src/86box/winuae/include"
            "${CMAKE_SOURCE_DIR}/src/86box/include"
            "${CMAKE_SOURCE_DIR}/src/86box/cpu"
            "${CMAKE_SOURCE_DIR}/src/86box"
    )
    target_compile_definitions(amiberry_86box_video PRIVATE ${AMIBERRY_86BOX_DEFINITIONS})
    # C11's _Static_assert, which older GCCs don't take in C++
    target_compile_definitions(amiberry_86box_video PRIVATE $<$<CXX_COMPILER_ID:GNU>:_Static_assert=static_assert>)
    target_compile_options(amiberry_86box_video PRIVATE -w
            -include "${CMAKE_SOURCE_DIR}/src/86box/winuae/86box_video_redirect.h")

    if(ANDROID)
        # The core library is shared on Android; its objects must be PIC.
        set_target_properties(amiberry_86box amiberry_86box_video PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()

    # The glue and the core files that drive the PC include 86Box headers by
    # their 86box/... path.
    set_source_files_properties(
            "${CMAKE_SOURCE_DIR}/src/86box/winuae/86box_glue.cpp"
            "${CMAKE_SOURCE_DIR}/src/86box/winuae/86box_machine.cpp"
            "${CMAKE_SOURCE_DIR}/src/86box/winuae/keyboard_at_draco.cpp"
            "${CMAKE_SOURCE_DIR}/src/86box/winuae/86box_video.cpp"
            "${CMAKE_SOURCE_DIR}/src/atonce.cpp"
            "${CMAKE_SOURCE_DIR}/src/x86.cpp"
            "${CMAKE_SOURCE_DIR}/src/draco.cpp"
            PROPERTIES INCLUDE_DIRECTORIES
            "${CMAKE_SOURCE_DIR}/src/86box/winuae;${CMAKE_SOURCE_DIR}/src/86box/winuae/include;${CMAKE_SOURCE_DIR}/src/86box/include"
            COMPILE_DEFINITIONS "${AMIBERRY_86BOX_DEFINITIONS}")

    # The glue compiles with the core; the imported 86Box objects come in as
    # target objects. (SOURCE_FILES is in scope: this file is include()d from
    # cmake/SourceFiles.cmake before the target is created.)
    list(APPEND SOURCE_FILES
            ${AMIBERRY_86BOX_GLUE_SOURCES}
            ${AMIBERRY_86BOX_DRACO_SOURCES}
            ${AMIBERRY_86BOX_VIDEO_GLUE_SOURCES}
            $<TARGET_OBJECTS:amiberry_86box>
            $<TARGET_OBJECTS:amiberry_86box_video>
    )
endif()
