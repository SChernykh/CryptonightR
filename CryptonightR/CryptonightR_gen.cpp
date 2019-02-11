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
static const char* reg32[9] = {
	"ebx", "esi", "edi", "ebp",
	"esp", "r15d", "eax", "edx",
	"r9d"
};

static const char* reg64[9] = {
	"rbx", "rsi", "rdi", "rbp",
	"rsp", "r15", "rax", "rdx",
	"r9"
};

int compile_code(const V4_Instruction* code, std::vector<uint8_t>& machine_code)
{
	int num_insts = 0;
	machine_code.clear();
	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part1, (const uint8_t*) CryptonightR_template_part2);

#if DUMP_SOURCE_CODE
#define DUMP(x, ...) x << __VA_ARGS__

	std::ofstream f("random_math.inl");
	std::ofstream f_double("random_math_double.inl");
	std::ofstream f_asm("random_math.inc");
	DUMP(f, "// Auto-generated file, do not edit\n\n");
	DUMP(f_double, "// Auto-generated file, do not edit\n\n");
	DUMP(f_asm, "; Auto-generated file, do not edit\n\n");
	DUMP(f, "FORCEINLINE void random_math(v4_reg& r0, v4_reg& r1, v4_reg& r2, v4_reg& r3, const v4_reg r4, const v4_reg r5, const v4_reg r6, const v4_reg r7, const v4_reg r8)\n");
	DUMP(f, "{\n");
	DUMP(f_double, "FORCEINLINE void random_math_double(__m128i& r0, __m128i& r1, __m128i& r2, __m128i& r3, const __m128i r4, const __m128i r5, const __m128i r6, const __m128i r7, const __m128i r8)\n");
	DUMP(f_double, "{\n");
#else
#define DUMP(x, ...)
#endif

	uint32_t prev_rot_src = (uint32_t)(-1);

