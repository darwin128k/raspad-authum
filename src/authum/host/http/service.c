#include <authum/host/http/internal.h>
#include <lh/os/system/fs/path.h>
#include <authum/null.h>
#include <lh/core/char/map.h>
#include <lh/core/memory/std.h>
#include <authum/memory.h>

authum_bool_t
authum_http_owner_has_server(const authum_http_owner_t *owner, const authum_http_server_t *server)
{
    if (!owner || !server)
    {
        return authum_bool_false;
    }
    for (const authum_http_server_t *entry = owner->head; entry; entry = entry->next)
    {
        if (lh_math_eq(entry, server))
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

authum_bool_t
authum_http_is_terminated(const authum_char_t *string, authum_usize_t capacity)
{
    return lh_math_ne(lh_str_ptr_find_of_null_terminator_by_size(string, capacity), authum_null);
}

/* Defaults for fields a caller's struct may predate; keep in step with
 * authum_http_options_init. */
void
authum_http_options_fill_defaults(authum_http_options_t *options)
{
    lh_memory_std_set(options, 0, sizeof lh_ptr_deref(options));
    options->struct_size = lh_cast_static(authum_u32_t, sizeof lh_ptr_deref(options));
    options->per_ip_pending = 4;
}

authum_bool_t
authum_http_options_read(const authum_http_options_t *in, authum_http_options_t *out)
{
    authum_http_options_fill_defaults(out);
    if (!in)
    {
        return authum_bool_false;
    }
    /* The old layout starts with the bind text: a valid IPv4 address puts four
     * printable bytes there, so its first word is at least 0x2e000000 in either
     * byte order. A size is far below 0x01000000. */
    const authum_u32_t size = in->struct_size;
    if (lh_math_ge(size, 0x01000000u))
    {
        const authum_http_options_v0_t *old = lh_cast_reinterpret(const authum_http_options_v0_t *, in);
        lh_memory_std_copy(out->bind, old->bind, sizeof out->bind);
        out->port = old->port;
        lh_memory_std_copy(out->sources, old->sources, sizeof out->sources);
        out->source_count = old->source_count;
        lh_memory_std_copy(out->allow_dirs, old->allow_dirs, sizeof out->allow_dirs);
        lh_memory_std_copy(out->allow_extensions, old->allow_extensions,
                           sizeof out->allow_extensions);
        lh_memory_std_copy(out->root_extensions, old->root_extensions, sizeof out->root_extensions);
        lh_memory_std_copy(out->deny_prefixes, old->deny_prefixes, sizeof out->deny_prefixes);
        out->max_clients = old->max_clients;
        out->per_ip = old->per_ip;
        out->requests_per_second = old->requests_per_second;
        out->timeout_seconds = old->timeout_seconds;
        out->cpu = old->cpu;
        out->access_log = old->access_log;
        out->bytes_per_second = old->bytes_per_second;
        out->max_file_bytes = old->max_file_bytes;
        return authum_bool_true;
    }
    if (lh_math_lt(size, AUTHUM_HTTP_OPTIONS_MIN_SIZE))
    {
        return authum_bool_false;
    }
    /* Only the caller's bytes are read; fields past them keep the defaults,
     * and fields of a newer caller that this host does not know are ignored. */
    lh_memory_std_copy(out, in, lh_math_lt(size, sizeof lh_ptr_deref(out)) ? size
                                                                         : sizeof lh_ptr_deref(out));
    out->struct_size = lh_cast_static(authum_u32_t, sizeof lh_ptr_deref(out));
    return authum_bool_true;
}

authum_bool_t
authum_http_options_is_valid(const authum_http_options_t *options)
{
    if (!options || !options->source_count ||
        lh_math_gt(options->source_count, AUTHUM_HTTP_MAX_SOURCES) ||
        lh_math_lt(options->port, 1) || lh_math_gt(options->port, LH_NET_PORT_MAX) ||
        lh_math_lt(options->max_clients, 1) ||
        lh_math_gt(options->max_clients, AUTHUM_HTTP_MAX_CLIENTS) ||
        lh_math_lt(options->per_ip, 1) || lh_math_gt(options->per_ip, 64) ||
        lh_math_lt(options->requests_per_second, 1) ||
        lh_math_gt(options->requests_per_second, 1000) || lh_math_lt(options->timeout_seconds, 1) ||
        lh_math_gt(options->timeout_seconds, 120) || lh_math_lt(options->cpu, -1) ||
        lh_math_gt(options->cpu, 1023) || lh_math_lt(options->bytes_per_second, 1024) ||
        lh_math_gt(options->bytes_per_second, 1073741824) ||
        lh_math_lt(options->max_file_bytes, 1) || lh_math_gt(options->max_file_bytes, 1073741824) ||
        !authum_http_is_terminated(options->bind, sizeof options->bind) ||
        !authum_http_is_terminated(options->allow_dirs, sizeof options->allow_dirs) ||
        !authum_http_is_terminated(options->allow_extensions, sizeof options->allow_extensions) ||
        !authum_http_is_terminated(options->root_extensions, sizeof options->root_extensions) ||
        !authum_http_is_terminated(options->deny_prefixes, sizeof options->deny_prefixes) ||
        !authum_http_is_terminated(options->trusted_proxies, sizeof options->trusted_proxies) ||
        lh_math_lt(options->per_ip_pending, 1) || lh_math_gt(options->per_ip_pending, 64) ||
        lh_math_gt(options->access_log, 2) || !lh_ptr_deref(options->allow_extensions))
    {
        return authum_bool_false;
    }
    for (authum_usize_t index = 0; lh_math_lt(index, options->source_count);
         index = lh_math_add_one(index))
    {
        if (!authum_http_is_terminated(options->sources[index], sizeof options->sources[index]) ||
            !lh_fs_path_is_text_absolute(options->sources[index],
                                         lh_os_system_fs_path_style_native()) ||
            !authum_http_is_terminated(options->sources[index], sizeof options->sources[index]))
        {
            return authum_bool_false;
        }
    }
    return authum_bool_true;
}

authum_http_server_t *
authum_http_service_open(authum_ptr context, const authum_http_options_t *caller,
                         authum_char_t *error, authum_usize_t error_size)
{
    authum_unused(authum_http_host_reap());
    if (!error)
    {
        error_size = 0;
    }
    /* The caller's struct may be from an older module: read it into the
     * current layout (kept off the stack: the struct is about 70 KiB). */
    authum_http_options_t *options = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(options));
    if (!options)
    {
        if (error && error_size)
        {
            authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP options allocation failed"));
        }
        return authum_null;
    }
    authum_http_server_t *server = authum_null;
    if (authum_http_options_read(caller, options))
    {
        server = authum_http_service_open_read(context, options, error, error_size);
    }
    else if (error && error_size)
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size,
                                             "HTTP options struct_size %u is not supported",
                                             caller ? caller->struct_size : 0u));
    }
    authum_memory_free(options);
    return server;
}

