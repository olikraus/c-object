#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <immintrin.h>
#include <ctype.h>
#include <stdarg.h>

/*
  JSON Operation Format Specification:
  {
    "op": "intersection" | "intersection-check",
    "arg1": [ { "dnf": <dnf> }, ... ] | { "dnf": <dnf> },
    "arg2": [ { "dnf": <dnf> }, ... ] | { "dnf": <dnf> }
  }
  
  <dnf> ::= [ <and-term>, ... ]
  <and-term> ::= { <attr>: [ <val>, ... ], ... }

  Example command for 100MB benchmark generation:
  ./dnf -gpsd 2 20 -gdnf 2 2 3 -gic 200000 1 -o tmp.json && ls -al tmp.json

  To test/run the benchmark:
  ./dnfjsonparser -v -o r.json tmp.json
*/

static double get_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

typedef struct {
    char *buffer;       /* Raw JSON content. Allocated and written by djp_read_file(). */
    size_t size;        /* Size of the raw JSON content. Written by djp_read_file(). */
    uint32_t *pos_array;/* Array of structural character positions. Allocated by djp_init()/djp_ensure_capacity(), written by djp_scan(). */
    size_t pos_cnt;     /* Number of structural characters found. Written by djp_scan(). */
    size_t pos_max;     /* Allocated size of pos_array. Written by djp_init() and djp_ensure_capacity(). */
    size_t token_idx;   /* Current token index during recursive parsing. Reset by djp_parse(), updated by consumption functions. */
    int verbose;        /* Output verbosity flag. Controlled by the caller (default: 0 in djp_init()). */
    const char *out_filename; /* Output filename for operation results. Controlled by the caller. */
    
    /* Operation State */
    char op_name[64];
    co psd;             /* Problem Space Description */
    co arg1_dnf_list;   /* Vector of BVDNFs */
    co arg2_dnf_list;   /* Vector of BVDNFs */
    co bvpos;           /* PSD: bvpos vector */
    uint64_t total_bits;/* PSD: total bits */
    co bvmask;          /* PSD: bvmask vector */
    co bvattributes;    /* PSD: bvattributes map */
    co bvvaluepos;      /* PSD: bvvaluepos map */
} djp_t;

void djp_print(djp_t *p, const char *fmt, ...) {
    if (p->verbose) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

void djp_init(djp_t *p) {
    p->buffer = NULL;
    p->size = 0;
    p->pos_max = 128;
    p->pos_array = (uint32_t*)malloc(p->pos_max * sizeof(uint32_t));
    p->pos_cnt = 0;
    p->token_idx = 0;
    p->verbose = 0;
    p->out_filename = NULL;
    
    p->psd = NULL;
    p->arg1_dnf_list = NULL;
    p->arg2_dnf_list = NULL;
    p->bvpos = NULL;
    p->total_bits = 0;
    p->bvmask = NULL;
    p->bvattributes = NULL;
    p->bvvaluepos = NULL;
    p->op_name[0] = '\0';
}

void djp_destroy(djp_t *p) {
    if (p->buffer) free(p->buffer);
    if (p->pos_array) free(p->pos_array);
    if (p->psd) coDelete(p->psd);
    if (p->arg1_dnf_list) coDelete(p->arg1_dnf_list);
    if (p->arg2_dnf_list) coDelete(p->arg2_dnf_list);
    memset(p, 0, sizeof(djp_t));
}

void djp_ensure_capacity(djp_t *p) {
    if (p->pos_cnt + 16 > p->pos_max) {
        p->pos_max *= 2;
        p->pos_array = (uint32_t*)realloc(p->pos_array, p->pos_max * sizeof(uint32_t));
        if (!p->pos_array) {
            fprintf(stderr, "Error: Failed to reallocate structural character array\n");
            exit(1);
        }
    }
}

void djp_error(djp_t *p, const char *msg) {
    size_t pos = (p->token_idx < p->pos_cnt) ? p->pos_array[p->token_idx] : p->size;
    fprintf(stderr, "Parser Error at position %zu (token %zu): %s\n", pos, p->token_idx, msg);
    exit(1);
}

int djp_read_file(djp_t *p, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror(filename);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 0) {
        fclose(fp);
        return 0;
    }

    p->size = (size_t)size;
    p->buffer = (char*)malloc(p->size + 32); 
    if (!p->buffer) {
        fclose(fp);
        return 0;
    }

    djp_print(p, "Reading file...\n");
    double t1 = get_ms();
    size_t bytes_read = fread(p->buffer, 1, p->size, fp);
    p->buffer[bytes_read] = '\0';
    memset(p->buffer + bytes_read, 0, 32);
    double t2 = get_ms();
    djp_print(p, "Read time:  %.4f ms\n", t2 - t1);

    fclose(fp);
    return 1;
}

