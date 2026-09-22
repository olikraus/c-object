#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Global variable to store the detected SIMD width: 8 (uint64), 16 (m128), 32 (m256), 64 (m512) */
/* The value is the size of the base element in bytes */
int co_bv_base_size = 8; 

/* Function pointer types */
typedef void (*co_bv_set_fn)(co_BVType bv, uint64_t bit_idx);
typedef void (*co_bv_clr_fn)(co_BVType bv, uint64_t bit_idx);
typedef void (*co_bv_clear_all_fn)(co_BVType bv);
typedef int (*co_bv_get_fn)(co_BVType bv, uint64_t bit_idx);
typedef void (*co_bv_op3_fn)(co_BVType res, co_BVType a, co_BVType b);
typedef int (*co_bv_is_equal_fn)(co_BVType a, co_BVType b);
typedef int (*co_bv_is_subset_fn)(co_BVType a, co_BVType b);
typedef int (*co_bv_super_sub_test_fn)(co_BVType a, co_BVType b);
typedef int (*co_bv_is_disjoint_fn)(co_BVType a, co_BVType b);
typedef int (*co_bv_and_tst_zero_fn)(co_BVType res, co_BVType a, co_BVType b);
typedef int (*co_bv_and3_tst_nonzero_fn)(co_BVType a, co_BVType b, co_BVType c);

/* Implementations forward declarations */
void co_bv_set_u64(co_BVType bv, uint64_t bit_idx);
void co_bv_clr_u64(co_BVType bv, uint64_t bit_idx);
void co_bv_clear_all_u64(co_BVType bv);
int co_bv_get_u64(co_BVType bv, uint64_t bit_idx);
void co_bv_or_u64(co_BVType res, co_BVType a, co_BVType b);
void co_bv_and_u64(co_BVType res, co_BVType a, co_BVType b);
void co_bv_xor_u64(co_BVType res, co_BVType a, co_BVType b);
void co_bv_andnot_u64(co_BVType res, co_BVType a, co_BVType b);
int co_bv_is_equal_u64(co_BVType a, co_BVType b);
int co_bv_is_subset_u64(co_BVType a, co_BVType b);
int co_bv_super_sub_test_u64(co_BVType a, co_BVType b);
int co_bv_is_disjoint_u64(co_BVType a, co_BVType b);
int co_bv_and_tst_zero_u64(co_BVType res, co_BVType a, co_BVType b);
int co_bv_and3_tst_nonzero_u64(co_BVType a, co_BVType b, co_BVType c);

/* Global function pointers, default to u64 scalar versions */
static co_bv_set_fn co_bv_set_ptr = co_bv_set_u64;
static co_bv_clr_fn co_bv_clr_ptr = co_bv_clr_u64;
static co_bv_clear_all_fn co_bv_clear_all_ptr = co_bv_clear_all_u64;
static co_bv_get_fn co_bv_get_ptr = co_bv_get_u64;
static co_bv_op3_fn co_bv_or_ptr = co_bv_or_u64;
static co_bv_op3_fn co_bv_and_ptr = co_bv_and_u64;
static co_bv_op3_fn co_bv_xor_ptr = co_bv_xor_u64;
static co_bv_op3_fn co_bv_andnot_ptr = co_bv_andnot_u64;
static co_bv_is_equal_fn co_bv_is_equal_ptr = co_bv_is_equal_u64;
static co_bv_is_subset_fn co_bv_is_subset_ptr = co_bv_is_subset_u64;
static co_bv_super_sub_test_fn co_bv_super_sub_test_ptr = co_bv_super_sub_test_u64;
static co_bv_is_disjoint_fn co_bv_is_disjoint_ptr = co_bv_is_disjoint_u64;
static co_bv_and_tst_zero_fn co_bv_and_tst_zero_ptr = co_bv_and_tst_zero_u64;
static co_bv_and3_tst_nonzero_fn co_bv_and3_tst_nonzero_ptr = co_bv_and3_tst_nonzero_u64;

/* coBitVectorType functions */
static int coBVInit(co o, void *data) { return 1; }
static long coBVSize(cco o) { return ((co_BVType)o)->cnt * co_bv_base_size; }
static void coBVPrint(const cco o) { printf("BitVector(%d elements)\n", ((co_BVType)o)->cnt); }
static void coBVDestroy(co o) {
    co_BVType bv = (co_BVType)o;
    if (bv->data.u64) free(bv->data.u64);
    bv->data.u64 = NULL;
}
static co coBVClone(cco o) {
    co_BVType src = (co_BVType)o;
    co_BVType dest = coNewBV(src->cnt * co_bv_base_size * 8);
    if (dest) {
        memcpy(dest->data.u64, src->data.u64, src->cnt * co_bv_base_size);
    }
    return (co)dest;
}

struct coFnStruct coBitVectorStruct = {coBVInit, coBVSize, coBVPrint, coBVDestroy, coBVClone};
coFn coBitVectorType = &coBitVectorStruct;

/* Implementations */

