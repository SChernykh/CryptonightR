_TEXT_CN_MAINLOOP SEGMENT PAGE READ EXECUTE
PUBLIC CryptonightR_asm

; CryptonightR assembler code
; Doesn't spill registers on stack unlike compiler generated code

ALIGN 64
CryptonightR_asm PROC
	mov	QWORD PTR [rsp+16], rbx
	mov	QWORD PTR [rsp+24], rbp
	mov	QWORD PTR [rsp+32], rsi
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15
	push	rdi
	sub	rsp, 64
	mov	r12, rcx
	mov	r8, QWORD PTR [r12+32]
	mov	rdx, r12
	xor	r8, QWORD PTR [r12]
	mov	r15, QWORD PTR [r12+40]
	mov	r9, r8
	xor	r15, QWORD PTR [r12+8]
	mov	r11, QWORD PTR [r12+224]
	mov	r12, QWORD PTR [r12+56]
	xor	r12, QWORD PTR [rdx+24]
	mov	rax, QWORD PTR [rdx+48]
	xor	rax, QWORD PTR [rdx+16]
	movaps	XMMWORD PTR [rsp+48], xmm6
	movq	xmm0, r12
	movaps	XMMWORD PTR [rsp+32], xmm7
	movaps	XMMWORD PTR [rsp+16], xmm8
	movaps	XMMWORD PTR [rsp+16], xmm9
	mov	r12, QWORD PTR [rdx+88]
	xor	r12, QWORD PTR [rdx+72]
	movq	xmm6, rax
	mov	rax, QWORD PTR [rdx+80]
	xor	rax, QWORD PTR [rdx+64]
	punpcklqdq xmm6, xmm0
	and	r9d, 2097136
	movq	xmm0, r12
	movq	xmm7, rax
	punpcklqdq xmm7, xmm0
	mov r10d, r9d
	movq	xmm9, rsp
	mov rsp, r8
	mov	r8d, 524288

	mov	ebx, [rdx+96]
	mov	esi, [rdx+100]
	mov	edi, [rdx+104]
	mov	ebp, [rdx+108]

	ALIGN 64
main_loop:
	movdqa	xmm5, XMMWORD PTR [r9+r11]
	movq	xmm0, r15
	movq	xmm4, rsp
	punpcklqdq xmm4, xmm0
	lea	rdx, QWORD PTR [r9+r11]

	aesenc	xmm5, xmm4
	movd	r10d, xmm5
	and	r10d, 2097136

	mov	r12d, r9d
	mov	eax, r9d
	xor	r9d, 48
	xor	r12d, 16
	xor	eax, 32
	movdqu	xmm0, XMMWORD PTR [r9+r11]
	movdqu	xmm2, XMMWORD PTR [r12+r11]
	movdqu	xmm1, XMMWORD PTR [rax+r11]
	paddq	xmm0, xmm7
	paddq	xmm2, xmm6
	paddq	xmm1, xmm4
	movdqu	XMMWORD PTR [r12+r11], xmm0
	movq	r12, xmm5
	movdqu	XMMWORD PTR [rax+r11], xmm2
	movdqu	XMMWORD PTR [r9+r11], xmm1

	movdqa	xmm0, xmm5
	pxor	xmm0, xmm6
	movdqu	XMMWORD PTR [rdx], xmm0
	lea	eax, [ebx+esi]
	lea	edx, [edi+ebp]
	shl rdx, 32
	or	rax, rdx

	mov	r13, QWORD PTR [r10+r11]
	mov	r14, QWORD PTR [r10+r11+8]
	xor	r13, rax

	movd eax, xmm6
	movd edx, xmm7

	INCLUDE random_math.inc

	mov	rax, r13
	mul	r12
	movq	xmm0, rax
	movq	xmm3, rdx
	punpcklqdq xmm3, xmm0

	mov	r9d, r10d
	mov	r12d, r10d
	xor	r9d, 16
	xor	r12d, 32
	xor	r10d, 48
	movdqa	xmm1, XMMWORD PTR [r12+r11]
	xor	rdx, QWORD PTR [r12+r11]
	xor	rax, QWORD PTR [r11+r12+8]
	movdqa	xmm2, XMMWORD PTR [r9+r11]
	pxor	xmm3, xmm2
	paddq	xmm7, XMMWORD PTR [r10+r11]
	paddq	xmm1, xmm4
	paddq	xmm3, xmm6
	movdqu	XMMWORD PTR [r9+r11], xmm7
	movdqu	XMMWORD PTR [r12+r11], xmm3
	movdqu	XMMWORD PTR [r10+r11], xmm1

	movdqa	xmm7, xmm6
	add	r15, rax
	add	rsp, rdx
	xor	r10, 48
	mov	QWORD PTR [r10+r11], rsp
	xor	rsp, r13
	mov	r9d, esp
	mov	QWORD PTR [r10+r11+8], r15
	and	r9d, 2097136
	xor	r15, r14
	movdqa	xmm6, xmm5
	dec	r8d
	jnz	main_loop

	movq	rsp, xmm9

	mov	rbx, QWORD PTR [rsp+136]
	mov	rbp, QWORD PTR [rsp+144]
	mov	rsi, QWORD PTR [rsp+152]
	movaps	xmm6, XMMWORD PTR [rsp+48]
	movaps	xmm7, XMMWORD PTR [rsp+32]
	movaps	xmm8, XMMWORD PTR [rsp+16]
	movaps	xmm9, XMMWORD PTR [rsp]
	add	rsp, 64
	pop	rdi
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	ret	0
CryptonightR_asm ENDP

_TEXT_CN_MAINLOOP ENDS
END
