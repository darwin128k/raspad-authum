#include <authum/host/hooks.h>

#include <lh/util/ptr.h>

static authum_bool_t
authum_hooks_host_add(authum_ptr owner, const authum_char_t *tag, authum_hook_point_t point,
                      authum_hook_fn fn, authum_ptr user)
{
    return authum_hook_registry_add(authum_hook_registry(), owner, tag, point, fn, user);
}

static authum_bool_t
authum_hooks_host_remove(authum_ptr owner, authum_hook_point_t point, authum_hook_fn fn,
                         authum_ptr user)
{
    return authum_hook_registry_remove(authum_hook_registry(), owner, point, fn, user);
}

static const authum_hook_ops_t authum_hooks_host_ops_table = {
    .add = authum_hooks_host_add,
    .remove = authum_hooks_host_remove,
};

const authum_hook_ops_t *
authum_hooks_host_ops(void)
{
    return lh_addr_of(authum_hooks_host_ops_table);
}
