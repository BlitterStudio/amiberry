@ Some functions and tests to increase performance in drawing.cpp and custom.cpp

.arm

.global copy_screen_8bit
.global copy_screen_16bit_swap
.global copy_screen_32bit_to_16bit_neon

.text

.align 8


@----------------------------------------------------------------
@ copy_screen_8bit
@
@ r0: uae_u8   *dst
@ r1: uae_u8   *src
@ r2: int      bytes always a multiple of 64: even number of lines, number of pixel per line is multiple of 32 (320, 640, 800, 1024, 1152, 1280)
@ r3: uae_u32  *clut
@
@ void copy_screen_8bit(uae_u8 *dst, uae_u8 *src, int bytes, uae_u32 *clut);
@
@----------------------------------------------------------------
copy_screen_8bit:
  stmdb     sp!, {r4-r6, lr}
copy_screen_8bit_loop:
  pld       [r1, #192]
  mov       lr, #64
copy_screen_8bit_loop_2:
  ldr       r4, [r1], #4
  and       r5, r4, #255
  ldr       r6, [r3, r5, lsl #2]
  ubfx      r5, r4, #8, #8
  strh      r6, [r0], #2
  ldr       r6, [r3, r5, lsl #2]
  ubfx      r5, r4, #16, #8
  strh      r6, [r0], #2
  ldr       r6, [r3, r5, lsl #2]
  ubfx      r5, r4, #24, #8
  strh      r6, [r0], #2
  ldr       r6, [r3, r5, lsl #2]
  subs      lr, lr, #4
  strh      r6, [r0], #2
  bgt       copy_screen_8bit_loop_2
  subs      r2, r2, #64
  bgt       copy_screen_8bit_loop
  ldmia     sp!, {r4-r6, pc}


@----------------------------------------------------------------
@ copy_screen_16bit_swap
@
@ r0: uae_u8   *dst
@ r1: uae_u8   *src
@ r2: int      bytes always a multiple of 128: even number of lines, 2 bytes per pixel, number of pixel per line is multiple of 32 (320, 640, 800, 1024, 1152, 1280)
@
@ void copy_screen_16bit_swap(uae_u8 *dst, uae_u8 *src, int bytes);
@
@----------------------------------------------------------------
copy_screen_16bit_swap:
  pld       [r1, #192]
  vldmia    r1!, {q8, q9}
  vrev16.8  q8, q8
  vldmia    r1!, {q10}
  vrev16.8  q9, q9
  vldmia    r1!, {q11}
  vrev16.8  q10, q10
  vldmia    r1!, {q12}
  vrev16.8  q11, q11
  vldmia    r1!, {q13}
  vrev16.8  q12, q12
  vldmia    r1!, {q14}
  vrev16.8  q13, q13
  vldmia    r1!, {q15}
  vrev16.8  q14, q14
  vrev16.8  q15, q15
  subs      r2, r2, #128  @ we handle 16 * 8 bytes per loop
  vstmia    r0!, {q8-q15}
  bne       copy_screen_16bit_swap
  bx        lr


@----------------------------------------------------------------
@ copy_screen_32bit_to_16bit_neon
@
@ r0: uae_u8   *dst - Format (bits): rrrr rggg gggb bbbb
@ r1: uae_u8   *src - Format (bytes) in memory rgba
@ r2: int      bytes
@
@ void copy_screen_32bit_to_16bit_neon(uae_u8 *dst, uae_u8 *src, int bytes);
@
@----------------------------------------------------------------
copy_screen_32bit_to_16bit_neon:
  pld       [r1, #192]
  vld4.8    {d18-d21}, [r1]!
  vld4.8    {d22-d25}, [r1]!
  vswp      d19, d22
  vswp      d21, d24         @ -> q9=r, q10=b, q11=g, q12=a
  vsri.i8   q9, q11, #5      @ q9:  rrrr rggg
  vshr.u8   q8, q10, #3      @ q8:  000b bbbb
  vshr.u8   q11, q11, #2     @ q11: 00gg gggg
  vsli.i8   q8, q11, #5      @ q8:  gggb bbbb
  vswp      d17, d18
  subs      r2, r2, #64      @ processd 4 (bytes per pixel) * 16 (pixel)
  vst2.8    {d16-d17}, [r0]!
  vst2.8    {d18-d19}, [r0]!
  bne       copy_screen_32bit_to_16bit_neon
  bx        lr

