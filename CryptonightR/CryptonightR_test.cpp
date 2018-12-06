#include "definitions.h"
#include <chrono>
#include <iostream>
#include <random>
#include <atomic>

// CryptonightR reference implementation
// It's basically CryptonightV2 with random math instead of div+sqrt
void CryptonightR_ref(cryptonight_ctx* ctx0, const V4_Instruction* code, int code_size)
{
	uint8_t* l0 = ctx0->long_state;
	uint64_t* h0 = (uint64_t*)ctx0->hash_state;

	__m128i ax0 = _mm_set_epi64x(h0[1] ^ h0[5], h0[0] ^ h0[4]);
	__m128i bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);
	__m128i bx1 = _mm_set_epi64x(h0[9] ^ h0[11], h0[8] ^ h0[10]);

	uint64_t idx0 = h0[0] ^ h0[4];
	uint64_t idx1 = idx0 & 0x1FFFF0;

	// 8 registers for random math
	// r0-r3 are variable
	// r4-r7 are constants taken from main loop registers on every iteration
	uint32_t r[8];

	// Initial register values for random math
	r[0] = static_cast<uint32_t>(h0[12]);
	r[1] = static_cast<uint32_t>(h0[12] >> 32);
	r[2] = static_cast<uint32_t>(h0[13]);
	r[3] = static_cast<uint32_t>(h0[13] >> 32);

	for (size_t i = 0; i < 524288; i++)
	{
		__m128i cx = _mm_load_si128((__m128i *)&l0[idx1]);

		cx = _mm_aesenc_si128(cx, ax0);

		// SHUFFLE1 from CryptonightV2
		{
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x10]);
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x20]);
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x30]);
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x10], _mm_add_epi64(chunk3, bx1));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x20], _mm_add_epi64(chunk1, bx0));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x30], _mm_add_epi64(chunk2, ax0));
		}

		_mm_store_si128((__m128i *)&l0[idx1], _mm_xor_si128(bx0, cx));

		idx0 = _mm_cvtsi128_si64(cx);
		idx1 = idx0 & 0x1FFFF0;

		uint64_t hi, lo, cl, ch;
		cl = ((uint64_t*)&l0[idx1])[0];
		ch = ((uint64_t*)&l0[idx1])[1];

		// Random math (replaces integer math from CryptonightV2)
		{
			cl ^= (r[0] + r[1]) | (static_cast<uint64_t>(r[2] + r[3]) << 32);

			// Random math constants are taken from main loop registers
			// They're new on every iteration
			r[4] = static_cast<uint32_t>(_mm_cvtsi128_si32(ax0));
			r[5] = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_srli_si128(ax0, 8)));
			r[6] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx0));
			r[7] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx1));

			v4_random_math(code, code_size, r);
		}

		lo = _umul128(idx0, cl, &hi);

		// SHUFFLE2 from CNv2
		{
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l0[idx1 ^ 0x10]), _mm_set_epi64x(lo, hi));
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x20]);
			hi ^= ((uint64_t*)&l0[idx1 ^ 0x20])[0];
			lo ^= ((uint64_t*)&l0[idx1 ^ 0x20])[1];
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x30]);
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x10], _mm_add_epi64(chunk3, bx1));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x20], _mm_add_epi64(chunk1, bx0));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x30], _mm_add_epi64(chunk2, ax0));
		}

		uint64_t al0 = ax0.m128i_u64[0] + hi;
		uint64_t ah0 = ax0.m128i_u64[1] + lo;
		((uint64_t*)&l0[idx1])[0] = al0;
		((uint64_t*)&l0[idx1])[1] = ah0;
		ah0 ^= ch;
		al0 ^= cl;
		ax0 = _mm_set_epi64x(ah0, al0);
		idx0 = al0;
		idx1 = idx0 & 0x1FFFF0;

		bx1 = bx0;
		bx0 = cx;
	}
}

#include "random_math.inl"

