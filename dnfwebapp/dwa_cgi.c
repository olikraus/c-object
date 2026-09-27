#include "dnfwebapp.h"

int main(int argc, char **argv) {
    dwa_t p;
    dwa_init(&p);
    
    /* CGI apps shouldn't usually be verbose to stdout unless it's part of the JSON */
    p.verbose = 0; 

    char *content_length_str = getenv("CONTENT_LENGTH");
    if (!content_length_str) {
        printf("Content-Type: application/json\n\n");
        printf("{\"error\": \"No CONTENT_LENGTH provided\"}\n");
        return 0;
    }

    size_t content_length = (size_t)atoll(content_length_str);
    if (content_length == 0) {
        printf("Content-Type: application/json\n\n");
        printf("{\"error\": \"Empty content\"}\n");
        return 0;
    }

    p.size = content_length;
    p.buffer = (char*)malloc(p.size + 32);
    if (!p.buffer) {
        printf("Content-Type: application/json\n\n");
        printf("{\"error\": \"Memory allocation failed\"}\n");
        return 0;
    }

    size_t bytes_read = fread(p.buffer, 1, p.size, stdin);
    p.buffer[bytes_read] = '\0';
    memset(p.buffer + bytes_read, 0, 32);

    coBVDetect();
    
    dwa_scan(&p);

    /* Output CGI header */
    printf("Content-Type: application/json\n\n");

    /* Process the JSON (Operation or Validation) */
    p.token_idx = 0;
    int is_op = 0;
    if (dwa_peek_token_char(&p) == '{') {
        is_op = 1;
    }
    
    if (is_op) {
        dwa_process_op_json(&p);
    } else {
        dwa_validate(&p);
        co wrapper = coNewMap(CO_STRDUP | CO_FREE_VALS);
        coMapAdd(wrapper, "status", (cco)coNewStr(CO_STRDUP, "valid"));
        coMapAdd(wrapper, "log", (cco)coNewStr(CO_STRDUP, p.log_buffer));
        coWriteJSON(wrapper, 0, 0, stdout);
        printf("\n");
        coDelete(wrapper);
    }

    dwa_destroy(&p);
    return 0;
}