void djp_scan(djp_t *p) {
    /* SIMD Tables */
    /* 
       Bit 0: 0x01: " (0x22)
       Bit 1: 0x02: , (0x2C)
       Bit 2: 0x04: [ (0x5B)
       Bit 3: 0x08: \ (0x5C)
       Bit 4: 0x10: ] (0x5D)
       Bit 5: 0x20: { (0x7B)
       Bit 6: 0x40: } (0x7D)
       Bit 7: 0x80: : (0x3A)
    */
    __m128i ht = _mm_setr_epi8(
        0x00, 0x00, 0x03, 0x80, 0x00, 0x1C, 0x00, 0x60,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    );
    __m128i lt = _mm_setr_epi8(
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x24, 0x0A, 0x50, 0x00, 0x00
    );
    __m128i low_mask = _mm_set1_epi8(0x0F);
    __m128i zero = _mm_setzero_si128();

    djp_print(p, "Scanning for structural characters (SSE4.2 Optimized)...\n");
    double t1 = get_ms();

    int in_string = 0;
    int skip_next_byte = 0;
    long pos = 0;
    size_t bytes_read = p->size;
    char *buffer = p->buffer;

    while (pos < (long)bytes_read) {
        __m128i data = _mm_loadu_si128((__m128i*)(buffer + pos));
        
        __m128i low_nibbles = _mm_and_si128(data, low_mask);
        __m128i high_nibbles = _mm_and_si128(_mm_srli_epi16(data, 4), low_mask);

        __m128i low_lookup = _mm_shuffle_epi8(lt, low_nibbles);
        __m128i high_lookup = _mm_shuffle_epi8(ht, high_nibbles);

        __m128i result = _mm_and_si128(low_lookup, high_lookup);

        if (!_mm_testz_si128(result, result)) {
            uint32_t found_mask = (uint32_t)~_mm_movemask_epi8(_mm_cmpeq_epi8(result, zero)) & 0xFFFF;
            
            if (skip_next_byte) {
                found_mask &= ~1U;
                skip_next_byte = 0;
            }

            if (found_mask != 0) {
                djp_ensure_capacity(p);
                
                while (found_mask != 0) {
                    int i = __builtin_ctz(found_mask);
                    found_mask &= (found_mask - 1); 
                    
                    char c = buffer[pos + i];
                    if (!in_string) {
                        p->pos_array[p->pos_cnt++] = (uint32_t)(pos + i);
                        if (c == '\"') in_string = 1;
                    } else {
                        if (c == '\\') {
                            if (i < 15) {
                                found_mask &= ~(1U << (i + 1));
                            } else {
                                skip_next_byte = 1;
                            }
                        } else if (c == '\"') {
                            in_string = 0;
                            p->pos_array[p->pos_cnt++] = (uint32_t)(pos + i);
                        }
                    }
                }
            }
        } else {
            skip_next_byte = 0;
        }
        pos += 16;
    }

    double t2 = get_ms();
    djp_print(p, "Scan time:  %.4f ms\n", t2 - t1);
    djp_print(p, "Structural characters found: %zu\n", p->pos_cnt);
}

char djp_peek_token_char(djp_t *p) {
    if (p->token_idx >= p->pos_cnt) return '\0';
    return p->buffer[p->pos_array[p->token_idx]];
}

static char djp_consume_token_char(djp_t *p) {
    if (p->token_idx >= p->pos_cnt) return '\0';
    return p->buffer[p->pos_array[p->token_idx++]];
}

int djp_has_content_between_tokens(djp_t *p) {
    if (p->token_idx == 0 || p->token_idx >= p->pos_cnt) return 0;
    size_t start = p->pos_array[p->token_idx - 1] + 1;
    size_t end = p->pos_array[p->token_idx];
    for (size_t i = start; i < end; i++) {
        if (!isspace((unsigned char)p->buffer[i])) return 1;
    }
    return 0;
}

/* Low-level parsing helpers */

