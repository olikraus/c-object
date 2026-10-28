#include "dnfwebapp.h"

static void dwa_validate_value(dwa_t *p);

static void dwa_validate_object(dwa_t *p) {
    dwa_consume_token_char(p);
    if (dwa_peek_token_char(p) == '}' && !dwa_has_content_between_tokens(p)) {
        dwa_consume_token_char(p);
        return;
    }
    for (;;) {
        if (dwa_has_content_between_tokens(p)) dwa_error(p, "Expected object key");
        dwa_skip_string(p);
        if (dwa_has_content_between_tokens(p) || dwa_consume_token_char(p) != ':')
            dwa_error(p, "Expected ':'");
        dwa_validate_value(p);
        char c = dwa_consume_token_char(p);
        if (c == '}') break;
        if (c != ',') dwa_error(p, "Expected ',' or '}'");
        if (dwa_peek_token_char(p) == '}' && !dwa_has_content_between_tokens(p)) dwa_error(p, "Trailing comma");
    }
}

static void dwa_validate_array(dwa_t *p) {
    dwa_consume_token_char(p);
    if (dwa_peek_token_char(p) == ']' && !dwa_has_content_between_tokens(p)) {
        dwa_consume_token_char(p);
        return;
    }
    for (;;) {
        dwa_validate_value(p);
        char c = dwa_consume_token_char(p);
        if (c == ']') break;
        if (c != ',') dwa_error(p, "Expected ',' or ']'");
        if (dwa_peek_token_char(p) == ']' && !dwa_has_content_between_tokens(p)) dwa_error(p, "Trailing comma");
    }
}

static void dwa_validate_value(dwa_t *p) {
    if (dwa_has_content_between_tokens(p)) {
        dwa_parse_primitive(p);
        return;
    }
    char c = dwa_peek_token_char(p);
    if (c == '{') dwa_validate_object(p);
    else if (c == '[') dwa_validate_array(p);
    else if (c == '\"') dwa_skip_string(p);
    else dwa_error(p, "Expected JSON value");
    if (dwa_has_content_between_tokens(p)) dwa_error(p, "Unexpected content after value");
}

void dwa_validate(dwa_t *p) {
    dwa_print(p, "Validating JSON structure (Reference)...\n");
    double t1 = get_ms();
    p->token_idx = 0;
    if (p->pos_cnt == 0) {
        dwa_parse_primitive(p);
    } else {
        dwa_validate_value(p);
        if (p->token_idx != p->pos_cnt || dwa_has_content_between_tokens(p))
            dwa_error(p, "Unexpected content after value");
    }
    double t2 = get_ms();
    dwa_print(p, "Validation time: %.4f ms\n", t2 - t1);
}
