#include <authum/placeholders.h>
#include <authum/null.h>

authum_bool_t
authum_placeholder_expand(const authum_placeholder_service_t *service,
                          const authum_char_t *template, authum_char_t *out,
                          authum_usize_t out_size)
{
    if (!service || !service->ops || authum_null_eq((service->ops)->expand) || !template || !out ||
        !out_size)
    {
        return authum_bool_false;
    }
    return service->ops->expand(service->owner, template, out, out_size);
}

authum_bool_t
authum_placeholder_register(const authum_placeholder_service_t *service, const authum_char_t *name,
                            const authum_char_t *value)
{
    if (!service || !service->ops || authum_null_eq((service->ops)->register_placeholder) ||
        !name || !value)
    {
        return authum_bool_false;
    }
    return service->ops->register_placeholder(service->owner, name, value);
}

authum_bool_t
authum_placeholder_unregister(const authum_placeholder_service_t *service,
                              const authum_char_t *name)
{
    if (!service || !service->ops || authum_null_eq((service->ops)->unregister_placeholder) ||
        !name)
    {
        return authum_bool_false;
    }
    return service->ops->unregister_placeholder(service->owner, name);
}
