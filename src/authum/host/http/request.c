#include <authum/host/http/internal.h>
#include <lh/http/parser.h>

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
        lh_math_gt(lh_http_request_get_version_minor(lh_addr_of(request)), 1))
    {
        authum_http_response_set(client, 400, "Bad Request", 0, 1);
        return;
    }
    const lh_str_view_t *method = lh_http_request_get_method(lh_addr_of(request));
    const lh_str_view_t *target = lh_http_request_get_target(lh_addr_of(request));
    const authum_char_t *path = lh_str_view_get_data(target);
    authum_usize_t path_length = lh_str_view_get_size(target);
    authum_http_access_token(client->method, sizeof client->method, lh_str_view_get_data(method),
                             lh_str_view_get_size(method));
    authum_http_access_token(client->path, sizeof client->path, path, path_length);
    const lh_str_view_t get = lh_str_view_initializer_lit("GET");
    const lh_str_view_t head_method = lh_str_view_initializer_lit("HEAD");
    authum_i32_t head = lh_str_view_equals(method, lh_addr_of(head_method), lh_bool_false);
    if (!head && !lh_str_view_equals(method, lh_addr_of(get), lh_bool_false))
    {
        authum_http_response_set(client, 405, "Method Not Allowed", 0, 1);
        return;
    }
    /* Field names compare without ASCII case; a body is not served, so its framing is refused. */
    const lh_str_view_t transfer_encoding = lh_str_view_initializer_lit("Transfer-Encoding");
    const lh_str_view_t content_length = lh_str_view_initializer_lit("Content-Length");
    for (authum_usize_t i = 0; lh_math_lt(i, lh_http_request_get_header_count(lh_addr_of(request)));
         i = lh_math_add_one(i))
    {
        const lh_str_view_t *name = lh_http_header_get_name(lh_addr_of(headers[i]));
        if (lh_str_view_equals(name, lh_addr_of(transfer_encoding), lh_bool_true) ||
            lh_str_view_equals(name, lh_addr_of(content_length), lh_bool_true))
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
