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
static const char* reg32[8] = {
	"ebx", "esi", "edi", "ebp",
	"esp", "r15d", "eax", "edx"
};

static const char* reg64[8] = {
	"rbx", "rsi", "rdi", "rbp",
	"rsp", "r15", "rax", "rdx"
};

int compile_code(const V4_Instruction* code, std::vector<uint8_t>& machine_code)
{
	int num_insts = 0;
	machine_code.clear();
	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part1, (const uint8_t*) CryptonightR_template_part2);

#if DUMP_SOURCE_CODE
#define DUMP(x, ...) x << __VA_ARGS__

	std::ofstream f("random_math.inl");
	std::ofstream f_asm("random_math.inc");
	DUMP(f, "// Auto-generated file, do not edit\n\n");
	DUMP(f_asm, "; Auto-generated file, do not edit\n\n");
	DUMP(f, "FORCEINLINE void random_math(v4_reg& r0, v4_reg& r1, v4_reg& r2, v4_reg& r3, const v4_reg r4, const v4_reg r5, const v4_reg r6, const v4_reg r7)\n");
	DUMP(f, "{\n");
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

		V4_InstructionCompact op;
		op.opcode = (inst.opcode == MUL) ? inst.opcode : (inst.opcode + 2);
		op.dst_index = inst.dst_index;
		op.src_index = inst.src_index;

		const uint32_t a = inst.dst_index;
		const uint32_t b = inst.src_index;
		const uint8_t c = *((uint8_t*)&op);

		switch (inst.opcode)
		{
		case MUL:
			DUMP(f, "\tr" << a << " *= " << 'r' << b << ";\t");
			DUMP(f_asm, "\timul\t" << reg64[a] << ", " << reg64[b]);
			break;

		case ADD:
			{
				DUMP(f, "\tr" << a << " += " << 'r' << b << " + ");
				DUMP(f_asm, "\tadd\t" << reg64[a] << ", " << reg64[b] << "\n");
				DUMP(f_asm, "\tadd\t" << reg64[a] << ", " << inst.C);
				DUMP(f, inst.C << "U;\t");
			}
			break;

		case SUB:
			DUMP(f, "\tr" << a << " -= " << 'r' << b << ";\t");
			DUMP(f_asm, "\tsub\t" << reg64[a] << ", " << reg64[b]);
			break;

		case ROR:
#if RANDOM_MATH_64_BIT == 1
			DUMP(f, "\tr" << a << " = _rotr64(r" << a << ", r" << b << ");");
#else
			DUMP(f, "\tr" << a << " = _rotr(r" << a << ", r" << b << ");");
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
			DUMP(f_asm, "\txor\t" << reg64[a] << ", " << reg64[b]);
			break;
		}

		DUMP(f, "\n");
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
			machine_code.resize(machine_code.size() - sizeof(uint32_t));
			machine_code.insert(machine_code.end(), (uint8_t*)&inst.C, ((uint8_t*)&inst.C) + sizeof(uint32_t));
		}
	}

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part2, (const uint8_t*) CryptonightR_template_part3);

	*(int*)(machine_code.data() + machine_code.size() - 4) = static_cast<int>((((const uint8_t*)CryptonightR_template_mainloop) - ((const uint8_t*)CryptonightR_template_part1)) - machine_code.size());

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part3, (const uint8_t*) CryptonightR_template_end);

#if DUMP_SOURCE_CODE
	f << "}\n";
	f.close();
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

		V4_InstructionCompact op;
		op.opcode = (inst.opcode == MUL) ? inst.opcode : (inst.opcode + 2);
		op.dst_index = inst.dst_index;
		op.src_index = inst.src_index;

        const uint32_t a = inst.dst_index;
        const uint32_t b = inst.src_index;
        const uint8_t c = *reinterpret_cast<const uint8_t*>(&op);

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
			machine_code.resize(machine_code.size() - sizeof(uint32_t));
			machine_code.insert(machine_code.end(), (uint8_t*)&inst.C, ((uint8_t*)&inst.C) + sizeof(uint32_t));
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
		const V4_InstructionCompact inst = reinterpret_cast<V4_InstructionCompact*>(&i)[0];

		const uint8_t opcode = (inst.opcode <= 2) ? MUL : (inst.opcode - 2);
		const uint32_t a = inst.dst_index;
		const uint32_t b = inst.src_index;

		switch (opcode)
		{
		case MUL:
			f_asm << "\timul\t" << reg64[a] << ", " << reg64[b];
			break;

		case ADD:
			f_asm << "\tadd\t" << reg64[a] << ", " << reg64[b] << "\n";
			f_asm << "\tadd\t" << reg64[a] << ", 80000000h";
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
		const V4_InstructionCompact inst = reinterpret_cast<V4_InstructionCompact*>(&i)[0];

		const uint8_t opcode = (inst.opcode <= 2) ? MUL : (inst.opcode - 2);
		const uint32_t b = inst.src_index;

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

	V4_Instruction code[NUM_INSTRUCTIONS * 2];
	v4_random_math_init(code, RND_SEED);

	std::vector<uint8_t> machine_code;
	compile_code(code, machine_code);
	return 0;
#else
	return CryptonightR_test();
#endif
}
