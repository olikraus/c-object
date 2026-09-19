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
typedef int (*co_bv_get_fn)(co_BVType bv, uint64_t bit_idx);
typedef void (*co_bv_op3_fn)(co_BVType res, co_BVType a, co_BVType b);
typedef int (*co_bv_is_equal_fn)(co_BVType a, co_BVType b);

/* Implementations forward declarations */
void co_bv_set_u64(co_BVType bv, uint64_t bit_idx);
void co_bv_clr_u64(co_BVType bv, uint64_t bit_idx);
int co_bv_get_u64(co_BVType bv, uint64_t bit_idx);
void co_bv_or_u64(co_BVType res, co_BVType a, co_BVType b);
void co_bv_and_u64(co_BVType res, co_BVType a, co_BVType b);
void co_bv_andnot_u64(co_BVType res, co_BVType a, co_BVType b);
int co_bv_is_equal_u64(co_BVType a, co_BVType b);

/* Global function pointers, default to u64 scalar versions */
static co_bv_set_fn co_bv_set_ptr = co_bv_set_u64;
static co_bv_clr_fn co_bv_clr_ptr = co_bv_clr_u64;
static co_bv_get_fn co_bv_get_ptr = co_bv_get_u64;
static co_bv_op3_fn co_bv_or_ptr = co_bv_or_u64;
static co_bv_op3_fn co_bv_and_ptr = co_bv_and_u64;
static co_bv_op3_fn co_bv_andnot_ptr = co_bv_andnot_u64;
static co_bv_is_equal_fn co_bv_is_equal_ptr = co_bv_is_equal_u64;

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
int co_bv_get_u64(co_BVType bv, uint64_t bit_idx) {
    return (bv->data.u64[bit_idx >> 6] >> (bit_idx & 0x3f)) & 1;
}
void co_bv_or_u64(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.u64[i] = a->data.u64[i] | b->data.u64[i];
}
void co_bv_and_u64(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.u64[i] = a->data.u64[i] & b->data.u64[i];
}
void co_bv_andnot_u64(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.u64[i] = a->data.u64[i] & ~b->data.u64[i];
}
int co_bv_is_equal_u64(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.u64, b->data.u64, (size_t)a->cnt * co_bv_base_size) == 0;
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
void co_bv_andnot_m128(co_BVType res, co_BVType a, co_BVType b) {
    /* Note: _mm_andnot_si128(b, a) computes (~b & a) */
    for (int i = 0; i < res->cnt; i++) res->data.m128[i] = _mm_andnot_si128(b->data.m128[i], a->data.m128[i]);
}
__attribute__((target("sse2")))
int co_bv_is_equal_m128(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.m128, b->data.m128, (size_t)a->cnt * co_bv_base_size) == 0;
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
void co_bv_andnot_m256(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m256[i] = _mm256_andnot_si256(b->data.m256[i], a->data.m256[i]);
}
__attribute__((target("avx2")))
int co_bv_is_equal_m256(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.m256, b->data.m256, (size_t)a->cnt * co_bv_base_size) == 0;
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
void co_bv_andnot_m512(co_BVType res, co_BVType a, co_BVType b) {
    for (int i = 0; i < res->cnt; i++) res->data.m512[i] = _mm512_andnot_si512(b->data.m512[i], a->data.m512[i]);
}
__attribute__((target("avx512f")))
int co_bv_is_equal_m512(co_BVType a, co_BVType b) {
    if (a->cnt != b->cnt) return 0;
    return memcmp(a->data.m512, b->data.m512, (size_t)a->cnt * co_bv_base_size) == 0;
}

void coBVDetect(void) {
    // Default (already set by static initialization, but re-assert here)
    co_bv_base_size = 8;
    co_bv_set_ptr = co_bv_set_u64;
    co_bv_clr_ptr = co_bv_clr_u64;
    co_bv_get_ptr = co_bv_get_u64;

    if (__builtin_cpu_supports("avx512f")) {
        co_bv_base_size = 64;
        co_bv_set_ptr = co_bv_set_m512;
        co_bv_clr_ptr = co_bv_clr_m512;
        co_bv_get_ptr = co_bv_get_m512;
        co_bv_or_ptr = co_bv_or_m512;
        co_bv_and_ptr = co_bv_and_m512;
        co_bv_andnot_ptr = co_bv_andnot_m512;
        co_bv_is_equal_ptr = co_bv_is_equal_m512;
    } else if (__builtin_cpu_supports("avx2")) {
        co_bv_base_size = 32;
        co_bv_set_ptr = co_bv_set_m256;
        co_bv_clr_ptr = co_bv_clr_m256;
        co_bv_get_ptr = co_bv_get_m256;
        co_bv_or_ptr = co_bv_or_m256;
        co_bv_and_ptr = co_bv_and_m256;
        co_bv_andnot_ptr = co_bv_andnot_m256;
        co_bv_is_equal_ptr = co_bv_is_equal_m256;
    } else if (__builtin_cpu_supports("sse2")) {
        co_bv_base_size = 16;
        co_bv_set_ptr = co_bv_set_m128;
        co_bv_clr_ptr = co_bv_clr_m128;
        co_bv_get_ptr = co_bv_get_m128;
        co_bv_or_ptr = co_bv_or_m128;
        co_bv_and_ptr = co_bv_and_m128;
        co_bv_andnot_ptr = co_bv_andnot_m128;
        co_bv_is_equal_ptr = co_bv_is_equal_m128;
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
    memset(mem, 0, (size_t)bv->cnt * co_bv_base_size);
    bv->data.u64 = mem;
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

void coBVSet(co_BVType bv, uint64_t bit_idx) {
    co_bv_set_ptr(bv, bit_idx);
}

void coBVClr(co_BVType bv, uint64_t bit_idx) {
    co_bv_clr_ptr(bv, bit_idx);
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

void coBVANDNOT(co_BVType res, co_BVType a, co_BVType b) {
    co_bv_andnot_ptr(res, a, b);
}

int coBVIsEqual(co_BVType a, co_BVType b) {
    return co_bv_is_equal_ptr(a, b);
}