// CryptonightR C++ generated code
void CryptonightR(cryptonight_ctx* ctx0)
{
	uint8_t* l0 = ctx0->long_state;
	uint64_t* h0 = (uint64_t*)ctx0->hash_state;

	__m128i ax0 = _mm_set_epi64x(h0[1] ^ h0[5], h0[0] ^ h0[4]);
	__m128i bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);
	__m128i bx1 = _mm_set_epi64x(h0[9] ^ h0[11], h0[8] ^ h0[10]);

	uint64_t idx0 = h0[0] ^ h0[4];
	uint64_t idx1 = idx0 & 0x1FFFF0;

	uint32_t r0 = static_cast<uint32_t>(h0[12]);
	uint32_t r1 = static_cast<uint32_t>(h0[12] >> 32);
	uint32_t r2 = static_cast<uint32_t>(h0[13]);
	uint32_t r3 = static_cast<uint32_t>(h0[13] >> 32);

	for (size_t i = 0; i < 524288; i++)
	{
		__m128i cx = _mm_load_si128((__m128i *)&l0[idx1]);

		cx = _mm_aesenc_si128(cx, ax0);

		{
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x10]);
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x20]);
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x30]);
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x10], _mm_add_epi64(chunk3, bx1));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x20], _mm_add_epi64(chunk1, bx0));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x30], _mm_add_epi64(chunk2, ax0));
		}

		_mm_store_si128((__m128i *)&l0[idx1], _mm_xor_si128(bx0, cx));

		idx0 = _mm_cvtsi128_si64(cx);
		idx1 = idx0 & 0x1FFFF0;

		uint64_t hi, lo, cl, ch;
		cl = ((uint64_t*)&l0[idx1])[0];
		ch = ((uint64_t*)&l0[idx1])[1];

		cl ^= (r0 + r1) | (static_cast<uint64_t>(r2 + r3) << 32);
		random_math(r0, r1, r2, r3, ax0.m128i_u32[0], ax0.m128i_u32[2], bx0.m128i_u32[0], bx1.m128i_u32[0]);

		lo = _umul128(idx0, cl, &hi);

		{
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l0[idx1 ^ 0x10]), _mm_set_epi64x(lo, hi));
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x20]);
			hi ^= ((uint64_t*)&l0[idx1 ^ 0x20])[0];
			lo ^= ((uint64_t*)&l0[idx1 ^ 0x20])[1];
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[idx1 ^ 0x30]);
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x10], _mm_add_epi64(chunk3, bx1));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x20], _mm_add_epi64(chunk1, bx0));
			_mm_store_si128((__m128i *)&l0[idx1 ^ 0x30], _mm_add_epi64(chunk2, ax0));
		}

		uint64_t al0 = ax0.m128i_u64[0] + hi;
		uint64_t ah0 = ax0.m128i_u64[1] + lo;
		((uint64_t*)&l0[idx1])[0] = al0;
		((uint64_t*)&l0[idx1])[1] = ah0;
		ah0 ^= ch;
		al0 ^= cl;
		ax0 = _mm_set_epi64x(ah0, al0);
		idx0 = al0;
		idx1 = idx0 & 0x1FFFF0;

		bx1 = bx0;
		bx0 = cx;
	}
}

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOL AddPrivilege(const TCHAR* pszPrivilege)
{
	HANDLE           hToken;
	TOKEN_PRIVILEGES tp;
	BOOL             status;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return FALSE;

	if (!LookupPrivilegeValue(NULL, pszPrivilege, &tp.Privileges[0].Luid))
		return FALSE;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	status = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	if (!status || (GetLastError() != ERROR_SUCCESS))
		return FALSE;

	CloseHandle(hToken);
	return TRUE;
}

cryptonight_ctx* cryptonight_alloc_ctx()
{
	cryptonight_ctx* ptr = (cryptonight_ctx*)_mm_malloc(sizeof(cryptonight_ctx), 4096);

	SIZE_T iLargePageMin = GetLargePageMinimum();
	if (MEMORY > iLargePageMin)
	{
		iLargePageMin *= 2;
	}

	ptr->long_state = (uint8_t*) VirtualAlloc(NULL, iLargePageMin, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);

	std::mt19937_64 rnd;
	for (int i = 0; i < MEMORY / sizeof(uint64_t); ++i)
	{
		((uint64_t*)ptr->long_state)[i] = rnd();
	}
	for (int i = 0; i < sizeof(ptr->hash_state) / sizeof(uint64_t); ++i)
	{
		((uint64_t*)ptr->hash_state)[i] = rnd();
	}

	ptr->ctx_info[0] = 1;

	return ptr;
}

typedef void(*mainloop_func)(cryptonight_ctx*);

extern "C" void CryptonightR_asm(cryptonight_ctx* ctx0);
extern "C" void cnv2_mainloop_ivybridge_asm(cryptonight_ctx* ctx0);
extern "C" void cnv2_mainloop_ryzen_asm(cryptonight_ctx* ctx0);
extern void compile_code(const V4_Instruction* code, int code_size, std::vector<uint8_t>& machine_code);

static double get_rdtsc_speed()
{
	uint64_t tsc1 = __rdtsc();
	auto t1 = std::chrono::high_resolution_clock::now();
	for (;;)
	{
		uint64_t tsc2 = __rdtsc();
		auto t2 = std::chrono::high_resolution_clock::now();
		const double dt = static_cast<double>(t2.time_since_epoch().count() - t1.time_since_epoch().count());
		if (dt > 1e9)
		{
			return (tsc2 - tsc1) / dt;
		}
	}
}

