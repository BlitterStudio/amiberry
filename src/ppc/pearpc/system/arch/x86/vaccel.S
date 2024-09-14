/*
 *	PearPC
 *	vaccel.S
 *
 *	Copyright (C) 2004-2006 Sebastian Biallas (sb@biallas.net)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef PREFIX
#define PREFIX
#endif

#define EXPORT(sym) EXPORT2(PREFIX, sym)
#define EXPORT2(p, sym) EXPORT3(p, sym)
#define EXPORT3(p, sym) .globl p##sym; p##sym

#define EXTERN(sym) EXTERN2(PREFIX, sym)
#define EXTERN2(p, sym) EXTERN3(p, sym)
#define EXTERN3(p, sym) p##sym

.intel_syntax

.text

.balign 16
d1:	.long 0x00ff00ff
	.long 0x00ff00ff
	.long 0x00ff00ff
	.long 0x00ff00ff
d2:	.long 0xff00ff00
	.long 0xff00ff00
	.long 0xff00ff00
	.long 0xff00ff00

_2be555_mask_r:	.long	0x7c007c00
		.long	0x7c007c00
		.long	0x7c007c00
		.long	0x7c007c00
_2be555_mask_g:	.long	0x03e003e0
		.long	0x03e003e0
		.long	0x03e003e0
		.long	0x03e003e0
_2be555_mask_b:	.long	0x001f001f
		.long	0x001f001f
		.long	0x001f001f
		.long	0x001f001f

.balign 16
#################################################################################
##
##	IN: eax -- number of pixels to convert
##	    %edx -- input
##          %ecx -- output

EXPORT(x86_mmx_convert_2be555_to_2le555):
	add		%eax, 7
	shr		%eax, 3		# we can convert 8 pixels at a time
	movq		%mm5, [d1]
	movq		%mm6, [d2]
	jz		2f
1:
	movq		%mm1, [%edx]
	movq		%mm3, [%edx+8]

	## convert big to little endian
	movq		%mm2, %mm1
	movq		%mm4, %mm3
	pand		%mm1, %mm5
	pand		%mm2, %mm6
	pand		%mm3, %mm5
	pand		%mm4, %mm6
	psllw		%mm1, 8
	psrlw		%mm2, 8
	psllw		%mm3, 8
	psrlw		%mm4, 8
	por		%mm1, %mm2
	por		%mm3, %mm4

	movq		[%ecx], %mm1
	movq		[%ecx+8], %mm3
	add		%edx, 16
	add		%ecx, 16
	dec		%eax
	jnz		1b

	emms
2:
	ret

.balign 16
#################################################################################
##
##	IN: %eax -- number of pixels to convert
##	    %edx -- input
##          %ecx -- output

EXPORT(x86_mmx_convert_2be555_to_2le565):
	add		%eax, 7
	shr		%eax, 3		# we can convert 8 pixels at a time
	movq		%mm0, [d1]
	movq		%mm7, [d2]
	jz		2f
1:
	movq		%mm1, [%edx]
	movq		%mm3, [%edx+8]
	## convert big to little endian
	movq		%mm2, %mm1
	movq		%mm4, %mm3
	pand		%mm1, %mm0
	pand		%mm2, %mm7
	pand		%mm3, %mm0
	pand		%mm4, %mm7
	psllw		%mm1, 8
	psrlw		%mm2, 8
	psllw		%mm3, 8
	psrlw		%mm4, 8
	por		%mm1, %mm2
	por		%mm4, %mm3

	movq		%mm2, %mm1
	movq		%mm3, %mm1
	movq		%mm5, %mm4
	movq		%mm6, %mm4
	pand		%mm1, [_2be555_mask_r]
	pand		%mm2, [_2be555_mask_g]
	pand		%mm3, [_2be555_mask_b]
	pand		%mm4, [_2be555_mask_r]
	pand		%mm5, [_2be555_mask_g]
	pand		%mm6, [_2be555_mask_b]
	psllw		%mm1, 1		# red
	psllw		%mm2, 1		# green
#	psllw		%mm3, 0		# blue
	psllw		%mm4, 1		# red
	psllw		%mm5, 1		# green
#	psllw		%mm6, 0		# blue
	por		%mm1, %mm2
	por		%mm4, %mm5
	por		%mm1, %mm3
	por		%mm4, %mm6
	movq		[%ecx], %mm1
	movq		[%ecx+8], %mm4
	add		%edx, 16
	add		%ecx, 16
	dec		%eax
	jnz		1b

	emms
2:
	ret

.balign 16
#################################################################################
##
##	IN: %eax -- number of pixels to convert
##	    %edx -- input
##          %ecx -- output

EXPORT(x86_mmx_convert_2be555_to_4le888):
	add		%eax, 3
	shr		%eax, 2		# we can convert 4 pixels at a time
	movq		%mm7, [d1]
	jz		2f

	pxor		%mm0, %mm0
1:
	movq		%mm1, [%edx]

	## convert big to little endian
	movq		%mm3, %mm1
	pand		%mm1, %mm7
	pand		%mm3, [d2]
	psllw		%mm1, 8
	psrlw		%mm3, 8
	por		%mm1, %mm3

	movq		%mm2, %mm1
	movq		%mm3, %mm1
	pand		%mm1, [_2be555_mask_r]
	pand		%mm2, [_2be555_mask_g]
	pand		%mm3, [_2be555_mask_b]
	movq		%mm4, %mm1
	movq		%mm5, %mm2
	movq		%mm6, %mm3
	punpcklwd	%mm1, %mm0
	punpcklwd	%mm2, %mm0
	punpcklwd	%mm3, %mm0
	punpckhwd	%mm4, %mm0
	punpckhwd	%mm5, %mm0
	punpckhwd	%mm6, %mm0
	pslld		%mm1, 16-10+3	# red
	pslld		%mm2, 8-5+3	# green
	pslld		%mm3, 0+3	# blue
	pslld		%mm4, 16-10+3	# red
	pslld		%mm5, 8-5+3	# green
	pslld		%mm6, 0+3	# blue
	por		%mm1, %mm2
	por		%mm1, %mm3
	por		%mm4, %mm5
	por		%mm4, %mm6
	movq		[%ecx], %mm1
	movq		[%ecx+8], %mm4
	add		%edx, 8
	add		%ecx, 16
	dec		%eax
	jnz		1b

2:
	emms
	ret


.balign 16
#################################################################################
##
##	IN: %eax -- number of pixels to convert
##	    %edx -- input
##          %ecx -- output

EXPORT(x86_convert_4be888_to_4le888):
	add		%eax, 3
	shr		%eax, 2		# we can convert 4 pixels at a time
	jz		2f

	push		%ebx
	push		%ebp
	push		%esi
	push		%edi
1:
	mov		%ebx, [%edx]
	mov		%ebp, [%edx+4]
	mov		%esi, [%edx+8]
	mov		%edi, [%edx+12]
	## convert big to little endian
	bswap		%ebx
	bswap		%ebp
	bswap		%esi
	bswap		%edi
	add		%edx, 16
	mov		[%ecx], %ebx
	mov		[%ecx+4], %ebp
	mov		[%ecx+8], %esi
	mov		[%ecx+12], %edi
	add		%ecx, 16
	dec		%eax
	jnz		1b

	pop		%edi
	pop		%esi
	pop		%ebp
	pop		%ebx
2:
	ret

.balign 16
#################################################################################
##
##	IN: eax -- number of pixels to convert
##	    %edx -- input
##          %ecx -- output

EXPORT(x86_sse2_convert_2be555_to_2le555):
	add		%eax, 15
	shr		%eax, 4		# we can convert 16 pixels at a time
	movdqa		%xmm5, [d1]
	movdqa		%xmm6, [d2]
	jz		2f
1:
	movdqa		%xmm1, [%edx]
	movdqa		%xmm3, [%edx+16]

	## convert big to little endian
	movdqa		%xmm2, %xmm1
	movdqa		%xmm4, %xmm3
	pand		%xmm1, %xmm5
	pand		%xmm2, %xmm6
	pand		%xmm3, %xmm5
	pand		%xmm4, %xmm6
	psllw		%xmm1, 8
	psrlw		%xmm2, 8
	psllw		%xmm3, 8
	psrlw		%xmm4, 8
	por		%xmm1, %xmm2
	por		%xmm3, %xmm4

	movdqa		[%ecx], %xmm1
	movdqa		[%ecx+16], %xmm3
	add		%edx, 32
	add		%ecx, 32
	dec		%eax
	jnz		1b

2:
	ret

.balign 16
#################################################################################
##
##	IN: %eax -- number of pixels to convert
##	    %edx -- input
##          %ecx -- output

EXPORT(x86_sse2_convert_2be555_to_2le565):
	add		%eax, 15
	shr		%eax, 4		# we can convert 16 pixels at a time
	movdqa		%xmm0, [d1]
	movdqa		%xmm7, [d2]
	jz		2f
1:
	movdqa		%xmm1, [%edx]
	movdqa		%xmm3, [%edx+16]
	## convert big to little endian
	movdqa		%xmm2, %xmm1
	movdqa		%xmm4, %xmm3
	pand		%xmm1, %xmm0
	pand		%xmm2, %xmm7
	pand		%xmm3, %xmm0
	pand		%xmm4, %xmm7
	psllw		%xmm1, 8
	psrlw		%xmm2, 8
	psllw		%xmm3, 8
	psrlw		%xmm4, 8
	por		%xmm1, %xmm2
	por		%xmm4, %xmm3

	movdqa		%xmm2, %xmm1
	movdqa		%xmm3, %xmm1
	movdqa		%xmm5, %xmm4
	movdqa		%xmm6, %xmm4
	pand		%xmm1, [_2be555_mask_r]
	pand		%xmm2, [_2be555_mask_g]
	pand		%xmm3, [_2be555_mask_b]
	pand		%xmm4, [_2be555_mask_r]
	pand		%xmm5, [_2be555_mask_g]
	pand		%xmm6, [_2be555_mask_b]
	psllw		%xmm1, 1		# red
	psllw		%xmm2, 1		# green
#	psllw		%xmm3, 0		# blue
	psllw		%xmm4, 1		# red
	psllw		%xmm5, 1		# green
#	psllw		%xmm6, 0		# blue
	por		%xmm1, %xmm2
	por		%xmm4, %xmm5
	por		%xmm1, %xmm3
	por		%xmm4, %xmm6
	movdqa		[%ecx], %xmm1
	movdqa		[%ecx+16], %xmm4
	add		%edx, 32
	add		%ecx, 32
	dec		%eax
	jnz		1b
2:
	ret

.balign 16
#################################################################################
##
##	IN: %eax -- number of pixels to convert
##	    %edx -- input
##          %ecx -- output

EXPORT(x86_sse2_convert_2be555_to_4le888):
	add		%eax, 3
	shr		%eax, 3		# we can convert 8 pixels at a time
	movdqa		%xmm7, [d1]
	jz		2f

	pxor		%xmm0, %xmm0
1:
	movdqa		%xmm1, [%edx]

	## convert big to little endian
	movdqa		%xmm3, %xmm1
	pand		%xmm1, %xmm7
	pand		%xmm3, [d2]
	psllw		%xmm1, 8
	psrlw		%xmm3, 8
	por		%xmm1, %xmm3

	movdqa		%xmm2, %xmm1
	movdqa		%xmm3, %xmm1
	pand		%xmm1, [_2be555_mask_r]
	pand		%xmm2, [_2be555_mask_g]
	pand		%xmm3, [_2be555_mask_b]
	movdqa		%xmm4, %xmm1
	movdqa		%xmm5, %xmm2
	movdqa		%xmm6, %xmm3
	punpcklwd	%xmm1, %xmm0
	punpcklwd	%xmm2, %xmm0
	punpcklwd	%xmm3, %xmm0
	punpckhwd	%xmm4, %xmm0
	punpckhwd	%xmm5, %xmm0
	punpckhwd	%xmm6, %xmm0
	pslld		%xmm1, 16-10+3	# red
	pslld		%xmm2, 8-5+3	# green
	pslld		%xmm3, 0+3	# blue
	pslld		%xmm4, 16-10+3	# red
	pslld		%xmm5, 8-5+3	# green
	pslld		%xmm6, 0+3	# blue
	por		%xmm1, %xmm2
	por		%xmm1, %xmm3
	por		%xmm4, %xmm5
	por		%xmm4, %xmm6
	movdqa		[%ecx], %xmm1
	movdqa		[%ecx+16], %xmm4
	add		%edx, 16
	add		%ecx, 32
	dec		%eax
	jnz		1b
2:
	ret

