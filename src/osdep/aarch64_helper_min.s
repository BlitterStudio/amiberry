// Some functions and tests to increase performance in drawing.cpp and custom.cpp

//.arm

.global save_host_fp_regs
.global restore_host_fp_regs

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
