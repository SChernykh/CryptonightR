#include "definitions.h"
#include <chrono>
#include <iostream>
#include <random>
#include <atomic>

// CryptonightR reference implementation
// It's basically CryptonightV2 with random math instead of div+sqrt
void CryptonightR_ref(cryptonight_ctx* ctx0, const V4_Instruction* code)
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
	v4_reg r[8];
	v4_reg* data = reinterpret_cast<v4_reg*>(h0 + 12);

	// Initial register values for random math
	r[0] = data[0];
	r[1] = data[1];
	r[2] = data[2];
	r[3] = data[3];

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
#if RANDOM_MATH_64_BIT == 1
		cl ^= (r[0] + r[1]) ^ (r[2] + r[3]);

		// Random math constants are taken from main loop registers
		// They're new on every iteration
		r[4] = static_cast<uint64_t>(_mm_cvtsi128_si64(ax0));
		r[5] = static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_srli_si128(ax0, 8)));
		r[6] = static_cast<uint64_t>(_mm_cvtsi128_si64(bx0));
		r[7] = static_cast<uint64_t>(_mm_cvtsi128_si64(bx1));
#else
		cl ^= (r[0] + r[1]) | (static_cast<uint64_t>(r[2] + r[3]) << 32);

		// Random math constants are taken from main loop registers
		// They're new on every iteration
		r[4] = static_cast<uint32_t>(_mm_cvtsi128_si32(ax0));
		r[5] = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_srli_si128(ax0, 8)));
		r[6] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx0));
		r[7] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx1));
#endif
		v4_random_math(code, r);

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

void CryptonightR_double_ref(cryptonight_ctx* ctx0, cryptonight_ctx* ctx1, const V4_Instruction* code)
{
	uint8_t* l0 = ctx0->long_state;
	uint64_t* h0 = (uint64_t*)ctx0->hash_state;
	uint8_t* l1 = ctx1->long_state;
	uint64_t* h1 = (uint64_t*)ctx1->hash_state;

	uint64_t axl0 = h0[0] ^ h0[4];
	uint64_t axh0 = h0[1] ^ h0[5];
	__m128i bx00 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);
	__m128i bx01 = _mm_set_epi64x(h0[9] ^ h0[11], h0[8] ^ h0[10]);
	uint64_t axl1 = h1[0] ^ h1[4];
	uint64_t axh1 = h1[1] ^ h1[5];
	__m128i bx10 = _mm_set_epi64x(h1[3] ^ h1[7], h1[2] ^ h1[6]);
	__m128i bx11 = _mm_set_epi64x(h1[9] ^ h1[11], h1[8] ^ h1[10]);

	uint64_t idx00 = h0[0] ^ h0[4];
	uint64_t idx10 = h1[0] ^ h1[4];
	uint32_t idx01 = idx00 & 0x1FFFF0;
	uint32_t idx11 = idx10 & 0x1FFFF0;

	v4_reg r[2][8];
	v4_reg* data0 = reinterpret_cast<v4_reg*>(h0 + 12);
	v4_reg* data1 = reinterpret_cast<v4_reg*>(h1 + 12);

	r[0][0] = data0[0];
	r[0][1] = data0[1];
	r[0][2] = data0[2];
	r[0][3] = data0[3];

	r[1][0] = data1[0];
	r[1][1] = data1[1];
	r[1][2] = data1[2];
	r[1][3] = data1[3];

	for (size_t i = 0; i < 524288; i++)
	{
		__m128i cx0 = _mm_load_si128((__m128i *)&l0[idx01]);
		const __m128i ax0 = _mm_set_epi64x(axh0, axl0);
		cx0 = _mm_aesenc_si128(cx0, ax0);

		{
			uint32_t k = idx01 ^ 0x10;
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l0[k]); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk1, bx00)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk2, ax0)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk3, bx01));
		}

		_mm_store_si128((__m128i *)&l0[idx01], _mm_xor_si128(bx00, cx0));

		idx00 = _mm_cvtsi128_si64(cx0);
		idx01 = idx00 & 0x1FFFF0;

		__m128i cx1 = _mm_load_si128((__m128i *)&l1[idx11]);
		const __m128i ax1 = _mm_set_epi64x(axh1, axl1);
		cx1 = _mm_aesenc_si128(cx1, ax1);

		{
			uint32_t k = idx11 ^ 0x10;
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l1[k]); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk1, bx10)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk2, ax1)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk3, bx11));
		}

		_mm_store_si128((__m128i *)&l1[idx11], _mm_xor_si128(bx10, cx1));

		idx10 = _mm_cvtsi128_si64(cx1);
		idx11 = idx10 & 0x1FFFF0;

		uint64_t hi, lo, cl, ch;
		cl = ((uint64_t*)&l0[idx01])[0];
		ch = ((uint64_t*)&l0[idx01])[1];

