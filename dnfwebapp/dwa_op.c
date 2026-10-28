#include "dnfwebapp.h"
#include <limits.h>

/* Phase 1: PSD Collection */

static int32_t dwa_intern_value(dwa_t *p, const char *attribute, const char *value, int create) {
    if (p->value_ids == NULL) {
        if (!create) return -1;
        p->value_ids = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
        if (p->value_ids == NULL) dwa_error(p, "Failed to allocate value map");
    }

    co attribute_values = (co)coMapGet(p->value_ids, attribute);
    if (attribute_values == NULL) {
        if (!create) return -1;
        attribute_values = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
        if (attribute_values == NULL || coMapAdd(p->value_ids, attribute, attribute_values) == NULL) {
            if (attribute_values) coDelete(attribute_values);
            dwa_error(p, "Failed to allocate attribute value map");
        }
        attribute_values = (co)coMapGet(p->value_ids, attribute);
    }

    if (p->value_strings == NULL) {
        if (!create) return -1;
        p->value_strings = coNewMap(CO_STRDUP | CO_STRFREE | CO_FREE_VALS);
        if (p->value_strings == NULL) dwa_error(p, "Failed to allocate reverse value map");
    }
    co attribute_strings = (co)coMapGet(p->value_strings, attribute);
    if (attribute_strings == NULL) {
        if (!create) return -1;
        attribute_strings = coNewVector(CO_FREE_VALS);
        if (attribute_strings == NULL || coMapAdd(p->value_strings, attribute, attribute_strings) == NULL) {
            if (attribute_strings) coDelete(attribute_strings);
            dwa_error(p, "Failed to allocate reverse attribute value list");
        }
        attribute_strings = (co)coMapGet(p->value_strings, attribute);
    }

    cco value_id_object = coMapGet(attribute_values, value);
    if (value_id_object != NULL) return (int32_t)coDblGet(value_id_object);
    if (!create) return -1;

    long next_id = coVectorSize(attribute_strings);
    if (next_id >= INT32_MAX) dwa_error(p, "Too many distinct values for one attribute");
    int32_t value_id = (int32_t)next_id;
    if (!coPSDExtendByValue(p->psd, attribute, value_id))
        dwa_error(p, "Failed to extend problem-space description");

    co value_id_number = coNewDbl(value_id);
    if (value_id_number == NULL || coMapAdd(attribute_values, value, value_id_number) == NULL) {
        if (value_id_number) coDelete(value_id_number);
        dwa_error(p, "Failed to record attribute value");
    }
    co value_string = coNewStr(CO_STRDUP, value);
    if (value_string == NULL || coVectorAdd(attribute_strings, value_string) < 0) {
        if (value_string) coDelete(value_string);
        dwa_error(p, "Failed to record reverse attribute value");
    }
    return value_id;
}

