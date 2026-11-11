#include <authum/host/http/internal.h>
#include <lh/os/tick.h>
#include <lh/core/memory/std.h>
#include <lh/core/char/map.h>

lh_os_fs_file_t *
authum_http_client_get_file_mut(authum_http_client_t *client)
{
    return lh_addr_of(client->file);
}

void
authum_http_client_close(authum_http_server_t *server, authum_http_client_t *client)
{
    if (lh_os_net_socket_is_valid(lh_addr_of(client->fd)))
    {
        const authum_char_t *event =
            client->close_reason
                ? client->close_reason
                : (client->state && lh_math_eq(client->sent, client->output_size) &&
                           !client->remaining
                       ? "complete"
                       : "disconnect");
        authum_http_access_close(server, client, event, authum_http_now_ms());
        lh_os_net_socket_close(lh_addr_of(client->fd));
        if (client->pending && lh_math_ge(client->ip_slot, 0))
        {
            lh_http_limit_peer_drop_pending(lh_addr_of(server->peers[client->ip_slot]));
        }
        authum_http_peer_release(server, client->ip_slot);
    }
    lh_os_fs_file_deinit(authum_http_client_get_file_mut(client));
    lh_memory_std_set(client, 0, sizeof lh_ptr_deref(client));
    lh_os_net_socket_init(lh_addr_of(client->fd));
    lh_os_fs_file_init(authum_http_client_get_file_mut(client));
    client->ip_slot = -1;
}

/* With every slot taken, the connection that has waited longest without
 * finishing its request makes room, if it is old enough not to be a client
 * whose request is simply still on the way. */
authum_i32_t
evict_oldest_reader(authum_http_server_t *server, authum_i64_t now)
{
    authum_i32_t victim = -1;
    for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
         index = lh_math_add_one(index))
    {
        const authum_http_client_t *client = lh_addr_of(server->clients[index]);
        if (lh_os_net_socket_is_valid(lh_addr_of(client->fd)) && !client->state &&
            lh_math_ge(lh_math_sub(now, client->born), AUTHUM_HTTP_EVICT_MIN_AGE_MS) &&
            (lh_math_lt(victim, 0) || lh_math_lt(client->born, server->clients[victim].born)))
        {
            victim = index;
        }
    }
    if (lh_math_ge(victim, 0))
    {
        server->clients[victim].close_reason = "evicted";
        authum_http_refusal_count(server, lh_addr_of(server->refusals.evicted), now);
        authum_http_client_close(server, lh_addr_of(server->clients[victim]));
    }
    return victim;
}

void
authum_http_clients_accept(authum_http_server_t *server, authum_i64_t now)
{
    for (authum_usize_t attempt = 0; lh_math_lt(attempt, 32); attempt = lh_math_add_one(attempt))
    {
        authum_http_client_t peer = {.born = now, .progress = now};
        authum_u32_t address;
        if (!authum_http_socket_accept(lh_addr_of(server->listener), lh_addr_of(peer),
                                       lh_addr_of(address)))
        {
            return;
        }
        peer.id = ++server->next_id;
        /* A trusted proxy is not limited itself: the limits apply to the
         * client it names, once its request has arrived. */
        authum_bool_t proxied = authum_http_trusted(server, address);
        authum_i32_t ip = -1;
        if (!proxied)
        {
            ip = authum_http_peer_acquire(server, address, now, lh_addr_of(server->options));
            if (lh_math_lt(ip, 0))
            {
                authum_http_refusal_count(server, lh_addr_of(server->refusals.limit), now);
                lh_os_net_socket_close(lh_addr_of(peer.fd));
                continue;
            }
            /* Connections that have not sent their request yet are capped per
             * address, so one address cannot hold the slots. */
            if (lh_math_ge(lh_http_limit_peer_get_pending(lh_addr_of(server->peers[ip])),
                           lh_cast_static(authum_u32_t, server->options.per_ip_pending)))
            {
                authum_http_peer_release(server, ip);
                authum_http_refusal_count(server, lh_addr_of(server->refusals.pending), now);
                lh_os_net_socket_close(lh_addr_of(peer.fd));
                continue;
            }
        }
        authum_i32_t slot = -1;
        for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
             index = lh_math_add_one(index))
        {
            if (!lh_os_net_socket_is_valid(lh_addr_of(server->clients[index].fd)))
            {
                slot = index;
                break;
            }
        }
        if (lh_math_lt(slot, 0))
        {
            slot = evict_oldest_reader(server, now);
        }
        if (lh_math_lt(slot, 0))
        {
            authum_http_peer_release(server, ip);
            authum_http_refusal_count(server, lh_addr_of(server->refusals.slots), now);
            lh_os_net_socket_close(lh_addr_of(peer.fd));
            continue;
        }

        lh_os_fs_file_init(authum_http_client_get_file_mut(lh_addr_of(peer)));
        peer.ip_slot = ip;
        peer.address = address;
        peer.proxied = proxied;
        if (lh_math_ge(ip, 0))
        {
            peer.pending = authum_bool_true;
            lh_http_limit_peer_add_pending(lh_addr_of(server->peers[ip]));
        }
        server->clients[slot] = peer;
        if (lh_math_ge(server->options.access_log, 2))
        {
            authum_http_access_record(server, lh_addr_of(server->clients[slot]), "connect");
        }
    }
}