char *djp_alloc_string(djp_t *p) {
    if (djp_peek_token_char(p) != '\"') djp_error(p, "Expected '\"'");
    uint32_t start = p->pos_array[p->token_idx++] + 1;
    if (djp_peek_token_char(p) != '\"') djp_error(p, "Expected closing '\"'");
    uint32_t end = p->pos_array[p->token_idx++];
    size_t len = end - start;
    char *s = (char*)malloc(len + 1);
    memcpy(s, p->buffer + start, len);
    s[len] = '\0';
    return s;
}

void djp_skip_string(djp_t *p) {
    if (djp_peek_token_char(p) != '\"') djp_error(p, "Expected '\"'");
    p->token_idx++;
    if (djp_peek_token_char(p) != '\"') djp_error(p, "Expected closing '\"'");
    p->token_idx++;
}

int32_t djp_parse_int(djp_t *p) {
    if (p->token_idx == 0 || p->token_idx + 1 >= p->pos_cnt) return 0;
    
    size_t start = p->pos_array[p->token_idx - 1] + 1;
    while (isspace((unsigned char)p->buffer[start])) start++;
    
    int32_t val = 0;
    while (p->buffer[start] >= '0' && p->buffer[start] <= '9') {
        val = val * 10 + (p->buffer[start] - '0');
        start++;
    }
    return val;
}

/* Phase 1: PSD Collection */

void djp_collect_psd_dnf(djp_t *p) {
    if (djp_consume_token_char(p) != '[') djp_error(p, "Expected '[' for DNF");
    if (djp_peek_token_char(p) == ']') {
        djp_consume_token_char(p);
        return;
    }
    for (;;) {
        if (djp_consume_token_char(p) != '{') djp_error(p, "Expected '{' for AND-term");
        if (djp_peek_token_char(p) != '}') {
            for (;;) {
                char *attr_name = djp_alloc_string(p);
                if (djp_consume_token_char(p) != ':') djp_error(p, "Expected ':'");
                if (djp_consume_token_char(p) != '[') djp_error(p, "Expected '['");
                
                if (djp_peek_token_char(p) != ']') {
                    for (;;) {
                        int32_t val = djp_parse_int(p);
                        coPSDExtendByValue(p->psd, attr_name, val);
                        char c = djp_consume_token_char(p);
                        if (c == ']') break;
                        if (c != ',') djp_error(p, "Expected ',' or ']'");
                    }
                } else {
                    djp_consume_token_char(p); // consume ']'
                }
                free(attr_name);
                char c = djp_consume_token_char(p);
                if (c == '}') break;
                if (c != ',') djp_error(p, "Expected ',' or '}'");
            }
        } else {
            djp_consume_token_char(p); // consume '}'
        }
        char c = djp_consume_token_char(p);
        if (c == ']') break;
        if (c != ',') djp_error(p, "Expected ',' or ']'");
        if (djp_peek_token_char(p) == ']') djp_error(p, "Trailing comma not allowed");
    }
}

void djp_collect_psd_recursive(djp_t *p) {
    char c = djp_peek_token_char(p);
    if (c == '{') {
        djp_consume_token_char(p);
        if (djp_peek_token_char(p) == '}') {
            djp_consume_token_char(p);
            return;
        }
        for (;;) {
            char *key = djp_alloc_string(p);
            if (djp_consume_token_char(p) != ':') djp_error(p, "Expected ':'");
            if (strcmp(key, "dnf") == 0) {
                djp_collect_psd_dnf(p);
            } else {
                djp_collect_psd_recursive(p);
            }
            free(key);
            char c2 = djp_consume_token_char(p);
            if (c2 == '}') break;
            if (c2 != ',') djp_error(p, "Expected ',' or '}'");
        }
    } else if (c == '[') {
        djp_consume_token_char(p);
        if (djp_peek_token_char(p) == ']') {
            djp_consume_token_char(p);
            return;
        }
        for (;;) {
            djp_collect_psd_recursive(p);
            char c2 = djp_consume_token_char(p);
            if (c2 == ']') break;
            if (c2 != ',') djp_error(p, "Expected ',' or ']'");
        }
    } else if (c == '\"') {
        djp_skip_string(p);
    } else {
        while (p->token_idx < p->pos_cnt && p->buffer[p->pos_array[p->token_idx]] != ',' && p->buffer[p->pos_array[p->token_idx]] != ']' && p->buffer[p->pos_array[p->token_idx]] != '}') p->token_idx++;
    }
}

/* Phase 3: Bitvector DNF Construction */

