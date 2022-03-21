@ Some functions and tests to increase performance in drawing.cpp and custom.cpp

.arm

.global save_host_fp_regs
.global restore_host_fp_regs

.text

.align 8

@----------------------------------------------------------------
@ save_host_fp_regs
@----------------------------------------------------------------
save_host_fp_regs:
	vstmia    r0!, {d7-d15}
  bx        lr

@----------------------------------------------------------------
@ restore_host_fp_regs
@----------------------------------------------------------------
restore_host_fp_regs:
  vldmia    r0!, {d7-d15}
  bx        lr
