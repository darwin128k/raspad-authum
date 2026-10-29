#include <authum/host/http/internal.h>
#include <authum/null.h>
#include <authum/memory.h>
#include <lh/os/net.h>
#include <lh/os/system.h>

void
authum_http_server_release(authum_http_server_t *server)
{
    for (authum_usize_t i = 0; lh_math_lt(i, AUTHUM_HTTP_MAX_SOURCES); i = lh_math_add_one(i))
    {
        if (lh_math_ne(server->roots[i], LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID))
        {
            lh_os_system_fs_file_close(server->roots[i]);
        }
    }
    if (lh_math_ne(server->listener, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID))
    {
        lh_os_system_net_socket_close(server->listener);
    }
    lh_os_wake_close(lh_addr_of(server->wake));
    lh_os_net_deinit();
    authum_memory_free(server->clients);
    authum_memory_free(server);
}

authum_http_server_t *
authum_http_server_create(const authum_http_options_t *options, const authum_threads_t *threads,

                          authum_char_t *error, authum_u32_t error_size)
{
    authum_http_server_t *server = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(server));
    if (!server)
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP state allocation failed"));
        return authum_null;
    }
    /* Winsock counts its starts: one for each server, ended in its release. */
    if (!lh_os_net_init())
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "network start: %s",
                                             authum_http_get_error_text()));
        authum_memory_free(server);
        return authum_null;
    }
    server->options = lh_ptr_deref(options);
    server->listener = LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID;
    lh_os_wake_init(lh_addr_of(server->wake));
    atomic_init(lh_addr_of(server->log_read), 0);
    atomic_init(lh_addr_of(server->log_write), 0);
    atomic_init(lh_addr_of(server->log_dropped), 0);
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
    if (!threads || !threads->ops || authum_null_eq((threads->ops)->start) ||
        authum_null_eq((threads->ops)->stop))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "host has no managed-thread API on this platform"));
        goto failed;
    }
    server->threads = lh_ptr_deref(threads);
    server->clients = authum_memory_alloc_zero(lh_cast_static(authum_usize_t, options->max_clients),
                                               sizeof lh_ptr_deref(server->clients));
    if (!server->clients)
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "client buffer allocation failed"));
        goto failed;
    }
    for (authum_i32_t i = 0; lh_math_lt(i, options->max_clients); i = lh_math_add_one(i))
    {
        server->clients[i].fd = LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID;
        server->clients[i].file = LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    for (authum_usize_t i = 0; lh_math_lt(i, options->source_count); i = lh_math_add_one(i))
    {
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
    if (!lh_os_wake_open(lh_addr_of(server->wake), 1000))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP wake signal: %s",
                                             authum_http_get_error_text()));
        goto failed;
    }
    server->listener = authum_http_listener_open(options, error, error_size);
    if (lh_math_eq(server->listener, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID))
    {
        goto failed;
    }
    server->thread =
        authum_thread_start(lh_addr_of(server->threads), authum_http_run, authum_http_wake, server,
                            options->cpu, AUTHUM_THREAD_BLOCK_SIGPIPE, error, error_size);
    if (!server->thread)
    {
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
    /* Both endpoints stay live until the worker has joined. */
    authum_unused(lh_os_wake_signal(lh_addr_of(server->wake)));
}

void
authum_http_server_shutdown(authum_http_server_t *server)
{
    if (!server || server->joined)
    {
        return;
    }
    authum_thread_stop(lh_addr_of(server->threads), server->thread);
    server->thread = authum_null;
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

void
authum_http_server_destroy(authum_http_server_t *server)
{
    if (!server)
    {
        return;
    }
    authum_http_server_shutdown(server);
    authum_http_server_release(server);
}
