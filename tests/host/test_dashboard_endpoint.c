#include <stdio.h>
#include <string.h>

#include "http_server.h"
#include "pharmacy_protocol.h"

int main(void) {
    pharmacy_protocol_context_t ctx;
    http_request_t request;
    char payload[HTTP_SERVER_RESPONSE_CAPACITY];
    int status = 0;

    pharmacy_protocol_init(&ctx);
    memset(&request, 0, sizeof(request));
    snprintf(request.method, sizeof(request.method), "GET");
    snprintf(request.path, sizeof(request.path), "/");
    if (http_server_route_request(&ctx, &request, payload, sizeof(payload), &status) != 0 ||
        status != 200 || strstr(payload, "<!doctype html>") == NULL) {
        fprintf(stderr, "dashboard endpoint failed: status=%d\n", status);
        return 1;
    }
    return 0;
}
