#include "dnfwebapp.h"

/* Local helper: not in header */
static char dwa_consume_token_char(dwa_t *p) {
    if (p->token_idx >= p->pos_cnt) return '\0';
    return p->buffer[p->pos_array[p->token_idx++]];
}

/* Phase 1: PSD Collection */

static void dwa_collect_psd_dnf(dwa_t *p) {
    if (dwa_consume_token_char(p) != '[') dwa_error(p, "Expected '[' for DNF");
    if (dwa_peek_token_char(p) == ']') {
        dwa_consume_token_char(p);
        return;
    }
    for (;;) {
        if (dwa_consume_token_char(p) != '{') dwa_error(p, "Expected '{' for AND-term");
        if (dwa_peek_token_char(p) != '}') {
            for (;;) {
                char *attr_name = dwa_alloc_string(p);
                if (dwa_consume_token_char(p) != ':') dwa_error(p, "Expected ':'");
                if (dwa_consume_token_char(p) != '[') dwa_error(p, "Expected '['");
                
                if (dwa_peek_token_char(p) != ']') {
                    for (;;) {
                        int32_t val = dwa_parse_int(p);
                        coPSDExtendByValue(p->psd, attr_name, val);
                        char c = dwa_consume_token_char(p);
                        if (c == ']') break;
                        if (c != ',') dwa_error(p, "Expected ',' or ']'");
                    }
                } else {
                    dwa_consume_token_char(p); // consume ']'
                }
                free(attr_name);
                char c = dwa_consume_token_char(p);
                if (c == '}') break;
                if (c != ',') dwa_error(p, "Expected ',' or '}'");
            }
        } else {
            dwa_consume_token_char(p); // consume '}'
        }
        char c = dwa_consume_token_char(p);
        if (c == ']') break;
        if (c != ',') dwa_error(p, "Expected ',' or ']'");
        if (dwa_peek_token_char(p) == ']') dwa_error(p, "Trailing comma not allowed");
    }
}

void dwa_collect_psd_recursive(dwa_t *p) {
    char c = dwa_peek_token_char(p);
    if (c == '{') {
        dwa_consume_token_char(p);
        if (dwa_peek_token_char(p) == '}') {
            dwa_consume_token_char(p);
            return;
        }
        for (;;) {
            char *key = dwa_alloc_string(p);
            if (dwa_consume_token_char(p) != ':') dwa_error(p, "Expected ':'");
            if (strcmp(key, "dnf") == 0) {
                dwa_collect_psd_dnf(p);
            } else {
                dwa_collect_psd_recursive(p);
            }
            free(key);
            char c2 = dwa_consume_token_char(p);
            if (c2 == '}') break;
            if (c2 != ',') dwa_error(p, "Expected ',' or '}'");
        }
    } else if (c == '[') {
        dwa_consume_token_char(p);
        if (dwa_peek_token_char(p) == ']') {
            dwa_consume_token_char(p);
            return;
        }
        for (;;) {
            dwa_collect_psd_recursive(p);
            char c2 = dwa_consume_token_char(p);
            if (c2 == ']') break;
            if (c2 != ',') dwa_error(p, "Expected ',' or ']'");
        }
    } else if (c == '\"') {
        dwa_skip_string(p);
    } else {
        while (p->token_idx < p->pos_cnt && p->buffer[p->pos_array[p->token_idx]] != ',' && p->buffer[p->pos_array[p->token_idx]] != ']' && p->buffer[p->pos_array[p->token_idx]] != '}') p->token_idx++;
    }
}

/* Phase 3: Bitvector DNF Construction */

