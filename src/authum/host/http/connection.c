#include <authum/host/http/internal.h>
#include <lh/memory/std.h>

void
authum_http_client_close(authum_http_server_t *server, authum_http_client_t *client)
{
    if (lh_math_ge(client->fd, 0))
    {
        const authum_char_t *event =
            client->close_reason
                ? client->close_reason
                : (client->state && lh_math_eq(client->sent, client->output_size) &&
                           !client->remaining
                       ? "complete"
                       : "disconnect");
        authum_http_access_record(server, client, event);
        lh_os_system_net_socket_close(client->fd);
        --server->peers[client->ip_slot].active;
    }
    if (lh_math_ge(client->file, 0))
    {
        close(client->file);
    }
    lh_memory_std_set(client, 0, sizeof lh_ptr_deref(client));
    client->fd = client->file = -1;
}

void
authum_http_clients_accept(authum_http_server_t *server, authum_i64_t now)
{
    for (authum_usize_t attempt = 0; lh_math_lt(attempt, 32); attempt = lh_math_add_one(attempt))
    {
        authum_http_client_t peer = {.id = ++server->next_id, .born = now, .progress = now};
        authum_u32_t address;
        authum_i32_t socket =
            authum_http_socket_accept(server->listener, lh_addr_of(peer), lh_addr_of(address));
        if (lh_math_lt(socket, 0))
        {
            return;
        }

        authum_i32_t slot = -1;
        for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
             index = lh_math_add_one(index))
        {
            if (lh_math_lt(server->clients[index].fd, 0))
            {
                slot = index;
                break;
            }
        }
        authum_i32_t ip =
            lh_math_lt(slot, 0)
                ? -1
                : authum_http_peer_acquire(server, address, now, lh_addr_of(server->options));
        if (lh_math_lt(ip, 0))
        {
            authum_http_access_record(server, lh_addr_of(peer), "reject_limit");
            lh_os_system_net_socket_close(socket);
            continue;
        }

        peer.fd = socket;
        peer.file = -1;
        peer.ip_slot = ip;
        server->clients[slot] = peer;
        authum_http_access_record(server, lh_addr_of(server->clients[slot]), "connect");
    }
}

void
authum_http_client_read(authum_http_server_t *server, authum_http_client_t *client,
                        authum_i64_t now)
{
    authum_i64_t received =
        authum_http_socket_receive(client->fd, lh_math_add(client->input, client->used),
                                   lh_math_sub(sizeof client->input, client->used));
    if (lh_math_gt(received, 0))
    {
        client->used += lh_cast_static(authum_usize_t, received);
        client->progress = now;
        authum_http_request_read(client, server->roots, lh_addr_of(server->options));
        if (client->state)
        {
            authum_http_access_record(server, client, "request");
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
        sent = authum_http_socket_send(client->fd, lh_math_add(client->output, client->sent), size);
        if (lh_math_gt(sent, 0))
        {
            client->sent += lh_cast_static(authum_usize_t, sent);
        }
    }
    else if (lh_math_gt(client->remaining, 0))
    {
        if (lh_math_gt(lh_cast_static(off_t, budget), client->remaining))
        {
            budget = lh_cast_static(authum_usize_t, client->remaining);
        }
        sent = authum_http_socket_send_file(client->fd, client->file, lh_addr_of(client->offset),
                                            budget);
        if (lh_math_gt(sent, 0))
        {
            client->remaining -= sent;
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