static double rdtsc_speed = get_rdtsc_speed();

template<typename T, typename ...Us>
static void benchmark(T f, const char* name, Us... args)
{
	const int64_t end_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + static_cast<uint64_t>(BENCHMARK_DURATION) * 1000000000;
	int64_t min_dt = std::numeric_limits<int64_t>::max();

	do
	{
		const uint64_t t1 = __rdtsc();
		std::atomic_thread_fence(std::memory_order_seq_cst);

		f(args...);

		std::atomic_thread_fence(std::memory_order_seq_cst);
		const uint64_t t2 = __rdtsc();

		const int64_t dt = t2 - t1;
		if (dt < min_dt)
		{
			min_dt = dt;
		}
	} while (std::chrono::high_resolution_clock::now().time_since_epoch().count() < end_time);

	std::cout << name << ": " << min_dt / (rdtsc_speed * 524288.0) << " ns/iteration" << std::endl;
}

int CryptonightR_test()
{
	SetThreadAffinityMask(GetCurrentThread(), 1 << 3);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	AddPrivilege(TEXT("SeLockMemoryPrivilege"));
	cryptonight_ctx* ctx0 = cryptonight_alloc_ctx();
	cryptonight_ctx* ctx1 = cryptonight_alloc_ctx();
	cryptonight_ctx* ctx2 = cryptonight_alloc_ctx();
	cryptonight_ctx* ctx3 = cryptonight_alloc_ctx();

	V4_Instruction code[1024];
	int code_size = v4_random_math_init(code, RND_SEED);
	std::vector<uint8_t> machine_code;
	compile_code(code, code_size, machine_code);

	mainloop_func CryptonightR_generated = (mainloop_func) VirtualAlloc(0, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	memcpy(CryptonightR_generated, machine_code.data(), machine_code.size());

	// Do initial integrity check
	CryptonightR_ref(ctx0, code, code_size);
	CryptonightR(ctx1);
	CryptonightR_asm(ctx2);
	CryptonightR_generated(ctx3);

	if (memcmp(ctx0->long_state, ctx1->long_state, MEMORY) != 0)
	{
		std::cerr << "C++ generated code doesn't match reference code" << std::endl;
		return 1;
	}

	if (memcmp(ctx0->long_state, ctx2->long_state, MEMORY) != 0)
	{
		std::cerr << "ASM generated code doesn't match reference code" << std::endl;
		return 2;
	}

	if (memcmp(ctx0->long_state, ctx3->long_state, MEMORY) != 0)
	{
		std::cerr << "Generated machine code doesn't match reference code" << std::endl;
		return 3;
	}

	// Run benchmarks if the integrity check passed
	std::cout << "rdtsc speed: " << rdtsc_speed << " GHz" << std::endl;
	std::cout << "Running " << BENCHMARK_DURATION << " second benchmarks..." << std::endl;

	benchmark(CryptonightR_ref, "CryptonightR (reference code)", ctx0, code, code_size);
	benchmark(CryptonightR, "CryptonightR (C++ code)", ctx1);
	benchmark(CryptonightR_asm, "CryptonightR (ASM code)", ctx2);
	benchmark(CryptonightR_generated, "CryptonightR (generated machine code)", ctx3);

	// Show CryptonightV2 performance for comparison
	{
		int data[4];
		__cpuidex(data, 0, 0);
		char vendor[32] = {};
		((int*)vendor)[0] = data[1];
		((int*)vendor)[1] = data[3];
		((int*)vendor)[2] = data[2];

		mainloop_func func = (strcmp(vendor, "GenuineIntel") == 0) ? cnv2_mainloop_ivybridge_asm : cnv2_mainloop_ryzen_asm;
		benchmark(func, "CryptonightV2", ctx1);
	}

	std::cout << std::endl;

	memcpy(ctx0->long_state, ctx3->long_state, MEMORY);

	// Test 1000 random code sequences and compare them with reference code
	for (int i = 1; i <= 1000; ++i)
	{
		code_size = v4_random_math_init(code, i);
		compile_code(code, code_size, machine_code);
		memcpy(CryptonightR_generated, machine_code.data(), machine_code.size());
		FlushInstructionCache(GetCurrentProcess(), CryptonightR_generated, machine_code.size());

		CryptonightR_ref(ctx0, code, code_size);
		CryptonightR_generated(ctx3);

		if (memcmp(ctx0->long_state, ctx3->long_state, MEMORY) != 0)
		{
			std::cerr << "Generated machine code doesn't match reference code" << std::endl;
			return 7;
		}

		std::cout << "\rRandom code test " << i << " passed";
	}

	return 0;
}
