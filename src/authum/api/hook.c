#include <authum/hook.h>

#include <lh/math/ops.h>

/** The name of each point, in the order of ::authum_hook_point_t. */
static const authum_char_t *const g_hook_point_names[authum_hook_point_count] = {
    "client_connect",
    "client_kill",
    "client_command",
};

const authum_hook_ops_t *
authum_hooks_get_ops(const authum_hooks_t *self)
{
    return authum_null_eq(self) ? authum_null : self->ops;
}

authum_ptr
authum_hooks_get_owner(const authum_hooks_t *self)
{
    return authum_null_eq(self) ? authum_null : self->owner;
}

authum_bool_t
authum_hooks_add(const authum_hooks_t *self, authum_hook_point_t point, authum_hook_fn fn,
                 authum_ptr user)
{
    const authum_hook_ops_t *ops = authum_hooks_get_ops(self);

    if (authum_null_eq(ops) || authum_null_eq(ops->add) || authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    return ops->add(authum_hooks_get_owner(self), point, fn, user);
}

authum_bool_t
authum_hooks_remove(const authum_hooks_t *self, authum_hook_point_t point, authum_hook_fn fn,
                    authum_ptr user)
{
    const authum_hook_ops_t *ops = authum_hooks_get_ops(self);

    if (authum_null_eq(ops) || authum_null_eq(ops->remove) || authum_null_eq(fn))
    {
        return authum_bool_false;
    }
    return ops->remove(authum_hooks_get_owner(self), point, fn, user);
}

const authum_char_t *
authum_hook_point_name(authum_hook_point_t point)
{
    if (lh_math_ge(point, authum_hook_point_count))
    {
        return "-";
    }
    return g_hook_point_names[point];
}
