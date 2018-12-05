#include <stdlib.h>
#include <stdint.h>
#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "definitions.h"

#include "CryptonightR_template.h"

void generate_code(std::mt19937& rnd, uint8_t* code, int& num_instructions, int& code_size, std::vector<uint8_t>& machine_code)
{
	machine_code.clear();
	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part1, (const uint8_t*) CryptonightR_template_part2);

#if DUMP_CODE
#define DUMP(x, ...) x << __VA_ARGS__

	std::ofstream f("random_math.inl");
	std::ofstream f_asm("random_math.inc");
	f << "// Auto-generated file, do not edit\n\n";
	f_asm << "; Auto-generated file, do not edit\n\n";
	f << "FORCEINLINE void random_math(uint32_t& r0, uint32_t& r1, uint32_t& r2, uint32_t& r3, const uint32_t r4, const uint32_t r5, const uint32_t r6, const uint32_t r7)\n";
	f << "{\n";
	f << "\t// Comments below show latency for each register on an abstract CPU:\n";
	f << "\t// - Superscalar, out-of-order execution, fully pipelined\n";
	f << "\t// - Two integer ALUs\n";
	f << "\t// - ALU0 can do all operations\n";
	f << "\t// - ALU1 can only do 1-cycle operations\n";
	f << "\t// - Latency for MUL is 3 cycles, throughput is 1 MUL/cycle\n";
	f << "\t// - Latency for all other operation is 1 cycle\n\n";
#else
#define DUMP(x, ...)
#endif

	uint32_t latency[8] = {};
	uint32_t min_possible_latency[8] = {};
	num_instructions = 0;
	code_size = 0;

	bool alu_busy[TOTAL_LATENCY][ALU_COUNT] = {};

	auto all_alu_busy = [&alu_busy](int latency)
	{
		for (int i = 0; i < ALU_COUNT; ++i)
		{
			if (!alu_busy[latency][i])
			{
				return false;
			}
		}
		return true;
	};

	auto use_alu = [&alu_busy](int latency)
	{
		for (int i = ALU_COUNT - 1; i >= 0; --i)
		{
			if (!alu_busy[latency][i])
			{
				alu_busy[latency][i] = true;
				return;
			}
		}
	};

	uint32_t prev_rot_src = (uint32_t)(-1);

	int num_retries = 0;

	do
	{
		uint32_t next_latency;

		uint8_t data[4];
		*((uint32_t*)data) = rnd();
		const SInstruction inst = reinterpret_cast<SInstruction*>(data)[0];

		const uint32_t a = inst.dst_index;
		const uint32_t b = inst.src_index;
		const int prev_code_size = code_size;

		switch (inst.opcode)
		{
		case MUL1:
		case MUL2:
		case MUL3:
			next_latency = std::max(latency[a], latency[b]);
			while ((next_latency < TOTAL_LATENCY) && alu_busy[next_latency][0])
			{
				++next_latency;
			}
			next_latency += 3;

			if (next_latency <= TOTAL_LATENCY)
			{
				DUMP(f, "\tr" << a << " *= " << 'r' << b << ";\t");
				DUMP(f_asm, "\timul\t" << reg[a] << ", " << reg[b]);
				min_possible_latency[a] = std::max(min_possible_latency[a], min_possible_latency[b]) + 3;
				alu_busy[next_latency - 3][0] = true;
				latency[a] = next_latency;
				code[code_size++] = data[0];
			}
			break;

		case ADD:
			next_latency = std::max(latency[a], latency[b]);
			while ((next_latency < TOTAL_LATENCY) && all_alu_busy(next_latency))
			{
				++next_latency;
			}
			next_latency += 1;

			if (next_latency <= TOTAL_LATENCY)
			{
				int c = reinterpret_cast<const int8_t*>(data)[1];

				DUMP(f, "\tr" << a << " += " << 'r' << b << ((c < 0) ? " - " : " + "));
				DUMP(f_asm, "\tlea\t" << reg[a] << ", [" << reg[a] << "+" << reg[b] << ((c < 0) ? "-" : "+"));

				min_possible_latency[a] = std::max(min_possible_latency[a], min_possible_latency[b]) + 1;
				use_alu(next_latency - 1);
				latency[a] = next_latency;
				if (c < 0)
				{
					c = -c;
				}
				DUMP(f, c << ";\t");
				DUMP(f_asm, c << "]");
				code[code_size++] = data[0];
				code[code_size++] = data[1];
			}
			break;

		case SUB:
			next_latency = std::max(latency[a], latency[(a != b) ? b : (b + 4)]);
			while ((next_latency < TOTAL_LATENCY) && all_alu_busy(next_latency))
			{
				++next_latency;
			}
			next_latency += 1;

			if (next_latency <= TOTAL_LATENCY)
			{
				DUMP(f, "\tr" << a << " -= " << 'r' << ((a != b) ? b : (b + 4)) << ";\t");
				DUMP(f_asm, "\tsub\t" << reg[a] << ", " << reg[((a != b) ? b : (b + 4))]);
				min_possible_latency[a] = std::max(min_possible_latency[a], min_possible_latency[(a != b) ? b : (b + 4)]) + 1;
				use_alu(next_latency - 1);
				latency[a] = next_latency;
				code[code_size++] = data[0];
			}
			break;

		case ROR:
			next_latency = std::max(latency[a], latency[b]);
			while ((next_latency < TOTAL_LATENCY) && all_alu_busy(next_latency))
			{
				++next_latency;
			}
			next_latency += 1;

			if (next_latency <= TOTAL_LATENCY)
			{
				DUMP(f, "\tr" << a << " = _rotr(r" << a << ", r" << b << ");");
				if (b != prev_rot_src)
				{
					DUMP(f_asm, "\tmov\tecx, " << reg[b] << "\n");
					prev_rot_src = b;

					const uint8_t* p1 = (const uint8_t*)instructions_mov[data[0]];
					const uint8_t* p2 = (const uint8_t*)instructions_mov[data[0] + 1];
					machine_code.insert(machine_code.end(), p1, p2);
				}
				DUMP(f_asm, "\tror\t" << reg[a] << ", cl");
				min_possible_latency[a] = std::max(min_possible_latency[a], min_possible_latency[b]) + 1;
				use_alu(next_latency - 1);
				latency[a] = next_latency;
				code[code_size++] = data[0];
			}
			break;

		case ROL:
			next_latency = std::max(latency[a], latency[b]);
			while ((next_latency < TOTAL_LATENCY) && all_alu_busy(next_latency))
			{
				++next_latency;
			}
			next_latency += 1;

			if (next_latency <= TOTAL_LATENCY)
			{
				DUMP(f, "\tr" << a << " = _rotl(r" << a << ", r" << b << ");");
				if (b != prev_rot_src)
				{
					DUMP(f_asm, "\tmov\tecx, " << reg[b] << "\n");
					prev_rot_src = b;

					const uint8_t* p1 = (const uint8_t*)instructions_mov[data[0]];
					const uint8_t* p2 = (const uint8_t*)instructions_mov[data[0] + 1];
					machine_code.insert(machine_code.end(), p1, p2);
				}
				DUMP(f_asm, "\trol\t" << reg[a] << ", cl");
				min_possible_latency[a] = std::max(min_possible_latency[a], min_possible_latency[b]) + 1;
				use_alu(next_latency - 1);
				latency[a] = next_latency;
				code[code_size++] = data[0];
			}
			break;

		case XOR:
			next_latency = std::max(latency[a], latency[(a != b) ? b : (b + 4)]);
			while ((next_latency < TOTAL_LATENCY) && all_alu_busy(next_latency))
			{
				++next_latency;
			}
			next_latency += 1;

			if (next_latency <= TOTAL_LATENCY)
			{
				DUMP(f, "\tr" << a << " ^= " << 'r' << ((a != b) ? b : (b + 4)) << ";\t");
				DUMP(f_asm, "\txor\t" << reg[a] << ", " << reg[((a != b) ? b : (b + 4))]);
				min_possible_latency[a] = std::max(min_possible_latency[a], min_possible_latency[(a != b) ? b : (b + 4)]) + 1;
				use_alu(next_latency - 1);
				latency[a] = next_latency;
				code[code_size++] = data[0];
			}
			break;
		}

		if (prev_code_size != code_size)
		{
			DUMP(f, "\t\t// " << latency[0] << ", " << latency[1] << ", " << latency[2] << ", " << latency[3] << "\n");
			DUMP(f_asm, "\n");
			++num_instructions;

			if (a == prev_rot_src)
			{
				prev_rot_src = (uint32_t)(-1);
			}

			const uint8_t* p1 = (const uint8_t*) instructions[data[0]];
			const uint8_t* p2 = (const uint8_t*) instructions[data[0] + 1];
			machine_code.insert(machine_code.end(), p1, p2);
			if (inst.opcode == ADD)
			{
				machine_code.back() = data[1];
			}
		}
		else
		{
			++num_retries;
		}
	} while (((latency[0] < TOTAL_LATENCY) || (latency[1] < TOTAL_LATENCY) || (latency[2] < TOTAL_LATENCY) || (latency[3] < TOTAL_LATENCY)) && (num_retries < 64));
	DUMP(f, "\n");

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part2, (const uint8_t*) CryptonightR_template_part3);

	*(int*)(machine_code.data() + machine_code.size() - 4) = static_cast<int>((((const uint8_t*)CryptonightR_template_mainloop) - ((const uint8_t*)CryptonightR_template_part1)) - machine_code.size());

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part3, (const uint8_t*) CryptonightR_instruction0);