// uint64
void co_bv_set_u64(co_BVType bv, uint64_t bit_idx) {
    bv->data.u64[bit_idx >> 6] |= (1ULL << (bit_idx & 0x3f));
}
void co_bv_clr_u64(co_BVType bv, uint64_t bit_idx) {
    bv->data.u64[bit_idx >> 6] &= ~(1ULL << (bit_idx & 0x3f));
}
void co_bv_clear_all_u64(co_BVType bv) {
    for (int i = 0; i < bv->cnt; i++) bv->data.u64[i] = 0;
}
int co_bv_get_u64(co_BVType bv, uint64_t bit_idx) {
    return (bv->data.u64[bit_idx >> 6] >> (bit_idx & 0x3f)) & 1;
}
void co_bv_or_u64(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.u64[i] = a->data.u64[i] | b->data.u64[i];
}
void co_bv_and_u64(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.u64[i] = a->data.u64[i] & b->data.u64[i];
}
void co_bv_xor_u64(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.u64[i] = a->data.u64[i] ^ b->data.u64[i];
}
void co_bv_andnot_u64(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.u64[i] = a->data.u64[i] & ~b->data.u64[i];
}
int co_bv_is_equal_u64(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.u64, b->data.u64, (size_t)a->cnt * co_bv_base_size) == 0;
}
int co_bv_is_subset_u64(co_BVType a, co_BVType b) {
    for (int i = 0; i < a->cnt; i++) {
        if (a->data.u64[i] & ~b->data.u64[i]) return 0;
    }
    return 1;
}
int co_bv_super_sub_test_u64(co_BVType a, co_BVType b) {
    int res = 3;
    for (int i = 0; i < a->cnt; i++) {
        if (a->data.u64[i] & ~b->data.u64[i]) res &= ~1;
        if (b->data.u64[i] & ~a->data.u64[i]) res &= ~2;
        if (res == 0) return 0;
    }
    return res;
}
int co_bv_is_disjoint_u64(co_BVType a, co_BVType b) {
    for (int i = 0; i < a->cnt; i++) {
        if (a->data.u64[i] & b->data.u64[i]) return 0;
    }
    return 1;
}
/* 
   Computes bitwise res = a & b. 
   Returns 1 if at least one bit is set in the result, 0 if result is all zeros.
*/
int co_bv_and_tst_zero_u64(co_BVType res, co_BVType a, co_BVType b) {
    uint64_t nz = 0;
    for (int i = 0; i < res->cnt; i++) {
        res->data.u64[i] = a->data.u64[i] & b->data.u64[i];
        nz |= res->data.u64[i];
    }
    return nz ? 1 : 0;
}
int co_bv_and3_tst_nonzero_u64(co_BVType a, co_BVType b, co_BVType c) {
    for (int i = 0; i < a->cnt; i++) {
        if (a->data.u64[i] & b->data.u64[i] & c->data.u64[i]) return 1;
    }
    return 0;
}

// m128
__attribute__((target("sse2")))
void co_bv_set_m128(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m128;
    p[bit_idx >> 6] |= (1ULL << (bit_idx & 0x3f));
}
__attribute__((target("sse2")))
void co_bv_clr_m128(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m128;
    p[bit_idx >> 6] &= ~(1ULL << (bit_idx & 0x3f));
}
__attribute__((target("sse2")))
void co_bv_clear_all_m128(co_BVType bv) {
    __m128i zero = _mm_setzero_si128();
    for (int i = 0; i < bv->cnt; i++) bv->data.m128[i] = zero;
}
__attribute__((target("sse2")))
int co_bv_get_m128(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m128;
    return (p[bit_idx >> 6] >> (bit_idx & 0x3f)) & 1;
}
__attribute__((target("sse2")))
void co_bv_or_m128(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m128[i] = _mm_or_si128(a->data.m128[i], b->data.m128[i]);
}
__attribute__((target("sse2")))
void co_bv_and_m128(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m128[i] = _mm_and_si128(a->data.m128[i], b->data.m128[i]);
}
__attribute__((target("sse2")))
void co_bv_xor_m128(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m128[i] = _mm_xor_si128(a->data.m128[i], b->data.m128[i]);
}
__attribute__((target("sse2")))
void co_bv_andnot_m128(co_BVType res, co_BVType a, co_BVType b) {
    /* Note: _mm_andnot_si128(b, a) computes (~b & a) */
    for (int i = 0; i < res->cnt; i++) res->data.m128[i] = _mm_andnot_si128(b->data.m128[i], a->data.m128[i]);
}
__attribute__((target("sse2")))
int co_bv_is_equal_m128(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.m128, b->data.m128, (size_t)a->cnt * co_bv_base_size) == 0;
}
__attribute__((target("sse2")))
int co_bv_is_subset_m128(co_BVType a, co_BVType b) {
    __m128i zero = _mm_setzero_si128();
    for (int i = 0; i < a->cnt; i++) {
        __m128i v = _mm_andnot_si128(b->data.m128[i], a->data.m128[i]);
        if (_mm_movemask_epi8(_mm_cmpeq_epi8(v, zero)) != 0xffff) return 0;
    }
    return 1;
}
__attribute__((target("sse2")))
int co_bv_super_sub_test_m128(co_BVType a, co_BVType b) {
    __m128i zero = _mm_setzero_si128();
    int res = 3;
    for (int i = 0; i < a->cnt; i++) {
        if (res & 1) {
            __m128i v = _mm_andnot_si128(b->data.m128[i], a->data.m128[i]);
            if (_mm_movemask_epi8(_mm_cmpeq_epi8(v, zero)) != 0xffff) res &= ~1;
        }
        if (res & 2) {
            __m128i v = _mm_andnot_si128(a->data.m128[i], b->data.m128[i]);
            if (_mm_movemask_epi8(_mm_cmpeq_epi8(v, zero)) != 0xffff) res &= ~2;
        }
        if (res == 0) return 0;
    }
    return res;
}
__attribute__((target("sse2")))
int co_bv_is_disjoint_m128(co_BVType a, co_BVType b) {
    __m128i zero = _mm_setzero_si128();
    for (int i = 0; i < a->cnt; i++) {
        __m128i v = _mm_and_si128(a->data.m128[i], b->data.m128[i]);
        if (_mm_movemask_epi8(_mm_cmpeq_epi8(v, zero)) != 0xffff) return 0;
    }
    return 1;
}
/* 
   Computes bitwise res = a & b. 
   Returns 1 if at least one bit is set in the result, 0 if result is all zeros.
*/
__attribute__((target("sse2")))
int co_bv_and_tst_zero_m128(co_BVType res, co_BVType a, co_BVType b) {
    __m128i zero = _mm_setzero_si128();
    int nz = 0;
    for (int i = 0; i < res->cnt; i++) {
        res->data.m128[i] = _mm_and_si128(a->data.m128[i], b->data.m128[i]);
        if (_mm_movemask_epi8(_mm_cmpeq_epi8(res->data.m128[i], zero)) != 0xffff) nz = 1;
    }
    return nz;
}
__attribute__((target("sse2")))
int co_bv_and3_tst_nonzero_m128(co_BVType a, co_BVType b, co_BVType c) {
    __m128i zero = _mm_setzero_si128();
    for (int i = 0; i < a->cnt; i++) {
        __m128i v = _mm_and_si128(a->data.m128[i], _mm_and_si128(b->data.m128[i], c->data.m128[i]));
        if (_mm_movemask_epi8(_mm_cmpeq_epi8(v, zero)) != 0xffff) return 1;
    }
    return 0;
}

