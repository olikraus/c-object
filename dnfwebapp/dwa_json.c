#include "dnfwebapp.h"

int dwa_read_file(dwa_t *p, const char *filename) {
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

    dwa_print(p, "Reading file...\n");
    double t1 = get_ms();
    size_t bytes_read = fread(p->buffer, 1, p->size, fp);
    p->buffer[bytes_read] = '\0';
    memset(p->buffer + bytes_read, 0, 32);
    double t2 = get_ms();
    dwa_print(p, "Read time:  %.4f ms\n", t2 - t1);

    fclose(fp);
    return 1;
}

void dwa_scan(dwa_t *p) {
#if defined(CO_HAS_INTEL_SIMD)
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

    dwa_print(p, "Scanning for structural characters (SSE4.2 Optimized)...\n");
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
            if (skip_next_byte) { found_mask &= ~1U; skip_next_byte = 0; }
            if (found_mask != 0) {
                dwa_ensure_capacity(p);
                while (found_mask != 0) {
                    int i = __builtin_ctz(found_mask);
                    found_mask &= (found_mask - 1); 
                    char c = buffer[pos + i];
                    if (!in_string) {
                        p->pos_array[p->pos_cnt++] = (uint32_t)(pos + i);
                        if (c == '\"') in_string = 1;
                    } else {
                        if (c == '\\') {
                            if (i < 15) found_mask &= ~(1U << (i + 1));
                            else skip_next_byte = 1;
                        } else if (c == '\"') {
                            in_string = 0;
                            p->pos_array[p->pos_cnt++] = (uint32_t)(pos + i);
                        }
                    }
                }
            }
        } else skip_next_byte = 0;
        pos += 16;
    }
    double t2 = get_ms();
    dwa_print(p, "Scan time:  %.4f ms\n", t2 - t1);
    dwa_print(p, "Structural characters found: %zu\n", p->pos_cnt);
