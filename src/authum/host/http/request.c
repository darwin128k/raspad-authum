#include "internal.h"
#include <lh/http/parser.h>
#include <strings.h>

void
authum_http_request_read(authum_http_client_t *client, const int *roots,
                         const authum_http_options_t *options)
{
    lh_http_parser_t parser;
    lh_http_request_t request;
    lh_http_header_t headers[64];
    lh_str_view_t input = lh_str_view_initializer(client->input, client->used);
    lh_http_parser_init(&parser);
    lh_http_parser_set_limits(&parser, sizeof client->input, sizeof client->input);
    lh_http_parse_status_t parsed = lh_http_parser_request(&parser, input, &request, headers, 64);
    if (parsed == lh_http_parse_incomplete && client->used < sizeof client->input)
    {
        return;
    }
    if (parsed != lh_http_parse_complete || lh_http_parser_get_consumed(&parser) != client->used ||
        request.version_minor > 1)
    {
        authum_http_response_set(client, 400, "Bad Request", 0, 1);
        return;
    }
    const char *method = request.method.begin, *path = request.target.begin;
    size_t method_length = request.method.size, path_length = request.target.size;
    authum_http_access_token(client->method, sizeof client->method, method, method_length);
    authum_http_access_token(client->path, sizeof client->path, path, path_length);
    int head = method_length == 4 && !memcmp(method, "HEAD", 4);
    if (!head && !(method_length == 3 && !memcmp(method, "GET", 3)))
    {
        authum_http_response_set(client, 405, "Method Not Allowed", 0, 1);
        return;
    }
    for (size_t i = 0; i < request.header_count; i++)
    {
        if ((headers[i].name.size == 17 &&
             !strncasecmp(headers[i].name.begin, "Transfer-Encoding", 17)) ||
            (headers[i].name.size == 14 &&
             !strncasecmp(headers[i].name.begin, "Content-Length", 14)))
        {
            authum_http_response_set(client, 400, "Bad Request", 0, 1);
            return;
        }
    }
    off_t length = 0;
    for (unsigned i = 0; i < options->source_count; i++)
    {
        client->file = authum_http_resource_open(roots[i], options, path, path_length, &length);
        if (client->file >= 0)
        {
            client->source = i + 1;
            break;
        }
    }
    if (client->file < 0)
    {
        authum_http_response_set(client, 404, "Not Found", 0, 1);
    }
    else
    {
        authum_http_response_set(client, 200, "OK", length, head);
    }
}