// m256
__attribute__((target("avx2")))
void co_bv_set_m256(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m256;
    p[bit_idx >> 6] |= (1ULL << (bit_idx & 0x3f));
}
__attribute__((target("avx2")))
void co_bv_clr_m256(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m256;
    p[bit_idx >> 6] &= ~(1ULL << (bit_idx & 0x3f));
}
__attribute__((target("avx2")))
void co_bv_clear_all_m256(co_BVType bv) {
    __m256i zero = _mm256_setzero_si256();
    for (int i = 0; i < bv->cnt; i++) bv->data.m256[i] = zero;
}
__attribute__((target("avx2")))
int co_bv_get_m256(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m256;
    return (p[bit_idx >> 6] >> (bit_idx & 0x3f)) & 1;
}
__attribute__((target("avx2")))
void co_bv_or_m256(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m256[i] = _mm256_or_si256(a->data.m256[i], b->data.m256[i]);
}
__attribute__((target("avx2")))
void co_bv_and_m256(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m256[i] = _mm256_and_si256(a->data.m256[i], b->data.m256[i]);
}
__attribute__((target("avx2")))
void co_bv_xor_m256(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m256[i] = _mm256_xor_si256(a->data.m256[i], b->data.m256[i]);
}
__attribute__((target("avx2")))
void co_bv_andnot_m256(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m256[i] = _mm256_andnot_si256(b->data.m256[i], a->data.m256[i]);
}
__attribute__((target("avx2")))
int co_bv_is_equal_m256(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.m256, b->data.m256, (size_t)a->cnt * co_bv_base_size) == 0;
}
__attribute__((target("avx2")))
int co_bv_is_subset_m256(co_BVType a, co_BVType b) {
    for (int i = 0; i < a->cnt; i++) {
        if (!_mm256_testz_si256(a->data.m256[i], _mm256_andnot_si256(b->data.m256[i], a->data.m256[i]))) return 0;
    }
    return 1;
}
__attribute__((target("avx2")))
int co_bv_super_sub_test_m256(co_BVType a, co_BVType b) {
    int res = 3;
    for (int i = 0; i < a->cnt; i++) {
        if (res & 1) {
            if (!_mm256_testz_si256(a->data.m256[i], _mm256_andnot_si256(b->data.m256[i], a->data.m256[i]))) res &= ~1;
        }
        if (res & 2) {
            if (!_mm256_testz_si256(b->data.m256[i], _mm256_andnot_si256(a->data.m256[i], b->data.m256[i]))) res &= ~2;
        }
        if (res == 0) return 0;
    }
    return res;
}
__attribute__((target("avx2")))
int co_bv_is_disjoint_m256(co_BVType a, co_BVType b) {
    for (int i = 0; i < a->cnt; i++) {
        if (!_mm256_testz_si256(a->data.m256[i], b->data.m256[i])) return 0;
    }
    return 1;
}
/* 
   Computes bitwise res = a & b. 
   Returns 1 if at least one bit is set in the result, 0 if result is all zeros.
*/
__attribute__((target("avx2")))
int co_bv_and_tst_zero_m256(co_BVType res, co_BVType a, co_BVType b) {
    int nz = 0;
    for (int i = 0; i < res->cnt; i++) {
        res->data.m256[i] = _mm256_and_si256(a->data.m256[i], b->data.m256[i]);
        if (!_mm256_testz_si256(res->data.m256[i], res->data.m256[i])) nz = 1;
    }
    return nz;
}
__attribute__((target("avx2")))
int co_bv_and3_tst_nonzero_m256(co_BVType a, co_BVType b, co_BVType c) {
    for (int i = 0; i < a->cnt; i++) {
        __m256i v = _mm256_and_si256(a->data.m256[i], _mm256_and_si256(b->data.m256[i], c->data.m256[i]));
        if (!_mm256_testz_si256(v, v)) return 1;
    }
    return 0;
}

