
	.text

	.globl enable060fpu

	.arch 68060

enable060fpu:
	| enable 68060 FPU
	movec pcr,d0
	bclr #1,d0
	movec d0,pcr
	rts