co djp_parse_bvdnf(djp_t *p) {
    co bvdnf = coNewVector(CO_FREE_VALS);
    djp_consume_token_char(p);
    if (djp_peek_token_char(p) == ']') {
        djp_consume_token_char(p);
        return bvdnf;
    }

    for (;;) {
        djp_consume_token_char(p);
        co_BVType bv = coNewBV(p->total_bits);
        
        if (djp_peek_token_char(p) != '}') {
            coBVSetToUniversal(p->psd, bv);
            for (;;) {
                char *attr_name = djp_alloc_string(p);
                cco attr_meta = coMapGet(p->bvattributes, attr_name);
                int32_t attr_idx = coInt32VectorGet(attr_meta, 0);
                int32_t start_bit = coInt32VectorGet(attr_meta, 1);
                co val_map = (co)coMapGet(p->bvvaluepos, attr_name);
                
                co_BVType mask = (co_BVType)coVectorGet(p->bvmask, attr_idx);
                coBVANDNOT(bv, bv, mask); // Clear attribute bits

                djp_consume_token_char(p); // :
                djp_consume_token_char(p); // [
                
                if (djp_peek_token_char(p) != ']') {
                    for (;;) {
                        int32_t val = djp_parse_int(p);
                        char buf[32]; sprintf(buf, "%d", val);
                        cco pos_obj = coMapGet(val_map, buf);
                        if (pos_obj) {
                            int32_t local_pos = (int32_t)coDblGet(pos_obj);
                            coBVSet(bv, start_bit + local_pos);
                        }
                        
                        char c = djp_consume_token_char(p);
                        if (c == ']') break;
                        if (c != ',') djp_error(p, "Expected ',' or ']'");
                    }
                } else {
                    djp_consume_token_char(p);
                }
                free(attr_name);
                char c = djp_consume_token_char(p);
                if (c == '}') break;
                if (c != ',') djp_error(p, "Expected ',' or '}'");
            }
        } else {
            djp_consume_token_char(p);
            coBVSetToUniversal(p->psd, bv);
        }
        coVectorAdd(bvdnf, (cco)bv);
        char c = djp_consume_token_char(p);
        if (c == ']') break;
        if (c != ',') djp_error(p, "Expected ',' or ']'");
        if (djp_peek_token_char(p) == ']') djp_error(p, "Trailing comma not allowed");
    }
    return bvdnf;
}

void djp_build_bvdnf_recursive(djp_t *p, co *target_list) {
    char c = djp_peek_token_char(p);
    if (c == '{') {
        djp_consume_token_char(p);
        if (djp_peek_token_char(p) == '}') {
            djp_consume_token_char(p);
            return;
        }
        for (;;) {
            char *key = djp_alloc_string(p);
            djp_consume_token_char(p); // :
            if (strcmp(key, "op") == 0) {
                char *op = djp_alloc_string(p);
                strncpy(p->op_name, op, 63);
                free(op);
            } else if (strcmp(key, "arg1") == 0) {
                p->arg1_dnf_list = coNewVector(CO_FREE_VALS);
                djp_build_bvdnf_recursive(p, &p->arg1_dnf_list);
            } else if (strcmp(key, "arg2") == 0) {
                p->arg2_dnf_list = coNewVector(CO_FREE_VALS);
                djp_build_bvdnf_recursive(p, &p->arg2_dnf_list);
            } else if (strcmp(key, "dnf") == 0) {
                /* Expect { "dnf": [...], "id": 123 } */
                co bvdnf = djp_parse_bvdnf(p);
                
                // Parse optional ID
                int32_t id = 0;
                // Peek next char. If it's a comma, there might be an "id" field
                char c_peek = djp_peek_token_char(p);
                if (c_peek == ',') {
                    djp_consume_token_char(p); // ,
                    char *key2 = djp_alloc_string(p);
                    djp_consume_token_char(p); // :
                    if (strcmp(key2, "id") == 0) {
                        id = djp_parse_int(p);
                    }
                    free(key2);
                }

                co dnf_obj = coNewMap(CO_STRDUP | CO_FREE_VALS);
                coMapAdd(dnf_obj, "dnf", (cco)bvdnf);
                coMapAdd(dnf_obj, "id", (cco)coNewDbl((double)id)); // Store ID as Dbl
                
                if (target_list && *target_list) coVectorAdd(*target_list, dnf_obj); else coDelete(dnf_obj);
            } else {
                djp_build_bvdnf_recursive(p, target_list);
            }
            free(key);
            char c2 = djp_consume_token_char(p);
            if (c2 == '}') break;
        }
    } else if (c == '[') {
        djp_consume_token_char(p);
        if (djp_peek_token_char(p) == ']') {
            djp_consume_token_char(p);
            return;
        }
        for (;;) {
            djp_build_bvdnf_recursive(p, target_list);
            char c2 = djp_consume_token_char(p);
            if (c2 == ']') break;
        }
    } else if (c == '\"') {
        djp_skip_string(p);
    } else {
        while (p->token_idx < p->pos_cnt && p->buffer[p->pos_array[p->token_idx]] != ',' && p->buffer[p->pos_array[p->token_idx]] != ']' && p->buffer[p->pos_array[p->token_idx]] != '}') p->token_idx++;
    }
}