// m512
__attribute__((target("avx512f")))
void co_bv_set_m512(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m512;
    p[bit_idx >> 6] |= (1ULL << (bit_idx & 0x3f));
}
__attribute__((target("avx512f")))
void co_bv_clr_m512(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m512;
    p[bit_idx >> 6] &= ~(1ULL << (bit_idx & 0x3f));
}
__attribute__((target("avx512f")))
void co_bv_clear_all_m512(co_BVType bv) {
    __m512i zero = _mm512_setzero_si512();
    for (int i = 0; i < bv->cnt; i++) bv->data.m512[i] = zero;
}
__attribute__((target("avx512f")))
int co_bv_get_m512(co_BVType bv, uint64_t bit_idx) {
    uint64_t *p = (uint64_t *)bv->data.m512;
    return (p[bit_idx >> 6] >> (bit_idx & 0x3f)) & 1;
}
__attribute__((target("avx512f")))
void co_bv_or_m512(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m512[i] = _mm512_or_si512(a->data.m512[i], b->data.m512[i]);
}
__attribute__((target("avx512f")))
void co_bv_and_m512(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m512[i] = _mm512_and_si512(a->data.m512[i], b->data.m512[i]);
}
__attribute__((target("avx512f")))
void co_bv_xor_m512(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m512[i] = _mm512_xor_si512(a->data.m512[i], b->data.m512[i]);
}
__attribute__((target("avx512f")))
void co_bv_andnot_m512(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m512[i] = _mm512_andnot_si512(b->data.m512[i], a->data.m512[i]);
}
__attribute__((target("avx512f")))
int co_bv_is_equal_m512(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.m512, b->data.m512, (size_t)a->cnt * co_bv_base_size) == 0;
}
__attribute__((target("avx512f")))
int co_bv_is_subset_m512(co_BVType a, co_BVType b) {
    for (int i = 0; i < a->cnt; i++) {
        if (_mm512_test_epi64_mask(a->data.m512[i], _mm512_andnot_si512(b->data.m512[i], a->data.m512[i])) != 0) return 0;
    }
    return 1;
}
__attribute__((target("avx512f")))
int co_bv_super_sub_test_m512(co_BVType a, co_BVType b) {
    int res = 3;
    for (int i = 0; i < a->cnt; i++) {
        if (res & 1) {
            if (_mm512_test_epi64_mask(a->data.m512[i], _mm512_andnot_si512(b->data.m512[i], a->data.m512[i])) != 0) res &= ~1;
        }
        if (res & 2) {
            if (_mm512_test_epi64_mask(b->data.m512[i], _mm512_andnot_si512(a->data.m512[i], b->data.m512[i])) != 0) res &= ~2;
        }
        if (res == 0) return 0;
    }
    return res;
}
__attribute__((target("avx512f")))
int co_bv_is_disjoint_m512(co_BVType a, co_BVType b) {
    for (int i = 0; i < a->cnt; i++) {
        if (_mm512_test_epi64_mask(a->data.m512[i], b->data.m512[i]) != 0) return 0;
    }
    return 1;
}
/* 
   Computes bitwise res = a & b. 
   Returns 1 if at least one bit is set in the result, 0 if result is all zeros.
*/
__attribute__((target("avx512f")))
int co_bv_and_tst_zero_m512(co_BVType res, co_BVType a, co_BVType b) {
    int nz = 0;
    for (int i = 0; i < res->cnt; i++) {
        res->data.m512[i] = _mm512_and_si512(a->data.m512[i], b->data.m512[i]);
        if (_mm512_test_epi64_mask(res->data.m512[i], res->data.m512[i]) != 0) nz = 1;
    }
    return nz;
}
__attribute__((target("avx512f")))
int co_bv_and3_tst_nonzero_m512(co_BVType a, co_BVType b, co_BVType c) {
    for (int i = 0; i < a->cnt; i++) {
        __m512i v = _mm512_and_si512(a->data.m512[i], _mm512_and_si512(b->data.m512[i], c->data.m512[i]));
        if (_mm512_test_epi64_mask(v, v) != 0) return 1;
    }
    return 0;
}

void coBVDetect(void) {
    // Default (already set by static initialization, but re-assert here)
    co_bv_base_size = 8;
    co_bv_set_ptr = co_bv_set_u64;
    co_bv_clr_ptr = co_bv_clr_u64;
    co_bv_clear_all_ptr = co_bv_clear_all_u64;
    co_bv_get_ptr = co_bv_get_u64;
    co_bv_or_ptr = co_bv_or_u64;
    co_bv_and_ptr = co_bv_and_u64;
    co_bv_xor_ptr = co_bv_xor_u64;
    co_bv_andnot_ptr = co_bv_andnot_u64;
    co_bv_is_equal_ptr = co_bv_is_equal_u64;
    co_bv_is_subset_ptr = co_bv_is_subset_u64;
    co_bv_super_sub_test_ptr = co_bv_super_sub_test_u64;
    co_bv_and_tst_zero_ptr = co_bv_and_tst_zero_u64;

    if (__builtin_cpu_supports("avx512f")) {
        co_bv_base_size = 64;
        co_bv_set_ptr = co_bv_set_m512;
        co_bv_clr_ptr = co_bv_clr_m512;
        co_bv_clear_all_ptr = co_bv_clear_all_m512;
        co_bv_get_ptr = co_bv_get_m512;
        co_bv_or_ptr = co_bv_or_m512;
        co_bv_and_ptr = co_bv_and_m512;
        co_bv_xor_ptr = co_bv_xor_m512;
        co_bv_andnot_ptr = co_bv_andnot_m512;
        co_bv_is_equal_ptr = co_bv_is_equal_m512;
        co_bv_is_subset_ptr = co_bv_is_subset_m512;
        co_bv_super_sub_test_ptr = co_bv_super_sub_test_m512;
        co_bv_is_disjoint_ptr = co_bv_is_disjoint_m512;
        co_bv_and_tst_zero_ptr = co_bv_and_tst_zero_m512;
        co_bv_and3_tst_nonzero_ptr = co_bv_and3_tst_nonzero_m512;
    } else if (__builtin_cpu_supports("avx2")) {
        co_bv_base_size = 32;
        co_bv_set_ptr = co_bv_set_m256;
        co_bv_clr_ptr = co_bv_clr_m256;
        co_bv_clear_all_ptr = co_bv_clear_all_m256;
        co_bv_get_ptr = co_bv_get_m256;
        co_bv_or_ptr = co_bv_or_m256;
        co_bv_and_ptr = co_bv_and_m256;
        co_bv_xor_ptr = co_bv_xor_m256;
        co_bv_andnot_ptr = co_bv_andnot_m256;
        co_bv_is_equal_ptr = co_bv_is_equal_m256;
        co_bv_is_subset_ptr = co_bv_is_subset_m256;
        co_bv_super_sub_test_ptr = co_bv_super_sub_test_m256;
        co_bv_is_disjoint_ptr = co_bv_is_disjoint_m256;
        co_bv_and_tst_zero_ptr = co_bv_and_tst_zero_m256;
        co_bv_and3_tst_nonzero_ptr = co_bv_and3_tst_nonzero_m256;
    } else if (__builtin_cpu_supports("sse2")) {
        co_bv_base_size = 16;
        co_bv_set_ptr = co_bv_set_m128;
        co_bv_clr_ptr = co_bv_clr_m128;
        co_bv_clear_all_ptr = co_bv_clear_all_m128;
        co_bv_get_ptr = co_bv_get_m128;
        co_bv_or_ptr = co_bv_or_m128;
        co_bv_and_ptr = co_bv_and_m128;
        co_bv_xor_ptr = co_bv_xor_m128;
        co_bv_andnot_ptr = co_bv_andnot_m128;
        co_bv_is_equal_ptr = co_bv_is_equal_m128;
        co_bv_is_subset_ptr = co_bv_is_subset_m128;
        co_bv_super_sub_test_ptr = co_bv_super_sub_test_m128;
        co_bv_is_disjoint_ptr = co_bv_is_disjoint_m128;
        co_bv_and_tst_zero_ptr = co_bv_and_tst_zero_m128;
        co_bv_and3_tst_nonzero_ptr = co_bv_and3_tst_nonzero_m128;
    }
}