#if RANDOM_MATH_64_BIT == 1
#define reg reg64
#else
#define reg reg32
#endif

	for (int i = 0;; ++i, ++num_insts)
	{
		const V4_Instruction inst = code[i];
		if (inst.opcode == RET)
		{
			break;
		}

		const uint8_t opcode = (inst.opcode == MUL) ? inst.opcode : (inst.opcode + 2);
		const uint8_t dst_index = inst.dst_index;
		const uint8_t src_index = inst.src_index;

		const uint32_t a = inst.dst_index;
		const uint32_t b = inst.src_index;
		const uint8_t c = opcode | (dst_index << V4_OPCODE_BITS) | (((src_index == 8) ? dst_index : src_index) << (V4_OPCODE_BITS + V4_DST_INDEX_BITS));

		switch (inst.opcode)
		{
		case MUL:
			DUMP(f, "\tr" << a << " *= " << 'r' << b << ";\t");
			DUMP(f_double, "\tr" << a << " = _mm_mul_epu32(r" << a << ", r" << b << ");\t");
			DUMP(f_asm, "\timul\t" << reg64[a] << ", " << reg64[b]);
			break;

		case ADD:
			{
				DUMP(f, "\tr" << a << " += " << 'r' << b << " + ");
				DUMP(f_double, "\tr" << a << " = _mm_add_epi32(_mm_add_epi32(r" << a << ", r" << b << "), _mm_shuffle_epi32(_mm_cvtsi32_si128(" << static_cast<int32_t>(inst.C) << "), _MM_SHUFFLE(1, 0, 1, 0)));\t");
				DUMP(f_asm, "\tadd\t" << reg64[a] << ", " << reg64[b] << "\n");
#if RANDOM_MATH_64_BIT == 1
                DUMP(f_asm, "\tmov\tecx, " << inst.C << "\n");
                DUMP(f_asm, "\tadd\t" << reg64[a] << ", rcx");
                prev_rot_src = (uint32_t)(-1);
#else
                DUMP(f_asm, "\tadd\t" << reg64[a] << ", " << inst.C);
#endif
				DUMP(f, inst.C << "U;\t");
			}
			break;

		case SUB:
			DUMP(f, "\tr" << a << " -= " << 'r' << b << ";\t");
			DUMP(f_double, "\tr" << a << " = _mm_sub_epi32(r" << a << ", r" << b << ");\t");
			DUMP(f_asm, "\tsub\t" << reg64[a] << ", " << reg64[b]);
			break;

		case ROR:
#if RANDOM_MATH_64_BIT == 1
			DUMP(f, "\tr" << a << " = _rotr64(r" << a << ", r" << b << ");");
#else
			DUMP(f, "\tr" << a << " = _rotr(r" << a << ", r" << b << ");");
			DUMP(f_double, "\t{\n\
\t\tconst uint32_t c[2] = { _mm_cvtsi128_si32(r" << b << "), _mm_extract_epi32(r" << b << ", 2) };\n\
\t\tconst uint32_t d[2] = { _mm_cvtsi128_si32(r" << a << "), _mm_extract_epi32(r" << a << ", 2) };\n\
\t\tr" << a << " = _mm_insert_epi32(_mm_cvtsi32_si128(_rotr(d[0], c[0])), _rotr(d[1], c[1]), 2);\n\
\t}");
#endif

			if (b != prev_rot_src)
			{
				DUMP(f_asm, "\tmov\trcx, " << reg64[b] << "\n");
				prev_rot_src = b;

				const uint8_t* p1 = (const uint8_t*)instructions_mov[c];
				const uint8_t* p2 = (const uint8_t*)instructions_mov[c + 1];
				machine_code.insert(machine_code.end(), p1, p2);
			}
			DUMP(f_asm, "\tror\t" << reg[a] << ", cl");
			break;

		case ROL:
#if RANDOM_MATH_64_BIT == 1
			DUMP(f, "\tr" << a << " = _rotl64(r" << a << ", r" << b << ");");
#else
			DUMP(f, "\tr" << a << " = _rotl(r" << a << ", r" << b << ");");
			DUMP(f_double, "\t{\n\
\t\tconst uint32_t c[2] = { _mm_cvtsi128_si32(r" << b << "), _mm_extract_epi32(r" << b << ", 2) };\n\
\t\tconst uint32_t d[2] = { _mm_cvtsi128_si32(r" << a << "), _mm_extract_epi32(r" << a << ", 2) };\n\
\t\tr" << a << " = _mm_insert_epi32(_mm_cvtsi32_si128(_rotl(d[0], c[0])), _rotl(d[1], c[1]), 2);\n\
\t}");
#endif

			if (b != prev_rot_src)
			{
				DUMP(f_asm, "\tmov\trcx, " << reg64[b] << "\n");
				prev_rot_src = b;

				const uint8_t* p1 = (const uint8_t*)instructions_mov[c];
				const uint8_t* p2 = (const uint8_t*)instructions_mov[c + 1];
				machine_code.insert(machine_code.end(), p1, p2);
			}
			DUMP(f_asm, "\trol\t" << reg[a] << ", cl");
			break;

		case XOR:
			DUMP(f, "\tr" << a << " ^= " << 'r' << b << ";\t");
			DUMP(f_double, "\tr" << a << " = _mm_xor_si128(r" << a << ", r" << b << ");\t");
			DUMP(f_asm, "\txor\t" << reg64[a] << ", " << reg64[b]);
			break;
		}

		DUMP(f, "\n");
		DUMP(f_double, "\n");
		DUMP(f_asm, "\n");

		if (a == prev_rot_src)
		{
			prev_rot_src = (uint32_t)(-1);
		}

		const uint8_t* p1 = (const uint8_t*)instructions[c];
		const uint8_t* p2 = (const uint8_t*)instructions[c + 1];
		machine_code.insert(machine_code.end(), p1, p2);
		if (inst.opcode == ADD)
		{
            uint32_t* p = reinterpret_cast<uint32_t*>(machine_code.data() + machine_code.size() - sizeof(uint32_t)
#if RANDOM_MATH_64_BIT == 1
                - 3
#endif
            );
            *p = inst.C;
        }
	}

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part2, (const uint8_t*) CryptonightR_template_part3);

	*(int*)(machine_code.data() + machine_code.size() - 4) = static_cast<int>((((const uint8_t*)CryptonightR_template_mainloop) - ((const uint8_t*)CryptonightR_template_part1)) - machine_code.size());

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part3, (const uint8_t*) CryptonightR_template_end);

#if DUMP_SOURCE_CODE
	f << "}\n";
	f_double << "}\n";
	f.close();
	f_double.close();
	f_asm.close();

	std::ofstream f_bin("random_math.bin", std::ios::out | std::ios::binary);
	f_bin.write((const char*)machine_code.data(), machine_code.size());
	f_bin.close();
#endif

	return num_insts;
}

static inline void insert_instructions(const V4_Instruction* code, std::vector<uint8_t>& machine_code)
{
    uint32_t prev_rot_src = (uint32_t)(-1);

    for (int i = 0;; ++i)
    {
        const V4_Instruction inst = code[i];
		if (inst.opcode == RET)
		{
			break;
		}

        const uint8_t opcode = (inst.opcode == MUL) ? inst.opcode : (inst.opcode + 2);

        const uint32_t a = inst.dst_index;
        const uint32_t b = inst.src_index;
        const uint8_t c = opcode | (inst.dst_index << V4_OPCODE_BITS) | (((inst.src_index == 8) ? inst.dst_index : inst.src_index) << (V4_OPCODE_BITS + V4_DST_INDEX_BITS));

        switch (inst.opcode)
        {
        case ROR:
        case ROL:
            if (b != prev_rot_src)
            {
                prev_rot_src = b;

                const uint8_t* p1 = (const uint8_t*) instructions_mov[c];
                const uint8_t* p2 = (const uint8_t*) instructions_mov[c + 1];
                machine_code.insert(machine_code.end(), p1, p2);
            }
            break;
        }

        if (a == prev_rot_src)
        {
            prev_rot_src = (uint32_t)(-1);
        }

        const uint8_t* p1 = (const uint8_t*)instructions[c];
        const uint8_t* p2 = (const uint8_t*)instructions[c + 1];
        machine_code.insert(machine_code.end(), p1, p2);
        if (inst.opcode == ADD)
        {
            uint32_t* p = reinterpret_cast<uint32_t*>(machine_code.data() + machine_code.size() - sizeof(uint32_t)
#if RANDOM_MATH_64_BIT == 1
                - 3
#endif
                );
            *p = inst.C;
#if RANDOM_MATH_64_BIT == 1
            prev_rot_src = (uint32_t)(-1);
#endif
        }
    }
}

