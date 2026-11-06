#include <authum/host/http/internal.h>
#include <authum/null.h>
#include <authum/memory.h>
#include <lh/os/net.h>
#include <lh/os/system.h>
#include <lh/os/thread.h>
#include <lh/core/runtime/logger.h>

void
authum_http_server_release_resources(authum_http_server_t *server)
{
    for (authum_usize_t i = 0; lh_math_lt(i, AUTHUM_HTTP_MAX_SOURCES); i = lh_math_add_one(i))
    {
        if (lh_math_ne(server->roots[i], LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID))
        {
            lh_os_system_fs_file_close(server->roots[i]);
        }
    }
    lh_os_net_socket_close(lh_addr_of(server->listener));
    lh_os_wake_close(lh_addr_of(server->wake));
    lh_os_net_deinit();
    authum_memory_free(server->clients);
    authum_memory_free(server);
}

void
authum_http_server_release(authum_http_server_t *server)
{
    /* A loop that never started is just freed; a running one is joined first. */
    lh_rail_loop_destroy(server->loop);
    authum_memory_free(server->orphan);
    authum_http_server_release_resources(server);
}

/** What the worker's first poll watches: the listener and the wake signal. */
void
authum_http_prepare_first_poll(authum_http_server_t *server)
{
    lh_os_net_socket_get_poll(lh_addr_of(server->listener), lh_os_net_socket_poll_read,
                              lh_addr_of(server->events[0]));
    lh_os_wake_get_poll(lh_addr_of(server->wake), lh_addr_of(server->events[1]));
    for (authum_i32_t index = 0; lh_math_lt(index, server->options.max_clients);
         index = lh_math_add_one(index))
    {
        lh_os_net_socket_poll_init_empty(lh_addr_of(server->events[lh_math_add(index, 2)]));
    }
}

authum_http_server_t *
authum_http_server_create(const authum_http_options_t *options, const authum_char_t *tag,
                          authum_char_t *error, authum_u32_t error_size)
{
    authum_http_server_t *server = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(server));
    if (!server)
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP state allocation failed"));
        return authum_null;
    }
    /* Winsock counts its starts: one for each server, ended in its release. */
    server->orphan = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(server->orphan));
    if (!server->orphan)
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP state allocation failed"));
        authum_memory_free(server);
        return authum_null;
    }
    authum_http_error_reset();
    if (!lh_os_net_init())
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "network start: %s",
                                             authum_http_get_error_text()));
        authum_memory_free(server->orphan);
        authum_memory_free(server);
        return authum_null;
    }
    server->options = lh_ptr_deref(options);
    /* The access log says which module served the request. The tag is copied
     * before the worker starts: a worker that outlives a bounded stop must not
     * read the unloaded module's image. */
    if (tag)
    {
        authum_unused(lh_str_ptr_format_cstr(server->tag_text, sizeof server->tag_text, "%s", tag));
        server->tag = server->tag_text;
    }
    lh_os_net_socket_init(lh_addr_of(server->listener));
    lh_os_wake_init(lh_addr_of(server->wake));
    atomic_init(lh_addr_of(server->log_read), 0);
    atomic_init(lh_addr_of(server->log_write), 0);
    atomic_init(lh_addr_of(server->log_dropped), 0);
    atomic_init(lh_addr_of(server->refs), 2);
    for (authum_usize_t i = 0; lh_math_lt(i, AUTHUM_HTTP_MAX_SOURCES); i = lh_math_add_one(i))
    {
        server->roots[i] = LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    if (!options->source_count || lh_math_gt(options->source_count, AUTHUM_HTTP_MAX_SOURCES) ||
        lh_math_lt(options->max_clients, 1) ||
        lh_math_gt(options->max_clients, AUTHUM_HTTP_MAX_CLIENTS) || lh_math_lt(options->cpu, -1) ||
        lh_math_ge(options->cpu, 1024))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "invalid HTTP configuration bounds"));
        goto failed;
    }
    server->clients = authum_memory_alloc_zero(lh_cast_static(authum_usize_t, options->max_clients),
                                               sizeof lh_ptr_deref(server->clients));
    if (!server->clients)
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "client buffer allocation failed"));
        goto failed;
    }
    for (authum_i32_t i = 0; lh_math_lt(i, options->max_clients); i = lh_math_add_one(i))
    {
        lh_os_net_socket_init(lh_addr_of(server->clients[i].fd));
        server->clients[i].file = LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
        server->clients[i].ip_slot = -1;
    }
    if (!authum_http_trusted_parse(server, error, error_size))
    {
        goto failed;
    }
    for (authum_usize_t i = 0; lh_math_lt(i, options->source_count); i = lh_math_add_one(i))
    {
        authum_http_error_reset();
        server->roots[i] = lh_os_system_fs_file_open_root(options->sources[i]);
        if (lh_math_eq(server->roots[i], LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID))
        {
            authum_unused(lh_str_ptr_format_cstr(
                error, error_size, "cannot open source[%u] '%.140s': %s",
                lh_cast_static(lh_uint_t, lh_math_add(i, 1)), options->sources[i],
                authum_http_get_error_text()));
            goto failed;
        }
    }
    authum_http_error_reset();
    if (!lh_os_wake_open(lh_addr_of(server->wake), 1000))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP wake signal: %s",
                                             authum_http_get_error_text()));
        goto failed;
    }
    if (!authum_http_listener_open(lh_addr_of(server->listener), options, error, error_size))
    {
        goto failed;
    }
    server->tokens = lh_math_div(options->bytes_per_second, 10.0);
    server->last_refill = authum_http_now_ms();
    authum_http_prepare_first_poll(server);
    /* The poll timeout of the worker: client timeouts and the byte budget tick on it. */
    authum_http_error_reset();
    server->loop = lh_rail_loop_try_create_waiting(20U, authum_http_step, server, authum_http_wait,
                                                 authum_http_wake, server);
    if (authum_null_eq(server->loop) ||
        !lh_rail_loop_try_start_on(server->loop, options->cpu, lh_os_thread_block_sigpipe))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "cannot start the HTTP worker: %s",
                                             authum_http_get_error_text()));
        goto failed;
    }
    return server;