#if RANDOM_MATH_64_BIT == 1
		cl ^= (r[0][0] + r[0][1]) ^ (r[0][2] + r[0][3]);

		// Random math constants are taken from main loop registers
		// They're new on every iteration
		r[0][4] = static_cast<uint64_t>(_mm_cvtsi128_si64(ax0));
		r[0][5] = static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_srli_si128(ax0, 8)));
		r[0][6] = static_cast<uint64_t>(_mm_cvtsi128_si64(bx00));
		r[0][7] = static_cast<uint64_t>(_mm_cvtsi128_si64(bx01));
#else
		cl ^= (r[0][0] + r[0][1]) | (static_cast<uint64_t>(r[0][2] + r[0][3]) << 32);

		// Random math constants are taken from main loop registers
		// They're new on every iteration
		r[0][4] = static_cast<uint32_t>(_mm_cvtsi128_si32(ax0));
		r[0][5] = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_srli_si128(ax0, 8)));
		r[0][6] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx00));
		r[0][7] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx01));
#endif
		v4_random_math(code, r[0]);

		lo = _umul128(idx00, cl, &hi);

		{
			uint32_t k = idx01 ^ 0x10;
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l0[k]), _mm_set_epi64x(lo, hi)); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[k]);
			hi ^= ((uint64_t*)&l0[k])[0];
			lo ^= ((uint64_t*)&l0[k])[1];
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk1, bx00)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk2, ax0)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk3, bx01));
		}

		axl0 += hi;
		axh0 += lo;
		((uint64_t*)&l0[idx01])[0] = axl0;
		((uint64_t*)&l0[idx01])[1] = axh0;
		axh0 ^= ch;
		axl0 ^= cl;
		idx00 = axl0;
		idx01 = idx00 & 0x1FFFF0;

		cl = ((uint64_t*)&l1[idx11])[0];
		ch = ((uint64_t*)&l1[idx11])[1];

#if RANDOM_MATH_64_BIT == 1
		cl ^= (r[1][0] + r[1][1]) ^ (r[1][2] + r[1][3]);

		// Random math constants are taken from main loop registers
		// They're new on every iteration
		r[1][4] = static_cast<uint64_t>(_mm_cvtsi128_si64(ax1));
		r[1][5] = static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_srli_si128(ax1, 8)));
		r[1][6] = static_cast<uint64_t>(_mm_cvtsi128_si64(bx10));
		r[1][7] = static_cast<uint64_t>(_mm_cvtsi128_si64(bx11));
#else
		cl ^= (r[1][0] + r[1][1]) | (static_cast<uint64_t>(r[1][2] + r[1][3]) << 32);

		// Random math constants are taken from main loop registers
		// They're new on every iteration
		r[1][4] = static_cast<uint32_t>(_mm_cvtsi128_si32(ax1));
		r[1][5] = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_srli_si128(ax1, 8)));
		r[1][6] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx10));
		r[1][7] = static_cast<uint32_t>(_mm_cvtsi128_si32(bx11));
#endif
		v4_random_math(code, r[1]);

		lo = _umul128(idx10, cl, &hi);

		{
			uint32_t k = idx11 ^ 0x10;
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l1[k]), _mm_set_epi64x(lo, hi)); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l1[k]);
			hi ^= ((uint64_t*)&l1[k])[0];
			lo ^= ((uint64_t*)&l1[k])[1];
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk1, bx10)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk2, ax1)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk3, bx11));
		}

		axl1 += hi;
		axh1 += lo;
		((uint64_t*)&l1[idx11])[0] = axl1;
		((uint64_t*)&l1[idx11])[1] = axh1;
		((uint64_t*)&l1[idx11])[1] = axh1;
		axh1 ^= ch;
		axl1 ^= cl;
		idx10 = axl1;
		idx11 = idx10 & 0x1FFFF0;

		bx01 = bx00;
		bx11 = bx10;
		bx00 = cx0;
		bx10 = cx1;
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

	v4_reg* data = reinterpret_cast<v4_reg*>(h0 + 12);
	v4_reg r0 = data[0];
	v4_reg r1 = data[1];
	v4_reg r2 = data[2];
	v4_reg r3 = data[3];

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

