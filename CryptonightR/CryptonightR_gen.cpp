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
	DUMP(f, "// Auto-generated file, do not edit\n\n");
	DUMP(f_asm, "; Auto-generated file, do not edit\n\n");
	DUMP(f, "FORCEINLINE void random_math(uint32_t& r0, uint32_t& r1, uint32_t& r2, uint32_t& r3, const uint32_t r4, const uint32_t r5, const uint32_t r6, const uint32_t r7)\n");
	DUMP(f, "{\n");
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

	machine_code.insert(machine_code.end(), (const uint8_t*) CryptonightR_template_part3, (const uint8_t*) CryptonightR_template_end);

#if DUMP_SOURCE_CODE
	f << "}\n";
	f.close();
	f_asm.close();

	std::ofstream f_bin("random_math.bin", std::ios::out | std::ios::binary);
	f_bin.write((const char*)machine_code.data(), machine_code.size());
	f_bin.close();
#endif
}

static inline void insert_instructions(const V4_Instruction* code, int code_size, std::vector<uint8_t>& machine_code)
{
    uint32_t prev_rot_src = (uint32_t)(-1);

    for (int i = 0; i < code_size; ++i)
    {
        const V4_Instruction inst = code[i];

        const uint32_t a = inst.dst_index;
        const uint32_t b = inst.src_index;
        const uint8_t c = reinterpret_cast<const uint8_t*>(code)[i];

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
            ++i;
            machine_code.back() = reinterpret_cast<const uint8_t*>(code)[i];
        }
    }
}

void compile_code_double(const V4_Instruction* code, int code_size, std::vector<uint8_t>& machine_code)
{
    machine_code.clear();
    machine_code.insert(machine_code.end(), (const uint8_t*)CryptonightR_template_double_part1, (const uint8_t*)CryptonightR_template_double_part2);
    insert_instructions(code, code_size, machine_code);
    machine_code.insert(machine_code.end(), (const uint8_t*)CryptonightR_template_double_part2, (const uint8_t*)CryptonightR_template_double_part3);
    insert_instructions(code, code_size, machine_code);
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
	//generate_asm_template();

#if DUMP_SOURCE_CODE
	V4_Instruction code[256];
	int code_size = v4_random_math_init(code, RND_SEED);

	std::vector<uint8_t> machine_code;
	compile_code(code, code_size, machine_code);
#endif

	return CryptonightR_test();
}
