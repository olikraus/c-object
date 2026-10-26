#include "dnfwebapp.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#define INITIAL_BUF_SIZE 65536

typedef struct {
    char host[256];
    int port;
} lambda_api_t;

static int connect_to_api(lambda_api_t *api) {
    struct sockaddr_in serv_addr;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    if (inet_aton(api->host, &serv_addr.sin_addr) == 0) {
        struct hostent *server = gethostbyname(api->host);
        if (server == NULL) {
            close(fd);
            return -1;
        }
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    }
    serv_addr.sin_port = htons(api->port);

    if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static char *http_get(lambda_api_t *api, const char *path, char *request_id, size_t *body_len) {
    char request[512];
    int req_len = snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "User-Agent: dwa-lambda/1.0\r\n"
             "Connection: close\r\n\r\n",
             path, api->host, api->port);

    int fd = connect_to_api(api);
    if (fd < 0) return NULL;
    
    write(fd, request, req_len);

    size_t buf_size = INITIAL_BUF_SIZE;
    char *response = malloc(buf_size);
    size_t total_received = 0;
    int n;
    while ((n = recv(fd, response + total_received, buf_size - total_received - 1, 0)) > 0) {
        total_received += n;
        if (total_received + 1024 > buf_size) {
            buf_size *= 2;
            response = realloc(response, buf_size);
        }
    }
    response[total_received] = '\0';
    close(fd);

    if (total_received == 0) {
        free(response);
        return NULL;
    }

    char *header_end = strstr(response, "\r\n\r\n");
    if (!header_end) {
        free(response);
        return NULL;
    }
    *header_end = '\0';
    char *body_start = header_end + 4;

    /* Parse Request ID */
    char *id_ptr = strstr(response, "Lambda-Runtime-Aws-Request-Id: ");
    if (id_ptr) {
        id_ptr += strlen("Lambda-Runtime-Aws-Request-Id: ");
        char *id_end = strstr(id_ptr, "\r\n");
        if (id_end) {
            size_t len = id_end - id_ptr;
            if (len < 255) {
                memcpy(request_id, id_ptr, len);
                request_id[len] = '\0';
            }
        }
    }

    /* Parse Content-Length */
    char *cl_ptr = strcasestr(response, "Content-Length: ");
    if (cl_ptr) {
        *body_len = atoll(cl_ptr + 16);
    } else {
        *body_len = total_received - (body_start - response);
    }

    char *actual_body = malloc(*body_len + 1);
    memcpy(actual_body, body_start, *body_len);
    actual_body[*body_len] = '\0';
    free(response);
    return actual_body;
}

static int http_post(lambda_api_t *api, const char *path, const char *body) {
    size_t body_len = strlen(body);
    size_t total_len = body_len + 1024;
    char *full_request = malloc(total_len);
    int header_len = snprintf(full_request, total_len,
             "POST %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "User-Agent: dwa-lambda/1.0\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n\r\n",
             path, api->host, api->port, body_len);
    
    memcpy(full_request + header_len, body, body_len);

    int fd = connect_to_api(api);
    if (fd < 0) {
        free(full_request);
        return -1;
    }
    
    write(fd, full_request, header_len + body_len);
    
    /* Signal end of request */
    shutdown(fd, SHUT_WR);

    /* Read response */
    char buf[1024];
    while (recv(fd, buf, sizeof(buf), 0) > 0);
    
    close(fd);
    free(full_request);
    return 0;
}



int main() {
    char *runtime_api = getenv("AWS_LAMBDA_RUNTIME_API");
    if (!runtime_api) return 1;

    lambda_api_t api;
    char *colon = strchr(runtime_api, ':');
    if (colon) {
        size_t host_len = colon - runtime_api;
        memcpy(api.host, runtime_api, host_len);
        api.host[host_len] = '\0';
        api.port = atoi(colon + 1);
    } else {
        strncpy(api.host, runtime_api, sizeof(api.host));
        api.port = 80;
    }

    coBVDetect();

    while (1) {
        char request_id[256] = {0};
        size_t body_len = 0;
        char *body = http_get(&api, "/2018-06-01/runtime/invocation/next", request_id, &body_len);
        if (!body) {
            usleep(100000);
            continue;
        }

        fprintf(stderr, "INFO: Received invocation %s (size: %zu)\n", request_id, body_len);

        dwa_t p;
        dwa_init(&p);
        p.size = body_len;
        p.buffer = body; 

        dwa_scan(&p);
        p.token_idx = 0;
        
        char *response_json = NULL;
        if (dwa_peek_token_char(&p) == '{') {
            /* Full Operation Cycle: mirrors dwa_process_op_json but captures to string */
            double t1 = get_ms();
            p.psd = coNewPSD();
            p.token_idx = 0;
            dwa_collect_psd_recursive(&p);
            double t2 = get_ms();
            dwa_print(&p, "psd parser time: %.4f ms\n", t2 - t1);

            double t3 = get_ms();
            coBVPreparePSD(p.psd);
            p.bvpos = (co)coMapGet(p.psd, "bvpos");
            p.total_bits = coInt32VectorGet(p.bvpos, coInt32VectorSize(p.bvpos) - 1);
            p.bvmask = (co)coMapGet(p.psd, "bvmask");
            p.bvattributes = (co)coMapGet(p.psd, "bvattributes");
            p.bvvaluepos = (co)coMapGet(p.psd, "bvvaluepos");
            double t4 = get_ms();
            dwa_print(&p, "psd bv prep time: %.4f ms\n", t4 - t3);
            
            double t5 = get_ms();
            p.token_idx = 0;
            dwa_build_bvdnf_recursive(&p, NULL);
            double t6 = get_ms();
            dwa_print(&p, "dnf parser time: %.4f ms\n", t6 - t5);
            
            response_json = dwa_execute_op_to_str(&p);
        } else {
            dwa_validate(&p);
            co wrapper = coNewMap(CO_STRDUP | CO_FREE_VALS);
            coMapAdd(wrapper, "status", (cco)coNewStr(CO_STRDUP, "valid"));
            coMapAdd(wrapper, "log", (cco)coNewStr(CO_STRDUP, p.log_buffer));
            
            size_t size = 0;
            FILE *mem_fp = open_memstream(&response_json, &size);
            if (mem_fp) {
                coWriteJSON(wrapper, 0, 0, mem_fp);
                fclose(mem_fp);
            }
            coDelete(wrapper);
        }

        char path[512];
        snprintf(path, sizeof(path), "/2018-06-01/runtime/invocation/%s/response", request_id);
        
        fprintf(stderr, "INFO: Posting response for %s (size: %zu)\n", request_id, strlen(response_json));
        http_post(&api, path, response_json);

        free(response_json);
        dwa_destroy(&p);
    }

    return 0;
}