static void dwa_restore_dnf_value_strings(dwa_t *p, co dnf) {
    for (long term_idx = 0; term_idx < coVectorSize(dnf); term_idx++) {
        co term = (co)coVectorGet(dnf, term_idx);
        co attributes = coNewVector(CO_FREE_VALS);
        if (attributes == NULL) dwa_error(p, "Failed to allocate output attribute list");

        coMapIterator iter;
        if (coMapLoopFirst(&iter, term)) {
            do {
                co attribute = coNewStr(CO_STRDUP, coMapLoopKey(&iter));
                if (attribute == NULL || coVectorAdd(attributes, attribute) < 0) {
                    if (attribute) coDelete(attribute);
                    coDelete(attributes);
                    dwa_error(p, "Failed to copy output attribute name");
                }
            } while (coMapLoopNext(&iter));
        }

        for (long attr_idx = 0; attr_idx < coVectorSize(attributes); attr_idx++) {
            const char *attribute = coStrGet(coVectorGet(attributes, attr_idx));
            cco value_ids = coMapGet(term, attribute);
            cco values = coMapGet(p->value_strings, attribute);
            if (!coIsInt32Vector(value_ids) || !coIsVector(values)) {
                coDelete(attributes);
                dwa_error(p, "Missing reverse value mapping for output term");
            }

            co strings = coNewVector(CO_FREE_VALS);
            if (strings == NULL) {
                coDelete(attributes);
                dwa_error(p, "Failed to allocate output string values");
            }
            for (long value_idx = 0; value_idx < coInt32VectorSize(value_ids); value_idx++) {
                int32_t value_id = coInt32VectorGet(value_ids, value_idx);
                cco value = coVectorGet(values, value_id);
                if (!coIsStr(value)) {
                    coDelete(strings);
                    coDelete(attributes);
                    dwa_error(p, "Invalid reverse value ID in output term");
                }
                co copy = coNewStr(CO_STRDUP, coStrGet(value));
                if (copy == NULL || coVectorAdd(strings, copy) < 0) {
                    if (copy) coDelete(copy);
                    coDelete(strings);
                    coDelete(attributes);
                    dwa_error(p, "Failed to copy output string value");
                }
            }
            if (coMapAdd(term, attribute, strings) == NULL) {
                coDelete(strings);
                coDelete(attributes);
                dwa_error(p, "Failed to restore output string values");
            }
        }
        coDelete(attributes);
    }
}

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
                char attr_name[DWA_STR_BUF_SIZE];
                dwa_get_string(p, attr_name, sizeof(attr_name));
                if (dwa_consume_token_char(p) != ':') dwa_error(p, "Expected ':'");
                if (dwa_consume_token_char(p) != '[') dwa_error(p, "Expected '['");
                if (dwa_peek_token_char(p) != ']' || dwa_has_content_between_tokens(p)) {
                    for (;;) {
                        char *value = dwa_parse_value_string(p);
                        if (value == NULL) dwa_error(p, "Failed to parse value");
                        dwa_intern_value(p, attr_name, value, 1);
                        free(value);
                        char c = dwa_consume_token_char(p);
                        if (c == ']') break;
                        if (c != ',') dwa_error(p, "Expected ',' or ']'");
                    }
                } else {
                    dwa_consume_token_char(p); // consume ']'
                }
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
            char key[DWA_STR_BUF_SIZE];
            dwa_get_string(p, key, sizeof(key));
            if (dwa_consume_token_char(p) != ':') dwa_error(p, "Expected ':'");
            if (strcmp(key, "dnf") == 0) {
                dwa_collect_psd_dnf(p);
            } else {
                dwa_collect_psd_recursive(p);
            }
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
    p->token_idx++; // consume '['
    if (dwa_peek_token_char(p) == ']') {
        p->token_idx++; // consume ']'
        return bvdnf;
    }

    for (;;) {
        p->token_idx++; // consume '{'
        co_BVType bv = coNewBV(p->total_bits);
        
        if (dwa_peek_token_char(p) != '}') {
            coBVSetToUniversal(p->psd, bv);
            for (;;) {
                char attr_name[DWA_STR_BUF_SIZE];
                dwa_fast_get_string(p, attr_name, sizeof(attr_name));
                cco attr_meta = coMapGet(p->bvattributes, attr_name);
                int32_t attr_idx = coInt32VectorGet(attr_meta, 0);
                int32_t start_bit = coInt32VectorGet(attr_meta, 1);
                co val_map = (co)coMapGet(p->bvvaluepos, attr_name);
                
                co_BVType mask = (co_BVType)coVectorGet(p->bvmask, attr_idx);
                coBVANDNOT(bv, bv, mask); // Clear attribute bits

                p->token_idx += 2; // skip ':' and '['
                
                if (dwa_peek_token_char(p) != ']' || dwa_has_content_between_tokens(p)) {
                    for (;;) {
                        char *value = dwa_parse_value_string(p);
                        if (value == NULL) dwa_error(p, "Failed to parse value");
                        int32_t value_id = dwa_intern_value(p, attr_name, value, 0);
                        free(value);
                        if (value_id < 0) dwa_error(p, "Value missing from problem-space description");
                        char value_id_string[32];
                        snprintf(value_id_string, sizeof(value_id_string), "%d", value_id);
                        cco pos_obj = coMapGet(val_map, value_id_string);
                        if (pos_obj == NULL) dwa_error(p, "Value missing from bitvector index");
                        int32_t local_pos = (int32_t)coDblGet(pos_obj);
                        coBVSet(bv, start_bit + local_pos);
                        
                        if (dwa_consume_token_char(p) == ']') break;
                    }
                } else {
                    p->token_idx++; // consume ']'
                }
                if (dwa_consume_token_char(p) == '}') break;
            }
        } else {
            p->token_idx++; // consume '}'
            coBVSetToUniversal(p->psd, bv);
        }
        coVectorAdd(bvdnf, (cco)bv);
        if (dwa_consume_token_char(p) == ']') break;
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
            char key[DWA_STR_BUF_SIZE];
            dwa_get_string(p, key, sizeof(key));
            dwa_consume_token_char(p); // :
            if (strcmp(key, "op") == 0) {
                char op[DWA_STR_BUF_SIZE];
                dwa_get_string(p, op, sizeof(op));
                strncpy(p->op_name, op, sizeof(p->op_name) - 1);
                p->op_name[sizeof(p->op_name) - 1] = '\0';
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
                    char key2[DWA_STR_BUF_SIZE];
                    dwa_get_string(p, key2, sizeof(key2));
                    dwa_consume_token_char(p);
                    if (strcmp(key2, "id") == 0) {
                        id = dwa_parse_int(p);
                    }
                }
                co dnf_obj = coNewMap(CO_STRDUP | CO_FREE_VALS);
                coMapAdd(dnf_obj, "dnf", (cco)bvdnf);
                coMapAdd(dnf_obj, "id", (cco)coNewDbl((double)id));
                if (target_list && *target_list) coVectorAdd(*target_list, dnf_obj); else coDelete(dnf_obj);
            } else {
                dwa_build_bvdnf_recursive(p, target_list);
            }
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
                co result_dnf = coNewDNFFromBVDNF(p->psd, (cco)res_bv);
                dwa_restore_dnf_value_strings(p, result_dnf);
                coVectorAdd(result_list, (cco)result_dnf);
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