static co dwa_parse_bvdnf(dwa_t *p) {
    co bvdnf = coNewVector(CO_FREE_VALS);
    dwa_consume_token_char(p);
    if (dwa_peek_token_char(p) == ']') {
        dwa_consume_token_char(p);
        return bvdnf;
    }

    for (;;) {
        dwa_consume_token_char(p);
        co_BVType bv = coNewBV(p->total_bits);
        
        if (dwa_peek_token_char(p) != '}') {
            coBVSetToUniversal(p->psd, bv);
            for (;;) {
                char *attr_name = dwa_alloc_string(p);
                cco attr_meta = coMapGet(p->bvattributes, attr_name);
                int32_t attr_idx = coInt32VectorGet(attr_meta, 0);
                int32_t start_bit = coInt32VectorGet(attr_meta, 1);
                co val_map = (co)coMapGet(p->bvvaluepos, attr_name);
                
                co_BVType mask = (co_BVType)coVectorGet(p->bvmask, attr_idx);
                coBVANDNOT(bv, bv, mask); // Clear attribute bits

                dwa_consume_token_char(p); // :
                dwa_consume_token_char(p); // [
                
                if (dwa_peek_token_char(p) != ']') {
                    for (;;) {
                        int32_t val = dwa_parse_int(p);
                        char buf[32]; sprintf(buf, "%d", val);
                        cco pos_obj = coMapGet(val_map, buf);
                        if (pos_obj) {
                            int32_t local_pos = (int32_t)coDblGet(pos_obj);
                            coBVSet(bv, start_bit + local_pos);
                        }
                        
                        char c = dwa_consume_token_char(p);
                        if (c == ']') break;
                        if (c != ',') dwa_error(p, "Expected ',' or ']'");
                    }
                } else {
                    dwa_consume_token_char(p);
                }
                free(attr_name);
                char c = dwa_consume_token_char(p);
                if (c == '}') break;
                if (c != ',') dwa_error(p, "Expected ',' or '}'");
            }
        } else {
            dwa_consume_token_char(p);
            coBVSetToUniversal(p->psd, bv);
        }
        coVectorAdd(bvdnf, (cco)bv);
        char c = dwa_consume_token_char(p);
        if (c == ']') break;
        if (c != ',') dwa_error(p, "Expected ',' or ']'");
        if (dwa_peek_token_char(p) == ']') dwa_error(p, "Trailing comma not allowed");
    }
    return bvdnf;
}

void dwa_build_bvdnf_recursive(dwa_t *p, co *target_list) {
    char c = dwa_peek_token_char(p);
    if (c == '{') {
        dwa_consume_token_char(p);
        if (dwa_peek_token_char(p) == '}') {
            dwa_consume_token_char(p);
            return;
        }
        for (;;) {
            char *key = dwa_alloc_string(p);
            dwa_consume_token_char(p); // :
            if (strcmp(key, "op") == 0) {
                char *op = dwa_alloc_string(p);
                strncpy(p->op_name, op, 63);
                free(op);
            } else if (strcmp(key, "arg1") == 0) {
                p->arg1_dnf_list = coNewVector(CO_FREE_VALS);
                dwa_build_bvdnf_recursive(p, &p->arg1_dnf_list);
            } else if (strcmp(key, "arg2") == 0) {
                p->arg2_dnf_list = coNewVector(CO_FREE_VALS);
                dwa_build_bvdnf_recursive(p, &p->arg2_dnf_list);
            } else if (strcmp(key, "dnf") == 0) {
                co bvdnf = dwa_parse_bvdnf(p);
                int32_t id = 0;
                char c_peek = dwa_peek_token_char(p);
                if (c_peek == ',') {
                    dwa_consume_token_char(p);
                    char *key2 = dwa_alloc_string(p);
                    dwa_consume_token_char(p);
                    if (strcmp(key2, "id") == 0) {
                        id = dwa_parse_int(p);
                    }
                    free(key2);
                }
                co dnf_obj = coNewMap(CO_STRDUP | CO_FREE_VALS);
                coMapAdd(dnf_obj, "dnf", (cco)bvdnf);
                coMapAdd(dnf_obj, "id", (cco)coNewDbl((double)id));
                if (target_list && *target_list) coVectorAdd(*target_list, dnf_obj); else coDelete(dnf_obj);
            } else {
                dwa_build_bvdnf_recursive(p, target_list);
            }
            free(key);
            char c2 = dwa_consume_token_char(p);
            if (c2 == '}') break;
        }
    } else if (c == '[') {
        dwa_consume_token_char(p);
        if (dwa_peek_token_char(p) == ']') {
            dwa_consume_token_char(p);
            return;
        }
        for (;;) {
            dwa_build_bvdnf_recursive(p, target_list);
            char c2 = dwa_consume_token_char(p);
            if (c2 == ']') break;
        }
    } else if (c == '\"') {
        dwa_skip_string(p);
    } else {
        while (p->token_idx < p->pos_cnt && p->buffer[p->pos_array[p->token_idx]] != ',' && p->buffer[p->pos_array[p->token_idx]] != ']' && p->buffer[p->pos_array[p->token_idx]] != '}') p->token_idx++;
    }
}

