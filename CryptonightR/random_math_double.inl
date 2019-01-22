// Auto-generated file, do not edit

FORCEINLINE void random_math_double(__m128i& r0, __m128i& r1, __m128i& r2, __m128i& r3, const __m128i r4, const __m128i r5, const __m128i r6, const __m128i r7)
{
	r3 = _mm_xor_si128(r3, r4);	
	r0 = _mm_mul_epu32(r0, r3);	
	r0 = _mm_add_epi32(_mm_add_epi32(r0, r4), _mm_shuffle_epi32(_mm_cvtsi32_si128(598137265), _MM_SHUFFLE(1, 0, 1, 0)));	
	r3 = _mm_sub_epi32(r3, r7);	
	r0 = _mm_mul_epu32(r0, r3);	
	r3 = _mm_xor_si128(r3, r1);	
	r0 = _mm_add_epi32(_mm_add_epi32(r0, r2), _mm_shuffle_epi32(_mm_cvtsi32_si128(1743933451), _MM_SHUFFLE(1, 0, 1, 0)));	
	r1 = _mm_add_epi32(_mm_add_epi32(r1, r6), _mm_shuffle_epi32(_mm_cvtsi32_si128(584808783), _MM_SHUFFLE(1, 0, 1, 0)));	
	r0 = _mm_mul_epu32(r0, r2);	
	r2 = _mm_sub_epi32(r2, r4);	
	r1 = _mm_xor_si128(r1, r2);	
	r3 = _mm_mul_epu32(r3, r2);	
	r1 = _mm_xor_si128(r1, r4);	
	r2 = _mm_mul_epu32(r2, r3);	
	r3 = _mm_mul_epu32(r3, r5);	
	r2 = _mm_mul_epu32(r2, r5);	
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r3), _mm_extract_epi32(r3, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r1), _mm_extract_epi32(r1, 2) };
		r1 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotl(d[0], c[0])), _rotl(d[1], c[1]), 2);
	}
	r2 = _mm_mul_epu32(r2, r0);	
	r1 = _mm_sub_epi32(r1, r3);	
	r1 = _mm_add_epi32(_mm_add_epi32(r1, r4), _mm_shuffle_epi32(_mm_cvtsi32_si128(-1113025137), _MM_SHUFFLE(1, 0, 1, 0)));	
	r2 = _mm_sub_epi32(r2, r6);	
	r2 = _mm_xor_si128(r2, r6);	
	r0 = _mm_sub_epi32(r0, r2);	
	r2 = _mm_add_epi32(_mm_add_epi32(r2, r3), _mm_shuffle_epi32(_mm_cvtsi32_si128(1308674353), _MM_SHUFFLE(1, 0, 1, 0)));	
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r2), _mm_extract_epi32(r2, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r2), _mm_extract_epi32(r2, 2) };
		r2 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotr(d[0], c[0])), _rotr(d[1], c[1]), 2);
	}
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r1), _mm_extract_epi32(r1, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r0), _mm_extract_epi32(r0, 2) };
		r0 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotr(d[0], c[0])), _rotr(d[1], c[1]), 2);
	}
	r1 = _mm_sub_epi32(r1, r7);	
	r0 = _mm_xor_si128(r0, r5);	
	r3 = _mm_mul_epu32(r3, r6);	
	r1 = _mm_mul_epu32(r1, r0);	
	r1 = _mm_add_epi32(_mm_add_epi32(r1, r0), _mm_shuffle_epi32(_mm_cvtsi32_si128(-1642698742), _MM_SHUFFLE(1, 0, 1, 0)));	
	r0 = _mm_mul_epu32(r0, r2);	
	r1 = _mm_mul_epu32(r1, r1);	
	r0 = _mm_mul_epu32(r0, r2);	
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r3), _mm_extract_epi32(r3, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r0), _mm_extract_epi32(r0, 2) };
		r0 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotl(d[0], c[0])), _rotl(d[1], c[1]), 2);
	}
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r1), _mm_extract_epi32(r1, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r1), _mm_extract_epi32(r1, 2) };
		r1 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotr(d[0], c[0])), _rotr(d[1], c[1]), 2);
	}
	r0 = _mm_sub_epi32(r0, r4);	
	r2 = _mm_add_epi32(_mm_add_epi32(r2, r5), _mm_shuffle_epi32(_mm_cvtsi32_si128(-222514421), _MM_SHUFFLE(1, 0, 1, 0)));	
	r0 = _mm_add_epi32(_mm_add_epi32(r0, r4), _mm_shuffle_epi32(_mm_cvtsi32_si128(508711903), _MM_SHUFFLE(1, 0, 1, 0)));	
	r2 = _mm_mul_epu32(r2, r2);	
	r0 = _mm_mul_epu32(r0, r0);	
	r2 = _mm_mul_epu32(r2, r4);	
	r2 = _mm_xor_si128(r2, r0);	
	r1 = _mm_add_epi32(_mm_add_epi32(r1, r5), _mm_shuffle_epi32(_mm_cvtsi32_si128(1366708954), _MM_SHUFFLE(1, 0, 1, 0)));	
	r1 = _mm_mul_epu32(r1, r4);	
	r2 = _mm_mul_epu32(r2, r1);	
	r2 = _mm_mul_epu32(r2, r3);	
	r0 = _mm_mul_epu32(r0, r3);	
	r3 = _mm_mul_epu32(r3, r7);	
	r3 = _mm_mul_epu32(r3, r6);	
	r3 = _mm_mul_epu32(r3, r4);	
	r1 = _mm_sub_epi32(r1, r5);	
	r0 = _mm_xor_si128(r0, r5);	
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r6), _mm_extract_epi32(r6, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r0), _mm_extract_epi32(r0, 2) };
		r0 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotl(d[0], c[0])), _rotl(d[1], c[1]), 2);
	}
	r1 = _mm_mul_epu32(r1, r5);	
	r3 = _mm_xor_si128(r3, r7);	
	r0 = _mm_xor_si128(r0, r4);	
	r3 = _mm_add_epi32(_mm_add_epi32(r3, r7), _mm_shuffle_epi32(_mm_cvtsi32_si128(-89728199), _MM_SHUFFLE(1, 0, 1, 0)));	
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r4), _mm_extract_epi32(r4, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r3), _mm_extract_epi32(r3, 2) };
		r3 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotl(d[0], c[0])), _rotl(d[1], c[1]), 2);
	}
	r1 = _mm_add_epi32(_mm_add_epi32(r1, r5), _mm_shuffle_epi32(_mm_cvtsi32_si128(1124980062), _MM_SHUFFLE(1, 0, 1, 0)));	
	{
		const uint32_t c[2] = { _mm_cvtsi128_si32(r2), _mm_extract_epi32(r2, 2) };
		const uint32_t d[2] = { _mm_cvtsi128_si32(r3), _mm_extract_epi32(r3, 2) };
		r3 = _mm_insert_epi32(_mm_cvtsi32_si128(_rotr(d[0], c[0])), _rotr(d[1], c[1]), 2);
	}
	r1 = _mm_mul_epu32(r1, r3);	
	r0 = _mm_mul_epu32(r0, r1);	
}
