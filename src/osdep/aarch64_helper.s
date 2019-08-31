// Some functions and tests to increase performance in drawing.cpp and custom.cpp

//.arm

.global save_host_fp_regs
.global restore_host_fp_regs
.global copy_screen_8bit
.global copy_screen_16bit_swap
.global copy_screen_32bit_to_16bit
.global ARM_doline_n1
.global NEON_doline_n2
.global NEON_doline_n3
.global NEON_doline_n4
.global NEON_doline_n5
.global NEON_doline_n6
.global NEON_doline_n7
.global NEON_doline_n8

.text

.align 8

//----------------------------------------------------------------
// save_host_fp_regs
//----------------------------------------------------------------
save_host_fp_regs:
  st4  {v7.2D-v10.2D}, [x0], #64
  st4  {v11.2D-v14.2D}, [x0], #64
  st1  {v15.2D}, [x0], #16
  ret
  
//----------------------------------------------------------------
// restore_host_fp_regs
//----------------------------------------------------------------
restore_host_fp_regs:
  ld4  {v7.2D-v10.2D}, [x0], #64
  ld4  {v11.2D-v14.2D}, [x0], #64
  ld1  {v15.2D}, [x0], #16
  ret


//----------------------------------------------------------------
// copy_screen_8bit
//
// x0: uae_u8   *dst
// x1: uae_u8   *src
// x2: int      bytes always a multiple of 64: even number of lines, number of pixel per line is multiple of 32 (320, 640, 800, 1024, 1152, 1280)
// x3: uae_u32  *clut
//
// void copy_screen_8bit(uae_u8 *dst, uae_u8 *src, int bytes, uae_u32 *clut);
//
//----------------------------------------------------------------
copy_screen_8bit:
  mov       x7, #64
