#include <authum/host/http/internal.h>
#include <authum/null.h>
#include <lh/core/memory/std.h>
#include <lh/core/http/parser.h>
#include <lh/core/char/map.h>

enum
{
    /** X-Forwarded-For fields read from one request. */
    AUTHUM_HTTP_MAX_FORWARDED = 8
};

static void
refuse(authum_http_client_t *client, authum_i32_t code, const authum_char_t *status,
       const authum_char_t *reason)
{
    client->reason = reason;
    authum_http_response_set(client, code, status, 0, 1);
}

/* Content-Length is accepted only as an empty body: digits that are all zero. */
static authum_bool_t
zero_length(const lh_str_view_t *value)
{
    const authum_char_t *text = lh_str_view_get_data(value);
    authum_usize_t size = lh_str_view_get_size(value);
    authum_usize_t digits = 0;
    for (authum_usize_t i = 0; i < size; ++i)
    {
        if (text[i] == ' ' || text[i] == '\t')
        {
            continue;
        }
        if (text[i] != '0')
        {
            return authum_bool_false;
        }
        ++digits;
    }
    return digits ? authum_bool_true : authum_bool_false;
}

static void
format_address(authum_u32_t address, authum_char_t *text, authum_usize_t capacity)
{
    const authum_byte_t *octets = (const authum_byte_t *)&address;
    lh_net_ip4_t ip;
    lh_net_ip4_init(lh_addr_of(ip), octets[0], octets[1], octets[2], octets[3]);
    authum_usize_t size = lh_net_ip4_format(lh_addr_of(ip), text, lh_math_sub(capacity, 1));
    text[size] = 0;
}

