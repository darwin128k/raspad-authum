#include <authum/host/http/internal.h>
#include <lh/os/net/socket.h>

static void
prepare_connections(authum_http_server_t *server, lh_os_net_socket_poll_t *events, authum_i64_t now,
                    double tokens)
{
    events[0] =
        (lh_os_net_socket_poll_t){.handle = server->listener, .events = lh_os_net_socket_poll_read};
    events[1] = (lh_os_net_socket_poll_t){.handle = lh_os_wake_get_handle(lh_addr_of(server->wake)),
                                          .events = lh_os_net_socket_poll_read};
    for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
         index = lh_math_add_one(index))
    {
        authum_http_client_t *client = lh_addr_of(server->clients[index]);
        if (lh_math_ne(client->fd, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID) &&
            authum_http_client_expired(client, lh_addr_of(server->options), now))
        {
            client->close_reason = "timeout";
            authum_http_client_close(server, client);
        }
        lh_uint_t interest = client->state
                                 ? (lh_math_ge(tokens, 1) ? lh_os_net_socket_poll_write : 0)
                                 : lh_os_net_socket_poll_read;
        events[lh_math_add(index, 2)] =
            (lh_os_net_socket_poll_t){.handle = client->fd, .events = interest};
    }
}

static void
dispatch_connection(authum_http_server_t *server, authum_http_client_t *client, lh_uint_t events,
                    authum_i64_t now, double *tokens)
{
    if (lh_math_eq(client->fd, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID))
    {
        return;
    }
    if (lh_math_bit_and(events, lh_os_net_socket_poll_error))
    {
        authum_http_client_close(server, client);
        return;
    }
    if (!client->state && (lh_math_bit_and(events, lh_os_net_socket_poll_read)))
    {
        authum_http_client_read(server, client, now);
    }
    else if (client->state && (lh_math_bit_and(events, lh_os_net_socket_poll_write)) &&
             lh_math_ge(lh_ptr_deref(tokens), 1))
    {
        authum_http_client_write(server, client, now, tokens);
    }
    if (lh_math_ne(client->fd, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID) && (lh_math_bit_and(events, lh_os_net_socket_poll_hangup)))
    {
        authum_http_client_close(server, client);
    }
}

void
authum_http_run(authum_ptr arg)
{
    authum_http_server_t *server = arg;
    authum_i64_t last = authum_http_now_ms();
    double tokens = lh_math_div(server->options.bytes_per_second, 10.0);
    authum_usize_t cursor = 0;

    for (;;)
    {
        lh_os_net_socket_poll_t events[lh_math_add(AUTHUM_HTTP_MAX_CLIENTS, 2)];
        authum_i64_t now = authum_http_now_ms();
        authum_http_budget_refill(lh_addr_of(tokens), lh_addr_of(last), now,
                                  lh_addr_of(server->options));
        prepare_connections(server, events, now, tokens);
        lh_ssize_t ready = lh_os_net_socket_poll(
            events, lh_math_add(lh_cast_static(lh_usize_t, server->options.max_clients), 2), 20);
        if (lh_math_eq(ready, LH_OS_RESULT_INVALID))
        {
            break;
        }
        if (events[1].ready)
        {
            break;
        }

        /* Rotate the first connection to share the global budget fairly. */
        for (authum_usize_t count = 0;
             lh_math_lt(count, lh_cast_static(authum_usize_t, server->options.max_clients));
             count = lh_math_add_one(count))
        {
            authum_usize_t index =
                lh_math_mod((lh_math_add(cursor, count)), server->options.max_clients);
            dispatch_connection(server, lh_addr_of(server->clients[index]),
                                events[lh_math_add(index, 2)].ready, now, lh_addr_of(tokens));
        }
        cursor = lh_math_mod((lh_math_add(cursor, 1)), server->options.max_clients);
        if (lh_math_bit_and(events[0].ready, lh_os_net_socket_poll_read))
        {
            authum_http_clients_accept(server, now);
        }
    }

    for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
         index = lh_math_add_one(index))
    {
        server->clients[index].close_reason = "shutdown";
        authum_http_client_close(server, lh_addr_of(server->clients[index]));
    }
}