/* Phase 4: Execution */

void dwa_execute_op(dwa_t *p) {
    if (!p->arg1_dnf_list || !p->arg2_dnf_list) return;
    int is_check = (strcmp(p->op_name, "intersection-check") == 0);
    co result_list = coNewVector(CO_FREE_VALS);
    long n = coVectorSize(p->arg1_dnf_list);
    long m = coVectorSize(p->arg2_dnf_list);

    dwa_print(p, "Arg1 DNF count: %ld\n", n);
    dwa_print(p, "Arg2 DNF count: %ld\n", m);
    dwa_print(p, "Total intersections to execute: %ld\n", n * m);

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
    dwa_print(p, "Op execution time: %.4f ms\n", t2 - t1);
    dwa_print(p, "Total time (excluding JSON write): %.4f ms\n", t2 - p->start_time);

    FILE *out_f = stdout;
    if (p->out_filename != NULL) {
        out_f = fopen(p->out_filename, "w");
        if (out_f == NULL) {
            perror(p->out_filename);
            out_f = stdout;
        }
    }

    dwa_print(p, "Operation '%s' result: %ld items generated.\n", p->op_name, coVectorSize(result_list));
    double t3 = get_ms();
    
    co wrapper = coNewMap(CO_STRDUP | CO_FREE_VALS);
    coMapAdd(wrapper, "result", (cco)result_list);
    coMapAdd(wrapper, "log", (cco)coNewStr(CO_STRDUP, p->log_buffer));
    
    coWriteJSON(wrapper, 0, 0, out_f);
    fprintf(out_f, "\n");
    double t4 = get_ms();
    dwa_print(p, "Write result time: %.4f ms\n", t4 - t3);

    if (out_f != stdout) fclose(out_f);
    coDelete(wrapper);
}

void dwa_process_op_json(dwa_t *p) {
    double t1 = get_ms();
    p->psd = coNewPSD();
    p->token_idx = 0;
    dwa_collect_psd_recursive(p);
    double t2 = get_ms();
    dwa_print(p, "psd parser time: %.4f ms\n", t2 - t1);

    double t3 = get_ms();
    coBVPreparePSD(p->psd);
    p->bvpos = (co)coMapGet(p->psd, "bvpos");
    p->total_bits = coInt32VectorGet(p->bvpos, coInt32VectorSize(p->bvpos) - 1);
    p->bvmask = (co)coMapGet(p->psd, "bvmask");
    p->bvattributes = (co)coMapGet(p->psd, "bvattributes");
    p->bvvaluepos = (co)coMapGet(p->psd, "bvvaluepos");
    double t4 = get_ms();
    dwa_print(p, "psd bv prep time: %.4f ms\n", t4 - t3);
    
    double t5 = get_ms();
    p->token_idx = 0;
    dwa_build_bvdnf_recursive(p, NULL);
    double t6 = get_ms();
    dwa_print(p, "dnf parser time: %.4f ms\n", t6 - t5);
    
    dwa_execute_op(p);
}