#if RANDOM_MATH_64_BIT == 1
		cl ^= (r0 + r1) ^ (r2 + r3);
		random_math(r0, r1, r2, r3, ax0.m128i_u64[0], ax0.m128i_u64[1], bx0.m128i_u64[0], bx1.m128i_u64[0]);
#else
		cl ^= (r0 + r1) | (static_cast<uint64_t>(r2 + r3) << 32);
		random_math(r0, r1, r2, r3, ax0.m128i_u32[0], ax0.m128i_u32[2], bx0.m128i_u32[0], bx1.m128i_u32[0]);
#endif

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

void CryptonightR_double(cryptonight_ctx* ctx0, cryptonight_ctx* ctx1)
{
	uint8_t* l0 = ctx0->long_state;
	uint64_t* h0 = (uint64_t*)ctx0->hash_state;
	uint8_t* l1 = ctx1->long_state;
	uint64_t* h1 = (uint64_t*)ctx1->hash_state;

	uint64_t axl0 = h0[0] ^ h0[4];
	uint64_t axh0 = h0[1] ^ h0[5];
	__m128i bx00 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);
	__m128i bx01 = _mm_set_epi64x(h0[9] ^ h0[11], h0[8] ^ h0[10]);
	uint64_t axl1 = h1[0] ^ h1[4];
	uint64_t axh1 = h1[1] ^ h1[5];
	__m128i bx10 = _mm_set_epi64x(h1[3] ^ h1[7], h1[2] ^ h1[6]);
	__m128i bx11 = _mm_set_epi64x(h1[9] ^ h1[11], h1[8] ^ h1[10]);

	uint64_t idx00 = h0[0] ^ h0[4];
	uint64_t idx10 = h1[0] ^ h1[4];
	uint32_t idx01 = idx00 & 0x1FFFF0;
	uint32_t idx11 = idx10 & 0x1FFFF0;

	v4_reg* data0 = reinterpret_cast<v4_reg*>(h0 + 12);
	v4_reg* data1 = reinterpret_cast<v4_reg*>(h1 + 12);

	v4_reg r00 = data0[0];
	v4_reg r01 = data0[1];
	v4_reg r02 = data0[2];
	v4_reg r03 = data0[3];

	v4_reg r10 = data1[0];
	v4_reg r11 = data1[1];
	v4_reg r12 = data1[2];
	v4_reg r13 = data1[3];

	for (size_t i = 0; i < 524288; i++)
	{
		__m128i cx0 = _mm_load_si128((__m128i *)&l0[idx01]);
		const __m128i ax0 = _mm_set_epi64x(axh0, axl0);
		cx0 = _mm_aesenc_si128(cx0, ax0);

		{
			uint32_t k = idx01 ^ 0x10;
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l0[k]); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk1, bx00)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk2, ax0)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk3, bx01));
		}

		_mm_store_si128((__m128i *)&l0[idx01], _mm_xor_si128(bx00, cx0));

		idx00 = _mm_cvtsi128_si64(cx0);
		idx01 = idx00 & 0x1FFFF0;

		__m128i cx1 = _mm_load_si128((__m128i *)&l1[idx11]);
		const __m128i ax1 = _mm_set_epi64x(axh1, axl1);
		cx1 = _mm_aesenc_si128(cx1, ax1);

		{
			uint32_t k = idx11 ^ 0x10;
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l1[k]); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk1, bx10)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk2, ax1)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk3, bx11));
		}

		_mm_store_si128((__m128i *)&l1[idx11], _mm_xor_si128(bx10, cx1));

		idx10 = _mm_cvtsi128_si64(cx1);
		idx11 = idx10 & 0x1FFFF0;

		uint64_t hi, lo, cl, ch;
		cl = ((uint64_t*)&l0[idx01])[0];
		ch = ((uint64_t*)&l0[idx01])[1];

