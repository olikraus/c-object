#include "dnfwebapp.h"

static void dwa_validate_value(dwa_t *p);

static void dwa_validate_object(dwa_t *p) {
    dwa_consume_token_char(p);
    if (dwa_peek_token_char(p) == '}') { dwa_consume_token_char(p); return; }
    for (;;) {
        dwa_skip_string(p);
        dwa_consume_token_char(p); // :
        dwa_validate_value(p);
        char c = dwa_consume_token_char(p);
        if (c == '}') break;
        if (dwa_peek_token_char(p) == '}' && !dwa_has_content_between_tokens(p)) dwa_error(p, "Trailing comma");
    }
}

static void dwa_validate_array(dwa_t *p) {
    dwa_consume_token_char(p);
    if (dwa_peek_token_char(p) == ']') { dwa_consume_token_char(p); return; }
    for (;;) {
        dwa_validate_value(p);
        char c = dwa_consume_token_char(p);
        if (c == ']') break;
        if (dwa_peek_token_char(p) == ']' && !dwa_has_content_between_tokens(p)) dwa_error(p, "Trailing comma");
    }
}

static void dwa_validate_value(dwa_t *p) {
    char c = dwa_peek_token_char(p);
    if (c == '{') dwa_validate_object(p);
    else if (c == '[') dwa_validate_array(p);
    else if (c == '\"') dwa_skip_string(p);
    else {
        if (c == ',' || c == ']' || c == '}' || c == '\0' || c == ':') return;
        dwa_error(p, "Unexpected char");
    }
}

void dwa_validate(dwa_t *p) {
    dwa_print(p, "Validating JSON structure (Reference)...\n");
    double t1 = get_ms();
    p->token_idx = 0;
    dwa_validate_value(p);
    double t2 = get_ms();
    dwa_print(p, "Validation time: %.4f ms\n", t2 - t1);
}
