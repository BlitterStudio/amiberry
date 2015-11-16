@ Some functions and tests to increase performance in drawing.cpp and custom.cpp

.arm

.global TRACE_Start
.global TRACE_mem
.global copy_screen_16bit_swap
.global copy_screen_32bit_to_16bit_neon
.global ARM_doline_n1
.global NEON_doline_n2
.global NEON_doline_n3
.global NEON_doline_n4
.global NEON_doline_n6
.global NEON_doline_n8

.text

.align 8

TRACE_Start:
  stmdb   sp!, {r0, r1}
  stmdb   sp!, {r1}
  ldr     r1, =TRACE_mem
  str     r0, [r1]
  add     r1, r1, #4
  ldmia   sp!, {r0}
  str     r0, [r1]
  add     r1, r1, #4
  str     r2, [r1]
  add     r1, r1, #4
  str     r3, [r1]

  add     r1, r1, #4
  str     r4, [r1]
  add     r1, r1, #4
  str     r5, [r1]
  add     r1, r1, #4
  str     r6, [r1]
  add     r1, r1, #4
  str     r7, [r1]

  add     r1, r1, #4
  str     r8, [r1]
  add     r1, r1, #4
  str     r9, [r1]
  add     r1, r1, #4
  str     r10, [r1]
  add     r1, r1, #4
  str     r11, [r1]

  add     r1, r1, #4
  str     r12, [r1]
  add     r1, r1, #4
  str     r13, [r1]
  add     r1, r1, #4
  str     r14, [r1]
  add     r1, r1, #4
  str     r15, [r1]

  ldmia   sp!, {r0, r1}
  bx      lr
    

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


