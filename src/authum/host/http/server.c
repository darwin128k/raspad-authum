#define _GNU_SOURCE
#include "internal.h"
#include <fcntl.h>
#include <lh/os/system.h>

void
authum_http_server_release(authum_http_server_t *server)
{
    for (unsigned i = 0; i < 16; i++)
    {
        if (server->roots[i] >= 0)
        {
            close(server->roots[i]);
        }
    }
    if (server->listener >= 0)
    {
        lh_os_system_net_socket_close(server->listener);
    }
    lh_os_wake_close(&server->wake);
    free(server->clients);
    free(server);
}

authum_http_server_t *
authum_http_server_create(const authum_http_options_t *options, const authum_threads_t *threads,

                          char *error, unsigned error_size)
{
    authum_http_server_t *server = calloc(1, sizeof *server);
    if (!server)
    {
        snprintf(error, error_size, "HTTP state allocation failed");
        return NULL;
    }
    server->options = *options;
    server->listener = -1;
    lh_os_wake_init(&server->wake);
    atomic_init(&server->log_read, 0);
    atomic_init(&server->log_write, 0);
    atomic_init(&server->log_dropped, 0);
    for (unsigned i = 0; i < 16; i++)
    {
        server->roots[i] = -1;
    }
    if (!options->source_count || options->source_count > 16 || options->max_clients < 1 ||
        options->max_clients > 256 || options->cpu < -1 || options->cpu >= 1024)
    {
        snprintf(error, error_size, "invalid HTTP configuration bounds");
        goto failed;
    }
    if (!threads || !threads->ops || !AUTHUM_ABI_HAS(threads->ops, start) ||
        !AUTHUM_ABI_HAS(threads->ops, stop))
    {
        snprintf(error, error_size, "host has no managed-thread API; update Authum");
        goto failed;
    }
    server->threads = *threads;
    server->clients = calloc((size_t)options->max_clients, sizeof *server->clients);
    if (!server->clients)
    {
        snprintf(error, error_size, "client buffer allocation failed");
        goto failed;
    }
    for (int i = 0; i < options->max_clients; i++)
    {
        server->clients[i].fd = server->clients[i].file = -1;
    }
    for (authum_usize_t i = 0; i < options->source_count; ++i)
    {
        server->roots[i] =
            open(options->sources[i], O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (server->roots[i] < 0)
        {
            snprintf(error, error_size, "cannot open source[%u] '%.140s': %s", i + 1,
                     options->sources[i], strerror(errno));
            goto failed;
        }
    }
    if (!lh_os_wake_open(&server->wake, 1000))
    {
        snprintf(error, error_size, "HTTP wake signal: %s",
                 strerror((int)lh_os_system_get_last_error_code()));
        goto failed;
    }
    server->listener = authum_http_listener_open(options, error, error_size);
    if (server->listener < 0)
    {
        goto failed;
    }
    server->thread =
        authum_thread_start(&server->threads, authum_http_run, authum_http_wake, server,
                            options->cpu, AUTHUM_THREAD_BLOCK_SIGPIPE, error, error_size);
    if (!server->thread)
    {
        goto failed;
    }
    return server;
failed:
    authum_http_server_release(server);
    return NULL;
}

void
authum_http_wake(void *arg)
{
    authum_http_server_t *server = arg;
    /* Both endpoints stay live until the worker has joined. */
    (void)lh_os_wake_signal(&server->wake);
}

void
authum_http_server_shutdown(authum_http_server_t *server)
{
    if (!server || server->joined)
    {
        return;
    }
    authum_thread_stop(&server->threads, server->thread);
    server->thread = NULL;
    server->joined = 1;
}

authum_usize_t
authum_http_server_memory(const authum_http_server_t *server)
{
    return server ? sizeof *server +
                        (authum_usize_t)server->options.max_clients * sizeof *server->clients
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
