#include "internal.h"

static void
prepare_connections(authum_http_server_t *server, struct pollfd *events, authum_i64_t now,
                    double tokens)
{
    events[0] = (struct pollfd){.fd = server->listener, .events = POLLIN};
    events[1] = (struct pollfd){.fd = server->wake[0], .events = POLLIN};
    for (authum_i32_t index = 0; index < server->options.max_clients; ++index)
    {
        authum_http_client_t *client = &server->clients[index];
        if (client->fd >= 0 && authum_http_client_expired(client, &server->options, now))
        {
            client->close_reason = "timeout";
            authum_http_client_close(server, client);
        }
        short interest = client->state ? (tokens >= 1 ? POLLOUT : 0) : POLLIN;
        events[index + 2] = (struct pollfd){.fd = client->fd, .events = interest};
    }
}

static void
dispatch_connection(authum_http_server_t *server, authum_http_client_t *client, short events,
                    authum_i64_t now, double *tokens)
{
    if (client->fd < 0)
    {
        return;
    }
    if (events & (POLLERR | POLLNVAL))
    {
        authum_http_client_close(server, client);
        return;
    }
    if (!client->state && (events & POLLIN))
    {
        authum_http_client_read(server, client, now);
    }
    else if (client->state && (events & POLLOUT) && *tokens >= 1)
    {
        authum_http_client_write(server, client, now, tokens);
    }
    if (client->fd >= 0 && (events & POLLHUP))
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
        struct pollfd events[258];
        authum_i64_t now = authum_http_now_ms();
        authum_http_budget_refill(&tokens, &last, now, &server->options);
        prepare_connections(server, events, now, tokens);
        int ready = poll(events, (nfds_t)server->options.max_clients + 2, 20);
        if (ready < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }
        if (events[1].revents)
        {
            break;
        }

        /* Rotate the first connection to share the global budget fairly. */
        for (authum_usize_t count = 0; count < (authum_usize_t)server->options.max_clients; ++count)
        {
            authum_usize_t index = (cursor + count) % server->options.max_clients;
            dispatch_connection(server, &server->clients[index], events[index + 2].revents, now,
                                &tokens);
        }
        cursor = (cursor + 1) % server->options.max_clients;
        if (events[0].revents & POLLIN)
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
