#include <authum/host/http/internal.h>
#include <lh/core/memory/std.h>

authum_i32_t
authum_http_peer_acquire(authum_http_server_t *server, authum_u32_t ip, authum_i64_t now,
                         const authum_http_options_t *options)
{
    authum_i32_t free_slot = -1;
    /* A full table must not refuse a new address: an idle peer whose
     * one-second window has not ended yet is reused, oldest window first. */
    authum_i32_t idle_slot = -1;
    for (authum_i32_t i = 0; lh_math_lt(i, AUTHUM_HTTP_MAX_PEERS); i = lh_math_add_one(i))
    {
        authum_http_peer_t *peer = lh_addr_of(server->peers[i]);
        if (peer->used && lh_math_eq(peer->ip, ip))
        {
            if (lh_math_ge(lh_math_sub(now, peer->window), 1000))
            {
                peer->window = now;
                peer->count = 0;
            }
            if (lh_math_ge(peer->active, lh_cast_static(authum_u32_t, options->per_ip)) ||
                lh_math_ge(peer->count, lh_cast_static(authum_u32_t, options->requests_per_second)))
            {
                return -1;
            }
            peer->count = lh_math_add_one(peer->count);
            peer->active = lh_math_add_one(peer->active);
            return i;
        }
        if (!peer->used || (!peer->active && lh_math_ge(lh_math_sub(now, peer->window), 1000)))
        {
            free_slot = i;
        }
        else if (!peer->active &&
                 (lh_math_lt(idle_slot, 0) ||
                  lh_math_lt(peer->window, server->peers[idle_slot].window)))
        {
            idle_slot = i;
        }
    }
    if (lh_math_lt(free_slot, 0))
    {
        free_slot = idle_slot;
    }
    if (lh_math_lt(free_slot, 0))
    {
        return -1;
    }
    server->peers[free_slot] =
        (authum_http_peer_t){.ip = ip, .used = 1, .active = 1, .count = 1, .window = now};
    return free_slot;
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
    double burst = lh_math_div(options->bytes_per_second, 10.0);
    lh_ptr_deref(tokens) +=
        lh_math_div(lh_math_mul((lh_math_sub(now, lh_ptr_deref(last))),
                                lh_cast_static(double, options->bytes_per_second)),
                    1000.0);
    lh_ptr_deref(last) = now;
    if (lh_math_gt(lh_ptr_deref(tokens), burst))
    {
        lh_ptr_deref(tokens) = burst;
    }
}

void
authum_http_peer_release(authum_http_server_t *server, authum_i32_t slot)
{
    if (lh_math_ge(slot, 0) && server->peers[slot].active)
    {
        server->peers[slot].active = lh_math_sub_one(server->peers[slot].active);
    }
}

/* An address key (octets in network order) as a host-order number. */
static authum_u32_t
authum_http_host_order(authum_u32_t key)
{
    const authum_byte_t *o = (const authum_byte_t *)&key;
    return ((authum_u32_t)o[0] << 24) | ((authum_u32_t)o[1] << 16) | ((authum_u32_t)o[2] << 8) |
           (authum_u32_t)o[3];
}

static authum_bool_t
authum_http_parse_key(const authum_char_t *text, authum_usize_t size, authum_u32_t *key)
{
    lh_net_ip4_t ip;
    if (!size || !lh_net_ip4_parse(text, size, lh_addr_of(ip)))
    {
        return authum_bool_false;
    }
    lh_memory_std_copy(key, ip.octets, sizeof lh_ptr_deref(key));
    return authum_bool_true;
}

authum_bool_t
authum_http_trusted_parse(authum_http_server_t *server, authum_char_t *error,
                          authum_u32_t error_size)
{
    const authum_char_t *p = server->options.trusted_proxies;
    server->trusted_count = 0;
    while (*p)
    {
        while (*p == ' ' || *p == ',' || *p == '\t')
        {
            ++p;
        }
        if (!*p)
        {
            break;
        }
        const authum_char_t *start = p;
        while (*p && *p != ' ' && *p != ',' && *p != '\t')
        {
            ++p;
        }
        authum_usize_t size = (authum_usize_t)(p - start);
        authum_usize_t address_size = size;
        authum_u32_t bits = 32;
        for (authum_usize_t i = 0; i < size; ++i)
        {
            if (start[i] == '/')
            {
                address_size = i;
                if (i + 1 == size || size - i - 1 > 2)
                {
                    bits = 99;
                    break;
                }
                bits = 0;
                for (authum_usize_t j = i + 1; j < size; ++j)
                {
                    if (start[j] < '0' || start[j] > '9')
                    {
                        bits = 99;
                        break;
                    }
                    bits = bits * 10 + (authum_u32_t)(start[j] - '0');
                }
                break;
            }
        }
        authum_u32_t key = 0;
        if (bits > 32 || !authum_http_parse_key(start, address_size, &key) ||
            lh_math_ge(server->trusted_count, AUTHUM_HTTP_MAX_TRUSTED))
        {
            authum_char_t entry[64];
            authum_usize_t n = size < sizeof entry - 1 ? size : sizeof entry - 1;
            lh_memory_std_copy(entry, start, n);
            entry[n] = 0;
            authum_unused(lh_str_ptr_format_cstr(
                error, error_size,
                "trusted_proxy entry '%s' must be an IPv4 address or CIDR block (at most %d entries)",
                entry, AUTHUM_HTTP_MAX_TRUSTED));
            return authum_bool_false;
        }
        authum_u32_t mask = bits ? 0xffffffffu << (32 - bits) : 0;
        server->trusted_net[server->trusted_count] = authum_http_host_order(key) & mask;
        server->trusted_mask[server->trusted_count] = mask;
        server->trusted_count = lh_math_add_one(server->trusted_count);
    }
    return authum_bool_true;
}

authum_bool_t
authum_http_trusted(const authum_http_server_t *server, authum_u32_t address)
{
    authum_u32_t value = authum_http_host_order(address);
    for (authum_u32_t i = 0; lh_math_lt(i, server->trusted_count); i = lh_math_add_one(i))
    {
        if ((value & server->trusted_mask[i]) == server->trusted_net[i])
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

authum_u32_t
authum_http_forwarded_client(const authum_http_server_t *server,
                             const lh_str_view_t *const *values, authum_usize_t count,
                             authum_u32_t proxy)
{
    /* Walk the hops right to left (last field, last entry first): every hop
     * added by a trusted proxy is skipped, the first untrusted one is the
     * client. A malformed entry ends the walk: nothing left of it can be
     * believed, so the last trusted hop seen is used. */
    authum_u32_t client = proxy;
    for (authum_usize_t field = count; field > 0; --field)
    {
        const authum_char_t *text = lh_str_view_get_data(values[field - 1]);
        authum_usize_t end = lh_str_view_get_size(values[field - 1]);
        while (1)
        {
            authum_usize_t start = end;
            while (start > 0 && text[start - 1] != ',')
            {
                --start;
            }
            authum_usize_t a = start;
            authum_usize_t b = end;
            while (a < b && (text[a] == ' ' || text[a] == '\t'))
            {
                ++a;
            }
            while (b > a && (text[b - 1] == ' ' || text[b - 1] == '\t'))
            {
                --b;
            }
            authum_u32_t hop;
            if (!authum_http_parse_key(text + a, b - a, &hop))
            {
                return client;
            }
            client = hop;
            if (!authum_http_trusted(server, hop))
            {
                return hop;
            }
            if (!start)
            {
                break;
            }
            end = start - 1;
        }
    }
    return client;
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