void
authum_http_client_partial(authum_http_client_t *client)
{
    client->partial_us = lh_os_tick_us();
    if (lh_math_lt(client->partials, 0xffffu))
    {
        client->partials = lh_math_add_one(client->partials);
    }
}

void
authum_http_client_read(authum_http_server_t *server, authum_http_client_t *client,
                        authum_i64_t now)
{
    const authum_usize_t before = client->used;
    authum_i64_t received =
        authum_http_socket_receive(lh_addr_of(client->fd), lh_math_add(client->input, client->used),
                                   lh_math_sub(sizeof client->input, client->used));
    if (lh_math_gt(received, 0))
    {
        client->used += lh_cast_static(authum_usize_t, received);
        client->progress = now;
        /* Parse only when a line has ended (or the buffer is full): a request
         * dripped byte by byte then costs one scan per line, not one per byte. */
        if (!lh_str_ptr_find_of_char(lh_math_add(client->input, before),
                                     lh_cast_static(authum_usize_t, received), lh_char_map_lf) &&
            lh_math_lt(client->used, sizeof client->input))
        {
            authum_http_client_partial(client);
            return;
        }
        const authum_i32_t slot = client->ip_slot;
        authum_http_request_read(server, client, now);
        if (client->state)
        {
            /* The request has arrived: no longer counted as unfinished. */
            if (client->pending && lh_math_ge(slot, 0))
            {
                lh_http_limit_peer_drop_pending(lh_addr_of(server->peers[slot]));
            }
            client->pending = authum_bool_false;
            if (lh_math_ge(server->options.access_log, 2))
            {
                authum_http_access_record(server, client, "request");
            }
        }
        else
        {
            authum_http_client_partial(client);
        }
    }
    else if (lh_math_ne(received, AUTHUM_HTTP_IO_PENDING))
    {
        authum_http_client_close(server, client);
    }
}

void
authum_http_client_write(authum_http_server_t *server, authum_http_client_t *client,
                         authum_i64_t now, double *tokens)
{
    authum_usize_t budget = lh_math_gt(lh_ptr_deref(tokens), 65536)
                                ? 65536
                                : lh_cast_static(authum_usize_t, lh_ptr_deref(tokens));
    authum_i64_t sent = 0;
    if (lh_math_lt(client->sent, client->output_size))
    {
        authum_usize_t remaining = lh_math_sub(client->output_size, client->sent);
        authum_usize_t size = lh_math_lt(remaining, budget) ? remaining : budget;
        sent = authum_http_socket_send(lh_addr_of(client->fd),
                                       lh_math_add(client->output, client->sent), size);
        if (lh_math_gt(sent, 0))
        {
            client->sent += lh_cast_static(authum_usize_t, sent);
        }
    }
    else if (lh_math_gt(client->remaining, 0))
    {
        if (lh_math_gt(lh_cast_static(lh_u64_t, budget), client->remaining))
        {
            budget = lh_cast_static(authum_usize_t, client->remaining);
        }
        sent = authum_http_socket_send_file(lh_addr_of(client->fd),
                                            authum_http_client_get_file_mut(client),
                                            lh_addr_of(client->offset), budget);
        if (lh_math_gt(sent, 0))
        {
            client->remaining -= lh_cast_static(lh_u64_t, sent);
            client->body_bytes += lh_cast_static(authum_u64_t, sent);
        }
        else if (!sent)
        {
            authum_http_client_close(server, client);
            return;
        }
    }

    if (lh_math_gt(sent, 0))
    {
        lh_ptr_deref(tokens) -= sent;
        client->progress = now;
    }
    authum_bool_t complete = lh_math_eq(client->sent, client->output_size) && !client->remaining;
    if (lh_math_eq(sent, AUTHUM_HTTP_IO_ERROR) || complete)
    {
        authum_http_client_close(server, client);
    }
}
