#include "internal.h"

static authum_bool_t
owns_server(const authum_http_owner_t *owner, const authum_http_server_t *server)
{
    if (!owner || !server)
    {
        return authum_bool_false;
    }
    for (const authum_http_server_t *entry = owner->head; entry; entry = entry->next)
    {
        if (entry == server)
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

static authum_bool_t
terminated(const authum_char_t *string, authum_usize_t capacity)
{
    return memchr(string, 0, capacity) != NULL;
}

static authum_bool_t
valid_options(const authum_http_options_t *options)
{
    if (!options || options->size < sizeof *options || !options->source_count ||
        options->source_count > AUTHUM_HTTP_MAX_SOURCES || options->port < 1 ||
        options->port > 65535 || options->max_clients < 1 || options->max_clients > 256 ||
        options->per_ip < 1 || options->per_ip > 64 || options->requests_per_second < 1 ||
        options->requests_per_second > 1000 || options->timeout_seconds < 1 ||
        options->timeout_seconds > 120 || options->cpu < -1 || options->cpu > 1023 ||
        options->bytes_per_second < 1024 || options->bytes_per_second > 1073741824 ||
        options->max_file_bytes < 1 || options->max_file_bytes > 1073741824 ||
        !terminated(options->bind, sizeof options->bind) ||
        !terminated(options->allow_dirs, sizeof options->allow_dirs) ||
        !terminated(options->allow_extensions, sizeof options->allow_extensions) ||
        !terminated(options->root_extensions, sizeof options->root_extensions) ||
        !terminated(options->deny_prefixes, sizeof options->deny_prefixes) ||
        !*options->allow_extensions)
    {
        return authum_bool_false;
    }
    for (authum_usize_t index = 0; index < options->source_count; ++index)
    {
        if (options->sources[index][0] != '/' ||
            !terminated(options->sources[index], sizeof options->sources[index]))
        {
            return authum_bool_false;
        }
    }
    return authum_bool_true;
}

static authum_http_server_t *
service_open(authum_ptr context, const authum_http_options_t *options, authum_char_t *error,
             authum_usize_t error_size)
{
    authum_http_owner_t *owner = context;
    if (!error)
    {
        error_size = 0;
    }
    if (!owner || !valid_options(options))
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "invalid HTTP options or owner");
        }
        return NULL;
    }
    /* Bound per-owner service state independently of connection limits. */
    authum_usize_t count = 0;
    for (authum_http_server_t *entry = owner->head; entry; entry = entry->next)
    {
        ++count;
    }
    if (count >= 8)
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "HTTP server limit reached for owner");
        }
        return NULL;
    }
    authum_http_server_t *server =
        authum_http_server_create(options, owner->threads, error, error_size);
    if (server)
    {
        server->next = owner->head;
        owner->head = server;
    }
    return server;
}

static void
service_shutdown(authum_ptr context, authum_http_server_t *server)
{
    if (owns_server(context, server))
    {
        authum_http_server_shutdown(server);
    }
}

static void
service_close(authum_ptr context, authum_http_server_t *server)
{
    authum_http_owner_t *owner = context;
    if (!owns_server(owner, server))
    {
        return;
    }
    if (server->drain_depth)
    {
        server->close_pending = authum_bool_true;
        return;
    }
    authum_http_server_t **link = &owner->head;
    while (*link != server)
    {
        link = &(*link)->next;
    }
    *link = server->next;
    authum_http_server_destroy(server);
}

static authum_usize_t
service_drain(authum_ptr owner, authum_http_server_t *server, authum_http_log_fn sink,
              authum_ptr user, authum_usize_t limit)
{
    if (!owns_server(owner, server) || server->close_pending)
    {
        return 0;
    }
    ++server->drain_depth;
    authum_usize_t drained = authum_http_server_drain(server, sink, user, limit);
    --server->drain_depth;
    if (!server->drain_depth && server->close_pending)
    {
        service_close(owner, server);
    }
    return drained;
}

static authum_usize_t
service_memory(authum_ptr owner, const authum_http_server_t *server)
{
    return owns_server(owner, server) ? authum_http_server_memory(server) : 0;
}

void
authum_http_host_init(authum_http_owner_t *owner, const authum_threads_t *threads,
                      authum_http_service_t *service)
{
    static const authum_http_ops_t ops = {sizeof ops,    service_open,  service_shutdown,
                                          service_close, service_drain, service_memory};
    owner->head = NULL;
    owner->threads = threads;
    service->ops = &ops;
    service->owner = owner;
}

void
authum_http_host_drop_owner(authum_http_owner_t *owner)
{
    if (owner)
    {
        while (owner->head)
        {
            service_close(owner, owner->head);
        }
    }
}