void compile_code_double(const V4_Instruction* code, std::vector<uint8_t>& machine_code)
{
    machine_code.clear();
    machine_code.insert(machine_code.end(), (const uint8_t*)CryptonightR_template_double_part1, (const uint8_t*)CryptonightR_template_double_part2);
    insert_instructions(code, machine_code);
    machine_code.insert(machine_code.end(), (const uint8_t*)CryptonightR_template_double_part2, (const uint8_t*)CryptonightR_template_double_part3);
    insert_instructions(code, machine_code);
    machine_code.insert(machine_code.end(), (const uint8_t*)CryptonightR_template_double_part3, (const uint8_t*)CryptonightR_template_double_part4);
    *(int*)(machine_code.data() + machine_code.size() - 4) = static_cast<int>((((const uint8_t*)CryptonightR_template_double_mainloop) - ((const uint8_t*)CryptonightR_template_double_part1)) - machine_code.size());
    machine_code.insert(machine_code.end(), (const uint8_t*)CryptonightR_template_double_part4, (const uint8_t*)CryptonightR_template_double_end);
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
	void CryptonightR_template_end();
	void CryptonightR_template_double_part1();
	void CryptonightR_template_double_mainloop();
	void CryptonightR_template_double_part2();
	void CryptonightR_template_double_part3();
	void CryptonightR_template_double_part4();
	void CryptonightR_template_double_end();
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
)===";

	for (int i = 0; i <= 256; ++i)
		f_asm << "PUBLIC CryptonightR_instruction" << i << "\n";

	for (int i = 0; i <= 256; ++i)
		f_asm << "PUBLIC CryptonightR_instruction_mov" << i << "\n";

	f_asm << "\nINCLUDE CryptonightR_template.inc\n\n";

#if RANDOM_MATH_64_BIT == 1
#define reg reg64
#else
#define reg reg32
#endif

	for (int i = 0; i <= 256; ++i)
	{
		f_asm << "CryptonightR_instruction" << i << ":\n";

		const uint8_t c = i;

		uint8_t opcode = c & ((1 << V4_OPCODE_BITS) - 1);
		opcode = (opcode <= 2) ? MUL : (opcode - 2);

		const uint32_t a = (c >> V4_OPCODE_BITS) & ((1 << V4_DST_INDEX_BITS) - 1);
		uint32_t b = (c >> (V4_OPCODE_BITS + V4_DST_INDEX_BITS)) & ((1 << V4_SRC_INDEX_BITS) - 1);
		if (((opcode == ADD) || (opcode == SUB) || (opcode == XOR)) && (a == b))
		{
			b = 8;
		}

		switch (opcode)
		{
		case MUL:
			f_asm << "\timul\t" << reg64[a] << ", " << reg64[b];
			break;

		case ADD:
            f_asm << "\tadd\t" << reg64[a] << ", " << reg64[b] << "\n";
#if RANDOM_MATH_64_BIT == 1
            f_asm << "\tmov ecx, 80000000h\n";
            f_asm << "\tadd\t" << reg64[a] << ", rcx";
#else
			f_asm << "\tadd\t" << reg64[a] << ", 80000000h";
#endif
			break;

		case SUB:
			f_asm << "\tsub\t" << reg64[a] << ", " << reg64[b];
			break;

		case ROR:
			f_asm << "\tror\t" << reg[a] << ", cl";
			break;

		case ROL:
			f_asm << "\trol\t" << reg[a] << ", cl";
			break;

		case XOR:
			f_asm << "\txor\t" << reg64[a] << ", " << reg64[b];
			break;
		}
		f_asm << "\n";
	}

	for (int i = 0; i <= 256; ++i)
	{
		f_asm << "CryptonightR_instruction_mov" << i << ":\n";

		const uint8_t c = i;

		uint8_t opcode = c & ((1 << V4_OPCODE_BITS) - 1);
		opcode = (opcode <= 2) ? MUL : (opcode - 2);

		const uint32_t b = (c >> (V4_OPCODE_BITS + V4_DST_INDEX_BITS)) & ((1 << V4_SRC_INDEX_BITS) - 1);

		switch (opcode)
		{
		case ROR:
		case ROL:
			f_asm << "\tmov\trcx, " << reg64[b];
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
#if DUMP_SOURCE_CODE
	generate_asm_template();

	V4_Instruction code[NUM_INSTRUCTIONS_MAX + 1];
	v4_random_math_init(code, RND_SEED);

	std::vector<uint8_t> machine_code;
	compile_code(code, machine_code);
	return 0;
#else
	return CryptonightR_test();
#endif
}