copy_screen_8bit_loop:
  ldrsw     x4, [x1], #4
  ubfx      x5, x4, #0, #8
  ldrsw     x6, [x3, x5, lsl #2]
  ubfx      x5, x4, #8, #8
  strh      w6, [x0], #2
  ldrsw     x6, [x3, x5, lsl #2]
  ubfx      x5, x4, #16, #8
  strh      w6, [x0], #2
  ldrsw     x6, [x3, x5, lsl #2]
  ubfx      x5, x4, #24, #8
  strh      w6, [x0], #2
  ldrsw     x6, [x3, x5, lsl #2]
  subs      x7, x7, #4
  strh      w6, [x0], #2
  bgt       copy_screen_8bit_loop
  subs      x2, x2, #64
  bgt       copy_screen_8bit
  ret
  

//----------------------------------------------------------------
// copy_screen_16bit_swap
//
// x0: uae_u8   *dst
// x1: uae_u8   *src
// x2: int      bytes always a multiple of 128: even number of lines, 2 bytes per pixel, number of pixel per line is multiple of 32 (320, 640, 800, 1024, 1152, 1280)
//
// void copy_screen_16bit_swap(uae_u8 *dst, uae_u8 *src, int bytes);
//
//----------------------------------------------------------------
copy_screen_16bit_swap:
  ld4       {v0.2D-v3.2D}, [x1], #64
  rev16     v0.16b, v0.16b
  rev16     v1.16b, v1.16b
  rev16     v2.16b, v2.16b
  rev16     v3.16b, v3.16b
  subs      w2, w2, #64
  st4       {v0.2D-v3.2D}, [x0], #64
  bne copy_screen_16bit_swap
  ret
  

//----------------------------------------------------------------
// copy_screen_32bit_to_16bit
//
// x0: uae_u8   *dst - Format (bits): rrrr rggg gggb bbbb
// x1: uae_u8   *src - Format (bytes) in memory rgba
// x2: int      bytes
//
// void copy_screen_32bit_to_16bit(uae_u8 *dst, uae_u8 *src, int bytes);
//
//----------------------------------------------------------------
copy_screen_32bit_to_16bit:
  ldr       x3, =Masks_rgb
  ld2r      {v0.4S-v1.4S}, [x3]
copy_screen_32bit_to_16bit_loop:
  ld1       {v3.4S}, [x1], #16
  rev64     v3.16B, v3.16B
  ushr      v4.4S, v3.4S, #16
  ushr      v5.4S, v3.4S, #13
  ushr      v3.4S, v3.4S, #11
  bit       v3.16B, v4.16B, v0.16B
  bit       v3.16B, v5.16B, v1.16B
  xtn       v3.4H, v3.4S
  rev32     v3.4H, v3.4H
  subs      w2, w2, #16
  st1       {v3.4H}, [x0], #8
  bne       copy_screen_32bit_to_16bit_loop
  ret


//----------------------------------------------------------------
// ARM_doline_n1
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void ARM_doline_n1(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
ARM_doline_n1:
  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x3, x3, x2           // real_bplpt[0]

  ldr       x10, =Lookup_doline_n1
 
ARM_doline_n1_loop:
  ldr       w2, [x3], #4

  ubfx      x5, x2, #28, #4
  ldr       w6, [x10, x5, lsl #2]
  
  ubfx      x5, x2, #24, #4
  ldr       w7, [x10, x5, lsl #2]

  ubfx      x5, x2, #20, #4
  ldr       w8, [x10, x5, lsl #2]

  ubfx      x5, x2, #16, #4
  ldr       w9, [x10, x5, lsl #2]
  stp       w6, w7, [x0], #8     
  stp       w8, w9, [x0], #8     
  
  ubfx      x5, x2, #12, #4
  ldr       w6, [x10, x5, lsl #2]

  ubfx      x5, x2, #8, #4
  ldr       w7, [x10, x5, lsl #2]

  ubfx      x5, x2, #4, #4
  ldr       w8, [x10, x5, lsl #2]

  ubfx      x5, x2, #0, #4
  ldr       w9, [x10, x5, lsl #2]
  stp       w6, w7, [x0], #8     
  stp       w8, w9, [x0], #8     

  subs      x1, x1, #1
  bgt       ARM_doline_n1_loop
  
  ret


.align 8

//----------------------------------------------------------------
// NEON_doline_n2
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void NEON_doline_n2(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
NEON_doline_n2:

  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x2, x3, x2           // real_bplpt[0]
  add       x3, x2, #200
  
// Load masks to registers
  movi      v16.8b, #0x55
  movi      v17.16b, #0x03
  movi      v18.8b, #1
  
NEON_doline_n2_loop:
  ld1       {v0.8b}, [x2], #8
  ld1       {v1.8b}, [x3], #8
  
  ushr      v2.8b, v0.8b, #1      // tmpb = b >> shift
  ushl      v3.8b, v1.8b, v18.8b  // tmpa = a << shift
  bit       v1.8b, v2.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v3.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  zip2      v4.8b, v1.8b, v0.8b
  zip1      v5.8b, v1.8b, v0.8b

  zip1      v5.2d, v4.2d, v5.2d
  
  ushr      v0.16b, v5.16b, #6
  ushr      v1.16b, v5.16b, #4
  ushr      v2.16b, v5.16b, #2

  and       v1.16b, v1.16b, v17.16b
  and       v2.16b, v2.16b, v17.16b
  and       v5.16b, v5.16b, v17.16b

  zip2      v21.8h, v0.8h, v1.8h
  zip1      v23.8h, v0.8h, v1.8h

  zip2      v20.8h, v2.8h, v5.8h
  zip1      v22.8h, v2.8h, v5.8h

  zip2      v1.4s, v21.4s, v20.4s
  zip1      v3.4s, v21.4s, v20.4s

  uzp2      v0.2d, v1.2d, v1.2d
  uzp2      v2.2d, v3.2d, v3.2d

  st4       { v0.d, v1.d, v2.d, v3.d }[0], [x0], #32
  
  cmp       x1, #1    // Exit from here if odd number of words
  beq       NEON_doline_n2_exit
   
  subs      x1, x1, #2    // We handle 2 words (64 bit) per loop: wordcount -= 2

  zip2      v5.4s, v23.4s, v22.4s
  zip1      v7.4s, v23.4s, v22.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32

  bgt       NEON_doline_n2_loop
  
NEON_doline_n2_exit:
  ret


.align 8

//----------------------------------------------------------------
// NEON_doline_n3
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void NEON_doline_n3(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
NEON_doline_n3:
  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x2, x3, x2           // real_bplpt[0]
  add       x3, x2, #200
  add       x4, x3, #200

  // Load masks to registers
  movi      v16.8b, #0x55
  movi      v17.8b, #0x33
  movi      v18.16b, #0x0f

NEON_doline_n3_loop:
  movi      v19.8b, #1

  ld1       {v0.8b}, [x2], #8
  ld1       {v1.8b}, [x3], #8
  ld1       {v2.8b}, [x4], #8
  movi      v3.8b, #0

  ushr      v4.8b, v0.8b, #1      // tmpb = b >> shift
  ushl      v5.8b, v1.8b, v19.8b  // tmpa = a << shift
  bit       v1.8b, v4.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v5.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v4.8b, v2.8b, #1      // tmpb = b >> shift
  bif       v2.8b, v3.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false
  bit       v3.8b, v4.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 

  add       v19.8b, v19.8b, v19.8b  // now each element contains 2
      
  ushr      v4.8b, v0.8b, #2      // tmpb = b >> shift
  ushl      v5.8b, v2.8b, v19.8b  // tmpa = a << shift
  bit       v2.8b, v4.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v5.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v4.8b, v1.8b, #2      // tmpb = b >> shift
  ushl      v5.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v4.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v1.8b, v5.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  zip2      v4.8b, v3.8b, v2.8b
  zip1      v5.8b, v3.8b, v2.8b
  zip2      v6.8b, v1.8b, v0.8b
  zip1      v7.8b, v1.8b, v0.8b

  zip1      v20.8h, v4.8h, v6.8h
  zip1      v21.8h, v5.8h, v7.8h

  ushr      v0.16b, v20.16b, #4
  ushr      v1.16b, v21.16b, #4

  and       v20.16b, v20.16b, v18.16b
  and       v21.16b, v21.16b, v18.16b

  zip2      v5.4s, v1.4s, v21.4s
  zip1      v7.4s, v1.4s, v21.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32
  
  cmp       x1, #1    // Exit from here if odd number of words
  beq       NEON_doline_n3_exit
   
  subs      x1, x1, #2    // We handle 2 words (64 bit) per loop: wordcount -= 2

  zip2      v5.4s, v0.4s, v20.4s
  zip1      v7.4s, v0.4s, v20.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32

  bgt       NEON_doline_n3_loop

NEON_doline_n3_exit:
  ret


.align 8

//----------------------------------------------------------------
// NEON_doline_n4
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void NEON_doline_n4(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
NEON_doline_n4:
  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x2, x3, x2           // real_bplpt[0]
  add       x3, x2, #200
  add       x4, x3, #200
  add       x5, x4, #200

  // Load masks to registers
  movi      v16.8b, #0x55
  movi      v17.8b, #0x33
  movi      v18.16b, #0x0f

NEON_doline_n4_loop:
  movi      v19.8b, #1

  ld1       {v0.8b}, [x2], #8
  ld1       {v1.8b}, [x3], #8
  ld1       {v2.8b}, [x4], #8
  ld1       {v3.8b}, [x5], #8

  ushr      v4.8b, v0.8b, #1      // tmpb = b >> shift
  ushl      v5.8b, v1.8b, v19.8b  // tmpa = a << shift
  bit       v1.8b, v4.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v5.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v4.8b, v2.8b, #1      // tmpb = b >> shift
  ushl      v5.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v4.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v2.8b, v5.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  add       v19.8b, v19.8b, v19.8b  // now each element contains 2
      
  ushr      v4.8b, v0.8b, #2      // tmpb = b >> shift
  ushl      v5.8b, v2.8b, v19.8b  // tmpa = a << shift
  bit       v2.8b, v4.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v5.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v4.8b, v1.8b, #2      // tmpb = b >> shift
  ushl      v5.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v4.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v1.8b, v5.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  zip2      v4.8b, v3.8b, v2.8b
  zip1      v5.8b, v3.8b, v2.8b
  zip2      v6.8b, v1.8b, v0.8b
  zip1      v7.8b, v1.8b, v0.8b

  zip1      v20.8h, v4.8h, v6.8h
  zip1      v21.8h, v5.8h, v7.8h

  ushr      v0.16b, v20.16b, #4
  ushr      v1.16b, v21.16b, #4

  and       v20.16b, v20.16b, v18.16b
  and       v21.16b, v21.16b, v18.16b

  zip2      v5.4s, v1.4s, v21.4s
  zip1      v7.4s, v1.4s, v21.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32
  
  cmp       x1, #1    // Exit from here if odd number of words
  beq       NEON_doline_n4_exit
   
  subs      x1, x1, #2    // We handle 2 words (64 bit) per loop: wordcount -= 2

  zip2      v5.4s, v0.4s, v20.4s
  zip1      v7.4s, v0.4s, v20.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32

  bgt       NEON_doline_n4_loop

NEON_doline_n4_exit:
  ret


.align 8

//----------------------------------------------------------------
// NEON_doline_n5
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void NEON_doline_n5(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
NEON_doline_n5:
  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x2, x3, x2           // real_bplpt[0]
  add       x3, x2, #200
  add       x4, x3, #200
  add       x5, x4, #200
  add       x6, x5, #200

  // Load masks to registers
  movi      v16.8b, #0x55
  movi      v17.8b, #0x33
  movi      v18.16b, #0x0f

NEON_doline_n5_loop:
  movi      v19.8b, #1

  ld1       {v0.8b}, [x2], #8
  ld1       {v1.8b}, [x3], #8
  ld1       {v2.8b}, [x4], #8
  ld1       {v3.8b}, [x5], #8
  ld1       {v4.8b}, [x6], #8
  movi      v5.8b, #0
  movi      v6.8b, #0
  movi      v7.8b, #0

  ushr      v20.8b, v0.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v1.8b, v19.8b  // tmpa = a << shift
  bit       v1.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v2.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v2.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #1      // tmpb = b >> shift
  bif       v4.8b, v5.8b, v16.8b   // b = b and bit set from tmpa if mask (0x55) is false
  bit       v5.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 

  add       v19.8b, v19.8b, v19.8b  // now each element contains 2

  ushr      v20.8b, v0.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v2.8b, v19.8b  // tmpa = a << shift
  bit       v2.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v1.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v1.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #2      // tmpb = b >> shift
  bif       v4.8b, v6.8b, v17.8b   // b = b and bit set from tmpa if mask (0x55) is false
  bit       v6.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 

  ushr      v20.8b, v5.8b, #2      // tmpb = b >> shift
  bif       v5.8b, v7.8b, v17.8b   // b = b and bit set from tmpa if mask (0x55) is false
  bit       v7.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 

  add       v19.8b, v19.8b, v19.8b  // now each element contains 4

  ushr      v20.8b, v3.8b, #4
  ushl      v21.8b, v7.8b, v19.8b
  bit       v7.8b, v20.8b, v18.8b
  bif       v3.8b, v21.8b, v18.8b

  ushr      v20.8b, v2.8b, #4
  ushl      v21.8b, v6.8b, v19.8b
  bit       v6.8b, v20.8b, v18.8b
  bif       v2.8b, v21.8b, v18.8b

  ushr      v20.8b, v1.8b, #4
  ushl      v21.8b, v5.8b, v19.8b
  bit       v5.8b, v20.8b, v18.8b
  bif       v1.8b, v21.8b, v18.8b

  ushr      v20.8b, v0.8b, #4
  ushl      v21.8b, v4.8b, v19.8b
  bit       v4.8b, v20.8b, v18.8b
  bif       v0.8b, v21.8b, v18.8b

  zip1      v20.16b, v7.16b, v6.16b
  zip1      v21.16b, v5.16b, v4.16b
  zip1      v22.16b, v3.16b, v2.16b
  zip1      v23.16b, v1.16b, v0.16b

  zip2      v0.8h, v20.8h, v21.8h
  zip1      v1.8h, v20.8h, v21.8h
  zip2      v2.8h, v22.8h, v23.8h
  zip1      v3.8h, v22.8h, v23.8h

  zip2      v5.4s, v1.4s, v3.4s
  zip1      v7.4s, v1.4s, v3.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32
  
  cmp       x1, #1    // Exit from here if odd number of words
  beq       NEON_doline_n5_exit
   
  subs      x1, x1, #2    // We handle 2 words (64 bit) per loop: wordcount -= 2

  zip2      v5.4s, v0.4s, v2.4s
  zip1      v7.4s, v0.4s, v2.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32

  bgt       NEON_doline_n5_loop

NEON_doline_n5_exit:
  ret


.align 8

//----------------------------------------------------------------
// NEON_doline_n6
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void NEON_doline_n6(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
NEON_doline_n6:
  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x2, x3, x2           // real_bplpt[0]
  add       x3, x2, #200
  add       x4, x3, #200
  add       x5, x4, #200
  add       x6, x5, #200
  add       x7, x6, #200

  // Load masks to registers
  movi      v16.8b, #0x55
  movi      v17.8b, #0x33
  movi      v18.16b, #0x0f

NEON_doline_n6_loop:
  movi      v19.8b, #1

  ld1       {v0.8b}, [x2], #8
  ld1       {v1.8b}, [x3], #8
  ld1       {v2.8b}, [x4], #8
  ld1       {v3.8b}, [x5], #8
  ld1       {v4.8b}, [x6], #8
  ld1       {v5.8b}, [x7], #8
  movi      v6.8b, #0
  movi      v7.8b, #0

  ushr      v20.8b, v0.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v1.8b, v19.8b  // tmpa = a << shift
  bit       v1.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v2.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v2.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v5.8b, v19.8b  // tmpa = a << shift
  bit       v5.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v4.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  add       v19.8b, v19.8b, v19.8b  // now each element contains 2

  ushr      v20.8b, v0.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v2.8b, v19.8b  // tmpa = a << shift
  bit       v2.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v1.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v1.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #2      // tmpb = b >> shift
  bif       v4.8b, v6.8b, v17.8b   // b = b and bit set from tmpa if mask (0x55) is false
  bit       v6.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 

  ushr      v20.8b, v5.8b, #2      // tmpb = b >> shift
  bif       v5.8b, v7.8b, v17.8b   // b = b and bit set from tmpa if mask (0x55) is false
  bit       v7.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 

  add       v19.8b, v19.8b, v19.8b  // now each element contains 4

  ushr      v20.8b, v3.8b, #4
  ushl      v21.8b, v7.8b, v19.8b
  bit       v7.8b, v20.8b, v18.8b
  bif       v3.8b, v21.8b, v18.8b

  ushr      v20.8b, v2.8b, #4
  ushl      v21.8b, v6.8b, v19.8b
  bit       v6.8b, v20.8b, v18.8b
  bif       v2.8b, v21.8b, v18.8b

  ushr      v20.8b, v1.8b, #4
  ushl      v21.8b, v5.8b, v19.8b
  bit       v5.8b, v20.8b, v18.8b
  bif       v1.8b, v21.8b, v18.8b

  ushr      v20.8b, v0.8b, #4
  ushl      v21.8b, v4.8b, v19.8b
  bit       v4.8b, v20.8b, v18.8b
  bif       v0.8b, v21.8b, v18.8b

  zip1      v20.16b, v7.16b, v6.16b
  zip1      v21.16b, v5.16b, v4.16b
  zip1      v22.16b, v3.16b, v2.16b
  zip1      v23.16b, v1.16b, v0.16b

  zip2      v0.8h, v20.8h, v21.8h
  zip1      v1.8h, v20.8h, v21.8h
  zip2      v2.8h, v22.8h, v23.8h
  zip1      v3.8h, v22.8h, v23.8h

  zip2      v5.4s, v1.4s, v3.4s
  zip1      v7.4s, v1.4s, v3.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32
  
  cmp       x1, #1    // Exit from here if odd number of words
  beq       NEON_doline_n6_exit
   
  subs      x1, x1, #2    // We handle 2 words (64 bit) per loop: wordcount -= 2

  zip2      v5.4s, v0.4s, v2.4s
  zip1      v7.4s, v0.4s, v2.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32

  bgt       NEON_doline_n6_loop

NEON_doline_n6_exit:
  ret


.align 8

//----------------------------------------------------------------
// NEON_doline_n7
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void NEON_doline_n7(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
NEON_doline_n7:
  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x2, x3, x2           // real_bplpt[0]
  add       x3, x2, #200
  add       x4, x3, #200
  add       x5, x4, #200
  add       x6, x5, #200
  add       x7, x6, #200
  add       x8, x7, #200

  // Load masks to registers
  movi      v16.8b, #0x55
  movi      v17.8b, #0x33
  movi      v18.16b, #0x0f

NEON_doline_n7_loop:
  movi      v19.8b, #1

  ld1       {v0.8b}, [x2], #8
  ld1       {v1.8b}, [x3], #8
  ld1       {v2.8b}, [x4], #8
  ld1       {v3.8b}, [x5], #8
  ld1       {v4.8b}, [x6], #8
  ld1       {v5.8b}, [x7], #8
  ld1       {v6.8b}, [x8], #8
  movi      v7.8b, #0

  ushr      v20.8b, v0.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v1.8b, v19.8b  // tmpa = a << shift
  bit       v1.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v2.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v2.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v5.8b, v19.8b  // tmpa = a << shift
  bit       v5.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v4.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v6.8b, #1      // tmpb = b >> shift
  bif       v6.8b, v7.8b, v16.8b   // b = b anh bit set from tmpa if mask (0x55) is false
  bit       v7.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 

  add       v19.8b, v19.8b, v19.8b  // now each element contains 2

  ushr      v20.8b, v0.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v2.8b, v19.8b  // tmpa = a << shift
  bit       v2.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v1.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v1.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v6.8b, v19.8b  // tmpa = a << shift
  bit       v6.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v4.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v5.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v7.8b, v19.8b  // tmpa = a << shift
  bit       v7.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v5.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  add       v19.8b, v19.8b, v19.8b  // now each element contains 4

  ushr      v20.8b, v3.8b, #4
  ushl      v21.8b, v7.8b, v19.8b
  bit       v7.8b, v20.8b, v18.8b
  bif       v3.8b, v21.8b, v18.8b

  ushr      v20.8b, v2.8b, #4
  ushl      v21.8b, v6.8b, v19.8b
  bit       v6.8b, v20.8b, v18.8b
  bif       v2.8b, v21.8b, v18.8b

  ushr      v20.8b, v1.8b, #4
  ushl      v21.8b, v5.8b, v19.8b
  bit       v5.8b, v20.8b, v18.8b
  bif       v1.8b, v21.8b, v18.8b

  ushr      v20.8b, v0.8b, #4
  ushl      v21.8b, v4.8b, v19.8b
  bit       v4.8b, v20.8b, v18.8b
  bif       v0.8b, v21.8b, v18.8b

  zip1      v20.16b, v7.16b, v6.16b
  zip1      v21.16b, v5.16b, v4.16b
  zip1      v22.16b, v3.16b, v2.16b
  zip1      v23.16b, v1.16b, v0.16b

  zip2      v0.8h, v20.8h, v21.8h
  zip1      v1.8h, v20.8h, v21.8h
  zip2      v2.8h, v22.8h, v23.8h
  zip1      v3.8h, v22.8h, v23.8h

  zip2      v5.4s, v1.4s, v3.4s
  zip1      v7.4s, v1.4s, v3.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32
  
  cmp       x1, #1    // Exit from here if odd number of words
  beq       NEON_doline_n7_exit
   
  subs      x1, x1, #2    // We handle 2 words (64 bit) per loop: wordcount -= 2

  zip2      v5.4s, v0.4s, v2.4s
  zip1      v7.4s, v0.4s, v2.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32

  bgt       NEON_doline_n7_loop

NEON_doline_n7_exit:
  ret


.align 8

//----------------------------------------------------------------
// NEON_doline_n8
//
// x0: uae_u32   *pixels
// x1: int       wordcount
// x2: int       lineno 
//
// void NEON_doline_n8(uae_u32 *pixels, int wordcount, int lineno);
//
//----------------------------------------------------------------
NEON_doline_n8:
  mov       x3, #1600
  mul       x2, x2, x3
  ldr       x3, =line_data
  add       x2, x3, x2           // real_bplpt[0]
  add       x3, x2, #200
  add       x4, x3, #200
  add       x5, x4, #200
  add       x6, x5, #200
  add       x7, x6, #200
  add       x8, x7, #200
  add       x9, x8, #200

  // Load masks to registers
  movi      v16.8b, #0x55
  movi      v17.8b, #0x33
  movi      v18.16b, #0x0f

NEON_doline_n8_loop:
  movi      v19.8b, #1

  ld1       {v0.8b}, [x2], #8
  ld1       {v1.8b}, [x3], #8
  ld1       {v2.8b}, [x4], #8
  ld1       {v3.8b}, [x5], #8
  ld1       {v4.8b}, [x6], #8
  ld1       {v5.8b}, [x7], #8
  ld1       {v6.8b}, [x8], #8
  ld1       {v7.8b}, [x9], #8

  ushr      v20.8b, v0.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v1.8b, v19.8b  // tmpa = a << shift
  bit       v1.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v2.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v2.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v5.8b, v19.8b  // tmpa = a << shift
  bit       v5.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v4.8b, v21.8b, v16.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v6.8b, #1      // tmpb = b >> shift
  ushl      v21.8b, v7.8b, v19.8b  // tmpa = a << shift
  bit       v7.8b, v20.8b, v16.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v6.8b, v21.8b, v16.8b  // b = b anh bit set from tmpa if mask (0x55) is false

  add       v19.8b, v19.8b, v19.8b  // now each element contains 2

  ushr      v20.8b, v0.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v2.8b, v19.8b  // tmpa = a << shift
  bit       v2.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v0.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v1.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v3.8b, v19.8b  // tmpa = a << shift
  bit       v3.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v1.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v4.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v6.8b, v19.8b  // tmpa = a << shift
  bit       v6.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v4.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  ushr      v20.8b, v5.8b, #2      // tmpb = b >> shift
  ushl      v21.8b, v7.8b, v19.8b  // tmpa = a << shift
  bit       v7.8b, v20.8b, v17.8b  // a = a and bit set from tmpb if mask (0x55) is true 
  bif       v5.8b, v21.8b, v17.8b  // b = b and bit set from tmpa if mask (0x55) is false

  add       v19.8b, v19.8b, v19.8b  // now each element contains 4

  ushr      v20.8b, v3.8b, #4
  ushl      v21.8b, v7.8b, v19.8b
  bit       v7.8b, v20.8b, v18.8b
  bif       v3.8b, v21.8b, v18.8b

  ushr      v20.8b, v2.8b, #4
  ushl      v21.8b, v6.8b, v19.8b
  bit       v6.8b, v20.8b, v18.8b
  bif       v2.8b, v21.8b, v18.8b

  ushr      v20.8b, v1.8b, #4
  ushl      v21.8b, v5.8b, v19.8b
  bit       v5.8b, v20.8b, v18.8b
  bif       v1.8b, v21.8b, v18.8b

  ushr      v20.8b, v0.8b, #4
  ushl      v21.8b, v4.8b, v19.8b
  bit       v4.8b, v20.8b, v18.8b
  bif       v0.8b, v21.8b, v18.8b

  zip1      v20.16b, v7.16b, v6.16b
  zip1      v21.16b, v5.16b, v4.16b
  zip1      v22.16b, v3.16b, v2.16b
  zip1      v23.16b, v1.16b, v0.16b

  zip2      v0.8h, v20.8h, v21.8h
  zip1      v1.8h, v20.8h, v21.8h
  zip2      v2.8h, v22.8h, v23.8h
  zip1      v3.8h, v22.8h, v23.8h

  zip2      v5.4s, v1.4s, v3.4s
  zip1      v7.4s, v1.4s, v3.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32
  
  cmp       x1, #1    // Exit from here if odd number of words
  beq       NEON_doline_n8_exit
   
  subs      x1, x1, #2    // We handle 2 words (64 bit) per loop: wordcount -= 2

  zip2      v5.4s, v0.4s, v2.4s
  zip1      v7.4s, v0.4s, v2.4s

  uzp2      v4.2d, v5.2d, v5.2d
  uzp2      v6.2d, v7.2d, v7.2d

  st4       { v4.d, v5.d, v6.d, v7.d }[0], [x0], #32

  bgt       NEON_doline_n8_loop

NEON_doline_n8_exit:
  ret



.align 8

Masks_rgb:
  .long 0x0000f800, 0x000007e0

Lookup_doline_n1:
  .long 0x00000000, 0x01000000, 0x00010000, 0x01010000
  .long 0x00000100, 0x01000100, 0x00010100, 0x01010100
  .long 0x00000001, 0x01000001, 0x00010001, 0x01010001
  .long 0x00000101, 0x01000101, 0x00010101, 0x01010101