#if RANDOM_MATH_64_BIT == 1
		cl ^= (r00 + r01) ^ (r02 + r03);
		random_math(r00, r01, r02, r03, ax0.m128i_u64[0], ax0.m128i_u64[1], bx00.m128i_u64[0], bx01.m128i_u64[0]);
#else
		cl ^= (r00 + r01) | (static_cast<uint64_t>(r02 + r03) << 32);
		random_math(r00, r01, r02, r03, ax0.m128i_u32[0], ax0.m128i_u32[2], bx00.m128i_u32[0], bx01.m128i_u32[0]);
#endif

		lo = _umul128(idx00, cl, &hi);

		{
			uint32_t k = idx01 ^ 0x10;
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l0[k]), _mm_set_epi64x(lo, hi)); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[k]);
			hi ^= ((uint64_t*)&l0[k])[0];
			lo ^= ((uint64_t*)&l0[k])[1];
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk1, bx00)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk2, ax0)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk3, bx01));
		}

		axl0 += hi;
		axh0 += lo;
		((uint64_t*)&l0[idx01])[0] = axl0;
		((uint64_t*)&l0[idx01])[1] = axh0;
		axh0 ^= ch;
		axl0 ^= cl;
		idx00 = axl0;
		idx01 = idx00 & 0x1FFFF0;

		cl = ((uint64_t*)&l1[idx11])[0];
		ch = ((uint64_t*)&l1[idx11])[1];

#if RANDOM_MATH_64_BIT == 1
		cl ^= (r10 + r11) ^ (r12 + r13);
		random_math(r10, r11, r12, r13, ax1.m128i_u64[0], ax1.m128i_u64[1], bx10.m128i_u64[0], bx11.m128i_u64[0]);
#else
		cl ^= (r10 + r11) | (static_cast<uint64_t>(r12 + r13) << 32);
		random_math(r10, r11, r12, r13, ax1.m128i_u32[0], ax1.m128i_u32[2], bx10.m128i_u32[0], bx11.m128i_u32[0]);
#endif

		lo = _umul128(idx10, cl, &hi);

		{
			uint32_t k = idx11 ^ 0x10;
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l1[k]), _mm_set_epi64x(lo, hi)); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l1[k]);
			hi ^= ((uint64_t*)&l1[k])[0];
			lo ^= ((uint64_t*)&l1[k])[1];
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk1, bx10)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk2, ax1)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk3, bx11));
		}

		axl1 += hi;
		axh1 += lo;
		((uint64_t*)&l1[idx11])[0] = axl1;
		((uint64_t*)&l1[idx11])[1] = axh1;
		((uint64_t*)&l1[idx11])[1] = axh1;
		axh1 ^= ch;
		axl1 ^= cl;
		idx10 = axl1;
		idx11 = idx10 & 0x1FFFF0;

		bx01 = bx00;
		bx11 = bx10;
		bx00 = cx0;
		bx10 = cx1;
	}
}

#include "random_math_double.inl"