/* Phase 4: Execution */

void djp_execute_op(djp_t *p) {
    if (!p->arg1_dnf_list || !p->arg2_dnf_list) return;
    int is_check = (strcmp(p->op_name, "intersection-check") == 0);
    co result_list = coNewVector(CO_FREE_VALS);
    long n = coVectorSize(p->arg1_dnf_list);
    long m = coVectorSize(p->arg2_dnf_list);

    djp_print(p, "Arg1 DNF count: %ld\n", n);
    djp_print(p, "Arg2 DNF count: %ld\n", m);
    djp_print(p, "Total intersections to execute: %ld\n", n * m);

    double t1 = get_ms();
    for (long i = 0; i < n; i++) {
        co dnf_obj1 = (co)coVectorGet(p->arg1_dnf_list, i);
        co bv1 = (co)coMapGet(dnf_obj1, "dnf");
        int32_t id1 = (int32_t)coDblGet(coMapGet(dnf_obj1, "id"));

        for (long j = 0; j < m; j++) {
            co dnf_obj2 = (co)coVectorGet(p->arg2_dnf_list, j);
            co bv2 = (co)coMapGet(dnf_obj2, "dnf");
            int32_t id2 = (int32_t)coDblGet(coMapGet(dnf_obj2, "id"));
            
            if (is_check) {
                int is_not_empty = coBVDNFIntersectionCheck(p->psd, (cco)bv1, (cco)bv2);
                
                co res_obj = coNewMap(CO_STRDUP | CO_FREE_VALS);
                co id_vec = coNewInt32Vector(CO_NONE);
                coInt32VectorAdd(id_vec, id1);
                coInt32VectorAdd(id_vec, id2);
                coMapAdd(res_obj, "id", (cco)id_vec);
                coMapAdd(res_obj, "isEmpty", (cco)coNewBool(!is_not_empty));
                coVectorAdd(result_list, (cco)res_obj);
            } else {
                co res_bv = coNewBVDNFByIntersectionWithoutMinimization(p->psd, (cco)bv1, (cco)bv2);
                coVectorAdd(result_list, (cco)coNewDNFFromBVDNF(p->psd, (cco)res_bv));
                coDelete(res_bv);
            }
        }
    }
    double t2 = get_ms();
    djp_print(p, "Op execution time: %.4f ms\n", t2 - t1);

    /* Output results */
    FILE *out_f = stdout;
    if (p->out_filename != NULL) {
        out_f = fopen(p->out_filename, "w");
        if (out_f == NULL) {
            perror(p->out_filename);
            out_f = stdout;
        }
    }

    djp_print(p, "Operation '%s' result:\n", p->op_name);
    double t3 = get_ms();
    coWriteJSON(result_list, 0, 0, out_f);
    fprintf(out_f, "\n");
    double t4 = get_ms();
    djp_print(p, "Write result time: %.4f ms\n", t4 - t3);

    if (out_f != stdout) {
        fclose(out_f);
    }
    coDelete(result_list);
}


void djp_process_op_json(djp_t *p) {
    /* Phase 1: PSD Collection */
    double t1 = get_ms();
    p->psd = coNewPSD();
    p->token_idx = 0;
    djp_collect_psd_recursive(p);
    double t2 = get_ms();
    djp_print(p, "psd parser time: %.4f ms\n", t2 - t1);

    /* Phase 2: BV Preparation */
    double t3 = get_ms();
    coBVPreparePSD(p->psd);
    
    p->bvpos = (co)coMapGet(p->psd, "bvpos");
    p->total_bits = coInt32VectorGet(p->bvpos, coInt32VectorSize(p->bvpos) - 1);
    p->bvmask = (co)coMapGet(p->psd, "bvmask");
    p->bvattributes = (co)coMapGet(p->psd, "bvattributes");
    p->bvvaluepos = (co)coMapGet(p->psd, "bvvaluepos");

    double t4 = get_ms();
    djp_print(p, "psd bv prep time: %.4f ms\n", t4 - t3);
    
    /* Phase 3: BVDNF Construction */
    double t5 = get_ms();
    p->token_idx = 0;
    djp_build_bvdnf_recursive(p, NULL);
    double t6 = get_ms();
    djp_print(p, "dnf parser time: %.4f ms\n", t6 - t5);
    
    djp_execute_op(p);
}

