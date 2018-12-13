#pragma once

#include <stdint.h>
#include <memory.h>

extern "C" void hash_extra_blake(const void *data, size_t length, char *hash);

#include "../slow_hash_test/variant4_random_math.h"

constexpr uint32_t RND_SEED = 123;
constexpr uint32_t BENCHMARK_DURATION = 5; // seconds

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
#define NOINLINE __attribute__ ((noinline))
#define UNREACHABLE __builtin_unreachable()
#else
#define FORCEINLINE __forceinline
#define NOINLINE __declspec(noinline)
#define UNREACHABLE __assume(false)
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
#define DUMP_SOURCE_CODE 0