void CryptonightR_double_SSE(cryptonight_ctx* ctx0, cryptonight_ctx* ctx1)
{
	uint8_t* l0 = ctx0->long_state;
	uint64_t* h0 = (uint64_t*)ctx0->hash_state;
	uint8_t* l1 = ctx1->long_state;
	uint64_t* h1 = (uint64_t*)ctx1->hash_state;

	uint64_t axl0 = h0[0] ^ h0[4];
	uint64_t axh0 = h0[1] ^ h0[5];
	__m128i bx00 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);
	__m128i bx01 = _mm_set_epi64x(h0[9] ^ h0[11], h0[8] ^ h0[10]);
	uint64_t axl1 = h1[0] ^ h1[4];
	uint64_t axh1 = h1[1] ^ h1[5];
	__m128i bx10 = _mm_set_epi64x(h1[3] ^ h1[7], h1[2] ^ h1[6]);
	__m128i bx11 = _mm_set_epi64x(h1[9] ^ h1[11], h1[8] ^ h1[10]);

	uint64_t idx00 = h0[0] ^ h0[4];
	uint64_t idx10 = h1[0] ^ h1[4];
	uint32_t idx01 = idx00 & 0x1FFFF0;
	uint32_t idx11 = idx10 & 0x1FFFF0;

	v4_reg* data0 = reinterpret_cast<v4_reg*>(h0 + 12);
	v4_reg* data1 = reinterpret_cast<v4_reg*>(h1 + 12);

	__m128i r0 = _mm_set_epi32(0, data1[0], 0, data0[0]);
	__m128i r1 = _mm_set_epi32(0, data1[1], 0, data0[1]);
	__m128i r2 = _mm_set_epi32(0, data1[2], 0, data0[2]);
	__m128i r3 = _mm_set_epi32(0, data1[3], 0, data0[3]);

	for (size_t i = 0; i < 524288; i++)
	{
		__m128i cx0 = _mm_load_si128((__m128i *)&l0[idx01]);
		const __m128i ax0 = _mm_set_epi64x(axh0, axl0);
		cx0 = _mm_aesenc_si128(cx0, ax0);

		{
			uint32_t k = idx01 ^ 0x10;
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l0[k]); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk1, bx00)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk2, ax0)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk3, bx01));
		}

		_mm_store_si128((__m128i *)&l0[idx01], _mm_xor_si128(bx00, cx0));

		idx00 = _mm_cvtsi128_si64(cx0);
		idx01 = idx00 & 0x1FFFF0;

		__m128i cx1 = _mm_load_si128((__m128i *)&l1[idx11]);
		const __m128i ax1 = _mm_set_epi64x(axh1, axl1);
		cx1 = _mm_aesenc_si128(cx1, ax1);

		{
			uint32_t k = idx11 ^ 0x10;
			const __m128i chunk1 = _mm_load_si128((__m128i *)&l1[k]); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk1, bx10)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk2, ax1)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk3, bx11));
		}

		_mm_store_si128((__m128i *)&l1[idx11], _mm_xor_si128(bx10, cx1));

		idx10 = _mm_cvtsi128_si64(cx1);
		idx11 = idx10 & 0x1FFFF0;

		uint64_t hi, lo, cl, ch;
		cl = ((uint64_t*)&l0[idx01])[0];
		ch = ((uint64_t*)&l0[idx01])[1];

		const __m128i random_math_result = _mm_or_si128(_mm_and_si128(_mm_add_epi32(r0, r1), _mm_set_epi64x(0xFFFFFFFFLL, 0xFFFFFFFFLL)), _mm_slli_epi64(_mm_add_epi32(r2, r3), 32));
		cl ^= static_cast<uint64_t>(_mm_cvtsi128_si64(random_math_result));
		{
			const __m128i r4 = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(ax0), _mm_castsi128_ps(ax1)));
			const __m128i r5 = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(ax1), _mm_castsi128_ps(ax0)));
			const __m128i r6 = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(bx00), _mm_castsi128_ps(bx10)));
			const __m128i r7 = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(bx01), _mm_castsi128_ps(bx11)));
			random_math_double(r0, r1, r2, r3, r4, r5, r6, r7);
		}

		lo = _umul128(idx00, cl, &hi);

		{
			uint32_t k = idx01 ^ 0x10;
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l0[k]), _mm_set_epi64x(lo, hi)); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l0[k]);
			hi ^= ((uint64_t*)&l0[k])[0];
			lo ^= ((uint64_t*)&l0[k])[1];
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk1, bx00)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l0[k]);
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk2, ax0)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l0[k], _mm_add_epi64(chunk3, bx01));
		}

		axl0 += hi;
		axh0 += lo;
		((uint64_t*)&l0[idx01])[0] = axl0;
		((uint64_t*)&l0[idx01])[1] = axh0;
		axh0 ^= ch;
		axl0 ^= cl;
		idx00 = axl0;
		idx01 = idx00 & 0x1FFFF0;

		cl = ((uint64_t*)&l1[idx11])[0];
		ch = ((uint64_t*)&l1[idx11])[1];

		cl ^= static_cast<uint64_t>(_mm_cvtsi128_si64(_mm_srli_si128(random_math_result, 8)));

		lo = _umul128(idx10, cl, &hi);

		{
			uint32_t k = idx11 ^ 0x10;
			const __m128i chunk1 = _mm_xor_si128(_mm_load_si128((__m128i *)&l1[k]), _mm_set_epi64x(lo, hi)); k ^= 0x30;
			const __m128i chunk2 = _mm_load_si128((__m128i *)&l1[k]);
			hi ^= ((uint64_t*)&l1[k])[0];
			lo ^= ((uint64_t*)&l1[k])[1];
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk1, bx10)); k ^= 0x10;
			const __m128i chunk3 = _mm_load_si128((__m128i *)&l1[k]);
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk2, ax1)); k ^= 0x20;
			_mm_store_si128((__m128i *)&l1[k], _mm_add_epi64(chunk3, bx11));
		}

		axl1 += hi;
		axh1 += lo;
		((uint64_t*)&l1[idx11])[0] = axl1;
		((uint64_t*)&l1[idx11])[1] = axh1;
		((uint64_t*)&l1[idx11])[1] = axh1;
		axh1 ^= ch;
		axl1 ^= cl;
		idx10 = axl1;
		idx11 = idx10 & 0x1FFFF0;

		bx01 = bx00;
		bx11 = bx10;
		bx00 = cx0;
		bx10 = cx1;
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

    if (!ptr->long_state)
        ptr->long_state = (uint8_t*)VirtualAlloc(NULL, iLargePageMin, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	return ptr;
}