/* Validation Reference */

void djp_validate_value(djp_t *p);

void djp_validate_object(djp_t *p) {
    djp_consume_token_char(p);
    if (djp_peek_token_char(p) == '}') { djp_consume_token_char(p); return; }
    for (;;) {
        djp_skip_string(p);
        djp_consume_token_char(p); // :
        djp_validate_value(p);
        char c = djp_consume_token_char(p);
        if (c == '}') break;
        if (djp_peek_token_char(p) == '}' && !djp_has_content_between_tokens(p)) djp_error(p, "Trailing comma");
    }
}

void djp_validate_array(djp_t *p) {
    djp_consume_token_char(p);
    if (djp_peek_token_char(p) == ']') { djp_consume_token_char(p); return; }
    for (;;) {
        djp_validate_value(p);
        char c = djp_consume_token_char(p);
        if (c == ']') break;
        if (djp_peek_token_char(p) == ']' && !djp_has_content_between_tokens(p)) djp_error(p, "Trailing comma");
    }
}

void djp_validate_value(djp_t *p) {
    char c = djp_peek_token_char(p);
    if (c == '{') djp_validate_object(p);
    else if (c == '[') djp_validate_array(p);
    else if (c == '\"') djp_skip_string(p);
    else {
        if (c == ',' || c == ']' || c == '}' || c == '\0' || c == ':') return;
        djp_error(p, "Unexpected char");
    }
}

void djp_validate(djp_t *p) {
    djp_print(p, "Validating JSON structure (Reference)...\n");
    double t1 = get_ms();
    p->token_idx = 0;
    djp_validate_value(p);
    double t2 = get_ms();
    djp_print(p, "Validation time: %.4f ms\n", t2 - t1);
}

int main(int argc, char **argv) {
  int i;
  int verbose = 0;
  int force_validate = 0;
  const char *filename = NULL;
  const char *out_filename = NULL;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0) verbose = 1;
    else if (strcmp(argv[i], "-t") == 0) force_validate = 1;
    else if (strcmp(argv[i], "-o") == 0) {
        if (i + 1 < argc) {
            out_filename = argv[++i];
        } else {
            fprintf(stderr, "Error: -o option requires a filename\n");
            return 1;
        }
    }
    else if (argv[i][0] == '-') { fprintf(stderr, "Unknown option: %s\n", argv[i]); return 1; }
    else filename = argv[i];
  }

  if (filename == NULL) {
    fprintf(stderr, "Usage: %s [-v] [-t] [-o <output.json>] <input.json>\n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v: Verbose output (timing and internal state)\n");
    fprintf(stderr, "  -t: Force structural validation mode (reference implementation)\n");
    fprintf(stderr, "  -o <file>: Write operation results to specified file\n");
    return 1;
  }

  djp_t p;
  djp_init(&p);
  p.verbose = verbose;
  p.out_filename = out_filename;

  coBVDetect();
  const char *simd_name = "Scalar uint64_t";
  if (co_bv_base_size == 16) simd_name = "SSE2 128-bit";
  else if (co_bv_base_size == 32) simd_name = "AVX2 256-bit";
  else if (co_bv_base_size == 64) simd_name = "AVX-512 512-bit";
  djp_print(&p, "Using Bitset Base Type: %s\n", simd_name);

  double t_start = get_ms();

  if (!djp_read_file(&p, filename)) {
      djp_destroy(&p);
      return 1;
  }

  djp_scan(&p);

  if (force_validate) {
      djp_validate(&p);
  } else {
      /* Detect Mode: Validation or Operation */
      p.token_idx = 0;
      int is_op = 0;
      if (djp_peek_token_char(&p) == '{') {
          is_op = 1;
      }
      if (is_op) {
          djp_process_op_json(&p);
      } else {
          djp_validate(&p);
      }
  }

  double t_end = get_ms();
  djp_print(&p, "Total time: %.4f ms (including file read)\n", t_end - t_start);

  djp_destroy(&p);
  return 0;
}
