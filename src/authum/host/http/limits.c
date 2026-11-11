#include <authum/host/http/internal.h>
#include <lh/core/bit/endian.h>
#include <lh/core/char/map.h>
#include <lh/core/memory/std.h>

authum_i32_t
authum_http_peer_acquire(authum_http_server_t *server, authum_u32_t ip, authum_i64_t now,
                         const authum_http_options_t *options)
{
    lh_usize_t slot = 0u;

    return lh_http_limit_peer_try_acquire(server->peers, AUTHUM_HTTP_MAX_PEERS, ip, now,
                                          lh_cast_static(lh_u32_t, options->per_ip),
                                          lh_cast_static(lh_u32_t, options->requests_per_second),
                                          lh_addr_of(slot))
               ? lh_cast_static(authum_i32_t, slot)
               : -1;
}

authum_bool_t
authum_http_client_expired(const authum_http_client_t *client, const authum_http_options_t *options,
                           authum_i64_t now)
{
    authum_i64_t timeout = lh_math_mul(options->timeout_seconds, 1000LL);
    authum_i64_t age = lh_math_sub(now, client->born);
    /* The request itself gets at most AUTHUM_HTTP_HEADER_TIMEOUT_MS: a slot is
     * not held for the whole transfer timeout by a client that never asks. */
    authum_i64_t header_timeout =
        lh_math_lt(timeout, AUTHUM_HTTP_HEADER_TIMEOUT_MS) ? timeout : AUTHUM_HTTP_HEADER_TIMEOUT_MS;
    if (lh_math_gt(lh_math_sub(now, client->progress), timeout) ||
        (!client->state && lh_math_gt(age, header_timeout)))
    {
        return authum_bool_true;
    }
    /* No fixed lifetime: after the grace period a transfer only has to keep
     * an average rate, so a large map on a busy server still completes. */
    return lh_math_gt(age, AUTHUM_HTTP_RATE_GRACE_MS) &&
           lh_math_lt(lh_math_mul(lh_cast_static(authum_i64_t, client->body_bytes), 1000LL),
                      lh_math_mul(age, AUTHUM_HTTP_MIN_BYTES_PER_SECOND));
}

void
authum_http_budget_refill(double *tokens, authum_i64_t *last, authum_i64_t now,
                          const authum_http_options_t *options)
{
    const lh_real_t per_second = lh_cast_static(lh_real_t, options->bytes_per_second);

    lh_ptr_deref(tokens) = lh_http_limit_bucket_get_refilled(
        lh_ptr_deref(tokens), lh_math_sub(now, lh_ptr_deref(last)), per_second,
        lh_math_div(per_second, 10.0));
    lh_ptr_deref(last) = now;
}

void
authum_http_peer_release(authum_http_server_t *server, authum_i32_t slot)
{
    if (lh_math_ge(slot, 0))
    {
        lh_http_limit_peer_release(lh_addr_of(server->peers[slot]));
    }
}

authum_u32_t
authum_http_host_order(authum_u32_t key)
{
    return lh_bit_unpack_be32(lh_cast_reinterpret(lh_byte_ptr, lh_addr_of(key)));
}

void
authum_http_key_to_ip4(authum_u32_t key, lh_net_ip4_t *out)
{
    lh_net_ip4_init_by_u32(out, authum_http_host_order(key));
}

authum_u32_t
authum_http_key_of_ip4(const lh_net_ip4_t *address)
{
    authum_u32_t key = 0u;

    lh_bit_pack_be32(lh_net_ip4_to_u32(address), lh_cast_reinterpret(lh_byte_ptr, lh_addr_of(key)));
    return key;
}

authum_bool_t
authum_http_trusted_parse(authum_http_server_t *server, authum_char_t *error,
                          authum_u32_t error_size)
{
    /* Entries are separated by spaces, commas or tabs, in any number. */
    const authum_char_t separators[3] = {lh_char_map_space, lh_char_map_comma, lh_char_map_ht};

    server->trusted_count = 0u;

    lh_str_view_t list;

    lh_str_view_init(lh_addr_of(list), server->options.trusted_proxies);

    lh_str_view_t entry;
    lh_usize_t pos = 0u;
    lh_bool_t had_separator = lh_bool_false;

    while (lh_str_view_split_next_of(lh_addr_of(list), separators, sizeof separators,
                                     lh_addr_of(pos), lh_addr_of(entry), lh_addr_of(had_separator)))
    {
        if (lh_str_view_is_empty(lh_addr_of(entry)))
        {
            continue;
        }
        if (lh_math_ge(server->trusted_count, AUTHUM_HTTP_MAX_TRUSTED) ||
            !lh_net_cidr_try_parse(lh_str_view_get_data(lh_addr_of(entry)),
                                   lh_str_view_get_size(lh_addr_of(entry)),
                                   lh_addr_of(server->trusted[server->trusted_count])))
        {
            /* The entry is a view into the list: copied, cut, to be shown. */
            authum_char_t shown[64];
            authum_unused(lh_str_ptr_copy_cstr(shown, sizeof shown,
                                               lh_str_view_get_data(lh_addr_of(entry)),
                                               lh_str_view_get_size(lh_addr_of(entry))));
            authum_unused(lh_str_ptr_format_cstr(
                error, error_size,
                "trusted_proxy entry '%s' must be an IPv4 address or CIDR block (at most %d "
                "entries)",
                shown, AUTHUM_HTTP_MAX_TRUSTED));
            return authum_bool_false;
        }
        server->trusted_count = lh_math_add_one(server->trusted_count);
    }
    return authum_bool_true;
}

authum_bool_t
authum_http_trusted(const authum_http_server_t *server, authum_u32_t address)
{
    lh_net_ip4_t ip;

    authum_http_key_to_ip4(address, lh_addr_of(ip));
    return lh_net_cidr_list_contains(server->trusted, server->trusted_count, lh_addr_of(ip));
}

authum_u32_t
authum_http_forwarded_client(const authum_http_server_t *server,
                             const lh_str_view_t *const *values, authum_usize_t count,
                             authum_u32_t proxy)
{
    lh_net_ip4_t from;

    authum_http_key_to_ip4(proxy, lh_addr_of(from));

    lh_net_ip4_t client;

    lh_http_forwarded_copy_client(values, count, lh_addr_of(from), server->trusted,
                                  server->trusted_count, lh_addr_of(client));
    return authum_http_key_of_ip4(lh_addr_of(client));
}

void
authum_http_refusals_flush(authum_http_server_t *server, authum_i64_t now)
{
    authum_http_refusals_t *r = lh_addr_of(server->refusals);
    authum_i64_t second = now / 1000;
    if (second == r->second)
    {
        return;
    }
    if (r->limit || r->pending || r->slots || r->evicted || r->unlogged)
    {
        authum_char_t line[AUTHUM_HTTP_LOG_LINE_CAPACITY];
        authum_unused(lh_str_ptr_format_cstr(
            line, sizeof line,
            "http event=refused_summary rate_or_per_ip=%u per_ip_pending=%u no_slot=%u "
            "evicted=%u unlogged_refusals=%u module=%s",
            r->limit, r->pending, r->slots, r->evicted, r->unlogged,
            server->tag ? server->tag : "-"));
        authum_http_access_line(server, line);
    }
    lh_memory_std_set(r, 0, sizeof lh_ptr_deref(r));
    r->second = second;
}

void
authum_http_refusal_count(authum_http_server_t *server, authum_u32_t *counter, authum_i64_t now)
{
    authum_http_refusals_flush(server, now);
    lh_ptr_deref(counter) = lh_math_add_one(lh_ptr_deref(counter));
}
