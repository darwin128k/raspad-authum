#include <authum/host/http/internal.h>

authum_i32_t
authum_http_peer_acquire(authum_http_server_t *server, authum_u32_t ip, authum_i64_t now,
                         const authum_http_options_t *options)
{
    authum_i32_t free_slot = -1;
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
    return lh_math_gt(lh_math_sub(now, client->progress), timeout) ||
           lh_math_gt(lh_math_sub(now, client->born), 300000) ||
           (!client->state && lh_math_gt(lh_math_sub(now, client->born), timeout));
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
