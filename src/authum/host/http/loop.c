#include "internal.h"
#include <lh/os/net/socket.h>

static void
prepare_connections(authum_http_server_t *server, lh_os_net_socket_poll_t *events, authum_i64_t now,
                    double tokens)
{
    events[0] =
        (lh_os_net_socket_poll_t){.handle = server->listener, .events = lh_os_net_socket_poll_read};
    events[1] = (lh_os_net_socket_poll_t){.handle = lh_os_wake_get_handle(&server->wake),
                                          .events = lh_os_net_socket_poll_read};
    for (authum_i32_t index = 0; index < server->options.max_clients; ++index)
    {
        authum_http_client_t *client = &server->clients[index];
        if (client->fd >= 0 && authum_http_client_expired(client, &server->options, now))
        {
            client->close_reason = "timeout";
            authum_http_client_close(server, client);
        }
        lh_uint_t interest = client->state ? (tokens >= 1 ? lh_os_net_socket_poll_write : 0)
                                           : lh_os_net_socket_poll_read;
        events[index + 2] = (lh_os_net_socket_poll_t){.handle = client->fd, .events = interest};
    }
}

static void
dispatch_connection(authum_http_server_t *server, authum_http_client_t *client, lh_uint_t events,
                    authum_i64_t now, double *tokens)
{
    if (client->fd < 0)
    {
        return;
    }
    if (events & lh_os_net_socket_poll_error)
    {
        authum_http_client_close(server, client);
        return;
    }
    if (!client->state && (events & lh_os_net_socket_poll_read))
    {
        authum_http_client_read(server, client, now);
    }
    else if (client->state && (events & lh_os_net_socket_poll_write) && *tokens >= 1)
    {
        authum_http_client_write(server, client, now, tokens);
    }
    if (client->fd >= 0 && (events & lh_os_net_socket_poll_hangup))
    {
        authum_http_client_close(server, client);
    }
}

void
authum_http_run(void *arg)
{
    authum_http_server_t *server = arg;
    authum_i64_t last = authum_http_now_ms();
    double tokens = server->options.bytes_per_second / 10.0;
    authum_usize_t cursor = 0;

    for (;;)
    {
        lh_os_net_socket_poll_t events[AUTHUM_HTTP_MAX_CLIENTS + 2];
        authum_i64_t now = authum_http_now_ms();
        authum_http_budget_refill(&tokens, &last, now, &server->options);
        prepare_connections(server, events, now, tokens);
        lh_ssize_t ready =
            lh_os_net_socket_poll(events, (lh_usize_t)server->options.max_clients + 2, 20);
        if (ready == LH_OS_RESULT_INVALID)
        {
            break;
        }
        if (events[1].ready)
        {
            break;
        }

        /* Rotate the first connection to share the global budget fairly. */
        for (authum_usize_t count = 0; count < (authum_usize_t)server->options.max_clients; ++count)
        {
            authum_usize_t index = (cursor + count) % server->options.max_clients;
            dispatch_connection(server, &server->clients[index], events[index + 2].ready, now,
                                &tokens);
        }
        cursor = (cursor + 1) % server->options.max_clients;
        if (events[0].ready & lh_os_net_socket_poll_read)
        {
            authum_http_clients_accept(server, now);
        }
    }

    for (authum_i32_t index = 0; index < server->options.max_clients; ++index)
    {
        server->clients[index].close_reason = "shutdown";
        authum_http_client_close(server, &server->clients[index]);
    }
}