@----------------------------------------------------------------
@ ARM_doline_n1
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void ARM_doline_n1(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
ARM_doline_n1:
  stmdb     sp!, {r4-r9, lr}

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r3, r3, r2           @ real_bplpt[0]

  ldr       r4, =Lookup_doline_n1

  sub       r3, r3, #4
  
ARM_doline_n1_loop:
  ldr       r2, [r3, #4]!
@  add       r3, r3, #4

  ubfx      r5, r2, #28, #4
  add       r5, r4, r5, lsl #2
  ldr       r6, [r5]
  
  ubfx      r5, r2, #24, #4
  add       r5, r4, r5, lsl #2
  ldr       r7, [r5]

  ubfx      r5, r2, #20, #4
  add       r5, r4, r5, lsl #2
  ldr       r8, [r5]

  ubfx      r5, r2, #16, #4
  add       r5, r4, r5, lsl #2
  ldr       r9, [r5]
  stmia     r0!, {r6-r9}
  
  ubfx      r5, r2, #12, #4
  add       r5, r4, r5, lsl #2
  ldr       r6, [r5]

  ubfx      r5, r2, #8, #4
  add       r5, r4, r5, lsl #2
  ldr       r7, [r5]

  ubfx      r5, r2, #4, #4
  add       r5, r4, r5, lsl #2
  ldr       r8, [r5]

  ubfx      r5, r2, #0, #4
  add       r5, r4, r5, lsl #2
  ldr       r9, [r5]
  stmia     r0!, {r6-r9}

  subs      r1, r1, #1
  bgt       ARM_doline_n1_loop
  
  ldmia     sp!, {r4-r9, pc}


.align 8

@----------------------------------------------------------------
@ NEON_doline_n2
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n2(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n2:

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n2_loop:
  @ Load data as early as possible
  vldmia    r2!, {d4}
  vldmia    r3!, {d6}

@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2		  @ tmp = b >> shift
  vand.8    d2, d16, d19     @ a = tmp & mask
  vand.8    d6, d6, d19     @ b = b & mask
@      MERGE_0(b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2		  @ tmp = b >> shift
  vand.8    d0, d16, d19     @ a = tmp & mask
  vand.8    d4, d4, d19     @ b = b & mask

@      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4		  @ tmp = b >> shift
  vand.8    d3, d16, d20     @ a = tmp & mask
  vand.8    d2, d2, d20     @ b = b & mask
@      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4			@ tmp = b >> shift
  vand.8    d1, d16, d20     @ a = tmp & mask
  vand.8    d0, d0, d20     @ b = b & mask
@      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4			@ tmp = b >> shift
  vand.8    d7, d16, d20     @ a = tmp & mask
  vand.8    d6, d6, d20     @ b = b & mask
@      MERGE_0(b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4			@ tmp = b >> shift
  vand.8    d5, d16, d20     @ a = tmp & mask
  vand.8    d4, d4, d20     @ b = b & mask
  
  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  bxeq      lr
   
  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  vzip.8    d7, d5
  vzip.8    d6, d4
  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n2_loop
  
NEON_doline_n2_exit:
  bx        lr


.align 8

@----------------------------------------------------------------
@ NEON_doline_n3
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n3(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n3:
  stmdb     sp!, {r4, lr}

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200

  @ Load data as early as possible
  vldmia    r4!, {d0}

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n3_loop:
@ Load from real_bplpt (now loaded earlier)
@  vld1.8    d0, [r4]!
@  vld1.8    d4, [r2]!
@  vld1.8    d6, [r3]!

  @ Load data as early as possible
  vldmia    r2!, {d4}
  vldmia    r3!, {d6}

@      MERGE_0(b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1		  @ tmp = b >> shift
  vand.8    d2, d16, d18     @ a = tmp & mask
  vand.8    d0, d0, d18     @ b = b & mask

@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4		  @ tmp = b >> shift
  vand.8    d3, d16, d20     @ a = tmp & mask
  vand.8    d2, d2, d20     @ b = b & mask
@      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4			@ tmp = b >> shift
  vand.8    d1, d16, d20     @ a = tmp & mask
  vand.8    d0, d0, d20     @ b = b & mask
@      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4			@ tmp = b >> shift
  vand.8    d7, d16, d20     @ a = tmp & mask
  vand.8    d6, d6, d20     @ b = b & mask
@      MERGE_0(b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4			@ tmp = b >> shift
  vand.8    d5, d16, d20     @ a = tmp & mask
  vand.8    d4, d4, d20     @ b = b & mask
  
  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4, pc}
  
  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  @ Load next data (if needed) as early as possible
  vldmiagt  r4!, {d0}

  vzip.8    d7, d5
  vzip.8    d6, d4
  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n3_loop
  
NEON_doline_n3_exit:
  ldmia     sp!, {r4, pc}


.align 8

@----------------------------------------------------------------
@ NEON_doline_n4
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n4(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n4:
  stmdb     sp!, {r4-r5, lr}

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200
  add       r5, r4, #200

  @ Load data as early as possible
  vldmia    r4!, {d0}
  vldmia    r5!, {d2}

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n4_loop:
@ Load from real_bplpt (now loaded earlier)
@  vld1.8    d0, [r4]!
@  vld1.8    d2, [r5]!
@  vld1.8    d4, [r2]!
@  vld1.8    d6, [r3]!

  @ Load data as early as possible
  vldmia    r2!, {d4}

@      MERGE (b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1      @ tmpb = b >> shift
  vshl.u8   d17, d2, #1      @ tmpa = a << shift

  vldmia    r3!, {d6}

  vbit.u8   d2, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d18     @ b = b and bit set from tmpa if mask is false
@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4		  @ tmp = b >> shift
  vand.8    d3, d16, d20     @ a = tmp & mask
  vand.8    d2, d2, d20     @ b = b & mask
@      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4			@ tmp = b >> shift
  vand.8    d1, d16, d20     @ a = tmp & mask
  vand.8    d0, d0, d20     @ b = b & mask
@      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4			@ tmp = b >> shift
  vand.8    d7, d16, d20     @ a = tmp & mask
  vand.8    d6, d6, d20     @ b = b & mask
@      MERGE_0(b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4			@ tmp = b >> shift
  vand.8    d5, d16, d20     @ a = tmp & mask
  vand.8    d4, d4, d20     @ b = b & mask
  
  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4-r5, pc}
  
  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  @ Load next data (if needed) as early as possible
  vldmiagt  r4!, {d0}

  vzip.8    d7, d5
  vzip.8    d6, d4

  vldmiagt  r5!, {d2}

  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n4_loop
  
NEON_doline_n4_exit:
  ldmia     sp!, {r4-r5, pc}


.align 8

@----------------------------------------------------------------
@ NEON_doline_n6
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n6(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n6:
  stmdb     sp!, {r4-r7, lr}
  
  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200
  add       r5, r4, #200
  add       r6, r5, #200
  add       r7, r6, #200
  
@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n6_loop:
  @ Load data as early as possible
  vldmia    r6!, {d5}
  vldmia    r7!, {d7}

  @ Load data as early as possible
  vldmia    r4!, {d0}
@      MERGE (b2, b3, 0x55555555, 1);
  vshr.u8   d16, d5, #1      @ tmpb = b >> shift
  vshl.u8   d17, d7, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r5!, {d2}
  vbit.u8   d7, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d5, d17, d18     @ b = b and bit set from tmpa if mask is false
  @ Load data as early as possible
  vldmia    r2!, {d4}
@      MERGE (b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1      @ tmpb = b >> shift
  vshl.u8   d17, d2, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r3!, {d6}
  vbit.u8   d2, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d18     @ b = b and bit set from tmpa if mask is false
@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b0, b2, 0x33333333, 2);
  vshr.u8   d16, d7, #2		  @ tmp = b >> shift
  vand.8    d3, d16, d19     @ a = tmp & mask
  vand.8    d7, d7, d19     @ b = b & mask
@      MERGE_0(b1, b3, 0x33333333, 2);
  vshr.u8   d16, d5, #2		  @ tmp = b >> shift
  vand.8    d1, d16, d19     @ a = tmp & mask
  vand.8    d5, d5, d19     @ b = b & mask
@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE (b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4      @ tmpb = b >> shift
  vshl.u8   d17, d3, #4      @ tmpa = a << shift
  vbit.u8   d3, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d2, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4      @ tmpb = b >> shift
  vshl.u8   d17, d1, #4      @ tmpa = a << shift
  vbit.u8   d1, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4      @ tmpb = b >> shift
  vshl.u8   d17, d7, #4      @ tmpa = a << shift
  vbit.u8   d7, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4      @ tmpb = b >> shift
  vshl.u8   d17, d5, #4      @ tmpa = a << shift
  vbit.u8   d5, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d20     @ b = b and bit set from tmpa if mask is false

  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4-r7, pc}

  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  vzip.8    d7, d5
  vzip.8    d6, d4
  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n6_loop
  
NEON_doline_n6_exit:
  ldmia     sp!, {r4-r7, pc}
 

.align 8

@----------------------------------------------------------------
@ NEON_doline_n8
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n8(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n8:
  stmdb     sp!, {r4-r9, lr}
  
  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200
  add       r5, r4, #200
  add       r6, r5, #200
  add       r7, r6, #200
  add       r8, r7, #200
  add       r9, r8, #200
  
  @ Load data as early as possible
  vldmia    r8!, {d1}
  vldmia    r9!, {d3}

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n8_loop:
  @ Load data as early as possible
  vldmia    r6!, {d5}
@      MERGE (b0, b1, 0x55555555, 1);
  vshr.u8   d16, d1, #1      @ tmpb = b >> shift
  vshl.u8   d17, d3, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r7!, {d7}
  vbit.u8   d3, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d1, d17, d18     @ b = b and bit set from tmpa if mask is false
  @ Load data as early as possible
  vldmia    r4!, {d0}
@      MERGE (b2, b3, 0x55555555, 1);
  vshr.u8   d16, d5, #1      @ tmpb = b >> shift
  vshl.u8   d17, d7, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r5!, {d2}
  vbit.u8   d7, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d5, d17, d18     @ b = b and bit set from tmpa if mask is false
  @ Load data as early as possible
  vldmia    r2!, {d4}
@      MERGE (b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1      @ tmpb = b >> shift
  vshl.u8   d17, d2, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r3!, {d6}
  vbit.u8   d2, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d18     @ b = b and bit set from tmpa if mask is false
@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE (b0, b2, 0x33333333, 2);
  vshr.u8   d16, d7, #2      @ tmpb = b >> shift
  vshl.u8   d17, d3, #2      @ tmpa = a << shift
  vbit.u8   d3, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d7, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b1, b3, 0x33333333, 2);
  vshr.u8   d16, d5, #2      @ tmpb = b >> shift
  vshl.u8   d17, d1, #2      @ tmpa = a << shift
  vbit.u8   d1, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d5, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE (b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4      @ tmpb = b >> shift
  vshl.u8   d17, d3, #4      @ tmpa = a << shift
  vbit.u8   d3, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d2, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4      @ tmpb = b >> shift
  vshl.u8   d17, d1, #4      @ tmpa = a << shift
  vbit.u8   d1, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4      @ tmpb = b >> shift
  vshl.u8   d17, d7, #4      @ tmpa = a << shift
  vbit.u8   d7, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4      @ tmpb = b >> shift
  vshl.u8   d17, d5, #4      @ tmpa = a << shift
  vbit.u8   d5, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d20     @ b = b and bit set from tmpa if mask is false

  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4-r9, pc}

  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  @ Load data as early as possible
  vldmiagt  r8!, {d1}
  
  vzip.8    d7, d5
  vzip.8    d6, d4

  @ Load data as early as possible
  vldmiagt  r9!, {d3}

  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n8_loop
  
NEON_doline_n8_exit:
  ldmia     sp!, {r4-r9, pc}


.data

.align 8

Lookup_doline_n1:
  .long 0x00000000, 0x01000000, 0x00010000, 0x01010000
  .long 0x00000100, 0x01000100, 0x00010100, 0x01010100
  .long 0x00000001, 0x01000001, 0x00010001, 0x01010001
  .long 0x00000101, 0x01000101, 0x00010101, 0x01010101

TRACE_mem:
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
  .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
