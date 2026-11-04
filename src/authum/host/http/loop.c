#include <authum/host/http/internal.h>
#include <lh/os/tick.h>
#include <authum/null.h>

void (*authum_http_test_step_hook)(authum_http_server_t *server) = authum_null;

static void
prepare_connections(authum_http_server_t *server, lh_os_net_socket_poll_t *events, authum_i64_t now,
                    double tokens)
{
    events[0] =
        (lh_os_net_socket_poll_t){.handle = server->listener, .events = lh_os_net_socket_poll_read};
    events[1] = (lh_os_net_socket_poll_t){.handle = lh_os_wake_get_handle(lh_addr_of(server->wake)),
                                          .events = lh_os_net_socket_poll_read};
    server->deferred_readers = 0;
    const lh_u64_t now_us = lh_os_tick_us();
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
        /* Batch a request that keeps arriving in fragments (a drip): after a
         * few, its next bytes are read one short moment later, together,
         * instead of one wake-up per packet. */
        if (!client->state && lh_math_gt(client->partials, AUTHUM_HTTP_FREE_PARTIALS))
        {
            const lh_u64_t elapsed = lh_math_sub(now_us, client->partial_us);
            if (lh_math_lt(elapsed, AUTHUM_HTTP_MIN_STEP_US))
            {
                const lh_u64_t due = lh_math_sub(AUTHUM_HTTP_MIN_STEP_US, elapsed);
                if (!server->deferred_readers || lh_math_lt(due, server->deferred_due_us))
                {
                    server->deferred_due_us = due;
                }
                interest = 0;
                server->deferred_readers = lh_math_add_one(server->deferred_readers);
            }
        }
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

/* The release call of an abandoned worker loop (lh_exec_loop_try_abandon): it
 * runs on the worker after its last step. The owner may still hold the server
 * (between its shutdown and its destroy): whoever of the two lets go last frees it. */
void
authum_http_worker_release(authum_ptr arg)
{
    authum_http_server_t *server = arg;
    if (lh_math_eq(atomic_fetch_sub_explicit(lh_addr_of(server->refs), 1, memory_order_acq_rel), 1))
    {
        authum_http_server_release_resources(server);
    }
}

void
authum_http_wait(authum_ptr arg, lh_u32_t timeout_ms)
{
    authum_http_server_t *server = arg;
    const lh_usize_t count =
        lh_math_add(lh_cast_static(lh_usize_t, server->options.max_clients), 2);

    /* A reader that is still sending its request in pieces is left out of the
     * poll for a moment (see prepare_connections); wake when the first one is
     * due (rounded up, so it is not taken back early and re-deferred). Only
     * drips wait: other sockets in the poll still wake it at once, so on a
     * coarse timer (Windows' default 15.6 ms) the rounding delays nobody else. */
    if (server->deferred_readers)
    {
        const lh_u64_t due_ms = lh_math_div(lh_math_add(server->deferred_due_us, 999u), 1000u);
        const lh_u32_t cap = lh_math_lt(due_ms, 1u) ? 1u : lh_cast_static(lh_u32_t, due_ms);
        if (lh_math_gt(timeout_ms, cap))
        {
            timeout_ms = cap;
        }
    }
    const lh_ssize_t polled = lh_os_net_socket_poll(server->events, count, timeout_ms);
    if (lh_math_eq(polled, LH_OS_RESULT_INVALID))
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
    if (authum_http_test_step_hook)
    {
        authum_http_test_step_hook(server);
    }
    authum_i64_t now = authum_http_now_ms();

    if (lh_exec_loop_is_last_step(server->loop))
    {
        for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
             index = lh_math_add_one(index))
        {
            server->clients[index].close_reason = "shutdown";
            authum_http_client_close(server, lh_addr_of(server->clients[index]));
        }
        authum_http_refusals_flush(server, lh_math_add(now, 1000));
        /* Free the port here: after a stop that stopped waiting, nobody else will. */
        if (lh_math_ne(server->listener, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID))
        {
            lh_os_system_net_socket_close(server->listener);
            server->listener = LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID;
        }
        return;
    }
    authum_http_refusals_flush(server, now);
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