co_BVType coNewBV(uint64_t bits) {
    co_BVType bv = malloc(sizeof(struct coBVStruct));
    if (bv == NULL) return NULL;
    
    bv->fn = coBitVectorType;
    bv->flags = CO_FREE_VALS;

    int bits_per_element = co_bv_base_size * 8;
    bv->cnt = (int)((bits + bits_per_element - 1) / bits_per_element);
    if (bv->cnt == 0 && bits > 0) bv->cnt = 1;
    
    void *mem = NULL;
    /* Align to 64 bytes for AVX-512 */
    if (posix_memalign(&mem, 64, (size_t)bv->cnt * co_bv_base_size) != 0) {
        free(bv);
        return NULL;
    }
    bv->data.u64 = mem;
    coBVClearAll(bv);
    return bv;
}

void coDeleteBV(co_BVType bv) {
    coDelete((co)bv);
}

void coBVPreparePSD(co psd) {
  if (psd == NULL)
    return;

  assert(coIsMap(psd));

  co psd_inner = (co)coMapGet(psd, "psd");
  if (psd_inner == NULL || !coIsMap(psd_inner))
    return;

  /* Force CO_FREE_VALS for the root PSD map to ensure members are deleted/replaced correctly */
  psd->flags |= CO_FREE_VALS;

  /* Create or clear bvattributes map */
  co bvattributes = (co)coMapGet(psd, "bvattributes");
  if (bvattributes == NULL) {
    bvattributes = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
    coMapAdd(psd, "bvattributes", bvattributes);
    bvattributes = (co)coMapGet(psd, "bvattributes");
  } else {
    coMapClear(bvattributes);
  }

  /* Create or clear bvpos vector */
  co bvpos = (co)coMapGet(psd, "bvpos");
  if (bvpos == NULL) {
    bvpos = coNewInt32Vector(CO_NONE);
    coMapAdd(psd, "bvpos", bvpos);
    bvpos = (co)coMapGet(psd, "bvpos");
  } else {
    coInt32VectorClear(bvpos);
  }

  /* Create or clear bvvaluepos map */
  co bvvaluepos = (co)coMapGet(psd, "bvvaluepos");
  if (bvvaluepos == NULL) {
    bvvaluepos = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
    coMapAdd(psd, "bvvaluepos", bvvaluepos);
    bvvaluepos = (co)coMapGet(psd, "bvvaluepos");
  } else {
    coMapClear(bvvaluepos);
  }

  /* Create or clear bvmask vector */
  co bvmask = (co)coMapGet(psd, "bvmask");
  if (bvmask == NULL) {
    bvmask = coNewVector(CO_FREE_VALS);
    coMapAdd(psd, "bvmask", bvmask);
    bvmask = (co)coMapGet(psd, "bvmask");
  } else {
    coVectorClear(bvmask);
  }

  int32_t current_bit_offset = 0;
  int32_t attr_idx = 0;

  coInt32VectorAdd(bvpos, current_bit_offset);

  coMapIterator iter;
  if (coMapLoopFirst(&iter, psd_inner)) {
    do {
      const char *attr_name = coMapLoopKey(&iter);
      cco psd_vec = coMapLoopValue(&iter);

      if (psd_vec != NULL && coIsInt32Vector(psd_vec)) {
        int32_t num_values = (int32_t)coInt32VectorSize(psd_vec);

        co meta = coNewInt32Vector(CO_NONE);
        coInt32VectorAdd(meta, attr_idx);
        coInt32VectorAdd(meta, current_bit_offset);
        coInt32VectorAdd(meta, num_values);

        coMapAdd(bvattributes, attr_name, meta);

        /* Create value to bit map */
        co val_map = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
        coMapAdd(bvvaluepos, attr_name, val_map);
        for (int32_t i = 0; i < num_values; i++) {
            char buf[32];
            sprintf(buf, "%d", coInt32VectorGet(psd_vec, i));
            coMapAdd(val_map, buf, coNewDbl(i));
        }

        current_bit_offset += num_values;
        coInt32VectorAdd(bvpos, current_bit_offset);
        attr_idx++;
      }
    } while (coMapLoopNext(&iter));
  }

  /* Generate masks - needs total bits from the last entry of bvpos */
  uint64_t total_bits = coInt32VectorGet(bvpos, coInt32VectorSize(bvpos) - 1);
  if (coMapLoopFirst(&iter, bvattributes)) {
      do {
          cco meta = coMapLoopValue(&iter);
          int32_t start_bit = coInt32VectorGet(meta, 1);
          int32_t num_values = coInt32VectorGet(meta, 2);

          co_BVType mask = coNewBV(total_bits);
          for (int32_t i = 0; i < num_values; i++) {
              coBVSet(mask, start_bit + i);
          }
          coVectorAdd(bvmask, (cco)mask);
      } while (coMapLoopNext(&iter));
  }
}