void init_ctx(cryptonight_ctx* ptr, uint64_t seed)
{
	std::mt19937_64 rnd;
	rnd.seed(seed);
	for (int i = 0; i < MEMORY / sizeof(uint64_t); ++i)
	{
		((uint64_t*)ptr->long_state)[i] = rnd();
	}
	for (int i = 0; i < sizeof(ptr->hash_state) / sizeof(uint64_t); ++i)
	{
		((uint64_t*)ptr->hash_state)[i] = rnd();
	}

	ptr->ctx_info[0] = 1;
}

typedef void(*mainloop_func)(cryptonight_ctx*);
typedef void(*mainloop_double_func)(cryptonight_ctx*, cryptonight_ctx*);

extern "C" void CryptonightR_asm(cryptonight_ctx* ctx0);
extern "C" void CryptonightR_double_asm(cryptonight_ctx* ctx0, cryptonight_ctx* ctx1);
extern "C" void cnv2_mainloop_ivybridge_asm(cryptonight_ctx* ctx0);
extern "C" void cnv2_mainloop_ryzen_asm(cryptonight_ctx* ctx0);
extern "C" void cnv2_double_mainloop_sandybridge_asm(cryptonight_ctx* ctx0, cryptonight_ctx* ctx1);
extern int compile_code(const V4_Instruction* code, std::vector<uint8_t>& machine_code);
extern void compile_code_double(const V4_Instruction* code, std::vector<uint8_t>& machine_code);

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
	int64_t min_dt = std::numeric_limits<int64_t>::max();

	for (uint32_t i = 0; i < BENCHMARK_DURATION; ++i)
	{
		const int64_t end_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + 1000000000;

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

		const char progress[] = "|/-\\";
		std::cout << name << ": " << min_dt / (rdtsc_speed * 524288.0) << " ns/iteration\t" << progress[i % (sizeof(progress) - 1)] << '\r';
	}
	std::cout << name << ": " << min_dt / (rdtsc_speed * 524288.0) << " ns/iteration\t\t\t" << std::endl;
}

