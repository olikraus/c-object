#ifndef DNFWEBAPP_H
#define DNFWEBAPP_H

#define _GNU_SOURCE
#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#endif

typedef struct {
    char *buffer;       /* Raw JSON content. */
    size_t size;        /* Size of the raw JSON content. */
    uint32_t *pos_array;/* Array of structural character positions. */
    size_t pos_cnt;     /* Number of structural characters found. */
    size_t pos_max;     /* Allocated size of pos_array. */
    size_t token_idx;   /* Current token index during recursive parsing. */
    int verbose;        /* Output verbosity flag. */
    const char *out_filename; /* Output filename for operation results. */
    
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

    char *log_buffer;
    size_t log_size;
    size_t log_max;
    double start_time;
} dwa_t;

/* dwa.c */
double get_ms(void);
void dwa_print(dwa_t *p, const char *fmt, ...);
void dwa_init(dwa_t *p);
void dwa_destroy(dwa_t *p);
void dwa_ensure_capacity(dwa_t *p);
void dwa_error(dwa_t *p, const char *msg);

/* dwa_json.c */
int dwa_read_file(dwa_t *p, const char *filename);
void dwa_scan(dwa_t *p);
char dwa_peek_token_char(dwa_t *p);
int dwa_has_content_between_tokens(dwa_t *p);
char *dwa_alloc_string(dwa_t *p);
void dwa_skip_string(dwa_t *p);
int32_t dwa_parse_int(dwa_t *p);

/* dwa_op.c */
void dwa_collect_psd_recursive(dwa_t *p);
void dwa_build_bvdnf_recursive(dwa_t *p, co *target_list);
void dwa_process_op_json(dwa_t *p);
void dwa_execute_op(dwa_t *p);

/* dwa_verify.c */
void dwa_validate(dwa_t *p);

/* dwa_cli.c */
int dwa_cli(int argc, char **argv);

#endif
