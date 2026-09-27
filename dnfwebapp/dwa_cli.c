#include "dnfwebapp.h"

int dwa_cli(int argc, char **argv) {
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
    fprintf(stderr, "Usage: dnfwebapp [-v] [-t] [-o <output.json>] <input.json>\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v: Verbose output (timing and internal state)\n");
    fprintf(stderr, "  -t: Force structural validation mode (reference implementation)\n");
    fprintf(stderr, "  -o <file>: Write operation results to specified file\n");
    return 1;
  }

  dwa_t p;
  dwa_init(&p);
  p.verbose = verbose;
  p.out_filename = out_filename;

  coBVDetect();
  const char *simd_name = "Scalar uint64_t";
#if defined(CO_HAS_ARM_SIMD)
  if (co_bv_base_size == 16) simd_name = "ARM NEON 128-bit";
#else
  if (co_bv_base_size == 16) simd_name = "SSE2 128-bit";
#endif
  else if (co_bv_base_size == 32) simd_name = "AVX2 256-bit";
  else if (co_bv_base_size == 64) simd_name = "AVX-512 512-bit";
  dwa_print(&p, "Using Bitset Base Type: %s\n", simd_name);

  if (!dwa_read_file(&p, filename)) {
      dwa_destroy(&p);
      return 1;
  }

  dwa_scan(&p);

  if (force_validate) {
      dwa_validate(&p);
  } else {
      /* Detect Mode: Validation or Operation */
      p.token_idx = 0;
      int is_op = 0;
      if (dwa_peek_token_char(&p) == '{') {
          is_op = 1;
      }
      if (is_op) {
          dwa_process_op_json(&p);
      } else {
          dwa_validate(&p);
      }
  }

  dwa_destroy(&p);
  return 0;
}

int main(int argc, char **argv) {
    return dwa_cli(argc, argv);
}