co_BVType coNewBVFromANDTerm(cco psd, cco and_term) {
    co bvpos = (co)coMapGet(psd, "bvpos");
    if (!bvpos) return NULL;
    uint64_t total_bits = coInt32VectorGet(bvpos, coInt32VectorSize(bvpos) - 1);
    co_BVType bv = coNewBV(total_bits);
    if (!bv) return NULL;

    co bvattributes = (co)coMapGet(psd, "bvattributes");
    co bvvaluepos = (co)coMapGet(psd, "bvvaluepos");
    co bvmask = (co)coMapGet(psd, "bvmask");

    coMapIterator iter;
    if (coMapLoopFirst(&iter, bvattributes)) {
        do {
            const char *attr_name = coMapLoopKey(&iter);
            cco attr_meta = coMapLoopValue(&iter);
            int32_t attr_idx = coInt32VectorGet(attr_meta, 0);
            
            cco and_term_vals = coMapGet(and_term, attr_name);
            if (and_term_vals == NULL) {
                /* Attribute missing: Universal, set all bits using mask */
                co_BVType mask = (co_BVType)coVectorGet(bvmask, attr_idx);
                coBVOR(bv, bv, mask);
            } else if (coIsInt32Vector(and_term_vals)) {
                /* Attribute present: set specific bits */
                cco val_map = coMapGet(bvvaluepos, attr_name);
                int32_t start_bit = coInt32VectorGet(attr_meta, 1);
                for (long i = 0; i < coInt32VectorSize(and_term_vals); i++) {
                    int32_t v = coInt32VectorGet(and_term_vals, i);
                    char buf[32];
                    sprintf(buf, "%d", v);
                    cco pos_obj = coMapGet(val_map, buf);
                    if (pos_obj) {
                        int32_t local_pos = (int32_t)coDblGet(pos_obj);
                        coBVSet(bv, start_bit + local_pos);
                    }
                }
            }
        } while (coMapLoopNext(&iter));
    }
    return bv;
}

co coNewANDTermFromBV(cco psd, co_BVType bv) {
    co and_term = coNewMap(CO_STRDUP | CO_FREE_VALS);
    co psd_inner = (co)coMapGet(psd, "psd");
    co bvattributes = (co)coMapGet(psd, "bvattributes");
    co bvmask = (co)coMapGet(psd, "bvmask");
    co bvpos = (co)coMapGet(psd, "bvpos");
    uint64_t total_bits = coInt32VectorGet(bvpos, coInt32VectorSize(bvpos) - 1);

    coMapIterator iter;
    if (coMapLoopFirst(&iter, bvattributes)) {
        do {
            const char *attr_name = coMapLoopKey(&iter);
            cco attr_meta = coMapLoopValue(&iter);
            int32_t attr_idx = coInt32VectorGet(attr_meta, 0);
            int32_t start_bit = coInt32VectorGet(attr_meta, 1);
            int32_t num_values = coInt32VectorGet(attr_meta, 2);

            co_BVType mask = (co_BVType)coVectorGet(bvmask, attr_idx);
            
            /* Check if attribute is universal: (bv & mask) == mask */
            co_BVType temp = coNewBV(total_bits);
            coBVAND(temp, bv, mask);
            int is_universal = coBVIsEqual(temp, mask);
            coDeleteBV(temp);

            if (!is_universal) {
                co vals = NULL;
                cco psd_vec = coMapGet(psd_inner, attr_name);

                for (int32_t i = 0; i < num_values; i++) {
                    if (coBVGet(bv, start_bit + i)) {
                        if (vals == NULL) vals = coNewInt32Vector(CO_NONE);
                        coInt32VectorAdd(vals, coInt32VectorGet(psd_vec, i));
                    }
                }
                if (vals) coMapAdd(and_term, attr_name, vals);
            }
        } while (coMapLoopNext(&iter));
    }
    return and_term;
}

co coNewBVDNFFromDNF(cco psd, cco dnf) {
    co bv_dnf = coNewVector(CO_FREE_VALS);
    long cnt = coVectorSize(dnf);
    for (long i = 0; i < cnt; i++) {
        cco and_term = coVectorGet(dnf, i);
        co_BVType bv = coNewBVFromANDTerm(psd, and_term);
        coVectorAdd(bv_dnf, (cco)bv);
    }
    return bv_dnf;
}

co coNewDNFFromBVDNF(cco psd, cco bv_dnf) {
    co dnf = coNewVector(CO_FREE_VALS);
    long cnt = coVectorSize(bv_dnf);
    for (long i = 0; i < cnt; i++) {
        co_BVType bv = (co_BVType)coVectorGet(bv_dnf, i);
        co and_term = coNewANDTermFromBV(psd, bv);
        coVectorAdd(dnf, and_term);
    }
    return dnf;
}

co coNewBVDNFByIntersectionWithoutMinimization(cco psd, cco arg1, cco arg2) {
    co bvpos = (co)coMapGet(psd, "bvpos");
    if (!bvpos) return NULL;
    uint64_t total_bits = coInt32VectorGet(bvpos, coInt32VectorSize(bvpos) - 1);

    co res = coNewVector(CO_FREE_VALS);
    long cnt1 = coVectorSize(arg1);
    long cnt2 = coVectorSize(arg2);

    for (long i = 0; i < cnt1; i++) {
        co_BVType a = (co_BVType)coVectorGet(arg1, i);
        for (long j = 0; j < cnt2; j++) {
            co_BVType b = (co_BVType)coVectorGet(arg2, j);
            co_BVType intersected = coNewBV(total_bits);
            if (coBVANDTermIntersect(psd, intersected, a, b)) {
                coVectorAdd(res, (cco)intersected);
            } else {
                coDeleteBV(intersected);
            }
        }
    }
    return res;
}

int coBVDNFIntersectionWithoutMinimization(cco psd, co arg1, cco arg2) {
    co res = coNewBVDNFByIntersectionWithoutMinimization(psd, arg1, arg2);
    if (res == NULL) return 0;

    coVectorClear(arg1);
    long cnt = coVectorSize(res);
    for (long i = 0; i < cnt; i++) {
        co element = (co)coVectorGet(res, i);
        /* Move elements to arg1. We need to clone because res will be deleted and it has CO_FREE_VALS */
        coVectorAdd(arg1, coClone((cco)element));
    }
    coDelete(res);
    return 1;
}