void
authum_http_request_read(authum_http_server_t *server, authum_http_client_t *client,
                         authum_i64_t now)
{
    const authum_http_options_t *options = lh_addr_of(server->options);
    lh_http_parser_t parser;
    lh_http_parser_init(lh_addr_of(parser));
    lh_http_parser_set_limits(lh_addr_of(parser), sizeof client->input, sizeof client->input);
    lh_http_request_t request;
    lh_str_view_t input = lh_str_view_initializer(client->input, client->used);
    lh_http_header_t headers[64];
    lh_http_parse_status_t parsed =
        lh_http_parser_request(lh_addr_of(parser), input, lh_addr_of(request), headers, 64);
    if (lh_math_eq(parsed, lh_http_parse_incomplete))
    {
        if (lh_math_lt(client->used, sizeof client->input))
        {
            return;
        }
        /* The buffer is full: a request line that never ended is a long URI,
         * anything after it is a header block over its budget. */
        if (!lh_str_ptr_find_of_char(client->input, client->used, lh_char_map_lf))
        {
            refuse(client, 414, "URI Too Long", "uri_too_long");
        }
        else
        {
            refuse(client, 431, "Request Header Fields Too Large", "header_too_large");
        }
        return;
    }
    if (lh_math_ne(parsed, lh_http_parse_complete))
    {
        lh_http_error_t error = lh_http_parser_get_error(lh_addr_of(parser));
        const authum_bool_t line_ended =
            lh_str_ptr_find_of_char(client->input, client->used, lh_char_map_lf) ? authum_bool_true
                                                                                : authum_bool_false;
        if (error == lh_http_error_header_capacity ||
            ((error == lh_http_error_head_size || error == lh_http_error_line_size) && line_ended))
        {
            refuse(client, 431, "Request Header Fields Too Large", "header_too_large");
        }
        else if (error == lh_http_error_head_size || error == lh_http_error_line_size)
        {
            refuse(client, 414, "URI Too Long", "uri_too_long");
        }
        else
        {
            refuse(client, 400, "Bad Request", "malformed");
        }
        return;
    }
    if (lh_math_ne(lh_http_parser_get_consumed(lh_addr_of(parser)), client->used))
    {
        refuse(client, 400, "Bad Request", "data_after_head");
        return;
    }
    if (lh_math_gt(lh_http_request_get_version_minor(lh_addr_of(request)), 1))
    {
        refuse(client, 400, "Bad Request", "version");
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
        refuse(client, 405, "Method Not Allowed", "method");
        return;
    }
    /* Field names compare without ASCII case. A body is never read: chunked
     * framing is refused, and Content-Length only as an explicit zero. */
    const lh_str_view_t transfer_encoding = lh_str_view_initializer_lit("Transfer-Encoding");
    const lh_str_view_t content_length = lh_str_view_initializer_lit("Content-Length");
    const lh_str_view_t forwarded_for = lh_str_view_initializer_lit("X-Forwarded-For");
    const lh_str_view_t *forwarded[AUTHUM_HTTP_MAX_FORWARDED];
    authum_usize_t forwarded_count = 0;
    for (authum_usize_t i = 0; lh_math_lt(i, lh_http_request_get_header_count(lh_addr_of(request)));
         i = lh_math_add_one(i))
    {
        const lh_str_view_t *name = lh_http_header_get_name(lh_addr_of(headers[i]));
        const lh_str_view_t *value = lh_http_header_get_value(lh_addr_of(headers[i]));
        if (lh_str_view_equals(name, lh_addr_of(transfer_encoding), lh_bool_true) ||
            (lh_str_view_equals(name, lh_addr_of(content_length), lh_bool_true) &&
             !zero_length(value)))
        {
            refuse(client, 400, "Bad Request", "body_framing");
            return;
        }
        if (client->proxied && lh_str_view_equals(name, lh_addr_of(forwarded_for), lh_bool_true))
        {
            if (forwarded_count == AUTHUM_HTTP_MAX_FORWARDED)
            {
                refuse(client, 400, "Bad Request", "forwarded_for");
                return;
            }
            forwarded[forwarded_count++] = value;
        }
    }
    if (client->proxied)
    {
        /* The proxy itself is never limited; its client is, from here on. */
        authum_u32_t address =
            authum_http_forwarded_client(server, forwarded, forwarded_count, client->address);
        authum_i32_t slot = authum_http_peer_acquire(server, address, now, options);
        lh_memory_std_copy(client->via, client->ip, sizeof client->via);
        format_address(address, client->ip, sizeof client->ip);
        client->address = address;
        if (lh_math_lt(slot, 0))
        {
            refuse(client, 429, "Too Many Requests", "rate_limited");
            return;
        }
        client->ip_slot = slot;
    }
    authum_char_t relative[2048];
    const authum_char_t *reason = authum_null;
    if (!authum_http_resource_check(options, path, path_length, relative, sizeof relative,
                                    lh_addr_of(reason)))
    {
        if (reason && (lh_str_ptr_equals(reason, "bad_encoding", lh_bool_false) || lh_str_ptr_equals(reason, "bad_target", lh_bool_false)))
        {
            refuse(client, 400, "Bad Request", reason);
        }
        else if (reason && lh_str_ptr_equals(reason, "uri_too_long", lh_bool_false))
        {
            refuse(client, 414, "URI Too Long", reason);
        }
        else
        {
            refuse(client, 404, "Not Found", reason);
        }
        return;
    }
    lh_u64_t length = 0;
    for (authum_usize_t i = 0; lh_math_lt(i, options->source_count); i = lh_math_add_one(i))
    {
        if (authum_http_resource_open(authum_http_client_get_file_mut(client),
                                      lh_addr_of(authum_http_server_get_roots_mut(server)[i]),
                                      options, relative, lh_addr_of(length), lh_addr_of(reason)))
        {
            client->source = lh_math_add(i, 1);
            break;
        }
    }
    if (!lh_os_fs_file_is_valid(authum_http_client_get_file_mut(client)))
    {
        refuse(client, 404, "Not Found", reason);
    }
    else
    {
        authum_http_response_set(client, 200, "OK", length, head);
    }
}
