#include "internal.h"

int
authum_http_peer_acquire(authum_http_server_t *server, authum_u32_t ip, authum_i64_t now,
                         const authum_http_options_t *options)
{
    int free_slot = -1;
    for (int i = 0; i < AUTHUM_HTTP_MAX_PEERS; i++)
    {
        authum_http_peer_t *peer = &server->peers[i];
        if (peer->used && peer->ip == ip)
        {
            if (now - peer->window >= 1000)
            {
                peer->window = now;
                peer->count = 0;
            }
            if (peer->active >= (authum_u32_t)options->per_ip ||
                peer->count >= (authum_u32_t)options->requests_per_second)
            {
                return -1;
            }
            ++peer->count;
            ++peer->active;
            return i;
        }
        if (!peer->used || (!peer->active && now - peer->window >= 1000))
        {
            free_slot = i;
        }
    }
    if (free_slot < 0)
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
    authum_i64_t timeout = options->timeout_seconds * 1000LL;
    return now - client->progress > timeout || now - client->born > 300000 ||
           (!client->state && now - client->born > timeout);
}

void
authum_http_budget_refill(double *tokens, authum_i64_t *last, authum_i64_t now,
                          const authum_http_options_t *options)
{
    double burst = options->bytes_per_second / 10.0;
    *tokens += (now - *last) * (double)options->bytes_per_second / 1000.0;
    *last = now;
    if (*tokens > burst)
    {
        *tokens = burst;
    }
}