co coNewBVDNFByIntersection(cco psd, cco arg1, cco arg2) {
    co bvpos = (co)coMapGet(psd, "bvpos");
    if (!bvpos) return NULL;
    uint64_t total_bits = coInt32VectorGet(bvpos, coInt32VectorSize(bvpos) - 1);

    co res = coNewVector(CO_FREE_VALS);
    long cnt1 = coVectorSize(arg1);
    long cnt2 = coVectorSize(arg2);

    /* Scratch buffer for candidate terms */
    co_BVType scratch = coNewBV(total_bits);

    for (long i = 0; i < cnt1; i++) {
        co_BVType a = (co_BVType)coVectorGet(arg1, i);
        for (long j = 0; j < cnt2; j++) {
            co_BVType b = (co_BVType)coVectorGet(arg2, j);
            if (coBVANDTermIntersect(psd, scratch, a, b)) {
                /* Online Subset Minimization */
                int skip = 0;
                long k;
                for (k = 0; k < coVectorSize(res); k++) {
                    co_BVType existing = (co_BVType)coVectorGet(res, k);
                    if (existing == NULL) continue;
                    
                    int test = coBVSuperSubTest(scratch, existing);
                    if (test & 1) { /* intersected is subset of existing */
                        skip = 1;
                        break;
                    }
                    if (test & 2) { /* intersected is superset of existing */
                        coDelete((co)existing);
                        res->v.list[k] = NULL;
                    }
                }
                
                if (!skip) {
                    /* Add a clone of the scratch buffer to the result */
                    coVectorAdd(res, coClone((cco)scratch));
                }
            }
        }
    }
    coDeleteBV(scratch);

    /* Compress */
    long write_idx = 0;
    long total = coVectorSize(res);
    for (long read_idx = 0; read_idx < total; read_idx++) {
        if (res->v.list[read_idx] != NULL) {
            res->v.list[write_idx++] = res->v.list[read_idx];
        }
    }
    res->v.cnt = write_idx;

    return res;
}

int coBVDNFIntersection(cco psd, co arg1, cco arg2) {
    co res = coNewBVDNFByIntersection(psd, arg1, arg2);
    if (res == NULL) return 0;

    coVectorClear(arg1);
    long cnt = coVectorSize(res);
    for (long i = 0; i < cnt; i++) {
        co element = (co)coVectorGet(res, i);
        coVectorAdd(arg1, coClone((cco)element));
    }
    coDelete(res);
    return 1;
}

co coBVDNFNewCofactor(cco psd, cco dnf, const char *attr_name, int32_t value) {
    co bvattributes = (co)coMapGet(psd, "bvattributes");
    co bvvaluepos = (co)coMapGet(psd, "bvvaluepos");
    if (!bvattributes || !bvvaluepos) return NULL;

    cco attr_meta = coMapGet(bvattributes, attr_name);
    cco val_map = coMapGet(bvvaluepos, attr_name);
    if (!attr_meta || !val_map) return NULL;

    char buf[32];
    sprintf(buf, "%d", value);
    cco pos_obj = coMapGet(val_map, buf);
    if (!pos_obj) return coNewVector(CO_FREE_VALS); /* Empty result if value not in domain */

    int32_t start_bit = coInt32VectorGet(attr_meta, 1);
    int32_t local_pos = (int32_t)coDblGet(pos_obj);
    uint64_t bit_idx = (uint64_t)start_bit + local_pos;

    co res = coNewVector(CO_FREE_VALS);
    long cnt = coVectorSize(dnf);
    for (long i = 0; i < cnt; i++) {
        co_BVType bv = (co_BVType)coVectorGet(dnf, i);
        if (coBVGet(bv, bit_idx)) {
            co_BVType cloned = (co_BVType)coClone((cco)bv);
            /* Set entire attribute to universal in the cofactor */
            int32_t num_values = coInt32VectorGet(attr_meta, 2);
            for (int32_t j = 0; j < num_values; j++) {
                coBVSet(cloned, (uint64_t)start_bit + j);
            }
            coVectorAdd(res, (cco)cloned);
        }
    }
    return res;
}

int coBVDNFCheckUniversal(cco psd, cco dnf) {
    /* Use symbolic engine for universal check for now. */
    co symbolic_dnf = coNewDNFFromBVDNF(psd, dnf);
    int res = coDNFCheckUniversal(psd, symbolic_dnf);
    coDelete(symbolic_dnf);
    return res;
}

void coBVDNFMinimizeANDTermSubset(co dnf) {
    long i, j;
    if (!coIsVector(dnf)) return;
    
    for (i = 0; i < coVectorSize(dnf); i++) {
        co_BVType a = (co_BVType)coVectorGet(dnf, i);
        if (!a) continue;
        for (j = 0; j < coVectorSize(dnf); j++) {
            if (i == j) continue;
            co_BVType b = (co_BVType)coVectorGet(dnf, j);
            if (!b) continue;
            
            if (coBVIsSubset(a, b)) {
                coVectorErase(dnf, i);
                i--;
                break;
            }
        }
    }
}

