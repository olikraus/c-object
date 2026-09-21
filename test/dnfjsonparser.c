#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <immintrin.h>

static double get_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

/* Global storage for detected structural character positions */
uint32_t *json_char_pos_array = NULL;
size_t json_char_pos_cnt = 0;
size_t json_char_pos_max = 0;

void json_char_pos_init(void) {
    json_char_pos_max = 128;
    json_char_pos_array = (uint32_t*)malloc(json_char_pos_max * sizeof(uint32_t));
    json_char_pos_cnt = 0;
}

void json_char_pos_ensure_capacity(void) {
    if (json_char_pos_cnt + 16 > json_char_pos_max) {
        json_char_pos_max *= 2;
        json_char_pos_array = (uint32_t*)realloc(json_char_pos_array, json_char_pos_max * sizeof(uint32_t));
        if (!json_char_pos_array) {
            fprintf(stderr, "Error: Failed to reallocate structural character array\n");
            exit(1);
        }
    }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input.json>\n", argv[0]);
    return 1;
  }

  const char *filename = argv[1];
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    perror(filename);
    return 1;
  }

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (size < 0) {
      fclose(fp);
      return 1;
  }

  char *buffer = malloc(size + 32); /* Extra space for SIMD safety and padding */
  if (!buffer) {
      fclose(fp);
      return 1;
  }

  size_t bytes_read = fread(buffer, 1, size, fp);
  buffer[bytes_read] = '\0';
  memset(buffer + bytes_read, 0, 32);

  json_char_pos_init();

  /* SIMD Tables */
  __m128i ht = _mm_setr_epi8(
      0x00, 0x00, 0x03, 0x00, 0x00, 0x1C, 0x00, 0x60,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  );
  __m128i lt = _mm_setr_epi8(
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x24, 0x0A, 0x50, 0x00, 0x00
  );
  __m128i low_mask = _mm_set1_epi8(0x0F);
  __m128i zero = _mm_setzero_si128();

  printf("Scanning for structural characters (SSE4.2 Optimized)...\n");
  double t1 = get_ms();

  int in_string = 0;
  int skip_next_byte = 0;

  long pos = 0;
  while (pos < (long)bytes_read) {
      __m128i data = _mm_loadu_si128((__m128i*)(buffer + pos));
      
      __m128i low_nibbles = _mm_and_si128(data, low_mask);
      __m128i high_nibbles = _mm_and_si128(_mm_srli_epi16(data, 4), low_mask);

      __m128i low_lookup = _mm_shuffle_epi8(lt, low_nibbles);
      __m128i high_lookup = _mm_shuffle_epi8(ht, high_nibbles);

      __m128i result = _mm_and_si128(low_lookup, high_lookup);

      /* SSE4.1 check if block has any interesting bytes */
      if (!_mm_testz_si128(result, result)) {
          uint32_t found_mask = (uint32_t)_mm_movemask_epi8(_mm_cmpgt_epi8(result, zero));
          
          if (skip_next_byte) {
              found_mask &= ~1U;
              skip_next_byte = 0;
          }

          if (found_mask != 0) {
              json_char_pos_ensure_capacity();
              
              while (found_mask != 0) {
                  int i = __builtin_ctz(found_mask);
                  found_mask &= (found_mask - 1); /* Clear processed bit */
                  
                  char c = buffer[pos + i];
                  if (!in_string) {
                      json_char_pos_array[json_char_pos_cnt++] = (uint32_t)(pos + i);
                      if (c == '\"') in_string = 1;
                  } else {
                      /* Inside string: only care about quotes and escapes */
                      if (c == '\\') {
                          /* Skip the very next byte in the stream */
                          if (i < 15) {
                              found_mask &= ~(1U << (i + 1));
                          } else {
                              skip_next_byte = 1;
                          }
                      } else if (c == '\"') {
                          in_string = 0;
                          json_char_pos_array[json_char_pos_cnt++] = (uint32_t)(pos + i);
                      }
                  }
              }
          }
      } else {
          /* No structural characters in this block, but might need to clear skip flag */
          skip_next_byte = 0;
      }
      pos += 16;
  }

  double t2 = get_ms();
  printf("File: %s\n", filename);
  printf("Size: %ld bytes\n", size);
  printf("Structural characters found: %zu\n", json_char_pos_cnt);
  printf("Scan time: %.4f ms\n", t2 - t1);

  free(buffer);
  free(json_char_pos_array);
  fclose(fp);
  return 0;
}
