#if defined(_WIN32)
#    include <authum/host/http.h>
#    include <authum/null.h>
#    include <authum/unused.h>
#    include <lh/util/ptr.h>
#    include <stdio.h>

static authum_http_server_t *
unsupported(authum_ptr owner, const authum_http_options_t *options, authum_char_t *error,
            authum_usize_t size)
{
    authum_unused(owner);
    authum_unused(options);
    if (error && size)
    {
        snprintf(error, size, "HTTP transport is currently implemented for Linux only");
    }
    return authum_null;
}

void
authum_http_host_init(authum_http_owner_t *owner, const authum_threads_t *threads,
                      authum_http_service_t *service)
{
    static const authum_http_ops_t ops = {.open = unsupported};
    owner->head = authum_null;
    owner->threads = threads;
    service->owner = owner;
    service->ops = lh_addr_of(ops);
}

void
authum_http_host_drop_owner(authum_http_owner_t *owner)
{
    authum_unused(owner);
}
#endif
