#include <stdlib.h>
#include <stdint.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include "definitions.h"

#include "CryptonightR_template.h"

// Registers to use in generated x86-64 code
static const char* reg[8] = {
	"ebx", "esi", "edi", "ebp",
	"esp", "r15d", "eax", "edx"
};

void compile_code(const V4_Instruction* code, int code_size, std::vector<uint8_t>& machine_code)
{
	machine_code.clear();
	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part1, (const uint8_t*) CryptonightR_template_part2);

#if DUMP_SOURCE_CODE
#define DUMP(x, ...) x << __VA_ARGS__

	std::ofstream f("random_math.inl");
	std::ofstream f_asm("random_math.inc");
	DUMP(f, "// Auto-generated file, do not edit\n\n";
	DUMP(f_asm, "; Auto-generated file, do not edit\n\n";
	DUMP(f, "FORCEINLINE void random_math(uint32_t& r0, uint32_t& r1, uint32_t& r2, uint32_t& r3, const uint32_t r4, const uint32_t r5, const uint32_t r6, const uint32_t r7)\n";
	DUMP(f, "{\n";
#else
#define DUMP(x, ...)
#endif

	uint32_t prev_rot_src = (uint32_t)(-1);

	for (int i = 0; i < code_size; ++i)
	{
		const V4_Instruction inst = code[i];

		const uint32_t a = inst.dst_index;
		const uint32_t b = inst.src_index;

		switch (inst.opcode)
		{
		case MUL1:
		case MUL2:
		case MUL3:
			DUMP(f, "\tr" << a << " *= " << 'r' << b << ";\t");
			DUMP(f_asm, "\timul\t" << reg[a] << ", " << reg[b]);
			break;

		case ADD:
			{
				int c = reinterpret_cast<const int8_t*>(code)[i + 1];

				DUMP(f, "\tr" << a << " += " << 'r' << b << ((c < 0) ? " - " : " + "));
				DUMP(f_asm, "\tlea\t" << reg[a] << ", [" << reg[a] << "+" << reg[b] << ((c < 0) ? "-" : "+"));

				if (c < 0)
					c = -c;

				DUMP(f, c << ";\t");
				DUMP(f_asm, c << "]");
			}
			break;

		case SUB:
			DUMP(f, "\tr" << a << " -= " << 'r' << b << ";\t");
			DUMP(f_asm, "\tsub\t" << reg[a] << ", " << reg[b]);
			break;

		case ROR:
			DUMP(f, "\tr" << a << " = _rotr(r" << a << ", r" << b << ");");
			if (b != prev_rot_src)
			{
				DUMP(f_asm, "\tmov\tecx, " << reg[b] << "\n");
				prev_rot_src = b;

				const uint8_t* p1 = (const uint8_t*)instructions_mov[((uint8_t*)code)[i]];
				const uint8_t* p2 = (const uint8_t*)instructions_mov[((uint8_t*)code)[i] + 1];
				machine_code.insert(machine_code.end(), p1, p2);
			}
			DUMP(f_asm, "\tror\t" << reg[a] << ", cl");
			break;

		case ROL:
			DUMP(f, "\tr" << a << " = _rotl(r" << a << ", r" << b << ");");
			if (b != prev_rot_src)
			{
				DUMP(f_asm, "\tmov\tecx, " << reg[b] << "\n");
				prev_rot_src = b;

				const uint8_t* p1 = (const uint8_t*)instructions_mov[((uint8_t*)code)[i]];
				const uint8_t* p2 = (const uint8_t*)instructions_mov[((uint8_t*)code)[i] + 1];
				machine_code.insert(machine_code.end(), p1, p2);
			}
			DUMP(f_asm, "\trol\t" << reg[a] << ", cl");
			break;

		case XOR:
			DUMP(f, "\tr" << a << " ^= " << 'r' << b << ";\t");
			DUMP(f_asm, "\txor\t" << reg[a] << ", " << reg[b]);
			break;
		}

		DUMP(f, "\n");
		DUMP(f_asm, "\n");

		if (a == prev_rot_src)
		{
			prev_rot_src = (uint32_t)(-1);
		}

		const uint8_t* p1 = (const uint8_t*)instructions[((uint8_t*)code)[i]];
		const uint8_t* p2 = (const uint8_t*)instructions[((uint8_t*)code)[i] + 1];
		machine_code.insert(machine_code.end(), p1, p2);
		if (inst.opcode == ADD)
		{
			++i;
			machine_code.back() = ((uint8_t*)code)[i];
		}
	}

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part2, (const uint8_t*) CryptonightR_template_part3);

	*(int*)(machine_code.data() + machine_code.size() - 4) = static_cast<int>((((const uint8_t*)CryptonightR_template_mainloop) - ((const uint8_t*)CryptonightR_template_part1)) - machine_code.size());

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part3, (const uint8_t*) CryptonightR_instruction0);

#if DUMP_SOURCE_CODE
	f << "}\n";
	f.close();
	f_asm.close();

	std::ofstream f_bin("random_math.bin", std::ios::out | std::ios::binary);
	f_bin.write((const char*)machine_code.data(), machine_code.size());
	f_bin.close();
#endif
}

static void generate_asm_template()
{
	std::ofstream f("CryptonightR_template.h");
	f << R"===(// Auto-generated file, do not edit

extern "C"
{
	void CryptonightR_template_part1();
	void CryptonightR_template_mainloop();
	void CryptonightR_template_part2();
	void CryptonightR_template_part3();
)===";

	for (int i = 0; i <= 256; ++i)
		f << "\tvoid CryptonightR_instruction" << i << "();\n";

	for (int i = 0; i <= 256; ++i)
		f << "\tvoid CryptonightR_instruction_mov" << i << "();\n";

	f << R"===(}

const void* instructions[257] = {
)===";

	for (int i = 0; i <= 256; ++i)
		f << "\tCryptonightR_instruction" << i << ",\n";

	f << "};\n\n";

	f << "const void* instructions_mov[257] = {\n";

	for (int i = 0; i <= 256; ++i)
		f << "\tCryptonightR_instruction_mov" << i << ",\n";

	f << "};\n";
	f.close();

	std::ofstream f_asm("CryptonightR_template.asm");
	f_asm << R"===(; Auto-generated file, do not edit

_TEXT_CN_TEMPLATE SEGMENT PAGE READ EXECUTE
PUBLIC CryptonightR_template_part1
PUBLIC CryptonightR_template_mainloop
PUBLIC CryptonightR_template_part2
PUBLIC CryptonightR_template_part3
)===";

	for (int i = 0; i <= 256; ++i)
		f_asm << "PUBLIC CryptonightR_instruction" << i << "\n";

	for (int i = 0; i <= 256; ++i)
		f_asm << "PUBLIC CryptonightR_instruction_mov" << i << "\n";

	f_asm << R"===(
CryptonightR_template_part1:
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
CryptonightR_template_mainloop:
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

CryptonightR_template_part2:
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
	jnz	CryptonightR_template_mainloop

CryptonightR_template_part3:
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

)===";

	for (int i = 0; i <= 256; ++i)
	{
		f_asm << "CryptonightR_instruction" << i << ":\n";
		const V4_Instruction inst = reinterpret_cast<V4_Instruction*>(&i)[0];

		const uint32_t a = inst.dst_index;
		const uint32_t b = inst.src_index;

		switch (inst.opcode)
		{
		case MUL1:
		case MUL2:
		case MUL3:
			f_asm << "\timul\t" << reg[a] << ", " << reg[b];
			break;

		case ADD:
			f_asm << "\tlea\t" << reg[a] << ", [" << reg[a] << "+" << reg[b] << "+1]";
			break;

		case SUB:
			f_asm << "\tsub\t" << reg[a] << ", " << reg[b];
			break;

		case ROR:
			f_asm << "\tror\t" << reg[a] << ", cl";
			break;

		case ROL:
			f_asm << "\trol\t" << reg[a] << ", cl";
			break;

		case XOR:
			f_asm << "\txor\t" << reg[a] << ", " << reg[b];
			break;
		}
		f_asm << "\n";
	}

	for (int i = 0; i <= 256; ++i)
	{
		f_asm << "CryptonightR_instruction_mov" << i << ":\n";
		const V4_Instruction inst = reinterpret_cast<V4_Instruction*>(&i)[0];

		const uint32_t b = inst.src_index;

		switch (inst.opcode)
		{
		case ROR:
		case ROL:
			f_asm << "\tmov\tecx, " << reg[b];
			break;
		}
		f_asm << "\n";
	}

	f_asm << R"===(_TEXT_CN_TEMPLATE ENDS
END
)===";

	f_asm.close();
}

extern int CryptonightR_test();

int main()
{
	// Only needs to be done if VM instructions list, encoding or semantics change
	generate_asm_template();

#if DUMP_SOURCE_CODE
	V4_Instruction code[256];
	int code_size = v4_random_math_init(code, RND_SEED);

	std::vector<uint8_t> machine_code;
	compile_code(code, code_size, machine_code);
#endif

	return CryptonightR_test();
}