char *dwa_execute_op_to_str(dwa_t *p) {
    if (!p->arg1_dnf_list || !p->arg2_dnf_list) {
        dwa_print(p, "Error: Missing arguments for operation.\n");
        return strdup("{}");
    }
    int is_check = (p->op_name[0] != '\0' && strcmp(p->op_name, "intersection-check") == 0);
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
                co result_dnf = coNewDNFFromBVDNF(p->psd, (cco)res_bv);
                dwa_restore_dnf_value_strings(p, result_dnf);
                coVectorAdd(result_list, (cco)result_dnf);
                coDelete(res_bv);
            }
        }
    }
    double t2 = get_ms();
    dwa_print(p, "Op execution time: %.4f ms\n", t2 - t1);
    dwa_print(p, "Total time (excluding JSON write): %.4f ms\n", t2 - p->start_time);
    dwa_print(p, "Operation '%s' result: %ld items generated.\n", p->op_name, coVectorSize(result_list));

    co wrapper = coNewMap(CO_STRDUP | CO_FREE_VALS);
    coMapAdd(wrapper, "result", (cco)result_list);
    coMapAdd(wrapper, "log", (cco)coNewStr(CO_STRDUP, p->log_buffer));

    char *buf = NULL;
    size_t size = 0;
    FILE *mem_fp = open_memstream(&buf, &size);
    if (mem_fp) {
        coWriteJSON(wrapper, 0, 0, mem_fp);
        fclose(mem_fp);
    } else {
        buf = strdup("{}");
    }
    
    coDelete(wrapper);
    return buf;
}
