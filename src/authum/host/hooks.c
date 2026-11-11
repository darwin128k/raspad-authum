/**
 * @file hooks.c
 * @brief The module hook points over lh's hook chains — see hooks.h.
 */

#include <authum/host/hooks.h>
#include <authum/unused.h>
#include <lh/core/assert/static.h>

/** The chain every module hooks into. */
authum_hook_registry_t authum_hook_registry_global;

/* A module's handler is lh's handler itself: the same arguments (the call is
   lh's opaque pointer) and a boolean of the same size and values. */
lh_assert_static(sizeof(authum_bool_t) == sizeof(lh_bool_t),
                 "authum and lh booleans must have the same size");

lh_os_module_hook_t *
authum_hook_registry_get_chain_mut(authum_hook_registry_t *self, authum_hook_point_t point)
{
    return lh_addr_of(self->chains[point]);
}

lh_os_module_hook_cb
authum_hook_registry_get_callback(authum_hook_fn fn)
{
    return lh_cast_reinterpret(lh_os_module_hook_cb, fn);
}

authum_hook_registry_t *
authum_hook_registry(void)
{
    return lh_addr_of(authum_hook_registry_global);
}

void
authum_hook_registry_init(authum_hook_registry_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    for (authum_usize_t point = 0u; lh_math_lt(point, authum_hook_point_count); ++point)
    {
        lh_os_module_hook_init(authum_hook_registry_get_chain_mut(self, point));
    }
}

authum_bool_t
authum_hook_registry_add(authum_hook_registry_t *self, authum_ptr owner, const authum_char_t *tag,
                         authum_hook_point_t point, authum_hook_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) || lh_math_ge(point, authum_hook_point_count))
    {
        return authum_bool_false;
    }
    return lh_os_module_hook_try_add(authum_hook_registry_get_chain_mut(self, point), owner, tag,
                                     authum_hook_registry_get_callback(fn), user);
}

authum_bool_t
authum_hook_registry_remove(authum_hook_registry_t *self, authum_ptr owner,
                            authum_hook_point_t point, authum_hook_fn fn, authum_ptr user)
{
    if (authum_null_eq(self) || authum_null_eq(fn) || lh_math_ge(point, authum_hook_point_count))
    {
        return authum_bool_false;
    }
    return lh_os_module_hook_try_remove(authum_hook_registry_get_chain_mut(self, point), owner,
                                        authum_hook_registry_get_callback(fn), user);
}

void
authum_hook_registry_drop_owner(authum_hook_registry_t *self, authum_ptr owner)
{
    if (authum_null_eq(self))
    {
        return;
    }
    for (authum_usize_t point = 0u; lh_math_lt(point, authum_hook_point_count); ++point)
    {
        lh_os_module_hook_remove_owner(authum_hook_registry_get_chain_mut(self, point), owner);
    }
}

authum_bool_t
authum_hook_registry_call(authum_hook_registry_t *self, authum_hook_point_t point,
                          authum_hook_call_t *call, authum_bool_t *overran)
{
    if (!authum_null_eq(overran))
    {
        lh_ptr_deref(overran) = authum_bool_false;
    }
    if (authum_null_eq(self) || authum_null_eq(call) ||
        lh_math_ge(point, authum_hook_point_count))
    {
        return authum_bool_true;
    }

    lh_bool_t dropped = lh_bool_false;

    /* The handler that stopped the call names its module in the call, so the caller
       names who decided; the verdict itself is the call's proceed. */
    authum_unused(lh_os_module_hook_call(authum_hook_registry_get_chain_mut(self, point), call,
                                         lh_cast_static(lh_u64_t, authum_hook_budget_us),
                                         lh_addr_of(call->decided_by), lh_addr_of(dropped)));
    if (!authum_null_eq(overran))
    {
        lh_ptr_deref(overran) = dropped;
    }
    return call->proceed;
}
