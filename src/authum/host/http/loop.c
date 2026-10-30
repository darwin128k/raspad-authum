#include <authum/host/http/internal.h>

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
authum_http_wait(authum_ptr arg, lh_u32_t timeout_ms)
{
    authum_http_server_t *server = arg;
    const lh_usize_t count =
        lh_math_add(lh_cast_static(lh_usize_t, server->options.max_clients), 2);

    if (lh_math_eq(lh_os_net_socket_poll(server->events, count, timeout_ms), LH_OS_RESULT_INVALID))
    {
        /* Nothing is known ready: the next step only prepares the next poll. */
        for (lh_usize_t index = 0; lh_math_lt(index, count); index = lh_math_add_one(index))
        {
            server->events[index].ready = 0;
        }
    }
}

void
authum_http_step(authum_ptr arg)
{
    authum_http_server_t *server = arg;
    authum_i64_t now = authum_http_now_ms();

    if (lh_os_loop_is_last_step(server->loop))
    {
        for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
             index = lh_math_add_one(index))
        {
            server->clients[index].close_reason = "shutdown";
            authum_http_client_close(server, lh_addr_of(server->clients[index]));
        }
        return;
    }
    authum_http_budget_refill(lh_addr_of(server->tokens), lh_addr_of(server->last_refill), now,
                              lh_addr_of(server->options));
    if (server->events[1].ready)
    {
        authum_unused(lh_os_wake_reset(lh_addr_of(server->wake)));
    }
    /* Rotate the first connection to share the global budget fairly. */
    for (authum_usize_t count = 0;
         lh_math_lt(count, lh_cast_static(authum_usize_t, server->options.max_clients));
         count = lh_math_add_one(count))
    {
        authum_usize_t index =
            lh_math_mod((lh_math_add(server->cursor, count)), server->options.max_clients);
        dispatch_connection(server, lh_addr_of(server->clients[index]),
                            server->events[lh_math_add(index, 2)].ready, now,
                            lh_addr_of(server->tokens));
    }
    server->cursor = lh_math_mod((lh_math_add(server->cursor, 1)), server->options.max_clients);
    if (lh_math_bit_and(server->events[0].ready, lh_os_net_socket_poll_read))
    {
        authum_http_clients_accept(server, now);
    }
    prepare_connections(server, server->events, now, server->tokens);
}
