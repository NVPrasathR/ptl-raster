#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stddef.h>

#include "../application/pharmacy_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_SERVER_MAX_METHOD 8
#define HTTP_SERVER_MAX_PATH 128
#define HTTP_SERVER_MAX_BODY 4096
#define HTTP_SERVER_MAX_AUTHORIZATION 96
#define HTTP_SERVER_RESPONSE_CAPACITY 65536

typedef struct {
    char method[HTTP_SERVER_MAX_METHOD];
    char path[HTTP_SERVER_MAX_PATH];
    char authorization[HTTP_SERVER_MAX_AUTHORIZATION];
    char body[HTTP_SERVER_MAX_BODY];
    size_t body_len;
} http_request_t;

int http_server_parse_request(const char *request, size_t request_len, http_request_t *out_request);
int http_server_route_request(pharmacy_protocol_context_t *ctx,
                             const http_request_t *request,
                             char *response,
                             size_t response_cap,
                             int *http_status_out);
int http_server_render_response(int http_status,
                               const char *json_body,
                               char *response,
                               size_t response_cap);

#ifdef __cplusplus
}
#endif

#endif
