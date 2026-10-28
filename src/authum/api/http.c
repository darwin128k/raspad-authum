#include <authum/http.h>
#include <authum/null.h>
#include <lh/char/map.h>
#include <lh/util/ptr.h>
#include <lh/memory/std.h>
#include <stdio.h>
#include <string.h>

void
authum_http_options_init(authum_http_options_t *options)
{
    lh_memory_std_set(options, 0, sizeof lh_ptr_deref(options));
    options->size = sizeof lh_ptr_deref(options);
    strcpy(options->bind, "127.0.0.1");
    options->port = 17015;
    options->max_clients = 64;
    options->per_ip = 4;
    options->requests_per_second = 20;
    options->timeout_seconds = 15;
    options->cpu = -1;
    options->access_log = authum_bool_true;
    options->bytes_per_second = lh_math_mul(lh_math_mul(8, 1024), 1024);
    options->max_file_bytes = lh_math_mul(lh_math_mul(128, 1024), 1024);
}

authum_bool_t
authum_http_source_add(authum_http_options_t *options, const authum_char_t *path)
{
    if (!options || lh_math_lt(options->size, sizeof lh_ptr_deref(options)) || !path ||
        lh_math_ne(path[0], lh_char_map_slash) ||
        lh_math_ge(options->source_count, AUTHUM_HTTP_MAX_SOURCES) ||
        lh_math_ge(strlen(path), AUTHUM_HTTP_PATH_CAPACITY))
    {
        return authum_bool_false;
    }
    strcpy(options->sources[options->source_count++], path);
    return authum_bool_true;
}

authum_http_server_t *
authum_http_open(const authum_http_service_t *service, const authum_http_options_t *options,
                 authum_char_t *error, authum_usize_t error_size)
{
    if (!service || !service->ops || !AUTHUM_ABI_HAS(service->ops, open) ||
        !AUTHUM_ABI_HAS(service->ops, shutdown) || !AUTHUM_ABI_HAS(service->ops, close))
    {
        if (error && error_size)
        {
            snprintf(error, error_size, "host has no HTTP service; update Authum");
        }
        return authum_null;
    }
    return service->ops->open(service->owner, options, error, error_size);
}

void
authum_http_shutdown(const authum_http_service_t *service, authum_http_server_t *server)
{
    if (service && service->ops && AUTHUM_ABI_HAS(service->ops, shutdown))
    {
        service->ops->shutdown(service->owner, server);
    }
}

void
authum_http_close(const authum_http_service_t *service, authum_http_server_t *server)
{
    if (service && service->ops && AUTHUM_ABI_HAS(service->ops, close))
    {
        service->ops->close(service->owner, server);
    }
}

authum_usize_t
authum_http_drain(const authum_http_service_t *service, authum_http_server_t *server,
                  authum_http_log_fn sink, authum_ptr user, authum_usize_t limit)
{
    return service && service->ops && AUTHUM_ABI_HAS(service->ops, drain)
               ? service->ops->drain(service->owner, server, sink, user, limit)
               : 0;
}

authum_usize_t
authum_http_memory_usage(const authum_http_service_t *service, const authum_http_server_t *server)
{
    return service && service->ops && AUTHUM_ABI_HAS(service->ops, memory_usage)
               ? service->ops->memory_usage(service->owner, server)
               : 0;
}