void coBVDNFMinimizeByANDTermMerge(cco psd, co dnf) {
    if (!coIsVector(dnf)) return;

    co bvmask = (co)coMapGet(psd, "bvmask");
    co bvpos = (co)coMapGet(psd, "bvpos");
    if (!bvmask || !bvpos) return;
    uint64_t total_bits = coInt32VectorGet(bvpos, coInt32VectorSize(bvpos) - 1);

    co_BVType xor_res = coNewBV(total_bits);
    co_BVType temp = coNewBV(total_bits);

    int changed = 1;
    while (changed) {
        changed = 0;
        long i, j;
        for (i = 0; i < coVectorSize(dnf); i++) {
            co_BVType a = (co_BVType)coVectorGet(dnf, i);
            for (j = i + 1; j < coVectorSize(dnf); j++) {
                co_BVType b = (co_BVType)coVectorGet(dnf, j);
                
                coBVXOR(xor_res, a, b);
                
                /* Count how many attributes have bits set in the XOR result */
                int diff_attr_count = 0;
                long mask_cnt = coVectorSize(bvmask);
                for (long k = 0; k < mask_cnt; k++) {
                    co_BVType m = (co_BVType)coVectorGet(bvmask, k);
                    if (coBVANDTstZero(temp, xor_res, m)) {
                        diff_attr_count++;
                    }
                }
                
                if (diff_attr_count == 1) {
                    /* Merge terms */
                    coBVOR(a, a, b);
                    coVectorErase(dnf, j);
                    changed = 1;
                    break;
                } else if (diff_attr_count == 0) {
                    /* Identical terms */
                    coVectorErase(dnf, j);
                    changed = 1;
                    break;
                }
            }
            if (changed) break;
        }
    }
    coDeleteBV(xor_res);
    coDeleteBV(temp);
}

void coBVDNFMinimizeClearFullDomain(cco psd, co dnf) {
    /* In BV representation, "clearing full domain" (making an attribute universal) 
       is just having all bits set. MinimizeANDTermSubset will handle the logical 
       consequences. */
    coBVDNFMinimizeANDTermSubset(dnf);
}

int coBVDNFIsSubset(cco psd, cco subset_dnf, cco superset_dnf) {
    if (coVectorEmpty(subset_dnf)) return 1;
    if (coVectorEmpty(superset_dnf)) return 0;

    /* Use symbolic engine for subset check as a robust baseline. 
       This ensures bitvector DNF results can be verified against symbolic logic. */
    co symbolic_subset = coNewDNFFromBVDNF(psd, subset_dnf);
    co symbolic_superset = coNewDNFFromBVDNF(psd, superset_dnf);
    int res = coDNFIsSubset(psd, symbolic_subset, symbolic_superset);
    coDelete(symbolic_subset);
    coDelete(symbolic_superset);
    return res;
}

int coBVDNFIsEqual(cco psd, cco dnf1, cco dnf2) {
    return coBVDNFIsSubset(psd, dnf1, dnf2) && coBVDNFIsSubset(psd, dnf2, dnf1);
}

void coBVSet(co_BVType bv, uint64_t bit_idx) {
    co_bv_set_ptr(bv, bit_idx);
}

void coBVClr(co_BVType bv, uint64_t bit_idx) {
    co_bv_clr_ptr(bv, bit_idx);
}

void coBVClearAll(co_BVType bv) {
    co_bv_clear_all_ptr(bv);
}

int coBVGet(co_BVType bv, uint64_t bit_idx) {
    return co_bv_get_ptr(bv, bit_idx);
}

void coBVOR(co_BVType res, co_BVType a, co_BVType b) {
    co_bv_or_ptr(res, a, b);
}

void coBVAND(co_BVType res, co_BVType a, co_BVType b) {
    co_bv_and_ptr(res, a, b);
}

void coBVXOR(co_BVType res, co_BVType a, co_BVType b) {
    co_bv_xor_ptr(res, a, b);
}

void coBVANDNOT(co_BVType res, co_BVType a, co_BVType b) {
    co_bv_andnot_ptr(res, a, b);
}

int coBVIsEqual(co_BVType a, co_BVType b) {
    return co_bv_is_equal_ptr(a, b);
}

int coBVIsSubset(co_BVType a, co_BVType b) {
    return co_bv_is_subset_ptr(a, b);
}

int coBVSuperSubTest(co_BVType a, co_BVType b) {
    return co_bv_super_sub_test_ptr(a, b);
}

int coBVIsDisjoint(co_BVType a, co_BVType b) {
    return co_bv_is_disjoint_ptr(a, b);
}

/* 
   Computes bitwise res = a & b. 
   Returns 1 if at least one bit is set in the result, 0 if result is all zeros.
*/
int coBVANDTstZero(co_BVType res, co_BVType a, co_BVType b) {
    return co_bv_and_tst_zero_ptr(res, a, b);
}

int coBVAND3TstNonZero(co_BVType a, co_BVType b, co_BVType c) {
    return co_bv_and3_tst_nonzero_ptr(a, b, c);
}

int coBVANDTermIntersectionCheck(cco psd, co_BVType a, co_BVType b) {
    co bvmask = (co)coMapGet(psd, "bvmask");
    if (!bvmask) return 0;
    long cnt = coVectorSize(bvmask);
    for (long i = 0; i < cnt; i++) {
        co_BVType m = (co_BVType)coVectorGet(bvmask, i);
        if (!coBVAND3TstNonZero(a, b, m)) return 0;
    }
    return 1;
}

int coBVDNFIntersectionCheck(cco psd, cco arg1, cco arg2) {
    long cnt1 = coVectorSize(arg1);
    long cnt2 = coVectorSize(arg2);
    for (long i = 0; i < cnt1; i++) {
        co_BVType a = (co_BVType)coVectorGet(arg1, i);
        for (long j = 0; j < cnt2; j++) {
            co_BVType b = (co_BVType)coVectorGet(arg2, j);
            if (coBVANDTermIntersectionCheck(psd, a, b)) return 1;
        }
    }
    return 0;
}

int coBVANDTermIntersect(cco psd, co_BVType res, co_BVType a, co_BVType b) {
    co bvmask = (co)coMapGet(psd, "bvmask");
    if (!bvmask) return 0;

    if (coBVANDTstZero(res, a, b) == 0) return 0;

    long cnt = coVectorSize(bvmask);
    for (long i = 0; i < cnt; i++) {
        co_BVType m = (co_BVType)coVectorGet(bvmask, i);
        if (coBVIsDisjoint(res, m)) {
            /* Empty intersection for this attribute: entire term is empty */
            coBVClearAll(res);
            return 0;
        }
    }
    return 1;
}
