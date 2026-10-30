#include <authum/host/hooks.h>

#include <authum/unused.h>
#include <lh/os/tick.h>
#include <lh/size.h>
#include <lh/util/ptr.h>

/** The chain every module hooks into. */
static authum_hook_registry_t g_hooks;

authum_hook_registry_t *
authum_hook_registry(void)
{
    return lh_addr_of(g_hooks);
}

void
authum_hook_registry_init(authum_hook_registry_t *self)
{
    authum_usize_t point;

    if (authum_null_eq(self))
    {
        return;
    }
    for (point = 0u; lh_math_lt(point, authum_hook_point_count); point = lh_math_add_one(point))
    {
        self->chains[point].count = 0u;
    }
}

authum_bool_t
authum_hook_registry_add(authum_hook_registry_t *self, authum_ptr owner, authum_hook_point_t point,
                 authum_hook_fn fn, authum_ptr user)
{
    authum_hook_chain_t *chain;

    if (authum_null_eq(self) || authum_null_eq(fn) ||
        lh_math_ge(point, authum_hook_point_count))
    {
        return authum_bool_false;
    }
    chain = lh_addr_of(self->chains[point]);
    if (lh_math_ge(chain->count, authum_hooks_max))
    {
        return authum_bool_false;
    }
    /* The handlers are kept in the order they were hooked; the chain runs them
     * from the last one, so the module that hooked last is called first. */
    chain->handlers[chain->count].fn = fn;
    chain->handlers[chain->count].user = user;
    chain->handlers[chain->count].owner = owner;
    chain->count = lh_math_add_one(chain->count);
    return authum_bool_true;
}

authum_bool_t
authum_hook_registry_remove(authum_hook_registry_t *self, authum_ptr owner, authum_hook_point_t point,
                    authum_hook_fn fn, authum_ptr user)
{
    authum_hook_chain_t *chain;
    authum_usize_t index;

    if (authum_null_eq(self) || authum_null_eq(fn) ||
        lh_math_ge(point, authum_hook_point_count))
    {
        return authum_bool_false;
    }
    chain = lh_addr_of(self->chains[point]);
    for (index = 0u; lh_math_lt(index, chain->count); index = lh_math_add_one(index))
    {
        authum_hook_handler_t *handler = lh_addr_of(chain->handlers[index]);

        if (handler->fn != fn || handler->user != user || handler->owner != owner)
        {
            continue;
        }
        chain->count = lh_math_sub_one(chain->count);
        for (; lh_math_lt(index, chain->count); index = lh_math_add_one(index))
        {
            chain->handlers[index] = chain->handlers[lh_math_add_one(index)];
        }
        return authum_bool_true;
    }
    return authum_bool_false;
}

void
authum_hook_registry_drop_owner(authum_hook_registry_t *self, authum_ptr owner)
{
    authum_usize_t point;

    if (authum_null_eq(self))
    {
        return;
    }
    for (point = 0u; lh_math_lt(point, authum_hook_point_count); point = lh_math_add_one(point))
    {
        authum_hook_chain_t *chain = lh_addr_of(self->chains[point]);
        authum_usize_t index = 0u;

        while (lh_math_lt(index, chain->count))
        {
            if (chain->handlers[index].owner == owner)
            {
                chain->count = lh_math_sub_one(chain->count);
                for (; lh_math_lt(index, chain->count); index = lh_math_add_one(index))
                {
                    chain->handlers[index] = chain->handlers[lh_math_add_one(index)];
                }
                continue;
            }
            index = lh_math_add_one(index);
        }
    }
}

authum_bool_t
authum_hook_registry_call(authum_hook_registry_t *self, authum_hook_point_t point,
                          authum_hook_call_t *call, authum_bool_t *overran)
{
    authum_hook_chain_t *chain;
    authum_usize_t index = 0u;

    if (!authum_null_eq(overran))
    {
        *overran = authum_bool_false;
    }
    if (authum_null_eq(self) || authum_null_eq(call) ||
        lh_math_ge(point, authum_hook_point_count))
    {
        return authum_bool_true;
    }
    chain = lh_addr_of(self->chains[point]);
    for (index = chain->count; lh_math_gt(index, 0u); index = lh_math_sub_one(index))
    {
        authum_hook_handler_t *handler = lh_addr_of(chain->handlers[lh_math_sub_one(index)]);
        lh_usize_t started = lh_os_tick_us();
        authum_bool_t keep = handler->fn(handler->user, call);

        if (lh_math_gt(lh_math_sub(lh_os_tick_us(), started), authum_hook_budget_us))
        {
            if (!authum_null_eq(overran))
            {
                *overran = authum_bool_true;
            }
            authum_hook_registry_remove(self, handler->owner, point, handler->fn, handler->user);
            continue;
        }
        if (!keep)
        {
            break;
        }
    }
    return call->proceed;
}