int CryptonightR_test()
{
	SetThreadAffinityMask(GetCurrentThread(), 1 << 3);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	AddPrivilege(TEXT("SeLockMemoryPrivilege"));

	cryptonight_ctx* ctx[5];
	for (int i = 0; i < 5; ++i)
	{
		ctx[i] = cryptonight_alloc_ctx();
		init_ctx(ctx[i], i % 2);
	}

	V4_Instruction code[1024];

	//int max_code_size = 0;
	//int max_height = 0;
	//uint64_t inst_count[V4_INSTRUCTION_COUNT] = {};
	//int prog_count[128] = {};
	//uint64_t total_inst_count = 0;
	//for (int i = 0; i < 10000000; ++i)
	//{
	//	int code_size = v4_random_math_init(code, i);
	//	++prog_count[code_size];
	//	if (code_size > max_code_size)
	//	{
	//		max_code_size = code_size;
	//		max_height = i;
	//	}
	//	for (int j = 0; code[j].opcode != RET; ++j)
	//	{
	//		++inst_count[code[j].opcode];
	//		++total_inst_count;
	//	}
	//}
	//printf("max code size = %d for height %d\n", max_code_size, max_height);
	//printf("MUL %.2f%%\n", static_cast<double>(inst_count[MUL]) * 100.0 / total_inst_count);
	//printf("ADD %.2f%%\n", static_cast<double>(inst_count[ADD]) * 100.0 / total_inst_count);
	//printf("SUB %.2f%%\n", static_cast<double>(inst_count[SUB]) * 100.0 / total_inst_count);
	//printf("ROR %.2f%%\n", static_cast<double>(inst_count[ROR]) * 100.0 / total_inst_count);
	//printf("ROL %.2f%%\n", static_cast<double>(inst_count[ROL]) * 100.0 / total_inst_count);
	//printf("XOR %.2f%%\n", static_cast<double>(inst_count[XOR]) * 100.0 / total_inst_count);
	//for (int i = 0; i < 128; ++i)
	//{
	//	if (prog_count[i] > 0)
	//	{
	//		printf("%d\t%d\n", i, prog_count[i]);
	//	}
	//}
	//printf("Average length: %.6f\n", total_inst_count / 1e7);

	v4_random_math_init(code, RND_SEED);
	std::vector<uint8_t> machine_code, machine_code_double;
	compile_code(code, machine_code);
    compile_code_double(code, machine_code_double);

	mainloop_func CryptonightR_generated = (mainloop_func) VirtualAlloc(0, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    mainloop_double_func CryptonightR_double_generated = (mainloop_double_func)VirtualAlloc(0, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(CryptonightR_generated, machine_code.data(), machine_code.size());
    memcpy(CryptonightR_double_generated, machine_code_double.data(), machine_code_double.size());

	// Do initial integrity check
	CryptonightR_double_ref(ctx[0], ctx[1], code);

    init_ctx(ctx[2], 0);
    init_ctx(ctx[3], 1);
    CryptonightR_double(ctx[2], ctx[3]);
	if ((memcmp(ctx[0]->long_state, ctx[2]->long_state, MEMORY) != 0) || (memcmp(ctx[1]->long_state, ctx[3]->long_state, MEMORY) != 0))
	{
		std::cerr << "C++ code (double) doesn't match reference code" << std::endl;
		return 2;
	}

	init_ctx(ctx[2], 0);
	init_ctx(ctx[3], 1);
	CryptonightR_double_SSE(ctx[2], ctx[3]);
	if ((memcmp(ctx[0]->long_state, ctx[2]->long_state, MEMORY) != 0) || (memcmp(ctx[1]->long_state, ctx[3]->long_state, MEMORY) != 0))
	{
		std::cerr << "C++ SSE code (double) doesn't match reference code" << std::endl;
		return 2;
	}

	init_ctx(ctx[2], 0);
    init_ctx(ctx[3], 1);
    CryptonightR_double_asm(ctx[2], ctx[3]);
    if ((memcmp(ctx[0]->long_state, ctx[2]->long_state, MEMORY) != 0) || (memcmp(ctx[1]->long_state, ctx[3]->long_state, MEMORY) != 0))
    {
        std::cerr << "ASM code (double) doesn't match reference code" << std::endl;
        return 3;
    }

	init_ctx(ctx[2], 0);
	init_ctx(ctx[3], 1);
	CryptonightR_double_generated(ctx[2], ctx[3]);
	if ((memcmp(ctx[0]->long_state, ctx[2]->long_state, MEMORY) != 0) || (memcmp(ctx[1]->long_state, ctx[3]->long_state, MEMORY) != 0))
	{
		std::cerr << "Generated machine code (double) doesn't match reference code" << std::endl;
		return 4;
	}

    for (int i = 0; i < 5; ++i)
	{
		init_ctx(ctx[i], 5489);
	}
	CryptonightR_ref(ctx[0], code);
    CryptonightR(ctx[2]);
	CryptonightR_asm(ctx[3]);
	CryptonightR_generated(ctx[4]);

    if (memcmp(ctx[0]->long_state, ctx[2]->long_state, MEMORY) != 0)
	{
		std::cerr << "C++ code doesn't match reference code" << std::endl;
		return 6;
	}

	if (memcmp(ctx[0]->long_state, ctx[3]->long_state, MEMORY) != 0)
	{
		std::cerr << "ASM code doesn't match reference code" << std::endl;
		return 7;
	}

	if (memcmp(ctx[0]->long_state, ctx[4]->long_state, MEMORY) != 0)
	{
		std::cerr << "Generated machine code doesn't match reference code" << std::endl;
		return 8;
	}

	// Run benchmarks if the integrity check passed
	std::cout << "rdtsc speed: " << rdtsc_speed << " GHz" << std::endl;
	std::cout << "Running " << BENCHMARK_DURATION << " second benchmarks..." << std::endl;

	benchmark(CryptonightR_double_ref, "CryptonightR_double (reference code)", ctx[0], ctx[1], code);
    benchmark(CryptonightR_double, "CryptonightR_double (C++ code)", ctx[0], ctx[1]);
	benchmark(CryptonightR_double_SSE, "CryptonightR_double (C++ SSE code)", ctx[0], ctx[1]);
	benchmark(CryptonightR_double_asm, "CryptonightR_double (ASM code)", ctx[0], ctx[1]);
    benchmark(CryptonightR_double_generated, "CryptonightR_double (generated machine code)", ctx[0], ctx[1]);
    benchmark(cnv2_double_mainloop_sandybridge_asm, "CryptonightV2_double", ctx[0], ctx[1]);

	std::cout << std::endl;

	benchmark(CryptonightR_ref, "CryptonightR (reference code)", ctx[0], code);
    benchmark(CryptonightR, "CryptonightR (C++ code)", ctx[1]);
	benchmark(CryptonightR_asm, "CryptonightR (ASM code)", ctx[2]);
	benchmark(CryptonightR_generated, "CryptonightR (generated machine code)", ctx[3]);

	// Show CryptonightV2 performance for comparison
	{
		int data[4];
		__cpuidex(data, 0, 0);
		char vendor[32] = {};
		((int*)vendor)[0] = data[1];
		((int*)vendor)[1] = data[3];
		((int*)vendor)[2] = data[2];

		mainloop_func func = (strcmp(vendor, "GenuineIntel") == 0) ? cnv2_mainloop_ivybridge_asm : cnv2_mainloop_ryzen_asm;
		benchmark(func, "CryptonightV2", ctx[1]);
	}

	std::cout << std::endl;

	memcpy(ctx[0]->long_state, ctx[3]->long_state, MEMORY);

	// Test 1000 random code sequences and compare them with reference code
	for (int i = 0; i < 1000; ++i)
	{
		v4_random_math_init(code, i);
		const int num_insts = compile_code(code, machine_code);
        compile_code_double(code, machine_code_double);
		memcpy(CryptonightR_generated, machine_code.data(), machine_code.size());
        memcpy(CryptonightR_double_generated, machine_code_double.data(), machine_code_double.size());
        FlushInstructionCache(GetCurrentProcess(), CryptonightR_generated, machine_code.size());
        FlushInstructionCache(GetCurrentProcess(), CryptonightR_double_generated, machine_code_double.size());

        init_ctx(ctx[0], i);
        init_ctx(ctx[1], i);
        CryptonightR_ref(ctx[0], code);
		CryptonightR_generated(ctx[1]);
		benchmark(CryptonightR_generated, "CryptonightR (generated machine code)", ctx[3]);

		if (memcmp(ctx[0]->long_state, ctx[1]->long_state, MEMORY) != 0)
		{
			std::cerr << "Generated machine code doesn't match reference code" << std::endl;
			return 7;
		}

        init_ctx(ctx[0], i * 2);
        init_ctx(ctx[1], i * 2 + 1);
        init_ctx(ctx[2], i * 2);
        init_ctx(ctx[3], i * 2 + 1);
        CryptonightR_double_ref(ctx[0], ctx[1], code);
        CryptonightR_double_generated(ctx[2], ctx[3]);

        if ((memcmp(ctx[0]->long_state, ctx[2]->long_state, MEMORY) != 0) || (memcmp(ctx[1]->long_state, ctx[3]->long_state, MEMORY) != 0))
        {
            std::cerr << "Generated machine code doesn't match reference code" << std::endl;
            return 7;
        }

        std::cout << "Random code test " << i << " (" << num_insts << " instructions) passed\n\n";
	}

	return 0;
}