#elif defined(CO_HAS_ARM_SIMD)
    static const uint8_t ht_data[16] = {
        0x00, 0x00, 0x03, 0x80, 0x00, 0x1C, 0x00, 0x60,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t lt_data[16] = {
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x24, 0x0A, 0x50, 0x00, 0x00
    };
    static const uint8_t powers[16] = {
        1, 2, 4, 8, 16, 32, 64, 128,
        1, 2, 4, 8, 16, 32, 64, 128
    };
    uint8x16_t ht = vld1q_u8(ht_data);
    uint8x16_t lt = vld1q_u8(lt_data);
    uint8x16_t bit_mask = vld1q_u8(powers);
    uint8x16_t low_mask = vdupq_n_u8(0x0F);

    dwa_print(p, "Scanning for structural characters (ARM NEON Optimized)...\n");
    double t1 = get_ms();

    int in_string = 0;
    int skip_next_byte = 0;
    long pos = 0;
    size_t bytes_read = p->size;
    char *buffer = p->buffer;

    while (pos < (long)bytes_read) {
        uint8x16_t data = vld1q_u8((const uint8_t *)(buffer + pos));
        uint8x16_t low_nibbles = vandq_u8(data, low_mask);
        uint8x16_t high_nibbles = vandq_u8(vshrq_n_u8(data, 4), low_mask);
        uint8x16_t low_lookup = vqtbl1q_u8(lt, low_nibbles);
        uint8x16_t high_lookup = vqtbl1q_u8(ht, high_nibbles);
        uint8x16_t result = vandq_u8(low_lookup, high_lookup);

        if (vmaxvq_u8(result) != 0) {
            uint8x16_t cmp = vtstq_u8(result, result);
            uint8x16_t masked = vandq_u8(cmp, bit_mask);
            uint32_t found_mask = (uint32_t)vaddv_u8(vget_low_u8(masked)) |
                                 ((uint32_t)vaddv_u8(vget_high_u8(masked)) << 8);

            if (skip_next_byte) { found_mask &= ~1U; skip_next_byte = 0; }
            if (found_mask != 0) {
                dwa_ensure_capacity(p);
                while (found_mask != 0) {
                    int i = __builtin_ctz(found_mask);
                    found_mask &= (found_mask - 1); 
                    char c = buffer[pos + i];
                    if (!in_string) {
                        p->pos_array[p->pos_cnt++] = (uint32_t)(pos + i);
                        if (c == '\"') in_string = 1;
                    } else {
                        if (c == '\\') {
                            if (i < 15) found_mask &= ~(1U << (i + 1));
                            else skip_next_byte = 1;
                        } else if (c == '\"') {
                            in_string = 0;
                            p->pos_array[p->pos_cnt++] = (uint32_t)(pos + i);
                        }
                    }
                }
            }
        } else skip_next_byte = 0;
        pos += 16;
    }
    double t2 = get_ms();
    dwa_print(p, "Scan time:  %.4f ms\n", t2 - t1);
    dwa_print(p, "Structural characters found: %zu\n", p->pos_cnt);
#else
    dwa_print(p, "Scanning for structural characters (Scalar)...\n");
    double t1 = get_ms();
    int in_string = 0;
    int escape = 0;
    for (size_t pos = 0; pos < p->size; pos++) {
        char c = p->buffer[pos];
        if (escape) { escape = 0; continue; }
        if (!in_string) {
            if (c == '\"' || c == ',' || c == '[' || c == ']' || c == '{' || c == '}' || c == ':') {
                dwa_ensure_capacity(p);
                p->pos_array[p->pos_cnt++] = (uint32_t)pos;
                if (c == '\"') in_string = 1;
            }
        } else {
            if (c == '\\') escape = 1;
            else if (c == '\"') {
                dwa_ensure_capacity(p);
                in_string = 0;
                p->pos_array[p->pos_cnt++] = (uint32_t)pos;
            }
        }
    }
    double t2 = get_ms();
    dwa_print(p, "Scan time:  %.4f ms\n", t2 - t1);
    dwa_print(p, "Structural characters found: %zu\n", p->pos_cnt);
#endif
}

char dwa_peek_token_char(dwa_t *p) {
    if (p->token_idx >= p->pos_cnt) return '\0';
    return p->buffer[p->pos_array[p->token_idx]];
}

char dwa_consume_token_char(dwa_t *p) {
    if (p->token_idx >= p->pos_cnt) return '\0';
    return p->buffer[p->pos_array[p->token_idx++]];
}

int dwa_has_content_between_tokens(dwa_t *p) {
    if (p->token_idx == 0 || p->token_idx >= p->pos_cnt) return 0;
    size_t start = p->pos_array[p->token_idx - 1] + 1;
    size_t end = p->pos_array[p->token_idx];
    for (size_t i = start; i < end; i++) {
        if (!isspace((unsigned char)p->buffer[i])) return 1;
    }
    return 0;
}

char *dwa_alloc_string(dwa_t *p) {
    if (dwa_peek_token_char(p) != '\"') dwa_error(p, "Expected '\"'");
    uint32_t start = p->pos_array[p->token_idx++] + 1;
    if (dwa_peek_token_char(p) != '\"') dwa_error(p, "Expected closing '\"'");
    uint32_t end = p->pos_array[p->token_idx++];
    size_t len = end - start;
    char *s = (char*)malloc(len + 1);
    memcpy(s, p->buffer + start, len);
    s[len] = '\0';
    return s;
}

void dwa_skip_string(dwa_t *p) {
    if (dwa_peek_token_char(p) != '\"') dwa_error(p, "Expected '\"'");
    p->token_idx++;
    if (dwa_peek_token_char(p) != '\"') dwa_error(p, "Expected closing '\"'");
    p->token_idx++;
}

int32_t dwa_parse_int(dwa_t *p) {
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
