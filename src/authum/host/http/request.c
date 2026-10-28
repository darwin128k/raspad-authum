#include <authum/host/http/internal.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/size.h>
#include <lh/math/ops.h>
#include <lh/util/ptr.h>
#include <lh/memory/std.h>
#include <lh/http/parser.h>
#include <strings.h>

void
authum_http_request_read(authum_http_client_t *client, const authum_i32_t *roots,
                         const authum_http_options_t *options)
{
    lh_http_parser_t parser;
    lh_http_request_t request;
    lh_http_header_t headers[64];
    lh_str_view_t input = lh_str_view_initializer(client->input, client->used);
    lh_http_parser_init(lh_addr_of(parser));
    lh_http_parser_set_limits(lh_addr_of(parser), sizeof client->input, sizeof client->input);
    lh_http_parse_status_t parsed =
        lh_http_parser_request(lh_addr_of(parser), input, lh_addr_of(request), headers, 64);
    if (lh_math_eq(parsed, lh_http_parse_incomplete) &&
        lh_math_lt(client->used, sizeof client->input))
    {
        return;
    }
    if (lh_math_ne(parsed, lh_http_parse_complete) ||
        lh_math_ne(lh_http_parser_get_consumed(lh_addr_of(parser)), client->used) ||
        lh_math_gt(request.version_minor, 1))
    {
        authum_http_response_set(client, 400, "Bad Request", 0, 1);
        return;
    }
    const authum_char_t *method = request.method.begin, *path = request.target.begin;
    authum_usize_t method_length = request.method.size, path_length = request.target.size;
    authum_http_access_token(client->method, sizeof client->method, method, method_length);
    authum_http_access_token(client->path, sizeof client->path, path, path_length);
    authum_i32_t head = lh_math_eq(method_length, 4) && !lh_memory_std_compare(method, "HEAD", 4);
    if (!head && !(lh_math_eq(method_length, 3) && !lh_memory_std_compare(method, "GET", 3)))
    {
        authum_http_response_set(client, 405, "Method Not Allowed", 0, 1);
        return;
    }
    for (authum_usize_t i = 0; lh_math_lt(i, request.header_count); i = lh_math_add_one(i))
    {
        if ((lh_math_eq(headers[i].name.size, 17) &&
             !strncasecmp(headers[i].name.begin, "Transfer-Encoding", 17)) ||
            (lh_math_eq(headers[i].name.size, 14) &&
             !strncasecmp(headers[i].name.begin, "Content-Length", 14)))
        {
            authum_http_response_set(client, 400, "Bad Request", 0, 1);
            return;
        }
    }
    off_t length = 0;
    for (authum_usize_t i = 0; lh_math_lt(i, options->source_count); i = lh_math_add_one(i))
    {
        client->file =
            authum_http_resource_open(roots[i], options, path, path_length, lh_addr_of(length));
        if (lh_math_ge(client->file, 0))
        {
            client->source = lh_math_add(i, 1);
            break;
        }
    }
    if (lh_math_lt(client->file, 0))
    {
        authum_http_response_set(client, 404, "Not Found", 0, 1);
    }
    else
    {
        authum_http_response_set(client, 200, "OK", length, head);
    }
}
