#include "internal.h"

void
authum_http_client_close(authum_http_server_t *server, authum_http_client_t *client)
{
    if (client->fd >= 0)
    {
        const char *event =
            client->close_reason
                ? client->close_reason
                : (client->state && client->sent == client->output_size && !client->remaining
                       ? "complete"
                       : "disconnect");
        authum_http_access_record(server, client, event);
        lh_os_system_net_socket_close(client->fd);
        --server->peers[client->ip_slot].active;
    }
    if (client->file >= 0)
    {
        close(client->file);
    }
    memset(client, 0, sizeof *client);
    client->fd = client->file = -1;
}

void
authum_http_clients_accept(authum_http_server_t *server, authum_i64_t now)
{
    for (authum_usize_t attempt = 0; attempt < 32; ++attempt)
    {
        authum_http_client_t peer = {.id = ++server->next_id, .born = now, .progress = now};
        authum_u32_t address;
        int socket = authum_http_socket_accept(server->listener, &peer, &address);
        if (socket < 0)
        {
            return;
        }

        authum_i32_t slot = -1;
        for (authum_i32_t index = 0; index < server->options.max_clients; ++index)
        {
            if (server->clients[index].fd < 0)
            {
                slot = index;
                break;
            }
        }
        authum_i32_t ip =
            slot < 0 ? -1 : authum_http_peer_acquire(server, address, now, &server->options);
        if (ip < 0)
        {
            authum_http_access_record(server, &peer, "reject_limit");
            lh_os_system_net_socket_close(socket);
            continue;
        }

        peer.fd = socket;
        peer.file = -1;
        peer.ip_slot = ip;
        server->clients[slot] = peer;
        authum_http_access_record(server, &server->clients[slot], "connect");
    }
}

void
authum_http_client_read(authum_http_server_t *server, authum_http_client_t *client,
                        authum_i64_t now)
{
    authum_i64_t received = authum_http_socket_receive(client->fd, client->input + client->used,
                                                       sizeof client->input - client->used);
    if (received > 0)
    {
        client->used += (authum_usize_t)received;
        client->progress = now;
        authum_http_request_read(client, server->roots, &server->options);
        if (client->state)
        {
            authum_http_access_record(server, client, "request");
        }
    }
    else if (received != AUTHUM_HTTP_IO_PENDING)
    {
        authum_http_client_close(server, client);
    }
}

void
authum_http_client_write(authum_http_server_t *server, authum_http_client_t *client,
                         authum_i64_t now, double *tokens)
{
    authum_usize_t budget = *tokens > 65536 ? 65536 : (authum_usize_t)*tokens;
    authum_i64_t sent = 0;
    if (client->sent < client->output_size)
    {
        authum_usize_t remaining = client->output_size - client->sent;
        authum_usize_t size = remaining < budget ? remaining : budget;
        sent = authum_http_socket_send(client->fd, client->output + client->sent, size);
        if (sent > 0)
        {
            client->sent += (authum_usize_t)sent;
        }
    }
    else if (client->remaining > 0)
    {
        if ((off_t)budget > client->remaining)
        {
            budget = (authum_usize_t)client->remaining;
        }
        sent = authum_http_socket_send_file(client->fd, client->file, &client->offset, budget);
        if (sent > 0)
        {
            client->remaining -= sent;
            client->body_bytes += (authum_u64_t)sent;
        }
        else if (!sent)
        {
            authum_http_client_close(server, client);
            return;
        }
    }

    if (sent > 0)
    {
        *tokens -= sent;
        client->progress = now;
    }
    authum_bool_t complete = client->sent == client->output_size && !client->remaining;
    if (sent == AUTHUM_HTTP_IO_ERROR || complete)
    {
        authum_http_client_close(server, client);
    }
}
