#include "dnfwebapp.h"

double get_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void dwa_print(dwa_t *p, const char *fmt, ...) {
    va_list args;
    char *msg = NULL;
    int len;

    va_start(args, fmt);
    len = vasprintf(&msg, fmt, args);
    va_end(args);

    if (len >= 0 && msg) {
        if (p->verbose) {
            printf("%s", msg);
        }
        
        if (p->log_size + len + 1 > p->log_max) {
            p->log_max = p->log_size + len + 1024;
            p->log_buffer = (char*)realloc(p->log_buffer, p->log_max);
        }
        if (p->log_buffer) {
            memcpy(p->log_buffer + p->log_size, msg, len);
            p->log_size += len;
            p->log_buffer[p->log_size] = '\0';
        }
        free(msg);
    }
}

void dwa_init(dwa_t *p) {
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

    p->log_size = 0;
    p->log_max = 1024;
    p->log_buffer = (char*)malloc(p->log_max);
    if (p->log_buffer) p->log_buffer[0] = '\0';
    p->start_time = get_ms();
}

void dwa_destroy(dwa_t *p) {
    if (p->buffer) free(p->buffer);
    if (p->pos_array) free(p->pos_array);
    if (p->psd) coDelete(p->psd);
    if (p->arg1_dnf_list) coDelete(p->arg1_dnf_list);
    if (p->arg2_dnf_list) coDelete(p->arg2_dnf_list);
    if (p->log_buffer) free(p->log_buffer);
    memset(p, 0, sizeof(dwa_t));
}

void dwa_ensure_capacity(dwa_t *p) {
    if (p->pos_cnt + 16 > p->pos_max) {
        p->pos_max *= 2;
        p->pos_array = (uint32_t*)realloc(p->pos_array, p->pos_max * sizeof(uint32_t));
        if (!p->pos_array) {
            fprintf(stderr, "Error: Failed to reallocate structural character array\n");
            exit(1);
        }
    }
}

void dwa_error(dwa_t *p, const char *msg) {
    size_t pos = (p->token_idx < p->pos_cnt) ? p->pos_array[p->token_idx] : p->size;
    fprintf(stderr, "Parser Error at position %zu (token %zu): %s\n", pos, p->token_idx, msg);
    exit(1);
}
