#if defined(_WIN32)
#    include <authum/host/http.h>
#    include <stdio.h>

static authum_http_server_t *
unsupported(authum_ptr owner, const authum_http_options_t *options, authum_char_t *error,
            authum_usize_t size)
{
    (void)owner;
    (void)options;
    if (error && size)
    {
        snprintf(error, size, "HTTP transport is currently implemented for Linux only");
    }
    return NULL;
}

void
authum_http_host_init(authum_http_owner_t *owner, const authum_threads_t *threads,
                      authum_http_service_t *service)
{
    static const authum_http_ops_t ops = {.size = sizeof ops, .open = unsupported};
    owner->head = NULL;
    owner->threads = threads;
    service->owner = owner;
    service->ops = &ops;
}

void
authum_http_host_drop_owner(authum_http_owner_t *owner)
{
    (void)owner;
}
#endif
