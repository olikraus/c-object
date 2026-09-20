#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "co.h"

static double get_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void print_help(const char *prog) {
  printf("Usage: %s [options] [arg1.json arg2.json ...]\n", prog);
  printf("Options:\n");
  printf("  -h                  Show this help message\n");
  printf("  -union              Execute DNF union on exactly two input files\n");
  printf("  -intersection       Execute DNF intersection on exactly two input files\n");
  printf("  -subtract           Execute DNF subtraction on exactly two input files\n");
  printf("  -complement         Execute DNF complement on exactly one input file\n");
  printf("  -equal              Check whether two DNF files are equal\n");
  printf("  -cofactor <a> <v>   Execute DNF cofactor for attribute <a> and value <v>\n");
  printf("  -check-universal    Check whether a DNF covers the entire PSD space\n");
  printf("  -test               Execute several internal consistency tests on a single DNF\n");
  printf("  -test-isec <a> <v>  Execute intersection benchmark (requires -gpsd) with <a> terms and <v> values each\n");
  printf("  -o <file>           Write the resulting DNF to a named JSON file\n");
  printf("  -o1 <file>          Write the first input DNF (arg1) to a named JSON file\n");
  printf("  -o2 <file>          Write the second input DNF (arg2) to a named JSON file\n");
  printf("  -psd                Additionally output the PSD content\n");
  printf("  -ipsd <file>        Import PSD from a JSON file as initial setup\n");
  printf("  -opsd <file>        Write the extended PSD to a named JSON file\n");
  printf("  -gpsd <attrs> <val> Quickly generate a PSD with attributes/values\n");
  printf("  -gdnf <t> <a> <v>   Generate a random DNF as result (t: terms, a: attrs, v: values)\n");
  printf("  -seed <s>           Set the seed for random operations (default: 12345)\n");
  printf("  -v                  Verbose mode (output input files and commands)\n");
}