authum_http_server_t *
authum_http_service_open_read(authum_http_owner_t *owner, const authum_http_options_t *options,
                              authum_char_t *error, authum_usize_t error_size)
{
    if (!owner || !authum_http_options_is_valid(options))
    {
        if (error && error_size)
        {
            authum_unused(lh_str_ptr_format_cstr(error, error_size, "invalid HTTP options or owner"));
        }
        return authum_null;
    }
    /* Bound per-owner service state independently of connection limits. */
    authum_usize_t count = 0;
    for (authum_http_server_t *entry = owner->head; entry; entry = entry->next)
    {
        count = lh_math_add_one(count);
    }
    if (lh_math_ge(count, 8))
    {
        if (error && error_size)
        {
            authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP server limit reached for owner"));
        }
        return authum_null;
    }
    authum_http_server_t *server = authum_http_server_create(options, owner->tag, error, error_size);
    if (server)
    {
        server->next = owner->head;
        owner->head = server;
    }
    return server;
}

void
authum_http_service_shutdown(authum_ptr context, authum_http_server_t *server)
{
    if (authum_http_owner_has_server(context, server))
    {
        authum_http_server_shutdown(server);
    }
}

void
authum_http_service_close(authum_ptr context, authum_http_server_t *server)
{
    authum_http_owner_t *owner = context;
    if (!authum_http_owner_has_server(owner, server))
    {
        return;
    }
    if (server->drain_depth)
    {
        server->close_pending = authum_bool_true;
        return;
    }
    authum_http_server_t **link = lh_addr_of(owner->head);
    while (lh_math_ne(lh_ptr_deref(link), server))
    {
        link = lh_addr_of((lh_ptr_deref(link))->next);
    }
    lh_ptr_deref(link) = server->next;
    authum_http_server_destroy(server);
}

authum_usize_t
authum_http_service_drain(authum_ptr owner, authum_http_server_t *server, authum_http_log_fn sink,
                          authum_ptr user, authum_usize_t limit)
{
    if (!authum_http_owner_has_server(owner, server) || server->close_pending)
    {
        return 0;
    }
    server->drain_depth = lh_math_add_one(server->drain_depth);
    authum_usize_t drained = authum_http_server_drain(server, sink, user, limit);
    server->drain_depth = lh_math_sub_one(server->drain_depth);
    if (!server->drain_depth && server->close_pending)
    {
        authum_http_service_close(owner, server);
    }
    return drained;
}

authum_usize_t
authum_http_service_get_memory(authum_ptr owner, const authum_http_server_t *server)
{
    return authum_http_owner_has_server(owner, server) ? authum_http_server_memory(server) : 0;
}

void
authum_http_host_init(authum_http_owner_t *owner, authum_http_service_t *service)
{
    static const authum_http_ops_t ops = {.open = authum_http_service_open,
                                          .shutdown = authum_http_service_shutdown,
                                          .close = authum_http_service_close,
                                          .drain = authum_http_service_drain,
                                          .memory_usage = authum_http_service_get_memory};
    owner->head = authum_null;
    owner->tag = authum_null;
    service->ops = lh_addr_of(ops);
    service->owner = owner;
}

void
authum_http_host_drop_owner(authum_http_owner_t *owner)
{
    if (owner)
    {
        while (owner->head)
        {
            authum_http_service_close(owner, owner->head);
        }
    }
    authum_unused(authum_http_host_reap());
}
