#pragma once

#include <stdint.h>

enum EInstructionList
{
	// MUL has 3 times higher frequency
	MUL1,	// a*b
	MUL2,	// a*b
	MUL3,	// a*b
	ADD,	// a+b + C, -128 <= C <= 127
	SUB,	// a-b
	ROR,	// rotate right "a" by "b & 31" bits
	ROL,	// rotate left "a" by "b & 31" bits
	XOR,	// a^b
};

// There are 8 total registers:
// - 4 variable registers
// - 4 constant registers initialized from loop variables
//
// This is why dst_index is 2 bits
struct SInstruction
{
	uint8_t opcode : 3;
	uint8_t dst_index : 2;
	uint8_t src_index : 3;
};
static_assert(sizeof(SInstruction) == 1, "Instruction size must be 1 byte");

constexpr uint32_t RND_SEED = 123;
constexpr uint32_t BENCHMARK_DURATION = 5; // seconds

// Generate code with latency = 54 cycles, which is equivalent to 18 multiplications
constexpr uint32_t TOTAL_LATENCY = 18 * 3;

// Available ALU count for latency calculation
constexpr int ALU_COUNT = 2;

// Registers to use in generated x86-64 code
static const char* reg[8] = {
	"ebx", "esi", "edi", "ebp",
	"esp", "r15d", "eax", "edx"
};

#define MEMORY 2097152

typedef struct {
	uint8_t hash_state[224]; // Need only 200, explicit align
	uint8_t* long_state;
	uint8_t ctx_info[24]; //Use some of the extra memory for flags
	const void* input;
	uint8_t* variant1_table;
	const uint32_t* t_fn;
} cryptonight_ctx;

#ifdef __GNUC__
#define FORCEINLINE __attribute__((always_inline)) inline
#else
#define FORCEINLINE __forceinline
#endif

#ifdef __GNUC__

#include <x86intrin.h>

static inline uint64_t __umul128(uint64_t a, uint64_t b, uint64_t* hi)
{
	unsigned __int128 r = (unsigned __int128)a * (unsigned __int128)b;
	*hi = r >> 64;
	return (uint64_t)r;
}

#else

#include <intrin.h>
#define __umul128 _umul128

#endif // __GNUC__

// If you changed RND_SEED, set it to 1 to update random_math.inc, random_math.inl, random_math.bin
#define DUMP_CODE 0