int main(int argc, char **argv) {
  coBVDetect();
  int help = 0;
  int verbose = 0;
  int show_psd = 0;
  int is_union = 0;
  int is_intersection = 0;
  int is_subtract = 0;
  int is_complement = 0;
  int is_equal = 0;
  int is_cofactor = 0;
  int is_check_universal = 0;
  int is_test = 0;
  int is_test_isec = 0;
  long test_isec_terms = 0;
  long test_isec_vals = 0;
  const char *cofactor_attr = NULL;
  int32_t cofactor_val = 0;
  int needs_delete = 0;
  const char *o_path = NULL;
  const char *o1_path = NULL;
  const char *o2_path = NULL;
  const char *ipsd_path = NULL;
  const char *opsd_path = NULL;
  long gpsd_attrs = 0;
  long gpsd_vals = 0;
  int has_gpsd = 0;
  long gdnf_terms = 0;
  long gdnf_attrs = 0;
  long gdnf_vals = 0;
  int has_gdnf = 0;
  unsigned int seed = 12345;

  if (argc == 1) {
    help = 1;
  }

  const char *files[128];
  int file_cnt = 0;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      help = 1;
    } else if (strcmp(argv[i], "-v") == 0) {
      verbose = 1;
    } else if (strcmp(argv[i], "-psd") == 0) {
      show_psd = 1;
    } else if (strcmp(argv[i], "-union") == 0) {
      is_union = 1;
    } else if (strcmp(argv[i], "-intersection") == 0) {
      is_intersection = 1;
    } else if (strcmp(argv[i], "-subtract") == 0) {
      is_subtract = 1;
    } else if (strcmp(argv[i], "-complement") == 0) {
      is_complement = 1;
    } else if (strcmp(argv[i], "-equal") == 0) {
      is_equal = 1;
    } else if (strcmp(argv[i], "-cofactor") == 0) {
      if (i + 2 < argc) {
        cofactor_attr = argv[i + 1];
        cofactor_val = (int32_t)atol(argv[i + 2]);
        is_cofactor = 1;
        i += 2;
      } else {
        fprintf(stderr, "Error: -cofactor option requires two arguments: <attribute-name> <value>\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-check-universal") == 0) {
      is_check_universal = 1;
    } else if (strcmp(argv[i], "-test") == 0) {
      is_test = 1;
    } else if (strcmp(argv[i], "-test-isec") == 0) {
      if (i + 2 < argc) {
        test_isec_terms = atol(argv[i + 1]);
        test_isec_vals = atol(argv[i + 2]);
        is_test_isec = 1;
        i += 2;
      } else {
        fprintf(stderr, "Error: -test-isec option requires two arguments: <terms> <values>\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        o_path = argv[i + 1];
        i++;
      } else {
        fprintf(stderr, "Error: -o option requires a file path argument\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-o1") == 0) {
      if (i + 1 < argc) {
        o1_path = argv[i + 1];
        i++;
      } else {
        fprintf(stderr, "Error: -o1 option requires a file path argument\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-o2") == 0) {
      if (i + 1 < argc) {
        o2_path = argv[i + 1];
        i++;
      } else {
        fprintf(stderr, "Error: -o2 option requires a file path argument\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-gpsd") == 0) {
      if (i + 2 < argc) {
        gpsd_attrs = atol(argv[i + 1]);
        gpsd_vals = atol(argv[i + 2]);
        has_gpsd = 1;
        i += 2;
      } else {
        fprintf(stderr, "Error: -gpsd option requires two arguments: <number-of-attributes> <number-of-values-per-attribute>\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-gdnf") == 0) {
      if (i + 3 < argc) {
        gdnf_terms = atol(argv[i + 1]);
        gdnf_attrs = atol(argv[i + 2]);
        gdnf_vals = atol(argv[i + 3]);
        has_gdnf = 1;
        i += 3;
      } else {
        fprintf(stderr, "Error: -gdnf option requires three arguments: <terms> <attributes> <values>\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-ipsd") == 0) {
      if (i + 1 < argc) {
        ipsd_path = argv[i + 1];
        i++;
      } else {
        fprintf(stderr, "Error: -ipsd option requires a file path argument\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-opsd") == 0) {
      if (i + 1 < argc) {
        opsd_path = argv[i + 1];
        i++;
      } else {
        fprintf(stderr, "Error: -opsd option requires a file path argument\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-seed") == 0) {
      if (i + 1 < argc) {
        seed = (unsigned int)atol(argv[i + 1]);
        i++;
      } else {
        fprintf(stderr, "Error: -seed option requires an integer argument\n");
        return 1;
      }
    } else {
      /* Positional argument */
      if (file_cnt < 128) {
        files[file_cnt++] = argv[i];
      } else {
        fprintf(stderr, "Error: Too many input files (max 128)\n");
        return 1;
      }
    }
  }

  if (help) {
    print_help(argv[0]);
    return 0;
  }

  srand(seed);

  /* Validate constraints */
  if (ipsd_path != NULL && has_gpsd) {
    fprintf(stderr, "Error: -ipsd and -gpsd are mutually exclusive\n");
    return 1;
  }

  if (is_union && is_intersection) {
    fprintf(stderr, "Error: -union and -intersection are mutually exclusive\n");
    return 1;
  }

  if (is_union && is_subtract) {
    fprintf(stderr, "Error: -union and -subtract are mutually exclusive\n");
    return 1;
  }

  if (is_union && is_complement) {
    fprintf(stderr, "Error: -union and -complement are mutually exclusive\n");
    return 1;
  }

  if (is_intersection && is_subtract) {
    fprintf(stderr, "Error: -intersection and -subtract are mutually exclusive\n");
    return 1;
  }

  if (is_intersection && is_complement) {
    fprintf(stderr, "Error: -intersection and -complement are mutually exclusive\n");
    return 1;
  }

  if (is_subtract && is_complement) {
    fprintf(stderr, "Error: -subtract and -complement are mutually exclusive\n");
    return 1;
  }

  if (is_union && is_equal) {
    fprintf(stderr, "Error: -union and -equal are mutually exclusive\n");
    return 1;
  }

  if (is_intersection && is_equal) {
    fprintf(stderr, "Error: -intersection and -equal are mutually exclusive\n");
    return 1;
  }

  if (is_subtract && is_equal) {
    fprintf(stderr, "Error: -subtract and -equal are mutually exclusive\n");
    return 1;
  }

  if (is_complement && is_equal) {
    fprintf(stderr, "Error: -complement and -equal are mutually exclusive\n");
    return 1;
  }

  if (is_test_isec) {
    if (file_cnt != 0 || has_gdnf) {
      fprintf(stderr, "Error: -test-isec benchmark cannot be combined with other DNF inputs (files or -gdnf)\n");
      return 1;
    }
  }

  if (is_union || is_intersection || is_subtract || is_equal) {
    if (file_cnt + (has_gdnf ? 1 : 0) != 2) {
      fprintf(stderr, "Error: Pairwise operations (-union, -intersection, -subtract or -equal) require exactly two input DNFs (files or generated)\n");
      return 1;
    }
  }

  if (is_complement || is_cofactor || is_check_universal || is_test) {
    if (file_cnt + (has_gdnf ? 1 : 0) != 1) {
      fprintf(stderr, "Error: Unary operations require exactly one input DNF (file or generated)\n");
      return 1;
    }
  }

  /* Generate or import PSD */
  co psd = NULL;
  if (ipsd_path != NULL) {
    FILE *ipsd_f = fopen(ipsd_path, "rb");
    if (ipsd_f == NULL) {
      perror(ipsd_path);
      return 1;
    }
    psd = coReadJSONByFP(ipsd_f);
    fclose(ipsd_f);
    if (psd == NULL) {
      fprintf(stderr, "Error: Failed to parse initial PSD from %s\n", ipsd_path);
      return 1;
    }
    if (!coIsMap(psd) || coMapGet(psd, "psd") == NULL || !coIsMap(coMapGet(psd, "psd"))) {
      fprintf(stderr, "Error: Invalid PSD file format in %s\n", ipsd_path);
      coDelete(psd);
      return 1;
    }
    /* Recursively convert loaded lists in the PSD to Int32Vectors */
    psd = coConvertToInt32Vector(psd);
  } else if (has_gpsd) {
    psd = coNewPSD();
    if (psd == NULL) {
      fprintf(stderr, "Error: Failed to create PSD object\n");
      return 1;
    }
    co psd_inner = (co)coMapGet(psd, "psd");
    long attr_i;
    for (attr_i = 0; attr_i < gpsd_attrs; attr_i++) {
      char attr_name[32];
      sprintf(attr_name, "%ld", attr_i);
      
      co val_vec = coNewInt32Vector(CO_NONE);
      if (val_vec == NULL) {
        coDelete(psd);
        return 1;
      }
      
      long val_v;
      for (val_v = 0; val_v < gpsd_vals; val_v++) {
        if (coInt32VectorAddUnique(val_vec, (int32_t)val_v) < 0) {
          coDelete(val_vec);
          coDelete(psd);
          return 1;
        }
      }
      
      if (coMapAdd(psd_inner, attr_name, val_vec) == NULL) {
        coDelete(val_vec);
        coDelete(psd);
        return 1;
      }
    }
  } else {
    psd = coNewPSD();
    if (psd == NULL) {
      fprintf(stderr, "Error: Failed to create PSD object\n");
      return 1;
    }
  }

  /* Load, convert, validate, and extend with any input files */
  co loaded_files[128];
  int i_file;
  for (i_file = 0; i_file < file_cnt; i_file++) {
    FILE *f = fopen(files[i_file], "rb");
    if (f == NULL) {
      perror(files[i_file]);
      int k;
      for (k = 0; k < i_file; k++) {
        coDelete(loaded_files[k]);
      }
      coDelete(psd);
      return 1;
    }
    co arg = coReadJSONByFP(f);
    fclose(f);

    if (arg == NULL) {
      fprintf(stderr, "Error: Failed to parse JSON from %s\n", files[i_file]);
      int k;
      for (k = 0; k < i_file; k++) {
        coDelete(loaded_files[k]);
      }
      coDelete(psd);
      return 1;
    }

    arg = coConvertToInt32Vector(arg);
    if (!coDNFIsValid(arg)) {
      fprintf(stderr, "Error: %s does not have a valid DNF structure\n", files[i_file]);
      coDelete(arg);
      int k;
      for (k = 0; k < i_file; k++) {
        coDelete(loaded_files[k]);
      }
      coDelete(psd);
      return 1;
    }

    loaded_files[i_file] = arg;

    /* Extend automatically */
    coPSDExtendByDNF(psd, arg);
  }

  /* Generate random DNF if requested and add to loaded_files */
  if (has_gdnf) {
    co psd_inner = (co)coMapGet(psd, "psd");
    if (psd_inner == NULL || !coIsMap(psd_inner) || coMapSize(psd_inner) == 0) {
      fprintf(stderr, "Error: No attributes found in the PSD space. Cannot generate random DNF.\n");
      int k;
      for (k = 0; k < file_cnt; k++) {
        coDelete(loaded_files[k]);
      }
      coDelete(psd);
      return 1;
    }

    /* Collect all available attributes from psd_inner */
    const char *avail_attrs[2048];
    int avail_attrs_cnt = 0;
    coMapIterator iter_a;
    if (coMapLoopFirst(&iter_a, psd_inner)) {
      do {
        if (avail_attrs_cnt < 2048) {
          avail_attrs[avail_attrs_cnt++] = coMapLoopKey(&iter_a);
        }
      } while (coMapLoopNext(&iter_a));
    }

    co random_dnf = coNewVector(CO_FREE_VALS);
    if (random_dnf == NULL) {
      fprintf(stderr, "Error: Failed to create result DNF vector\n");
      int k;
      for (k = 0; k < file_cnt; k++) {
        coDelete(loaded_files[k]);
      }
      coDelete(psd);
      return 1;
    }


    long term_idx;
    for (term_idx = 0; term_idx < gdnf_terms; term_idx++) {
      co and_term = coNewMap(CO_STRDUP | CO_FREE_VALS);
      if (and_term == NULL) {
        int k;
        for (k = 0; k < file_cnt; k++) {
          coDelete(loaded_files[k]);
        }
        coDelete(psd);
        coDelete(random_dnf);
        return 1;
      }

      /* Shuffle and select gdnf_attrs from avail_attrs */
      long actual_attrs_to_select = (gdnf_attrs < avail_attrs_cnt) ? gdnf_attrs : avail_attrs_cnt;
      const char *temp_attrs[2048];
      memcpy(temp_attrs, avail_attrs, avail_attrs_cnt * sizeof(char*));
      
      long step;
      for (step = 0; step < actual_attrs_to_select; step++) {
        long r_idx = step + (rand() % (avail_attrs_cnt - step));
        const char *swap = temp_attrs[step];
        temp_attrs[step] = temp_attrs[r_idx];
        temp_attrs[r_idx] = swap;
      }

      /* For each selected attribute, select gdnf_vals from its available values */
      for (step = 0; step < actual_attrs_to_select; step++) {
        cco psd_vec = coMapGet(psd_inner, temp_attrs[step]);
        assert(psd_vec != NULL && coIsInt32Vector(psd_vec));
        
        long avail_vals_cnt = coInt32VectorSize(psd_vec);
        if (avail_vals_cnt == 0) {
          continue; /* Skip if no values are in the PSD vector */
        }

        /* Collect values */
        int32_t *temp_vals = malloc(avail_vals_cnt * sizeof(int32_t));
        if (temp_vals == NULL) {
          int k;
          for (k = 0; k < file_cnt; k++) {
            coDelete(loaded_files[k]);
          }
          coDelete(psd);
          coDelete(and_term);
          coDelete(random_dnf);
          return 1;
        }
        
        long val_v;
        for (val_v = 0; val_v < avail_vals_cnt; val_v++) {
          temp_vals[val_v] = coInt32VectorGet(psd_vec, val_v);
        }

        long actual_vals_to_select = (gdnf_vals < avail_vals_cnt) ? gdnf_vals : avail_vals_cnt;
        long val_idx;
        for (val_idx = 0; val_idx < actual_vals_to_select; val_idx++) {
          long r_idx = val_idx + (rand() % (avail_vals_cnt - val_idx));
          int32_t swap = temp_vals[val_idx];
          temp_vals[val_idx] = temp_vals[r_idx];
          temp_vals[r_idx] = swap;
        }

        co val_vec_new = coNewInt32Vector(CO_NONE);
        if (val_vec_new == NULL) {
          free(temp_vals);
          int k;
          for (k = 0; k < file_cnt; k++) {
            coDelete(loaded_files[k]);
          }
          coDelete(psd);
          coDelete(and_term);
          coDelete(random_dnf);
          return 1;
        }

        for (val_idx = 0; val_idx < actual_vals_to_select; val_idx++) {
          coInt32VectorAddUnique(val_vec_new, temp_vals[val_idx]);
        }

        free(temp_vals);

        if (coMapAdd(and_term, temp_attrs[step], val_vec_new) == NULL) {
          coDelete(val_vec_new);
          int k;
          for (k = 0; k < file_cnt; k++) {
            coDelete(loaded_files[k]);
          }
          coDelete(psd);
          coDelete(and_term);
          coDelete(random_dnf);
          return 1;
        }
      }

      if (coVectorAdd(random_dnf, and_term) < 0) {
        coDelete(and_term);
        int k;
        for (k = 0; k < file_cnt; k++) {
          coDelete(loaded_files[k]);
        }
        coDelete(psd);
        coDelete(random_dnf);
        return 1;
      }
    }
    
    if (file_cnt < 128) {
      loaded_files[file_cnt++] = random_dnf;
    } else {
      fprintf(stderr, "Error: Cannot add generated DNF, max file limit reached\n");
      coDelete(random_dnf);
      int k;
      for (k = 0; k < file_cnt; k++) {
        coDelete(loaded_files[k]);
      }
      coDelete(psd);
      return 1;
    }
  }

  /* Verbose output of input files and commands */
  if (verbose) {
    for (i_file = 0; i_file < file_cnt; i_file++) {
      if (has_gdnf && i_file == file_cnt - 1) {
        printf("--- Generated Random DNF ---\n");
      } else {
        printf("--- Input Argument %d (%s) ---\n", i_file + 1, files[i_file]);
      }
      coWriteJSON(loaded_files[i_file], 0, 0, stdout);
      printf("\n\n");
    }
    if (is_union) {
      printf("Command: coDNFUnion(arg1, arg2)\n\n");
    } else if (is_intersection) {
      printf("Command: coDNFIntersection(arg1, arg2)\n\n");
    } else if (is_subtract) {
      printf("Command: coNewDNFBySubtraction(psd, arg1, arg2)\n\n");
    } else if (is_complement) {
      printf("Command: coDNFComplementBySubtract(psd, arg1)\n\n");
    } else if (is_equal) {
      printf("Command: coDNFIsEqual(psd, arg1, arg2)\n\n");
    } else if (is_cofactor) {
      printf("Command: coDNFNewCofactor(psd, arg1, \"%s\", %d)\n\n", cofactor_attr, cofactor_val);
    } else if (is_check_universal) {
      printf("Command: coDNFCheckUniversal(psd, arg1)\n\n");
    } else if (is_test) {
      printf("Command: Internal consistency tests\n\n");
    }
  }

  co result_dnf = NULL;
  
  if (has_gdnf && !is_union && !is_intersection && !is_subtract && !is_complement && !is_equal && !is_cofactor && !is_check_universal && !is_test) {
    result_dnf = loaded_files[file_cnt - 1];
  }

  /* Universal save logic for arg1 and arg2 (excluding benchmark which has its own) */
  if (!is_test_isec) {
    co arg1 = (file_cnt > 0) ? loaded_files[0] : NULL;
    co arg2 = (file_cnt > 1) ? loaded_files[1] : NULL;
    if (o1_path != NULL && arg1 != NULL) {
      FILE *f1 = fopen(o1_path, "w");
      if (f1) { coWriteJSON(arg1, 0, 0, f1); fprintf(f1, "\n"); fclose(f1); }
      else perror(o1_path);
    }
    if (o2_path != NULL && arg2 != NULL) {
      FILE *f2 = fopen(o2_path, "w");
      if (f2) { coWriteJSON(arg2, 0, 0, f2); fprintf(f2, "\n"); fclose(f2); }
      else perror(o2_path);
    }
  }

  /* Execute operation if requested */
  if (is_union || is_intersection || is_subtract || is_complement || is_equal || is_cofactor || is_check_universal || is_test || is_test_isec) {
    co arg1 = (file_cnt > 0) ? loaded_files[0] : NULL;
    co arg2 = (file_cnt > 1) ? loaded_files[1] : NULL;

    if (is_equal) {
      if (coDNFIsEqual(psd, arg1, arg2)) {
        printf("equal\n");
      } else {
        printf("not equal\n");
      }
      for (i_file = 0; i_file < file_cnt; i_file++) {
        coDelete(loaded_files[i_file]);
      }
      coDelete(psd);
      return 0;
    } else if (is_test) {
      double t1, t2;
      
      printf("Pre-step: Calculate cdnf = complement(arg1)\n");
      t1 = get_ms();
      co cdnf = coDNFComplementBySubtract(psd, arg1);
      t2 = get_ms();
      printf("  coDNFComplementBySubtract(arg1:%ld) -> cdnf:%ld (%.2f ms)\n\n", 
             coVectorSize(arg1), coVectorSize(cdnf), t2 - t1);
      
      /* Test 1: intersection(cdnf, dnf) must be empty */
      printf("Test 1: intersection(cdnf, dnf) must be empty\n");
      t1 = get_ms();
      co isec = coNewDNFByIntersection(psd, cdnf, arg1);
      t2 = get_ms();
      printf("  coNewDNFByIntersection(psd, cdnf:%ld, arg1:%ld) -> isec:%ld (%.2f ms)\n",
             coVectorSize(cdnf), coVectorSize(arg1), coVectorSize(isec), t2 - t1);
      printf("  Result: %s\n\n", coDNFIsEmpty(isec) ? "PASS" : "FAIL");
      coDelete(isec);

      /* Test 2: union(cdnf, dnf) must be universal with shannon expansion */
      printf("Test 2: union(cdnf, dnf) must be universal via Shannon Expansion\n");
      co u = coClone(cdnf);
      coDNFUnion(psd, u, arg1);
      t1 = get_ms();
      int res2 = coDNFCheckUniversal(psd, u);
      t2 = get_ms();
      printf("  coDNFCheckUniversal(u:%ld) -> %d (%.2f ms)\n", 
             coVectorSize(u), res2, t2 - t1);
      printf("  Result: %s\n\n", res2 ? "PASS" : "FAIL");

      /* Test 3: union(cdnf, dnf) must be universal with complement */
      printf("Test 3: union(cdnf, dnf) must be universal via Complement\n");
      t1 = get_ms();
      int res3 = coDNFCheckUniversalByComplement(psd, u);
      t2 = get_ms();
      printf("  coDNFCheckUniversalByComplement(u:%ld) -> %d (%.2f ms)\n",
             coVectorSize(u), res3, t2 - t1);
      printf("  Result: %s\n\n", res3 ? "PASS" : "FAIL");
      coDelete(u);

      /* Test 4: Original DNF Universal check agreement on dnf */
      printf("Test 4: Original DNF Universal check agreement (Shannon vs Complement)\n");
      t1 = get_ms();
      int u1 = coDNFCheckUniversal(psd, arg1);
      t2 = get_ms();
      printf("  coDNFCheckUniversal(arg1:%ld) -> %d (%.2f ms)\n", coVectorSize(arg1), u1, t2 - t1);
      
      t1 = get_ms();
      int u2 = coDNFCheckUniversalByComplement(psd, arg1);
      t2 = get_ms();
      printf("  coDNFCheckUniversalByComplement(arg1:%ld) -> %d (%.2f ms)\n", coVectorSize(arg1), u2, t2 - t1);
      printf("  Result: %s\n\n", (u1 == u2) ? "PASS" : "FAIL");

      /* Test 5: Complement universal check agreement on cdnf */
      printf("Test 5: Complement DNF Universal check agreement (Shannon vs Complement)\n");
      t1 = get_ms();
      int cu1 = coDNFCheckUniversal(psd, cdnf);
      t2 = get_ms();
      printf("  coDNFCheckUniversal(cdnf:%ld) -> %d (%.2f ms)\n", coVectorSize(cdnf), cu1, t2 - t1);
      
      t1 = get_ms();
      int cu2 = coDNFCheckUniversalByComplement(psd, cdnf);
      t2 = get_ms();
      printf("  coDNFCheckUniversalByComplement(cdnf:%ld) -> %d (%.2f ms)\n", coVectorSize(cdnf), cu2, t2 - t1);
      printf("  Result: %s\n\n", (cu1 == cu2) ? "PASS" : "FAIL");

      /* Test 6: ccdnf == dnf */
      printf("Test 6: Double complement equals original\n");
      t1 = get_ms();
      co ccdnf = coDNFComplementBySubtract(psd, cdnf);
      t2 = get_ms();
      printf("  coDNFComplementBySubtract(cdnf:%ld) -> ccdnf:%ld (%.2f ms)\n", 
             coVectorSize(cdnf), coVectorSize(ccdnf), t2 - t1);
      
      t1 = get_ms();
      int eq_res = coDNFIsEqual(psd, ccdnf, arg1);
      t2 = get_ms();
      printf("  coDNFIsEqual(ccdnf:%ld, arg1:%ld) -> %d (%.2f ms)\n",
             coVectorSize(ccdnf), coVectorSize(arg1), eq_res, t2 - t1);
      printf("  Result: %s\n\n", eq_res ? "PASS" : "FAIL");
      
      coDelete(ccdnf);
      coDelete(cdnf);
      for (i_file = 0; i_file < file_cnt; i_file++) {
        coDelete(loaded_files[i_file]);
      }
      coDelete(psd);
      return 0;
    } else if (is_test_isec) {
      const char *simd_name = "Scalar uint64_t";
      if (co_bv_base_size == 16) simd_name = "SSE2 128-bit";
      else if (co_bv_base_size == 32) simd_name = "AVX2 256-bit";
      else if (co_bv_base_size == 64) simd_name = "AVX-512 512-bit";
      printf("Benchmark using Bitset Base Type: %s\n", simd_name);

      co psd_inner = (co)coMapGet(psd, "psd");
      if (coMapSize(psd_inner) < 2) {
        fprintf(stderr, "Error: -test-isec requires at least 2 attributes in the PSD (use -gpsd 2 3 or similar)\n");
        for (i_file = 0; i_file < file_cnt; i_file++) coDelete(loaded_files[i_file]);
        coDelete(psd);
        return 1;
      }

      /* Collect attributes from psd */
      const char *attrs[2048];
      int attrs_cnt = 0;
      coMapIterator it_a;
      if (coMapLoopFirst(&it_a, psd_inner)) {
        do { if (attrs_cnt < 2048) attrs[attrs_cnt++] = coMapLoopKey(&it_a); } while (coMapLoopNext(&it_a));
      }

      int b;
      for (b = 0; b < 2; b++) {
        co dnf_gen = coNewVector(CO_FREE_VALS);
        long t;
        for (t = 0; t < test_isec_terms; t++) {
          co term = coNewMap(CO_STRDUP | CO_FREE_VALS);
          int a_idx;
          for (a_idx = 0; a_idx < attrs_cnt; a_idx++) {
            if (b == 0 && a_idx == 1) continue; /* Arg1 excludes 2nd attribute */
            if (b == 1 && a_idx == 0) continue; /* Arg2 excludes 1st attribute */
            
            cco domain = coMapGet(psd_inner, attrs[a_idx]);
            co vals = coNewInt32Vector(CO_NONE);
            long domain_size = coInt32VectorSize(domain);
            long to_pick = (test_isec_vals < domain_size) ? test_isec_vals : domain_size;
            
            int32_t *v_pool = malloc(domain_size * sizeof(int32_t));
            long vi;
            for (vi = 0; vi < domain_size; vi++) v_pool[vi] = coInt32VectorGet(domain, vi);
            for (vi = 0; vi < to_pick; vi++) {
              long r = vi + (rand() % (domain_size - vi));
              int32_t swap = v_pool[vi]; v_pool[vi] = v_pool[r]; v_pool[r] = swap;
              coInt32VectorAddUnique(vals, v_pool[vi]);
            }
            free(v_pool);
            coMapAdd(term, attrs[a_idx], vals);
          }
          coVectorAdd(dnf_gen, term);
        }
        loaded_files[file_cnt++] = dnf_gen;
      }

      co arg1 = loaded_files[0];
      co arg2 = loaded_files[1];

      if (o1_path != NULL) {
        FILE *f1 = fopen(o1_path, "w");
        if (f1) { coWriteJSON(arg1, 0, 0, f1); fprintf(f1, "\n"); fclose(f1); }
        else perror(o1_path);
      }
      if (o2_path != NULL) {
        FILE *f2 = fopen(o2_path, "w");
        if (f2) { coWriteJSON(arg2, 0, 0, f2); fprintf(f2, "\n"); fclose(f2); }
        else perror(o2_path);
      }

      double t1, t2;
      t1 = get_ms();
      co result = coNewDNFByIntersectionWithoutMinimization(psd, arg1, arg2);
      t2 = get_ms();
      printf("  coNewDNFByIntersectionWithoutMinimization(arg1:%ld, arg2:%ld) -> result:%ld (%.2f ms)\n", 
             coVectorSize(arg1), coVectorSize(arg2), coVectorSize(result), t2 - t1);

      /* BV Comparison Path */
      coBVPreparePSD(psd);
      co bv_arg1 = coNewBVDNFFromDNF(psd, arg1);
      co bv_arg2 = coNewBVDNFFromDNF(psd, arg2);
      t1 = get_ms();
      co bv_result = coNewBVDNFByIntersectionWithoutMinimization(psd, bv_arg1, bv_arg2);
      t2 = get_ms();
      co dnf_bv_result = coNewDNFFromBVDNF(psd, bv_result);
      printf("  coNewBVDNFByIntersectionWithoutMinimization(arg1:%ld, arg2:%ld) -> result:%ld (%.2f ms)\n",
             coVectorSize(bv_arg1), coVectorSize(bv_arg2), coVectorSize(bv_result), t2 - t1);
      
      int bv_eq = coDNFIsEqual(psd, result, dnf_bv_result);
      printf("  BVDNF vs DNF Equality: %s\n", bv_eq ? "PASS" : "FAIL");
      coDelete(dnf_bv_result);

      /* Subset Minimization Path (First Step after intersection) */
      long before_subset = coVectorSize(result);
      t1 = get_ms();
      coDNFMinimizeANDTermSubset(result);
      t2 = get_ms();
      printf("  coDNFMinimizeANDTermSubset(result:%ld) -> result:%ld (%.2f ms)\n", 
             before_subset, coVectorSize(result), t2 - t1);

      long bv_before_subset = coVectorSize(bv_result);
      t1 = get_ms();
      coBVDNFMinimizeANDTermSubset(bv_result);
      t2 = get_ms();
      co dnf_bv_subset = coNewDNFFromBVDNF(psd, bv_result);
      printf("  coBVDNFMinimizeANDTermSubset(result:%ld) -> result:%ld (%.2f ms)\n", 
             bv_before_subset, coVectorSize(bv_result), t2 - t1);
      int bv_subset_eq = coDNFIsEqual(psd, result, dnf_bv_subset);
      printf("  BVDNF vs DNF Equality (ANDTermSubset): %s\n", bv_subset_eq ? "PASS" : "FAIL");
      coDelete(dnf_bv_subset);

      co bv_result_raw = coClone(bv_result);

      co result_raw = coClone(result);
      
      printf("  Volumes: arg1:%d, arg2:%d, result_raw:%d\n", 
             coDNFGetVolume(psd, arg1), coDNFGetVolume(psd, arg2), coDNFGetVolume(psd, result_raw));

      long before_clear = coVectorSize(result);
      t1 = get_ms();
      coDNFMinimizeClearFullDomain(psd, result);
      coDNFMinimizeANDTermSubset(result);
      t2 = get_ms();
      printf("  coDNFMinimizeClearFullDomain(result:%ld) -> result:%ld (%.2f ms)\n", 
             before_clear, coVectorSize(result), t2 - t1);

      /* BV ClearFullDomain Path */
      long bv_before_clear = coVectorSize(bv_result);
      t1 = get_ms();
      coBVDNFMinimizeClearFullDomain(psd, bv_result);
      coBVDNFMinimizeANDTermSubset(bv_result);
      t2 = get_ms();
      co dnf_bv_clear = coNewDNFFromBVDNF(psd, bv_result);
      printf("  coBVDNFMinimizeClearFullDomain(result:%ld) -> result:%ld (%.2f ms)\n", 
             bv_before_clear, coVectorSize(bv_result), t2 - t1);
      int bv_clear_eq = coDNFIsEqual(psd, result, dnf_bv_clear);
      printf("  BVDNF vs DNF Equality (ClearFullDomain): %s\n", bv_clear_eq ? "PASS" : "FAIL");
      coDelete(dnf_bv_clear);

      long before_merge = coVectorSize(result);
      t1 = get_ms();
      coDNFMinimizeByANDTermMerge(result);
      coDNFMinimizeANDTermSubset(result);
      t2 = get_ms();
      printf("  coDNFMinimizeByANDTermMerge(result:%ld) -> result:%ld (%.2f ms)\n", 
             before_merge, coVectorSize(result), t2 - t1);

      /* BV TermMerge Path */
      long bv_before_merge = coVectorSize(bv_result);
      t1 = get_ms();
      coBVDNFMinimizeByANDTermMerge(psd, bv_result);
      coBVDNFMinimizeANDTermSubset(bv_result);
      t2 = get_ms();
      co dnf_bv_merge = coNewDNFFromBVDNF(psd, bv_result);
      printf("  coBVDNFMinimizeByANDTermMerge(result:%ld) -> result:%ld (%.2f ms)\n", 
             bv_before_merge, coVectorSize(bv_result), t2 - t1);
      int bv_merge_eq = coDNFIsEqual(psd, result, dnf_bv_merge);
      printf("  BVDNF vs DNF Equality (TermMerge): %s\n", bv_merge_eq ? "PASS" : "FAIL");
      coDelete(dnf_bv_merge);

      t1 = get_ms();
      int eq = coDNFIsEqual(psd, result_raw, result);
      t2 = get_ms();
      printf("  coDNFIsEqual(result_raw:%ld, result:%ld) -> %s (%.2f ms)\n",
             coVectorSize(result_raw), coVectorSize(result), eq ? "PASS" : "FAIL", t2 - t1);

      /* BV IsEqual Path */
      t1 = get_ms();
      int bv_eq_res = coBVDNFIsEqual(psd, bv_result_raw, bv_result);
      t2 = get_ms();
      printf("  coBVDNFIsEqual(result_raw:%ld, result:%ld) -> %s (%.2f ms)\n",
             coVectorSize(bv_result_raw), coVectorSize(bv_result), bv_eq_res ? "PASS" : "FAIL", t2 - t1);

      printf("  Final volume: %d\n", coDNFGetVolume(psd, result));

      coDelete(bv_arg1);
      coDelete(bv_arg2);
      coDelete(bv_result);
      coDelete(bv_result_raw);
      coDelete(result_raw);
      
      printf("  ---\n");
      t1 = get_ms();
      co result2 = coNewDNFByIntersection(psd, arg1, arg2);
      t2 = get_ms();
      printf("  coNewDNFByIntersection(psd, arg1:%ld, arg2:%ld) -> result:%ld (%.2f ms)\n", 
             coVectorSize(arg1), coVectorSize(arg2), coVectorSize(result2), t2 - t1);

      /* BV Automatic Intersection Path */
      co bv_arg1_2 = coNewBVDNFFromDNF(psd, arg1);
      co bv_arg2_2 = coNewBVDNFFromDNF(psd, arg2);
      t1 = get_ms();
      co bv_result2 = coNewBVDNFByIntersection(psd, bv_arg1_2, bv_arg2_2);
      t2 = get_ms();
      co dnf_bv_result2 = coNewDNFFromBVDNF(psd, bv_result2);
      printf("  coNewBVDNFByIntersection(psd, arg1:%ld, arg2:%ld) -> result:%ld (%.2f ms)\n",
             coVectorSize(bv_arg1_2), coVectorSize(bv_arg2_2), coVectorSize(bv_result2), t2 - t1);
      
      int bv_eq2 = coDNFIsEqual(psd, result2, dnf_bv_result2);
      printf("  BVDNF vs DNF Equality (Intersection): %s\n", bv_eq2 ? "PASS" : "FAIL");
      coDelete(dnf_bv_result2);

      long before_clear2 = coVectorSize(result2);
      t1 = get_ms();
      coDNFMinimizeClearFullDomain(psd, result2);
      coDNFMinimizeANDTermSubset(result2);
      t2 = get_ms();
      printf("  coDNFMinimizeClearFullDomain(result:%ld) -> result:%ld (%.2f ms)\n", 
             before_clear2, coVectorSize(result2), t2 - t1);

      /* BV ClearFullDomain Path 2 */
      long bv_before_clear2 = coVectorSize(bv_result2);
      t1 = get_ms();
      coBVDNFMinimizeClearFullDomain(psd, bv_result2);
      coBVDNFMinimizeANDTermSubset(bv_result2);
      t2 = get_ms();
      co dnf_bv_clear2 = coNewDNFFromBVDNF(psd, bv_result2);
      printf("  coBVDNFMinimizeClearFullDomain(result:%ld) -> result:%ld (%.2f ms)\n", 
             bv_before_clear2, coVectorSize(bv_result2), t2 - t1);
      int bv_clear_eq2 = coDNFIsEqual(psd, result2, dnf_bv_clear2);
      printf("  BVDNF vs DNF Equality (ClearFullDomain 2): %s\n", bv_clear_eq2 ? "PASS" : "FAIL");
      coDelete(dnf_bv_clear2);

      long before_merge2 = coVectorSize(result2);
      t1 = get_ms();
      coDNFMinimizeByANDTermMerge(result2);
      coDNFMinimizeANDTermSubset(result2);
      t2 = get_ms();
      printf("  coDNFMinimizeByANDTermMerge(result:%ld) -> result:%ld (%.2f ms)\n", 
             before_merge2, coVectorSize(result2), t2 - t1);

      /* BV TermMerge Path 2 */
      long bv_before_merge2 = coVectorSize(bv_result2);
      t1 = get_ms();
      coBVDNFMinimizeByANDTermMerge(psd, bv_result2);
      coBVDNFMinimizeANDTermSubset(bv_result2);
      t2 = get_ms();
      co dnf_bv_merge2 = coNewDNFFromBVDNF(psd, bv_result2);
      printf("  coBVDNFMinimizeByANDTermMerge(result:%ld) -> result:%ld (%.2f ms)\n", 
             bv_before_merge2, coVectorSize(bv_result2), t2 - t1);
      int bv_merge_eq2 = coDNFIsEqual(psd, result2, dnf_bv_merge2);
      printf("  BVDNF vs DNF Equality (TermMerge 2): %s\n", bv_merge_eq2 ? "PASS" : "FAIL");
      coDelete(dnf_bv_merge2);

      t1 = get_ms();
      int eq2 = coDNFIsEqual(psd, result, result2);
      t2 = get_ms();
      printf("  coDNFIsEqual(manual:%ld, automatic:%ld) -> %s (%.2f ms)\n",
             coVectorSize(result), coVectorSize(result2), eq2 ? "PASS" : "FAIL", t2 - t1);
      printf("  Final volume: %d\n", coDNFGetVolume(psd, result2));

      coDelete(bv_arg1_2);
      coDelete(bv_arg2_2);
      coDelete(bv_result2);
      coDelete(result2);
      result_dnf = result;
      needs_delete = 1;
    } else if (is_union) {
      if (coDNFUnion(psd, arg1, arg2) == 0) {
        fprintf(stderr, "Error: Union operation failed\n");
        int k;
        for (k = 0; k < file_cnt; k++) {
          coDelete(loaded_files[k]);
        }
        coDelete(psd);
        return 1;
      }
      result_dnf = arg1; /* Mutated in-place */
    } else if (is_intersection) {
      if (coDNFIntersection(psd, arg1, arg2) == 0) {
        fprintf(stderr, "Error: Intersection operation failed\n");
        int k;
        for (k = 0; k < file_cnt; k++) {
          coDelete(loaded_files[k]);
        }
        coDelete(psd);
        return 1;
      }
      result_dnf = arg1; /* Mutated in-place */
    } else if (is_subtract) {
      result_dnf = coNewDNFBySubtraction(psd, arg1, arg2);
      if (result_dnf == NULL) {
        fprintf(stderr, "Error: Subtraction operation failed\n");
        int k;
        for (k = 0; k < file_cnt; k++) {
          coDelete(loaded_files[k]);
        }
        coDelete(psd);
        return 1;
      }
      needs_delete = 1;
    } else if (is_complement) {
      result_dnf = coDNFComplementBySubtract(psd, arg1);
      if (result_dnf == NULL) {
        fprintf(stderr, "Error: Complement operation failed\n");
        int k;
        for (k = 0; k < file_cnt; k++) {
          coDelete(loaded_files[k]);
        }
        coDelete(psd);
        return 1;
      }
      needs_delete = 1;
    } else if (is_cofactor) {
      result_dnf = coDNFNewCofactor(psd, arg1, cofactor_attr, cofactor_val);
      if (result_dnf == NULL) {
        fprintf(stderr, "Error: Cofactor operation failed\n");
        int k;
        for (k = 0; k < file_cnt; k++) {
          coDelete(loaded_files[k]);
        }
        coDelete(psd);
        return 1;
      }
      needs_delete = 1;
    }
  }

  /* Output Result DNF */
  if (result_dnf != NULL && (o_path != NULL || !is_test_isec)) {
    FILE *out_f = stdout;
    if (o_path != NULL) {
      out_f = fopen(o_path, "w");
      if (out_f == NULL) {
        perror(o_path);
        out_f = stdout;
      }
    }

    if (verbose) {
      printf("--- Result DNF ---\n");
    }
    coWriteJSON(result_dnf, 0, 0, out_f);
    fprintf(out_f, "\n");
    if (out_f != stdout) {
      fclose(out_f);
    }
  }

  /* Output PSD if requested */
  if (show_psd) {
    if (verbose) {
      printf("\n--- PSD Content ---\n");
    } else if (result_dnf != NULL) {
      printf("\n");
    }
    coWriteJSON(psd, 0, 0, stdout);
    printf("\n");
  }

  /* Output PSD to a file if requested */
  if (opsd_path != NULL) {
    FILE *opsd_f = fopen(opsd_path, "w");
    if (opsd_f == NULL) {
      perror(opsd_path);
    } else {
      coWriteJSON(psd, 0, 0, opsd_f);
      fprintf(opsd_f, "\n");
      fclose(opsd_f);
    }
  }

  /* Cleanup */
  for (i_file = 0; i_file < file_cnt; i_file++) {
    coDelete(loaded_files[i_file]);
  }
  if (result_dnf != NULL && needs_delete) {
    coDelete(result_dnf);
  }
  coDelete(psd);
  return 0;
}
