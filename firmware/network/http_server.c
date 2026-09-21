#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../web/dashboard_assets.h"

static void http_trim_headers(char *text) {
    size_t len = 0U;
    if (text == NULL) {
        return;
    }
    len = strlen(text);
    while (len > 0U && (text[len - 1U] == '\r' || text[len - 1U] == '\n' || text[len - 1U] == ' ' || text[len - 1U] == '\t')) {
        text[len - 1U] = '\0';
        --len;
    }
}

int http_server_parse_request(const char *request, size_t request_len, http_request_t *out_request) {
    const char *line_end = NULL;
    const char *body_cursor = NULL;
    char request_line[HTTP_SERVER_MAX_PATH + 32];
    char method[HTTP_SERVER_MAX_METHOD];
    char path[HTTP_SERVER_MAX_PATH];
    char version[16];
    size_t line_len = 0U;
    size_t body_len = 0U;

    if (request == NULL || request_len == 0U || out_request == NULL) {
        return -1;
    }

    memset(out_request, 0, sizeof(*out_request));
    line_end = memchr(request, '\n', request_len);
    if (line_end == NULL) {
        return -1;
    }

    line_len = (size_t)(line_end - request);
    if (line_len >= sizeof(request_line)) {
        return -1;
    }
    memcpy(request_line, request, line_len);
    request_line[line_len] = '\0';
    http_trim_headers(request_line);

    if (sscanf(request_line, "%7s %127s %15s", method, path, version) != 3) {
        return -1;
    }
    snprintf(out_request->method, sizeof(out_request->method), "%s", method);
    snprintf(out_request->path, sizeof(out_request->path), "%s", path);

    body_cursor = line_end + 1U;
    while (body_cursor < request + request_len) {
        const char *next_newline = memchr(body_cursor, '\n', (size_t)(request + request_len - body_cursor));
        if (next_newline == NULL) {
            break;
        }
        size_t header_length = (size_t)(next_newline - body_cursor);
        if (header_length != 0u && body_cursor[header_length - 1u] == '\r') {
            --header_length;
        }
        if (header_length == 0u) {
            body_cursor = next_newline + 1U;
            break;
        }
        static const char authorization_header[] = "Authorization:";
        if (header_length > sizeof(authorization_header) - 1u &&
            strncasecmp(body_cursor, authorization_header,
                        sizeof(authorization_header) - 1u) == 0) {
            const char *value = body_cursor + sizeof(authorization_header) - 1u;
            while (value < next_newline && (*value == ' ' || *value == '\t')) {
                ++value;
            }
            size_t value_length = (size_t)(next_newline - value);
            if (value_length != 0u && value[value_length - 1u] == '\r') {
                --value_length;
            }
            if (value_length >= sizeof(out_request->authorization)) {
                return -1;
            }
            memcpy(out_request->authorization, value, value_length);
            out_request->authorization[value_length] = '\0';
        }
        body_cursor = next_newline + 1U;
    }
    if (body_cursor < request + request_len) {
        body_len = (size_t)(request + request_len - body_cursor);
        if (body_len >= sizeof(out_request->body)) {
            return -1;
        }
        memcpy(out_request->body, body_cursor, body_len);
        out_request->body[body_len] = '\0';
        out_request->body_len = body_len;
    }

    return 0;
}

int http_server_render_response(int http_status,
                               const char *json_body,
                               char *response,
                               size_t response_cap) {
    size_t json_len = 0U;
    int written = 0;
    if (response == NULL || response_cap == 0U) {
        return -1;
    }
    json_len = (json_body == NULL) ? 0U : strlen(json_body);
    written = snprintf(response,
                       response_cap,
                       "HTTP/1.1 %d OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: %zu\r\n"
                       "Connection: close\r\n\r\n"
                       "%s",
                       http_status,
                       json_len,
                       json_body == NULL ? "" : json_body);
    if (written < 0 || (size_t)written >= response_cap) {
        return -1;
    }
    return 0;
}

int http_server_route_request(pharmacy_protocol_context_t *ctx,
                             const http_request_t *request,
                             char *response,
                             size_t response_cap,
                             int *http_status_out) {
    int http_status = 500;
    char payload[PHARMACY_JSON_RESPONSE_CAPACITY];
    if (ctx == NULL || request == NULL || response == NULL || response_cap == 0U) {
        return -1;
    }
    if (strcmp(request->method, "GET") == 0 && strcmp(request->path, "/") == 0) {
        size_t html_length = 0u;
        const uint8_t *html = raster_dashboard_get_html(&html_length);
        int header_length = snprintf(
            response, response_cap,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %zu\r\nConnection: close\r\n\r\n",
            html_length);
        if (html == NULL || header_length < 0 ||
            (size_t)header_length + html_length >= response_cap) {
            return -1;
        }
        memcpy(response + (size_t)header_length, html, html_length);
        response[(size_t)header_length + html_length] = '\0';
        if (http_status_out != NULL) {
            *http_status_out = 200;
        }
        return 0;
    }
    memset(payload, 0, sizeof(payload));
    if (strcmp(request->method, "GET") == 0 || strcmp(request->method, "POST") == 0 || strcmp(request->method, "PUT") == 0) {
        ctx->request_authorized =
            ctx->authorize != NULL &&
            ctx->authorize(ctx->authorize_context, request->authorization);
        http_status = pharmacy_protocol_handle_request(ctx,
                                                     request->method,
                                                     request->path,
                                                     request->body,
                                                     request->body_len,
                                                     payload,
                                                     sizeof(payload));
        ctx->request_authorized = false;
        if (http_status_out != NULL) {
            *http_status_out = http_status;
        }
        return http_server_render_response(http_status, payload, response, response_cap);
    }

    if (http_status_out != NULL) {
        *http_status_out = 405;
    }
    snprintf(response,
             response_cap,
             "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"success\":false,\"error\":{\"code\":\"METHOD_NOT_ALLOWED\",\"message\":\"Unsupported HTTP method\"}}\n");
    return 0;
}