#if DUMP_CODE
	f << "\t// Theoretical minimal latency: " << min_possible_latency[0] << ", " << min_possible_latency[1] << ", " << min_possible_latency[2] << ", " << min_possible_latency[3] << "\n";
	f << "\t// Total instructions: " << num_instructions << "\n";
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
		const SInstruction inst = reinterpret_cast<SInstruction*>(&i)[0];

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
			f_asm << "\tsub\t" << reg[a] << ", " << reg[((a != b) ? b : (b + 4))];
			break;

		case ROR:
			f_asm << "\tror\t" << reg[a] << ", cl";
			break;

		case ROL:
			f_asm << "\trol\t" << reg[a] << ", cl";
			break;

		case XOR:
			f_asm << "\txor\t" << reg[a] << ", " << reg[((a != b) ? b : (b + 4))];
			break;
		}
		f_asm << "\n";
	}

	for (int i = 0; i <= 256; ++i)
	{
		f_asm << "CryptonightR_instruction_mov" << i << ":\n";
		const SInstruction inst = reinterpret_cast<SInstruction*>(&i)[0];

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
	//generate_asm_template();

#if DUMP_CODE
	std::mt19937 rnd;
	rnd.seed(RND_SEED);

	uint8_t code[1024];
	int num_instructions;
	int code_size;
	std::vector<uint8_t> machine_code;
	generate_code(rnd, code, num_instructions, code_size, machine_code);
#endif

	return CryptonightR_test();
}