failed:
    authum_http_server_release(server);
    return authum_null;
}

void
authum_http_wake(authum_ptr arg)
{
    authum_http_server_t *server = arg;
    /* Called by the owner's stop and wakes only, never after an abandon: from
     * then on the owner reaps the loop, which wakes nothing. */
    authum_unused(lh_os_wake_signal(lh_addr_of(server->wake)));
}

void
authum_http_server_shutdown(authum_http_server_t *server)
{
    if (!server || server->joined || server->abandoned)
    {
        return;
    }
    /* The owner is the game thread: it waits a bounded time. A worker stuck in
     * a file call (a source on a slow network share) is given up instead of
     * freezing the server on unload: the loop has it free its hold on the
     * server when it ends, and the reaper frees the loop later. */
    if (server->loop && !lh_rail_loop_try_stop(server->loop, AUTHUM_HTTP_STOP_TIMEOUT_MS) &&
        lh_rail_loop_try_abandon(server->loop, authum_http_worker_release, server))
    {
        server->abandoned = authum_bool_true;
        authum_unused(lh_logger_log(
            lh_runtime_logger_get_mut(), lh_logger_level_warning,
            "http event=stop_timeout module=%s port=%d waited_ms=%d: the HTTP worker "
            "is blocked (often a source on a network drive); it is left to finish on "
            "its own and frees its memory when it does",
            server->tag ? server->tag : "-", server->options.port,
            lh_cast_static(int, AUTHUM_HTTP_STOP_TIMEOUT_MS)));
        return;
    }
    lh_rail_loop_destroy(server->loop);
    server->loop = authum_null;
    server->joined = 1;
}

authum_usize_t
authum_http_server_memory(const authum_http_server_t *server)
{
    return server ? lh_math_add(
                        sizeof lh_ptr_deref(server),
                        lh_math_mul(lh_cast_static(authum_usize_t, server->options.max_clients),
                                    sizeof lh_ptr_deref(server->clients)))
                  : 0;
}

/* Game thread only: loops of abandoned workers not yet reaped. */
static authum_http_orphan_t *g_orphans = authum_null;

authum_usize_t
authum_http_host_reap(void)
{
    authum_usize_t left = 0;
    authum_http_orphan_t **slot = lh_addr_of(g_orphans);
    while (lh_ptr_deref(slot))
    {
        authum_http_orphan_t *orphan = lh_ptr_deref(slot);
        /* Never waits: the loop is freed only once its thread has ended. */
        if (lh_rail_loop_try_reap(orphan->loop))
        {
            lh_ptr_deref(slot) = orphan->next;
            authum_memory_free(orphan);
            continue;
        }
        left = lh_math_add_one(left);
        slot = lh_addr_of(orphan->next);
    }
    return left;
}

void
authum_http_server_destroy(authum_http_server_t *server)
{
    if (!server)
    {
        return;
    }
    authum_http_server_shutdown(server);
    if (server->abandoned)
    {
        /* The worker may still use the server: the last of the two frees it,
         * and the reaper frees the loop once its thread has ended. */
        authum_http_orphan_t *orphan = server->orphan;
        orphan->loop = server->loop;
        orphan->next = g_orphans;
        g_orphans = orphan;
        if (lh_math_eq(atomic_fetch_sub_explicit(lh_addr_of(server->refs), 1, memory_order_acq_rel),
                       1))
        {
            authum_http_server_release_resources(server);
        }
        return;
    }
    authum_http_server_release(server);
}
